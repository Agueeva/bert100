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
#include "ad537x.h"

#define NR_CHANNELS	(1)

#define NORM_IBTX(x)	((x) << 6) 
#define UNNORM_IBTX(x)	((x) >> 6)

typedef struct Controller {
	uint8_t curr_channel;
	int8_t control_enable[NR_CHANNELS];
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
	ADC_EnqueueRequest(&contr->adcr,16);
	Timer_Start(&g_ControlTimer, 10);
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
	uint16_t sp;
	uint8_t ch = contr->curr_channel;
	int16_t diff;
	int32_t new_ibias;
	int K = 1;
	sp = contr->imon_setpoint[ch];
	diff = (sp - adval); 
	if(contr->control_enable[ch] < 0) {
		new_ibias = (int32_t)contr->ibias_outval[ch] - (diff / K);
	} else {
		new_ibias = (int32_t)contr->ibias_outval[ch] + (diff / K);
	}
	if(new_ibias < 0) {
		new_ibias = 0;
	} else if(new_ibias > UINT32_C(65535)) {
		new_ibias = 65535;
	}
#if 0
	if(ch == 0) {
		Con_Printf_P("sp %d, diff %d, new ibias %u\n",sp,diff,new_ibias);
	}
#endif
	contr->ibias_outval[ch] = new_ibias;
	if(contr->control_enable[ch]) {
		DAC_Set(0,contr->ibias_outval[ch]);
	}
	contr->curr_channel = (ch + 1) % NR_CHANNELS;
	if(contr->curr_channel != 0) {
		ADC_EnqueueRequest(&contr->adcr,16);
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
		if(ch >= NR_CHANNELS) {
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
           "set_imon    <channel> < 0 | 1 > ?<setpoint>? #  Configure setpoint for monitor current");

static int8_t 
cmd_closed_loop(Interp * interp, uint8_t argc, char *argv[])
{
	Controller *contr = &g_Controller;
	uint8_t ch;
	uint8_t value;
	if(argc > 1) {
		ch = astrtoi16(argv[1]) - 1;
		if(ch >= NR_CHANNELS) {
			return -EC_ARGRANGE;
		}
	} else {
		return -EC_BADNUMARGS;
	}
	if(argc > 2) {
		//uint16_t startval;
		value = astrtoi16(argv[2]);
		contr->control_enable[ch] = value;
		value = astrtoi16(argv[2]);
		if(argc > 3) {
			contr->ibias_outval[ch] = astrtoi16(argv[3]);
		}
		//Ibtx_Get(ch,&pwmval);
		//contr->ibias_outval[ch] = NORM_IBTX(pwmval); 
	} else {
		value = !!contr->control_enable[ch];
		Interp_Printf_P(interp,"Control Loop %d: %d, outval %u\n",ch + 1,value,contr->ibias_outval[ch]);
	}
        return 0;
}


INTERP_CMD(closed_loop, cmd_closed_loop,
           "closed_loop <channel> < 0 | 1 > <DAC startval> #  Close / open control loop");

