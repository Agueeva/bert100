#include <stdint.h>
void Control_Init();
void ControlLoop(uint8_t channel,int8_t direction,uint16_t startval);
void ControlImon(uint8_t channel,uint16_t value);
