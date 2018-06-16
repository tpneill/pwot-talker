/*
   PWoT Talker

   This software may be used and distributed according to the terms
   of the GNU Public License, incorporated herein by reference.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <unistd.h>
#define IN_MODULE_C
#include "linkedList.h"
#include "module.h"
#include "syslog.h"

#ifdef DMALLOC
  #include "dmalloc.h"
#endif

moduleObject newModule(void)
{
	moduleObject module;

	if((module = (moduleObject)malloc(sizeof(struct moduleStruct))) == NULL)
		return NULL;

	if(addObject(MODULE_LIST, module) == NULL) {
		free(module);
		return NULL;
	}

	module->name[0] = module->file[0] = '\0';
	module->handle = NULL;
	module->removable = 1;
	module->useCount = 0;

	return module;
}

void deleteModule(moduleObject module)
{
	deleteObject(MODULE_LIST, module);
	free(module);
}

moduleObject getModule(char *name)
{
	linkedListObject list;

	for(list = firstObject(MODULE_LIST); list != NULL; list = list->next) {
		if(strcmp(MODULE(list)->name, name) == 0)
			return MODULE(list);
	}

	return NULL;
}

signed int loadModule(moduleObject module)
{
	char fnb[256];
	signed int (*initCall)(), (*removeCall)();
	unsigned int (*removableCall)();

	if(getModule(module->name) != module)
		return -5;

	if(getcwd(fnb, 255) == NULL)
		return -2;

	sprintf(module->file, "%s/%s/mod%s.so", fnb, "modules", module->name);
	if(!(module->handle = dlopen(module->file, RTLD_NOW|RTLD_GLOBAL)))
		return -1;

	initCall = dlsym(module->handle, "pwotInit");
	removeCall = dlsym(module->handle, "pwotRemove");
	removableCall = dlsym(module->handle, "pwotRemovable");

	if((initCall == NULL)||(removeCall == NULL)||(removableCall == NULL)) {
		dlclose(module->handle);
		return -3;
	}

	if(initCall() < 0) {
		dlclose(module->handle);
		return -4;
	}

	module->removable = removableCall();
	writeSyslog(SL_INFO "loadModule: loaded '%s'", module->file);
	
	return 1;
}

signed int unloadModule(moduleObject module)
{
	signed int (*removeCall)();

	if(module->removable == 0)
		return -1;

	if(module->useCount != 0)
		return -3;

	removeCall = dlsym(module->handle, "pwotRemove");
	
	if(removeCall == NULL) {
		dlclose(module->handle);
		return -2;
	}

	removeCall();
	dlclose(module->handle);
	writeSyslog(SL_INFO "unloadModule: unloaded '%s'", module->file);

	return 1;
}

signed int reloadModule(moduleObject module)
{
	signed int (*removeCall)();
	FILE *fp;

	if(!(fp = fopen(module->file, "r")))
		return -5;
	else
		fclose(fp);

	removeCall = dlsym(module->handle, "pwotRemove");
       	if(removeCall != NULL)
		removeCall();

	dlclose(module->handle);
	writeSyslog(SL_INFO "reloadModule: unloaded '%s', now loading ...", module->file);

	return loadModule(module);
}

int initModules(void)
{
	moduleObject module;
	int ret;

	module = newModule();
	strcpy(module->name, "std");

	switch((ret = loadModule(module))) {
	case -4: writeSyslog(SL_INFO "initModules(): initCall() < 0"); break;
	case -3: writeSyslog(SL_INFO "initModules(): some calls NULL"); break;
	case -2: writeSyslog(SL_INFO "initModules(): cwd() failed"); break;
	case -1: writeSyslog(SL_INFO "initModules(): dlopen(\"%s\") failed", module->file); break;
	case  1: writeSyslog(SL_INFO "initModules(): success"); break;
	default: writeSyslog(SL_INFO "initModules(): Unknown return code %d", ret); break;
	}

	if(ret < 0)
		deleteModule(module);

	return 1;
}
