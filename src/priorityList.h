/*
   PWoT Talker

   This software may be used and distributed according to the terms
   of the GNU Public License, incorporated herein by reference.
*/
#ifndef SB_P_PLIST
#define SB_P_PLIST

#ifndef P_LINKEDLIST
#include "linkedList.h"
#endif /* LINKEDLIST */

#ifndef IN_PLIST

/* Return a new linkedListObject with priority 'p'. */
extern linkedListObject newPlist(void *object, int p);


/* Return a new linkedListObject. */
extern linkedListObject newList(void *object);


/* Add 'object' to the end of the linked list. */
extern linkedListObject listAdd(linkedListObject llo, void *object);


/* Insert 'object' in the list so that all objects after it have a 
 * lower priority than 'p'. */
extern linkedListObject plistAdd(linkedListObject llo, void* object , int p);


/* Find the start of the linked list that contains 'llo'.*/ 
extern linkedListObject listFirst(linkedListObject llo);


/* Find the end of the linked list that contains 'llo'.*/
extern linkedListObject listLast(linkedListObject llo);


/* Find the first linkedListObject with priority 'p' in the linked list 
 * that contains 'llo'. (starts searching from 'listFirst(llo)') */
extern linkedListObject plistFirstAt(linkedListObject llo, int p);


/* Find the last linkedListObject with priority 'p' in the linked list 
 * that contains 'llo'. (searches back from 'listlast(llo)') */
extern linkedListObject plistLastAt(linkedListObject llo, int p);


/* Remove the linkedListObject from it's list. 
 * **  N.B. this doesn't free 'llo'.. it returns it! ** */
extern linkedListObject listRemove(linkedListObject llo);


/* Remove the linkedListObject with object 'object' from 'llo'.
 * ** N.B. this doesn't free the linkedListObject.. it returns it! ** */
extern linkedListObject listRmObject(linkedListObject llo, void *object);


#endif /* IN_PLIST */
#endif /* SB_P_PLIST */
