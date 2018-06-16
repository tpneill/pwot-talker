/*
   PWoT Talker

   This software may be used and distributed according to the terms
   of the GNU Public License, incorporated herein by reference.
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/utsname.h>
#include <time.h>
#include <fcntl.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <unistd.h>
#include <setjmp.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#define IN_MAIN_C
#include "linkedList.h"
#include "user.h"
#include "room.h"
#include "command.h"
#include "module.h"
#include "misc.h"
#include "socket.h"
#include "syslog.h"
#include "bgTask.h"
#include "baseCommands.h"
#include "event.h"
#include "pwot.h"

#ifdef DMALLOC
  #include "dmalloc.h"
#endif

time_t sysTime;
int killed;

void initGlobal(void)
{
	char realHostname[256]; // config->-ify me now!
	struct utsname un;
	int check, i;

	/* Create the array of Linked Lists  */
	for(i = 0; i < LINKED_LISTS; i++)
		globalLists[i] = newLinkedList();

	/* Set the sorting function for the commands list */
	COMMAND_LIST->lessThan = commandListSort;

	
	writeSyslog(SL_BOOT "** %s v%s starting", SHORT_NAME, VERSION);
#ifndef DEBUG
	printf("** %s v%s starting\n", SHORT_NAME, VERSION);
#endif /* DEBUG */

	if (uname(&un) != 0) {
		writeSyslog(SL_INFO "** This OS doesn't have a userlevel uname() call");
		gethostname((un.nodename), SYS_NMLN);
		strcpy((un.sysname), "unknown");
		strcpy((un.release), "unknown");
		strcpy((un.version), "unknown");
		strcpy((un.machine), "unknown");
	}
	getRealHostname(un.nodename, realHostname);
	writeSyslog(SL_BOOT "Booting on %s (%s %s %s)",
		    realHostname, un.sysname, un.release, un.machine);
	
	writeSyslog(SL_BOOT "Staring on ports %d, %d", ports[0], ports[1]);
#ifndef DEBUG
        printf("Booting on %s (%s %s %s)\n",
                    realHostname, un.sysname, un.release, un.machine);
        printf("Staring on ports %d, %d\n", ports[0], ports[1]);
#endif /* DEBUG */

	if((check = initSockets()) != 0) {
		writeSyslog(SL_BOOT "** Can't bind()/listen() on port %d!", abs(check));
#ifndef DEBUG
		printf("** Can't bind()/listen() on port %d!\n", abs(check));
#endif /* DEBUG */
		exit(-1);
	}

	initRooms();
	initCommands();
	initModules();
	writeSyslog(SL_BOOT "** Started");
#ifndef DEBUG
	printf("** Started\n");
        switch(fork()) {
	case -1:
		fprintf(stderr, "Couldn't fork!\n");
		exit(-1);
	case 0:
		break;
	default:
		/* sleep(1); -- Why do we ever need this? */
		exit(0);
	}
#else
        writeSyslog(SL_DEBUG "Debug mode - not fork()ing");
#endif /* DEBUG */
}

void weirdString(char *str)
{
	static char buffer[4096], sb[12];
	int i;

	sprintf(buffer, SL_DEBUG "Weird:");
	for(i = 0; str[i] != '\0'; i++) {
		sprintf(sb, " %hu", (unsigned short int) str[i]);
		strcat(buffer, sb);
	}
	writeSyslog(buffer);
}

int getArgs(int argc, char **argv)
{
  int option_index = 0;
  int c;
  int tmp[] = {0, 0};

  /* Valid long command line options */
  static struct option long_options[] =
  {
    {"help", 0, 0, 'h'},
    {"version", 0, 0, 'V'},
    {0, 0, 0, 0}
  };

  /* Valid short command line options */
  char short_options[]="hp:w:V";

  /* Loop while there are still more arguments to process */
  while(c = getopt_long(argc, argv, short_options, long_options, &option_index), c != -1) {

    switch (c) {

      /* There is a long option */
      case 0:
        break;

      /* Help - Display help & exit */
      case 'h':
        printf("%s v%s (%s)\n" ,SHORT_NAME, VERSION, EMAIL);
        printf("Usage: %s [-p nnn] [-w nnn]\n\n", argv[0]);
        printf("  -p nnn \t set Main port [7000]\n"
         "  -w nnn \t set Arch port [main port + 1]\n"
         "  -h, --help \t display this help screen and exit\n"
         "  -V, --version  display version information and exit\n\n");
	exit(0);
        break;

      /* Version Information */
      case 'V':
        printf("\nPWoT v%s\n\n", VERSION);
        exit(0);
        break;

      /* Output file */
      case 'p':
	tmp[0] = atoi(optarg);
        if(tmp[0] < 1) {
          printf("** Invalid Port '%s'\n", optarg);
          exit(-1);
        }
        break;

      case 'w':
        tmp[1] = atoi(optarg);
        if(tmp[1] < 1) {
          printf("** Invalid Wiz Port '%s'\n", optarg);
          exit(-1);
        }
        break;
      }
  }

  /* If the Wiz port was not specifically set, make it port + 1 */
  if(tmp[1] == 0 && tmp[0] > 0)
    tmp[1] = tmp[0] + 1;

  if(tmp[0])
    ports[0] = tmp[0];
  else
    ports[0] = 7000;

  if(tmp[1])
    ports[1] = tmp[1];
  else
    ports[1] = 7001;

  /* Return the number of arguments processed */
  return optind;
}


int main(int argc, char **argv)
{
	fd_set mask;
	unsigned int len, i;
	struct timeval timeout;
	char inpstr[1024];
	linkedListObject list, next;
	
	getArgs(argc, argv);
	initGlobal();

	for(;;) {
		/* Set read timeout at 5 seconds */
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;

		sysTime = time(NULL);

		taskCheck();
		eventCheck();

		setupReadmask(&mask);
		if(select(FD_SETSIZE, &mask, 0, 0, &timeout) == -1)
			continue;

		for(i = 0; i < S_PORTS; ++i) {
			if(FD_ISSET(listenSock[i], &mask))
				acceptConnection(listenSock[i], i);
		}

		list = firstObject(USER_LIST);
		killed = 0;
		/* Process user input. There's a lot to get through before we deal
		   with the input ... */
		while(list != NULL) {
			next = list->next;
			
			/* Ignore temp users */
			if(USER(list)->status == US_TEMP) {
				list = next;
				continue;
			}
			
			/* Make sure there's data on the incoming socket first */
			if(!(FD_ISSET(USER(list)->socket, &mask))) {
				list = next;
				continue;
			}

			/* Do the read. If there's an error (read() == 0) remove the user */
			inpstr[0] = '\0';
			if(!(len = read(USER(list)->socket, inpstr, sizeof(inpstr)))) {
				disconnectUser(USER(list), 1);
				list = next;
				continue;
			}
			
			/* Hmmm ... tsk tsk ... */
			if(len > 1020) {
				writeSyslog(SL_INFO "%s may be trying to overrun input buffer",
					    USER(list)->name);
				writeUser(USER(list), "** Your input attempt has been ignored as it was too long.\n");
			}
			
			/* Is it an IAC code? */
			if((unsigned short int)inpstr[0] == 255) {
				inpstr[len] = '\0';
				handleIac(USER(list), inpstr, len);
				list = next;
				continue;
			}
			
			/* Does it begin with a control character? (It shouldn't ...) */
			if(inpstr[0] < 32 && !(strchr("\t\n\r", inpstr[0]))) {
				inpstr[len] = '\0';
				weirdString(inpstr);
				list = next;
				continue;
			}
			
			/* At last - we've accepted the input as valid. Strip the
			   trailing \n or \r\n off the end */
			terminateString(inpstr);
			
			/* Now call the input handler for the user. If this fails
			   with a serious error code (<0) say byebye to the user */
			if(USER(list)->inputCall(USER(list), inpstr) < 0)
				disconnectUser(USER(list), 2);
			
			/* Next! */
			if (killed != 1)
			{
				list = next;
			}
			else
			{
				list = firstObject(USER_LIST);
				killed = 1;
			}		
		}
	}
}
