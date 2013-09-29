/**
 **********************************************************************************
 * Interface to the Silabs Si570 Programmable Oscilator XO
 **********************************************************************************
 */

#include "types.h"
#include "pvar.h"
#include "interpreter.h"
#include "console.h"
#include "hex.h"
#include "xo.h"
#include "i2cmaster.h"


#if 0
#define REG_N1H_HSDIV	(7)
#define REG_N1L_RFREQ32	(8)
#define REG_RFREQ24	(9)
#define REG_RFREQ16	(10)
#define REG_RFREQ8	(11)
#define REG_RFREQ0	(12)
#else
#define REG_N1H_HSDIV	(13)
#define REG_N1L_RFREQ32	(14)
#define REG_RFREQ24	(15)
#define REG_RFREQ16	(16)
#define REG_RFREQ8	(17)
#define REG_RFREQ0	(18)
#endif

#define REG_FREEZE	(135)
#define 	RST_REG         (1 << 7)
#define 	NEW_FREQ        (1 << 6)
#define 	FREEZE_M        (1 << 5)
#define 	FREEZE_VCADC    (1 << 4)
#define 	RECALL          (1 << 0)

#define REG_FREEZE_DCO	(137)
#define 	FREEZE_DCO      (1 << 4)

typedef struct SiXO {
	uint16_t i2cAddr;
	uint32_t fXTAL;
	uint32_t outFreq;
	uint32_t maxFreq;
	uint32_t minFreq;
} SiXO;

static SiXO gSiXO[1];

/**
 *************************************************************************
 * the list of possible hsdiv dividers (not the register values) .
 *************************************************************************
 */
static const uint8_t hsdiv_tab[] = {
	4, 5, 6, 7, 9, 11
};


static bool 
set_frequency(SiXO *xo,uint32_t freq) 
{
	uint8_t buf[4];
	int i;
	uint8_t i2c_result;
	uint32_t n1,hs_div;
	uint32_t best_n1,best_hs_div;
	uint64_t rfreq;
	uint32_t div;
	uint32_t fxtal = xo->fXTAL;
	uint64_t maxdco = UINT64_C(5670000000);
	uint64_t mindco = UINT64_C(4850000000);
	uint64_t optdco = (maxdco + mindco) >> 1; 
	uint32_t optDividers = optdco / freq;
	float dev_factor,best_factor;

	if(freq > xo->maxFreq || freq < xo->minFreq) {
		return false;
	}
	if(freq == xo->outFreq) {
		return true;
	}
	//Con_Printf("Optimal dividers %lu\n",(uint32_t)optDividers);
	best_factor = 1000;
	best_n1 = best_hs_div =  0;
	for(i = 0; i < array_size(hsdiv_tab); i++) {
		hs_div = hsdiv_tab[i];
		n1 = optDividers / hs_div;	
		if((n1 < 1) || (n1 > 129)) {
			continue;
		}
		dev_factor = 1.0 * optDividers / (n1 * hs_div);
		if(dev_factor < 1) {
			dev_factor = 1 / dev_factor;
		}
		if(dev_factor < best_factor) {
			best_factor = dev_factor;
			best_n1 = n1;
			best_hs_div = hs_div;
			//Con_Printf("opt %f, bestn1 %d, best_hs_div %d\n",best_factor,best_n1,hs_div);
		}
	}
	n1 = best_n1; 
	hs_div = best_hs_div;
	div = fxtal * hs_div * n1;
	rfreq = ((1 << 28) * (uint64_t)freq + (fxtal >> 1)) / fxtal * hs_div * n1;
#if 0
	Con_Printf("n1 %lu hsdiv %lu\n",n1,hs_div);
	Con_Printf("rfreq %llx\n",rfreq);
	SleepMs(100);
#endif
	/* Now write everything to the I2C registers */
	buf[0] = FREEZE_DCO;
	i2c_result = I2C_Write8(xo->i2cAddr, REG_FREEZE_DCO, buf,1);
	buf[0] = FREEZE_M;
	i2c_result = I2C_Write8(xo->i2cAddr, REG_FREEZE, buf,1);
	if(i2c_result != I2C_RESULT_OK) {
		Con_Printf("I2C error Code %d\n",i2c_result);
		return false;
	}
	buf[0] = ((hs_div - 4) << 5) | (((n1 - 1) & 0x7f) >> 2);
	buf[1] = (((n1 - 1) & 3) << 6) | ((rfreq >> 32) & 0x3f);
	i2c_result = I2C_Write8(xo->i2cAddr, REG_N1H_HSDIV, buf,2);
	buf[0] = (rfreq >> 24) & 0xff; 
	buf[1] = (rfreq >> 16) & 0xff; 
	buf[2] = (rfreq >> 8) & 0xff; 
	buf[3] = (rfreq >> 0) & 0xff; 
	i2c_result = I2C_Write8(xo->i2cAddr, REG_RFREQ24, buf,4);
	buf[0] = 0;
	i2c_result = I2C_Write8(xo->i2cAddr, REG_FREEZE_DCO, buf,1);
#if 0
	buf[0] = 0;
	i2c_result = I2C_Write8(xo->i2cAddr, REG_FREEZE, buf,1);
#endif
	buf[0] = NEW_FREQ;
	i2c_result = I2C_Write8(xo->i2cAddr, REG_FREEZE, buf,1);

	// Should check for lock before doing this : */
	xo->outFreq = freq;

	return true;
}

/**
 *****************************************************************
 * Readback the Frequency from I2C-Bus. Should only be used
 * on startup.
 *****************************************************************
 */
static bool 
read_frequency(SiXO *xo,uint32_t *freqRet) 
{
	uint8_t buf[6];
	uint8_t n1,hsdiv;
	uint8_t i2c_result;
	uint64_t rfreq;
	uint32_t outfreq;
	uint64_t fdco;

	i2c_result = I2C_Read8(xo->i2cAddr, REG_N1H_HSDIV, buf,6);
	if(i2c_result != I2C_RESULT_OK) {
		return false;
	}
	n1 = ((buf[0] & 0x1f) << 2);
	hsdiv = (buf[0] >> 5) & 7; 
	n1 = (n1 & 0xfc) | ((buf[1] >> 6) & 3);
	hsdiv = hsdiv + 4;
	n1 = n1 + 1;
	rfreq = (uint64_t)(buf[1] & 0x3f) << 32;
	rfreq |= (uint32_t)(buf[2]) << 24;
	rfreq |= (uint32_t)(buf[3]) << 16;
	rfreq |= (uint32_t)(buf[4]) << 8;
	rfreq |= (uint32_t)(buf[5]) << 0;
	outfreq = rfreq / hsdiv / n1 * xo->fXTAL / (1 << 28);
	fdco = rfreq / 64 * xo->fXTAL / (1 << 22);
	//Con_Printf("RFREQ %llx\n",rfreq);
	//Con_Printf("DCO %llu\n",fdco);	
	//Con_Printf("len %u\n",uitoa64(fdco,str));
	//Con_Printf("DCO %s\n",str);	
	if(freqRet) {
		*freqRet = outfreq;
	}
	return true;
}
/**
 ******************************************************************************
 * \fn static int8_t cmd_synth(Interp * interp, uint8_t argc, char *argv[])
 * Command shell interface for controlling the Silabs Si570 Oscillator 
 ******************************************************************************
 */

static int8_t
cmd_synth(Interp * interp, uint8_t argc, char *argv[])
{
	SiXO *xo = &gSiXO[0];	
	uint32_t freq;		
	if(argc < 2) {
		if(read_frequency(xo,&freq) == true) {
			Con_Printf("XO Frequency %lu\n",freq);	
			Con_Printf("XTAL Frequency %lu\n",xo->fXTAL);	
		} else {
			Con_Printf("Read Frequency failed\n");	
		}
		return 0;
#if 0
	} else if(strcmp(argv[1],"ppm") == 0) {
		float ppm = 1000000 * fabs((1. - (xo->fXTAL /  114285000)));	
		Con_Printf("ppm %lu\n",(1. - (xo->fXTAL /  114285000)));	
#endif
	}
	freq = astrtoi32(argv[1]);
	set_frequency(xo,freq);
        return 0;
}
INTERP_CMD(synthCmd,"synth", cmd_synth, "synth  ?<freq>? # Set/Get frequency of synthesizer");

static bool 
PVSynth_SetFreq (void *cbData, uint32_t adId, const char *strP)
{
	SiXO *xo = cbData;
	uint64_t freq;
	freq = astrtoi32(strP);
	set_frequency(xo,freq);
	return true;
}

static bool 
PVSynth_GetFreq (void *cbData, uint32_t adId, char *bufP,uint16_t maxlen)
{
	SiXO *xo = cbData;
	if(xo->outFreq == 0) {
		read_frequency(xo,&xo->outFreq);
	}
	SNPrintf(bufP,maxlen,"%lu",xo->outFreq);
	return true;
}

static bool 
PVSynth_GetFXTAL(void *cbData, uint32_t adId, char *bufP,uint16_t maxlen)
{
	SiXO *xo = cbData;
	SNPrintf(bufP,maxlen,"%lu",xo->fXTAL);
	return true;
}

#define ORDERED_FREQUENCY 644531250		

void
XO_Init(const char *name,uint16_t i2cAddr) 
{
	SiXO *xo = &gSiXO[0];	
	uint8_t buf[6];
	uint8_t i2c_result;
	uint32_t freqRet;
	xo->i2cAddr = i2cAddr;	
	xo->fXTAL = 114285000;
	xo->maxFreq = 810 * 1000000;
	xo->minFreq = 10 * 1000000;
	xo->outFreq = 0;
	PVar_New(PVSynth_GetFreq,PVSynth_SetFreq,xo,0,"%s.freq",name);
	PVar_New(PVSynth_GetFXTAL,NULL,xo,0,"%s.fxtal",name);
	buf[0] = RECALL;
	i2c_result = I2C_Write8(xo->i2cAddr, REG_FREEZE, buf,1);
	if(i2c_result != I2C_RESULT_OK) {
		Con_Printf("Failed to read write to synthesizer\n");
	} else if(read_frequency(xo,&freqRet) == false) {
		Con_Printf("Failed to read frequency from Synthesizer\n");
	} else {
		xo->fXTAL = ((uint64_t)xo->fXTAL * ORDERED_FREQUENCY + (freqRet >> 1)) / freqRet;
	}
	Interp_RegisterCmd(&synthCmd);
}
