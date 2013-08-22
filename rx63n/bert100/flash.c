#include <string.h>
#include "iodefine.h"
#include "atomic.h"
#include "tpos.h"
#include "events.h"
#include "timer.h"
#include "interpreter.h"
#include "hex.h"
#include "flash.h"
#include "console.h"
#include "fat.h"

static int8_t
cmd_memdump(Interp * interp, uint8_t argc, char *argv[])
{
        uint8_t *memP;
        uint32_t i;
	uint32_t length;
	if(argc < 3) {
		return -EC_BADNUMARGS;
	}
	memP = (uint8_t*) astrtoi32(argv[1]);
	length = astrtoi32(argv[2]);
        for (i = 0; i < length; i++) {
		Con_Printf("%02x ", memP[i]);
		if (i % 16 == 15) {
			Con_Printf("\n");
		}
	}
        return 0;
}

INTERP_CMD(memdumpCmd, "memdump", cmd_memdump, "memdump  # dump memory");

void Flash_Init(void)
{
	Interp_RegisterCmd(&memdumpCmd);
}
