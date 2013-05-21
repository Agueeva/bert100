/*
 *************************************************************
 * (C) 2009 2010 Jochen Karrer
 *//**
 * \file adc.c
 * A/D converter access routines for ATxmega 
 * All Functions should only be used from eventhandler and
 * not from interrupts because interrupt are not disabled 
 * during modification of the linked lists.
 *************************************************************
 */
#include <stdlib.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "irqflags.h"
#include "console.h"
#include "events.h"
#include "interpreter.h"
#include "hex.h"
#include "adc.h"
#include "calib.h"


uint8_t KALBSP_ReadCalibrationByte(uint8_t index)
{
	uint8_t result;
	__asm__ __volatile__ (
		"ldi r20, %3    ; Load command into temp register." "\n\t"
		"mov r30, %2   ; Move index to Z pointer."         "\n\t"
		"clr r31        ; Clear ZH."                        "\n\t"
		"sts %1, r20    ; Store command to CMD register."   "\n\t"
		"lpm %0, Z      ; Load Program memory to result"    "\n\t"
		"ldi r20, %4    ; Clean up CMD register."           "\n\t"
		"sts %1, r20"
		: "=r" (result)
		: "m" (NVM_CMD),
		"r" (index),
		"M" (NVM_CMD_READ_CALIB_ROW_gc),
		"M" (NVM_CMD_NO_OPERATION_gc)
		: "r20", "r30", "r31"
	);
	return result;
} 

/**
 **********************************************************************
 * \typedef struct ADConv
 * This type represents the ATxmega A/D converter.
 **********************************************************************
 */
typedef struct ADConv {
	ADC_Request *queueHead; /**< Begin of the request queue */
	ADC_Request *queueTail; /**< End of the request queue */
} ADConv;

/**
 ********************************************
 * \var ADConv g_adc;
 * The global instance of the A/D Converter.
 ********************************************
 */
ADConv g_adc;

/**
 ************************************************************
 * \fn static inline void adc_trigger_conversion(uint8_t channel);
 *	Trigger an A/D conversion.
 ************************************************************
 */
static inline void adc_trigger_conversion(uint8_t channel)
{
	uint8_t inputmode;
	ADCA.CH0.MUXCTRL = ((channel & 0xf) << ADC_CH_MUXPOS_gp);
	inputmode = channel >> 4;
	ADCA.CH0.CTRL = inputmode | ADC_CH_START_bm;
}

/**
 **********************************************************************************
 * \fn static void adc_intevent(void *clientData);
 * 	Eventhandler triggered by the Interrupt handler whenever a 
 * 	conversion is complete. It removes the request from the queue, 
 * 	reads the A/D converter result and invokes the callback.
 *	The callback is allowed to modify the request queue by
 *	reinserting its own request into the A/D request queue.
 **********************************************************************************
 */
static void adc_intevent(void *clientData)
{
	ADConv *adc = (ADConv *) clientData;
	ADC_Request *req;
	ADC_Request *next;
	uint16_t adval;

	adval = ADCA.CH0.RES;
	req = adc->queueHead;
	if (!req) {
		return;
	}
	next = adc->queueHead = req->next;
	req->next = NULL;
	if (next) {
		adc_trigger_conversion(next->channel);
		next->status = ADCR_CONVERTING;
	} else {
		adc->queueTail = NULL;
	}
	req->status = ADCR_IDLE;
	/* 
	 ********************************************************************
	 * The callback must be invoked last because it might re-enqueue
	 *  the request
	 ********************************************************************
	 */
	if (req->callBack) {
		req->callBack(req->clientData, adval);
	}
}

static EVENT_DECLARE(e_adc_interrupt,adc_intevent,&g_adc);

/**
 ***********************************************************************
 * \fn ISR(ADC_vect)
 * Interrupt handler of the A/D converter.
 ***********************************************************************
 */
ISR(ADCA_CH0_vect)
{
	EV_Trigger(&e_adc_interrupt);
}

/**
 *********************************************************************************
 * \fn int8_t ADC_EnqueueRequest(ADC_Request * req);
 * 	Insert a request into the linked list of outstanding conversion requests.
 * 	If it is the only request in the list then the conversion is started.
 *      Do not call this from an interrupt.
 * \param req The request which will be enqueued.
 *********************************************************************************
 */
int8_t ADC_EnqueueRequest(ADC_Request * req,int channel)
{
	ADConv *adc = &g_adc;
	if (req->status != ADCR_IDLE) {
		return -1;
	}
	req->next = NULL;
	req->channel = channel;
	barrier();
	if (adc->queueTail) {
		adc->queueTail->next = req;
		adc->queueTail = req;
		req->status = ADCR_ENQUEUED;
	} else {
		adc->queueHead = adc->queueTail = req;
		adc_trigger_conversion(req->channel);
		req->status = ADCR_CONVERTING;
	}
	return 0;
}

/**
 ***************************************************************************
 * \fn void ADC_Init(void);
 * 	Enable the A/D converter, set the conversion frequency and
 *	the Reference voltage.
 ***************************************************************************
 */
void ADC_Init(void)
{
	memset(&g_adc, 0, sizeof(g_adc));
	/* Enable with CPU-Clk/16 == 2MHz  */
	//ADCA.PRESCALER = ADC_PRESCALER_DIV16_gc;
	ADCA.PRESCALER = ADC_PRESCALER_DIV512_gc;
	ADCA.CALL =  SP_ReadCalibrationByte( 0x20 );
	ADCA.CALH =  SP_ReadCalibrationByte( 0x21 );
	//ADCA.CAL = SP_ReadCalibrationByte(0x20) | (uint16_t)SP_ReadCalibrationByte(0x21) << 8;
	//ADCA.REFCTRL = ADC_TEMPREF_bm | ADC_BANDGAP_bm | ADC_REFSEL_INT1V_gc;
	ADCA.REFCTRL = ADC_TEMPREF_bm | ADC_BANDGAP_bm | ADC_REFSEL_AREFB_gc;
	ADCA.CTRLB = ADC_RESOLUTION_12BIT_gc | ADC_CONMODE_bm;
	ADCA.CTRLA = ADC_ENABLE_bm;	
	ADCA.CH0.INTCTRL = ADC_CH_INTMODE_COMPLETE_gc | ADC_CH_INTLVL_LO_gc;
	ADCA.CH1.INTCTRL = ADC_CH_INTMODE_COMPLETE_gc | ADC_CH_INTLVL_OFF_gc;
	ADCA.CH2.INTCTRL = ADC_CH_INTMODE_COMPLETE_gc | ADC_CH_INTLVL_OFF_gc;
	ADCA.CH3.INTCTRL = ADC_CH_INTMODE_COMPLETE_gc | ADC_CH_INTLVL_OFF_gc;
}

/**
 ***************************************************************************
 * \fn static void adc_done(void *clientData, uint16_t adval);
 * Eventhandler invoked whenever a direct A/D conversion is complete.
 ***************************************************************************
 */
static void adc_done(void *clientData, uint16_t adval)
{
	uint16_t *adresult = (uint16_t *) clientData;
	*adresult = adval;
}

static ADC_Request adcr = {
	.channel = 0,
	.callBack = adc_done,
	.clientData = NULL,
	.status = ADCR_IDLE
};

/**
 *********************************************************************
 * \fn uint16_t ADC_Read(uint8_t channel);
 * Simple ADC read for users which do not want 
 * to user the event mechanism
 * channel 14 is bandgap.
 * \param channel The A/D converter channel which will be read.  
 *********************************************************************
 */
uint16_t ADC_Read(uint8_t channel)
{
	uint16_t adval;
	/* Be careful here that this is not called in a critical section ! */
	while (adcr.status != ADCR_IDLE) {
		EV_Yield();
	}
	adcr.clientData = &adval;
	if (ADC_EnqueueRequest(&adcr,channel) < 0) {
		return 0xffff;
	}
	while (adcr.status != ADCR_IDLE) {
		EV_Yield();
	}
	return adval;
}

/**
 ****************************************************************************
 * \fn static int8_t cmd_adc(Interp * interp, uint8_t argc, char *argv[])
 * Shell command for reading from the A/D-Converter.
 ****************************************************************************
 */
static int8_t cmd_adc(Interp * interp, uint8_t argc, char *argv[])
{
	uint8_t channel;
	uint16_t adval;
	if (argc < 2) {
		return -EC_BADNUMARGS;
	}
	if(strcmp(argv[1],"signed") == 0) {
		ADCA.CTRLB |= ADC_CONMODE_bm;
		return EC_OK;	
	}
	if(strcmp(argv[1],"unsigned") == 0) {
		ADCA.CTRLB &= ~ADC_CONMODE_bm;
		return EC_OK;	
	}
	channel = astrtoi16(argv[1]);
	if (channel > 0x1f) {
		return -EC_ARGRANGE;
	}
	adval = ADC_Read(channel);
	Interp_Printf_P(interp,"%d\n",adval);
	return 0;
}

INTERP_CMD(adc, cmd_adc, "adc         <channel>  # Read ADC channel");

/**
 ****************************************************************************
 * \fn static int8_t cmd_adcref(Interp * interp, uint8_t argc, char *argv[])
 * Shell command for reading and writing the Reference voltage settings
 * of the A/D Converter module.
 ****************************************************************************
 */
static int8_t cmd_adcref(Interp * interp, uint8_t argc, char *argv[])
{
	uint16_t tempref;
	uint16_t calib;
	tempref = SP_ReadCalibrationByte(0x2E) | (uint16_t)SP_ReadCalibrationByte(0x2F) << 8;
	calib = SP_ReadCalibrationByte(0x20) | (uint16_t)SP_ReadCalibrationByte(0x21) << 8;
	Interp_Printf_P(interp,"TempCalib %d, ADCCalib %d\n",tempref,calib);
	return 0;
}


INTERP_CMD(adcref, cmd_adcref,
	   "adcref      ?<ref_voltage_source>?  # Read/Set Ref-Voltage source ");
