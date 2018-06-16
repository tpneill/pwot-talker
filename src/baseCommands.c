/*
   PWoT Talker

   This software may be used and distributed according to the terms
   of the GNU Public License, incorporated herein by reference.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define IN_BASECOMMANDS_C
#define USERLEVELS
#include "linkedList.h"
#include "baseCommands.h"
#include "user.h"
#include "room.h"
#include "socket.h"
#include "module.h"
#include "command.h"
#include "misc.h"
#include "bgTask.h"
#include "syslog.h"

#ifdef DMALLOC
  #include "dmalloc.h"
#endif

/*

  The format of a command should be :

  signed int commandCall(userObject user, char *text, char *call);

  where :   user = user object of the calling user
            text = full arguments given to the command
            call = the command text given in the call

  The command should return 1 on success, 0 on a trivial error
  or < 0 with a specific error code.

  This also goes for commands in modules.

*/

static char textBuffer[4096];

signed int cmdLook(userObject user, char *text, char *call)
{
	roomObject room;
	linkedListObject list;
	int people = 0;
	char *wrapped;

	if(*text == '\0')
		room = user->room;
	else {
		if(!(room = getRoom(text))) {
			writeUser(user, "No such room.\n");
			return 0;
		}

		if(room->access > user->level) {
			writeUser(user, "No such room.\n");
			return 0;
		}
	}

	writeUser(user, "~BM*** %s ***~RS\n\n", room->label);

	if(room->description) {
		wrapped = wrapText(room->description, user->screenWidth, 0);
		writeUser(user, wrapped);
		free(wrapped);
	}

	for(list = firstObject(USER_LIST); list != NULL; list = list->next) {
		if((USER(list)->room == room)&&(USER(list) != user)) {
			if(people == 0)
				writeUser(user, "\nYou can see : %s %s ~RS\n", USER(list)->name, CropString(USER(list)->desc, user->screenWidth - 16 - strlen(USER(list)->name)));
			else
				writeUser(user, "              %s %s ~RS\n", USER(list)->name, CropString(USER(list)->desc, user->screenWidth - 16 - strlen(USER(list)->name)));
			people++;
		}
	}

	if(people == 0)
		writeUser(user, "\nThere is nobody else here.\n\n");
	else
		writeUser(user, "\n");

	writeUser(user, "~OLTopic~RS : %s ~RS\n\n", room->topic);
	return 1;
}

signed int cmdSay(userObject user, char *text, char *call)
{
	char *emph;

	if(*text == '\0') {
		writeUser(user, "Say what?\n");
		return 0;
	}

	switch(text[strlen(text) - 1]) {
	case '?': emph = "ask"; break;
	case '!': emph = "exclaim"; break;
	default:  emph = "say"; break;
	}

	writeUser(user, "You %s: %s ~RS\n", emph, text);
	sprintf(textBuffer, "%s %ss: %s ~RS\n", user->name, emph, text);
	writeRoomExcept(user->room, textBuffer, user);

	return 1;
}

signed int cmdQuit(userObject user, char *text, char *call)
{
	sprintf(textBuffer, "** ~OL~FRLOGOFF~RS: %s %s ~RS\n", user->name, user->desc);
	writeRoom(NULL, textBuffer);
	if(saveUser(user) < 0)
		writeUser(user, "** ~OL~FRYour userfile could not be saved due to an error!~RS\n");
	disconnectUser(user, 0);
	return 1;
}

signed int cmdInsmod(userObject user, char *text, char *call)
{
	moduleObject module;

	module = newModule();
	strcpy(module->name, text);

	switch(loadModule(module)) {
	case 1:
		writeUser(user, "** ~FG~OLINSMOD~RS: Module '%s' loaded\n", module->name);
		return 1;
	case -5:
		writeUser(user, "** ~FR~OLINSMOD~RS: Module '%s' already loaded\n", module->name);
		deleteModule(module);
		return 0;
	default:
		writeUser(user, "** ~FR~OLINSMOD~RS: Failed to load module '%s'\n", module->name);
		deleteModule(module);
		return 0;
	}
	return 0;
}

signed int cmdRmmod(userObject user, char *text, char *call)
{
	moduleObject module;
	int ret;

	if((module = getModule(text)) == NULL) {
		writeUser(user, "** ~FR~OLRMMOD~RS: No such module '%s' loaded\n", text);
		return 0;
	}

	switch((ret = unloadModule(module))) {
	case -1:
		writeUser(user, "** ~FR~OLRMMOD~RS: Module '%s' is not removable\n", module->name);
		return 0;
		break;

	case -2:
		writeUser(user, "** ~FY~OLRMMOD~RS: Module '%s' unloaded but no pwotRemove() call\n", module->name);
		deleteModule(module);
		return 1;
		break;

	case -3:
		writeUser(user, "** ~FR~OLRMMOD~RS: Module '%s' is in use (module->useCount > 0)\n", module->name);
		return 0;
		break;

	case 1:
		writeUser(user, "** ~FG~OLRMMOD~RS: Module '%s' unloaded\n", module->name);
		deleteModule(module);
		return 1;
		break;
	}

	deleteModule(module);
	writeUser(user, "** ~FR~OLRMMOD~RS: unloadModule() returned unknown value %d\n", ret);
	return 0;
}

signed int cmdLsmod(userObject user, char *text, char *call)
{
	linkedListObject list;
	char *start;
	int i, t, l;

	writeUser(user, "~BM*** Modules currently loaded ***~RS\n\n");

	for(list = firstObject(MODULE_LIST), i = 0; list != NULL; list = list->next, i++) {
		t = (user->screenWidth - (9 + strlen(MODULE(list)->name)));
		if(t < 6)
			writeUser(user, "  %s %u - %s\n", ((MODULE(list)->removable==0)?"~FRU~RS":"~FGR~RS"),
				                            (MODULE(list)->useCount), MODULE(list)->name);
		else {
			t -= 6;
			if(t < (l = strlen(MODULE(list)->file))) {
				start = MODULE(list)->file + (l - t);
				writeUser(user, "  %s %u - %s (...%s)\n",
					  ((MODULE(list)->removable==0)?"~FRU~RS":"~FGR~RS"),
					  MODULE(list)->useCount,
					  MODULE(list)->name, start);
			} else {
				       writeUser(user, "  %s %u - %s (%s)\n",
						 ((MODULE(list)->removable==0)?"~FRU~RS":"~FGR~RS"),
						 MODULE(list)->useCount,
						 MODULE(list)->name, MODULE(list)->file);
			}
		}
	}
	if(i == 0)
		writeUser(user, "  No modules loaded\n\n");
	else
		writeUser(user, "\n");

	return 1;
}

signed int cmdRemod(userObject user, char *text, char *call)
{
	moduleObject module;
	int ret;

	if((module = getModule(text)) == NULL) {
		writeUser(user, "** ~FR~OLREMOD~RS: Module '%s' not loaded\n", text);
		return 0;
	}

	switch((ret = reloadModule(module))) {
	case -5:
		writeUser(user, "** ~FR~OLREMOD~RS: Module file %s is missing!\n", module->file);
		return 0;
		break;
	case -4:
	case -3:
	case -2:
	case -1:
		writeUser(user, "** ~FR~OLREMOD~RS: Weirdness - checked, unloaded but not loadeded back!\n");
		deleteModule(module);
		return 0;
		break;

	case 1:
		writeUser(user, "** ~FG~OLREMOD~RS: Module '%s' reloaded\n", module->name);
		return 1;
		break;
	}

	deleteModule(module);
	writeUser(user, "** ~FR~OLREMOD~RS: reloadModule() returned unknown value %d\n", ret);
	return 0;
}

signed int cmdCommands(userObject user, char *text, char *call)
{
	linkedListObject list;
	char line[256], sb[80];
	int p, numPerLine, lvl;

	writeUser(user, "~BM*** Your commands ***~RS\n\n");

	numPerLine = ((user->screenWidth - 3) / 12) - 1;
	if(numPerLine < 1)
		numPerLine = 1;

	for(lvl = UL_NEW; lvl <= user->level; lvl++) {
		writeUser(user, " ~FY%s~RS\n\n", safeUserLevel(lvl));
		strcpy(line, "   ");
		for(list = firstObject(COMMAND_LIST), p = 0; list != NULL; list = list->next) {
			if(COMMAND(list)->level != lvl)
				continue;
			
			sprintf(sb, "%-11s ", COMMAND(list)->name);
			strcat(line, sb);
			p++;
			if(p > numPerLine) {
				strcat(line, "\n");
				writeUser(user, line);
				strcpy(line, "   ");
				p = 0;
			}
		}
		
		if(p != 0) {
			strcat(line, "\n");
			writeUser(user, line);
		}
		writeUser(user, "\n");
	}
	return 1;
}

int shutdownInputCall(userObject user, char *input)
{
	linkedListObject list, next;
	int i;

	if(*input == 'y' || *input == 'Y') {
		/* begin shutdown */
		writeUser(user, "** ~FR~OLShutting down now ...~RS\n");
		writeRoom(NULL, "** ~FR~OLTalker closing due to explicit shutdown ...~RS\n");
		writeSyslog(SL_BOOT "** Shutdown started by %s", user->name);

		for(list = firstObject(USER_LIST), next = list->next; list != NULL; list = next, next = (list)?list->next:NULL)
			cmdQuit(USER(list), NULL, NULL);

		for(list = firstObject(ROOM_LIST), next = list->next; list != NULL; list = next, next = (list)?list->next:NULL)
			deleteRoom(ROOM(list));

		for(list = firstObject(COMMAND_LIST), next = list->next; list != NULL; list = next, next = (list)?list->next:NULL)
			deleteCommand(COMMAND(list));

		/* Clear up the array of Linked Lists that we know about */
		for(i = 0; i < LINKED_LISTS; i++)
			free(globalLists[i]);	

		writeSyslog(SL_BOOT "** Shutdown complete");
		exit(0);
	} else {
		writeUser(user, "** ~FG~OLAborted~RS\n");
		user->inputCall = commandInputCall;
	}
	return 1;
}

signed int cmdShutdown(userObject user, char *text, char *call)
{
	writeUser(user, "** ~FR~OLAre you sure? This will close the talker!~RS [Y/N]\n");
	user->inputCall = shutdownInputCall;
	return 1;
}

signed int cmdTasks(userObject user, char *text, char *call)
{
	linkedListObject list;
	char *un;
	int p;

	writeUser(user, "~BM*** Background tasks ***~RS\n\n");

	for(list = firstObject(BGTASK_LIST), p = 0; list != NULL; list = list->next, p++) {
		if(BGTASK(list)->callingUser != NULL)
			un = BGTASK(list)->callingUser->name;
		else
			un = "<NO CALLER>";
		writeUser(user, "   %d - %-13s  %-20s (%d)\n", p, un,
			  BGTASK(list)->cmdCall, BGTASK(list)->taskPid);
	}

	writeUser(user, "There are currently %d background tasks\n\n", p);	

	return 1;
}

int initCommands(void)
{
	makeCommand("say",      UL_NEW,  cmdSay,      1);
	makeCommand("look",     UL_NEW,  cmdLook,     1);
	makeCommand("quit",     UL_NEW,  cmdQuit,     1);
	makeCommand("insmod",   UL_GOD,  cmdInsmod,   1);
	makeCommand("rmmod",    UL_GOD,  cmdRmmod,    1);
	makeCommand("lsmod",    UL_ARCH, cmdLsmod,    0);
	makeCommand("remod",    UL_GOD,  cmdRemod,    1);
	makeCommand("commands", UL_NEW,  cmdCommands, 0);
	makeCommand("shutdown", UL_GOD,  cmdShutdown, 1);
	makeCommand("tasks",    UL_ARCH, cmdTasks,    0);
	return 1;
}
