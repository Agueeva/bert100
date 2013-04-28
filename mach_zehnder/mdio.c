/**
 *****************************************************
 * Ethernet Phy control. Uses the RX Ethernet module
 * for the MDIO interface. So Phy_Init is called from
 * the Ethernet initilization.
 *****************************************************
 */

#include <stdint.h>
#include "console.h"
#include "interpreter.h"
#include "hex.h"

#define PIR_MDC	(1 << 0)	
#define PIR_MMD	(1 << 1)	/* 1 == Write, 0 == Read */
#define PIR_MDO	(1 << 2)
#define PIR_MDI	(1 << 3)
#define DIR_OUT		(1)
#define DIR_IN		(0)

static void
mdio_delay200ns(void)
{
	volatile uint8_t i;
	for(i = 0; i < 20; i++) {

	}
	return;
}
#define mdio_delay() mdio_delay200ns()

static inline void
SetDirection(uint8_t dir) 
{
	if(dir == DIR_IN) {
	} else {
	}
}

static inline void
SetMDC(uint8_t value) 
{
	if(value == 0) {
	} else {
	}
}

static inline void
SetMDO(uint8_t value) 
{
	if(value == 0) {
	} else {
	}
}

static uint8_t
GetMDI() 
{
	return 0;
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
	outval = 0x1800;
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
	outval = 0x6002;
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
	uint8_t addr,reg,val;
	if(argc == 4) {
		addr = astrtoi16(argv[1]);
		reg = astrtoi16(argv[2]);
		val = astrtoi16(argv[3]);
		
	} else if(argc == 3) {
		addr = astrtoi16(argv[1]);
		reg = astrtoi16(argv[2]);
		val = MDIO_Read(addr,reg);
		Con_Printf_P("0x%x",val);
	}
	return 0;
}

INTERP_CMD(mdio, cmd_mdio, "mdio <addr> <register> ?<value>?   # read write to/from mdio");

void
MDIO_Init(void)
{
	unsigned int i;
	for(i = 0; i < 32; i++) {
		Con_Printf_P("%u: 0x%04x\n",i,MDIO_Read(0x3f,i));
	}
}
