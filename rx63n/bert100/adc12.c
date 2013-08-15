/**
 * RX63N module
 */

#include "types.h"
#include "adc12.h"
#include "iodefine.h"
#include "console.h"
#include "interpreter.h"
#include "hex.h"

uint16_t ADC12_Read(int channel)
{
    uint16_t adc_value;
    volatile uint16_t *addr;
    if(channel < 16) {
    	S12AD.ADANS0.WORD = 0x0001 << channel;
    	S12AD.ADANS1.WORD = 0;
    } else if(channel <= 20) {
    	S12AD.ADANS0.WORD = 0;
    	S12AD.ADANS1.WORD = 0x0001 << (channel - 16);
    } else {
	Con_Printf("Illegal A/D channel %lu\n",channel);
	return 0;
    }	
    /* Start a conversion */
    S12AD.ADCSR.BIT.ADST = 1;
    /* Wait for the conversion to end */
    addr = &S12AD.ADDR0;
    while(1 == S12AD.ADCSR.BIT.ADST);
    /* Fetch ADC value */
    adc_value = (uint16_t)(addr[channel] & 0x0FFF);
    return adc_value;
}

/**
 ****************************************************************************
 * \fn static int8_t cmd_sci0(Interp * interp, uint8_t argc, char *argv[])
 ****************************************************************************
 */
static int8_t
cmd_adc12(Interp * interp, uint8_t argc, char *argv[])
{
	int channel;
	if(argc < 2) {
		return -EC_BADNUMARGS;
	}
	channel = astrtoi16(argv[1]);	
	Con_Printf("ADVAL: %u\n",ADC12_Read(channel));
	return 0;
}

INTERP_CMD(adc12Cmd, "adc12", cmd_adc12, "adc12 <channel-nr> # Read from 12 Bit A/D converter");

void
ADC12_Init(void) 
{
	MSTP_S12AD = 0;
	/* ADC clock = PCLK/8, single scan mode */
	S12AD.ADCSR.BYTE = 0x00;
	/* Selects AN000 */
	//S12AD.ADANS0.WORD = 0x0001;
	//PORT4.PMR.BYTE = 0x01;
	Interp_RegisterCmd(&adc12Cmd);
}
