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
#include "interpreter.h"
#include "hex.h"
#include "timer.h"

#define PIR_MDC	(1 << 0)	
#define PIR_MMD	(1 << 1)	/* 1 == Write, 0 == Read */
#define PIR_MDO	(1 << 2)
#define PIR_MDI	(1 << 3)
#define PHY_ADDR	(1)

#define phy_delay() DelayNs(200)

static uint16_t 
Phy_Read(uint16_t regAddr)
{
	unsigned int i;
	uint16_t outval,inval;
	ETHERC.PIR.LONG = PIR_MMD | PIR_MDO;	
	phy_delay();
	/* Preamble */
	for(i = 0; i < 32; i++) {
		ETHERC.PIR.LONG = PIR_MMD | PIR_MDO | PIR_MDC;
		phy_delay();
		ETHERC.PIR.LONG = PIR_MMD | PIR_MDO;	
		phy_delay();
	}
	outval = 0x1800;
	outval |= (PHY_ADDR << 5);
	outval |= (regAddr & 0x1f);
	for(i = 0; i < 14; i++,  outval <<= 1) {
		uint32_t mdo = (outval & 0x2000) ? PIR_MDO : 0; 
		ETHERC.PIR.LONG = PIR_MMD | mdo;	
		ETHERC.PIR.LONG = PIR_MMD | mdo | PIR_MDC;
		phy_delay();
		ETHERC.PIR.LONG = PIR_MMD | mdo;	
		phy_delay();
	}
	/* Bus release with one clock cycle */
	for(i = 0; i < 1; i++) {
	//	ETHERC.PIR.LONG = 0;	
		ETHERC.PIR.LONG = 0 | PIR_MDC;
		phy_delay();
		ETHERC.PIR.LONG = 0;	
		phy_delay();
	}
	inval = 0;
	for(i = 0; i < 16; i++) {
		ETHERC.PIR.LONG = PIR_MDC;
		phy_delay();
		inval <<= 1;
		ETHERC.PIR.LONG = 0;	
		phy_delay();
		/* 
		 ******************************************************
		 * delay of data is up to 300ns from rising edge so 
		 * better sample behind falling edge 
		 ******************************************************
		 */
		if(ETHERC.PIR.LONG & PIR_MDI) {
			inval |= 1;
		}
	}
	/* Let the device release the bus ? */
	ETHERC.PIR.LONG = 0 | PIR_MDC;
	phy_delay();
	ETHERC.PIR.LONG = 0;	
	phy_delay();
	return inval;
}

/**
 **********************************************************************
 * \fn static void Phy_Write(uint16_t regAddr,uint16_t value)
 * Write to a register of the PHY
 **********************************************************************
 */
static void
Phy_Write(uint16_t regAddr,uint16_t value)
{
	unsigned int i;
	uint16_t outval;
	/* Preamble */
	for(i = 0; i < 32; i++) {
		ETHERC.PIR.LONG = PIR_MMD | PIR_MDO;	
		ETHERC.PIR.LONG = PIR_MMD | PIR_MDO | PIR_MDC;
		phy_delay();
		ETHERC.PIR.LONG = PIR_MMD | PIR_MDO;	
		phy_delay();
	}
	outval = 0x5002;
	outval |= (PHY_ADDR << 7);
	outval |= (regAddr & 0x1f) << 2;	
	for(i = 0; i < 16; i++,  outval <<= 1) {
		uint32_t mdo = (outval & 0x8000) ? PIR_MDO : 0; 
		ETHERC.PIR.LONG = PIR_MMD | mdo;
		ETHERC.PIR.LONG = PIR_MMD | mdo | PIR_MDC;
		phy_delay();
		ETHERC.PIR.LONG = PIR_MMD | mdo;	
		phy_delay();
	}
	outval = value; 
	for(i = 0; i < 16; i++,  outval <<= 1) {
		uint32_t mdo = (outval & 0x8000) ? PIR_MDO : 0; 
		ETHERC.PIR.LONG = PIR_MMD | mdo;	
		ETHERC.PIR.LONG = PIR_MMD | mdo | PIR_MDC;
		phy_delay();
		ETHERC.PIR.LONG = PIR_MMD | mdo;	
		phy_delay();
	}
	ETHERC.PIR.LONG = 0;	
	ETHERC.PIR.LONG = 0 | PIR_MDC;
	phy_delay();
	ETHERC.PIR.LONG = 0;	
	phy_delay();
}

static int8_t
cmd_phy(Interp * interp, uint8_t argc, char *argv[])
{
	uint16_t regAddr;
	uint16_t value;
	int i;
	if(argc == 1) {
		for(i = 0; i < 32; i++) {
			Con_Printf("%02u: 0x%04x   ",i,Phy_Read(i));
			if((i & 3) == 3) {
				Con_Printf("\n");
			}
		}
	} else if(argc == 2) {
		regAddr = astrtoi16(argv[1]);
		Con_Printf("%u: 0x%04x\n",regAddr,Phy_Read(regAddr));
	} else if(argc == 3) {
		regAddr = astrtoi16(argv[1]);
		value = astrtoi16(argv[2]);
		Phy_Write(regAddr,value);
		Con_Printf("%u: 0x%04x\n",regAddr,Phy_Read(regAddr));
	}
	return 0;
}

INTERP_CMD(phyCmd, "phy", cmd_phy, "phy      # ");

void
Phy_Init(void)
{
	Interp_RegisterCmd(&phyCmd);
}
