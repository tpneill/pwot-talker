/*
   PWoT Talker

   This software may be used and distributed according to the terms
   of the GNU Public License, incorporated herein by reference.
*/

#ifndef P_LINKEDLIST
#define P_LINKEDLIST

/* 
 * PLEASE list the linked lists here.....
 *
 * 0 - User
 * 1 - Room
 * 2 - Command
 * 3 - Module   
 * 4 - Background Task
 * 5 - Events
 */
        
#define LINKED_LISTS 6


struct linkedListStruct {
        signed int (*lessThan)();
        signed int doDuplicateChecks;
        struct linkedListObjStruct *first;
};

typedef struct linkedListStruct* linkedList;


struct linkedListObjStruct {
	void *object;
	struct linkedListObjStruct *prev, *next;
};

typedef struct linkedListObjStruct* linkedListObject;


#ifndef IN_LINKEDLIST_C

extern linkedList globalLists[];


extern linkedList 	newLinkedList(void);
extern linkedListObject	addObject(linkedList, void*);
extern int		inList(linkedList, void*);
extern void* 		deleteObject(linkedList, void*);
extern linkedListObject	firstObject(linkedList);

#endif /* IN_LINKEDLIST_C */

#endif
