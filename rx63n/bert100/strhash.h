#ifndef _STRHASH_H
#define _STRHASH_H
#include "types.h"
typedef struct StrHashEntry StrHashEntry;
typedef struct StrHashTable StrHashTable;
/**
 *****************************************************************
 * For walking through hash tables.
 *****************************************************************
 */
typedef struct StrHashSearch {
        uint32_t nr_bucket;     /* The cursor */
        StrHashEntry *cursor;
        StrHashTable *table;
} StrHashSearch;

StrHashEntry *StrHash_CreateEntry(StrHashTable *table,const char *key);
StrHashEntry *StrHash_FindEntry(StrHashTable *table,const char *key);
StrHashEntry *StrNHash_FindEntry(StrHashTable *table,const char *key,uint16_t keylen);
StrHashTable *StrHash_New(void); 
void StrHash_SetValue(StrHashEntry *she,void *value);
void *StrHash_GetValue(StrHashEntry *she);
const char *StrHash_GetKey(StrHashEntry *she);

StrHashEntry * StrHash_FirstEntry(StrHashTable *table,StrHashSearch *search);
StrHashEntry * StrHash_NextEntry(StrHashSearch *search);
#endif
