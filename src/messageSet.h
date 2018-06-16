/*
   PWoT Talker

   This software may be used and distributed according to the terms
   of the GNU Public License, incorporated herein by reference.
*/
#ifndef SB_P_MSGSET
#define SB_P_MSGSET

#ifndef SB_P_MESSAGE
#include "message.h"
#endif /*SB_P_MESSAGE*/
#include "linkedList.h"

struct messageSetStruct 
{
 linkedListObject messages; /*The actual set of messages*/
 int size;		/*The number of messages present.*/
 int lastRead; /*The number of the last message accessed*/
 int highestMessage; /*The highest number assigned to a message.*/
};

typedef struct messageSetStruct* messageSet;

#ifndef IN_MSGSET
extern messageSet newMessageSet();  /*Contstuctor.*/
extern int deleteMessageSet(messageSet); /*Destructor.*/
extern msgObject addMessage(messageSet,msgObject); /*Add a message.*/
extern msgObject getMessage(messageSet,int);  /*Get a message.*/
extern msgObject removeMessage(messageSet,int); /*Remove a message.*/
extern int renumberMset(messageSet);
extern int loadMSet(messageSet,char *fname);
extern messageSet saveMSet(messageSet,char *fname);
#endif /*IN_MSGSET*/
#endif /*SB_P_MSGSET*/
