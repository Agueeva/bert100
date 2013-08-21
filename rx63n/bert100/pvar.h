#include "types.h"
typedef struct PVar PVar;
void PVars_Init(void);

/* First the Implementors interface */
typedef void PVar_SetCallback (void *clientData, const char *strP);
typedef void PVar_GetCallback (void *clientData, const char *bufP,uint16_t maxlen);
PVar *PVar_New(PVar_GetCallback *,PVar_GetCallback *,const char *format,...);

/* This is the users interface */
void PVar_Set(PVar *pvar,const char *valStr);
void PVar_Get(PVar *pvar,char *valP, uint16_t maxlen) ;
