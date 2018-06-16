/*
   PWoT Talker

   This software may be used and distributed according to the terms
   of the GNU Public License, incorporated herein by reference.
*/
#ifndef SB_P_MESSAGE
#define SB_P_MESSAGE

#define MESSAGE_INIT 15    /* Default length of a message */
#define MESSAGE_WIDTH 80   /* Default width of a message */ 
#define AUTHLEN 80
#define RESET_CODE "~RS"
extern char text[1024];

struct messageStruct 
{
  int status;              /* Current status of the message. */
  int maxWidth;            /* Maximum length of any line. */
  int maxLen;              /* Maximum number of lines. */
  int modified;   /* Non-zero if unsaved message has been modified. */
  int size;     /*Last non-NULL line*/	
  int id;  /*Message id... meaningful only in that it should be unique */
  unsigned int writeTime;  /* Time of last modification. */ 
  unsigned int readTime;   /* Time it was last read. */
  char *subject;		/*Subject line, if any.*/	
  char author[AUTHLEN];    /* The name of the last person to edit. */
  char **msg;             /* The text of the message. */
};


typedef struct messageStruct* msgObject;

//#define MSG_LIST 6
#define MSG_LIST globalLists[6]

#define MS_NORM 0
#define MS_EDIT 1
#define MS_EMENU 2
#define MS_EMENU2 3
#define MS_SMENU 4
#define MS_EMPTY 5
#define MS_ATMP 6
#define MS_BTMP 7

#ifndef IN_MESSAGE
extern msgObject findMessage(int);
/** Create a new message object. */
extern msgObject newMessage(char *);
extern msgObject defineMessage(char, int, int);
/** I giveth and I taketh away... */
extern void deleteMessage(msgObject msg);

/** write a new message and return it. Any previous text is overwritten,
** any text which extends beyond the given text is removed. The text is
** trunctated at mag->maxLen and -1 is returned.*/
extern msgObject writeMessage(msgObject , char *, char *); 

/** Write the message out to the user. */
extern int readMessage(msgObject, char *);

/** save the message out to file, overwrite the file if owrite is non-zero
 **/
extern int saveMessage(msgObject, char *, int owrite);

/** Set the text on the given line.. extend the message to fit if the line
 ** is longer than the given size. **/
extern int setMsgLine(msgObject, char *, char *, int);

/** Copy a message**/
extern msgObject copyMessage(msgObject);

/** Remove the given line from the message **/
//extern int wipeMessageLine(msgObject, char *, int);

/** Empty the message **/
extern int wipeMessage(msgObject, char *);
#ifdef DEBUG
void printMsg(msgObject msg);
#endif /*DEBUG*/
#endif /** IN_MESSAGE **/
#endif /** MESSAGE **/









