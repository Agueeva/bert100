#include "types.h"
#include "database.h"
#define DBKEY_PWRREF(channel)   (DBKEY_ADC((channel) * 0x100))
#define DBKEY_TEMPSENS_CORR(channel)   (DBKEY_ADC(1))

#define ADCH_TX_PWR(lane)	(0 + (lane))

void ADC12_Init(void);
int16_t ADC12_Read(int channel);
float ADC12_ReadVolt(int channel);
float ADC12_ReadDB(int channel);

float ADC12_ReadVoltMultiple(int channel,uint8_t repeatCnt);
