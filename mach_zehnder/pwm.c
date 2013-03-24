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
#include "pwm.h"
#include "eeprom.h"
#include "eeprom_map.h"
#include "timer.h"
#include "adc.h"
#include "ad537x.h"
#include "console.h"

#define array_size(x) (sizeof(x) / sizeof((x)[0]))
static uint16_t gPwmRes;
static bool gSweepSingle;
static uint16_t gSweepCounter = 0;	
static int16_t curve[256];
static void adc_done(void *clientData, uint16_t adval);

static ADC_Request adcr = {
        .channel = 16,
        .callBack = adc_done,
        .clientData = NULL,
        .status = ADCR_IDLE
};


#define PORT_TRIGGER      PORTC
#define PIN_TRIGGER       (1)
#define CTRL_TRIGGER      PORTC_PIN1CTRL 

/**
 ********************************************
 * Invert all PWM signals
 ********************************************
 */
static void
pwm_invert(void) 
{
	uint8_t i;
	volatile uint8_t *pinctrl;
	for(i = 0;i < 10; i++ ) {
		if(i < 6) {
			pinctrl = &PORTD.PIN0CTRL + i;
		} else {
			pinctrl = &PORTE.PIN0CTRL + i - 6;
		}
		*pinctrl ^= PORT_INVEN_bm;
	}
}

/**
 *************************************************************
 * Read back the value of a PWM channel
 *************************************************************
 */
static uint8_t 
PWM_Get(uint8_t channel,uint16_t *pwmval)
{
	if(channel >= 8) {
		return 1;
	}
	switch(channel) {
		case 0:
			*pwmval = TCD0.CCA;
			break;
		case 1:
			*pwmval = TCD0.CCB;
			break;
		case 2:
			*pwmval = TCD0.CCC;
			break;
		case 3:
			*pwmval = TCD0.CCD;
			break;
		case 4:
			*pwmval = TCD1.CCA;
			break;
		case 5:
			*pwmval = TCD1.CCB;
			break;
		case 6:
			*pwmval = TCE0.CCC;
			break;
		case 7:
			*pwmval = TCE0.CCD;
			break;
		default:
			*pwmval = 0;
			return 1;
	}
	return 0;
}

/**
 ***********************************************************
 * uint8_t PWM_Set(uint8_t channel,uint16_t pwmval) 
 * Set the pulse width of a pwm channel.
 ***********************************************************
 */
uint8_t
PWM_Set(uint8_t channel,uint16_t pwmval) 
{
	switch(channel) {
		case 0:
			TCD0.CCA = pwmval;
			break;
		case 1:
			TCD0.CCB = pwmval;
			break;
		case 2:
			TCD0.CCC = pwmval;
			break;
		case 3:
			TCD0.CCD = pwmval;
			break;
		case 4:
			TCD1.CCA = pwmval;
			break;
		case 5:
			TCD1.CCB = pwmval;
			break;
		case 6:
			TCE0.CCC = pwmval;
			break;
		case 7:
			TCE0.CCD = pwmval;
			break;
		default:
			return 1;
	}
	return 0;
}

static void sawToothProc(void *eventData);

TIMER_DECLARE(sawToothTimer,sawToothProc,NULL)

static void 
adc_done(void *clientData, uint16_t adval) 
{
	if(gSweepCounter < array_size(curve)) {
		curve[gSweepCounter] = adval;		
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
	//PWM_Set(4,gSweepCounter);
	DAC_Set(0,gSweepCounter << 8);
	gSweepCounter = (gSweepCounter + 1) % gPwmRes;
	if(gSweepCounter == 0)  {
		if(gSweepSingle == false) {
			Timer_Start(&sawToothTimer,4);
		} else {
			PWM_Set(4,0); /* Park with zero output */
			Con_Printf_P("\n");
			for(i = 0; i < array_size(curve);i++) {
				Con_Printf_P("%u, %d\n",i,curve[i]);
			}		
		}
	} else {
		Timer_Start(&sawToothTimer,4);
		ADC_EnqueueRequest(&adcr,16);
	}
}

/**
 ******************************************************************************
 * \fn void PWM_Init(void); 
 ******************************************************************************
 */
void
PWM_Init(void) {
	uint16_t pwmres;
	EEProm_Read(EEADDR(pwm_resolution),&pwmres,2);
	if(pwmres == 0xffff) {
		gPwmRes = pwmres = 256;
	}
	PORTD.OUTCLR = 0x3f;
	PORTE.OUTCLR = 0xc;
	PORTD.DIRSET = 0x3f;
	PORTE.DIRSET = 0xc;

	HIRESD_CTRL = HIRES_HREN_TC0_gc | HIRES_HREN_TC1_gc; 
	TCD0.CTRLA = 1; /* Divide by 1 */
	TCD1.CTRLA = 1; /* Divide by 1 */
        TCD0.PER = (pwmres - 1) & ~3;
        TCD1.PER = (pwmres - 1) & ~3;
	TCD0.CTRLB = TC_WGMODE_SS_gc | TC0_CCDEN_bm | TC0_CCCEN_bm | TC0_CCBEN_bm | TC0_CCAEN_bm; 
	TCD1.CTRLB = TC_WGMODE_SS_gc | TC0_CCBEN_bm | TC0_CCAEN_bm; 

	HIRESE_CTRL = HIRES_HREN_TC0_gc | HIRES_HREN_TC1_gc; 
	TCE0.CTRLA = 1; /* Divide by 1 */
        //TCD0.INTCTRLA = 1;
        TCE0.PER = (pwmres - 1) & ~3;
	TCE0.CTRLB = TC_WGMODE_SS_gc | TC0_CCDEN_bm | TC0_CCCEN_bm; 

	CTRL_TRIGGER = PORT_OPC_TOTEM_gc;
        PORT_TRIGGER.DIRSET = (1 << PIN_TRIGGER);
	//Timer_Start(&sawToothTimer,4);
}


/**
 *******************************************************************************
 * \fn static int8_t cmd_pwm(Interp * interp, uint8_t argc, char *argv[])
 *******************************************************************************
 */
static int8_t 
cmd_pwm(Interp * interp, uint8_t argc, char *argv[])
{
        uint16_t pwmval;
	uint8_t channel;
        if(argc == 3) {
		channel = astrtoi16(argv[1]);	
		pwmval = astrtoi16(argv[2]);
		PWM_Set(channel,pwmval);
	} else if(argc == 2) {
		if(strcmp(argv[1],"invert") == 0) {
			pwm_invert();
			return 0;
		} else {
			return -EC_BADNUMARGS;
		}
        } else if(argc == 1) {
		for(channel = 0; channel < 7;channel++) {
			PWM_Get(channel,&pwmval);
			Interp_Printf_P(interp,"%d ",pwmval);
		}
		Interp_Printf_P(interp,"\n");
        } else {
		return -EC_BADNUMARGS;
	}
        return 0;
}

/**
 *******************************************************************************
 * \fn static int8_t cmd_pwm(Interp * interp, uint8_t argc, char *argv[])
 *******************************************************************************
 */
static int8_t 
cmd_sweep(Interp * interp, uint8_t argc, char *argv[])
{
	if(argc == 1) {
		gSweepSingle = true;
		gSweepCounter = 0;
		PWM_Set(4,gSweepCounter);
		ADC_EnqueueRequest(&adcr,16);
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
		}
		return -EC_BADARG;
        } else {
		return -EC_BADNUMARGS;
	}
}

/**
 *************************************************************************************
 * \fn static int8_t cmd_pwmres(Interp * interp, uint8_t argc, char *argv[])
 * Configure the number of time slots of the PWM.
 *************************************************************************************
 */

static int8_t 
cmd_pwmres(Interp * interp, uint8_t argc, char *argv[])
{
        if(argc == 2) {
		uint16_t cnt_max;
		uint16_t pwmres;
		if(strcmp(argv[1],"save") == 0) {
			pwmres = (TCE0.PER | 3) + 1;
			EEProm_Write(EEADDR(pwm_resolution),&pwmres,2);
		} else {
			cnt_max = (astrtoi16(argv[1]) - 1) & ~3; 
			TCD0.PER = cnt_max;
			TCD1.PER = cnt_max;
			TCE0.PER = cnt_max;
			gPwmRes = (TCE0.PER | 3) + 1;
		}
        } else if(argc == 1) {
        	Interp_Printf_P(interp,"%d\n",(TCE0.PER | 3) + 1);
	} else {
		return -EC_BADNUMARGS;
	}
        return 0;
}


INTERP_CMD(pwm, cmd_pwm, "pwm         <channel> ?<value> | save? # Set the pwm to value");
INTERP_CMD(pwmres, cmd_pwmres, "pwmres      <value>           # Number of PWM-Levels");
INTERP_CMD(sweep, cmd_sweep, "sweep      start | stop           # start/stop sweep");
