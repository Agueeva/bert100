#ifndef _STRHASH_H
#define _STRHASH_H
#include "types.h"
typedef struct StrHashEntry StrHashEntry;
typedef struct StrHashTable StrHashTable;
typedef struct StrHashSearch StrHashSearch;
StrHashEntry *StrHash_CreateEntry(StrHashTable *table,const char *key);
StrHashEntry *StrHash_FindEntry(StrHashTable *table,const char *key);
StrHashEntry *StrNHash_FindEntry(StrHashTable *table,const char *key,uint16_t keylen);
StrHashTable *StrHash_New(void); 
void StrHash_SetValue(StrHashEntry *she,void *value);
void *StrHash_GetValue(StrHashEntry *she);
const char *StrHash_GetKey(StrHashEntry *she);
#endif
