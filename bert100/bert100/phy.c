/**
 *****************************************************
 * Ethernet Phy control. Uses the RX Ethernet module
 * for the MDIO interface. So Phy_Init is called from
 * the Ethernet initilization.
 *****************************************************
 */

#include "phy.h"
#include "iodefine.h"
#include "config.h"
#include "types.h"
#include "console.h"

#define PIR_MDC	(1 << 0)	
#define PIR_MMD	(1 << 1)	/* 1 == Write, 0 == Read */
#define PIR_MDO	(1 << 2)
#define PIR_MDI	(1 << 3)
#define PHY_ADDR	(31)

NOINLINE static void
mdio_delay200ns(void)
{
	asm("mov.l %0,r1"::"g"(F_CPU / 20000000) : "memory","r1");
	asm("label9279: ":::);
	asm("sub #1,r1":::"r1");
	asm("bne label9279":::);
}
#define mdio_delay() mdio_delay200ns()

static uint16_t 
Phy_Read(uint16_t regAddr)
{
	unsigned int i;
	uint16_t outval,inval;
	ETHERC.PIR.LONG = PIR_MMD | PIR_MDO;	
	mdio_delay();
	/* Preamble */
	for(i = 0; i < 32; i++) {
		ETHERC.PIR.LONG = PIR_MMD | PIR_MDO | PIR_MDC;
		mdio_delay();
		ETHERC.PIR.LONG = PIR_MMD | PIR_MDO;	
		mdio_delay();
	}
	outval = 0x1800;
	outval |= (PHY_ADDR << 5);
	outval |= (regAddr & 0x1f);
	for(i = 0; i < 14; i++,  outval <<= 1) {
		uint32_t mdo = (outval & 0x2000) ? PIR_MDO : 0; 
		ETHERC.PIR.LONG = PIR_MMD | mdo | PIR_MDC;
		mdio_delay();
		ETHERC.PIR.LONG = PIR_MMD | mdo;	
		mdio_delay();
	}
	/* Bus release with two clock cycle */
	for(i = 0; i < 2; i++) {
		ETHERC.PIR.LONG = 0 | PIR_MDC;
		mdio_delay();
		ETHERC.PIR.LONG = 0;	
		mdio_delay();
	}
	inval = 0;
	for(i = 0; i < 16; i++) {
		ETHERC.PIR.LONG = PIR_MDC;
		mdio_delay();
		inval <<= 1;
		if(ETHERC.PIR.LONG & PIR_MDI) {
			inval |= 1;
		}
		ETHERC.PIR.LONG = 0;	
		mdio_delay();
	}
	/* Let the device release the bus ? */
	ETHERC.PIR.LONG = 0 | PIR_MDC;
	mdio_delay();
	ETHERC.PIR.LONG = 0;	
	mdio_delay();
	return inval;
}

static void
Phy_Write(uint16_t regAddr,uint16_t value)
{
	unsigned int i;
	uint16_t outval;
	ETHERC.PIR.LONG = PIR_MMD | PIR_MDO;	
	mdio_delay();
	/* Preamble */
	for(i = 0; i < 32; i++) {
		ETHERC.PIR.LONG = PIR_MMD | PIR_MDO | PIR_MDC;
		mdio_delay();
		ETHERC.PIR.LONG = PIR_MMD | PIR_MDO;	
		mdio_delay();
	}
	outval = 0x6002;
	outval |= (PHY_ADDR << 7);
	outval |= (regAddr & 0x1f) << 2;	
	for(i = 0; i < 16; i++,  outval <<= 1) {
		uint32_t mdo = (outval & 0x8000) ? PIR_MDO : 0; 
		ETHERC.PIR.LONG = PIR_MMD | mdo | PIR_MDC;
		mdio_delay();
		ETHERC.PIR.LONG = PIR_MMD | mdo;	
		mdio_delay();
	}
	outval = value; 
	for(i = 0; i < 16; i++,  outval <<= 1) {
		uint32_t mdo = (outval & 0x8000) ? PIR_MDO : 0; 
		ETHERC.PIR.LONG = PIR_MMD | mdo | PIR_MDC;
		mdio_delay();
		ETHERC.PIR.LONG = PIR_MMD | mdo;	
		mdio_delay();
	}
	ETHERC.PIR.LONG = 0 | PIR_MDC;
	mdio_delay();
	ETHERC.PIR.LONG = 0;	
	mdio_delay();
}

void
Phy_Init(void)
{
	unsigned int i;
	for(i = 0; i < 32; i++) {
		Con_Printf("%u: 0x%04x\n",i,Phy_Read(i));
		SleepMs(100);
	}
}
