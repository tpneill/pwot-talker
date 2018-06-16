/*
   PWoT Talker

   This software may be used and distributed according to the terms
   of the GNU Public License, incorporated herein by reference.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#define IN_SYSLOG_C
#include "revbuf.h"
#include "syslog.h"
#include "misc.h"

#ifdef DMALLOC
  #include "dmalloc.h"
#endif

static char *syslogNames[5] = { "info", "boot", "login", "iac", "debug" };

void writeSyslog(const char *str, ...)
{
	va_list ap;
#ifndef DEBUG
	FILE *fp;
#endif /* DEBUG */
	char buffer[256];

	va_start(ap, str);

	if(*str == '<' && *(str+2) == '>') {
		sprintf(buffer, "syslog/%s", syslogNames[(*(str+1) - '0')]);
		str += 4;
	} else
		sprintf(buffer, "syslog/%s", syslogNames[0]);

#ifdef DEBUG
	printf("%s: ", buffer);
	vprintf(str, ap);
	puts("");
#else
	if((fp = fopen(buffer, "a"))) {
		fprintf(fp, "[%s] ", timeToString(time(0), 4));
		vfprintf(fp, str, ap);
		fprintf(fp, "\n");
		fclose(fp);
	}
#endif /* DEBUG */

        va_end(ap);
}
	
