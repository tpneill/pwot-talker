/*
   PWoT Talker

   This software may be used and distributed according to the terms
   of the GNU Public License, incorporated herein by reference.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#define IN_MISC_C
#include "misc.h"
#include "user.h"
#include "room.h"
#include "socket.h"
#include "syslog.h"
#include "colour.h"

#ifdef DMALLOC
  #include "dmalloc.h"
#endif

void terminateString(char *buf)
{
	while(*buf > 31 || *buf == '\011')
		buf++;
	*buf = '\0';
}

void stringToLower(char *buf)
{
	do
		*buf = tolower(*buf);
	while(*(buf++));
}

void stringToUpper(char *buf)
{
	do
		*buf = toupper(*buf);
	while(*(buf++));
}

char *durationToString(int secs, int type)
{
	static char ret[128], buf[30];
	unsigned int days, minutes, hours, seconds;

	days = secs / 86400;
	hours = (secs - (days * 86400)) / 3600;
	minutes = (secs - ((days * 86400) + (hours * 3600))) / 60;
	seconds = (secs - ((days * 86400) + (hours * 3600) + (minutes * 60)));

	switch(type) {
	case 1:
		sprintf(ret, "%u seconds", secs);
		break;

	case 2:
		sprintf(ret, "%u minutes", (int) (secs / 60));
		break;

	default:
		sprintf(ret, "%d day%s, ", days, (days==1)?"":"s");
		sprintf(buf, "%d hour%s, ", hours, (hours==1)?"":"s");
		strcat(ret, buf);
		sprintf(buf, "%d minute%s, ", minutes, (minutes==1)?"":"s");
		strcat(ret, buf);
		sprintf(buf, "%d second%s", seconds, (seconds==1)?"":"s");
		strcat(ret, buf);
		break;
	}

	return ret;
}

char *timeToString(time_t time, int type)
{
	unsigned static char days[7][10] = { "Sunday", "Monday", "Tuesday", "Wednesday",
					     "Thursday", "Friday", "Saturday" };
	unsigned static char mons[12][10] = { "January", "February", "March", "April", "May",
					      "June", "July", "August", "September",
					      "October", "November", "December" };
	unsigned static char ret[128], small[5];
	struct tm *ts;

	ts = localtime(&time);

	switch(type) {
	case 1: /* Prompt clock type */
		sprintf(ret, "%02d:%02d", ts->tm_hour, ts->tm_min);
		break;

	case 2: /* Board style clock */
		sprintf(ret, "%s %d %s %d, %02d:%02d", days[ts->tm_wday], ts->tm_mday,
			mons[ts->tm_mon], (ts->tm_year + 1900),
			ts->tm_hour, ts->tm_min);
		break;

	case 3: /* HTTP/1.x style */
		sprintf(ret, "%s, %d %s %d %02d:%02d:%02d GMT\n", days[ts->tm_wday],
			ts->tm_mday, mons[ts->tm_mon], (ts->tm_year + 1900),
			ts->tm_hour, ts->tm_min, ts->tm_sec);
		break;

	case 4: /* Syslog clock */
		sprintf(ret, "%02d:%02d:%02d, %02d/%02d/%04d", ts->tm_hour, ts->tm_min,
			ts->tm_sec, ts->tm_mday, ts->tm_mon + 1, (ts->tm_year + 1900));
		break;

	case 5: /* Mail clock */
		strncpy(small, mons[ts->tm_mon], 3);
		small[4] = '\0';
		sprintf(ret, "%s %d", small, ts->tm_mday);
		break;

	default: /* Verbose clock type */
		sprintf(ret, "%s, %d %s %d", days[ts->tm_wday], ts->tm_mday,
			mons[ts->tm_mon], ts->tm_year);
		break;
	}

	return ret;
}

/* 
   Gratuitous complex and undocumented function #1
*/
char *wrapText(char *full, unsigned int width, unsigned int tryCols)
{
	char *ret = NULL, *p, *s;
	unsigned int cp, len;

	len = strlen(full);
	if(!(ret = (char*)malloc(len + 3)))
		return full;

	strcpy(ret, full);

	width -= 2;
	p = ret;
	cp = 0;
	while(*p != '\0') {
		if(*p == '\n' || *p == '\r')
			cp = 0;

		if(cp > width) {
			s = p;
			while(*p != ' ') {
				p--;
				cp--;
				if(cp < 1) {
					p = s;
					cp = 0;
					goto nastyGoto;
				}
			}

			*p = '\n';
			cp = 0;
		}

	nastyGoto:

		p++;
		cp++;
	}

	return ret;
}

void notifyBug(char *function, userObject caller, char *other)
{
	char buffer[256];

	writeSyslog(SL_DEBUG "BUG: %s [%s, %s]",
		    function,
		    (caller)?((char*)caller->name):"<NULL CALLER>",
		    other);
	if(caller)
		writeUser(caller, "** Wow! You've found a bug! [Error track: %s]\n", function);
	sprintf(buffer, "** ~FR~OLBUG:~RS %s [%s, %s]\n",
		function,
		(caller)?((char*)caller->name):"<NULL CALLER>",
		other);
	writeRoomAbove(NULL, buffer, UL_GOD);
}

char *getHostDescription(char *hostname)
{
	FILE *fp;
	int i, l;
	static char buffer[256];

	sprintf(buffer, "%s/%s", "sites", hostname);
	if((fp = fopen(buffer, "r"))) {
		if(fgets(buffer, 255, fp)) {
			terminateString(buffer);
			fclose(fp);
			return buffer;
		} else {
			fclose(fp);
			return "Unknown location";
		}
	}

	l = strlen(hostname);
	for(i = 0; i < l; i++) {
		if(hostname[i] == '.') {
			sprintf(buffer, "%s/%s", "sites", &hostname[(i+1)]);
			if((fp = fopen(buffer, "r"))) {
				if(fgets(buffer, 255, fp)) {
					terminateString(buffer);
					fclose(fp);
					return buffer;
				} else {
					fclose(fp);
					return "Unknown location";
				}
			}
		}
	}

	return NULL;
}

/* Crop the given string to the given size.
   If the string is longer than length then it is cropped
   to the nearest word less than length and "..." is added.
*/
char *CropString(char *text, int length)
{
	char* last_space = NULL;
	char* src = NULL;
	char* dest = NULL;
	static char chopped_text[U_BIGLEN];
	int count = 0;

	if(length < 4)
		length = 4;

	/* If we get a NULL string then ... um ... panic? */
	if(!text)
		return (char *)"";

	/* If the string is already less than or equal to length, return it */
	if(strlen(monoiseString(text)) <= length)
		return text;

	/* If the length is less than 4 set it to 4. (Stupid to have < ~10) */
	if(length < 4)
		length = 4;

	/* Otherwise we have to do this choppy-choppy thing */

	/* Initialise pointers */
	src = text;
	dest = last_space = chopped_text;

	while((count < length - 3) && (*src != '\0')) {
		switch(*src) {

			case ' ':	/* Copy space & update pointer */
				count++;
				last_space = dest;
				*dest++ = *src++;
				break;

			case '~':	/* Copy control invisibly */
				*dest++ = *src++;
				*dest++ = *src++;
				*dest++ = *src++;
				break;

			default:	/* Copy normal char */
				count++;
				*dest++ = *src++;
				break;
		}
				
	} /* End while */
	
	/* Check if there was a space at all */
	if(isspace(*last_space)) {
		strncpy(last_space, "~RS...\0", 7);
	}
	else {
		strncpy(dest, "~RS...\0", 7);
	}

	return chopped_text;
}
