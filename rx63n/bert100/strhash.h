#ifndef _STRHASH_H
#define _STRHASH_H
typedef struct StrHashEntry StrHashEntry;
typedef struct StrHashTable StrHashTable;
StrHashEntry *StrHash_CreateEntry(StrHashTable *table,const char *key);
StrHashEntry *StrHash_FindEntry(StrHashTable *table,const char *key);
StrHashTable *StrHash_New(void); 
void StrHash_SetValue(StrHashEntry *she,void *value);
void *StrHash_GetValue(StrHashEntry *she);
char *StrHash_GetKey(StrHashEntry *she);
#endif
