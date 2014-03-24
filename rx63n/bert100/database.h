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

<<<<<<< HEAD
bool DB_SetObj(uint32_t tag, const void *buf, uint16_t len);
bool DB_GetObj(uint32_t tag, void *buf, uint16_t maxlen);
bool DB_SetObjName(uint32_t objKey, const char *format, ...);
void DB_Init(void);


=======
bool DB_SetObj(uint32_t tag, void *buf, uint16_t len);
bool DB_GetObj(uint32_t tag, void *buf, uint16_t maxlen);
void DB_Init(void);

>>>>>>> f787f742d2526f05a4fb11382eb6ac4b6db75b4f
#define DB_VarRead(tag,buf) DB_GetObj((tag),(buf),sizeof(*buf))
#define DB_VarWrite(tag,buf) DB_SetObj((tag),(buf),sizeof(*buf))
INLINE bool DB_VarReadStr(uint32_t tag,void *_buf,uint16_t maxlen) { 
	char *buf = _buf;
	buf[maxlen - 1] = 0; 
	return DB_GetObj(tag,buf,maxlen - 1); 
}

<<<<<<< HEAD
#define DB_VarInit(tag,buf, format...) { \
	if(DB_VarRead((tag),(buf)) == false) { \
		DB_VarWrite((tag),(buf));\
	} \
	DB_SetObjName((tag), format); \
}

#define DBKEY_VERSION(x)       	(0x08150000 | (x))
#define DBKEY_DAC(x)       	(0x08160000 | (x))
#define DBKEY_ADC(x)       	(0x08170000 | (x))
#define DBKEY_NTC(x)       	(0x08180000 | (x))
#define DBKEY_BERT(x)       	(0x08190000 | (x))
#define DBKEY_WSERV(x)       	(0x081A0000 | (x))
#define DBKEY_MODREG(x)       	(0x081B0000 | (x))
#define DBKEY_ETHERNET(x)      	(0x081C0000 | (x))
=======
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
>>>>>>> f787f742d2526f05a4fb11382eb6ac4b6db75b4f
#endif
