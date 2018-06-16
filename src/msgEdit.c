/*
   PWoT Talker

   This software may be used and distributed according to the terms
   of the GNU Public License, incorporated herein by reference.
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#define IN_EDIT_MOD
#include "user.h"
#include "command.h"
#include "message.h"
#include "syslog.h"
#include "socket.h"

#ifdef DMALLOC
  #include "dmalloc.h"
#endif

int showMessage(userObject user, msgObject msg)
{
  int i;
  
  if(user==NULL || msg==NULL || msg->msg==NULL || msg->status==MS_EMPTY)
    return 0;
  
  if(msg->size<0)
    return -1;
  for(i=0;i<=msg->size;i++)
    {
      if(msg->msg[i]!=NULL)
	if(msg->status==MS_NORM)		
	  writeUser(user,"%s\n",msg->msg[i]);
	else
	  writeUser(user,"%2d:%s\n",i+1,msg->msg[i]);
    }
  return 1;
}

int msgPrompt(userObject user, msgObject msg)
{
  if(user==NULL || msg==NULL || msg->msg == NULL)
    return 0;
  switch(msg->status)
    {
    case MS_NORM:
      showMessage(user,msg);
      writeUser(user,"Edit Message? (y/n)");
      break;
    case MS_EDIT:
      writeUser(user,"%2d>\n",msg->size+1);
      break;
    case MS_EMENU:
      writeUser(user,"(V)iew, (R)edo, (S)ave, (A)bandon, (E)dit <N> or (W)ipe <N> (N = 1-%d)\n",((msgObject)user->data)->maxLen);
      break;
    case MS_EMENU2:
      writeUser(user,"text>\n");
      break;			
    case MS_SMENU:
      showMessage(user,msg);
      writeUser(user,"(R)edo, (E)dit, (S)ave or (A)bandon message.\n");
      break;
    }
  return 1;
}

signed int chkSMenu(userObject user, msgObject msg, char c)
{
  msgObject tmpMsg;
  switch(c)
    {
    case 'R':
      wipeMessage(msg,user->name);
      msg->status=MS_EDIT;
      msgPrompt(user,msg);
      return 1;					
      break;
    case 'E':			
      msg->status=MS_EMENU;
      msgPrompt(user,msg);
      return 1;
      break;
    case 'A':
    case 'S':
      if(c=='A')
	{
	  writeUser(user,"Message Abandoned!\n");
	  wipeMessage(msg,user->name);	
	  deleteObject(MSG_LIST, msg);
	  free(msg);
	}
      else
	{
	  writeUser(user,"Message Saved\n");
	  msg->status=MS_NORM;
	}	
      user->data=NULL;
      popInputCall(user);
      userPrompt(user);
      return 1;
    }
  return 0;	
}

signed int chkEdit(userObject user, msgObject msg, char *text)
{
 int len;
 len=strlen(text);
 if(len<1)
  return 0;
 writeSyslog("write line: %s\n",text);
 if(!strcmp(".",text))
 {
  if(msg->size==0)
  {
   writeUser(user,"No text.\n");
   wipeMessage(msg,user->name);
   popInputCall(user);
   userPrompt(user);
   return 1;
  }
  msg->status=MS_SMENU;
  msgPrompt(user,msg);
  return 1;
 }
 writeSyslog("Message Size: %i",msg->size);
 if((len=setMsgLine(msg,text,user->name,msg->size))<0)
 {
  msg->status=MS_SMENU;
  msgPrompt(user,msg);
  return 0;
 }
 else if(len==0)
 {
  writeUser(user,"ERROR: unable to complete Message! exiting!\n");
  wipeMessage(msg,user->name);
  popInputCall(user);
  userPrompt(user);	
  return 0;
 }
 
 msgPrompt(user,msg);
 return 1;
}

signed int chkNMenu(userObject user, msgObject msg, char c)
{
 if(c=='N')
 {
  popInputCall(user);
  userPrompt(user);				
 }
 else if(c=='Y')
 {
  writeUser(user,"Sorry feature not supported yet.\n");
  popInputCall(user);
  userPrompt(user);				
 }
 else msgPrompt(user,msg);				
 return 1;
}

signed int chkEMenu(userObject user, msgObject msg, char *text)
{
  msgObject tmpMsg;
  char *words, c;
  int i=-1, good=0;
  if(text==NULL)
    {
      writeUser(user,"Error in Message Edit Menu.\n");
      msg->status=MS_SMENU;
      msgPrompt(user,msg);
      return 0;				
    }
  if(msg->status==MS_EMENU2)
    {
      msg->status=MS_EMENU;		
      setMsgLine(msg,text,user->name,msg->modified);
      msgPrompt(user,msg);
      return 1;
    }
  words=strtok(text," ");
  c=toupper(words[0]);
  words=strtok(NULL," ");	
  // if(words!=NULL)
  //  writeSyslog("word2: %s: %d\n",words, atoi(words));	
  switch(c)
    {
    case 'V':
      showMessage(user,msg);
      msgPrompt(user,msg);			
      good=1;
      break;
    case 'A':
    case 'S':
      if(c=='A')
	{
	  writeUser(user,"Message Abandoned!\n");
	  wipeMessage(msg,user->name);	
	  deleteObject(MSG_LIST, msg);
	  free(msg);
	}
      else
	{
	  writeUser(user,"Message Saved\n");
	  msg->status=MS_NORM;
	}
      user->data=NULL;
      popInputCall(user);
      userPrompt(user);
      good = 1;
      break;			
    case 'R':
      wipeMessage(msg,user->name);
      msg->status=MS_EDIT;
      msgPrompt(user,msg);
      good=1;					
      break;
    case 'E':
      if(words==NULL || (i=(atoi(words)-1))<0 || i>=msg->maxLen)
	{
	  writeUser(user,"You need to specify a line number (1-%d)\n",msg->maxLen);
	  msgPrompt(user,msg);
	  good=1;
	  break;
	}
      msg->modified=i;
      msg->status=MS_EMENU2;
      msgPrompt(user,msg);			
      good=1;
      break;
    case 'W':
      if(words==NULL || (i=(atoi(words)-1))<0 || i>=msg->maxLen)
	{
	  writeUser(user,"line %d invalid.\n",i);
	  writeUser(user,"You need to specify a line number (1-%d)\n",msg->maxLen);
	  msgPrompt(user,msg);
	  good = 1;
	  break;
	}
      if(i<msg->size)
	setMsgLine(msg,RESET_CODE,user->name,i);
      writeUser(user,"Message line %d wiped.\n",i+1);
      msgPrompt(user,msg);
      good = 1;
      break;
      /**
	 case 'D':
	 if(words==NULL || (i=(atoi(words)-1))<0 || i>=msg->maxLen)
	 {
	 writeUser(user,"You need to specify a line number (1-%d)\n",msg->maxLen);
	 msgPrompt(user,msg);
	 good = 1;
	 break;				
	 }
	 wipeMessageLine(msg,user->name,i);
	 writeUser(user,"Message line %d deleted.\n",i+1);
	 msgPrompt(user,msg);
	 good = 1;
	 break;
      */
    }
  return good;
}


signed int startMsgEdit(userObject user, char *text, msgObject msg)
{
  int len;	
  char c;	
  msgObject mainMsg;
  if(user==NULL)
    return -1;
  /*** If the user has status US_EDIT then they're editing
   *** (duh) so text is a command or a line, we also assume that
   *** the user->data has been prepared as a message.
   *** any other status and we forget about it. This is the
   *** msgEdit file.
   ***/
  if(user->status != US_EDIT)
    //    return -1;
   {
      if(msg==NULL || msg->status==MS_EMPTY) 
	{
	  if(msg==NULL)
	    msg = newMessage(user->name);
	  if(msg==NULL)
	    {
	      writeSyslog("Error creating message with write.\n");
	      return -1;
	    }
	}
      /** OK, there's no text.. so we enter editing mode after all **/
      if(text==NULL || text[0]=='\0') 
	{ 
	  pushInputCall(user,inputMsgEdit,MS_EDIT);
	  msgPrompt(user,msg); 
	  return 1; 
	} 
      writeMessage(msg,text,user->name);
      showMessage(user,msg);
      msg->status=MS_NORM;
      userPrompt(user);				
      return 1;
    }
  /** We're in the editing mode... **
      msg should be the user->data **/
  /** Curse ***
      If any one screws this up and changes the data
      object while in US_EDIT state then may the
      Might of PWOT cast them into the eternal pit of
      hell wherein they shall be subjected to an eternity of
      being poked with sharp pointy sticks by annoying people
      in bright orange jeans.
      Oh and a plague of boils upon them too if it's not too much
      trouble.
  **/ 
  /** Assume whoever had it last cleaned it up *cough* */
  if(user->data==NULL)
    user->data=newMessage(user->name);
  msg=(msgObject)user->data;
  
  len=strlen(text);	
  if(text==NULL || len==0)
    {
      msgPrompt(user,msg);				
      return 1;
    }
  
  /** Find out which menu we're in (depends on message status and
      go to the appropriate menu */
  c=toupper(text[0]);	
  writeSyslog("Edit menu No. %d, option %c\n",msg->status,c);
#ifdef DEBUG
  printMsg(msg);
#endif
  switch(msg->status)
    {
    case MS_NORM:
      return chkNMenu(user,msg,c);		
      break;
    case MS_EDIT:			
      return chkEdit(user,msg,text);
      break;
    case MS_SMENU:
      return chkSMenu(user,msg,c);
      break;
    case MS_EMENU:
    case MS_EMENU2:
      return chkEMenu(user,msg,text);
    default:
      writeUser(user,"Error in message editing subsystem.\n");
      writeUser(user,"Please inform system Admin and pass on the error code %d.\n",msg->status);
      writeUser(user,"Returning you to the normal operation.\n");
      popInputCall(user);
      wipeMessage(msg,user->name);
      userPrompt(user);			
      return 0;
    }
  return 0;	
}
