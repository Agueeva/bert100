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
Version_GetStr(void)
{
	SNPrintf(gVersion,sizeof(gVersion) - 1,"%s%02x%02x%02x",SWNAME,YEAR2,MONTH,DAY);
	return gVersion;
}

uint8_t 
Variant_Get(void)
{
	return gVariant;
}

static bool 
PVVersionSW_Get (void *cbData, uint32_t adId, char *bufP,uint16_t maxlen)
{
	SNPrintf(bufP,maxlen,"\"%s%02x%02x%02x\"",SWNAME,YEAR2,MONTH,DAY);
	return true;
}

static int8_t
cmd_version(Interp * interp, uint8_t argc, char *argv[])
{
        Interp_Printf_P(interp, "Software Version %s\n", Version_GetStr());
        Interp_Printf_P(interp, "Build date " __DATE__ " " __TIME__ "\n");
	return 0;
}

INTERP_CMD(versionCmd, "version", cmd_version, "version # Print version information");

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
 ******************************************************************************************
 * Register shell command and process variable interface to Bert
 ******************************************************************************************
 */
void
Version_Init(void)
{
	Interp_RegisterCmd(&versionCmd);
	Interp_RegisterCmd(&variantCmd);
	DB_VarInit(DBKEY_VARIANT,&gVariant,"system.variant");
	PVar_New(PVVersionSW_Get,NULL,NULL,0,"system.firmware");
	//PVar_New(PVVersionSW_Get,NULL,NULL,0,"system.variant");
}
