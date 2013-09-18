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

#define Calloc IRam_Calloc
#define Strdup IRam_Strdup

struct StrHashEntry {
	const char *key;
	void *value;
	struct StrHashEntry *next;
};

struct StrHashTable {
	StrHashEntry **buckets;		
	uint32_t nr_buckets;
};


INLINE uint16_t 
StrHashBucket(StrHashTable *table,const char *str) {
	return CRC16_String(str) & (table->nr_buckets - 1);	
}

INLINE uint16_t 
StrNHashBucket(StrHashTable *table,const char *str,uint16_t len) {
	return CRC16(0,str,len) & (table->nr_buckets - 1);	
}

/**
 **************************************************************
 * Create an entry in the Hash Table
 **************************************************************
 */
StrHashEntry *
StrHash_CreateEntry(StrHashTable *table,const char *key) 
{
	uint16_t idx = StrHashBucket(table,key);
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
#if 0
	newentry->prev = NULL;
	if(*first) {
		(*first)->prev = newentry; 
	} 
#endif
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
	uint16_t idx = StrHashBucket(table,key);
	StrHashEntry **first = &table->buckets[idx];
	StrHashEntry *cursor;
	for(cursor = *first; cursor; cursor = cursor->next) {
		if(strcmp(cursor->key,key) == 0) {
			break;
		}
	}
	return cursor;
}

StrHashEntry *
StrNHash_FindEntry(StrHashTable *table,const char *key,uint16_t keylen) 
{
	uint16_t idx = StrNHashBucket(table,key,keylen);
 	StrHashEntry **first = &table->buckets[idx];
	StrHashEntry *cursor;
	for(cursor = *first; cursor; cursor = cursor->next) {
		if(strncmp(cursor->key,key,keylen) == 0) {
			break;
		}
	}
	return cursor;
}

StrHashEntry *
StrHash_FirstEntry(StrHashTable *table,StrHashSearch *search)
{
        search->table = table;
        for(search->nr_bucket = 0;search->nr_bucket < table->nr_buckets;) {
                StrHashEntry **first=&table->buckets[search->nr_bucket];
                search->nr_bucket++;
                if(*first) {
                        search->cursor=*first;
                        return *first;
                }
        }
        return NULL;
}

StrHashEntry *
StrHash_NextEntry(StrHashSearch *search)
{
        StrHashEntry *entry = search->cursor;
        StrHashTable *table = search->table;
        if(entry->next) {
                search->cursor = entry->next;
                return search->cursor;
        }
        while(search->nr_bucket < table->nr_buckets) {
                StrHashEntry **first = &table->buckets[search->nr_bucket];
                search->nr_bucket++;
                if(*first) {
                        search->cursor=*first;
                        return *first;
                }
        }
        return NULL;
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

/**
  ***********************************************************************
  * Create a new String hash Table. The number of buckets must
  * be a power of two because of speed.
  ***********************************************************************
  */
StrHashTable * 
StrHash_New(uint32_t nrBuckets) {
	StrHashTable *sht = Calloc(sizeof(*sht));
	sht->buckets = (StrHashEntry **)Calloc(sizeof(void *) * nrBuckets);
	sht->nr_buckets = nrBuckets;
	return sht;
}
