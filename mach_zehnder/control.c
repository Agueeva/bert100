/**
 ****************************************************************************************
 *
 ****************************************************************************************
 */
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "events.h"
#include "timer.h"
#include "console.h"
#include "interpreter.h"
#include "hex.h"
#include "eeprom.h"
#include "eeprom_map.h"
#include "adc.h"
#include "pwm.h"
#include "tx.h"

#define NR_CHANNELS	(4)

#define NORM_IBTX(x)	((x) << 6) 
#define UNNORM_IBTX(x)	((x) >> 6)

typedef struct Controller {
	uint8_t curr_channel;
	uint8_t control_enable[NR_CHANNELS];
	uint16_t ibias_outval[NR_CHANNELS]; /* Normalized to 0 - 16384 */
	uint16_t imon_setpoint[NR_CHANNELS];
	ADC_Request adcr;
} Controller;

static Controller g_Controller;

void Control_TimerProc(void *eventData);
static TIMER_DECLARE(g_ControlTimer,Control_TimerProc,&g_Controller);

void 
Control_TimerProc(void *eventData)
{
	Controller *contr = (Controller *) eventData;	
	ADC_EnqueueRequest(&contr->adcr,TX_ImonGetAdChannel(contr->curr_channel));
	Timer_Start(&g_ControlTimer, 1);
}

/**
 *****************************************************************************************
 * \fn void Control_AdcDone(void *eventData,uint16_t adval)
 *****************************************************************************************
 */
void 
Control_AdcDone(void *eventData,uint16_t adval)
{
	Controller *contr = (Controller *) eventData;	
	uint16_t millivolt;
	uint16_t sp;
	uint8_t ch = contr->curr_channel;
	int16_t diff;
	int new_ibias;
	int K = 32;
	if(adval & 0x8000) {
		millivolt = 0;
	} else {
		millivolt = (((adval * 25) >> 5) * 25) >> 4;
	}
	sp = contr->imon_setpoint[ch];
	diff = (sp - millivolt) * 6; /* Normalized to 15000 */
	new_ibias = contr->ibias_outval[ch] + (diff / K);
	if(new_ibias < 0) {
		new_ibias = 0;
	} else if(new_ibias > 16384) {
		new_ibias = 16384;
	}
	if(ch == 0) {
	//	Con_Printf_P("sp %d, mv %u diff %d, new ibias %u\n",sp,millivolt,diff,new_ibias);
	}
	contr->ibias_outval[ch] = new_ibias;
	if(contr->control_enable[ch]) {
		Ibtx_Set(ch,contr->ibias_outval[ch] / 64);
	}
	contr->curr_channel = (ch + 1) % NR_CHANNELS;
	if(contr->curr_channel != 0) {
		ADC_EnqueueRequest(&contr->adcr,TX_ImonGetAdChannel(contr->curr_channel));
	}
}

/**
 ****************************************************************************
 * \fn void Control_Init() 
 ****************************************************************************
 */
void
Control_Init() 
{
	Controller *contr = &g_Controller;
	ADC_RequestInit(&contr->adcr,Control_AdcDone,contr);
	Timer_Start(&g_ControlTimer, 10);
}

/**
 *************************************************************************************
 * \fn static int8_t cmd_set_imon(Interp * interp, uint8_t argc, char *argv[])
 * Configure the setpoint for the control loop. 
 *************************************************************************************
 */
static int8_t 
cmd_set_imon(Interp * interp, uint8_t argc, char *argv[])
{
	Controller *contr = &g_Controller;
	uint8_t ch;
	uint16_t value;
	if(argc > 1) {
		ch = astrtoi16(argv[1]) - 1;
		if(ch > 3) {
			return -EC_ARGRANGE;
		}
	} else {
		return -EC_BADNUMARGS;
	}
	if(argc > 2) {
		value = astrtoi16(argv[2]);
		contr->imon_setpoint[ch] = value; 
	} else {
		value = contr->imon_setpoint[ch];
		Interp_Printf_P(interp,"I-Mon Setpoint %d: %d\n",ch + 1,value);
	}
	return 0;
}

INTERP_CMD(set_imon, cmd_set_imon,
           "set_imon    <channel> < 0 | 1 > #  Configure setpoint for monitor current");

static int8_t 
cmd_closed_loop(Interp * interp, uint8_t argc, char *argv[])
{
	Controller *contr = &g_Controller;
	uint8_t ch;
	uint8_t value;
	if(argc > 1) {
		ch = astrtoi16(argv[1]) - 1;
		if(ch > 3) {
			return -EC_ARGRANGE;
		}
	} else {
		return -EC_BADNUMARGS;
	}
	if(argc > 2) {
		uint16_t pwmval;
		value = astrtoi16(argv[2]);
		contr->control_enable[ch] = value;
		value = astrtoi16(argv[2]);
		Ibtx_Get(ch,&pwmval);
		contr->ibias_outval[ch] = NORM_IBTX(pwmval); 
	} else {
		value = !!contr->control_enable[ch];
		Interp_Printf_P(interp,"Control Loop %d: %d\n",ch + 1,value);
	}
        return 0;
}


INTERP_CMD(closed_loop, cmd_closed_loop,
           "closed_loop <channel> < 0 | 1 > #  Close / open control loop");

