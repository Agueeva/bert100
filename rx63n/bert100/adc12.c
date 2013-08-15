/**
 * RX63N module
 */

#include "types.h"
#include "adc12.h"
#include "iodefine.h"
#include "console.h"
#include "interpreter.h"
#include "hex.h"

uint16_t ADC12_Read(void)
{
    uint16_t adc_value;
    /* Start a conversion */
    S12AD.ADCSR.BIT.ADST = 1;
    /* Wait for the conversion to end */
    while(1 == S12AD.ADCSR.BIT.ADST);

    /* Fetch ADC value */
    adc_value = (uint16_t)(S12AD.ADDR0 & 0x0FFF);
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
	Con_Printf("ADVAL: %u\n",ADC12_Read());
	return 0;
}

INTERP_CMD(adc12Cmd, "adc12", cmd_adc12, "adc12  # Read from 12 Bit A/D converter");
void
ADC12_Init(void) 
{
	MSTP_S12AD = 0;
	/* ADC clock = PCLK/8, single scan mode */
	S12AD.ADCSR.BYTE = 0x00;
	/* Selects AN000 */
	S12AD.ADANS0.WORD = 0x0001;
	Interp_RegisterCmd(&adc12Cmd);
}
