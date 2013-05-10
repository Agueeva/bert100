#include "adc.h"

#define ADC_MON2G5_CH4 (16)
#define ADC_MON2G5_CH3 (17)
#define ADC_MON2G5_CH2 (18)
#define ADC_MON2G5_CH1 (19)

void TX_Init(void);

uint16_t TX_GetImon(uint8_t channel);

static inline int 
TX_ImonGetAdChannel(uint8_t channel) 
{
	return ADC_MON2G5_CH1 - channel;
}
