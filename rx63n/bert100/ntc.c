/**
 *
 */

#include "types.h"
#include "timer.h"
#include "adc12.h"
#include "interpreter.h"
#include "database.h"
#include "hex.h"
#include "console.h"
#include "ntc.h"
#include "pvar.h"
#include "ntc.h"
#include <stdlib.h>
#include <string.h>

#define NTC_EC_OK               (0)
#define NTC_EC_RANGE            (1)

#define ADCH_NTC_MOD    (12)
#define ADCH_NTC_AMP    (13)


static float gRPreMod = 10000.0;
static float gRPreAmp = 10000.0;

typedef struct Point {
        float temp;
        float R;
} Point;

const Point points[33] = {
        {150.0, 0.030166},
        {145.0, 0.033451},
        {140.0, 0.037179},
        {135.0, 0.04142},
        {130.0, 0.046259},
        {125.0, 0.051794},
        {120.0, 0.058144},
        {115.0, 0.06545},
        {110.0, 0.073881},
        {105.0, 0.08364},
        {100.0, 0.094973},
        {95.0, 0.10818},
        {90.0, 0.12362},
        {85.0, 0.14174},
        {80.0, 0.16307},
        {75.0, 0.18830},
        {70.0, 0.21824},
        {65.0, 0.25392},
        {60.0, 0.29663},
        {55.0, 0.34798},
        {50.0, 0.41000},
        {45.0, 0.48525},
        {40.0, 0.57703},
        {35.0, 0.6895},
        {30.0, 0.8218},
        {25.0, 1.0000},
        {20.0, 1.2142},
        {15.0, 1.4827},
        {10.0, 1.8216},
        {5.0, 2.2520},
        {0.0, 2.8024},
        {-50.0, 3.8452},
        {-100.0, 5.5297},
};

/**
 ***************************************************************************
 * Calculate the Temperature using a Table and linear
 * interpolation.
 ***************************************************************************
 */
static float 
NTC_Interpol(float ohm,float ntcOhm)
{
        const Point *p, *prev;
        float diff_ohm;
        float diff_temp;
        float temp;
        int i;
	ohm = ohm / ntcOhm;
        p = prev = NULL;
        for (i = 1; i < array_size(points); i++) {
                p = &points[i];
                if (ohm < p->R) {
                        break;
                }
        }
        prev = &points[i - 1];
        /* It is not allowed to have two equal entries to avoid division by zero */
        diff_ohm = p->R - prev->R;
        diff_temp = p->temp - prev->temp;
        temp = prev->temp + (ohm - prev->R) * (diff_temp) / diff_ohm;
        return temp;
}

/**
 ********************************************************************************
 * Read from the NTC
 ********************************************************************************
 */
static float 
NTC_Read(int16_t adchannel) 
{
	float volt = ADC12_ReadVolt(adchannel); 
	float Ri;
	float Rv;
	float ohm;
	Ri = 1000.0;
	if(adchannel == ADCH_NTC_MOD) {
		Rv = gRPreMod;
	} else if (adchannel == ADCH_NTC_AMP) {
		Rv = gRPreAmp;
	} else {
		Rv = 10000;
	}
	ohm = (4.1 / volt) * Ri - Ri - Rv;
	return NTC_Interpol(ohm,10000);
}

static int8_t
cmd_ntc(Interp *interp,uint8_t argc,char *argv[])
{
	if((argc == 3) && (strcmp(argv[1],"ohm") == 0)) {
		float ohm = astrtof32(argv[2]);
		Con_Printf("Ohm %f, NTC %f\n",ohm,NTC_Interpol(ohm,10000));
        	return 0;
	} else if((argc == 3) && (strcmp(argv[1],"volt") == 0)) {
		float volt = astrtof32(argv[2]);
		float Ri = 1000.0;
		float Rv = 10000;
		float ohm = (4.1 / volt) * Ri - Ri - Rv;
		Con_Printf("Ohm %f, NTC %f\n",ohm,NTC_Interpol(ohm,10000));
        	return 0;
	} else if((argc == 2) && (strcmp(argv[1],"RV1") == 0)) {
		Con_Printf("RV1(Amp): %f Ohm\n",gRPreAmp - 5100);
		return 0;
	} else if((argc == 2) && (strcmp(argv[1],"RV2") == 0)) {
		Con_Printf("RV2(Amp): %f Ohm\n",gRPreMod - 5100);
		return 0;
	} else if((argc == 3) && (strcmp(argv[1],"RV1") == 0)) {
		gRPreAmp = astrtof32(argv[2]) + 5100;
		DB_VarWrite(DBKEY_NTC_AMP_RPRE,&gRPreAmp);
        	return 0;
	} else if((argc == 3) && (strcmp(argv[1],"RV2") == 0)) {
		gRPreMod = astrtof32(argv[2]) + 5100;
		DB_VarWrite(DBKEY_NTC_MOD_RPRE,&gRPreMod);
        	return 0;
	} else if(argc == 1) {
		Con_Printf("Mod %f C, Amp %f C\n",NTC_Read(ADCH_NTC_MOD),NTC_Read(ADCH_NTC_AMP));
        	return 0;
	}
        return -EC_BADARG;
}

INTERP_CMD(ntcCmd, "ntc", cmd_ntc,
           "ntc ?RV1 | RV2? ?<value/Ohm>? ");

static bool
PVNTC_GetTemperature (void *cbData, uint32_t adchannel, char *bufP,uint16_t maxlen)
{
        float temperature;
        temperature = NTC_Read(adchannel);
        bufP[f32toa(temperature,bufP,maxlen)] = 0;
        return true;
}

#if 0
static bool
PVNTC_GetRV (void *cbData, uint32_t rvX, char *bufP,uint16_t maxlen)
{
        float val;
	if(rvX == 1) {
		val = gRPreAmp;
	} else if(rvX == 2) {
		val = gRPreMod;
	}
        bufP[f32toa(temperature,bufP,maxlen)] = 0;
        return true;
}
#endif

void 
NTC_Init(void) 
{
	Interp_RegisterCmd(&ntcCmd);		
	PVar_New(PVNTC_GetTemperature,NULL,NULL,12,"mzMod.temp");
	PVar_New(PVNTC_GetTemperature,NULL,NULL,13,"amp.temp");
	DB_VarInit(DBKEY_NTC_MOD_RPRE,&gRPreMod);
	DB_VarInit(DBKEY_NTC_AMP_RPRE,&gRPreAmp);
}
