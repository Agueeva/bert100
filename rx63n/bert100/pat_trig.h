#include "types.h"
void PatTrig_Init(const char *name);

typedef enum TrigPattern {
	patTrigOff = 0,
	patTrigPRBS7 = 1,
	patTrigPRBS9 = 2,
	patTrigPRBS15 = 3,
	patTrigPRBS23 = 4,
	patTrigPRBS31 = 5,
} TrigPattern;

void PatTrig_SelPat(uint8_t modId,TrigPattern pat);
