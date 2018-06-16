/*
   PWoT Talker

   This software may be used and distributed according to the terms
   of the GNU Public License, incorporated herein by reference.
*/
#include <stdlib.h>
#include <stdio.h>

#define IN_MSGSET
//#include "config.h"
#include "message.h"
#include "syslog.h"
#include "messageSet.h"
#include "linkedList.h"
#include "priorityList.h"

#ifdef DMALLOC
  #include "dmalloc.h"
#endif


/** Constructor.. create and initialise a new messageSet **/
messageSet newMessageSet()
{
 messageSet mset;
 mset =(messageSet)malloc(sizeof(struct messageSetStruct));
 if(mset==NULL)
  return NULL;
 mset->messages=NULL;
 mset->size=0;
 mset->lastRead=-1;
 mset->highestMessage=-1;	
 return mset;	
};

void deleteMessageSet(messageSet mset)
{
  linkedListObject llo;
  for(llo=mset->messages;llo!=NULL;llo=llo->next)
    if(llo->prev!=NULL)
      free(llo->prev);
  free(mset);
};

msgObject addMessage(messageSet mset, msgObject msg)
{
 linkedListObject llo;
 if(msg==NULL || msg==NULL)
  return NULL;
 if(mset->messages==NULL)
   {
     if((mset->messages = newList(mset->messages))==NULL)
       return NULL;
     mset->size++;
     mset->highestMessage=0;		
     return msg;
   }
 if((llo=listAdd(mset->messages,msg))==NULL)
   return NULL;
 llo->priority=(++mset->highestMessage);
 mset->size++;
 return msg;
}

msgObject getMessage(messageSet mset, int num)
{
  linkedListObject llo;
  int i;
  if(mset==NULL || mset->messages== NULL || num <0 || num>mset->highestMessage)
    return NULL;
  for(i=0, llo=mset->messages;
      i<mset->size && llo!=NULL &&  llo->priority!=num;
      i++, llo=llo->next);	
  if(i==mset->size)
    return NULL;
  if(llo==NULL)
    {
      mset->size=i;
      return NULL;
    }
  return (msgObject)llo->object;
}

int removeMessage(messageSet mset, int num)
{
 linkedListObject llo;
 int i;
 if(mset==NULL || mset->messages== NULL || num <0 || num>mset->highestMessage)
   return 0;
 for(i=0, llo=mset->messages;
     i<mset->size && llo!=NULL &&  llo->priority!=num;
     i++, llo=llo->next);	
 if(i==mset->size)
   return 0;
 if(llo==NULL)
 {
  mset->size=i;
  return 0;
 }
 
 listRemove(llo);
 free(llo);
 mset->size--;
 return 1;	
}	

int renumberMset(messageSet mset)
{
 linkedListObject llo;
 int i;
 if(mset==NULL) 
  return 0;
 if(mset->messages==NULL)
  return 1;
 for(i=0,llo=listFirst(mset->messages);
  llo!=NULL;
  i++,llo=llo->next);
 mset->size=i;
 mset->highestMessage=i-1;
 mset->lastRead=0;
 return i;
}

int saveMSet(messageSet mset, char *fname)
{
  FILE *fp;
  int i,j;
  linkedListObject llo;
  msgObject msg;

  fp = fopen(fname,"w");
  if(fp==NULL)
    {
      writeSyslog("module","Error opening messageSet save file %s",fname);
      return 0;
    }
  renumberMset(mset);
  fprintf(fp,"v1 %i\n",mset->size);
  for(llo=mset->messages,j=1;llo!=NULL;llo=llo->next,j++)
    {
      msg=(msgObject)llo->object;
      fprintf(fp,"$%i : %s",j,msg->author);
      fprintf(fp,"$%i : %i : %i : %u : %u\n",msg->maxWidth,
	     msg->maxLen, msg->size, msg->writeTime, msg->readTime);
      for(i=0;i<msg->size;i++)
	{
	  fprintf(fp,"#%s\n",msg->msg[i]);
	}  
      fprintf(fp,"\n");
    }
  fclose(fp);
  return 1;
}

