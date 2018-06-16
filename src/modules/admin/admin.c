/*
   PWoT Talker

   This software may be used and distributed according to the terms
   of the GNU Public License, incorporated herein by reference.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <netdb.h>
#define IN_TEST_C
#define USERLEVELS
#include "linkedList.h"
#include "user.h"
#include "room.h"
#include "command.h"
#include "socket.h"
#include "misc.h"

#ifdef DMALLOC
  #include "dmalloc.h"
#endif

commandObject coPromote, coDemote, coKill;
extern killed;
char textBuffer[4096];

signed int cmdPromote(userObject user, char *text, char *call)
{
	char *spc = NULL;
	userObject other;
	int userloaded = 0;

	spc = strchr(text, ' ');
	if (spc != NULL)
	{
		*spc = '\0';	
	}

	other = getUser(text);
	
	if(!other) 
	{
		//ok, user not logged in so load them in
		if((other = (userObject)malloc(sizeof(struct userStruct))) == NULL)
		{
			writeUser(user, "** ~FR~OLPROMOTE~RS: Oh dear. No Memory.~RS\n");
			return 0;
		}
		
		strcpy(other->name, text);
		other->name[0] = toupper(text[0]);
		if(loadUser(other) < 0) 
		{
			writeUser(user, "** ~FR~OLPROMOTE~RS: No such user.\n");
			return 0;
		}
		userloaded = 1;
	}
	
	if (other->level >= user->level)
	{
		writeUser(user, "** ~FR~OLPROMOTE~RS: That user is of equal or higher level.~RS\n");
		return 0;
	}
	
	other->level = other->level++;
	saveUser(other);
	if(userloaded == 1) 
 	{
		writeUser(user, "** ~FG~OLPROMOTE~RS: %s promoted and saved.\n", other->name);
		free(other);
	} 
	else 
	{
		writeUser(user, "** ~FG~OLPROMOTE~RS: %s promoted.\n", other->name);
		writeUser(other, "** ~FG~OLPROMOTE~RS: You have been promoted by %s.\n", user->name);
	}
	return 1;
}

signed int cmdDemote(userObject user, char *text, char *call)
{
	userObject other;
	int ihadtoload = 9;

	if(!(other = getUser(text))) {
		if(!(other = newUser())) {
			writeUser(user, "** ~FR~OLDEMOTE~RS: Gah! Out of memory!\n");
			return 0;
		}
		strcpy(other->name, text);
		other->name[0] = toupper(text[0]);
		if(loadUser(other) < 0) {
			deleteUser(other);
			writeUser(user, "** ~FR~OLDEMOTE~RS: No such user.\n");
			return 0;
		}
		ihadtoload = 1;
	} else
		ihadtoload = 0;

	if(other->level >= user->level) {
		writeUser(user, "** ~FR~OLDEMOTE~RS: Nice try, but you're not high enough yourself.\n");
		return 0;
	}

	if(other->level < 1) {
		writeUser(user, "** ~FR~OLDEMOTE~RS: %s can't be demoted further.\n", other->name);
		return 0;
	}

	other->level -= 1;

	if(ihadtoload == 1) {
		saveUser(other);
		writeUser(user, "** ~FG~OLDEMOTE~RS: %s demoted and saved.\n", other->name);
		deleteUser(other);
	} else {
		writeUser(user, "** ~FG~OLDEMOTE~RS: %s duly demoted.\n", other->name);
		writeUser(other, "** ~FG~OLDEMOTE~RS: You have been demoted by %s. Muhahaha.\n", user->name);
	}
	return 1;
}

signed int cmdKill(userObject user, char *text, char *call)
{
	char *spc = NULL;
	userObject other;

	spc = strchr(text, ' ');
	if (spc != NULL)
	{
		*spc = '\0';	
	}

	other = getUser(text);
	
	if(!other) 
	{
		writeUser(user, "**~FR That user is not logged on.~RS\n");
		return 0;
	}
	
	if (other->level >= user->level)
	{
		writeUser(user, "**~FR That user is of equal or higher level.~RS\n");
		return 0;
	}
	writeUser(user, "** ~FR You have killed %s.~RS\n",other->name);
	writeUser(other,"** ~FR~LI You have been KILLED!!!!!!!!!!!~RS\n");
	sprintf(textBuffer, "** ~FR %s has killed %s ~RS\n", user->name, other->name);
        writeRoomExcept(NULL, textBuffer, user);
	killed = 1;
	disconnectUser(other, 0);
	
	return 0;
}


signed int pwotInit(void)
{
	coPromote  = makeCommand("promote",  UL_GOD, cmdPromote, 0);
        coDemote   = makeCommand("demote",   UL_GOD, cmdDemote,  0);
	coKill     = makeCommand("kill",     UL_GOD, cmdKill,    0);
	return 1;
}

signed int pwotRemove(void)
{
	deleteCommand(coPromote);
	deleteCommand(coDemote);
	deleteCommand(coKill);
	return 1;
}

unsigned int pwotRemovable(void)
{
	return 0;
}
