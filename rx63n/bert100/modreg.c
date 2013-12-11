/*
 *******************************************************************************************
 * Modulator for Mach Zehner Variant.
 * 0 dbm 2.32 Volt
 * 1 dbm 2.26 Volt
 * -60 mV pro dB
 *******************************************************************************************
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
#include "adc12.h"
#include "ad537x.h"

#define SYNC_RESET_DIROUT()     BSET(3,PORTJ.PDR.BYTE)
#define SYNC_RESET_HIGH()       BSET(3,PORTJ.PODR.BYTE)
#define SYNC_RESET_LOW()        BCLR(3,PORTJ.PODR.BYTE) 

typedef struct ModReg {
	Timer syncTimer;	
	float dacVolt[4];
	float regKI[4];
	int32_t advalAfter;
	int32_t advalBefore;
} ModReg;

static ModReg gModReg;

static void
ModSyncResetProc(void *eventData)
{
	ModReg *mr = eventData;
	float diff;
	Timer_Start(&mr->syncTimer,100);
	gModReg.advalAfter = ADC12_Read(3);
	DISABLE_IRQ();
	SYNC_RESET_HIGH();	
	DelayNs(400);
	SYNC_RESET_LOW();	
	ENABLE_IRQ();
	gModReg.advalBefore = ADC12_Read(3);
	diff = (gModReg.advalAfter - gModReg.advalBefore) * 3.3 / 4095;
	//Con_Printf("before %lu, after %lu\n",adval_before,adval_after);
	//Con_Printf("Diff %f\n",diff);
	mr->dacVolt[0] = mr->dacVolt[0] + mr->regKI[0] * diff;
	if(mr->dacVolt[0] > 10.) {
		mr->dacVolt[0] = 10.;
	} else if(mr->dacVolt[0] < -10.) {
		mr->dacVolt[0] = -10.;
	}
	DAC_Set(0,mr->dacVolt[0]);
}

/*
 ********************************************************************
 * Setup Tmer MTIO4C on Port E5 for clock output for the Modulator. 
 ********************************************************************
 */
static void
enable_modulator_clock(uint32_t hz,uint32_t delayCnt)
{
	MSTP(MTU4) = 0;
	/* Setup PE5 for MTIO4C/MTIO2B */
	MPC.PE5PFS.BYTE = 0x1; /* Switch Pin to MTIO4C mode */
	MTU.TSTR.BIT.CST4 = 0;
	MTU.TOER.BIT.OE4C = 1;	/* Enable the output,must be done before TIOR write */
	PORTE.PMR.BIT.B5 = 1; 

	MTU4.TCNT = 0;	/* Reset counter to avoid phase inversion relative to
			 * second signal */  
	MTU4.TGRC = (F_PCLK / (2 * hz) - 1);
	/* PCLK / 1 */
        MTU4.TCR.BIT.TPSC = 0;
        /* Counter Cleared by TGRC match */
        MTU4.TCR.BIT.CCLR = 5;
        /* Toggle at compare match */
        MTU4.TIORL.BIT.IOC = 7;
	MTU4.TMDR.BIT.MD = 0;   /* Normal mode */


	/* 
	 *********************************************************
	 * Setup PE4 with to be some microseconds before PE5 
 	 * required for delay compensation.
	 *********************************************************
	 */
	MPC.PE4PFS.BYTE = 0x1; /* Switch Pin to MTIO4D mode */
	MTU.TOER.BIT.OE4D = 1;	/* Enable the output,must be done before TIOR write */
	PORTE.PMR.BIT.B4 = 1; 
	MTU4.TGRD = (F_PCLK / (2 * hz) - 1) - delayCnt;
        MTU4.TIORL.BIT.IOD = 7;


	MTU.TSTR.BIT.CST4 = 1;
	
}

static int8_t
cmd_mod(Interp * interp, uint8_t argc, char *argv[])
{
	ModReg *mr = &gModReg;
	if((argc == 2)  && (strcmp(argv[1],"clk") == 0)) {
		Con_Printf("PE5: %u\n",PORTE.PIDR.BIT.B5);
		Con_Printf("TCNT %u\n",MTU4.TCNT);
		Con_Printf("TGRC %u\n",MTU4.TGRC);
	} else if((argc == 2)  && (strcmp(argv[1],"ki") == 0)) {
		Con_Printf("Regelkonstante Integral: %f / s\n",mr->regKI[0] * 10);		
	} else if((argc == 3)  && (strcmp(argv[1],"ki") == 0)) {
		mr->regKI[0] = astrtof32(argv[2]) / 10;		
		Con_Printf("Regelkonstante Integral: %f / s\n",mr->regKI[0] * 10);		
	} else if((argc == 2)  && (strcmp(argv[1],"volt") == 0)) {
		Con_Printf("Volt %f\n",mr->dacVolt[0]);
	} else if((argc == 3)  && (strcmp(argv[1],"volt") == 0)) {
		mr->dacVolt[0] = astrtof32(argv[2]);
	} else if((argc == 3)  && (strcmp(argv[1],"delay") == 0)) {
		float delayUs = astrtof32(argv[2]);
		uint32_t delayCnt = (48 * delayUs + 0.5);
		enable_modulator_clock(20000,delayCnt);
	} else if((argc == 2)  && (strcmp(argv[1],"ad") == 0)) {
		Con_Printf("StartVal: %lu, EndVal %lu\n",
			mr->advalBefore,mr->advalAfter);
	} else {
		return -EC_BADARG;
	}
	return 0;
}

INTERP_CMD(modCmd, "mod", cmd_mod, "mod");

void
ModReg_Init(void)
{
	ModReg *mr = &gModReg;
	int i;
	Timer_Init(&mr->syncTimer,ModSyncResetProc,mr);
	for(i = 0; i < 4; i++) {
		mr->regKI[i] = -0.2;
		mr->dacVolt[i] = 0;
	}
	enable_modulator_clock(20000,840);
	SYNC_RESET_DIROUT(); 
	Timer_Start(&mr->syncTimer,100);
	Interp_RegisterCmd(&modCmd);
}
