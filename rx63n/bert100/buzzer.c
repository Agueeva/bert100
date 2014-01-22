#include <stdlib.h>
#include <string.h>
#include "iodefine.h"
#include "types.h"
#include "interpreter.h"
#include "interrupt_handlers.h"
#include "console.h"
#include "hex.h"
#include "timer.h"
#include "atomic.h"
#include "config.h"
#include "version.h"


static void Buzzer_Start(uint32_t hz) {
	if(hz == 0) {
		MSTP(MTU3) = 1;
		return;
	}
	MSTP(MTU3) = 0;
        /* Setup PE5 for MTIO4C/MTIO2B */
        MPC.P17PFS.BYTE = 0x1; /* Switch Pin to MTIO4C mode */
        MTU.TSTR.BIT.CST3 = 0;
        //MTU.TOER.BIT.OE3C = 1;  /* Enable the output,must be done before TIOR write */
        PORT1.PMR.BIT.B7 = 1;

        MTU3.TCNT = 0;  /* Reset counter */
        MTU3.TGRA = (F_PCLK / (2 * hz) - 1);
        /* PCLK / 1 */
        MTU3.TCR.BIT.TPSC = 0;
        /* Counter Cleared by TGRA match */
        MTU3.TCR.BIT.CCLR = 1;
        /* Initial output 1 Toggle at compare match */
        MTU3.TIORH.BIT.IOA = 7;
        MTU3.TMDR.BIT.MD = 0;   /* Normal mode */

        MTU.TSTR.BIT.CST3 = 1;
}

static int8_t
cmd_buzzer(Interp * interp, uint8_t argc, char *argv[])
{
	uint16_t hz;
	if(argc > 1) {
		hz = astrtoi16(argv[1]);
		Buzzer_Start(hz); 
	} else {
		Con_Printf("P17: %u\n",PORT1.PIDR.BIT.B7);
	}
	return 0;
}
INTERP_CMD(buzzerCmd, "buzzer", cmd_buzzer, "buzzer <frequency> ");

void Buzzer_Init(void)
{
	Interp_RegisterCmd(&buzzerCmd);
	Buzzer_Start(4000);
}
