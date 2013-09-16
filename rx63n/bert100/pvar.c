/**
 * Process variables 
 */

#include "types.h"
#include <stdlib.h>
#include <string.h>
#include "tpos.h"
#include "strhash.h"
#include "iram.h"
#include "pvar.h"
#include "console.h"
#include "interpreter.h"
#include "hex.h"

static bool pvar_debug = 0;
typedef struct PVarTable {
        /* The hash table stores pointers to PVars */
	Mutex lock;
        StrHashTable *varHashTable;
} PVarTable;

struct PVar {
        StrHashEntry *she;
	PVar_SetCallback *setCallback;
	PVar_GetCallback *getCallback;
	void *cbData;
	uint32_t adId;
};

static PVarTable g_PVarTable;

/**
 ***************************************************************************
 * Find a variable by name.
 ***************************************************************************
 */
PVar *
PVar_Find(const char *name)
{
	PVarTable *pvt = &g_PVarTable;
	StrHashEntry *she = StrHash_FindEntry(pvt->varHashTable,name);
	PVar *pvar;
	if(!she) {
		return NULL;
	}
	pvar = StrHash_GetValue(she);
	return pvar;
}

/**
 * Find a PVar by name, only using the first N chars
 */
PVar *
PVar_NFind(const char *name,uint16_t maxlen)
{
	PVarTable *pvt = &g_PVarTable;
	StrHashEntry *she = StrNHash_FindEntry(pvt->varHashTable,name,maxlen);
	PVar *pvar;
	if(!she) {
		return NULL;
	}
	pvar = StrHash_GetValue(she);
	return pvar;
}

/**
 *******************************************************************************
 * \fn PVar * PVar_New(void *dataP,uint16_t maxlen,const char *format,...)
 *******************************************************************************
 */
PVar *
PVar_New(PVar_GetCallback *gcb, PVar_SetCallback *scb,void *cbData,uint32_t adId,const char *format,...)
{
        PVar *pvar;
	char printfBuf[42];
	const char *varname;
        PVarTable *pvt = &g_PVarTable;
        va_list ap;
        StrHashEntry *she;
        va_start(ap,format);
        VSNPrintf(printfBuf,array_size(printfBuf),format,ap);
        va_end(ap);
        pvar = PVar_Find(printfBuf);
	if(strcmp(printfBuf,format) == 0) {
		/* This saves probably from making a copy from flash to RAM */
		varname = format;	
	} else {
		varname = printfBuf;
	}
        if(pvar) {
		Con_Printf("Variable already exists");
                return pvar;
        }
	Mutex_Lock(&pvt->lock);
        she = StrHash_CreateEntry(pvt->varHashTable,varname);
        if(!she) {
                while(1) {
                        Con_Printf("Can not create string hash entry\n");
                }
		Mutex_Unlock(&pvt->lock);
                return NULL;
        }
        pvar = IRam_Calloc(sizeof(PVar));
        pvar->she = she;
        StrHash_SetValue(she,pvar);
	pvar->cbData = cbData;
	pvar->adId = adId;
	pvar->getCallback = gcb;
	pvar->setCallback = scb;
	Mutex_Unlock(&pvt->lock);
        return pvar;
}

#if 0
/**
 *********************************************************************************
 * Configure the callbacks for setting and getting the variable. 
 *********************************************************************************
 */
void
PVar_SetCallbacks(PVar *pvar,PVar_GetCallback *gcb,PVar_SetCallback *scb,void *cbData)
{
	pvar->cbData = cbData;
	pvar->getCallback = gcb;
	pvar->setCallback = scb;
	return;
}
#endif

bool
PVar_Set(PVar *pvar,const char *valStr) 
{
	if(pvar_debug) {
		Con_Printf("%s\n",valStr);
	}
	if(pvar->setCallback) {
		pvar->setCallback(pvar->cbData,pvar->adId,valStr);
		return true;
	} else {
		return false;
	}	
}

bool
PVar_Get(PVar *pvar,char *valP, uint16_t maxlen) 
{
	bool retval;
	valP[0] = 0; /* Terminate the string */
	if(pvar->getCallback) {
		retval = pvar->getCallback(pvar->cbData,pvar->adId,valP,maxlen);
		valP[maxlen - 1] = 0;
		return retval;
	} else {
		return false;
	}
}


const char *exampleText1[] = {
	"\"Kasper\"" , 
	"\"hat\"", 
	"\"Geburtstag\"",
};

const char *exampleText2[] = {
	"\"Alle\"" , 
	"\"meine\"", 
	"\"Entchen\"",
};

const char **exampleText = exampleText1;

static bool
Example_SetCallback (void *clientData, uint32_t adId, const char *strP)
{
	int16_t val = astrtoi16(strP); 
	if(val & 1) {
		exampleText = exampleText1;
	} else {
		exampleText = exampleText2;
	}
	return true;
}

static bool
Example_GetCallback (void *clientData, uint32_t adId, char *bufP,uint16_t maxlen)
{
	static int index = 0;
	strncpy(bufP,exampleText[index],maxlen);
	bufP[maxlen - 1] = 0;
	index = (index + 1) % array_size(exampleText1);
	return true;
}

/**
 **************************************************************************
 * Dump the Names of all variables to the console.
 **************************************************************************
 */
static void
PVars_Dump(void) {
	StrHashSearch hashSearch;
        PVarTable *pvt = &g_PVarTable;
	StrHashEntry * hashEntry;
	Mutex_Lock(&pvt->lock);
	hashEntry = StrHash_FirstEntry(pvt->varHashTable,&hashSearch);
	for(;hashEntry;hashEntry = StrHash_NextEntry(&hashSearch)) {
		const char *key = StrHash_GetKey(hashEntry);
		Con_Printf("%s\n",key);
	}
	Mutex_Unlock(&pvt->lock);
}
/**
 ******************************************************************************
 * \fn static int8_t cmd_pvar(Interp * interp, uint8_t argc, char *argv[])
 * Command shell interface for reading/writing process variables
 ******************************************************************************
 */

static int8_t
cmd_pvar(Interp * interp, uint8_t argc, char *argv[])
{
        PVar *pvar;
        if(argc < 2) {
                return -EC_BADNUMARGS;
        }
	if(strcmp(argv[1],"-dump") == 0) {
		PVars_Dump();
		return 0;
	}
	if(strcmp(argv[1],"-debug") == 0) {
		pvar_debug = 1;
		return 0;
	}
	if(strcmp(argv[1],"-undebug") == 0) {
		pvar_debug = 0;
		return 0;
	}
        pvar = PVar_Find(argv[1]);
        if(pvar) {
                if(argc > 2) {
			PVar_Set(pvar,argv[2]);
                } else {
			char str[40];
			PVar_Get(pvar,str,sizeof(str));
			Interp_Printf_P(interp,"%s\n",str);
                }
        }
        return 0;
}
INTERP_CMD(pvarCmd,"pvar", cmd_pvar, "pvar  <name> # read a pvar");

/**
 ******************************************************************
 * Initialize the Process variable server.
 ******************************************************************
 */
void
PVars_Init(void)
{
        PVarTable *pvt = &g_PVarTable;
	Mutex_Init(&pvt->lock);
        pvt->varHashTable = StrHash_New();
        Interp_RegisterCmd(&pvarCmd);
	PVar_New(Example_GetCallback,Example_SetCallback,NULL,0,"test.var1");
}
