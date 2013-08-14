/**
 *************************************************************
 * Hash Table with string as index.
 *************************************************************
 */
#include <string.h>
#include "types.h"
#include "rx_crc.h"
#include "strhash.h"
#include "console.h"
#include "iram.h"

#define NR_HASH_BUCKETS	64
#define Calloc IRam_Calloc
#define Strdup IRam_Strdup

struct StrHashEntry {
	const char *key;
	void *value;
	struct StrHashEntry *next;
	struct StrHashEntry *prev;
};

struct StrHashTable {
	StrHashEntry **buckets;		
};

static uint16_t 
StrHashBucket(const char *str) {
	return CRC16_String(str) % (NR_HASH_BUCKETS - 1);	
}

#if 0
static uint16_t 
StrNHashBucket(char *str,uint16_t len) {
	return CRC16(0,str,len) % (NR_HASH_BUCKETS - 1);	
}
#endif

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
	newentry = Calloc(sizeof(StrHashEntry));
	newentry->next = *first;
	newentry->prev = NULL;
	if(*first) {
		(*first)->prev = newentry; 
	} 
	*first = newentry;
	/* Dup only keys in RAM, flash is immutable */
	if((uint32_t) key  < 0xF0000000) {
		newentry->key = Strdup(key);
	} else {
		newentry->key = key;
	}
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

const char *
StrHash_GetKey(StrHashEntry *she) 
{
	return she->key;
}

StrHashTable * 
StrHash_New(void) {
	StrHashTable *sht = Calloc(sizeof(*sht));
	sht->buckets = (StrHashEntry **)Calloc(sizeof(void *) * NR_HASH_BUCKETS);
	return sht;
}
