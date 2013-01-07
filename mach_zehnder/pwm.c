/**
 ************************************************************************************
 * PWM control for Atmel XMega
 ( (C) 2010 2011 Jochen Karrer
 ************************************************************************************
 */
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <avr/io.h>
#include "hex.h"
#include "interpreter.h"
#include "pwm.h"
#include "eeprom.h"
#include "eeprom_map.h"

#define CHANNELS 4
static uint8_t ibtx_pwmch[CHANNELS] = {4,5,6,7};
static uint8_t imtx_pwmch[CHANNELS] = {0,1,2,3};
#define LPBIAS_CH	(1)

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

uint8_t
Ibtx_Set(uint8_t ibch,uint16_t pwmval)
{
	uint8_t pwmch;
	if(ibch < CHANNELS) {
		pwmch = ibtx_pwmch[ibch];
	} else {
		return 1;
	}
	PWM_Set(pwmch,pwmval);
	return 0;
}

uint8_t
Ibtx_Get(uint8_t ibch,uint16_t *pwmval)
{
	uint8_t pwmch;
	if(ibch < CHANNELS) {
		pwmch = ibtx_pwmch[ibch];
	} else {
		return 1;
	}
	return PWM_Get(pwmch,pwmval);
}

uint8_t
Imtx_Set(uint8_t imch,uint16_t pwmval)
{
	uint8_t pwmch;
	if(imch < CHANNELS) {
		pwmch = imtx_pwmch[imch];
	} else {
		return 1;
	}
	PWM_Set(pwmch,pwmval);
	return 0;
}

uint8_t
Imtx_Get(uint8_t imch,uint16_t *pwmval)
{
	uint8_t pwmch;
	if(imch < CHANNELS) {
		pwmch = imtx_pwmch[imch];
	} else {
		return 1;
	}
	return PWM_Get(pwmch,pwmval);
}

/**
 ******************************************************************************
 * \fn void PWM_Init(void); 
 ******************************************************************************
 */
void
PWM_Init(void) {
	uint16_t pwmres;
	uint8_t i;
	EEProm_Read(EEADDR(pwm_resolution),&pwmres,2);
	if(pwmres == 0xffff) {
		pwmres = 256;
	}
	PORTD.OUTCLR = 0x3f;
	PORTE.OUTCLR = 0xc;
	PORTD.DIRSET = 0x3f;
	PORTE.DIRSET = 0xc;
	for(i = 0;i < 4;i++) {
		uint16_t ibtx;
		EEProm_Read(EEADDR(ibtx_init[i]),&ibtx,2);
		if(ibtx == 0xffff) {
			ibtx = 0;
		}
		Ibtx_Set(i,ibtx);
	}
	HIRESD.CTRLA = HIRES_HREN_TC0_gc | HIRES_HREN_TC1_gc; 
	TCD0.CTRLA = 1; /* Divide by 1 */
	TCD1.CTRLA = 1; /* Divide by 1 */
        TCD0.PER = (pwmres - 1) & ~3;
        TCD1.PER = (pwmres - 1) & ~3;
	TCD0.CTRLB = TC_WGMODE_SS_gc | TC0_CCDEN_bm | TC0_CCCEN_bm | TC0_CCBEN_bm | TC0_CCAEN_bm; 
	TCD1.CTRLB = TC_WGMODE_SS_gc | TC0_CCBEN_bm | TC0_CCAEN_bm; 

	HIRESE.CTRLA = HIRES_HREN_TC0_gc | HIRES_HREN_TC1_gc; 
	TCE0.CTRLA = 1; /* Divide by 1 */
        //TCD0.INTCTRLA = 1;
        TCE0.PER = (pwmres - 1) & ~3;
	TCE0.CTRLB = TC_WGMODE_SS_gc | TC0_CCDEN_bm | TC0_CCCEN_bm; 
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
			uint16_t pwmval;
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
		}
        } else if(argc == 1) {
        	Interp_Printf_P(interp,"%d\n",(TCE0.PER | 3) + 1);
	} else {
		return -EC_BADNUMARGS;
	}
        return 0;
}

/**
 *************************************************************************************
 + \fn static int8_t cmd_ibtx(Interp * interp, uint8_t argc, char *argv[])
 * Shell command for setting the laser bias
 *************************************************************************************
 */
static int8_t 
cmd_ibtx(Interp * interp, uint8_t argc, char *argv[])
{
	uint8_t i;
	uint8_t pwmch;
	uint8_t ibch;
	uint16_t pwmval;
	if(argc == 1) {
		Interp_Printf_P(interp,"ibtx:");
		for(i = 0; i < CHANNELS; i++) {
			pwmch = ibtx_pwmch[i];
			PWM_Get(pwmch,&pwmval);
			Interp_Printf_P(interp," %u",pwmval);
		}
		Interp_Printf_P(interp,"\n");
	} else if(argc == 3) {
		ibch = astrtoi16(argv[1]) - 1;	
		if(ibch >= 4) {
			return -EC_BADARG;
		}
		if(strcmp(argv[2],"save") == 0) {
			if(Ibtx_Get(ibch,&pwmval) == 0) {
				EEProm_Write(EEADDR(ibtx_init[ibch]),&pwmval,2);
			} else {
				return -EC_ARGRANGE;
			}
		} else {
               		pwmval = astrtoi16(argv[2]);
			Ibtx_Set(ibch,pwmval);
		}
	} else if(argc == 4) {
		ibch = astrtoi16(argv[1]) - 1;	
		if(ibch >= 4) {
			return -EC_BADARG;
		}
		if(strcmp(argv[2],"max") == 0) {
			pwmval = astrtoi16(argv[3]);
			EEProm_Write(EEADDR(ibtx_max[ibch]),&pwmval,2);
		} else {
			return -EC_BADARG;
		}
	} else {
		return -EC_BADARG;
	}
	return -EC_OK;
}

/**
 *******************************************************************************************
 * \fn static int8_t cmd_imtx(Interp * interp, uint8_t argc, char *argv[])
 * Shell command for imtx control.
 *******************************************************************************************
 */
static int8_t 
cmd_imtx(Interp * interp, uint8_t argc, char *argv[])
{
	uint8_t i;
	uint8_t pwmch;
	uint8_t imch;
	uint16_t pwmval;
	if(argc == 1) {
		Interp_Printf_P(interp,"imtx:");
		for(i = 0; i < CHANNELS; i++) {
			pwmch = imtx_pwmch[i];
			PWM_Get(pwmch,&pwmval);
			Interp_Printf_P(interp," %u",pwmval);
		}
		Interp_Printf_P(interp,"\n");
	} else if(argc == 3) {
		imch = astrtoi16(argv[1]) - 1;	
		if(imch >= 4) {
			return -EC_BADARG;
		}
		if(strcmp(argv[2],"save") == 0) {
			if(Imtx_Get(imch,&pwmval) == 0) {
				EEProm_Write(EEADDR(imtx_init[imch]),&pwmval,2);
			} else {
				return -EC_ARGRANGE;
			}
		} else {
               		pwmval = astrtoi16(argv[2]);
			Imtx_Set(imch,pwmval);
		}
	} else if(argc == 4) {
		imch = astrtoi16(argv[1]) - 1;	
		if(imch >= 4) {
			return -EC_BADARG;
		}
		if(strcmp(argv[2],"max") == 0) {
			pwmval = astrtoi16(argv[3]);
			EEProm_Write(EEADDR(imtx_max[imch]),&pwmval,2);
		} else {
			return -EC_BADARG;
		}
	} else {
		return -EC_BADARG;
	}
	return -EC_OK;
}

INTERP_CMD(pwm, cmd_pwm, "pwm         <channel> ?<value> | save? # Set the pwm to value");
INTERP_CMD(pwmres, cmd_pwmres, "pwmres      <value>           # Number of PWM-Levels");
INTERP_CMD(ibtx, cmd_ibtx, "ibtx        ?<channel>? ?<value> | save? # Set/Get the pwm to value");
INTERP_CMD(imtx, cmd_imtx, "imtx        ?<channel>? ?<value> | save? # Set/Get the pwm to value");
