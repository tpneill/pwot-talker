/*
   PWoT Talker

   This software may be used and distributed according to the terms
   of the GNU Public License, incorporated herein by reference.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define IN_EVENT_C
#include "linkedList.h"
#include "event.h"

#ifdef DMALLOC
  #include "dmalloc.h"
#endif

extern time_t sysTime;

eventObject newEvent(void)
{
	eventObject event;

	if((event = (eventObject)malloc(sizeof(struct eventStruct))) == NULL)
		return NULL;

	if(addObject(EVENT_LIST, event) == NULL) {
		free(event);
		return NULL;
	}

	event->name[0] = '\0';
	event->onEvent = NULL;
	event->type = event->min = event->hour = event->day = 0;
	event->time = 0;

	return event;
}

void deleteEvent(eventObject event)
{
	deleteObject(EVENT_LIST, event);
	free(event);
}

eventObject makeEvent(char *name, int type, time_t time, int min, int hour, int day, void (*onEvent)())
{
	eventObject event;

	if((event = newEvent()) == NULL)
		return NULL;

	strcpy(event->name, name);
	event->type = type;
	event->time = time;
	event->min = min;
	event->hour = hour;
	event->day = day;
	event->onEvent = onEvent;

	return event;
}

void eventCheck(void)
{
	linkedListObject list, next;
	eventObject event;
	struct tm *ts;

	ts = localtime(&sysTime);

	if(firstObject(EVENT_LIST) == NULL)
		return;

	for(list = firstObject(EVENT_LIST), next=(list)?NULL:list->next; list != NULL; list = next) {
		next = list->next;
		event = EVENT(list);

		if(sysTime < event->time)
			continue;

		event->onEvent();
		
		switch(event->type) {
		case ET_ONCE:
			deleteEvent(event);
			break;
		case ET_WEEKLY:
			
		case ET_DAILY:
		case ET_HOURLY:
		default:
			continue;
		}
	}
}
