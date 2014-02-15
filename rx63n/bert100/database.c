/**
 **************************************************************************
 * Database using the internal Data flash of the Renesas RX.
 * It uses 2 blocks. If a block is full the variable space is defragmented
 * and written to the other block
 **************************************************************************
 */

#include <string.h>
#include "dataflash.h"
#include "database.h"
#include "types.h"
#include "interpreter.h"
#include "hex.h"
#include "timer.h"
#include "rx_crc.h"
#include "version.h"
#include "strhash.h"
#include "fat.h"

#define DB_SPACE	(8192)
#define MIN_WRITE	(8)
#define	DB_MAX_ENTRY_LEN	(0x200)

/**
 *********************************************
 * Possible results of a database check.
 *********************************************
 */
#define DBCHK_OK				(0)
#define DBCHK_COPY_REQUIRED 	(2)
#define DBCHK_ERASE_REQUIRED	(3)

typedef struct Database {
	Mutex dbSema;
	StrHashTable *objIdHash;
	uint32_t dbBlock[2];
	uint32_t fillLevel[2];
	uint16_t crcinit;
	uint8_t currBlock;
} Database;

static Database g_Database;

INLINE void
Write32(uint32_t value, void *addr)
{
	*(uint32_t *) (addr) = (value);
}

INLINE void
Write16(uint16_t value, void *addr)
{
	*(uint16_t *) (addr) = (value);
}

INLINE uint32_t
Read32(void *addr)
{
	return *(uint32_t *) (addr);
}

INLINE uint16_t
Read16(void *addr)
{
	return *(uint16_t *) (addr);
}

INLINE uint8_t
Read8(void *addr)
{
	return *(uint8_t *) (addr);
}

/**
 ***********************************************************
 * Find the end of the used space by a binary search
 ***********************************************************
 */
static uint32_t
FindEnd(uint32_t dbBlock)
{
	uint32_t min, max, mid;
	min = dbBlock;
	max = dbBlock + DB_SPACE;
	while (1) {
		mid = (max + min) >> 1;
		if (mid & (MIN_WRITE - 1)) {
			if (DFlash_BlankCheck2(min) == false) {
				return max;
			}
			return min;
		}
		if (DFlash_BlankCheck2(mid) == false) {
			min = mid;
		} else {
			max = mid;
		}
	}
}

/**
 ***************************************************************
 * \fn static void DB_Erase(Database * db, uint8_t blockNr)
 * Erase all flash blocks belonging to both database blocks
 ***************************************************************
 */
static void
DB_Erase(Database * db, uint8_t blockNr)
{
	if (blockNr == 0) {
		DFlash_Erase(db->dbBlock[0],DB_SPACE);
	} else {
		DFlash_Erase(db->dbBlock[1],DB_SPACE);
	}
}

/**
 ***********************************************************************************************
 * \fn static bool _DB_SetObj(uint8_t currBlock, uint32_t tag, const void *buf, uint16_t len)
 ***********************************************************************************************
 */
static bool
_DB_SetObj(uint8_t currBlock, uint32_t tag, const void *buf, uint16_t len)
{
	uint8_t data[8], hdr[8];
	uint16_t i;
	uint16_t cnt;
	uint32_t dbBlock;
	uint32_t tagLenP;
	uint32_t fillLevel;
	uint16_t crc;
	Database *db = &g_Database;

	if (len > DB_MAX_ENTRY_LEN) {
		Con_Printf("DB: Entry 0x%08lx to long (%u bytes)\n", tag, len);
		return false;
	}
	dbBlock = db->dbBlock[currBlock];
	fillLevel = db->fillLevel[currBlock];
	if ((len + fillLevel + 8 + (MIN_WRITE - 1)) > DB_SPACE) {
		return false;
	}
	tagLenP = dbBlock + fillLevel;
	fillLevel += MIN_WRITE;

	Write32(tag, hdr + 0);
	Write16(len, hdr + 4);
	crc = CRC16(db->crcinit, (uint8_t *) hdr, 6);
	crc = CRC16(crc, (uint8_t *) buf, len);
	Write16(crc, hdr + 6);

	//Con_Printf("Set var with CRC %04x\n",crc);
	cnt = len;
	while (cnt >= 8) {
		/* Copy to RAM because source might be in Dataflash ! */
		for (i = 0; i < 8; i++) {
			data[i] = ((uint8_t *) buf)[i];
		}
		DFlash_Write(dbBlock + fillLevel, data, 8);
		buf += 8;
		cnt -= 8;
		fillLevel += MIN_WRITE;
	}
	if (cnt) {
		memset(data, 0, sizeof(data));
		for (i = 0; i < cnt; i++) {
			data[i] = ((uint8_t *) buf)[i];
		}
		DFlash_Write(dbBlock + fillLevel, data, 8);
		fillLevel += MIN_WRITE;
	}
	DFlash_Write(tagLenP, hdr, 8);

	//Con_Printf("Fill level now %lu\n",fillLevel);
	db->fillLevel[currBlock] = fillLevel;
	return true;
}

/**
 ****************************************************+********************************
 * \fn static uint8_t * DB_GetObjP(uint8_t currBlock,uint32_t tag,uint16_t *lenP)
 * Get a pointer to the value of entry identified by TAG.
 *************************************************************************************
 */
static uint8_t *
DB_GetObjP(uint8_t currBlock, uint32_t tag, uint16_t * lenP)
{
	Database *db = &g_Database;
	uint32_t found = 0;
	uint16_t foundlen = 0;
	uint16_t len;
	uint32_t currtag;
	uint32_t fillLevel;
	uint32_t varP, endP, valP;
	uint16_t crc, calc_crc;

	varP = db->dbBlock[currBlock] + 8;
	fillLevel = db->fillLevel[currBlock];
	endP = db->dbBlock[currBlock] + fillLevel;

	while (varP < endP) {
		DFlash_Lock();
		calc_crc = CRC16(db->crcinit, (uint8_t *) varP, 6);
		currtag = Read32((void *)varP);
		varP += 4;
		len = Read16((void *)varP);
		varP += 2;
		crc = Read16((void *)varP);
		varP += 2;
		valP = varP;
		varP += (len + (MIN_WRITE - 1)) & ~(MIN_WRITE - 1);
		if ((tag == currtag) && (len <= DB_MAX_ENTRY_LEN) && (varP <= endP)) {
			calc_crc = CRC16(calc_crc, (uint8_t *) valP, len);
			if (calc_crc == crc) {
				found = valP;
				foundlen = len;
				//Con_Printf("Found at 0x%08lx 0x%08x\n",found,foundlen);
			} else {
				Con_Printf
				    ("DB: CRC error 0x%04x instead of 0x%04x, len %u, valP 0x%08lx\n",
				     crc, calc_crc, len, valP);
			}
		}
		if (len == 0) {
			break;
		}
		DFlash_Unlock();
	}
	if (found && lenP) {
		*lenP = foundlen;
	}
	return (uint8_t *) found;
}

/**
 *************************************************************************
 * This is the interface proc to the outside world
 *************************************************************************
 */
bool
DB_GetObj(uint32_t tag, void *buf, uint16_t maxlen)
{
	Database *db = &g_Database;
	bool retval;
	uint16_t len;
	uint8_t *valP;

	Mutex_Lock(&db->dbSema);
	valP = DB_GetObjP(db->currBlock, tag, &len);
	if (valP && (len <= maxlen)) {
		DFlash_Lock();
		memcpy(buf, valP, len);
		DFlash_Unlock();
		retval = true;
	} else {
		retval = false;
	}
	Mutex_Unlock(&db->dbSema);
	return retval;
}

/**
 ******************************************************************************
 * Write the block header
 * The serialNr is the Number of the (otherBlock + 1) modulo 256
 * A magic is written to identify the block as database. 
 ******************************************************************************
 */
static void
DB_WriteBlockHeader(Database * db, uint8_t blockNr)
{
	uint8_t data[8];
	uint8_t otherBlock = !blockNr;
	uint8_t seqNr;

	seqNr = Read8((void *)db->dbBlock[otherBlock]) + 1;
	memset(data, 0, 8);
	data[0] = seqNr;
	Write32(DB_MAGIC, data + 4);
	//Con_Printf("Sequence number is %u\n",seqNr);
	DFlash_Write(db->dbBlock[blockNr], data, 8);
}

/**
 *****************************************************************
 * Copy the database to the other block ommiting double entries
 *****************************************************************
 */
static void
DB_Defrag(Database * db)
{
	uint32_t varP, endP;
	uint32_t valP, otherValP;
	uint32_t fillLevel;
	uint8_t currBlock;
	uint8_t otherBlock;
	uint32_t tag;
	uint16_t len;
	uint16_t otherLen;

	currBlock = db->currBlock;
	otherBlock = !db->currBlock;

	if (currBlock == 0) {
		DB_Erase(db, 1);
		db->fillLevel[1] = 8;
		varP = db->dbBlock[0] + 8;
		fillLevel = db->fillLevel[0];
		endP = db->dbBlock[0] + fillLevel;
	} else {
		DB_Erase(db, 0);
		db->fillLevel[0] = 8;
		varP = db->dbBlock[1] + 8;
		fillLevel = db->fillLevel[1];
		endP = db->dbBlock[1] + fillLevel;
	}
	while (varP < endP) {
		DFlash_Lock();
		tag = Read32((void *)varP);
		varP += 4;
		len = Read16((void *)varP);
		varP += 4;
		DFlash_Unlock();

		otherValP = (uint32_t) DB_GetObjP(otherBlock, tag, &otherLen);
		varP += (len + (MIN_WRITE - 1)) & ~(MIN_WRITE - 1);

		if (!otherValP) {
			valP = (uint32_t) DB_GetObjP(currBlock, tag, &len);
			//Con_Printf("Tag %08lx not found in other DB\n",tag);  
			if (valP != 0) {
				if (_DB_SetObj(otherBlock, tag, (void *)valP, len) != true) {
					Con_Printf("DB Bug, can not set other var %08lx\n", tag);
				}
			} else {
				Con_Printf("DB Bug, can not find var %08lx\n", tag);
			}
		} else {
			//Con_Printf("Tag %08lx already in other DB\n",tag);    
		}
	}
	DB_WriteBlockHeader(db, otherBlock);
	db->currBlock = otherBlock;
}

/**
 ***********************************************************
 * Set an object in the database
 * This is an interface proc to the outside.
 ***********************************************************
 */
bool
DB_SetObj(uint32_t tag, const void *buf, uint16_t len)
{
	Database *db = &g_Database;
	bool retval;
	Mutex_Lock(&db->dbSema);

	if (_DB_SetObj(db->currBlock, tag, buf, len) == true) {
		retval = true;
	} else {
		DB_Defrag(db);
		retval = _DB_SetObj(db->currBlock, tag, buf, len);
	}
	Mutex_Unlock(&db->dbSema);
	return retval;
}

bool
DB_SetObjName(uint32_t objKey, const char *format, ...) 
{
	Database *db = &g_Database;
	char printfBuf[42];
        StrHashEntry *she;
	void *objId = (void *)objKey;
        va_list ap;
        va_start(ap,format);
        VSNPrintf(printfBuf,array_size(printfBuf),format,ap);
        va_end(ap);
	Mutex_Lock(&db->dbSema);
	she = StrHash_CreateEntry(db->objIdHash,printfBuf);
	if(!she) {
		Con_Printf("Can not create Hash table entry for DB Obj %s(0x%08x)\n",printfBuf,objKey);
		Mutex_Unlock(&db->dbSema);
		return false;
	}
	StrHash_SetValue(she,objId);
	Mutex_Unlock(&db->dbSema);
	return true;
}

/**
 ********************************************************
 * Create a database by initializing both blocks
 ********************************************************
 */
static void
DB_Create(Database * db)
{
	db->currBlock = 1;
	db->fillLevel[1] = 8;
	DB_Erase(db, db->currBlock);
	DB_WriteBlockHeader(db, db->currBlock);

	/* Last one created will have highest sequence Nr */
	db->currBlock = 0;
	db->fillLevel[0] = 8;
	DB_Erase(db, 0);
	DB_WriteBlockHeader(db, db->currBlock);
}

/**
 **********************************************************************************************
 * Check if the database is consistent.
 * returns ERASE_REQUIRED when header is bad. 
 * returns COPY_REQUIRED when at least one data entry is inconsistent. 
 **********************************************************************************************
 */
static uint8_t
DB_Check(Database * db, uint8_t blockNr)
{
	uint32_t varP, endP, valP;
	uint32_t fillLevel;
	uint32_t dbBlock;
	uint32_t tag;
	uint16_t len;
	uint32_t magic;
	uint16_t crc, calc_crc;

	fillLevel = db->fillLevel[blockNr];
	dbBlock = db->dbBlock[blockNr];
	endP = db->dbBlock[blockNr] + fillLevel;

	if (DFlash_BlankCheck2(dbBlock) == true) {
		Con_Printf("DBCHK: Header at %08x is blank\n", dbBlock);
		return DBCHK_ERASE_REQUIRED;
	}

	DFlash_Lock();
	magic = Read32((void *)(dbBlock + 4));
	if (magic != DB_MAGIC) {
		Con_Printf("DBCHK: Magic is wrong\n");
		DFlash_Unlock();
		return DBCHK_ERASE_REQUIRED;
	}
	DFlash_Unlock();

	for (varP = dbBlock; varP < endP; varP += 2) {
		if (DFlash_BlankCheck2(varP) == true) {
			Con_Printf("DBCHK: Data Area contains blank areas\n");
			return DBCHK_COPY_REQUIRED;
		}
	}
	for (; varP < dbBlock + DB_SPACE; varP += 2) {
		if (DFlash_BlankCheck2(varP) == false) {
			Con_Printf("DBCHK: Post Data Area contains unblank areas\n");
			return DBCHK_COPY_REQUIRED;
		}
	}
	for (varP = dbBlock + 8; varP < endP;) {
		DFlash_Lock();
		calc_crc = CRC16(db->crcinit, (uint8_t *) varP, 6);
		tag = Read32((void *)varP);
		varP += 4;
		len = Read16((void *)varP);
		varP += 2;
		crc = Read16((void *)varP);
		varP += 2;
		DFlash_Unlock();
		valP = varP;
		varP += (len + (MIN_WRITE - 1)) & ~(MIN_WRITE - 1);
		//Con_Printf("Check %08lx, len %04x, crc %04x addr %08lx\n",tag,len,crc,valP);
		if ((len == 0) || (len > DB_MAX_ENTRY_LEN) || (varP > endP)) {
			Con_Printf("DBCHK: One entry has illegal size, len %u, varP %08lx \n",
				   varP);
			return DBCHK_COPY_REQUIRED;
		}
		DFlash_Lock();
		calc_crc = CRC16(calc_crc, (uint8_t *) valP, len);
		DFlash_Unlock();
		if (crc != calc_crc) {
			Con_Printf("DBCHK: CRC %04x instead %04x for 0x%08lx, len %u at %08lx\n",
				   crc, calc_crc, tag, len, valP);
			return DBCHK_COPY_REQUIRED;
		}
	}
	if (varP != endP) {
		Con_Printf("DBCHK: Entry Size sum does not match end\n");
		return DBCHK_COPY_REQUIRED;
	}
	return DBCHK_OK;
}

/**
 ****************************************************************
 * Check and fix the database.
 ****************************************************************
 */
static void
DB_Fix(Database * db)
{
	static uint8_t result;
	result = DB_Check(db, db->currBlock);
	if (result) {
		Con_Printf("Database check result is %u\n", result);
	}
	switch (result) {
	    case DBCHK_OK:
		    break;

	    case DBCHK_COPY_REQUIRED:
		    DB_Defrag(db);
		    break;

	    case DBCHK_ERASE_REQUIRED:
		    DB_Create(db);
		    break;
	    default:
		    Con_Printf("Unexpected result %u in database check\n", result);
		    break;
	}
}

static void
Keys_Dump(Database *db) {
        StrHashSearch hashSearch;
        StrHashEntry * hashEntry;
        hashEntry = StrHash_FirstEntry(db->objIdHash,&hashSearch);
        for(;hashEntry;hashEntry = StrHash_NextEntry(&hashSearch)) {
                const char *key = StrHash_GetKey(hashEntry);
		uint32_t objId = (uint32_t)StrHash_GetValue(hashEntry);
                Con_Printf("0x%08lx: %s\n",objId,key);
        }
}

static uint32_t 
Key_FindByName(Database *db, const char *keyname) 
{
        StrHashEntry * hashEntry;
	uint32_t objId;
        hashEntry = StrHash_FindEntry(db->objIdHash,keyname);
	if(!hashEntry) {
		return 0;
	}	
	objId = (uint32_t)StrHash_GetValue(hashEntry);
	return objId;
}

/**
 *****************************************************************************
 * \fn static int8_t cmd_db(Interp * interp, uint8_t argc, char *argv[])
 *****************************************************************************
 */
static int8_t
cmd_db(Interp * interp, uint8_t argc, char *argv[])
{
	Database *db = &g_Database;
	unsigned int i;
	if (argc == 5 && (strcmp(argv[1],"set") == 0)) {
		uint32_t key;
		key = Key_FindByName(db,argv[2]);
		if(key == 0) {
			Con_Printf("Database entry \"%s\" not found\n");
			return 0;
		}		
		if(strcmp(argv[3],"byte") == 0) {
			uint8_t val = astrtoi16(argv[4]);
			DB_SetObj(key,&val,1);
		} else if(strncmp(argv[3],"bool",1) == 0) {
			uint8_t val = !!astrtoi16(argv[4]);
			DB_SetObj(key,&val,1);
		} else if(strncmp(argv[3],"long",1) == 0) {
			uint64_t val = astrtoi64(argv[4]);
			DB_SetObj(key,&val,8);
		} else if(strncmp(argv[3],"float",1) == 0) {
			float val = astrtof32(argv[4]);
			DB_SetObj(key,&val,4);
		} else if(strncmp(argv[3],"string",1) == 0) {
			DB_SetObj(key, argv[4],strlen(argv[4]) + 1);
		} else {
			return -EC_BADARG;
		}
	} else if ((argc == 3) && (strcmp(argv[1],"save") == 0)) {
		char *filename = argv[2];
		FRESULT res;
		UINT s2;
		FIL file;
		res = f_open(&file, filename, FA_CREATE_ALWAYS | FA_WRITE);
        	if (res) {
                	Con_Printf("open destination failed with %d\n", res);
                	return 0;
        	}
              	res = f_write(&file,(void*)DFLASH_MAP_ADDR(0), 2 * DB_SPACE, &s2);
        	if (res) {
                	Con_Printf("Write failed with %d\n", res);
        	}
              	f_close(&file);
               	return 0;
	} else if ((argc == 2) && (strcmp(argv[1],"keys") == 0)) {
		Keys_Dump(db); 
	} else if (argc > 2) {
		if (strcmp(argv[1], "get") == 0) {
			uint32_t key;
			uint16_t len;
			uint8_t *obj;
			key = Key_FindByName(db,argv[2]);
			if(key == 0) {
				Con_Printf("Database entry \"%s\" not found\n");
				return 0;
			}		
			obj = DB_GetObjP(db->currBlock, key, &len);
			if (obj) {
				DFlash_Lock();
				for (i = 0; i < len; i++) {
					Con_Printf("%02x ", obj[i]);
					if ((i & 15) == 15) {
						Con_Printf("\n");
					}
				}
				if ((i & 15) != 15) {
					Con_Printf("\n");
				}
				DFlash_Unlock();
			}
			return 0;
		} else if (strcmp(argv[1], "key") == 0) {
			uint32_t key;
			uint16_t len;
			uint8_t *obj;
			key = astrtoi32(argv[2]);
			obj = DB_GetObjP(db->currBlock, key, &len);
			if (obj) {
				DFlash_Lock();
				for (i = 0; i < len; i++) {
					Con_Printf("%02x ", obj[i]);
					if ((i & 15) == 15) {
						Con_Printf("\n");
					}
				}
				if ((i & 15) != 15) {
					Con_Printf("\n");
				}
				DFlash_Unlock();
			}
			return 0;
		} else {
			return -EC_BADARG;
		}
	} else if (argc > 1) {
		if (strcmp(argv[1], "defrag") == 0) {

			Mutex_Lock(&db->dbSema);
			DB_Defrag(db);
			Mutex_Unlock(&db->dbSema);

			Con_Printf("Database uses now %lu of %u bytes, in block %u\n",
				   db->fillLevel[db->currBlock], DB_SPACE, db->currBlock);
			return 0;
		} else if (strcmp(argv[1], "create") == 0) {
			Mutex_Lock(&db->dbSema);
			DB_Create(db);
			Mutex_Unlock(&db->dbSema);

			Con_Printf("Database is now empty\n",
				   db->fillLevel[db->currBlock], DB_SPACE, db->currBlock);
			return 0;
		} else if (strcmp(argv[1], "test") == 0) {
			uint8_t buf[20];
			uint32_t i;
			for (i = 0; i < 20; i++) {
				DB_SetObj(0x12345679, "Sau", 4);
			}
			Con_Printf("Time %lu\n", TimeMs_Get());
			for (i = 0; i < UINT32_C(100000); i++) {
				EV_Yield();
				DB_GetObj(0x12345679, buf, 20);
			}
			Con_Printf("Time %lu\n", TimeMs_Get());
			Con_Printf("%s\n", buf);
		} else {
			return -EC_BADARG;
		}
	}
	Con_Printf("Use Flash Block %u, used %lu of %u bytes\n",
		   db->currBlock, db->fillLevel[db->currBlock], DB_SPACE);
	return 0;
}

INTERP_CMD(dbCmd, "db", cmd_db, "db <create | defrag> # Database");

/**
 **********************************************************************************
 * \fn void DB_Init(void) 
 **********************************************************************************
 */
void
DB_Init(void)
{
	Database *db = &g_Database;
	uint32_t level0, level1;
	uint8_t version0, version1;

	/* 
	 * Mutex is not locked during init because there is no one reading the database before 
	 * init is left.
	 */
	Mutex_Init(&db->dbSema);
	//db->crcinit = CRC16(0,(uint8_t*)g_Version,3);
	db->objIdHash = StrHash_New(32);
	db->crcinit = DB_CRCINIT;

	db->dbBlock[0] = DFLASH_MAP_ADDR(0);
	db->dbBlock[1] = DFLASH_MAP_ADDR(DB_SPACE);

	db->fillLevel[0] = level0 = FindEnd(db->dbBlock[0]) - db->dbBlock[0];
	db->fillLevel[1] = level1 = FindEnd(db->dbBlock[1]) - db->dbBlock[1];

	if (level0 && (DFlash_BlankCheck2((uint32_t) db->dbBlock[0]) == true)) {
		Con_Printf("DB Block 0 is invalid, Erasing\n");
		DB_Erase(db, 0);
		level0 = 0;
	}
	if (level1 && (DFlash_BlankCheck2((uint32_t) db->dbBlock[1]) == true)) {
		Con_Printf("DB Block 1 is invalid, Erasing\n");
		DB_Erase(db, 1);
		level1 = 0;
	}
	db->fillLevel[0] = level0;
	db->fillLevel[1] = level1;
	//Con_Printf("level0 %lu level1 %lu\n",level0,level1);
	if (level0 && level1) {
		DFlash_Lock();
		version0 = Read8((void *)db->dbBlock[0]);
		version1 = Read8((void *)db->dbBlock[1]);
		DFlash_Unlock();
		/* Use modulo arithmetic */
		if (((version1 - version0) & 0xff) < 0x80) {
			db->currBlock = 1;
		} else {
			db->currBlock = 0;
		}
	} else if (level0) {
		db->currBlock = 0;
	} else if (level1) {
		db->currBlock = 1;
	} else {
		Con_Printf("DB: No database found, Creating a new one\n");
		DB_Create(db);
	}
	DB_Fix(db);
	Interp_RegisterCmd(&dbCmd);
}
