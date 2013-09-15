/*
 * 
 */
#include <string.h>
#include "interpreter.h"
#include "version.h"
#include "hex.h"
#include "console.h"
#include "pvar.h"

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

#define SWNAME	"EMLBERT"

static char g_Version[16];

const char *
Version_GetStr(void)
{
	SNPrintf(g_Version,sizeof(g_Version) - 1,"%s%02x%02x%02x",SWNAME,YEAR2,MONTH,DAY);
	return g_Version;
}

static bool 
PVVersionSW_Get (void *cbData, uint32_t adId, char *bufP,uint16_t maxlen)
{
	strncpy(bufP,Version_GetStr(),maxlen);
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

/**
 ******************************************************************************************
 * Register shell command and process variable interface to Bert
 ******************************************************************************************
 */
void
Version_Init(void)
{
	Interp_RegisterCmd(&versionCmd);
	PVar_New(PVVersionSW_Get,NULL,NULL,0,"version.sw");
}
