/*
 * Modulator 
 */

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


/*
 ********************************************************************
 * Setup Tmer MTIO4C for clock output for the Modulator. 
 ********************************************************************
 */
static void
enable_modulator_clock(void)
{
	MSTP(MTU4) = 0;
	/* Setup PE5 for MTIO4C/MTIO2B */
	BSET(5,PORTE.PDR.BYTE)
	BSET(5,PORTE.PODR.BYTE)
	MPC.PE5PFS.BYTE = 0x1; /* Switch Pin to MTIO4C mode */
	MTU.TSTR.BIT.CST4 = 0;
	MTU.TOER.BIT.OE4C = 1;	/* Enable the output,must be done before TIOR write */
	PORTE.PMR.BIT.B5 = 1; 

	MTU4.TGRC = 1199;
	/* PCLK / 1 */
        MTU4.TCR.BIT.TPSC = 0;
        /* Counter Cleared by TGRC match */
        MTU4.TCR.BIT.CCLR = 5;
        /* Toggle at compare match */
        MTU4.TIORL.BIT.IOC = 7;
	MTU4.TMDR.BIT.MD = 0;   /* Normal mode */
	MTU.TSTR.BIT.CST4 = 1;
}

static int8_t
cmd_mod(Interp * interp, uint8_t argc, char *argv[])
{
	Con_Printf("PE5: %u\n",PORTE.PIDR.BIT.B5);
	Con_Printf("TCNT %u\n",MTU4.TCNT);
	Con_Printf("TGRC %u\n",MTU4.TGRC);
	return 0;
}

INTERP_CMD(modCmd, "mod", cmd_mod, "mod");

void
ModReg_Init(void)
{
	enable_modulator_clock();
	Interp_RegisterCmd(&modCmd);
}
