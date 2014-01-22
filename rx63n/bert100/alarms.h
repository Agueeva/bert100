#include "types.h"

void Alarm_Set(uint8_t alarmNr);
void Alarm_Clear(uint8_t alarmNr);

#define ALARM_MOD_TEMP	(0)
#define ALARM_AMP_TEMP	(1)
#define ALARM_CPU_TEMP	(2)
#define ALARM_FAN_0	(3)
#define ALARM_FAN_1	(4)
#define ALARM_FAN_2	(5)
#define ALARM_FAN_3	(6)

