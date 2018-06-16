/*
   PWoT Talker

   This software may be used and distributed according to the terms
   of the GNU Public License, incorporated herein by reference.
*/

#ifndef P_MISC

#define P_MISC

#ifndef time_t
#include <time.h>
#endif /* time_t */

#ifndef IN_MISC_C
#ifndef P_USER
#include "user.h"
#endif /* P_USER */

extern void terminateString(char*);
extern void stringToLower(char*);
extern void stringToUpper(char*);
extern char *durationToString(int, int);
extern char *timeToString(time_t, int);
extern char *wrapText(char*, unsigned int, unsigned int);
extern void notifyBug(char*, userObject, char*);
extern char *verboseSiteName(char*);
extern char *getHostDescription(char*);
extern char *CropString(char*, int);
#endif /* IN_MISC_C */

#endif /* P_MISC */
