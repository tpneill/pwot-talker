/*
   PWoT Talker

   This software may be used and distributed according to the terms
   of the GNU Public License, incorporated herein by reference.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/utsname.h>
#include <netdb.h>
#include <sys/times.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <crypt.h>
#define TELOPTS
#define TELCMDS
#include <arpa/telnet.h>
#undef TELOPTS
#undef TELCMDS
#include <setjmp.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#define IN_SOCKET_C
#include "socket.h"
#include "linkedList.h"
#include "user.h"
#include "room.h"
#include "command.h"
#include "colour.h"
#include "syslog.h"
#include "misc.h"
#include "pwot.h"

#ifdef DMALLOC
  #include "dmalloc.h"
#endif

unsigned int listenSock[S_PORTS];
unsigned int ports[S_PORTS];
unsigned int socketError;

/* This is pretty nasty, but efficient. Just
   set this hook to something when you want
   niceLoginThings() to call some nonstandard
   code ... like a modularised mail system
   for example. This is a quick hack - it
   will eventually become a stack of hooks
   which can be inserted and popped at any
   time. Just don't get used to it. Oh, and
   for the moment remember to reset it to NULL
   when your module unloads. */
void (*loginHook)(userObject user);


int initSockets(void)
{
	struct sockaddr_in address;
	struct timeval tm;
	int i, size = sizeof(struct sockaddr_in), on = 1;

	loginHook = NULL;

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;

	tm.tv_sec = 60;
	tm.tv_usec = 0;

	for(i = 0; i < S_PORTS; i++) {
		if((listenSock[i] = socket(AF_INET, SOCK_STREAM, 0)) == -1)
			return -(ports[i]);

		setsockopt(listenSock[i], SOL_SOCKET, SO_REUSEADDR, (char*) &on, sizeof(on));
		setsockopt(listenSock[i], SOL_SOCKET, SO_LINGER, (char*) &tm, sizeof(tm));

		address.sin_port = htons(ports[i]);

		if(bind(listenSock[i], (struct sockaddr*) &address, size) == -1)
			return -(ports[i]);

		if(listen(listenSock[i], 10) == -1)
			return -(ports[i]);

		fcntl(listenSock[i], F_SETFL, O_NDELAY);
		fcntl(listenSock[i], F_SETFD, (long)0);
	}

	return 0;
}

void socketWriteTimeout(int sig)
{
	if(sig == SIGALRM)
		socketError = 1;
	return;
}

int writeToSocket(int sock, char *buf, int len)
{
	int sent;

	socketError = 0;

	signal(SIGALRM, socketWriteTimeout);
	alarm(10);

	sent = write(sock, buf, len);

	alarm(0);
	signal(SIGALRM, SIG_IGN);

	if(socketError != 0) {
		socketError = 0;
		return -1;
	}

	/* Note : sent passes back errors from write() ... */
	return sent;
}

int writeToUser(userObject user, const char *str, ...)
{
	char buffer[4096];
	va_list ap;

	va_start(ap, str);
#ifdef HAS_VSNPRINTF
	vsnprintf(buffer, 4095, str, ap);
#else
	vsprintf(buffer, str, ap);
#endif
	va_end(ap);

	return writeToSocket(user->socket, buffer, strlen(buffer));
}

int writeUser(userObject user, const char *str, ...)
{
	char buffer[4096], *out;
	va_list ap;

	va_start(ap, str);
#ifdef HAS_VSNPRINTF
	vsnprintf(buffer, 4095, str, ap);
#else
	vsprintf(buffer, str, ap);
#endif
	va_end(ap);

/*	We don't support checking if user has mono-only console yet ...

	if(user->colour == 0)
		out = monoiseString(buffer);
	else
*/
		out = colouriseString(buffer);

	return writeToSocket(user->socket, out, strlen(out));
}

void setupReadmask(fd_set *mask)
{
	linkedListObject list;
	int i;

	FD_ZERO(mask);
	for(i = 0; i < S_PORTS; i++)
		FD_SET(listenSock[i], mask);

	for(list = firstObject(USER_LIST); list != NULL; list = list->next)
		FD_SET(USER(list)->socket, mask);
}

void writeUserIacs(userObject user, int len, int c, ...)
{
	char buffer[256];
	va_list ap;
	int p, val;

	va_start(ap, c);
	for(p = 0; (p < len)&&(p < 254); p++) {
		val = (p==0)?c:va_arg(ap, int);
		buffer[p] = (char) val;
	}
	va_end(ap);

	writeToSocket(user->socket, buffer, len);
}

char *getSiteName(struct sockaddr_in address)
{
	static char siteName[256];
	struct hostent *h;

	if((h = gethostbyaddr((char*)&address.sin_addr, 4, AF_INET)) == NULL)
		strcpy(siteName, (char*)inet_ntoa(address.sin_addr));
	else
		strcpy(siteName, h->h_name);

	return siteName;
}

void disconnectUser(userObject user, int reason)
{
	writeSyslog(SL_LOGIN "** Logout: %s (%d)", user->name, reason);
	writeUser(user, "\n\nLogging out..\n\n");
	close(user->socket);
	deleteUser(user);
}

void niceLoginThings(userObject user)
{
	char buffer[256], *nm;

	writeUser(user, "\n\n");
	writeSyslog(SL_LOGIN "** Login(2): %s [%s]", user->name, user->siteHostname);

	if(fcntl(user->socket, F_SETFD, (long)0) != 0)
		writeSyslog(SL_DEBUG "** Couldn't disable the close-on-exec flag for %s", user->name);

	sprintf(buffer, "** ~FG~OLLOGIN~RS: %s %s ~RS\n", user->name, user->desc);
	writeRoomExcept(NULL, buffer, user);
	
	user->status = US_NORM;
	user->room = getDefaultRoom();
	user->lastLogin = time(0);
	user->loginTime = 0;
	if((nm = getHostDescription(user->siteHostname)))
		strcpy(user->verboseSite, nm);
	else
		strcpy(user->verboseSite, "Unknown location");
	strcpy(user->lastTell, "(no-one)");
	writeUser(user, "\nWelcome %s...\n\n", user->name);
	commandInputCall(user, ".look");
	
	/* See the comments at the top of the code */
	if(loginHook)
		loginHook(user);
}

int loginGetUsername();

int loginVerifyPassword(userObject user, char *input)
{
	char salt[3], check[30];
	int len;

	writeUserIacs(user, 3, IAC, WONT, TELOPT_ECHO);

	len = strlen(input);
	salt[0] = user->passwd[0];
	salt[1] = user->passwd[1];
	salt[2] = '\0';
	strcpy(check, crypt(input, salt));
	
	for(len = 0; len < 24; input[(len++)] = '\0');
	
	if(strcmp(user->passwd, check) == 0) {
		niceLoginThings(user);
		user->inputCall = commandInputCall;
		return 1;
	} else {
		writeUser(user, "Passwords don't match.\n\nEnter your name : ");
		user->inputCall = loginGetUsername;
		return 1;
	}

	return 0;
}

int loginGetPassword(userObject user, char *input)
{
	char salt[3], check[30], buffer[256];
	unsigned int len, otherSocket;
	userObject otherUser;

	writeUserIacs(user, 3, IAC, WONT, TELOPT_ECHO);

	if(user->passwd[0] == '*' && user->passwd[1] == '\0') {
		len = strlen(input);

		if(len == 0) {
			writeUser(user, "Login incorrect.\n\nEnter your name : ");
			user->inputCall = loginGetUsername;
			return 1;
		} else if(len < 4) {
			writeUserIacs(user, 3, IAC, WILL, TELOPT_ECHO);
			writeUser(user, "\nPassword too short. Try again : ");
			return 1;
		} else if(len > 12) {
			writeUserIacs(user, 3, IAC, WILL, TELOPT_ECHO);
			writeUser(user, "\nPassword too long. Try again : ");
			return 1;
		} else if(strncasecmp(user->name, input, len) == 0) {
			writeUserIacs(user, 3, IAC, WILL, TELOPT_ECHO);
			writeUser(user, "\nPassword blatantly obvious. Try again : ");
			return 1;
		}

		salt[0] = (65+(int)(26.0*rand()/(RAND_MAX+1.0)));
		salt[1] = (65+(int)(26.0*rand()/(RAND_MAX+1.0)));
		salt[2] = '\0';

		strcpy(user->passwd, crypt(input, salt));
		for(len = 0; len < 24; input[(len++)] = '\0');

		writeUserIacs(user, 3, IAC, WILL, TELOPT_ECHO);
		writeUser(user, "\nPlease enter the password again : ");
		user->inputCall = loginVerifyPassword;

		return 1;
	} else {
		salt[0] = user->passwd[0];
		salt[1] = user->passwd[1];
		salt[2] = '\0';
		strcpy(check, crypt(input, salt));

		for(len = 0; len < 24; input[(len++)] = '\0');

		if(strcmp(user->passwd, check) == 0) {
			otherUser = getUser(user->name);
			if(otherUser && (otherUser != user)) {
				writeUser(otherUser, "** Another login in your name. Switching sessions ...\n");
				otherSocket = otherUser->socket;
				otherUser->socket = user->socket;
				strcpy(otherUser->siteHostname, user->siteHostname);
				strcpy(otherUser->termType, user->termType);
				otherUser->screenWidth = user->screenWidth;
				otherUser->screenHeight = user->screenHeight;
				close(otherSocket);
				deleteUser(user);
				writeSyslog(SL_LOGIN "** Session swap: %s [%s]", otherUser->name, otherUser->siteHostname);
				sprintf(buffer, "** ~FY~OLSESSION SWAP~RS: %s %s ~RS\n", otherUser->name, otherUser->desc);
				writeRoomExcept(NULL, buffer, otherUser);
				writeUser(otherUser, "\n\n");
				commandInputCall(otherUser, ".look");
			} else {
				user->status = US_NORM;
				user->inputCall = commandInputCall;
				niceLoginThings(user);
			}
			return 1;
		} else {
			writeUser(user, "Login incorrect.\n\nEnter your name : ");
			user->inputCall = loginGetUsername;
			return 1;
		}
	}
	return 0;
}
 

int loginGetUsername(userObject user, char *input)
{
	char buffer[256];

	if(strlen(input) < 3) {
		if(strcasecmp(input, "who") == 0)
			commandInputCall(user, ".who");
		else
			writeUser(user, "Name too short. (Min 3 characters)\n\nEnter your name : ");
		return 1;
	}

	/* Check if name's longer than 12 chars */
	if(strlen(input) > 12) {
		writeUser(user, "Name too long. (Max 12 characters)\n\nEnter your name : ");
		return 1;
	}

	strcpy(user->name, input);
	user->name[0] = toupper(input[0]);

	sprintf(buffer, "** ~OL~FYLOGIN 1~RS: %s [%s]\n", user->name, user->siteHostname);
	writeRoomAbove(NULL, buffer, UL_USER);
	writeSyslog(SL_LOGIN "** Login(1): %s [%s]", user->name, user->siteHostname);

	if(loadUser(user) < 0) {
		writeUser(user, "New user ...\n");
	        user->passwd[0] = '*';
		user->passwd[1] = '\0';
		user->lastLogin = time(0);
		strcpy(user->desc, "is a new user");
		strcpy(user->email, "Unset");
		strcpy(user->url, "Unset");
		user->prompt = 1;
	}
	user->status = US_LOGIN;

	writeUserIacs(user, 3, IAC, WILL, TELOPT_ECHO);
	writeUser(user, "Enter password : ");
	user->inputCall = loginGetPassword;
	return 1;
}

int acceptConnection(int socket, int num)
{
	userObject user;
	struct sockaddr_in address;
	int a_sock, size = sizeof(struct sockaddr_in);

	if((user = newUser()) == NULL)
		return -1;

	a_sock = accept(socket, (struct sockaddr*)&address, &size);
	user->socket = a_sock;
	strcpy(user->siteHostname, getSiteName(address));
	memcpy(user->siteIp, &address.sin_addr, 4);

	/* if(isBanned(user->siteName, DOMAIN_BAN)) disconnectUser(); */

	/* if(socket == listenSock[NORMAL_PORT]) { */
	        writeSyslog(SL_LOGIN "Connect from %s [%hu.%hu.%hu.%hu]", user->siteHostname,
			    user->siteIp[0], user->siteIp[1], user->siteIp[2], user->siteIp[3]);
		user->status = US_LOGIN;
		user->inputCall = loginGetUsername;
		/* writeFileToUser(u, config->loginBanner); */
		writeUserIacs(user, 3, IAC, DO, TELOPT_TTYPE);
		writeUserIacs(user, 3, IAC, DO, TELOPT_NAWS);
		writeUserIacs(user, 3, IAC, DO, TELOPT_NEW_ENVIRON);
		writeUser(user, "\n\n%s v%s\n\nEnter your name : ", SHORT_NAME, VERSION);
	/* } */
		
        return 0;
}

void getRealHostname(char *old, char *new)
{
	struct hostent *h;

	if((h = gethostbyname(old)) == NULL)
		strcpy(new, old);
	else
		strcpy(new, h->h_name);
}

/*
  -- Beginning of IAC stuff --

  This whole section below handles all the IAC commands.
  Its important that when sending IACs that you use
  the writeUserIacs() function - IAC commands can
  contain the \0 character which will break the
  normal writeUser() function as it uses strlen()
  to work out what to send out.

  You have been warned.
*/

char *translateWWDD(int wwdd)
{
	switch(wwdd) {
	case WILL: return "WILL";
	case WONT: return "WONT";
	case DO:   return "DO";
	case DONT: return "DONT";
	}
	return "<ERR>";
}

int invertIacState(int wwdd)
{
	switch(wwdd) {
	case WILL: return WONT;
	case WONT: return WILL;
	case DO:   return DONT;
	case DONT: return DO;
	}
	return DONT;
}

int answerIacState(int wwdd, int inv)
{
	switch(wwdd) {
	case WILL: return (inv)?DONT:DO;
	case WONT: return (inv)?DO:DONT;
	case DO:   return (inv)?WONT:WILL;
	case DONT: return (inv)?WILL:WONT;
	}
	return WONT;
}

int invertAnswerIac(int wwdd)
{
	return invertIacState(answerIacState(wwdd, 1));
}

int handleIacSub(userObject user, char *input)
{
	char *end;

	switch(*input) {
	case TELOPT_NAWS:
		input++;
		user->screenWidth = *input << 8;
		user->screenWidth |= *(input + 1);
		user->screenHeight = *(input + 2) << 8;
		user->screenHeight |= *(input + 3);

		/* Sanity check - some broken clients always report broken values */
		if(user->screenWidth < 3)
			user->screenWidth = 80;
		if(user->screenHeight < 2)
			user->screenHeight = 23;

		break;

	case TELOPT_TTYPE:
		input++;
		if(*input == 0)
			input++;
		if(!(end = strchr(input, 255))) {
			writeSyslog(SL_IAC "IAC: TELCMD SB TTYPE : broken for %s", user->name);
			strcpy(user->termType, "vt100");
			return 0;
		} else {
			*end = '\0';
			writeSyslog(SL_IAC "IAC: TELCMD SB TTYPE : got '%s'", input);
			strcpy(user->termType, input);
		}
		break;

	case TELOPT_NEW_ENVIRON:
		writeSyslog(SL_IAC "IAC: TELCMD SB NEW-ENVIRON [ignored]");
		break;

	default:
		writeSyslog(SL_IAC "IAC: TELCMD SB %s (%hu) [unhandled]", TELOPT(*input),
			    (unsigned short int) *input);
		break;
	}
	return 1;
}

int handleWWDD(userObject user, int val, int wwdd)
{
	if(TELOPT_OK(val)) {
		switch(val) {
		case TELOPT_TM:
			writeSyslog(SL_IAC "IAC: TELOPT Timing Mark - Responding to user %s", user->name);
			writeUserIacs(user, 3, IAC, answerIacState(wwdd, 0), TELOPT_TM);
			break;

		case TELOPT_NAWS:
			if(wwdd != WILL) {
				user->screenWidth = 80;
				user->screenHeight = 23;
			}
			break;
			    
		case TELOPT_TTYPE:
			writeSyslog(SL_IAC "IAC: TELOPT TTYPE - %s", translateWWDD(wwdd));
			if(wwdd == WILL)
				writeUserIacs(user, 6, IAC, SB, TELOPT_TTYPE, TELQUAL_SEND, IAC, SE);
			else
				strcpy(user->termType, "unknown");
			break;

		case TELOPT_NEW_ENVIRON:
			/* Notes about TELOPT_NEW_ENVIRON :

			   The RFC states that this can be used to pass back
			   any environment variable. This is a huge security
			   risk and so the telnet client tends to block most
			   useful variables like USER. About the only useful
			   thing most clients will let us have is the DISPLAY
			   env. I'll implement more of this command if I find
			   that there's other USEFUL envs that can be returned. */
#define SEND 1
#define VAR 0
#define USERVAR 3
			writeSyslog(SL_IAC "IAC: TELOPT NEW-ENVIRON - %s", translateWWDD(wwdd));
			break;
#undef SEND
#undef VAR
#undef USERVAR

		default:
			writeSyslog(SL_IAC "IAC: TELOPT - %s %s (%d)", translateWWDD(wwdd), TELOPT(val), val);
			break;
		}
	} else
		writeSyslog(SL_IAC "IAC: Other (didn't expect) - %d", val);

	return 1;
}

int handleIac(userObject user, unsigned char *input, unsigned int len)
{
	unsigned int pos;

	for(pos = 0; pos < len; pos++) {
		if(input[pos] == IAC) {
			pos++;
			if(!TELCMD_OK(input[pos]))
				break;
			switch(input[pos]) {
			case WILL:
			case WONT:
			case DO:
			case DONT:
				handleWWDD(user, input[pos+1], input[pos]);
				break;
			case SUSP:
				writeSyslog(SL_IAC "IAC: SUSP - Send user %s AFK", user->name);
				break;
			case IP:
				writeSyslog(SL_IAC "IAC: IP - User %s being nasty", user->name);
				break;
			case AYT:
				writeSyslog(SL_IAC "IAC: AYT - Responding to user %s", user->name);
				writeUser(user, "\n[yes]\n");
				break;
			case SB:
				user->currentIac = SB;
				break;
			case SE:
				if(user->currentIac == SB) {
					user->currentIacSb[(user->currentIacPos++)] = IAC;
					user->currentIacSb[(user->currentIacPos++)] = SE;
					handleIacSub(user, user->currentIacSb);
				} else
					writeSyslog(SL_IAC "IAC: SE - End of sub without start for %s",
						    user->name);
				user->currentIac = user->currentIacPos = 0;
				break;
			default:
				writeSyslog(SL_IAC "IAC: TELCMD - %s (%hu)",
					    TELCMD(*input), (unsigned short int) input[pos]);
				break;
			}
		} else {
			switch(user->currentIac) {
			case 0:
				break;
			case SB:
				user->currentIacSb[user->currentIacPos] = input[pos];
				user->currentIacPos++;
				if(user->currentIacPos > (U_BIGLEN-4)) {
					writeSyslog(SL_IAC "IAC: SB - buffer exceeded. Ending subnegotiaion!");
					user->currentIacSb[(user->currentIacPos++)] = IAC;
					user->currentIacSb[(user->currentIacPos++)] = SE;
					handleIacSub(user, user->currentIacSb);
					user->currentIac = user->currentIacPos = 0;
				}
				break;
			default:
				writeSyslog(SL_IAC "socket.c::handleIac() - No IAC, got %hu",
					    (unsigned short int) input[pos]);
				break;
			}
		}
	}

	return 1;
}
