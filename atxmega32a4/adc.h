#ifndef _ADC_H
#define _ADC_H
/*
 *************************************************************
 * (C) 2009 Jochen Karrer
 *//**
 * \file adc.h
 * A/D Converter access using request queues.
 *************************************************************
 */

typedef void ADC_CallBack(void *clientData, uint16_t adval);
/*
 ****************************************************************
 * The ADC driver is organized as a queue. The user enqueues a 
 * queue-entry and when the data is ready a callback will be
 * invoked
 ****************************************************************
 */

#define ADCR_IDLE	0
#define ADCR_ENQUEUED	1
#define ADCR_CONVERTING 2

/**
 ************************************************************
 * \struct ADC_Request
 * The ADC request is a structure which will be enqueued
 * by the A/D conversion library and handled as soon as the
 * A/D converter is available. A callback will be invoked
 * when the conversion is complete.
 ************************************************************
 */
typedef struct ADC_Request {
	uint8_t channel;	/**< The channel selected for meassurement. */
	ADC_CallBack *callBack;	/**< The callBack procedure will be invoked when the meassurement is complete. */
	void *clientData;	/**< Argument for the callback procedure. */
	struct ADC_Request *next; /**< Next request in list of enqueued requests. */
	uint8_t status;		  /**< completion status of this request. */
} ADC_Request;

/**
 ************************************************************
 * \fn void ADC_Init(void);
 * Initialize the AD-Converter.
 ************************************************************
 */
void ADC_Init(void);

/**
 ***********************************************************
 * \fn uint16_t ADC_Read(uint8_t channel);
 * Read from the A/D Converter. This function will call
 * the event scheduler as long as the A/D converter is
 * not ready with reading.
 * Do not call this from an interrupt.
 ***********************************************************
 */
uint16_t ADC_Read(uint8_t channel);

/**
 ***********************************************************
 * \fn int8_t ADC_EnqueueRequest(ADC_Request * req);
 * Enqueue an request for reading an A/D Converter.
 * Do not call this from an interrupt.
 ***********************************************************
 */
int8_t ADC_EnqueueRequest(ADC_Request * req,int channel);


/**
 ***********************************************************
 * \fn static inline void ADC_RequestInit(ADC_Request *adcr,int channel,ADC_CallBack *proc,void *cd)
 * Initialize an A/D converter request.
 ***********************************************************
 */
static inline void
ADC_RequestInit(ADC_Request *adcr,ADC_CallBack *proc,void *cd)
{
        adcr->callBack = proc;
        adcr->clientData = cd;
        adcr->status = ADCR_IDLE;
}

#define ADC_CH_TEMP	(0)
#define ADC_CH_BGAP	(1)
#define ADC_CH_VCC	(2)
#define ADC_CH_DAC	(3)
#define ADC_CH_PA0 	(0x10)
#define ADC_CH_PA1	(0x11) 
#define ADC_CH_PA2	(0x12)
#define ADC_CH_PA3	(0x13)
#define ADC_CH_PA4	(0x14)
#define ADC_CH_PA5	(0x15)
#define ADC_CH_PA6	(0x16)
#define ADC_CH_PA7	(0x17)
#define ADC_CH_PB0	(0x18)
#define ADC_CH_PB1	(0x19)
#define ADC_CH_PB2	(0x1A)
#define ADC_CH_PB3	(0x1B)

#endif
