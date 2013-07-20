/**
 ***********************************************************************************
 * Module for Shift Register with 74HC594
 * Atmel: PC0 für Shift Clock
 *        PC1 für Data 
 * 	  PC4 für Latch  
 ***********************************************************************************
 */

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include "interpreter.h"
#include "hex.h"

#define PINCTRL_CLK_SR		PORTC.PIN0CTRL
#define PINCTRL_DATA_SR		PORTC.PIN1CTRL
#define PINCTRL_LATCH_SR	PORTC.PIN4CTRL
		

#define DATA_SR_DIROUT	 	PORTC.DIRSET = (1 << 1)
#define DATA_SR_HIGH		PORTC.OUTSET = (1 << 1)	
#define DATA_SR_LOW		PORTC.OUTCLR = (1 << 1)

#define CLK_SR_DIROUT		PORTC.DIRSET = (1 << 0)	
#define CLK_SR_HIGH		PORTC.OUTSET = (1 << 0)	
#define CLK_SR_LOW		PORTC.OUTCLR = (1 << 0)

#define LATCH_SR_DIROUT		PORTC.DIRSET = (1 << 4)	
#define LATCH_SR_HIGH		PORTC.OUTSET = (1 << 4)
#define LATCH_SR_LOW		PORTC.OUTCLR = (1 << 4)

static void
ShiftOut(uint16_t shiftval) 
{
	unsigned int i;	
	LATCH_SR_LOW;
	for(i = 0; i < 16; i++) {
		if(shiftval & 0x8000) {
			DATA_SR_HIGH;
		} else {
			DATA_SR_LOW;
		}
		CLK_SR_HIGH;
		shiftval <<= 1;
		CLK_SR_LOW;
	} 
	LATCH_SR_HIGH;
}

/**
 ***************************************************************************************
 * \fn static int8_t cmd_shift(Interp * interp, uint8_t argc, char *argv[])
 ***************************************************************************************
 */
static int8_t
cmd_shift(Interp * interp, uint8_t argc, char *argv[])
{
        uint16_t val;
	if(argc < 2) {
		return -EC_BADNUMARGS;
	}
	val = astrtoi16(argv[1]);
	ShiftOut(val);
        return 0;
}

INTERP_CMD(shift, cmd_shift, "shift value  # shift out a value");

void
ShiftReg_Init(uint16_t initval)
{	
	PINCTRL_CLK_SR = PORT_OPC_TOTEM_gc;
	PINCTRL_DATA_SR = PORT_OPC_TOTEM_gc;
	PINCTRL_LATCH_SR = PORT_OPC_TOTEM_gc;

	CLK_SR_DIROUT;
	DATA_SR_DIROUT;
	LATCH_SR_DIROUT;
	CLK_SR_LOW;
	LATCH_SR_LOW;
	ShiftOut(initval);
}
