/*
   PWoT Talker

   This software may be used and distributed according to the terms
   of the GNU Public License, incorporated herein by reference.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#define IN_BGTASK_C
#include "linkedList.h"
#include "bgTask.h"
#include "user.h"
#include "misc.h"
#include "socket.h"
#include "syslog.h"

#ifdef DMALLOC
  #include "dmalloc.h"
#endif

unsigned int spawnedTotal = 0;
unsigned int spawnedNow = 0;

signed int spawnTask(userObject user, char *command, char **args, void (*callback)())
{
	bgTaskObject task;

	if((task = (bgTaskObject)malloc(sizeof(struct bgTaskStruct))) == NULL)
		return -1;

	if(addObject(BGTASK_LIST, task) == NULL) {
		free(task);
		return -2;
	}

	strcpy(task->cmdCall, command);
	sprintf(task->tmpFile, "/tmp/.pwot.%d.%d-%d", getpid(), rand(), rand());
	task->callingUser = user;
	task->taskCallback = callback;

	switch(task->taskPid = fork()) {
	case -1:
		deleteObject(BGTASK_LIST, task);
		free(task);
		return -4;
	case 0: /* new task */		
		if((task->tmpFileHandle = open(task->tmpFile, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR)) == -1) {
			exit(-1);
		}
		dup2(task->tmpFileHandle, 0);
		dup2(task->tmpFileHandle, 1);
		dup2(task->tmpFileHandle, 2);
		execv(task->cmdCall, args);
		/* If we get here ... well, it failed */
		exit(-1);
		break;
	default:
		spawnedTotal++;
		spawnedNow++;
		break;
	}

	return 1;
}

void genericCallback(bgTaskObject task, char *data, unsigned int len)
{
	if(data == NULL || len < 1) {
		if(task->callingUser)
			writeUser(task->callingUser, "** Task %s returned null\n", task->cmdCall);
		else
			writeRoomAbove(NULL, "** A task returned null data\n", UL_ARCH);
	} else {
		if(task->callingUser) {
			writeUser(task->callingUser, "** Task %s returned %d bytes. Output follows\n",
				  task->cmdCall, len);
			writeUser(task->callingUser, "%s\n", data);
			writeUser(task->callingUser, "** EOF\n");
		} else {
			writeRoomAbove(NULL, "** A task returned. Output follows :\n", UL_ARCH);
			writeRoomAbove(NULL, data, UL_ARCH);
			writeRoomAbove(NULL, "** EOF\n", UL_ARCH);
		}
	}
}

/* A pointless function? Far from it */
void dontCareCallback(bgTaskObject task, char *data, unsigned int len)
{
	return;
}

void taskCheck(void)
{
	linkedListObject list, next;
	bgTaskObject task;
	int status, fd;
	pid_t pret;
	struct stat s;
	char *buffer;
	unsigned int bufferLen;

	if(firstObject(BGTASK_LIST) == NULL)
		return;

	for(list = firstObject(BGTASK_LIST), next=(list)?NULL:list->next; list != NULL; list = next) {
		next = list->next;
		task = BGTASK(list);

		pret = waitpid(task->taskPid, &status, WNOHANG|WUNTRACED);

		if(pret == task->taskPid) { /* It exited */
//			Doesn't this being commented out leak FDs?
//			close(task->tmpFileHandle);
			writeSyslog(SL_DEBUG "Task %s (pid %d) returned %d for %s",
				    task->cmdCall, task->taskPid, WEXITSTATUS(status),
				    (task->callingUser)?(char*)(task->callingUser->name):"<noone>");

			if((fd = open(task->tmpFile, O_RDONLY)) != -1) {
				if(fstat(fd, &s) != 0) {
					close(fd);
					buffer = NULL;
					bufferLen = 0;
				} else {
					buffer = (char*)malloc((size_t) (s.st_size + 1));
					if(!buffer)
						break;
					bufferLen = read(fd, buffer, s.st_size);
					close(fd);
				}
			} else {
				buffer = NULL;
				bufferLen = 0;
			}
			task->taskCallback(BGTASK(list), buffer, bufferLen);
			if(buffer)
				free(buffer);
			unlink(task->tmpFile);
			if(task->callingUser)
				task->callingUser->status = US_NORM;
			deleteObject(BGTASK_LIST, task);
			free(task);

		} else if(pret == -1) {
			writeSyslog(SL_DEBUG "waitpid() on task %s (pid %d) returned -1. Killing",
				    task->cmdCall, task->taskPid);
			kill(task->taskPid, SIGKILL);
			deleteObject(BGTASK_LIST, task);
			free(task);
		}
	}
}
