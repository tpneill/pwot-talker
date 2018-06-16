/*
   PWoT Talker

   This software may be used and distributed according to the terms
   of the GNU Public License, incorporated herein by reference.
*/

#ifndef P_USER

#define P_USER

#ifndef P_ROOM
#include "room.h"
#endif /* P_ROOM */

#ifndef P_REVBUF
#include "revbuf.h"
#endif /* P_REVBUF */

//#define USER_LIST 0
#define USER_LIST globalLists[0]

#define U_REVTELL 10
#define U_NORMLEN 80
#define U_BIGLEN  256
struct userStruct {
	char name[U_NORMLEN];        /* User's login name */
	char passwd[U_NORMLEN];      /* Password - crypt()'d of course */
	char siteIp[4];              /* User's numerical IP (one char per quad) */
	char siteHostname[U_BIGLEN]; /* User's hostname (if available, else textual IP) */
	char verboseSite[U_BIGLEN];  /* Description of user's site */
	char email[U_BIGLEN];        /* E-mail address */
	char url[U_BIGLEN];          /* Homepage address */
	char desc[U_BIGLEN];         /* Short description */
	char lastTell[U_NORMLEN];    /* Last user they talked to privately */
	char afkMessage[U_BIGLEN];   /* AFK Message */
	char siteDesc[U_BIGLEN];     /* Description of their current site */
	char termType[U_BIGLEN];     /* Term type */
	unsigned char currentIac;             /* If != '\0', we're in the middle of an IAC SB */
	unsigned char currentIacSb[U_BIGLEN]; /* Buffer for IAC SB */
	unsigned int currentIacPos;           /* Position in IAC SB buffer */
	signed int (*inputCall)();            /* Function to call with user input */
        unsigned int status;                  /* Current status */
	unsigned int socket;                  /* Socket filehandle */
	unsigned int lastLogin;               /* Last login start time */
	unsigned int lastLoginDuration;       /* Last login duration */
	unsigned int loginTime;               /* Current login time */
	unsigned int totalTime;               /* Total login time */
	unsigned int prompt;                  /* Prompt flags */
	unsigned int flags;                   /* User status flags */
	unsigned int level;                   /* User level */
	unsigned int lastInput;               /* Time of last user input */
	unsigned int inputsPerSec;            /* Number of inputs/sec for user */
	unsigned int screenWidth;             /* User's screen width */
	unsigned int screenHeight;            /* User's screen height */
	unsigned int sex;		      /* User's sex */
	void *data;                           /* Misc. data buffer pointer */
	roomObject room;                      /* User's current room */
	revbufObject revbuf;                  /* The review buffer */
	char *profile;			      /* Users Profile */
	/* messageObject message; */               /* Message buffer for composition/reading */
};
typedef struct userStruct* userObject;

#define USER(a) ((userObject)a->object)

#define US_TEMP  0
#define US_LOGIN 1
#define US_NORM  2
#define US_BUSY  3
#define US_AFK   4
#define US_EDIT 6

#define UL_NEW  0
#define UL_USER 1
#define UL_ARCH 2
#define UL_GOD  3

#define MALE    1
#define FEMALE  2
#define UNKNOWN 0

#ifdef USERLEVELS
static char *userLevels[] = {
	"New", "User", "Arch", "God"
};
#define safeUserLevel(a) (((a>=UL_NEW)&&(a<=UL_GOD))?userLevels[a]:"Unknown")
#endif /* USERLEVELS */

#ifndef IN_USER_C
extern userObject newUser(void);
extern void deleteUser(userObject);
extern int userMatch(char*);
extern userObject getUser(char*);
extern signed int loadUser(userObject);
extern signed int saveUser(userObject);
extern signed int pushInputCall(userObject, signed int(*call)(), int);
extern signed int popInputCall(userObject);
extern void userPrompt(userObject user);

#endif /* IN_USER_C */

#endif /* P_USER */
