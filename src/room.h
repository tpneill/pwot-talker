/*
   PWoT Talker

   This software may be used and distributed according to the terms
   of the GNU Public License, incorporated herein by reference.
*/

#ifndef P_REVBUF
#include "revbuf.h"
#endif

#ifndef P_ROOM

#define P_ROOM

//#define ROOM_LIST 1
#define ROOM_LIST globalLists[1]

#define R_REVBUF  20
#define R_NORMLEN 80
#define R_BIGLEN  256
struct roomStruct {
	char name[R_NORMLEN];        /* Room name */
	char label[R_NORMLEN];       /* Short description */
	char topic[R_BIGLEN];        /* Room topic */
        char *description;           /* Long room description */
	revbufObject revbuf;      	      /*  buffer of room activity */
	unsigned int access;                  /* Level required to enter room */
        unsigned int exit;                    /* Level required to leave room */
	unsigned int lock;                    /* Topic lock level */
	short int defaultRoom;                /* Is this the default room (i.e. main */
};
typedef struct roomStruct* roomObject;

#define ROOM(a) ((roomObject)a->object)

#ifndef IN_ROOM_C
extern roomObject newRoom(void);
extern void deleteRoom(roomObject);
extern roomObject getRoom(char*);
extern roomObject getDefaultRoom(void);
extern roomObject loadRoom(char*);
extern int saveRoom(roomObject);
extern void writeRoom(roomObject, char*);
extern void writeRoomExcept();
extern void writeRoomExceptTwo();
extern void writeRoomAbove(roomObject, char*, int);
extern void writeRoomBelow(roomObject, char*, int);
extern int initRooms(void);
#endif /* IN_ROOM_C */

#endif /* P_ROOM */
