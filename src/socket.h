/*
   PWoT Talker

   This software may be used and distributed according to the terms
   of the GNU Public License, incorporated herein by reference.
*/

#ifndef P_SOCKET

#define P_SOCKET
#define S_PORTS 2

#ifndef IN_SOCKET_C

#ifndef FD_SET
#include <sys/types.h>
#include <unistd.h>
#endif /* FD_SET */

extern unsigned int listenSock[S_PORTS];
extern unsigned int ports[S_PORTS];

extern void (*loginHook)();

extern int initSockets(void);
extern int writeToSocket(int, char*, int);
extern int writeUser(userObject, const char *str, ...);
extern int writeToUser(userObject, const char *str, ...);
extern void setupReadmask(fd_set*);
extern void disconnectUser(userObject, int);
extern int acceptConnection(int, int);
extern void getRealHostname(char*, char*);
extern int handleIac(userObject, unsigned char*, unsigned int);
#endif /* IN_SOCKET_C */

#endif /* P_SOCKET */
