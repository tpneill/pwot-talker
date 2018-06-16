/*
   PWoT Talker

   This software may be used and distributed according to the terms
   of the GNU Public License, incorporated herein by reference.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#define IN_USER_C
#include "linkedList.h"
#include "revbuf.h"
#include "user.h"
#include "socket.h"
#include "misc.h"
#include "colour.h"
#include "syslog.h"
#include "command.h"

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
userObject newUser(void)
{
	userObject user;

	if((user = (userObject)malloc(sizeof(struct userStruct))) == NULL)
		return NULL;

	if(addObject(USER_LIST, user) == NULL) {
		free(user);
		return NULL;
	}

	user->name[0] = user->passwd[0] = user->siteHostname[0] = '\0';
	user->email[0] = user->url[0] = user->desc [0] = '\0';
	user->lastTell[0] = user->afkMessage[0] = user->siteDesc[0] = '\0';
	user->siteIp[0] = user->siteIp[1] = user->siteIp[2] = user->siteIp[3] = '\0';
        user->currentIac = user->currentIacSb[0] = user->termType[0] = '\0';
        user->currentIacPos = 0;
	user->socket = user->lastLogin = user->lastLoginDuration = 0;
	user->loginTime = user->totalTime = user->prompt = user->flags = 0;
	user->level = user->lastInput = user->inputsPerSec = 0;
	user->screenWidth = 80;
	user->screenHeight = 24;
	user->data = user->room = NULL;
	user->revbuf = newRevbuf(30);
	user->sex = UNKNOWN;
	user->profile = NULL;
        /* user->message = NULL; */
	return user;
}

void deleteUser(userObject user)
{

	deleteObject(USER_LIST, user);

	deleteRevbuf(user->revbuf);

	if(user->data)
		free(user->data);

	if(user->profile)
		free(user->profile);

	free(user);
}

/*
  Returns a count of users who could match given text string
*/
int userMatch(char *name)
{
	int count = 0;
	int len = strlen(name);
	linkedListObject list;

	for(list = firstObject(USER_LIST); list != NULL; list = list->next) {
		if(strncasecmp(USER(list)->name, name, len) == 0) {
			/* If the entire username was passed in then it's
			   an exact match and this should only match the one user */
			if(strlen(USER(list)->name) == len)
				return 1;
			count++;
		}
	}

	return count;
}

userObject getUser(char *name)
{
	linkedListObject list;
	int len;

	/* Try for an exact match first ... */
	for(list = firstObject(USER_LIST); list != NULL; list = list->next) {
		if(strcmp(USER(list)->name, name) == 0) {
			return USER(list);
		}
	}

	/* ... else be less discrete */
	len = strlen(name);
	for(list = firstObject(USER_LIST); list != NULL; list = list->next) {
		if(strncasecmp(USER(list)->name, name, len) == 0) {
			return USER(list);
		}
	}

	return NULL;
}

signed int loadUser(userObject user)
{
	xmlDocPtr doc;
	xmlNodePtr node;
	char fnb[256];
	int ver;

	sprintf(fnb, "%s/%s", "users", user->name);
	if(!(doc = xmlParseFile(fnb)))
		return -1;

	if(strcmp(doc->LIBXML_ROOT->name, "userObject") != 0) {
		xmlFreeDoc(doc);
		return -1;
	}

	XMLGETSTR(doc->LIBXML_ROOT, "name", user->name, "!!");
	XMLGETSTR(doc->LIBXML_ROOT, "passwd", user->passwd, "!!");
	XMLGETINT(doc->LIBXML_ROOT, "lastLogin", user->lastLogin, 0);
	XMLGETINT(doc->LIBXML_ROOT, "loginTime", user->loginTime, 0);
	XMLGETINT(doc->LIBXML_ROOT, "totalTime", user->totalTime, 0);
	XMLGETINT(doc->LIBXML_ROOT, "version", ver, 0);

	for(node = doc->LIBXML_ROOT->LIBXML_CHILDS; node != NULL; node = node->next) {
		if(strcmp(node->name, "profile") == 0) {
			user->profile = xmlNodeGetContent(node); 
		} else if(strcmp(node->name, "net") == 0) {
			XMLGETSTR(node, "email", user->email, "Unset");
			XMLGETSTR(node, "url", user->url, "Unset");
			XMLGETSTR(node, "lastHostname", user->siteHostname, "Unknown");
		} else if(strcmp(node->name, "details") == 0) {
			XMLGETSTR(node, "desc", user->desc, "Unset");
			XMLGETINT(node, "status", user->status, 0);
			XMLGETINT(node, "prompt", user->prompt, 0);
			XMLGETINT(node, "flags", user->flags, 0);
			XMLGETINT(node, "level", user->level, 0);
			XMLGETINT(node, "sex", user->sex, 0);
		}
	}

	xmlFreeDoc(doc);
	return 1;
}

void myXmlSetProp(xmlNodePtr node, char *name, unsigned int val)
{
	char buffer[32];
	sprintf(buffer, "%d", val);
	xmlSetProp(node, name, buffer);
}

signed int saveUser(userObject user)
{
	FILE *fp;
	char fnb[256];
	xmlDocPtr doc;
	xmlNodePtr node;

	sprintf(fnb, "%s/%s", "users", user->name);

	doc = xmlNewDoc("1.0");
	node = doc->LIBXML_ROOT = xmlNewDocNode(doc, NULL, "userObject", NULL);
	xmlSetProp(node, "name", user->name);
	xmlSetProp(node, "passwd", user->passwd);
	myXmlSetProp(node, "lastLogin", user->lastLogin);
	myXmlSetProp(node, "loginTime", user->loginTime);
	myXmlSetProp(node, "totalTime", user->totalTime);
	myXmlSetProp(node, "version", 1);
	
	node = xmlNewChild(doc->LIBXML_ROOT, NULL, "profile", user->profile);

	node = xmlNewChild(doc->LIBXML_ROOT, NULL, "net", NULL);
	xmlSetProp(node, "email", user->email);
	xmlSetProp(node, "url", user->url);
	xmlSetProp(node, "lastHostname", user->siteHostname);

	node = xmlNewChild(doc->LIBXML_ROOT, NULL, "details", NULL);
	myXmlSetProp(node, "status", user->status);
	myXmlSetProp(node, "prompt", user->prompt);
	myXmlSetProp(node, "flags", user->flags);
	myXmlSetProp(node, "level", user->level);
	myXmlSetProp(node, "sex", user->sex);
	xmlSetProp(node, "desc", user->desc);


	if(!(fp = fopen(fnb, "w"))) {
		notifyBug("user.c::saveUser() code 0x05", user, "-");
		writeUser(user, "Your userfile has not been saved. Notify an administrator on your next visit.\n");
		xmlFreeDoc(doc);
		return -1;
	}
	xmlDocDump(fp, doc);
	fclose(fp);
	xmlFreeDoc(doc);

	return 1;
}

/** set inputCall to 'call' and put the previous inputCall at the end of the 
 inputHandlers. 'status' is preserved as the 'priority'  of the linkedListObject.
*/
signed int pushInputCall(userObject user, signed int(*call)(), int status)
{
  user->status=status;
  user->inputCall=call;
  return 1;
}

/*Set the inputCall to the inputCall at the end of the inputHandlers list.
 Also set the status to the priority of the last linkedListObject in the list.
*/
extern signed int popInputCall(userObject user)
{
  user->status=US_NORM;
  user->inputCall=commandInputCall;
  return 1;      
}

void userPrompt(userObject user)
{
  writeUser(user,"%s>\n",user->name);
}


