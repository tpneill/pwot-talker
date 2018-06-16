/*
   PWoT Talker

   This software may be used and distributed according to the terms
   of the GNU Public License, incorporated herein by reference.
*/

#include <stdio.h>
#include <stdlib.h>
#define IN_LINKEDLIST_C
#include "linkedList.h"

#ifdef DMALLOC
  #include "dmalloc.h"
#endif

linkedList globalLists[LINKED_LISTS];

/* Set wether duplicate items should be checked for and rejected
   if another identical item exists ** CURRENTLY NOT USED ** */
void setDuplicateChecking(linkedList list, int val)
{
	list->doDuplicateChecks = (val == 0) ? 0 : 1 ;
}


/* Make a new list and set all items appropriatly */
linkedList newLinkedList(void)
{
	linkedList newList = NULL;

	if((newList = (linkedList)malloc(sizeof(struct linkedListStruct))) == NULL) { 
		return NULL;
	}

	/* Set default Values */
	newList->first = NULL;
	newList->lessThan = NULL;
	newList->doDuplicateChecks = 0;
	
	return newList;
}

linkedListObject addObject(linkedList list, void *object)
{
	linkedListObject currentItem, tmpItem, newItem = NULL;

	/* Check we didn't get any invalid input */
	if((list == NULL) || (object == NULL)) {
		return NULL;
	}

        
	/* Check if the item already exists */
	if(list->doDuplicateChecks != 0) {
		if(inList(list, object))
			return NULL;
	}

	/* Create a new ListObject */
	if((newItem = (linkedListObject)malloc(sizeof(struct linkedListObjStruct))) == NULL) {
		return NULL;
	}
	newItem->object = object;

	/* If this is the 1st item in the list just add it straight away */
	if(list->first == NULL) {
		list->first = newItem;
		newItem->next = NULL;
		newItem->prev = NULL;
		return newItem;
	}

	currentItem = list->first;

	/* If there is no compare function then add it to the end of the list */
	if(list->lessThan == NULL) {
		/* Find the end of the list */
		while(currentItem->next)
			currentItem = currentItem->next;

		currentItem->next = newItem;
		newItem->next = NULL;
		newItem->prev = currentItem;

		return newItem;
	}

	/* See if the item should be inserted first in the list */               
	if(list->lessThan(newItem->object, currentItem->object)) {
		newItem->next = currentItem;
		newItem->prev = NULL;
		currentItem->prev = newItem;
		list->first = newItem;
		return newItem;
	}

	/* Otherwise loop till the correct position is found or the
	   end of the list is reached */
	while(currentItem->next && !list->lessThan(newItem->object, currentItem->next->object))
		currentItem = currentItem->next;

	/* Insert it directly AFTER currentItem */
	tmpItem = currentItem->next; 

	currentItem->next = newItem;
	newItem->prev = currentItem;
	newItem->next = tmpItem;

	/* Don't try to set the following item if it doesn't exist */
	if(tmpItem)
		tmpItem->prev = newItem;
		
	return newItem;
}


int inList(linkedList list, void *object)
{
	linkedListObject check;

	/* Check for invalid input */
	if(!list || !object)
		return 0;

	/* Iterate checking for matches */
        for(check = list->first; check != NULL; check = check->next)
                if(object == check->object)
                        return 1;

	return 0;
}


void* deleteObject(linkedList list, void *object)
{
	linkedListObject check;

	/* Check for invalid input */
	if(!list || !object)
		return NULL;

	check = list->first;

	/* Search for Object in list */
	for(check = list->first; check != NULL; check = check->next)
		if(object == check->object) {
			/* Delete linked list object */
			
			/* Set the Next items pointer if it exists */
			if(check->next)
				check->next->prev = check->prev;

			/* Set the previous items pointer if it exists */
			if(check->prev)
				check->prev->next = check->next;
			else /* Otherwise set the first item in the list to next */
				list->first = check->next;

			free(check);
			return object;
		}

	return NULL;
}


linkedListObject firstObject(linkedList list)
{
	if(!list)
		return NULL;

	return list->first;
}

