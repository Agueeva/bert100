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

#define SYNC_RESET_DIROUT()     BSET(4,PORTE.PDR.BYTE)
#define SYNC_RESET_HIGH()       BSET(4,PORTE.PODR.BYTE)
#define SYNC_RESET_LOW()        BCLR(4,PORTE.PODR.BYTE) 

typedef struct ModReg {
	Timer syncTimer;	
} ModReg;

static ModReg gModReg;

static void
ModSyncResetProc(void *eventData)
{
	ModReg *mr = eventData;
	Timer_Start(&mr->syncTimer,100);
	DISABLE_IRQ();
	SYNC_RESET_HIGH();	
	SYNC_RESET_LOW();	
	ENABLE_IRQ();
}

/*
 ********************************************************************
 * Setup Tmer MTIO4C on Port E5 for clock output for the Modulator. 
 ********************************************************************
 */
static void
enable_modulator_clock(uint32_t hz)
{
	MSTP(MTU4) = 0;
	/* Setup PE5 for MTIO4C/MTIO2B */
	BSET(5,PORTE.PDR.BYTE);
	BSET(5,PORTE.PODR.BYTE);
	MPC.PE5PFS.BYTE = 0x1; /* Switch Pin to MTIO4C mode */
	MTU.TSTR.BIT.CST4 = 0;
	MTU.TOER.BIT.OE4C = 1;	/* Enable the output,must be done before TIOR write */
	PORTE.PMR.BIT.B5 = 1; 

	MTU4.TGRC = (F_PCLK / (2 * hz) - 1);
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
	if((argc == 1)  && (strcmp(argv[1],"clk"))) {
		Con_Printf("PE5: %u\n",PORTE.PIDR.BIT.B5);
		Con_Printf("TCNT %u\n",MTU4.TCNT);
		Con_Printf("TGRC %u\n",MTU4.TGRC);
	}
	return 0;
}

INTERP_CMD(modCmd, "mod", cmd_mod, "mod");

void
ModReg_Init(void)
{
	ModReg *mr = &gModReg;
	Timer_Init(&mr->syncTimer,ModSyncResetProc,mr);
	enable_modulator_clock(20000);
	SYNC_RESET_DIROUT(); 
	Timer_Start(&mr->syncTimer,100);
	Interp_RegisterCmd(&modCmd);
}
