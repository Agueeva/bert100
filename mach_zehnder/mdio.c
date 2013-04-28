/**
 *****************************************************
 * Ethernet Phy control. Uses the RX Ethernet module
 * for the MDIO interface. So Phy_Init is called from
 * the Ethernet initilization.
 *****************************************************
 */

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include "console.h"
#include "interpreter.h"
#include "hex.h"
#include "mdio.h"
#include "timer.h"

#define DIR_OUT		(1)
#define DIR_IN		(0)

#define PORT_MDIO	PORTB
#define PORT_MDC	PORTB
#define PIN_MDIO	(2)
#define PIN_MDC		(3)
#define PINCTRL_MDIO	PORTB_PIN2CTRL
#define PINCTRL_MDC	PORTB_PIN3CTRL

static void
mdio_delay200ns(void)
{
	volatile uint8_t i;
	for(i = 0; i < 3; i++) {

	}
	return;
}
#define mdio_delay() mdio_delay200ns()

static inline void
SetDirection(uint8_t dir) 
{
	if(dir == DIR_IN) {
		PORT_MDIO.DIRCLR  = (1 << PIN_MDIO);
	} else {
		PORT_MDIO.DIRSET  = (1 << PIN_MDIO);
	}
}

static inline void
SetMDC(uint8_t value) 
{
	if(value == 0) {
		PORT_MDC.OUTCLR  = (1 << PIN_MDC);
	} else {
		PORT_MDC.OUTSET  = (1 << PIN_MDC);
	}
}

static inline void
SetMDO(uint8_t value) 
{
	if(value == 0) {
		PORT_MDIO.OUTCLR  = (1 << PIN_MDIO);
	} else {
		PORT_MDIO.OUTSET  = (1 << PIN_MDIO);
	}
}

static inline uint8_t
GetMDI() 
{
	return (PORT_MDIO.IN >> PIN_MDIO) & 1;
}


static uint16_t 
MDIO_Read(uint16_t phy_addr,uint16_t regAddr)
{
	unsigned int i;
	uint16_t outval,inval;
	SetMDO(1);
	SetDirection(DIR_OUT);
	mdio_delay();
	/* Preamble */
	for(i = 0; i < 32; i++) {
		SetMDC(1);
		mdio_delay();
		SetMDC(0);
		mdio_delay();
	}
	//outval = 0x1800;
	outval = 0x0c00;
	outval |= (phy_addr << 5);
	outval |= (regAddr & 0x1f);
	for(i = 0; i < 14; i++,  outval <<= 1) {
		uint8_t mdo = (outval & 0x2000) ? 1 : 0; 
		SetMDO(mdo);
		SetMDC(1);
		mdio_delay();
		SetMDC(0);
		mdio_delay();
	}
	/* Bus release with one clock cycle */
	for(i = 0; i < 1; i++) {
		SetDirection(DIR_IN);
		SetMDC(1);
		mdio_delay();
		SetMDC(0);
		mdio_delay();
	}
	inval = 0;
	for(i = 0; i < 16; i++) {
		//SleepMs(1000);
		SetMDC(1);
		mdio_delay();
		SetMDC(0);
		mdio_delay();
		/* 
		 ******************************************************
		 * delay of data is up to 300ns from rising edge so 
		 * better sample behind falling edge 
		 ******************************************************
		 */
		inval <<= 1;
		if(GetMDI()) {
			inval |= 1;
		}
	}
	/* Let the device release the bus ? */
	SetMDC(1);
	mdio_delay();
	SetMDC(0);
	mdio_delay();
	return inval;
}

void
MDIO_Write(uint16_t phy_addr,uint16_t regAddr,uint16_t value)
{
	unsigned int i;
	uint16_t outval;
	/* Preamble */
	SetMDO(1);
	SetDirection(DIR_OUT);
	mdio_delay();
	for(i = 0; i < 32; i++) {
		SetMDC(1);
		mdio_delay();
		SetMDC(0);
		mdio_delay();
	}
	//outval = 0x5002;
	outval = 0x1002;
	outval |= (phy_addr << 7);
	outval |= (regAddr & 0x1f) << 2;	
	for(i = 0; i < 16; i++,  outval <<= 1) {
		uint8_t mdo = (outval & 0x8000) ? 1 : 0; 
		SetMDO(mdo);		
		SetMDC(1);
		mdio_delay();
		SetMDC(0);
		mdio_delay();
	}
	outval = value; 
	for(i = 0; i < 16; i++,  outval <<= 1) {
		uint32_t mdo = (outval & 0x8000) ? 1 : 0; 
		SetMDO(mdo);
		SetMDC(1);
		mdio_delay();
		SetMDC(0);
		mdio_delay();
	}
	SetDirection(DIR_IN);
	SetMDC(1);
	mdio_delay();
	SetMDC(0);
	mdio_delay();
}

static int8_t
cmd_mdio(Interp * interp, uint8_t argc, char *argv[])
{
	uint8_t addr,reg;
	uint16_t val;
	if(argc == 4) {
		addr = astrtoi16(argv[1]);
		reg = astrtoi16(argv[2]);
		val = astrtoi16(argv[3]);
		MDIO_Write(addr,reg,val);
		return 0;
	} else if(argc == 3) {
		addr = astrtoi16(argv[1]);
		reg = astrtoi16(argv[2]);
		val = MDIO_Read(addr,reg);
		Con_Printf_P("%u.%u: 0x%x\n",addr,reg,val);
		return 0;
	}
	return -EC_BADNUMARGS;
}

INTERP_CMD(mdio, cmd_mdio, "mdio <addr> <register> ?<value>?   # read write to/from mdio");

void
MDIO_Init(void)
{
	unsigned int i;
	PINCTRL_MDC = PORT_OPC_TOTEM_gc;
	PINCTRL_MDIO = PORT_OPC_TOTEM_gc;
	PORT_MDC.DIRSET  = (1 << PIN_MDC);

	for(i = 0; i < 32; i++) {
		Con_Printf_P("%u: 0x%04x\n",i,MDIO_Read(0x1f,i));
	}
}
