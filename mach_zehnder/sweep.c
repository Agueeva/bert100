/**
 ************************************************************************************
 * PWM control for Atmel XMega
 ( (C) 2010 2011 Jochen Karrer
 ************************************************************************************
 */
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <avr/io.h>
#include "hex.h"
#include "interpreter.h"
#include "eeprom.h"
#include "eeprom_map.h"
#include "timer.h"
#include "adc.h"
#include "ad537x.h"
#include "console.h"
#include "control.h"

#define PORT_TRIGGER      PORTC
#define PIN_TRIGGER       (1)
#define CTRL_TRIGGER      PORTC_PIN1CTRL 

#define array_size(x) (sizeof(x) / sizeof((x)[0]))
static bool gSweepSingle;
static bool gSweepClose;
static uint16_t gPwmRes;
static uint16_t gSweepCounter = 0;	
static int16_t curve[256];
static void adc_done(void *clientData, uint16_t adval);

static ADC_Request adcr = {
        .channel = 16,
        .callBack = adc_done,
        .clientData = NULL,
        .status = ADCR_IDLE
};

static void sawToothProc(void *eventData);

TIMER_DECLARE(sawToothTimer,sawToothProc,NULL)

static void 
adc_done(void *clientData, uint16_t adval) 
{
	if(gSweepCounter < array_size(curve)) {
		curve[gSweepCounter] = adval;		
	}
}

void find_params(void) {
	int16_t i;
        int16_t maxx,maxy;
        int16_t minx,miny;
        int16_t midy;
        int16_t x,y;
        int8_t direction = 0;
	uint16_t bestdacval = 0;
	int8_t bestdirection = 0;
	
        maxx = 0; maxy = 0;
        minx = 32767; miny = 32767;
        for(i = 1; i < array_size(curve);i++) {
                x = i; 
                y = curve[i];
                if(y > maxy) {
                        maxy = y;
                        maxx = x;
                }
                if(y < miny) {
                        miny = y;
                        minx = x;
                }
        }
        midy = (miny + maxy) / 2;
        for(i = 2; i < array_size(curve); i++) {
                x = i; 
                y = curve[i];
                if(direction == 0) {
                        if(y > midy) {
                                direction = -1;
                        } else {
                                direction = 1;
                        }
                }
                if(direction == 1) {
                        if(y > midy) {
                                Con_Printf_P("closed_loop 1 1 %u\n",i << 8);
				if(abs((i << 8) - 32767) < abs(bestdacval - 32767)) {
					bestdacval = i << 8;
					bestdirection = 1;
				} 
                                direction = 0;
                                i += 5;
                        }
                } else {
                        if(y < midy) {
                                Con_Printf_P("closed_loop 1 -1 %u\n",i << 8);
				if(abs((i << 8) - 32767) < abs(bestdacval - 32767)) {
					bestdacval = i << 8;
					bestdirection = -1;
				} 
                                direction = 0;
                                i += 5;
                        }
                }
        }
	Con_Printf_P("set_imon 1 %u\n",midy);
	Con_Printf_P("Best is closed_loop 1 %d %u\n",bestdirection,bestdacval);
	if(gSweepClose) {
		ControlImon(0,midy);
		ControlLoop(0,bestdirection,bestdacval);
		Con_Printf_P("Locked loop on imon %u dac %u\n",midy,bestdacval);
	}
}

static void
sawToothProc(void *eventData)
{
	uint16_t i;
	if(gSweepCounter == 0) {
		PORT_TRIGGER.OUTCLR = (1 <<  PIN_TRIGGER);
	} else {
		PORT_TRIGGER.OUTSET = (1 <<  PIN_TRIGGER);
	}
	DAC_Set(0,gSweepCounter << 8);
	gSweepCounter = (gSweepCounter + 1) % array_size(curve);
	if(gSweepCounter == 0)  {
		if(gSweepSingle == false) {
			Timer_Start(&sawToothTimer,4);
		} else {
			Con_Printf_P("\n");
			for(i = 0; i < array_size(curve);i++) {
				Con_Printf_P("%u, %d\n",i,curve[i]);
			}		
			find_params();
		}
	} else {
		Timer_Start(&sawToothTimer,4);
		ADC_EnqueueRequest(&adcr,16);
	}
}

/**
 *******************************************************************************
 * \fn static int8_t cmd_pwm(Interp * interp, uint8_t argc, char *argv[])
 *******************************************************************************
 */
static int8_t 
cmd_sweep(Interp * interp, uint8_t argc, char *argv[])
{
	gSweepClose = false;
	if(argc == 1) {
		gSweepSingle = true;
		gSweepCounter = 0;
		//PWM_Set(4,gSweepCounter);
		//ADC_EnqueueRequest(&adcr,16);
		DAC_Set(0,gSweepCounter << 8);
		Timer_Start(&sawToothTimer,4);
		return 0;
	} else if(argc == 2) {
		if(strcmp(argv[1],"start") == 0) {
			gSweepSingle = false;
			Timer_Start(&sawToothTimer,4);
			return 0;
		} else if(strcmp(argv[1],"stop") == 0) {
			Timer_Cancel(&sawToothTimer);
			return 0;
		} else if(strcmp(argv[1],"lock") == 0) {
			ControlLoop(0,0,0);
			gSweepSingle = true;
			gSweepCounter = 0;
			DAC_Set(0,gSweepCounter << 8);
			Timer_Start(&sawToothTimer,4);
			gSweepClose = true;
			return 0;
		}
		return -EC_BADARG;
        } else {
		return -EC_BADNUMARGS;
	}
}

INTERP_CMD(sweep, cmd_sweep, "sweep      start | stop | lock          # start/stop/lock sweep");

void
Sweep_Init() {
        CTRL_TRIGGER = PORT_OPC_TOTEM_gc;
        PORT_TRIGGER.DIRSET = (1 << PIN_TRIGGER);
}

