/**
 *
 */
#include <stdint.h>
#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "timer.h"
#include "events.h"
#include "console.h"
#include "interpreter.h"
#include "editor.h"
#include "irqflags.h"


static int8_t
cmd_reboot(Interp * interp, uint8_t argc, char *argv[])
{
	CCP = 0xd8;
	WDT_CTRL = WDT_CEN_bm | WDT_ENABLE_bm | WDT_PER_8CLK_gc;
	ATOMIC_BLOCK(ATOMIC_FORCEON) {
		while(1) {
			/* Hang system */
		}
	}
        return -EC_OK;
}

INTERP_CMD(reboot, cmd_reboot, "reboot      # Reboot the system");


