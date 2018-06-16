/*
   PWoT Talker

   This software may be used and distributed according to the terms
   of the GNU Public License, incorporated herein by reference.
*/

#ifndef P_MODULE

#define P_MODULE

//#define MODULE_LIST 3
#define MODULE_LIST globalLists[3]

#define M_NORMLEN 80
#define M_BIGLEN  256
struct moduleStruct {
	char name[M_NORMLEN];        /* Module name */
	char file[M_BIGLEN];         /* Module filename */
	void *handle;                         /* Module handle */
	unsigned int removable;               /* Is module removable? */
	unsigned int useCount;                /* How many modules are dependant? */
};
typedef struct moduleStruct* moduleObject;

#define MODULE(a) ((moduleObject)a->object)

#define MOD_INC_USE(a) (MODULE(a)->useCount++)
#define MOD_DEC_USE(a) (MODULE(a)->useCount--)

#ifndef IN_MODULE_C
extern moduleObject newModule(void);
extern void deleteModule(moduleObject);
extern moduleObject getModule(char*);
extern signed int loadModule(moduleObject);
extern signed int unloadModule(moduleObject);
extern signed int reloadModule(moduleObject);
extern signed int initModules(void);
#endif /* IN_MODULE_C */

#endif /* P_MODULE */
