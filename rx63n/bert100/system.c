/**
 * Process variable interface to the system
 */
#include "interpreter.h"
#include "types.h"
#include "pvar.h"

static bool
PVExecScript_Set(void *cbData, uint32_t adId, const char *strP)
{
	Interp_StartScript(strP);
	return true;
}


void
SystemIf_Init(void) {
	PVar_New(NULL,PVExecScript_Set,NULL,0,"system.execScript");
}
