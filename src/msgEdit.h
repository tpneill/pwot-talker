/*
   PWoT Talker

   This software may be used and distributed according to the terms
   of the GNU Public License, incorporated herein by reference.
*/
#ifndef SB_MSGMNU
#define SB_MSGMNU

#ifndef P_USER 
#include "user.h"
#endif

#ifndef SB_P_MESSAGE
#include "message.h"
#endif

extern int showMessage(userObject user, msgObject msg);
extern int msgPrompt(userObject user, msgObject msg);
extern signed int chkSMenu(userObject user, msgObject msg, char c);
extern signed int chkEdit(userObject user, msgObject msg, char *text);
extern signed int chkNMenu(userObject user, msgObject msg, char c);
extern signed int chkEMenu(userObject user, msgObject msg, char *text);
extern signed int startMsgEdit(userObject user, char *text, msgObject msg);
inline signed int inputMsgEdit(userObject user, char *text)
{return inputMsgEdit(user,text,NULL);}

#endif // SB_MSGMNU





