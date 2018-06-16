/*
   PWoT Talker

   This software may be used and distributed according to the terms
   of the GNU Public License, incorporated herein by reference.
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <netdb.h>
#include <crypt.h>
#define IN_TEST_C
#define USERLEVELS
#include "linkedList.h"
#include "user.h"
#include "room.h"
#include "command.h"
#include "socket.h"
#include "misc.h"
#include "time.h"

#ifdef DMALLOC
  #include "dmalloc.h"
#endif

commandObject	coEmote, coPemote, coWho, coTell, coPeople, coWhere, coHname,
		coShout, coDsay, coWizshout, coExamine, coDesc, coShoutEmote,
		coCls, coTopic, coRevtell, coReview, coAfk, coGo, coMove, coSex,
		coTlock, coTunlock;

static char textBuffer[4096];

signed int cmdEmote(userObject user, char *text, char *call)
{
	char *spc;

	if(*text == '\0') {
		writeUser(user, "Emote what?\n");
		return 0;
	}

	if((text[0] == '\'' || text[0] == '`') && text[1] == 's')
		spc = "";
	else
		spc = " ";

	writeUser(user, "%s%s%s ~RS\n", user->name, spc, text);
	sprintf(textBuffer, "%s%s%s ~RS\n", user->name, spc, text);
	writeRoomExcept(user->room, textBuffer, user);

	return 1;
}

signed int cmdPemote(userObject user, char *text, char *call)
{
	char buffer[80], *spc = NULL;
	userObject other = NULL;

	spc = strchr(text, ' ');
	
	if(spc == NULL) 
	{
		writeUser(user, "~FRPemote who what?~RS\n");
		return 0;
	}

	for(*(spc++) = '\0'; isspace(*spc)&&(*spc != '\0'); spc++);
	
	if(*spc == '\0' || spc == NULL) 
	{
		writeUser(user, "~FRPemote who what?~RS\n");
		return 0;
	}

	other = getUser(text);
	
	if(!other) {
		writeUser(user, "~FRUser '%s' not logged in~RS\n", text);
		return 0;
	}

	if(other == user) {
		writeUser(user, "You've missed the point of a talker. Talk to someone ~OLelse~RS.\n");
		sprintf(buffer, "** ~OL%s~RS needs a life!\n", user->name);
		writeRoomExcept(user->room, buffer, user);
		return 1;
	}

	if(other->status != US_NORM) {
		switch(other->status) {
		case US_BUSY:
			writeUser(user, "%s is busy and couldn't reply. Try again later.\n", other->name);
			return 0;
		case US_LOGIN:
			writeUser(user, "User '%s' not logged in\n", text);
			return 0;
		case US_TEMP:
			notifyBug("modules/std.c::cmdTell() code 0x01", user, other->name);
			return 0;
		default:
			notifyBug("modules/std.c::cmdTell() code 0x02", user, other->name);
			return 0;
		}
	}

	writeUser(user, "~OL(%s >> %s)~RS %s %s ~RS\n", user->name, other->name, user->name, spc);
	addToRevbuf(user->revbuf, "(%s >> %s) %s %s ~RS\n", user->name, other->name, user->name, spc);
	writeUser(other, "~OL(%s << %s)~RS %s %s ~RS\n", other->name, user->name, user->name, spc);
	addToRevbuf(other->revbuf, "(%s << %s) %s %s ~RS\n", other->name, user->name, user->name, spc);

	return 1;
}

signed int cmdWho(userObject user, char *text, char *call)
{
	linkedListObject list;
	int num = 0;
	int max_desc = 0;
	int desc_space = 0;

	writeUser(user, "~BM*** Current users on %s ***~RS\n\n", timeToString(time(NULL), 2));

	/* Figure out how much space we have to play with for name & desc... */
	/* We want to leave a space at the left and 2 at the right */
	max_desc = user->screenWidth - 23;

	/* Ensure we have at least 15 chars */
	if(max_desc < 15)
		max_desc = 15;

	/* Write header */
	writeUser(user, " ~UL%-*s Room         Time~RS\n\n", max_desc, "Name & Desc");

	for(list = firstObject(USER_LIST); list != NULL; list = list->next) {
		if(USER(list)->status == US_NORM || USER(list)->status == US_BUSY ||
		   USER(list)->status == US_AFK) {

			/* Write Name, Desc & Room */
			desc_space = max_desc - strlen(USER(list)->name) - 1;
			writeUser(user, " %s %-*s~RS %-12s ", 
				USER(list)->name,
				desc_space, CropString(USER(list)->desc, desc_space),
				CropString(USER(list)->room->name, 12));
			
			/* Write Time or AFK */
			if(USER(list)->status != US_AFK)
				writeUser(user, "%-4d ", USER(list)->loginTime);
			else
				writeUser(user, "~BRAFK~RS  ");

			/* Write Level */
			switch(USER(list)->level) {
		
				case UL_NEW:
				case UL_ARCH:
				case UL_GOD:
					writeUser(user, "%c", safeUserLevel(USER(list)->level)[0]);
					break;

				default:
					break;
			}
			
			/* Terminate Line */
			writeUser(user, "\n");

			/* Incrememnt User Count */
			num++;
	           }
	}
	writeUser(user, "\n%d users online\n\n", num);

	return 1;
}

signed int cmdTell(userObject user, char *text, char *call)
{
	userObject other = NULL;
	char buffer[80], *spc = NULL;
	int count = 0;

	if(call[0] == '>' && call[1] == '>') {
		other = getUser(user->lastTell);
		spc = text;
		if(!other) {
			writeUser(user, "User '%s' not logged in\n", user->lastTell);
			return 0;
		}
	} else {
		spc = strchr(text, ' ');
		if(spc == NULL) {
			writeUser(user, "Tell who what?\n");
			return 0;
		}

		for(*(spc++) = '\0'; isspace(*spc)&&(*spc != '\0'); spc++);
		if(*spc == '\0' || spc == NULL) {
			writeUser(user, "Tell who what?\n");
			return 0;
		}

		other = getUser(text);
	}
	
	if(!other) {
		writeUser(user, "User '%s' not logged in\n", text);
		return 0;
	}

	/* Check there is only one match */
	count = userMatch(text);
	if(count > 1) {
		writeUser(user, "'%s' matches %d users.  Be more precise\n", text, count);
		return 0;
	}

	if(other == user) {
		writeUser(user, "You've missed the point of a talker. Talk to someone ~OLelse~RS.\n");
		sprintf(buffer, "** ~OL%s~RS needs a life!\n", user->name);
		writeRoomExcept(user->room, buffer, user);
		return 1;
	}

	if(other->status != US_NORM) {
		switch(other->status) {
		case US_BUSY:
			writeUser(user, "%s is busy and couldn't reply. Try again later.\n", other->name);
			return 0;
		case US_LOGIN:
			writeUser(user, "User '%s' not logged in\n", text);
			return 0;
		case US_TEMP:
			notifyBug("modules/std.c::cmdTell() code 0x01", user, other->name);
			return 0;
		default:
			notifyBug("modules/std.c::cmdTell() code 0x02", user, other->name);
			return 0;
		}
	}

	strcpy(user->lastTell, other->name);

	writeUser(user, "~OLYou tell %s~RS: %s ~RS\n", other->name, spc);
	addToRevbuf(user->revbuf, "~OLYou tell %s~RS: %s ~RS\n", other->name, spc);
	writeUser(other, "~OL%s tells you~RS: %s ~RS\n", user->name, spc);
	addToRevbuf(other->revbuf, "~OL%s tells you~RS: %s ~RS\n", user->name, spc);
	return 1;
}

signed int cmdPeople(userObject user, char *text, char *call)
{
	linkedListObject list;
	char *printName, *level, *status;

	writeUser(user, "~BM*** Users online ***~RS\n\n");
	writeUser(user, " ~UL%-15s %-05s %-46s %s~RS\n", "Name", "Level", "Hostname", "Status");
	for(list = firstObject(USER_LIST); list != NULL; list = list->next) {
		switch(USER(list)->status) {
		case US_NORM:
			status = "NORM";
			printName = USER(list)->name;
			break;
		case US_BUSY:
			status = "~FYBUSY~RS";
			printName = USER(list)->name;
			break;
		case US_TEMP:
			status = "~FMTEMP~RS";
			if(USER(list)->name[0] == '\0')
				printName = "[ No Name ]";
			else
				printName = USER(list)->name;
			break;
		case US_LOGIN:
			status = "LOGIN";
			if(USER(list)->name[0] == '\0')
				printName = "(Login)";
			else
				printName = "(Passwd)";
			break;
		case US_AFK:
			status = "~FRAFK~RS";
			printName = USER(list)->name;
			break;
		default:
			notifyBug("modules/std.c::cmdPeople() code 0x07", user, USER(list)->name);
			status = "~BROops!~RS";
			printName = "Oops!";
			break;
		}

		level = safeUserLevel(USER(list)->level);

		writeUser(user, " %-15s %-05s %-46s %s\n", printName, level,
			  USER(list)->siteHostname, status);
	}
	writeUser(user, "\n");

	return 1;
}

signed int cmdWhere(userObject user, char *text, char *call)
{
	linkedListObject list;

	writeUser(user, "~BM*** User locations ***~RS\n\n");
	writeUser(user, "~UL %-15s    %-40s~RS\n", "User", "Location");

	for(list = firstObject(USER_LIST); list != NULL; list = list->next) {
		if(USER(list)->status == US_NORM || USER(list)->status == US_BUSY ||
		   USER(list)->status == US_AFK)
			writeUser(user, " %-15s    %s\n", USER(list)->name, USER(list)->verboseSite);
	}
	writeUser(user, "\n");
	return 1;
}

signed int cmdVerboseName(userObject user, char *text, char *call)
{
	char *nm;

	if(!(nm = getHostDescription(text))) {
		writeUser(user, "** Host '%s' does not resolve.\n", text);
		return 0;
	} 

	writeUser(user, "** Host %s has description :\n** '%s'\n", text, nm);

	return 1;
}

signed int cmdShout(userObject user, char *text, char *call)
{

  sprintf(textBuffer,"%s shouts: %s\n",user->name, text);
  writeUser(user,"You shout: %s\n", text);
  writeRoomExcept(NULL, textBuffer, user);

  return 1;
}

signed int cmdShoutEmote(userObject user, char *text, char *call)
{

  sprintf(textBuffer,"%s shouts: %s %s\n",user->name,user->name, text);
  writeUser(user,"%s shouts: %s %s\n", user->name,user->name,text);
  writeRoomExcept(NULL, textBuffer, user);
  return 1;
}

signed int cmdWizshout(userObject user, char *text, char *call)
{

  sprintf(textBuffer,"%s wizshouts: %s\n",user->name, text);
  writeUser(user,"You wizshout: %s\n", text);
  //writeRoomAboveExcept(NULL, textBuffer, UL_ARCH, user);

  return 1;
}

signed int cmdDsay(userObject user, char *text, char *call)
{
  char *what;
  userObject other = NULL;
  char *spc = NULL;
  int count = 0;
 
  if(call[0] == ',' && call[1] == ',') 
    {
      other = getUser(user->lastTell);
      spc = text;
      if(!other) 
	{
	  writeUser(user, "User '%s' not logged in\n", user->lastTell);
	  return 0;
	}
    } 
  else 
    {
      spc = strchr(text, ' ');
      if(spc == NULL) 
	{
	  writeUser(user, "Dsay who what?\n");
	  return 0;
	}
      
      for(*(spc++) = '\0'; isspace(*spc)&&(*spc != '\0'); spc++);
      if(*spc == '\0' || spc == NULL) 
	{
	  writeUser(user, "Tell who what?\n");
	  return 0;
	}
      
      other = getUser(text);
    }
  
  if(!other) 
    {
      writeUser(user, "User '%s' not logged in\n", text);
      return 0;
    }

  /* Check there is only one match */
  count = userMatch(text);
  if(count > 1) {
  	writeUser(user, "'%s' matches %d users.  Be more precise\n", text, count);
  	return 0;
  }

  if(other == user)
    {
      writeUser(user, "Do you hear an echo?\n");
      return 0;
    }
	
  if(other->status != US_NORM) 
    {
      switch(other->status) 
	{
	case US_BUSY:
	  writeUser(user, "%s is busy and couldn't reply. Try again later.\n", other->name);
	  return 0;
	case US_LOGIN:
	  writeUser(user, "User '%s' not logged in\n", text);
	  return 0;
	case US_TEMP:
	  notifyBug("modules/std.c::cmdDsay() code 0x01", user, other->name);
	  return 0;
	default:
	  notifyBug("modules/std.c::cmdDsay() code 0x02", user, other->name);
	  return 0;
	}
    }
  strcpy(user->lastTell, other->name);

  switch(spc[strlen(spc) - 1]) 
    {
    case '?': 
      what = "ask"; 
      break;
    case '!': 
      what = "exclaim"; 
      break;
    default:  
      what = "say"; 
      break;
    }
  writeUser(user, "~OLYou %s to %s~RS: %s ~RS\n", what, other->name, spc);
  writeUser(other, "~OL%s %ss to you~RS: %s ~RS\n", user->name,what, spc);
  sprintf(textBuffer, "~OL%s %ss to %s~RS: %s ~RS\n", user->name, what, other->name, spc);
  writeRoomExceptTwo(user->room, textBuffer, user,other);
  return 1;

}

signed int cmdExamine(userObject user, char *text, char *call)
{
  userObject other = NULL;

	if (strlen(text) > 0) {
		other = getUser(text);
	
		if(!other) {
      			writeUser(user, "User '%s' not logged in\n", text);
      			return 0;
    		}
	} 
	else {
		other = user;
	}
	
	/* User name & desc */
	writeUser(user, "~UL*** %s %s ***~RS\n\n", other->name, other->desc);
	
	/* User profile, when they're implemented! */
	writeUser(user, "%s~RS\n\n", other->profile);
	
	/* Sex and login times and stuff */
	writeUser(user, "Level       : %s\n", safeUserLevel(other->level));
	
	switch(other->sex) {
		case MALE:
			writeUser(user, "Sex         : Male\n");
			break;
		case FEMALE:
			writeUser(user, "Sex         : Female\n");
			break;
		case UNKNOWN:
			writeUser(user, "Sex         : Unknown\n");
			break;
	}
		
	writeUser(user, "On Since    : %s\n", timeToString(other->lastLogin, 2));
	writeUser(user, "On For      : %s\n", durationToString(other->level, 2));
	writeUser(user, "Idle For    : %s\n", durationToString(other->level, 2));
	writeUser(user, "Total Login : %s\n", durationToString(other->level, 0));
	
	
	return 1;
}

/*  Change User Desc  */
signed int cmdDesc(userObject user, char *text, char *call)
{       
	char* strpointer = NULL;
	
        if(!text)
                /* XXX   
                   Need something here to write to error log, as should never
                   get a NULL text string passed in
                */
                return 0;
                
        /* If we got no new desc display the current one */
        if(text[0] == '\0') {
                writeUser(user, "** Your current description is: %s~RS\n", user->desc);
                return 1;
        }
   
	/* Trim any trailing spaces */
	strpointer = text + strlen(text) - 1;
	while(isspace(*strpointer)) {
		strpointer--;
	}
	*(++strpointer) = '\0';

        /* Check the desc is not larger than allowed */
        if(strlen(text) > (U_BIGLEN - 1)) {
                writeUser(user, "** Description too long. Max length is %d, you tried %d\n", U_BIGLEN - 1, strlen(text));
        }
        else {
                strncpy(user->desc, text, U_BIGLEN - 1);
                writeUser(user, "** Description changed to: %s~FW\n", user->desc);
        }
                
        return 1;
}

signed int cmdCls(userObject user, char *text, char *call)
{
	int count;

	for(count=0;count< user->screenHeight;count++)
	{	
		writeUser(user,"\n");
	}	
	return 1;
}

signed int cmdTopic(userObject user, char *text, char *call)
{
	int ret;
	if (strlen(text) > 0) {
	
		/* Check Permission */
		if(user->level < user->room->lock) {
			writeUser(user, "~FR** Sorry, but the topic is locked at level ~OL%s~RS\n", safeUserLevel(user->room->lock));
		}
		else {
			ret =  change_topic(user->room,text);
			switch(ret) {
				case -1: 
					writeUser(user,"~FR** Sorry, but that topic is to long.~RS\n");
					break;
				case 0:
					writeUser(user,"~FR** Topic succesfully changed.~RS\n");
					break;
				default:
					writeUser(user,"~FR** Erk, something went wrong~RS\n");
			
			}
		}
	}
	else {
		writeUser(user,"~FR** Hey, fool!! What you want to change the topic to?? Huh ??~RS\n");
	}
	return 0;
}

/* Lock the topic */
signed int cmdTlock(userObject user, char *text, char *call)
{
	if(user->room->lock == user->level)
		writeUser(user, "~FR** Topic is already locked at level ~OL%s~RS\n", safeUserLevel(user->room->lock));

	if(user->room->lock > user->level)
		writeUser(user, "~FR** Sorry, topic is locked at level ~OL%s~RS\n", safeUserLevel(user->room->lock));

	if(user->room->lock < user->level) {
		user->room->lock = user->level;
		writeUser(user, "~FR** Topic locked at level ~OL%s~RS\n", safeUserLevel(user->level));
		sprintf(textBuffer, "~FR** %s has locked the topic at level ~OL%s~RS\n", user->name, safeUserLevel(user->level));
		writeRoomExcept(user->room, textBuffer, user);
	}

	return 1;
}

/* Unlock the topic */
signed int cmdTunlock(userObject user, char *text, char *call)
{
	
	/* Check if it's locked already */
	if(user->room->lock == 0) {
		writeUser(user, "~FR** Topic is not locked~RS\n");
	}
	else {
		if(user->room->lock > user->level) {
			writeUser(user, "~FR** Sorry, topic is locked at level ~OL%s~RS\n", safeUserLevel(user->room->lock));
		}
		else {
			user->room->lock = 0;
			writeUser(user, "~FR** Topic unlocked~RS\n");
			sprintf(textBuffer, "~FR** %s has unlocked the topic~RS\n", user->name);
			writeRoomExcept(user->room, textBuffer, user);
		}

	}
	return 1;
}

signed int cmdReview(userObject user, char *text, char *call)
{
	int i;
	char *p;

        writeUser(user,"~BM*** Review buffer for %s ***~RS\n", user->room->name);
	for(i = 0; i < user->room->revbuf->size; i++) {
		if((p = getFromRevbuf(user->room->revbuf, i)))
			writeUser(user, "** %s", p);
	}
	return 1;
}

signed int cmdRevTell(userObject user, char *text, char *call)
{
        int i;
        char *p;

        writeUser(user,"~BM*** Review buffer for your private tells ***~RS\n");
        for(i = 0; i < user->revbuf->size; i++) {
		if((p = getFromRevbuf(user->revbuf, i)))
	                writeUser(user, "** %s", p);
	}

        return 1;
}

/* Command handler while user is locked AFK (i.e. .afk lock) */
int commandAfkLockHandler(userObject user, char *input)
{
        char salt[3], check[30];

	/* Crypt the user's response */
        salt[0] = user->passwd[0];
        salt[1] = user->passwd[1];
        salt[2] = '\0';

        strncpy(check, crypt(input, salt), 30);

	/* If password correct call normal AFK input handler, otherwise warn user */
	if(strcmp(check, user->passwd) == 0) {
		cmdCls(user, "", NULL);
		commandAfkHandler(user, "");
	}
	else {
		writeUser(user, "*** Incorrect password.\n");
		writeUser(user, "*** You are AFK with the session locked, enter your password to unlock it.\n");
	}

	return 1;
}

/* Command handler while user is AFK */
int commandAfkHandler(userObject user, char *input)
{
	writeUser(user, "*** You are no longer AFK.\n");
	user->status = US_NORM;
	user->inputCall = commandInputCall;

	/* Show user is no longer AFK.
	   Show message user went AFK with if they had one */
        if(strlen(user->afkMessage) > 0) {
                sprintf(textBuffer, "*** %s comes back from AFK. (%s)~RS\n", user->name, user->afkMessage);
        }
        else {
                sprintf(textBuffer, "*** %s comes back from AFK.~RS\n", user->name);
        }

        writeRoomExcept(user->room, textBuffer, user);

	if(strlen(input) > 0)
		return commandInputCall(user, input);
	else
		return 1;
}	

signed int cmdAfk(userObject user, char *text, char *call)
{
	int lock = 0;

	/* Set the user AFK */
        user->status = US_AFK; 

	/* Check for lock command */
	if(strlen(text) > 4) {
		lock = !strncasecmp(text, "lock ", 5);
	}
	else {
		lock = !strcasecmp(text, "lock");
	}
	
	if(lock) {
		writeUser(user, "*** You are now AFK with the session locked, enter your password to unlock it.\n");
	  	text += 4;
		user->inputCall = commandAfkLockHandler;
	}
	else {
		writeUser(user, "*** You are now AFK, press <return> to reset.\n");
		user->inputCall = commandAfkHandler;
	}

	/* Set the user's AFK message and send AFK message out */
	strncpy(user->afkMessage, text, U_BIGLEN - 1);

        if(strlen(user->afkMessage) > 0) {
		writeUser(user, "*** AFK Message: %s~RS\n", user->afkMessage);
		sprintf(textBuffer, "*** %s goes AFK : %s~RS\n", user->name, user->afkMessage);
	}
	else {
		sprintf(textBuffer, "*** %s goes AFK...~RS\n", user->name);
	}

	writeRoomExcept(user->room, textBuffer, user);

	return 1;
}

signed int cmdGo(userObject user, char *text, char *call)
{
	roomObject newRoom;

	/* Get the requested room */
	if(newRoom = getRoom(text)) {

		/* Check user can leave current room */
		if(user->level <  user->room->exit) {
			writeUser(user, "*** You are not a high enough level to leave this room!\n");
			return 0;
		}
		
		/* Check user can enter new room */
		if(user->level < newRoom->access) {
			writeUser(user, "*** You are not a high enough level to enter that room!\n");
			return 0;
 		}

                /* Tell the rest of the room they left */
                sprintf(textBuffer, "*** %s leaves here and goes to the room '%s'.~RS\n", user->name, newRoom->name);
                writeRoomExcept(user->room, textBuffer, user);

		/* Move the user to that room */
		user->room = newRoom;
		writeUser(user, "\n\n");
		cmdLook(user, text, NULL);

		/* Tell the rest of the new room they arrived */
	        sprintf(textBuffer, "*** %s enters this room.~RS\n", user->name);
		writeRoomExcept(user->room, textBuffer, user);
	
	}
	else {
		/* There was no match on the room name */
		writeUser(user, "*** There is no such room.~RS\n");
	}

	return 1;
}

/* Move user(s) to specified room */
signed int cmdMove(userObject user, char *text, char *call)
{
	roomObject room;
	userObject person;
	char *proom, *pperson;
	
	/* Get room & user and check we have at least one of each */
	if(!(proom = strtok(text, " ")) || !(pperson = strtok(NULL, " "))) {
		writeUser(user, "*** Usage: .move room user1 [user2] .....~RS\n");
		return 0;
	}

	/* Check we have a valid room */
	if(!(room = getRoom(proom))) {
		writeUser(user, "*** There is no such room~RS\n");
		return 0;
	}

	/* Otherwise we are ready to start moving users */

	/* Display text to room */
	sprintf(textBuffer, "~FT~OL*** %s dreams an ancient dream...~RS\n", user->name);
	writeRoomExcept(user->room, textBuffer, user);
	writeUser(user, "~FT~OL*** You dream an ancient dream...~RS\n");

	/* User moving loop */
	do {
		/* Check we have a real user */
		if(!(person = getUser(pperson))) {
			writeUser(user, "*** %s is not logged on.~RS\n", pperson);
			continue;
		}

		/* See if the user is a higher level */
		if(user->level <= person->level) {
			writeUser(user, "*** You are not a high enough level to move %s.\n", person->name);
			continue;
		}

		/* Check the mover has high enough level to move out of old room */
		if(user->level < person->room->exit) {
			writeUser(user, "*** You are not a high enough level to move %s out of the room,~RS\n", 
				  person->name);
			continue;
		}

		/* Check the mover has high enough level to move to new room */
		if(user->level < room->access) {
			writeUser(user, "*** You are not a high enough level to move %s into that room~RS\n", 
				  person->name);
			continue;
		}
		
		/* Everything looks OK, so move the person */
		sprintf(textBuffer, "~FT~OL*** A voice calls out \"It's You\" and %s vanished with a pop!~RS\n", 
			person->name);
		writeRoomExcept(person->room, textBuffer, person);
		person->room = room;
		sprintf(textBuffer, "~FT~OL*** %s appears with a surprised expression on their face!~RS\n", 
			person->name);
		writeRoomExcept(person->room, textBuffer, person);

		/* Show the user they are now in a new room */
		writeUser(person, "~RS\n\n");
		cmdLook(person, "", NULL);
		
	} while(pperson = strtok(NULL, " "));

	return 1;
}

/* Display / Change users Sex */
signed int cmdSex(userObject user, char *text, char *call)
{
	if(strncasecmp("male", text, 4) == 0) {
		if(user->sex == MALE) {
			writeUser(user, "*** Your sex is already set to Male.\n");
		}			
		else {
			writeUser(user, "*** You have changed your sex to Male.\n");
			sprintf(textBuffer, "*** %s has changed their sex to Male.\n", user->name);
			writeRoomExcept(user->room, textBuffer, user);
			user->sex = MALE;
		}
	}
	else if(strncasecmp("female", text, 6) == 0) {
		if(user->sex == FEMALE) {
			writeUser(user, "*** Your sex is already set to Female.\n");
		}
		else {
			writeUser(user, "*** You have changed your sex to Female.\n");	
			sprintf(textBuffer, "*** %s has changed their sex to Female.\n", user->name);
			writeRoomExcept(user->room, textBuffer, user);
			user->sex = FEMALE;
		}
	}
	else if(strncasecmp("none", text, 7) == 0) {
		if(user->sex == UNKNOWN) {
			writeUser(user, "*** Your sex is already set to Unknown.\n");
		}
		else {
			writeUser(user, "*** You have changed your sex to Unknown!\n");
			sprintf(textBuffer, "*** %s has changed their sex to Unknown.\n", user->name);
			writeRoomExcept(user->room, textBuffer, user);
			user->sex = UNKNOWN;
		}
	}
	else {
		writeUser(user, "*** Usage:  .sex [male | female | none]\n");

	}

	return 1;
}

signed int pwotInit(void)
{
	coEmote      = makeCommand("emote",    UL_USER, cmdEmote,	0);
	coPemote     = makeCommand("pemote",   UL_USER, cmdPemote,	0);
	coWho        = makeCommand("who",      UL_NEW,  cmdWho,		0);
	coTell       = makeCommand("tell",     UL_USER, cmdTell,	0);
	coPeople     = makeCommand("people",   UL_ARCH, cmdPeople,	0);
	coWhere      = makeCommand("where",    UL_USER, cmdWhere,	0);
	coHname      = makeCommand("hname",    UL_ARCH, cmdVerboseName,	0);
	coShout      = makeCommand("shout",    UL_USER, cmdShout,	0);
	coDsay       = makeCommand("dsay",     UL_NEW,  cmdDsay,	0);
	coWizshout   = makeCommand("wizshout", UL_ARCH, cmdWizshout,	0);
	coExamine    = makeCommand("examine",  UL_USER, cmdExamine,	0);
	coDesc       = makeCommand("desc",     UL_USER, cmdDesc,	0);
	coShoutEmote = makeCommand("shemote",  UL_USER, cmdShoutEmote,	0);
	coCls        = makeCommand("cls",      UL_USER, cmdCls,         0);
	coTopic      = makeCommand("topic",    UL_USER, cmdTopic,       0);
	coReview     = makeCommand("review",   UL_USER, cmdReview,      0);
        coRevtell    = makeCommand("revtell",  UL_USER, cmdRevTell,     0);
	coAfk	     = makeCommand("afk",      UL_USER, cmdAfk,         0);
	coGo	     = makeCommand("go",       UL_USER, cmdGo,		0);
	coMove       = makeCommand("move",     UL_ARCH, cmdMove,        0);
	coSex	     = makeCommand("sex",      UL_USER, cmdSex,         0);
	coTlock	     = makeCommand("tlock",    UL_ARCH, cmdTlock,	0);
	coTunlock    = makeCommand("tunlock",  UL_ARCH, cmdTunlock,	0);
	return 1;
}

signed int pwotRemove(void)
{
	deleteCommand(coEmote);
	deleteCommand(coPemote);
	deleteCommand(coWho);
	deleteCommand(coTell);
	deleteCommand(coPeople);
	deleteCommand(coWhere);
	deleteCommand(coHname);
	deleteCommand(coShout);
	deleteCommand(coDsay);
	deleteCommand(coWizshout);
	deleteCommand(coExamine);
	deleteCommand(coDesc);
	deleteCommand(coShoutEmote);
	deleteCommand(coCls);
	deleteCommand(coTopic);
	deleteCommand(coReview);
	deleteCommand(coRevtell);
	deleteCommand(coAfk);
	deleteCommand(coTlock);
	deleteCommand(coTunlock);
	return 1;
}

unsigned int pwotRemovable(void)
{
	return 0;
}
