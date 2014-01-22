#include "types.h"

void Alarm_Set(uint8_t alarmNr);
void Alarm_Clear(uint8_t alarmNr);

#define ALARM_MOD_TEMP	(0)
#define ALARM_AMP_TEMP	(1)
#define ALARM_CPU_TEMP	(2)
#define ALARM_FAN	(3)

