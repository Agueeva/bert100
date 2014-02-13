/**
 * Alarm module
 */
#include <string.h>
#include "alarms.h"
#include "types.h"
#include "buzzer.h"
#include "interpreter.h"
#include "console.h"
#include "hex.h"
#include "pvar.h"
#include "hex.h"

typedef struct Alarms {
	uint32_t alarmBits;
	uint32_t latchedAlarmBits;
} Alarms;

static Alarms gAlarms;

/**
 ***********************************************************
 * \fn void Alarm_Set(uint8_t alarmNr)
 * Set an Alarm Bit in the alarmBits and latchedAlarmBits
 * The Buzzer is enabled on alarm
 ***********************************************************
 */
void 
Alarm_Set(uint8_t alarmNr)
{
	uint32_t bitmask = (UINT32_C(1) << alarmNr);
	if(!(gAlarms.alarmBits & bitmask)) {
		gAlarms.alarmBits |= bitmask; 
		gAlarms.latchedAlarmBits |= bitmask; 
		Buzzer_SetAlarm(1);
	}
}

/**
 **********************************************************************
 * \fn void Alarm_Clear(uint8_t alarmNr)
 * Clear an Alarm Bit in the alarmBits buzt not in latchedAlarmBits
 * The Buzzer is disabled if no bit is set any more. 
 ***********************************************************************
 */
void 
Alarm_Clear(uint8_t alarmNr)
{
	uint32_t bitmask = (UINT32_C(1) << alarmNr);
	if(gAlarms.alarmBits & bitmask) {
		gAlarms.alarmBits &= ~(UINT32_C(1) << alarmNr);
		if(gAlarms.alarmBits == 0) {
			Buzzer_SetAlarm(0);
		}
	}
}

static bool
PVAlarms_Get (void *cbData, uint32_t adId, char *bufP,uint16_t maxlen)
{
	Alarms *al = cbData;
        SNPrintf(bufP,maxlen,"%lu",al->alarmBits);
        return true;
}

static bool
PVLatchedAlarms_Get (void *cbData, uint32_t adId, char *bufP,uint16_t maxlen)
{
	Alarms *al = cbData;
        SNPrintf(bufP,maxlen,"%lu",al->latchedAlarmBits);
        return true;
}

static bool
PVLatchedAlarms_Set(void *cbData, uint32_t adId, const char *strP)
{
	Alarms *al = cbData;
	uint32_t value = astrtoi32(strP);
	al->latchedAlarmBits = value; 
	al->latchedAlarmBits |= al->alarmBits;
        return true;
}

/**
 **********************************************************************************************
 * \fn static int8_t cmd_alarm(Interp *interp,uint8_t argc,char *argv[])
 *
 * Command shell interface to the alarm module. Shows latched alarms and allows 
 * to clear latched alarm.
 **********************************************************************************************
 */
static int8_t
cmd_alarm(Interp *interp,uint8_t argc,char *argv[])
{
	Alarms *al = &gAlarms; 
	Con_Printf("Alarms         0x%08lx\n",al->alarmBits);
	Con_Printf("Latched Alarms 0x%08lx\n",al->latchedAlarmBits);
	if((argc > 1) && (strcmp(argv[2],"clear") == 0)) {
		al->latchedAlarmBits = 0;
	}
	return 0;
}

INTERP_CMD(alarmCmd, "alarm", cmd_alarm,
           "alarm ?clear?   # show alarm bitfield / clear latched alarms");

/**
 *******************************************************************************
 * \fn void Alarm_Init(void) 
 * Initialize the Alarm module
 *******************************************************************************
 */

void
Alarm_Init(void) 
{
	Alarms *al = &gAlarms;
	al->alarmBits = 0;
	Interp_RegisterCmd(&alarmCmd);	
	PVar_New(PVAlarms_Get,NULL,al,0,"system.fault");
	PVar_New(PVLatchedAlarms_Get,PVLatchedAlarms_Set,al,0,"system.latchedFault");
}
