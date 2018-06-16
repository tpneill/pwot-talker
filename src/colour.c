/*
   PWoT Talker

   This software may be used and distributed according to the terms
   of the GNU Public License, incorporated herein by reference.
*/

#include <stdio.h>
#include <string.h>
#define IN_COLOUR_C
#include "colour.h"
#include "user.h"
#include "socket.h"

#ifdef DMALLOC
  #include "dmalloc.h"
#endif

unsigned static char colourBuffer[8192], monoBuffer[8192];

unsigned static char *colours[20][2] = {
	{ "RS", "\033[00m" }, /* Reset */
	{ "OL", "\033[01m" }, /* Bold */
	{ "UL", "\033[04m" }, /* Underline */
	{ "FK", "\033[30m" }, /* Black */
	{ "FR", "\033[31m" }, /* Red */
	{ "FG", "\033[32m" }, /* Green */
	{ "FY", "\033[33m" }, /* Yellow */
	{ "FB", "\033[34m" }, /* Blue */
	{ "FM", "\033[35m" }, /* Magenta */
	{ "FT", "\033[36m" }, /* Turquoise */
	{ "FW", "\033[37m" }, /* White */
	{ "BK", "\033[40m" }, /* Background Black */
	{ "BR", "\033[41m" }, /* Background Red */
	{ "BG", "\033[42m" }, /* Background Green */
	{ "BY", "\033[43m" }, /* Background Yellow */
	{ "BB", "\033[44m" }, /* Background Blue */
	{ "BM", "\033[45m" }, /* Background Magenta */
	{ "BT", "\033[46m" }, /* Background Turquoise */
	{ "BW", "\033[47m" }, /* Background White */
	{ NULL, NULL }        /* List terminator */
};

char *colouriseString(char *str)
{
	char *in, *out;
	int code, i, found;

	in = str;
	out = colourBuffer;

	while(*in != '\0') {
		found = 0;
		if(*in == '\\' && *(in+1) == '~')
			in++;
		else if(*in == '~') {
			for(code = 0; colours[code][0] != NULL; code++) {
				if(strncmp((in+1), colours[code][0], 2) == 0) {
					for(i = 0; i < 5; i++)
						*(out++) = colours[code][1][i];
					in += 3;
					found = 1;
				}
			}
		} else if(*in == '\n')
			*(out++) = '\r';

		if(found == 0)
			*(out++) = *(in++);
	}
	*out = '\0';

	return colourBuffer;
}

char *monoiseString(char *str)
{
	char *in, *out;
	int code, found;

	in = str;
	out = monoBuffer;

	while(*in != '\0') {
		found = 0;
		if(*in == '\\' && *(in+1) == '~')
			in++;
		else if(*in == '~') {
			for(code = 0; colours[code][0] != NULL; code++) {
				if(strncmp((in+1), colours[code][0], 2) == 0) {
					in += 3;
					found = 1;
				}
			}
		} else if(*in == '\n')
			*(out++) = '\r';

		if(found == 0)
			*(out++) = *(in++);
	}
	*out = '\0';

	return monoBuffer;
}
