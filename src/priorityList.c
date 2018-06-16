/*
   PWoT Talker

   This software may be used and distributed according to the terms
   of the GNU Public License, incorporated herein by reference.
*/
#include <stdlib.h>

#define IN_PLIST
#include "linkedList.h"
#include "priorityList.h"

#ifdef DMALLOC
  #include "dmalloc.h"
#endif

linkedListObject newPlist(void *object, int priority)
{
 linkedListObject newList;
 if(object==NULL)
  return NULL;
 
 newList = malloc(sizeof(struct linkedListObjStruct));
 if(newList==NULL)
  return NULL;
 newList->prev=NULL;
 newList->next=NULL;
 newList->object=object;
 newList->list=-1;
 newList->priority=priority;
 return newList;	
}

linkedListObject newList(void *object)
{
 return newPlist(object,0);
}

linkedListObject listAdd(linkedListObject llo, void *object)
{
 linkedListObject tmp, newList;
 
 if(llo==NULL || object==NULL)
  return NULL;
 
 newList = malloc(sizeof(struct linkedListObjStruct));
 if(newList==NULL)
  return NULL;
 
 newList->object= object;
 newList->list=-1;
 newList->priority=0;
 for(tmp=llo;tmp->next!=NULL;tmp=tmp->next);
 tmp->next=newList;
 newList->prev=tmp;
 return newList;
}

linkedListObject plistAdd(linkedListObject llo, void *object, int priority)
{
 linkedListObject tmp, newList;
 if(llo==NULL || object==NULL)
  return NULL;
 
 newList = malloc(sizeof(struct linkedListObjStruct));
 if(newList==NULL)
  return NULL;
 
 newList->list=-1;
 newList->object= object;
 newList->priority=0;
 if(priority>llo->priority)
 {
  newList->object=llo->object;
  llo->object=object;
  newList->priority=llo->priority;
  llo->priority=priority;
 }
 else
 {
  newList->object= object;
  newList->priority=0;
 }
 
 for(tmp=llo;
  tmp->next!=NULL && tmp->next->priority>=priority;
  tmp=tmp->next);
 if(tmp->next==NULL)
 {
  tmp->next=newList;
  newList->next=NULL;
  newList->prev=tmp;
 }
 else
 {
  newList->next=tmp->next;
  newList->prev=tmp;
  tmp->next=newList;
 }
 return newList;
}

linkedListObject listFirst(linkedListObject llo)
{
 if(llo==NULL)
  return NULL;
 for(;llo->prev!=NULL;llo=llo->prev);
 return llo;
}

linkedListObject listLast(linkedListObject llo)
{
 if(llo==NULL)
  return NULL;
 for(;llo->next!=NULL;llo=llo->next);
 return llo;
}


linkedListObject plistFirstAt(linkedListObject llo, int priority)
{
 linkedListObject tmp;
 for(tmp=listFirst(llo);
  tmp!=NULL && tmp->priority!=priority;
  tmp=tmp->next);
 return tmp;
}

linkedListObject plistLastAt(linkedListObject llo, int priority)
{
 linkedListObject tmp;
 for(tmp=listLast(llo);
  tmp!=NULL && tmp->priority!=priority;
  tmp=tmp->prev);
 return tmp;
}

linkedListObject listRemove(linkedListObject llo)
{
 if(llo==NULL)
  return NULL;
 if(llo->prev!=NULL)
  llo->prev=llo->next;
 if(llo->next!=NULL)
  llo->next=llo->prev;
 llo->next=NULL;
 llo->prev=NULL;
 return llo;
}

linkedListObject listRmObject(linkedListObject llo, void *object)
{
 linkedListObject tmp;
 if(llo==NULL || object==NULL)
  return NULL;
 for(tmp=listFirst(llo);tmp!=NULL && tmp->object!=object;tmp=tmp->next);
 return listRemove(tmp);
}
