#include "types.h"
#include "database.h"
#define DBKEY_PWRREF(channel)   (DBKEY_ADC((channel) * 0x100))
void ADC12_Init(void);
int16_t ADC12_Read(int channel);
float ADC12_ReadVolt(int channel);
