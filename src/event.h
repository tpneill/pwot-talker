/*
   PWoT Talker

   This software may be used and distributed according to the terms
   of the GNU Public License, incorporated herein by reference.
*/

#ifndef P_EVENT

#define P_EVENT

#ifndef _TIME_H
#include <time.h>
#endif /* _TIME_H */

#define ET_HOURLY 0
#define ET_DAILY  1
#define ET_WEEKLY 2
#define ET_ONCE   3

//#define EVENT_LIST 5
#define EVENT_LIST globalLists[5]

#define E_NORMLEN 256
#define E_BIGLEN 512
struct eventStruct {
	char name[E_NORMLEN]; /* Name of event */
	unsigned int type;             /* Type of event (hourly, daily etc.) */
	time_t time;                   /* Time event should occur */
        int min, hour, day;             /* For regular events */
	void (*onEvent)();             /* What to call on event time */
};
typedef struct eventStruct* eventObject;

#define EVENT(a) ((eventObject)a->object)

#ifndef IN_EVENT_C
extern eventObject newEvent(void);
extern void deleteEvent(eventObject);
extern eventObject makeEvent(char*, int, time_t, int, int, int, void (*onEvent)());
extern void eventCheck(void);
#endif /* IN_EVENT_C */

#endif /* P_EVENT */
