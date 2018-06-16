/*
   PWoT Talker

   This software may be used and distributed according to the terms
   of the GNU Public License, incorporated herein by reference.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#define IN_REVBUF_C
#include "revbuf.h"

#ifdef DMALLOC
  #include "dmalloc.h"
#endif

revbufObject newRevbuf(int size)
{
  revbufObject revbuf;
  int i;

  if((revbuf = (revbufObject)malloc(sizeof(struct revbufStruct))) == NULL)
    return NULL;

  revbuf->size = size;
  revbuf->position = 0;

  revbuf->buffer = (char**)calloc(size, sizeof(char*));
  for(i = 0; i < size; i++) {
    revbuf->buffer[i] = (char*)malloc(sizeof(char) * R_MAXSIZE);
    revbuf->buffer[i][0] = '\0';
  }

  return revbuf;
}

void deleteRevbuf(revbufObject revbuf)
{
  int i;

  for(i = 0; i < revbuf->size; i++)
    free(revbuf->buffer[i]);
  free(revbuf->buffer);
  free(revbuf);
}

void addToRevbuf(revbufObject revbuf, const char *str, ...)
{
	char buffer[4096];
	va_list ap;

	va_start(ap, str);
#ifdef HAS_VSNPRINTF
        vsnprintf(buffer, 4095, str, ap);
#else
        vsprintf(buffer, str, ap);
#endif

	strncpy(revbuf->buffer[revbuf->position], buffer, 255);
	revbuf->buffer[revbuf->position][255] = '\0';

	revbuf->position++;
	if(revbuf->position >= revbuf->size)
		revbuf->position = 0;
}

char* getFromRevbuf(revbufObject revbuf, int position)
{
  position += revbuf->position;
  if(position >= revbuf->size)
    position -= revbuf->size;

  if(revbuf->buffer[position][0] == '\0')
    return NULL;
  else
    return revbuf->buffer[position];
}

#ifdef DEBUG
void dumpRevbuf(revbufObject revbuf)
{
  int i;

  printf("** In memory order **\n");
  for(i = 0; i < revbuf->size; i++)
    printf("%02d - %s\n", i, revbuf->buffer[i]);

  printf("\n** In useful order **\n");
  for(i = 0; i < revbuf->size; i++)
    printf("%02d - %s\n", i, getFromRevbuf(revbuf, i));

  printf("\n");
}
#endif /* DEBUG */
