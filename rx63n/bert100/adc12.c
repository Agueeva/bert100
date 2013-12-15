/**
 * RX63N A/D converter module
 */

#include <math.h>
#include "types.h"
#include "adc12.h"
#include "iodefine.h"
#include "console.h"
#include "interpreter.h"
#include "hex.h"
#include "pvar.h"

#define NR_CHANNELS 21

typedef struct  ADCChan {
	uint16_t channelNr;
} ADCChan;

typedef struct ADC12 {
	ADCChan adch[NR_CHANNELS];
} ADC12;

static ADC12 gAdc12;

int16_t ADC12_Read(int channel)
{
    uint16_t adc_value;
    volatile uint16_t *addr;
    if(channel < 16) {
    	S12AD.ADANS0.WORD = 0x0001 << channel;
    	S12AD.ADANS1.WORD = 0;
    } else if(channel <= 20) {
    	S12AD.ADANS0.WORD = 0;
    	S12AD.ADANS1.WORD = 0x0001 << (channel - 16);
    } else {
	Con_Printf("Illegal A/D channel %lu\n",channel);
	return 0;
    }	
    /* Start a conversion */
    S12AD.ADCSR.BIT.ADST = 1;
    /* Wait for the conversion to end */
    addr = &S12AD.ADDR0;
    while(1 == S12AD.ADCSR.BIT.ADST);
    /* Fetch ADC value */
    adc_value = (uint16_t)(addr[channel] & 0x0FFF);
    return adc_value;
}

static bool 
PVAdc12_SetRaw (void *cbData, uint32_t adId, const char *strP)
{
	ADCChan *ch = cbData;
	Con_Printf("The value of the A/D Channel %u is readonly\n",ch->channelNr);
	return false;
}

static bool 
PVAdc12_GetRaw (void *cbData, uint32_t adId, char *bufP,uint16_t maxlen)
{
	uint32_t rawAdval;
	uint8_t cnt;
	ADCChan *ch = cbData;
	rawAdval = ADC12_Read(ch->channelNr);
	cnt = uitoa16(rawAdval,bufP);		
	bufP[cnt] = 0;
	return true;
}

static bool 
PVAdc12_SetVolt(void *cbData, uint32_t adId, const char *strP)
{
	ADCChan *ch = cbData;
	Con_Printf("The value of the A/D Channel %u is readonly\n",ch->channelNr);
	return false;
}

static bool 
PVAdc12_GetVolt(void *cbData, uint32_t adId, char *bufP,uint16_t maxlen)
{
	uint32_t rawAdval;
	float volt;
	uint8_t cnt;
	ADCChan *ch = cbData;
	rawAdval = ADC12_Read(ch->channelNr);
	volt = rawAdval * 3.300 / 4096;
	cnt = f32toa(volt,bufP,maxlen);
	bufP[cnt] = 0;
	return true;
}
static bool 
PVAdc12_GetDB(void *cbData, uint32_t adId, char *bufP,uint16_t maxlen)
{
	uint32_t rawAdval;
	float volt,db;
	uint8_t cnt;
	ADCChan *ch = cbData;
	rawAdval = ADC12_Read(ch->channelNr);
	volt = rawAdval * 3.300 / 4096;
	db = 10 * (log(volt) / log(10) - log(2.100) / log(10));
	cnt = f32toa(db,bufP,maxlen);
	bufP[cnt] = 0;
	return true;
}

static bool 
PVAdc12_GetTemperature (void *cbData, uint32_t adId, char *bufP,uint16_t maxlen)
{
	uint32_t adval;
	float temperature;
    	S12AD.ADANS0.WORD = 0;
    	S12AD.ADANS1.WORD = 0;
	S12AD.ADEXICR.WORD = (1 << 8); /* Temperature sensor select */
	/* Start a conversion */
	S12AD.ADCSR.BIT.ADST = 1;
	/* Wait for the conversion to end */
    	while(1 == S12AD.ADCSR.BIT.ADST);
	adval = S12AD.ADTSDR;
	temperature = (((adval * 3301) / 4096 - 1260) / 4.1) + 25 - 13;
	bufP[f32toa(temperature,bufP,maxlen)] = 0;
	return true;
}

/**
 ****************************************************************************
 * \fn static int8_t cmd_sci0(Interp * interp, uint8_t argc, char *argv[])
 ****************************************************************************
 */
static int8_t
cmd_adc12(Interp * interp, uint8_t argc, char *argv[])
{
	int channel;
	int32_t adval;
	float volt;
	float db;
	if(argc < 2) {
		return -EC_BADNUMARGS;
	}
	channel = astrtoi16(argv[1]);	
	adval = ADC12_Read(channel);
	volt = adval / 4095. * 3.3;
	if(volt > 0) {
		db = 10 * (log(volt) / log(10) - log(2.100) / log(10));
	} else {
		db = 0;
	}
	Con_Printf("ADVAL: %u, %f V %f db\n",adval, adval / 4095. * 3.300,db);
	return 0;
}

INTERP_CMD(adc12Cmd, "adc12", cmd_adc12, "adc12 <channel-nr> # Read from 12 Bit A/D converter");

/*
 ************************************************************************************************
 * \fn static int8_t cmd_temperature(Interp * interp, uint8_t argc, char *argv[])
 * Meassure the Temperature of the CPU
 ************************************************************************************************
 */
static int8_t
cmd_temperature(Interp * interp, uint8_t argc, char *argv[])
{
	int32_t adval;
	float temperature;
    	S12AD.ADANS0.WORD = 0;
    	S12AD.ADANS1.WORD = 0;
	S12AD.ADEXICR.WORD = (1 << 8); /* Temperature sensor select */
	/* Start a conversion */
	S12AD.ADCSR.BIT.ADST = 1;
	/* Wait for the conversion to end */
    	while(1 == S12AD.ADCSR.BIT.ADST);
	adval = S12AD.ADTSDR;
	temperature = (((adval * 3300) / 4095 - 1260) / 4.1) + 25 - 13;
	Con_Printf("adval %lu, temp %f\n",adval,temperature);
	return 0;	
}

INTERP_CMD(temperatureCmd, "temperature", cmd_temperature, "temperature  # Read temperature");

void
ADC12_Init(void) 
{
	ADC12 *adc = &gAdc12;
	ADCChan *ch;
	int i;
	MSTP_S12AD = 0;
	/* ADC clock = PCLK/8, single scan mode */
	S12AD.ADCSR.BYTE = 0x00;
	S12AD.ADCSR.BIT.CKS = 3;
	for(i = 0; i < NR_CHANNELS; i++) {
		ch = &adc->adch[i];
		ch->channelNr = i;
		PVar_New(PVAdc12_GetRaw,PVAdc12_SetRaw,ch,i,"adc12.raw%u",i);
		PVar_New(PVAdc12_GetVolt,PVAdc12_SetVolt,ch,i,"adc12.ch%u",i);
	}
	for(i = 0; i < 4; i++) {
		ADCChan *pwrChan = &adc->adch[11-i];
		PVar_New(PVAdc12_GetDB,NULL,pwrChan,11 - i,"tx.pwr%u",i);
	}
	PVar_New(PVAdc12_GetTemperature,NULL,adc,0,"system.temp");
	/* enable the Temperature Sensor */
	MSTP_TEMPS = 0;
	TEMPS.TSCR.BIT.TSEN = 1;
	TEMPS.TSCR.BIT.TSOE = 1;
	Interp_RegisterCmd(&adc12Cmd);
	Interp_RegisterCmd(&temperatureCmd);
}
