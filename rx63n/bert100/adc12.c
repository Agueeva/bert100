/**
 * RX63N A/D converter module
 */

#include <string.h>
#include <math.h>
#include "types.h"
#include "adc12.h"
#include "iodefine.h"
#include "console.h"
#include "interpreter.h"
#include "hex.h"
#include "pvar.h"
#include "version.h"
#include "database.h"

#define NR_CHANNELS 21

typedef struct  ADCChan {
	uint16_t channelNr;
} ADCChan;

typedef struct ADC12 {
	ADCChan adch[NR_CHANNELS];
	float pwrRef[4];
	float tempSensCorr;
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

float
ADC12_ReadVolt(int channel)
{
	return ADC12_Read(channel) * 3.3 / 4096;
}

float
ADC12_ReadVoltMultiple(int channel,uint8_t repeatCnt)
{
	int i;
	uint32_t sum;
	for(i = 0, sum = 0; i < repeatCnt; i++) {
		sum += ADC12_Read(channel);
	}
	return sum * 3.3 / (4096 * repeatCnt);
}

/**
  *****************************************************************
  * \fn float ADC12_GetTemperature(void)
  *****************************************************************
  */
float
ADC12_GetTemperature(void)
{
	int32_t adval;
	float temperature;
	ADC12 *adc = &gAdc12;
    	S12AD.ADANS0.WORD = 0;
    	S12AD.ADANS1.WORD = 0;
	S12AD.ADEXICR.WORD = (1 << 8); /* Temperature sensor select */
	/* Start a conversion */
	S12AD.ADCSR.BIT.ADST = 1;
	/* Wait for the conversion to end */
    	while(1 == S12AD.ADCSR.BIT.ADST);
	adval = S12AD.ADTSDR;
	S12AD.ADEXICR.WORD = 0; /* Temperature sensor unselect */
	temperature = (((adval * 3301) / 4096 - 1260) / 4.1) + 25 - adc->tempSensCorr;
	return temperature;
}
static bool 
PVAdc12_SetRaw (void *cbData, uint32_t chNr, const char *strP)
{
	Con_Printf("The value of the A/D Channel %u is readonly\n",chNr);
	return false;
}

static bool 
PVAdc12_GetRaw (void *cbData, uint32_t chNr, char *bufP,uint16_t maxlen)
{
	uint32_t rawAdval;
	uint8_t cnt;
	rawAdval = ADC12_Read(chNr);
	cnt = uitoa16(rawAdval,bufP);		
	bufP[cnt] = 0;
	return true;
}

static bool 
PVAdc12_SetVolt(void *cbData, uint32_t chNr, const char *strP)
{
	Con_Printf("The value of the A/D Channel %u is readonly\n",chNr);
	return false;
}

static bool 
PVAdc12_GetVolt(void *cbData, uint32_t chNr, char *bufP,uint16_t maxlen)
{
	uint32_t rawAdval;
	float volt;
	uint8_t cnt;
	rawAdval = ADC12_Read(chNr);
	volt = rawAdval * 3.300 / 4096;
	cnt = f32toa(volt,bufP,maxlen);
	bufP[cnt] = 0;
	return true;
}

static bool 
PVAdc12_GetDB(void *cbData, uint32_t chNr, char *bufP,uint16_t maxlen)
{
	uint32_t rawAdval;
	uint32_t adCh = 7 - chNr;
	float volt,db;
	uint8_t cnt;
	ADC12 *adc = cbData;
	rawAdval = ADC12_Read(adCh);
	volt = rawAdval * 3.300 / 4096;
	db = 10 * (log(volt) / log(10)) - adc->pwrRef[chNr];
	cnt = f32toa(db,bufP,maxlen);
	bufP[cnt] = 0;
	return true;
}

static bool
PVAdc12_SetPwrRef(void *cbData, uint32_t chNr, const char *strP)
{
        float val;
	ADC12 *adc = cbData;
        val = astrtof32(strP);
	DB_VarWrite(DBKEY_PWRREF(chNr),&val);
	adc->pwrRef[chNr] = val;
	return true;
}

static bool
PVAdc12_GetPwrRef(void *cbData, uint32_t chNr, char *bufP,uint16_t maxlen)
{
        float val;
	uint8_t cnt;
	ADC12 *adc = cbData;
	val = adc->pwrRef[chNr];
	cnt = f32toa(val,bufP,maxlen);
	bufP[cnt] = 0;
	return true;
}

static bool 
PVAdc12_GetTemperature (void *cbData, uint32_t adId, char *bufP,uint16_t maxlen)
{
	float temperature;
	temperature = ADC12_GetTemperature();
#if 0
	int32_t adval;
    	S12AD.ADANS0.WORD = 0;
    	S12AD.ADANS1.WORD = 0;
	S12AD.ADEXICR.WORD = (1 << 8); /* Temperature sensor select */
	/* Start a conversion */
	S12AD.ADCSR.BIT.ADST = 1;
	/* Wait for the conversion to end */
    	while(1 == S12AD.ADCSR.BIT.ADST);
	adval = S12AD.ADTSDR;
	S12AD.ADEXICR.WORD = 0; /* Temperature sensor unselect */
	temperature = (((adval * 3301) / 4096 - 1260) / 4.1) + 25 - 13;
#endif
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
	if(argc == 1) {
		Con_Printf("MSTP_S12AD %u\n", MSTP_S12AD);
		Con_Printf("ADCSR  0x%02x\n",S12AD.ADCSR.BYTE);
		Con_Printf("ADEXIR %02x\n",S12AD.ADEXICR.WORD);
    		Con_Printf("ADANS0 %04x\n",S12AD.ADANS0.WORD);
    		Con_Printf("ADANS1 %04x\n",S12AD.ADANS1.WORD);
		return 0;
	}
	if(strcmp(argv[1],"reset") == 0) {

		
	} else if(strcmp(argv[1],"ref") == 0) {
    		S12AD.ADANS0.WORD = 0;
    		S12AD.ADANS1.WORD = 0;
		S12AD.ADEXICR.BIT.OCS = 1; /* Ref. Voltage select */
		/* Start a conversion */
		S12AD.ADCSR.BIT.ADST = 1;
		/* Wait for the conversion to end */
		while(1 == S12AD.ADCSR.BIT.ADST);
		adval = S12AD.ADOCDR;
		S12AD.ADEXICR.BIT.OCS = 0; /* Ref. Voltage uselect */
		volt = ((adval * 3.300) / 4096);
		Con_Printf("Ref: %f Volt\n",volt);
		return 0;
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
	float temperature;
	float tempCorr;
	ADC12 *adc = &gAdc12;
	if(argc == 1) {
		temperature = ADC12_GetTemperature();
		Con_Printf("CPU Temp. %f C\n",temperature);
	} else if(argc == 2) {
		float measTemp;
		temperature = astrtof32(argv[1]); 
		measTemp = ADC12_GetTemperature();
		tempCorr = temperature - measTemp;
		adc->tempSensCorr -= tempCorr;
		Con_Printf("Temp Sensor correction ist now %f C\n",adc->tempSensCorr);
		DB_VarWrite(DBKEY_TEMPSENS_CORR(channel),&adc->tempSensCorr);
	}
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
	S12AD.ADCSR.BIT.CKS = 2;
	for(i = 0; i < NR_CHANNELS; i++) {
		ch = &adc->adch[i];
		ch->channelNr = i;
		PVar_New(PVAdc12_GetRaw,PVAdc12_SetRaw,adc,i,"adc12.raw%u",i);
		PVar_New(PVAdc12_GetVolt,PVAdc12_SetVolt,adc,i,"adc12.ch%u",i);
	}
	for(i = 0; i < 4; i++) {
		DB_VarInit(DBKEY_PWRREF(i),&adc->pwrRef[i],"tx%u.pwrRef",i);
		PVar_New(PVAdc12_GetDB,NULL,adc,i,"tx%u.pwr",i);
		PVar_New(PVAdc12_GetPwrRef,PVAdc12_SetPwrRef,adc,i,"tx%u.pwrRef",i);
	}
	PVar_New(PVAdc12_GetTemperature,NULL,adc,0,"system.temp");
	/* enable the Temperature Sensor */
	MSTP_TEMPS = 0;
	TEMPS.TSCR.BIT.TSEN = 1;
	TEMPS.TSCR.BIT.TSOE = 1;
	adc->tempSensCorr = 0;
	DB_VarInit(DBKEY_TEMPSENS_CORR(channel),&adc->tempSensCorr,"adc12.tempSensCorr");

	Interp_RegisterCmd(&adc12Cmd);
	Interp_RegisterCmd(&temperatureCmd);
}
