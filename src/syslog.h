/*
   PWoT Talker

   This software may be used and distributed according to the terms
   of the GNU Public License, incorporated herein by reference.
*/

#ifndef P_SYSLOG

#define P_SYSLOG

#define SL_INFO  "<0> "
#define SL_BOOT  "<1> "
#define SL_LOGIN "<2> "
#define SL_IAC   "<3> "
#define SL_DEBUG "<4> "

#ifndef IN_SYSLOG_C
extern void writeSyslog(const char*, ...);
#endif /* IN_SYSLOG_C */

#endif /* P_SYSLOG */
