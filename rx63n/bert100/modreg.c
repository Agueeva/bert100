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
	/* Setup PE5 for MTIO4C/MTIO2B */
	BSET(5,PORTE.PDR.BYTE)
	BCLR(5,PORTE.PODR.BYTE)
	MPC.PE5PFS.BYTE = 0x1; /* Switch Pin to MTIO4C mode */

	MSTP(MTU4) = 0;
	MTU4.TGRA = 1199;

	/* PCLK / 1 */
        MTU4.TCR.BIT.TPSC = 0;
        /* Counter Cleared by TGRC match */
        MTU4.TCR.BIT.CCLR = 5;
        /* Toggle at compare match */
        MTU4.TIORL.BIT.IOC = 5;

	MTU4.TMDR.BIT.MD = 2;   /* PWM mode 1 */
}

void
ModReg_Init(void)
{
	enable_modulator_clock();
}
