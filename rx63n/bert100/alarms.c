/**
 * Alarm module
 */
#include "alarms.h"
#include "types.h"
#include "buzzer.h"
#include "interpreter.h"
#include "console.h"
#include "hex.h"

static uint32_t gAlarmBits = 0;

void 
Alarm_Set(uint8_t alarmNr)
{
	uint32_t bitmask = (UINT32_C(1) << alarmNr);
	if(!(gAlarmBits & bitmask)) {
		gAlarmBits |= bitmask; 
		Buzzer_Start(4000);
	}
}

void 
Alarm_Clear(uint8_t alarmNr)
{
	uint32_t bitmask = (UINT32_C(1) << alarmNr);
	if(gAlarmBits & bitmask) {
		gAlarmBits &= ~(UINT32_C(1) << alarmNr);
		if(gAlarmBits == 0) {
			Buzzer_Start(0);
		}
	}
}

static int8_t
cmd_alarm(Interp *interp,uint8_t argc,char *argv[])
{
	Con_Printf("Alarms 0x%08lx\n",gAlarmBits);
	return 0;
}

INTERP_CMD(alarmCmd, "alarm", cmd_alarm,
           "alarm    # show alarm bitfield");

void
Alarm_Init(void) 
{
	Interp_RegisterCmd(&alarmCmd);	
}
