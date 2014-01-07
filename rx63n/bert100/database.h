/**
 *********************************************************************
 * Database header file.
 *********************************************************************
 */

#ifndef _DATABASE_H
#define _DATABASE_H
#include "types.h"
#define DB_MAGIC    	(0x08124716)
#define DB_CRCINIT      (0x03e0)

bool DB_SetObj(uint32_t tag, void *buf, uint16_t len);
bool DB_GetObj(uint32_t tag, void *buf, uint16_t maxlen);
void DB_Init(void);

#define DB_VarRead(tag,buf) DB_GetObj((tag),(buf),sizeof(*buf))
#define DB_VarWrite(tag,buf) DB_SetObj((tag),(buf),sizeof(*buf))
INLINE bool DB_VarReadStr(uint32_t tag,void *_buf,uint16_t maxlen) { 
	char *buf = _buf;
	buf[maxlen - 1] = 0; 
	return DB_GetObj(tag,buf,maxlen - 1); 
}

#define DB_VarInit(tag,buf) { \
	if(DB_VarRead((tag),(buf)) == false) { \
		DB_VarWrite((tag),(buf));\
	} \
}

#define DBTAG_DDCMP(x)       	(0x08150000 | (x))
#define DBTAG_WATCHDOG(x)    	(0x08160000 | (x))
#define DBTAG_CASHLESS_DEV(x)	(0x08170000 | (x))
#define DBTAG_LCD(x)			(0x08180000 | (x))
#define DBTAG_CREDITCARD(x)		(0x08190000 | (x))
#define DBTAG_SYSTEM(x)			(0x081a0000 | (x))
#endif
