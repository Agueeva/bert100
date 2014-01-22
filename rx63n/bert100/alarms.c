/**
 * Alarm module
 */
#include "alarms.h"
#include "types.h"
#include "buzzer.h"

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
