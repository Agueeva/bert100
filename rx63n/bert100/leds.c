/**
 *********************************************************************
 * This is the LED controling module.
 *********************************************************************
 */
#include "types.h"
#include "timer.h"
#include "shiftreg.h"
#include "hex.h"
#include "leds.h"
#include "interpreter.h"

typedef struct LedModule {
	Timer refreshTimer;
	uint16_t shiftreg;
} LedModule;

LedModule gLedModule;

static void 
LedsRefresh(void *eventData)
{
	LedModule *lm = eventData;	
	ShiftReg_Out(lm->shiftreg);
}

void
Led_Set(uint8_t led_nr,bool on) 
{
	LedModule *lm = &gLedModule;
	if(led_nr < 16) {
		if(on) {
			lm->shiftreg |= (UINT16_C(1) << led_nr);
		} else {
			lm->shiftreg &= ~(UINT16_C(1) << led_nr);
		}
		Timer_Start(&lm->refreshTimer,10);
		return;
	}
}

void 
Leds_Init() 
{
	LedModule *lm = &gLedModule;
	Timer_Init(&lm->refreshTimer,LedsRefresh,lm);
}
