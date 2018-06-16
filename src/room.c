/*
   PWoT Talker

   This software may be used and distributed according to the terms
   of the GNU Public License, incorporated herein by reference.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#define IN_ROOM_C
#include "linkedList.h"
#include "revbuf.h"
#include "room.h"
#include "user.h"
#include "socket.h"
#include "syslog.h"

#ifdef DMALLOC
  #include "dmalloc.h"
#endif

#ifdef LIBXML2
 #define LIBXML_CHILDS	children
 #define LIBXML_ROOT	children
#else  /* LIBXML2 */
 #define LIBXML_CHILDS  childs
 #define LIBXML_ROOT    root
#endif /* LIBXML2 */

/* Much evilness here but useful */
char *globalXmlTmpPtr;
#define XMLGETINT(node, name, ret, def) if((globalXmlTmpPtr = xmlGetProp(node, name))) { \
                                                ret = atoi(globalXmlTmpPtr); \
                                                free(globalXmlTmpPtr); \
                                         } else \
                                                ret = def;

#define XMLGETSTR(node, name, ret, def) if((globalXmlTmpPtr = xmlGetProp(node, name))) { \
                                                strcpy(ret, globalXmlTmpPtr); \
                                                free(globalXmlTmpPtr); \
                                        } else \
                                                strcpy(ret, def);

roomObject newRoom(void)
{
	roomObject room;

	if((room = (roomObject)malloc(sizeof(struct roomStruct))) == NULL)
		return NULL;

	if(addObject(ROOM_LIST, room) == NULL) {
		free(room);
		return NULL;
	}

	room->name[0] = room->label[0] = room->topic[0] = '\0';
	room->description = NULL;
	room->access = room->exit = room->lock = 0;
	room->revbuf = newRevbuf(50);
	return room;
}

void deleteRoom(roomObject room)
{
	deleteRevbuf(room->revbuf);
	deleteObject(ROOM_LIST, room);

	free(room);
}

roomObject getRoom(char *name)
{
	linkedListObject list;
	int len;

	/* Try for an exact match first ... */
	for(list = firstObject(ROOM_LIST); list != NULL; list = list->next) {
		if(strcmp(ROOM(list)->name, name) == 0)
			return ROOM(list);
	}

	/* ... else be less discrete */
	len = strlen(name);
	for(list = firstObject(ROOM_LIST); list != NULL; list = list->next) {
		if(strncasecmp(ROOM(list)->name, name, len) == 0)
			return ROOM(list);
	}

	return NULL;
}

roomObject getDefaultRoom(void)
{
	linkedListObject list;

	/* Try to find the "Default" room */
	for(list = firstObject(ROOM_LIST); list != NULL; list = list->next) {
		if(ROOM(list)->defaultRoom)
			return ROOM(list);
	}

	/* ... Otherwise return the 1st room in the list */
	return	ROOM(firstObject(ROOM_LIST));
}

roomObject loadRoom(char *name)
{
	xmlDocPtr doc;
	xmlNodePtr node;
	roomObject room;
	char fnb[256];
	int ver, active;
	
	/* At least skip warnings for "." and ".." files */
	if((strcmp(".", name) == 0) || (strcmp("..", name) == 0))
	   return NULL;

	/* Check we have a XML document */
  	sprintf(fnb, "%s/%s", "rooms", name);
  	if(!(doc = xmlParseFile(fnb))) {
		writeSyslog(SL_DEBUG "** loadRoom(): Unable to parse file %s/%s", "rooms", name);
  		return NULL; 
	}
	
	/* Check we have a roomObject tag */
  	if(strcmp(doc->LIBXML_ROOT->name, "roomObject") != 0) {
  		xmlFreeDoc(doc);
  		return NULL;
  	}
	
	/* Check if the room is active */
	XMLGETINT(doc->LIBXML_ROOT, "active", active, 0);
	if(active < 1) {
		writeSyslog(SL_INFO "** loadRoom(): Room %s/%s is not set active", "rooms", name);
		xmlFreeDoc(doc);
		return NULL;
	}

	/* Create a new room */
	if((room = newRoom()) == NULL)
		return NULL;
	
	/* Set the room name */
	strcpy(room->name, name);
	
	/* Load room values */
	XMLGETSTR(doc->LIBXML_ROOT, "label", room->label, "!!");
	XMLGETINT(doc->LIBXML_ROOT, "defaultRoom", room->defaultRoom, 0);
	XMLGETINT(doc->LIBXML_ROOT, "version", ver, 0);
	

	/* Get the rest of the room values */
	for(node = doc->LIBXML_ROOT->LIBXML_CHILDS; node != NULL; node = node->next) {
		if(strcmp(node->name, "description") == 0) {
			room->description = xmlNodeGetContent(node);
		} else if(strcmp(node->name, "details") == 0) {
			XMLGETINT(node, "access", room->access, 0);
			XMLGETINT(node, "exit", room->exit, 0);
			XMLGETINT(node, "lock", room->lock, 0);
			XMLGETSTR(node, "topic", room->topic, "Unset");
		} 
	}
	
  	xmlFreeDoc(doc);
  	return room;
}

int saveRoom(roomObject room)
{
	/* Not implemented and probably won't be */
	return -1;
}

void writeRoomExceptTwo(roomObject room, char *text, userObject notMe, userObject orMe)
{
	linkedListObject list;
	
	if (room != NULL)
	{
		 addToRevbuf(room->revbuf,text);
	}

	for(list = firstObject(USER_LIST); list != NULL; list = list->next) {
		if(((USER(list)->room == room) || (room == NULL))
		    && (USER(list) != notMe) && (USER(list) != orMe)) {
			if(USER(list)->status == US_NORM)
				writeUser(USER(list), text);
		}
	}
}

void writeRoomExcept(roomObject room, char *text, userObject notMe)
{
	writeRoomExceptTwo(room, text, notMe, NULL);
}

void writeRoomAboveExcept(roomObject room, char *text, int level,userObject notMe)
{
	linkedListObject list;
	
	if (room != NULL)
        {
                 addToRevbuf(room->revbuf,text);
        }

	for(list = firstObject(USER_LIST); list != NULL; list = list->next) 
	  {
	    if(((USER(list)->room == room) || (room == NULL)) && (USER(list)->level >= level) && USER(list) != notMe) 
	      {
		if(USER(list)->status == US_NORM)
		  writeUser(USER(list), text);
	      }
	  }
}

void writeRoomAbove(roomObject room, char *text, int level)
{
  writeRoomAboveExcept(room,text,level,NULL);
}

void writeRoomBelow(roomObject room, char *text, int level)
{
	linkedListObject list;

	if (room != NULL)
        {
                 addToRevbuf(room->revbuf,text);
        }

	for(list = firstObject(USER_LIST); list != NULL; list = list->next) {
		if(((USER(list)->room == room) || (room == NULL)) && (USER(list)->level < level)) {
			if(USER(list)->status == US_NORM)
				writeUser(USER(list), text);
		}
	}
}

void writeRoom(roomObject room, char *text)
{
	writeRoomExceptTwo(room, text, NULL, NULL);
}

int initRooms(void)
{
	roomObject room;
	struct dirent **namelist;
	int n;
	int loadedRooms = 0;
	
	n = scandir("rooms/", &namelist, 0, 0);

	/* Loop thought all files in the dir */
	while(n--) {
		/* If it's a "regular file" try to load it as a room */
		room = loadRoom(namelist[n]->d_name);
		
		if(room) {
			writeSyslog(SL_INFO "initRooms(): loaded room '%s/%s'", "rooms", namelist[n]->d_name);
			loadedRooms++;
		}
	}
	
	/* Make a default room if no valid room was loaded */
	if(loadedRooms == 0) {
		writeSyslog(SL_DEBUG "** initRooms(): No valid room files. Creating default");
		
		/* Create default main room */
		if(!(room = newRoom()))
			return -1;
		
		strcpy(room->name, "main");
		strcpy(room->label, "Main room");
		strcpy(room->topic, "No topic.");
		room->defaultRoom = 1;
	}

	return 1;
}

int change_topic(roomObject room, char *text)
{
	if (strlen(text) > R_BIGLEN)
	{
		return -1;
	}
	else
	{
	 	strcpy(room->topic,text);
		return 0;
	}
}

