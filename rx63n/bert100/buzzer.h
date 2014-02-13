#include "types.h"

#define BUZZER_MELODY_OK	0
#define BUZZER_MELODY_FAILED	1
#define BUZZER_MELODY_CLICK	2

void Buzzer_Init(void);
void Buzzer_SetAlarm(bool on);
bool Buzzer_SelectMelody(uint16_t nr);
