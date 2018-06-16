/*
   PWoT Talker

   This software may be used and distributed according to the terms
   of the GNU Public License, incorporated herein by reference.
*/
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define IN_MESSAGE
#include "message.h"
#include "user.h"
#include "socket.h"
#include "colour.h"
#include "misc.h"
#include "syslog.h"

#ifdef DMALLOC
  #include "dmalloc.h"
#endif

/** Create a new msgObject with teh given length and width. 
 Default to config-><msgWidth/msgLength> if the values are <1.
*/
msgObject findMessage(int id)
{
  linkedListObject list;
  for(list = firstObject(MSG_LIST); list != NULL; list = list->next)
    {
      if(((msgObject)list->object)->id == id)
	return (msgObject)list->object;
    }
  return NULL;
}
	
msgObject defineMessage(char *user, int length, int width)
{
 int i;	
 msgObject msg;
 int id;
 /*
 if((config->msgWidth <1 && width <1)|| (config->msgLen<1 && length<1))
  return NULL;
*/ 
 if(width<1)
   width=80;
 if(length<1)
   length=15;
 
 if((width <1)|| (length<1))
   return NULL;
 
 if( ( msg = (msgObject)malloc(sizeof(struct messageStruct))) == NULL )
   return NULL;
 
 if(width<1)
   //  msg->maxWidth=config->msgWidth;
   msg->maxWidth=80;
 else
  msg->maxWidth=width;
 if(length<1)
   //  msg->maxLen=config->msgLen;
   msg->maxLen=15;
 else
  msg->maxLen=length;
 
 msg->msg = calloc(msg->maxLen,sizeof(char *));
 
 if(msg->msg==NULL)
 {
  free(msg);
  return NULL;
 }	
 
 for(i=0;i<msg->maxLen;i++)
  msg->msg[i]=NULL;	
 id=rand();

 /** If we get 20 random numbers which match id's already in the list...
     We have an issue. The count is in to prevent an unlikly but possible 
     infinite loop.
 **/
 for(i=0;i<20;i++)
   if(!findMessage(id))
     break;
   else
     id=rand();
 if(i==20)
   return NULL;
 msg->id=id;
 strcpy(msg->author,user);
 msg->readTime=0;
 msg->writeTime = time(0);
 msg->size=0;
 msg->modified=0;	
 msg->subject=NULL;	
 /** All created messages are added to the list by default..
     I need somewhere I can put them that doesn't depend on 
     smail/message boards/whatever sick purposes you freaks can
     put them to.
 **/
 addObject(MSG_LIST, msg);
 return msg;
}

/** Create a new message object. */
msgObject newMessage(char *user)
{return defineMessage(user,0,0);}

void deleteMessage(msgObject msg)
{
 int i;
 if(msg==NULL || msg->msg==NULL)
  return;
 for(i=0;i<msg->maxLen;i++)
  if(msg->msg[i]!=NULL)
   free(msg->msg[i]);
 free(msg->msg);
 free(msg);	
 //	msg->status=MS_EMPTY;
}

/** Check the size of a message **/
int chkMsgSize(msgObject msg)
{
 if(msg==NULL) return -1;	
 for(msg->size=msg->maxLen;msg->size>0;msg->size--)
   if(msg->msg[msg->size]!=NULL && strcmp(RESET_CODE,msg->msg[msg->size]))
   break;	
 return msg->size;
}

/** Empty the message **/
int wipeMessage(msgObject msg, char *user)
{
 int i;
 if(msg==NULL || user==NULL || msg->msg==NULL)
  return 0;
 
 msg->readTime=0;
 msg->writeTime= time(0);
 msg->author[0]='\0';
 msg->status=MS_EMPTY;
 msg->size=0;
 for(i=0;i<msg->maxLen;i++)
  if(msg->msg[i]!=NULL)
   free(msg->msg[i]);
 return 1;
}

/** write a new message and return it. Any previous text is overwritten,
 ** any text which extends beyond the given text is removed. The text is
 ** trunctated at mag->maxLen.
** returns 0 or error, 1 on normal and -1 if  the last line of the message
is written to.*/
int writeMessage(msgObject msg, char *text, char *user) 
{
 char *p;
 int j;
 
 if(msg==NULL || msg->msg == NULL || text==NULL)
  return 0;
 
 if(strlen(text)<1) return 0;
 
 wipeMessage(msg,user);
 
 p=text;
 for(msg->size=0;*p!='\0' && msg->size<msg->maxLen;msg->size++)
 {
  msg->msg[msg->size]=malloc(msg->maxWidth);
  for(j=0;j<msg->maxWidth-1;j++,p++)
  {
   if(*p=='\n' || *p=='\r')
   {
    if(j==0)
    {
     sprintf(msg->msg[msg->size],"%s",RESET_CODE);
     break;
    }
    msg->msg[msg->size][j]='\0';
    break;
   }
   msg->msg[msg->size][j]=*p;
   if(*p=='\0')
    break;
  }
  if(j==(msg->maxWidth-2))
   msg->msg[msg->size]='\0';
 }
 
 if(msg->size==0 && msg->msg[0]==NULL)
  return 0;
 
 sprintf(msg->author,"%s", user);
 msg->writeTime= time(0);
 
 if(msg->size==msg->maxLen)
  return -1;
 return 1;
}


/** Set the text on the given line.
** if the line is longer that msg->maxWidth or a newline/carriage return is
** encountered it is truncated unless the following line is NULL.
** returns 0 on error, 1 on normal execution, -1 if  the last line of the message
** is written.
*/
int setMsgLine(msgObject msg, char *text, char *user, int line) 
{
 char *p;
 int i,j;
 
 if(msg==NULL || msg->msg == NULL || line>=msg->maxLen)
  return 0;
 if(strlen(text)<1)	
  return 0;
 p=text;
 j=strlen(RESET_CODE);
 for(i=0;i<line;i++)
 {
  if(msg->msg[i]==NULL)
  {
   msg->msg[i]=malloc(msg->maxLen);
   sprintf(msg->msg[i],"%s",RESET_CODE);
  }
 }
 writeSyslog("writing to line:%d",i);
 do
 {
  for(j=0, p=text;j<msg->maxWidth-1;j++,p++)
  {
   if(msg->msg[i]==NULL)
    msg->msg[i]=malloc(msg->maxWidth);
   
   if(*p=='\n' || *p=='\r')
   {
    if(j==0)
    {
     sprintf(msg->msg[i],"%s",RESET_CODE);
     break;
    }
    msg->msg[i][j]='\0';
    break;
   }
   msg->msg[i][j]=*p;
   if(*p=='\0')
    break;
  }
  if(j==(msg->maxWidth-2))
   msg->msg[i]='\0';
  i++;
 }		
 while( *p !='\0' && (i+1) < msg->maxLen && 
  (	(msg->msg[i+1]==NULL) || !strcmp(msg->msg[i+1],RESET_CODE) ));
 if(i==0 && msg->msg[0]==NULL)
 {
  msg->size=0; 
  return -1;
 }
 if(msg->size<i)
  msg->size=i;
 
 sprintf(msg->author,"%s", user);
 msg->writeTime= time(0);
 
 if(msg->size==msg->maxLen)
  return -1;
 return 1;
}

/** save the message out to file, overwrite the file if owrite is 
 ** non-zero, append to it otherwise.**/
int saveMessage(msgObject msg, char *fname, int msgNum, int owrite)
{
 FILE *fp;
   //	char fnb[256];
 char *perms;
 int i, size;
 
 if(msg==NULL || fname==NULL || msg->msg==NULL)
  return -1;
 
 if(owrite)
  perms = "w";
 else
  perms = "a";
 if(!(fp = fopen(fname, perms)))
  return -1;
 
 fprintf(fp, "\n");
 fprintf(fp, "*%d %s",msgNum, msg->author);
 fprintf(fp, "%u %u %d %d\n", 
  msg->writeTime, msg->readTime, msg->maxLen, msg->maxWidth);
 
 size =msg->maxLen;
 for(i=0;i<size;i++)
  if(msg->msg!=NULL)
   fprintf(fp,"%s\n",msg->msg[i]);
  
 return 1;
}


/** Remove the given line from the message. Move all lines below it up one.**
int wipeMessageLine(msgObject msg, char *user, int line)
{
 int i=0;
 if(msg==NULL || user==NULL || msg->msg==NULL)
  return 0; 
 
 if(line<0 || line>=msg->maxLen)
  return 0;
 if(msg->msg[line]==NULL)
  return 0;
 for(i=line;i<msg->maxLen-1;i++)
 {
  if(msg->msg[i+1]==NULL)
   break;
  strcpy(msg->msg[i],msg->msg[i+1]);
  fprintf(stderr,"%d:%s\n",i,msg->msg[i]);
 }
 if(msg->msg[i]!=NULL)
  free(msg->msg[i]);
 chkMsgSize(msg);	
 sprintf(msg->author,user);
 msg->writeTime = time(0);
 return 1;
}
*/
/*Copy a message, but don't keep the variable bits..
* Set the modified and readTime variables to 0.
* Set the writeTime to now amd the status to MS_NORM.*/
extern msgObject copyMessage(msgObject msg)
{
 msgObject newMsg;
 int i,copy=-1;
 if(msg==NULL)
  return NULL;
 newMsg=defineMessage(msg->author,msg->maxLen,msg->maxWidth);
 if(newMsg==NULL)
  return NULL;
 
 for(i=msg->maxLen-1;i>=0;i--)
 {
  if(copy<0)
  {
   if(msg->msg[i]!=NULL && !strcmp(msg->msg[i],RESET_CODE))
    copy=i; /* Copy is the first non-empty string...*/
   else
    newMsg->msg[i]=NULL;
  }
  /*Not an else because what we're testing may be changed in
  * the above statements...*/
  if(copy>=0)
  {
   newMsg->msg[i]=malloc(newMsg->maxWidth);
   if(msg->msg[i]==NULL)
    strcpy(newMsg->msg[i],RESET_CODE);
   else
    strcpy(newMsg->msg[i],msg->msg[i]);
  }	
 }
 newMsg->size=copy+1;
 newMsg->status=MS_NORM;
 newMsg->modified=0;
 newMsg->readTime=0;
 newMsg->writeTime=time(0);
 if(msg->subject==NULL)
  newMsg->subject=NULL;
 else
 {
  newMsg->subject=malloc(newMsg->maxLen);
  strcpy(newMsg->subject,msg->subject);
 }
 return newMsg;
}
#ifdef DEBUG
void printMsg(msgObject msg)
{
 writeSyslog("PrintMessage\n");
 if(msg==NULL)
 {
  writeSyslog("NULL MESSAGE\n");
  return;
 }
 writeSyslog("status: %d, maxWidth: %d, maxLen: %d, modified: %d, size: %d, Author: %s\n",
  msg->status, msg->maxWidth, msg->maxLen,msg->modified,msg->size,msg->author);
}
#endif














