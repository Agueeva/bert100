#include "types.h"
#define SYNTH_0	(0)
uint32_t Synth_GetFreq(uint8_t synthID);
void Synth_Init(const char *name,uint16_t i2cAddr);
bool Synth_SetFreq(uint8_t synthId,uint32_t freq);
