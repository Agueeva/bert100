/**
 * Process variables 
 */

#include "types.h"
#include <stdlib.h>
#include <string.h>
#include "strhash.h"
#include "iram.h"
#include "pvar.h"
#include "console.h"
#include "interpreter.h"
#include "hex.h"

typedef struct PVarTable {
        /* The hash table stores pointers to PVars */
        StrHashTable *varHashTable;
} PVarTable;

struct PVar {
        StrHashEntry *she;
	//void (*setProc) (void *clientData, const char *valStr);
        //void (*getProc) (void *clientData, char *valStr,uint16_t maxlen);
	void *eventData;
};

static PVarTable g_PVarTable;

#if 0
static void
DebugVar_SetFromStr(DebugVar * dvar, char *str)
{
        uint64_t uu;
        int64_t ii;
        double d;
        sscanf(str, "%" PRId64, &ii);
        sscanf(str, "%" PRIu64, &uu);
        sscanf(str, "%lf", &d);
        switch (dvar->type) {
            case DBGT_UINT8_T:
                    *(uint8_t *) dvar->dataP = uu;
                    break;
            case DBGT_UINT16_T:
                    *(uint16_t *) dvar->dataP = uu;
                    break;
            case DBGT_UINT32_T:
                    *(uint32_t *) dvar->dataP = uu;
                    break;
            case DBGT_UINT64_T:
                    *(uint64_t *) dvar->dataP = uu;
                    break;
            case DBGT_INT8_T:
                    *(int8_t *) dvar->dataP = ii;
                    break;
            case DBGT_INT16_T:
                    *(int16_t *) dvar->dataP = ii;
                    break;
            case DBGT_INT32_T:
                    *(int32_t *) dvar->dataP = ii;
                    break;
            case DBGT_INT64_T:
                    *(int64_t *) dvar->dataP = ii;
                    break;
            case DBGT_DOUBLE_T:
                    *(double *)dvar->dataP = d;
                    break;
            case DBGT_PROC64_T:
                    if (dvar->setProc) {
                            dvar->setProc(dvar->clientData, dvar->arg, uu);
                    }
                    break;
            default:
                    fprintf(stderr, "Variable has an illegal type\n");
                    break;
        }
}
#endif

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

PVar *
PVar_New(const char *format,...)
{
        PVar *pvar;
	char printfBuf[42];
        PVarTable *pvt = &g_PVarTable;
        va_list ap;
        StrHashEntry *she;
        va_start(ap,format);
        VSNPrintf(printfBuf,array_size(printfBuf),format,ap);
        va_end(ap);
        pvar = PVar_Find(printfBuf);
        if(pvar) {
		Con_Printf("Variable already exists");
                return pvar;
        }
        she = StrHash_CreateEntry(pvt->varHashTable,printfBuf);
        if(!she) {
                while(1) {
                        Con_Printf("Can not create string hash entry\n");
                }
                return NULL;
        }
        pvar = IRam_Calloc(sizeof(PVar));
        pvar->she = she;
        StrHash_SetValue(she,pvar);
        return pvar;
}

static int8_t
cmd_pvar(Interp * interp, uint8_t argc, char *argv[])
{
        PVar *pvar;
        if(argc < 2) {
                return -EC_BADNUMARGS;
        }
        pvar = PVar_Find(argv[1]);
        if(pvar) {
                if(argc > 2) {
        //                PVar_Set(pvar,argv[2]);
                } else {
        //               Interp_Printf_P(interp,"%s\n",PVar_Get(pvar));
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
        pvt->varHashTable = StrHash_New();
        Interp_RegisterCmd(&pvarCmd);
}
