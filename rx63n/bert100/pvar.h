#include "types.h"
typedef struct PVar PVar;
void PVars_Init(void);

/* First the Implementors interface */
typedef bool PVar_SetCallback (void *clientData,uint32_t adId, const char *strP);
typedef bool PVar_GetCallback (void *clientData,uint32_t adId, char *bufP,uint16_t maxlen);
PVar *PVar_New(PVar_GetCallback *,PVar_SetCallback *,void *cbData,uint32_t adId,const char *format,...);

/* This is the users interface */
bool PVar_Set(PVar *pvar,const char *valStr);
bool PVar_Get(PVar *pvar,char *valP, uint16_t maxlen) ;
PVar * PVar_Find(const char *name);
PVar * PVar_NFind(const char *name,uint16_t maxlen);
