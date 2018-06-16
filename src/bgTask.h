/*
   PWoT Talker

   This software may be used and distributed according to the terms
   of the GNU Public License, incorporated herein by reference.
*/

#ifndef P_BGTASK

#define P_BGTASK

#ifndef P_USER
#include "user.h"
#endif /* P_USER */

//#define BGTASK_LIST 4
#define BGTASK_LIST globalLists[4]

#define T_NORMLEN 256
#define T_BIGLEN 512
struct bgTaskStruct {
	char cmdCall[T_BIGLEN];  /* Text of command call */
	char tmpFile[T_NORMLEN]; /* Filename of return temp file */
	int tmpFileHandle;                /* Created by open() to tmpFile */
	pid_t taskPid;                    /* PID of background task */
	userObject callingUser;           /* Calling user */
	void (*taskCallback)();           /* To be called when task returns */
};
typedef struct bgTaskStruct* bgTaskObject;

#define BGTASK(a) ((bgTaskObject)a->object)
extern unsigned int spawnedTotal, spawnedNow;
extern signed int spawnTask(userObject, char*, char**, void (*callback)());
extern void genericCallback(bgTaskObject, char*, unsigned int);
extern void dontCareCallback(bgTaskObject, char*, unsigned int);
extern void taskCheck(void);
#ifndef IN_BGTASK_C

#endif /* IN_BGTASK_C */

#endif /* P_BGTASK */
