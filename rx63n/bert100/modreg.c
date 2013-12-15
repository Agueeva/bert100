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
	int32_t advalAfter[4];
	int32_t advalBefore[4];
	/* Mapping from Channel Number to A/D or D/A converter channel number */
	uint32_t adCh[4];
	uint32_t daCh[4];
} ModReg;

static ModReg gModReg;

static void
ModSyncResetProc(void *eventData)
{
	ModReg *mr = eventData;
	float diff;
	uint32_t ch,adCh,daCh;
	Timer_Start(&mr->syncTimer,100);
	for(ch = 0; ch < 4; ch++) {
		adCh = mr->adCh[ch];	
		mr->advalAfter[ch] = ADC12_Read(adCh);
	}
	DISABLE_IRQ();
	SYNC_RESET_HIGH();	
	DelayNs(400);
	SYNC_RESET_LOW();	
	ENABLE_IRQ();
	for(ch = 0; ch < 4; ch++) {
		daCh = mr->daCh[ch];
		adCh = mr->adCh[ch];	
		diff = (mr->advalAfter[ch] - mr->advalBefore[ch]) * 3.3 / 4095;
		//Con_Printf("before %lu, after %lu\n",adval_before,adval_after);
		//Con_Printf("Diff %f\n",diff);
		mr->dacVolt[ch] = mr->dacVolt[ch] + mr->regKI[ch] * diff;
		if(mr->dacVolt[ch] > 10.) {
			mr->dacVolt[ch] = 10.;
		} else if(mr->dacVolt[ch] < -10.) {
			mr->dacVolt[ch] = -10.;
		}
		DAC_Set(daCh,mr->dacVolt[ch]);
		mr->advalBefore[ch] = ADC12_Read(adCh);
	}
}

/*
 ********************************************************************
 * Setup Tmer MTIO4C on Port E5 for clock output for the Modulator. 
 ********************************************************************
 */
static void
enable_modulator_clock(uint32_t hz,float delayUs)
{
	uint32_t delayCnt = (48 * delayUs + 0.5);
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
	uint16_t ch,adCh;
	if((argc == 2)  && (strcmp(argv[1],"clk") == 0)) {
		Con_Printf("PE5: %u\n",PORTE.PIDR.BIT.B5);
		Con_Printf("TCNT %u\n",MTU4.TCNT);
		Con_Printf("TGRC %u\n",MTU4.TGRC);
		return 0;
	} else if((argc == 3)  && (strcmp(argv[1],"delay") == 0)) {
		float delayUs = astrtof32(argv[2]);
		enable_modulator_clock(20000,delayUs);
		return 0;
	} else if(argc < 3) {
		return -EC_BADNUMARGS;
	}
	ch = astrtoi16(argv[1]);
	if(ch > 4) {
		Con_Printf("Bad Channel number\n");
		return -EC_BADARG;
	}
	if((argc == 3)  && (strcmp(argv[2],"ki") == 0)) {
		Con_Printf("Regelkonstante Integral: %f / s\n",mr->regKI[ch] * 10);		
	} else if((argc == 4)  && (strcmp(argv[2],"ki") == 0)) {
		mr->regKI[ch] = astrtof32(argv[3]) / 10;		
		Con_Printf("Regelkonstante Integral: %f / s\n",mr->regKI[ch] * 10);		
	} else if((argc == 3)  && (strcmp(argv[2],"volt") == 0)) {
		Con_Printf("Volt %f\n",mr->dacVolt[ch]);
	} else if((argc == 4)  && (strcmp(argv[2],"volt") == 0)) {
		mr->dacVolt[ch] = astrtof32(argv[3]);
	} else if((argc == 3)  && (strcmp(argv[2],"ad") == 0)) {
		adCh = mr->adCh[ch];
		Con_Printf("StartVal: %lu, EndVal %lu\n",
			mr->advalBefore[adCh],mr->advalAfter[adCh]);
	} else {
		return -EC_BADARG;
	}
	return 0;
}

INTERP_CMD(modCmd, "mod", cmd_mod, "mod <channelNr> ki|volt|ad  ?<value>? ");

void
ModReg_Init(void)
{
	ModReg *mr = &gModReg;
	int i;
	Timer_Init(&mr->syncTimer,ModSyncResetProc,mr);
	for(i = 0; i < 4; i++) {
		mr->regKI[i] = -0.2;
		mr->dacVolt[i] = 0;

		mr->adCh[i] = 3 - i;
		mr->daCh[i] = i;
	}
	enable_modulator_clock(20000,17.5);
	SYNC_RESET_DIROUT(); 
	Timer_Start(&mr->syncTimer,100);
	Interp_RegisterCmd(&modCmd);
}
