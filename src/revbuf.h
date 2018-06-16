/*
   PWoT Talker

   This software may be used and distributed according to the terms
   of the GNU Public License, incorporated herein by reference.
*/

#ifndef P_REVBUF

#define P_REVBUF

#define R_MAXSIZE 256
struct revbufStruct {
  unsigned int size;
  unsigned int position;
  char **buffer;
};
typedef struct revbufStruct* revbufObject;

#ifndef IN_REVBUF_C
extern revbufObject newRevbuf(int);
extern void deleteRevbuf(revbufObject);
extern void addToRevbuf(revbufObject, const char *str, ...);
extern char* getFromRevbuf(revbufObject, int);
#ifdef DEBUG
extern void dumpRevbuf(revbufObject);
#endif /* DEBUG */
#endif /* IN_REVBUF_C */

#endif /* P_REVBUF */
