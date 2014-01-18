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

static float gRPreMod = 10000.0;
static float gRPreAmp = 10000.0;

typedef struct Point {
        float temp;
        float R;
} Point;

const Point points[33] = {
        {150.0, 301.66},
        {145.0, 334.51},
        {140.0, 371.79},
        {135.0, 414.2},
        {130.0, 462.59},
        {125.0, 517.94},
        {120.0, 581.44},
        {115.0, 654.5},
        {110.0, 738.81},
        {105.0, 836.4},
        {100.0, 949.73},
        {95.0, 1081.8},
        {90.0, 1236.2},
        {85.0, 1417.4},
        {80.0, 1630.7},
        {75.0, 1883.0},
        {70.0, 2182.4},
        {65.0, 2539.2},
        {60.0, 2966.3},
        {55.0, 3479.8},
        {50.0, 4100.0},
        {45.0, 4852.5},
        {40.0, 5770.3},
        {35.0, 6895},
        {30.0, 8218},
        {25.0, 10000},
        {20.0, 12142},
        {15.0, 14827},
        {10.0, 18216},
        {5.0, 22520},
        {0.0, 28024},
        {-50.0, 38452},
        {-100.0, 55297},
};

/**
 ***************************************************************************
 * Calculate the Temperature using a Table and linear
 * interpolation.
 ***************************************************************************
 */
static float 
NTC_Interpol_10k(float ohm)
{
        const Point *p, *prev;
        float diff_ohm;
        float diff_temp;
        float temp;
        int i;
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
	Rv = 10000.0; 
	ohm = (3.3 / volt) * Ri - Ri - Rv;
	return NTC_Interpol_10k(ohm);
}

static int8_t
cmd_ntc(Interp *interp,uint8_t argc,char *argv[])
{
	if((argc == 3) && (strcmp(argv[1],"ohm") == 0)) {
		float ohm = astrtof32(argv[2]);
		Con_Printf("Ohm %f, NTC %f\n",ohm,NTC_Interpol_10k(ohm));
	} else if((argc == 3) && (strcmp(argv[1],"volt") == 0)) {
		float volt = astrtof32(argv[2]);
		float Ri = 1000.0;
		float Rv = 10000;
		float ohm = (3.3 / volt) * Ri - Ri - Rv;
		Con_Printf("Ohm %f, NTC %f\n",ohm,NTC_Interpol_10k(ohm));
	} else {
		Con_Printf("Mod %f C, Amp %f C\n",NTC_Read(12),NTC_Read(13));
	}
        return 0;
}

INTERP_CMD(ntcCmd, "ntc", cmd_ntc,
           "ntc ohm <value> ");

static bool
PVNTC_GetTemperature (void *cbData, uint32_t adchannel, char *bufP,uint16_t maxlen)
{
        float temperature;
        temperature = NTC_Read(adchannel);
        bufP[f32toa(temperature,bufP,maxlen)] = 0;
        return true;
}

void 
NTC_Init(void) 
{
	Interp_RegisterCmd(&ntcCmd);		
	PVar_New(PVNTC_GetTemperature,NULL,NULL,12,"mzMod.temp");
	PVar_New(PVNTC_GetTemperature,NULL,NULL,13,"amp.temp");
	DB_VarInit(DBKEY_NTC_MOD_RPRE,&gRPreMod);
	DB_VarInit(DBKEY_NTC_AMP_RPRE,&gRPreAmp);
}
