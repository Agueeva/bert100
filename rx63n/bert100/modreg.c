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
#include <math.h>
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
#include "pvar.h"
#include "version.h"
#include "modreg.h"

#define SYNC_RESET_DIROUT()     BSET(3,PORTJ.PDR.BYTE)
#define SYNC_RESET_HIGH()       BSET(3,PORTJ.PODR.BYTE)
#define SYNC_RESET_LOW()        BCLR(3,PORTJ.PODR.BYTE) 
#define	CTRL_FAULT_LIMIT	(0.5) // Volt

typedef struct ModReg {
	Timer syncTimer;	
	float dacVolt[4];
	float regKI[4];
	float regKIEffPerInterval[4];
	bool  ctrlEnable[4];
	int32_t advalAfter[4];
	int32_t advalBefore[4];
	float deviation[4];
	float ctrlDevFiltered[4];
	bool ctrlLatchedFault[4];
	/* Mapping from Channel Number to A/D or D/A converter channel number */
	uint32_t adCh[4];
	uint32_t daCh[4];
	float rectDelayUs;
} ModReg;

static ModReg gModReg;

static void
ModulatorControlProc(void *eventData)
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
	DelayUs(10);
	for(ch = 0; ch < 4; ch++) {
		float pwrVolt;
		daCh = mr->daCh[ch];
		adCh = mr->adCh[ch];	
		diff = (mr->advalAfter[ch] - mr->advalBefore[ch]) * 3.3 / 4096;
		//Con_Printf("before %lu, after %lu\n",adval_before,adval_after);
		//Con_Printf("Diff %f\n",diff);
		mr->dacVolt[ch] = mr->dacVolt[ch] + mr->regKIEffPerInterval[ch] * diff;
		if(mr->dacVolt[ch] > 8.5) {
			mr->dacVolt[ch] = 0.;
		} else if(mr->dacVolt[ch] < -8.5) {
			mr->dacVolt[ch] = 0.;
		}
		DAC_Set(daCh,mr->dacVolt[ch]);
		mr->advalBefore[ch] = ADC12_Read(adCh);
		mr->ctrlDevFiltered[ch] = (mr->ctrlDevFiltered[ch] * 0.9) + (diff / 10); // IIR filtered

		pwrVolt = ADC12_ReadVolt(ADCH_TX_PWR(ch));
		if(fabs(mr->ctrlDevFiltered[ch]) > CTRL_FAULT_LIMIT) {
			mr->ctrlLatchedFault[ch] = true;
		} else if((pwrVolt < 0.21) || (pwrVolt > 3.2)) {
			mr->ctrlLatchedFault[ch] = true;
		}
		mr->deviation[ch] = diff;
	}
}

/*
 ********************************************************************
 * Setup Tmer MTIO4C on Port E5 for clock output for the Modulator. 
 ********************************************************************
 */
static void
enable_modulator_clock(ModReg *mr,uint32_t hz,float delayUs)
{
	uint32_t delayCnt;
	mr->rectDelayUs = delayUs;

	delayCnt = (48 * delayUs + 0.5);
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

static void
update_eff_ki(ModReg *mr,uint8_t chNr) 
{
	if(chNr >= array_size(mr->ctrlEnable)) {
		return;
	}
	if(mr->ctrlEnable[chNr]) {
		mr->regKIEffPerInterval[chNr] = mr->regKI[chNr] / 10;
	} else {
		mr->regKIEffPerInterval[chNr] = 0;
	}
}

void 
ModReg_ResetCtrlFault(uint8_t chNr) 
{
	ModReg *mr = &gModReg;
	if(chNr < array_size(mr->ctrlLatchedFault)) {
		mr->ctrlLatchedFault[chNr] = false;	
	}
}

/**
 ***************************************************************************************
 * \fn static int8_t cmd_mod(Interp * interp, uint8_t argc, char *argv[])
 * Command Shell interface to the Modulator controller.
 ***************************************************************************************
 */
static int8_t
cmd_mod(Interp * interp, uint8_t argc, char *argv[])
{
	ModReg *mr = &gModReg;
	uint16_t ch;
	if((argc == 2)  && (strcmp(argv[1],"clk") == 0)) {
		Con_Printf("PE5: %u\n",PORTE.PIDR.BIT.B5);
		Con_Printf("TCNT %u\n",MTU4.TCNT);
		Con_Printf("TGRC %u\n",MTU4.TGRC);
		return 0;
	} else if((argc == 2)  && (strcmp(argv[1],"delay") == 0)) {
		Con_Printf("%f us\n",mr->rectDelayUs);
		return 0;
	} else if((argc == 3)  && (strcmp(argv[1],"delay") == 0)) {
		float delayUs = astrtof32(argv[2]);
		enable_modulator_clock(mr,20000,delayUs);
		DB_VarWrite(DBKEY_MODREG_RECT_DELAY,&mr->rectDelayUs);
		return 0;
	} else if(argc < 3) {
		return -EC_BADNUMARGS;
	}
	ch = astrtoi16(argv[1]);
	if(ch >= 4) {
		Con_Printf("Bad Channel number\n");
		return -EC_BADARG;
	}
	if((argc == 3)  && (strcmp(argv[2],"ki") == 0)) {
		Con_Printf("Regelkonstante Integral: %f / s\n",mr->regKI[ch]);		
	} else if((argc == 4)  && (strcmp(argv[2],"ki") == 0)) {
		mr->regKI[ch] = astrtof32(argv[3]);		
		update_eff_ki(mr,ch);
	} else if((argc == 3)  && (strcmp(argv[2],"volt") == 0)) {
		Con_Printf("Volt %f\n",mr->dacVolt[ch]);
	} else if((argc == 4)  && (strcmp(argv[2],"volt") == 0)) {
		mr->dacVolt[ch] = astrtof32(argv[3]);
	} else if((argc == 3)  && (strcmp(argv[2],"ad") == 0)) {
		Con_Printf("StartVal: %lu, EndVal %lu ControlDev. %f, filtered %f\n",
			mr->advalBefore[ch],mr->advalAfter[ch],
			(mr->advalBefore[ch] - mr->advalAfter[ch]) * 3.300/4096,mr->ctrlDevFiltered[ch]);
	} else {
		return -EC_BADARG;
	}
	return 0;
}

INTERP_CMD(modCmd, "mod", cmd_mod, "mod <channelNr> ki|volt|ad  ?<value>? ");

/**
 ****************************************************************************************************
 * Modulator Bias: Read from the Control loop, written to the control Loop and directly 
 * to the  D/A converter.
 ****************************************************************************************************
 */
static bool
PVModBias_Set (void *cbData, uint32_t chNr, const char *strP)
{
	ModReg *mr = cbData;
        float val;
	uint32_t daCh;
	if(chNr >= array_size(mr->daCh)) {
		return false;
	}
	daCh = mr->daCh[chNr];
        val = astrtof32(strP);
	mr->dacVolt[chNr] = val;
        return DAC_Set(daCh,val);
}

static bool
PVModBias_Get (void *cbData, uint32_t chNr, char *bufP,uint16_t maxlen)
{
	ModReg *mr = cbData;
        float dacval;
        uint8_t cnt;
	if(chNr >= array_size(mr->dacVolt)) {
		return false;
	}
        dacval = mr->dacVolt[chNr]; 
        cnt = f32toa(dacval,bufP,maxlen);
        bufP[cnt] = 0;
        return true;
}

/*
 *****************************************************************************************
 * Get the difference between the voltage at the Integrator before and after the
 * meassurement interval. 
 *****************************************************************************************
 */
static bool
PVCtrlDev_Get (void *cbData, uint32_t chNr, char *bufP,uint16_t maxlen)
{
	ModReg *mr = cbData;
        float deviation;
        uint8_t cnt;
	if(chNr >= array_size(mr->dacVolt)) {
		return false;
	}
	/* normalize by multiplikation with 1/(meassurement Interval) */
	//deviation = (mr->advalBefore[chNr] - mr->advalAfter[chNr]) * 3.300/4096;
	deviation = mr->deviation[chNr];
        cnt = f32toa(deviation,bufP,maxlen);
        bufP[cnt] = 0;
        return true;
}
/**
 ***************************************************************************************
 * \fn static bool ModReg_Set/GetKi (void *cbData, uint32_t chNr, const char *strP)
 * Get/Set the Integral Konstant of the Control Loop
 ***************************************************************************************
 */

void
ModReg_SetKi(uint8_t chNr, float Ki) 
{
	ModReg *mr = &gModReg;
	mr->regKI[chNr] = Ki;		
	update_eff_ki(mr,chNr);
}

float
ModReg_GetKi(uint8_t chNr) 
{
	ModReg *mr = &gModReg;
	return mr->regKI[chNr];
}

/**
 ******************************************************************************************
 * Process variable interface for setting/getting integral constant of control loop
 ******************************************************************************************
 */
static bool
PVModKi_Set (void *cbData, uint32_t chNr, const char *strP)
{
	ModReg *mr = cbData;
        float val;
	if(chNr >= array_size(mr->regKI)) {
		return false;
	}
        val = astrtof32(strP);
	mr->regKI[chNr] = val;		
	update_eff_ki(mr,chNr);
        return true;
}

static bool
PVModKi_Get (void *cbData, uint32_t chNr, char *bufP,uint16_t maxlen)
{
	ModReg *mr = cbData;
        float ki;
        uint8_t cnt;
	if(chNr >= array_size(mr->regKI)) {
		return false;
	}
        ki = mr->regKI[chNr]; 
        cnt = f32toa(ki,bufP,maxlen);
        bufP[cnt] = 0;
        return true;
}

/*
 *****************************************************************************
 * Read or Set the Bias Voltage generated by the control loop.
 *****************************************************************************
 */
static bool
PVBias_Set (void *cbData, uint32_t chNr, const char *strP)
{
	ModReg *mr = cbData;
        float val;
	if(chNr >= array_size(mr->dacVolt)) {
		return false;
	}
        val = astrtof32(strP);
	mr->dacVolt[chNr] = val;		
        return true;
}

static bool
PVBias_Get (void *cbData, uint32_t chNr, char *bufP,uint16_t maxlen)
{
	ModReg *mr = cbData;
        float val;
        uint8_t cnt;
	if(chNr >= array_size(mr->dacVolt)) {
		return false;
	}
        val = mr->dacVolt[chNr]; 
        cnt = f32toa(val,bufP,maxlen);
        bufP[cnt] = 0;
        return true;
}

/**
 ****************************************************************************************
 * \fn static bool PVCtrlEnable_Set (void *cbData, uint32_t chNr, const char *strP)
 * Control loop enable/disable. Required because controller has to keep Ki 
 * during disabled loop. An effective Ki is updated instead of modifying Ki. 
 ****************************************************************************************
 */
static bool
PVCtrlEnable_Set (void *cbData, uint32_t chNr, const char *strP)
{
	ModReg *mr = cbData;
        bool enable;
	if(chNr >= array_size(mr->ctrlEnable)) {
		return false;
	}
        enable = astrtoi16(strP);
	mr->ctrlEnable[chNr] = enable;		
	update_eff_ki(mr,chNr);
        return true;
}

static bool
PVCtrlEnable_Get (void *cbData, uint32_t chNr, char *bufP,uint16_t maxlen)
{
	ModReg *mr = cbData;
        bool enable;
        uint8_t cnt;
	if(chNr >= array_size(mr->ctrlEnable)) {
		return false;
	}
        enable = mr->ctrlEnable[chNr]; 
        cnt = uitoa16(enable,bufP);
        bufP[cnt] = 0;
        return true;
}

/**
 ************************************************************************************************
 * \fn static bool PVCtrlFault_Get (void *cbData, uint32_t chNr, char *bufP,uint16_t maxlen)
 ************************************************************************************************
 */
static bool
PVCtrlFault_Get (void *cbData, uint32_t chNr, char *bufP,uint16_t maxlen)
{
	ModReg *mr = cbData;
	bool fault;
	float pwrVolt;
//	bool enable;
//        enable = mr->ctrlEnable[chNr]; 
	pwrVolt = ADC12_ReadVolt(ADCH_TX_PWR(chNr));
#if 0
	if(!enable) {
		fault = false;	
	} else 
#endif
	if((pwrVolt < 0.21) || (pwrVolt > 3.2)) {
		fault = true;	
	} else if(fabs(mr->ctrlDevFiltered[chNr]) > CTRL_FAULT_LIMIT) {
		fault = true;
	} else {
		fault = false;
	}
        bufP[0] = '0' + fault;
	bufP[1] = 0;
        return true;
}

static bool
PVLatchedFault_Get (void *cbData, uint32_t chNr, char *bufP,uint16_t maxlen)
{
	ModReg *mr = cbData;
	if(mr->ctrlLatchedFault[chNr] == true) {
        	bufP[0] = '1';
	} else {
        	bufP[0] = '0';
	}
	bufP[1] = 0;
        return true;
}

static bool
PVLatchedFault_Set (void *cbData, uint32_t chNr, const char *strP)
{
	ModReg *mr = cbData;
	mr->ctrlLatchedFault[chNr] = !!astrtoi16(strP);
        return true;
}


/**
 ****************************************************************************
 * \fn void ModReg_Init(void)
 * Initialize the Mach Zehnder Modulator controller
 ****************************************************************************
 */

void
ModReg_Init(void)
{
	ModReg *mr = &gModReg;
	int ch;
	if(Variant_Get() != VARIANT_MZ) {
		return;
	}
	Timer_Init(&mr->syncTimer,ModulatorControlProc,mr);
	for(ch = 0; ch < 4; ch++) {
		mr->regKI[ch] = -0.5;
		mr->ctrlEnable[ch] = true;
		update_eff_ki(mr,ch);
		mr->dacVolt[ch] = 0;

		mr->adCh[ch] = 3 - ch;
		mr->daCh[ch] = ch;
                PVar_New(PVModBias_Get,PVModBias_Set,mr,ch,"mzMod%d.modBias",ch);
                PVar_New(PVModKi_Get,PVModKi_Set,mr,ch,"mzMod%d.Ki",ch);
		PVar_New(PVCtrlEnable_Get,PVCtrlEnable_Set,mr,ch,"mzMod%d.ctrlEnable",ch);
                PVar_New(PVCtrlDev_Get,NULL,mr,ch,"mzMod%d.ctrlDev",ch);
                PVar_New(PVBias_Get,PVBias_Set,mr,ch,"mzMod%d.bias",ch);
                PVar_New(PVCtrlFault_Get,NULL,mr,ch,"mzMod%d.ctrlFault",ch);
                PVar_New(PVLatchedFault_Get,PVLatchedFault_Set,mr,ch,"mzMod%d.latchedCtrlFault",ch);
	}
	mr->rectDelayUs = 12.0;
	DB_VarInit(DBKEY_MODREG_RECT_DELAY,&mr->rectDelayUs,"mzMod.rectDelay");
	enable_modulator_clock(mr,20000,mr->rectDelayUs);

	SYNC_RESET_DIROUT(); 
	Timer_Start(&mr->syncTimer,100);
	Interp_RegisterCmd(&modCmd);
}
