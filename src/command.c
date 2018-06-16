/*
   PWoT Talker

   This software may be used and distributed according to the terms
   of the GNU Public License, incorporated herein by reference.
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#define IN_COMMAND_C
#include "linkedList.h"
#include "revbuf.h"
#include "command.h"
#include "user.h"
#include "socket.h"
#include "syslog.h"

#ifdef DMALLOC
  #include "dmalloc.h"
#endif

commandObject newCommand(char* name)
{
	commandObject command;

	if((command = (commandObject)malloc(sizeof(struct commandStruct))) == NULL)
		return NULL;

	/* Set the command name (So sorting works) */
	if(strlen(name) < 1)
		command->name[0] = '\0';
	else
		strcpy(command->name, name);

	if(addObject(COMMAND_LIST, command) == NULL) {
		free(command);
		return NULL;
	}

	command->call = NULL;
	command->level = command->defLevel = command->unmovable = 0;

	return command;
}

void deleteCommand(commandObject command)
{
	deleteObject(COMMAND_LIST, command);
	free(command);
}

commandObject getCommand(char *name, userObject user)
{
	linkedListObject list;
	int len;

	/* Try for an exact match first ... */
	for(list = firstObject(COMMAND_LIST); list != NULL; list = list->next) {
		if(COMMAND(list)->level <= user->level) {
			if(strcmp(COMMAND(list)->name, name) == 0)
				return COMMAND(list);
		}
	}

	/* ... else be less discrete */
	if((len = strlen(name)) > C_NORMLEN)
		return NULL;

	for(list = firstObject(COMMAND_LIST); list != NULL; list = list->next) {
		if(COMMAND(list)->level <= user->level) {
			if(strncasecmp(COMMAND(list)->name, name, len) == 0)
				return COMMAND(list);
		}
	}

	return NULL;
}

commandObject makeCommand(char *name, int level, int (*call)(), int unmovable)
{
	commandObject command;

	if((command = newCommand(name)) == NULL)
		return NULL;

	command->level = level;
	command->defLevel = level;
	command->call = call;
	command->unmovable = unmovable;

	return command;
}

int unknownCommand(userObject user, char *input)
{
	writeUser(user, "Unknown command '%s'\n", input);
	return 1;
}

int commandInputCall(userObject user, char *input)
{
	static char shortcuts[6][2][12] = {
		{ ";",   "emote" },
		{ ">",   "tell" },
		{ "<",   "pemote" },
		{ ",",   "dsay" },
		{ "?",   "commands"},
		{ "",    "say" }     /* Default command */
	};
	commandObject command;
	char *p;
	int i;

	if(*input == '.') { /* Real command */
		for(p = (input++); (!isspace(*p))&&(*p != '\0'); p++);
		if(*p != '\0') {
			*(p++) = '\0';
			for(; (isspace(*p))&&(*p != '\0'); p++);
		}

		if(!(command = getCommand(input, user)))
			return unknownCommand(user, input);

		return command->call(user, p, input);
	} else { /* Shortcut? */
		for(i = 0; shortcuts[i][0][0] != '\0'; i++) {
			if(input[0] == shortcuts[i][0][0]) {
				if(!(command = getCommand(shortcuts[i][1], user)))
					return unknownCommand(user, input);

				if(input[1] == shortcuts[i][0][0]) /* Catch >>, << and ,, */
					for(p = (input + 2); (isspace(*p))&&(*p != '\0'); p++);
				else
					for(p = (input + 1); (isspace(*p))&&(*p != '\0'); p++);

				return command->call(user, p, input);
			}
		}
		if(!(command = getCommand(shortcuts[i][1], user))) /* No default command? Erk! */
			return unknownCommand(user, input);
		
		return command->call(user, input, input);
	}
}

/* Simple "Less Than" Function used in the Command List
   Allows the commands to be stored in Alphabetical Order
*/
int commandListSort(commandObject cmdA, commandObject cmdB)
{
	if(strcmp(cmdA->name, cmdB->name) < 0)
		return 1;
	return 0;
}
