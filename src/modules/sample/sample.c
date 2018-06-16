/*
   PWoT Talker

   This software may be used and distributed according to the terms
   of the GNU Public License, incorporated herein by reference.
*/

/* 

   PWoT sample module implementation

   This is a really basic module implementation. It shows
   basically how it works. When your module is loaded
   with insmod, pwotInit() is called. This should return
   positive if all is okay, negative if it failed (and the
   module should be unloaded).

   When the module is unloaded, pwotRemove() is called. This
   should return positive if all is okay, negative if it
   failed (in this case, a warning message is shown but the
   module is still closed).

   pwotRemovable() should return 1 if a module can be safely
   unloaded, 0 if not. Unloadable modules are those which
   cannot be done without. Note that a module can be unloaded
   and reloaded even if this is set, but there will be no other
   functions called inbetween the unload and load. Modules
   should be ready for that behavior.

*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "user.h"
#include "command.h"
#include "socket.h"

#ifdef DMALLOC
  #include "dmalloc.h"
#endif

commandObject coSample;

signed int cmdSample(userObject user, char *text, char *call)
{
	writeUser(user, "** This is the sample command.\n");
	writeUser(user, "** You are logged in from %s [%hu.%hu.%hu.%hu]\n", user->siteHostname,
		  user->siteIp[0], user->siteIp[1], user->siteIp[2], user->siteIp[3]);
	return 1;
}

/*
  The login hook. Simple, really
*/
void modLoginHook(userObject user)
{
	writeUser(user, "** This is the sample login hook.\n");
}

/*
  The init command. Sets up the module, in this case adding a
  new command.
*/
signed int pwotInit(void)
{
	coSample = makeCommand("sample", UL_NEW, cmdSample, 0);
	loginHook = modLoginHook;
	return 1;
}

/*
  The remove command. This should undo anything the module does.
  In this case it removes the command we added in the init.
*/
signed int pwotRemove(void)
{
	deleteCommand(coSample);
	loginHook = NULL;
	return 1;
}

/*
  This isn't an important module, so we should let it be
  unloaded. Hence this is set to return 1.
*/
unsigned int pwotRemovable(void)
{
	return 1;
}
