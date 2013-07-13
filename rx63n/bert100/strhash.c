/**
 *************************************************************
 * Hash Table with string as index.
 *************************************************************
 */
#include <string.h>
#include "types.h"
#include "rx_crc.h"
#include "sram.h"
#include "strhash.h"

#define NR_HASH_BUCKETS	64

struct StrHashEntry {
	char *key;
	void *value;
	struct StrHashEntry *next;
	struct StrHashEntry *prev;
};

struct StrHashTable {
	StrHashEntry **buckets;		
};

static uint16_t 
StrHashBucket(char *str) {
	return CRC16_String(str) % (NR_HASH_BUCKETS - 1);	
}

/**
 **************************************************************
 * Create an entry in the Hash Table
 **************************************************************
 */
StrHashEntry *
StrHash_CreateEntry(StrHashTable *table,const char *key) 
{
	uint16_t idx = StrHashBucket(key);
	StrHashEntry **first = &table->buckets[idx];
	StrHashEntry *cursor;
	StrHashEntry *newentry;
	for(cursor = *first; cursor; cursor = cursor->next) {
		if(strcmp(cursor->key,key) == 0) {
			break;
		}
	}
	if(cursor) {
		/* Error: entry already exists */
		return NULL;	
	}
	newentry = sr_calloc(sizeof(StrHashEntry));
	newentry->next = *first;
	newentry->prev = NULL;
	if(*first) {
		(*first)->prev = newentry; 
	} 
	*first = newentry;
	newentry->key = sr_strdup(key);
	return newentry;
}

/**
 ***********************************************************
 * Find a hash Entry by Key.
 ***********************************************************
 */
StrHashEntry *
StrHash_FindEntry(StrHashTable *table,const char *key) 
{
	uint16_t idx = StrHashBucket(key);
	StrHashEntry **first = &table->buckets[idx];
	StrHashEntry *cursor;
	for(cursor = *first; cursor; cursor = cursor->next) {
		if(strcmp(cursor->key,key) == 0) {
			break;
		}
	}
	return cursor;
}

/**
 ***************************************************************
 * \fn void StrHash_SetValue
 ***************************************************************
 */
void 
StrHash_SetValue(StrHashEntry *she,void *value)
{
	she->value = value;	
}

void *
StrHash_GetValue(StrHashEntry *she)
{
	return she->value;
}

char *
StrHash_GetKey(StrHashEntry *she) 
{
	return she->key;
}

StrHashTable * 
StrHash_New(void) {
	StrHashTable *sht = sr_calloc(sizeof(*sht));
	sht->buckets = (StrHashEntry **)sr_calloc(sizeof(void *) * NR_HASH_BUCKETS);
	return sht;
}
