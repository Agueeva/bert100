/**
 * Alarm module
 */
#include "alarms.h"
#include "types.h"
#include "buzzer.h"
#include "interpreter.h"
#include "console.h"
#include "hex.h"
#include "pvar.h"

typedef struct Buzzer {
	uint32_t alarmBits;
} Buzzer;

static Buzzer gBuzzer;

void 
Alarm_Set(uint8_t alarmNr)
{
	uint32_t bitmask = (UINT32_C(1) << alarmNr);
	if(!(gBuzzer.alarmBits & bitmask)) {
		gBuzzer.alarmBits |= bitmask; 
		Buzzer_Start(2100);
	}
}

void 
Alarm_Clear(uint8_t alarmNr)
{
	uint32_t bitmask = (UINT32_C(1) << alarmNr);
	if(gBuzzer.alarmBits & bitmask) {
		gBuzzer.alarmBits &= ~(UINT32_C(1) << alarmNr);
		if(gBuzzer.alarmBits == 0) {
			Buzzer_Start(0);
		}
	}
}

static bool
PVAlarms_Get (void *cbData, uint32_t adId, char *bufP,uint16_t maxlen)
{
	Buzzer *bz = cbData;
        SNPrintf(bufP,maxlen,"%lu",bz->alarmBits);
        return true;
}


static int8_t
cmd_alarm(Interp *interp,uint8_t argc,char *argv[])
{
	Buzzer *bz = &gBuzzer; 
	Con_Printf("Alarms 0x%08lx\n",bz->alarmBits);
	return 0;
}

INTERP_CMD(alarmCmd, "alarm", cmd_alarm,
           "alarm    # show alarm bitfield");

void
Alarm_Init(void) 
{
	Buzzer *bz = &gBuzzer;
	bz->alarmBits = 0;
	Interp_RegisterCmd(&alarmCmd);	
	PVar_New(PVAlarms_Get,NULL,bz,0,"system.fault");
}
