#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "irqflags.h"
#include "console.h"
#include "events.h"
#include "interpreter.h"
#include "hex.h"
#include "ad537x.h"

#define PORT_SYNC	PORTE
#define	PIN_SYNC	0
#define PINCTRL_SYNC    PORTE.PIN0CTRL

#define PORT_SDI	PORTE
#define	PIN_SDI		1
#define PINCTRL_SDI	PORTE.PIN1CTRL

#define PORT_SCLK	PORTE
#define	PIN_SCLK	2
#define PINCTRL_SCLK	PORTE.PIN2CTRL

#define PORT_SDO	PORTE
#define	PIN_SDO		3
#define PINCTRL_SDO	PORTE.PIN3CTRL

#define PORT_BUSY	PORTE
#define	PIN_BUSY	4
#define PINCTRL_BUSY	PORTE.PIN4CTRL

#define PORT_RESET	PORTE
#define	PIN_RESET	5
#define PINCTRL_RESET	PORTE.PIN5CTRL

#define PORT_CLR	PORTE
#define	PIN_CLR		6
#define PINCTRL_CLR	PORTE.PIN6CTRL

static const char str_X1A[] PROGMEM = "x1a";
static const char str_X1B[] PROGMEM = "x1b";
static const char str_C[] PROGMEM = "c";
static const char str_M[] PROGMEM = "m";
static const char str_CONTROL[] PROGMEM = "control";
static const char str_OFS0[] PROGMEM = "ofs0";
static const char str_OFS1[] PROGMEM = "ofs1";
static const char str_ABSELECT[] PROGMEM = "abselect";


static uint32_t  
AD537x_Write(uint32_t value)
{
	int8_t i;
	uint32_t inval;
	inval = 0;
	PORT_SYNC.OUTCLR = (1 << PIN_SYNC);
	for(i = 24; i >= 0;i--) {
		if((value >> i) & 1) {
			PORT_SDI.OUTSET = (1 << PIN_SDI);
		} else {
			PORT_SDI.OUTCLR = (1 << PIN_SDI);
		}
		PORT_SCLK.OUTCLR = (1 << PIN_SCLK);
		if(PORT_SDO.IN & (1 << PIN_SDO)) { 
			inval = (inval << 1) | 1;
		} else {
			inval = (inval << 1);
		}
		PORT_SCLK.OUTSET = (1 << PIN_SCLK);
	}		
	return inval;
}

/*
 * Readback  
 */
static uint32_t 
AD537x_Readback(uint8_t reg,uint8_t channel) 
{
	uint32_t spiCmd;
	uint32_t result;
	spiCmd = ((uint32_t)reg << 13) | (((uint16_t)channel + 8) << 7);
	AD537x_Write(spiCmd);
	spiCmd = 0; /* NOP */	
	result = AD537x_Write(spiCmd);
	return result;	
}

static int8_t
cmd_dac(Interp *interp,uint8_t argc,char *argv[])
{

}

INTERP_CMD(dac, cmd_dac,
           "dac         <fieldname> ?<channel>? ?<value> # ");

void
AD537x_Init(void)
{
	
	PINCTRL_SYNC = PORT_OPC_TOTEM_gc;
	PORT_SYNC.DIRSET = (1 << PIN_SYNC);

	PINCTRL_SDI = PORT_OPC_TOTEM_gc;
	PORT_SDI.DIRSET = (1 << PIN_SDI);

	PINCTRL_SCLK = PORT_OPC_TOTEM_gc;
	PORT_SCLK.DIRSET = (1 << PIN_SCLK);

	PINCTRL_SDO = PORT_OPC_PULLUP_gc; 
	PORT_SDO.DIRCLR = (1 << PIN_SDO);
	
	PINCTRL_BUSY = PORT_OPC_PULLUP_gc;
	PORT_BUSY.DIRCLR = (1 << PIN_BUSY);

	PINCTRL_RESET = PORT_OPC_TOTEM_gc;
	PORT_RESET.DIRCLR = (1 << PIN_RESET);

	PINCTRL_CLR = PORT_OPC_TOTEM_gc;
	PORT_CLR.DIRSET = (1 << PIN_CLR);
}
