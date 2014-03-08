/*
 * 
 */
#include <string.h>
#include "interpreter.h"
#include "version.h"
#include "hex.h"
#include "console.h"
#include "pvar.h"
#include "database.h"

static uint8_t gVariant = 0;
static uint8_t gHwRevision = 1;
static char gSerialNumber[9]; /* 8 + 1 */

/* Year in BCD */
#define YEAR ((((__DATE__ [7]-'0')*16+(__DATE__[8]-'0'))*16 \
                                +(__DATE__ [9]-'0'))*16+(__DATE__ [10]-'0'))

#define YEAR2 (((__DATE__ [9]-'0'))*16+(__DATE__ [10]-'0'))

/* Month in BCD 0x01 - 0x12 */
#define MONTH (__DATE__ [2] == 'n' ? (__DATE__[1] == 'a' ? 1 : 6) \
              : __DATE__ [2] == 'b' ? 2 \
              : __DATE__ [2] == 'r' ? (__DATE__[0] == 'M' ? 3 : 4) \
              : __DATE__ [2] == 'y' ? 5 \
              : __DATE__ [2] == 'l' ? 7 \
              : __DATE__ [2] == 'g' ? 8 \
              : __DATE__ [2] == 'p' ? 9 \
              : __DATE__ [2] == 't' ? 0x10 \
              : __DATE__ [2] == 'v' ? 0x11 : 0x12)

#define DAY ((__DATE__ [4]==' ' ? 0 : __DATE__[4]-'0')*16+(__DATE__[5]-'0'))

#define SWNAME	"CBEe"

static char gVersion[16];

const char *
System_GetVersion(void)
{
	SNPrintf(gVersion,sizeof(gVersion) - 1,"%s%02x%02x%02x",SWNAME,YEAR2,MONTH,DAY);
	return gVersion;
}

uint8_t 
System_GetVariant(void)
{
	return gVariant;
}

const char *
System_GetSerialNumber(void)
{
	return gSerialNumber;
}

static bool 
PVVersionSW_Get (void *cbData, uint32_t adId, char *bufP,uint16_t maxlen)
{
	SNPrintf(bufP,maxlen,"\"%s%02x%02x%02x\"",SWNAME,YEAR2,MONTH,DAY);
	return true;
}

static bool 
PVVariantHW_Get (void *cbData, uint32_t adId, char *bufP,uint16_t maxlen)
{
	char *strVariant;
	if(gVariant == VARIANT_EML) {
		strVariant = "EML";
	} else if(gVariant == VARIANT_MZ) {
		strVariant = "MZ";
	} else {
		strVariant = "unknown";
	}
	SNPrintf(bufP,maxlen,"\"%s\"",strVariant);
	//SNPrintf(bufP,maxlen,"%d",gVariant);
	return true;
}

static bool 
PVHWRev_Get (void *cbData, uint32_t adId, char *bufP,uint16_t maxlen)
{
	SNPrintf(bufP,maxlen,"%u",gHwRevision);
	return true;
}

static int8_t
cmd_version(Interp * interp, uint8_t argc, char *argv[])
{
        Interp_Printf_P(interp, "Software Version %s\n", System_GetVersion());
        Interp_Printf_P(interp, "Build date " __DATE__ " " __TIME__ "\n");
	return 0;
}

INTERP_CMD(versionCmd, "version", cmd_version, "version # Print version information");

/**
 **********************************************************************************************
 * \fn static int8_t cmd_variant(Interp * interp, uint8_t argc, char *argv[])
 **********************************************************************************************
 */
static int8_t
cmd_variant(Interp * interp, uint8_t argc, char *argv[])
{
	char *strVariant;
	if((argc == 3) && (strcmp(argv[1],"set") == 0)) {
		gVariant = astrtoi16(argv[2]);
		DB_VarWrite(DBKEY_VARIANT,&gVariant);
	}
	if(gVariant == VARIANT_EML) {
		strVariant = "EML";
	} else if(gVariant == VARIANT_MZ) {
		strVariant = "MZ";
	} else {
		strVariant = "unknown";
	}
        Interp_Printf_P(interp, "Hardware Variant %u (%s)\n",gVariant,strVariant);
	return 0;
}

INTERP_CMD(variantCmd, "variant", cmd_variant, "variant # Print variant information");

/**
 ***************************************************************************************************
 * \fn static int8_t cmd_serialnumber(Interp * interp, uint8_t argc, char *argv[])
 ***************************************************************************************************
 */
static int8_t
cmd_serialnumber(Interp * interp, uint8_t argc, char *argv[])
{
	if((argc == 3) && (strcmp(argv[1],"set") == 0)) {
		SNPrintf(gSerialNumber,array_size(gSerialNumber),argv[2]);
		DB_VarWrite(DBKEY_SERIALNUMBER,&gSerialNumber);
	}
        Interp_Printf_P(interp, "SerialNumber: \"%s\"\n",gSerialNumber);
	return 0;
}

INTERP_CMD(serialnumberCmd, "serialnumber", cmd_serialnumber, "serialnumber ?set <serialnumber>? # Print / Set serialnumber");

/**
 ******************************************************************************************
 * Register shell command and process variable interface to Bert
 ******************************************************************************************
 */
void
Version_Init(void)
{
	Interp_RegisterCmd(&versionCmd);
	Interp_RegisterCmd(&variantCmd);
	Interp_RegisterCmd(&serialnumberCmd);
	SNPrintf(gSerialNumber,array_size(gSerialNumber),"0000000000");
	DB_VarInit(DBKEY_SERIALNUMBER,&gSerialNumber,"system.serialNr");
	DB_VarInit(DBKEY_HWREV,&gHwRevision,"system.hwRevision");
	DB_VarInit(DBKEY_VARIANT,&gVariant,"system.variant");
	PVar_New(PVVersionSW_Get,NULL,NULL,0,"system.firmware");
	PVar_New(PVVariantHW_Get,NULL,NULL,0,"system.variant");
	PVar_New(PVHWRev_Get,NULL,NULL,0,"system.hwRevision");
}
