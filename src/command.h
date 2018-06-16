/*
   PWoT Talker

   This software may be used and distributed according to the terms
   of the GNU Public License, incorporated herein by reference.
*/

#ifndef P_COMMAND

#define P_COMMAND

//#define COMMAND_LIST 2
#define COMMAND_LIST globalLists[2]

#define C_NORMLEN 12
struct commandStruct {
	char name[C_NORMLEN];        /* Command name */
	signed int (*call)();                 /* Command call */
	unsigned int level;                   /* Allowable call level */
        unsigned int defLevel;                /* Default level */
        unsigned int unmovable;               /* Unmovable level flag */
};
typedef struct commandStruct* commandObject;

#define COMMAND(a) ((commandObject)a->object)

#ifndef IN_COMMAND_C
extern commandObject newCommand(char*);
extern void deleteCommand(commandObject);
extern commandObject getCommand(char*, userObject);
extern commandObject makeCommand(char*, int, int (*call)(), int);
extern int commandInputCall(userObject, char*);
extern int commandListSort(commandObject, commandObject);
#endif /* IN_COMMAND_C */

#endif /* P_COMMAND */
