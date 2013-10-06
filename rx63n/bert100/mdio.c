/**
 *****************************************************
 * Ethernet Phy control. Uses the RX Ethernet module
 * for the MDIO interface. So Phy_Init is called from
 * the Ethernet initilization.
 *****************************************************
 */

#include "config.h"
#include "iodefine.h"
#include "types.h"
#include <string.h>
#include "console.h"
#include "interpreter.h"
#include "hex.h"
#include "mdio.h"
#include "timer.h"

#define DIR_OUT		(1)
#define DIR_IN		(0)

#define DIRCTRL_DIROUT	BSET(0,PORTA.PDR.BYTE)
#define DIRCTRL_HIGH	BSET(0,PORTA.PODR.BYTE)
#define DIRCTRL_LOW	BCLR(0,PORTA.PODR.BYTE)

#define MDIO_DIROUT	BSET(6,PORTE.PDR.BYTE)
#define MDIO_DIRIN	BCLR(6,PORTE.PDR.BYTE)
#define MDIO_HIGH	BSET(6,PORTE.PODR.BYTE)
#define MDIO_LOW	BCLR(6,PORTE.PODR.BYTE)
#define MDIO_READ	PORTE.PIDR.BIT.B6

#define MDC_DIROUT	BSET(7,PORTE.PDR.BYTE)
#define MDC_HIGH	BSET(7,PORTE.PODR.BYTE)
#define MDC_LOW		BCLR(7,PORTE.PODR.BYTE)

/// mdio_delay() DelayNs(200)
#define mdio_delay() 

typedef struct MDIOPort {
	uint8_t preambleLen;
} MDIOPort;

static MDIOPort gMDIOPort;

static inline void
SetDirection(uint8_t dir) 
{
	if(dir == DIR_IN) {
		MDIO_DIRIN;
		DIRCTRL_LOW;
	} else {
		DIRCTRL_HIGH;
		MDIO_DIROUT;
	}
}

static inline void
SetMDC(uint8_t value) 
{
	if(value == 0) {
		MDC_LOW;
	} else {
		MDC_HIGH;
	}
}

static inline void
SetMDO(uint8_t value) 
{
	if(value == 0) {
		MDIO_LOW;
	} else {
		MDIO_HIGH;
	}
}

static inline uint8_t
GetMDI() 
{
	return MDIO_READ;
}

/**
 ****************************************************************************
 * \fn static uint16_t MDIO_Read(uint8_t phy_addr,uint8_t regAddr)
 * Read 16 Bit from MDIO
 * This is not the standard Read command as used for PHY Registers.
 * Use this command after setting the Address with a separate address 
 * command with no other commands between.
 ****************************************************************************
 */
uint16_t 
MDIO_Read(uint8_t phy_addr,uint8_t regAddr)
{
	uint32_t i;
	uint32_t outval,inval;
	SetMDO(1);
	SetDirection(DIR_OUT);
	mdio_delay();
	/* Preamble */
	for(i = gMDIOPort.preambleLen; i > 0; i--) {
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
	SetDirection(DIR_IN);
	SetMDC(1);
	mdio_delay();
	SetMDC(0);
	mdio_delay();

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

uint16_t 
MDIO_ReadInc(uint8_t phy_addr,uint8_t regAddr)
{
	uint32_t i;
	uint32_t outval,inval;
	SetMDO(1);
	SetDirection(DIR_OUT);
	mdio_delay();
	/* Preamble */
	for(i = gMDIOPort.preambleLen; i > 0; i--) {
		SetMDC(1);
		mdio_delay();
		SetMDC(0);
		mdio_delay();
	}
	outval = 0x0800;
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
	SetDirection(DIR_IN);
	SetMDC(1);
	mdio_delay();
	SetMDC(0);
	mdio_delay();

	inval = 0;
	for(i = 0; i < 16; i++) {
		SetMDC(1);
		mdio_delay();
		inval <<= 1;
		SetMDC(0);
		mdio_delay();
		/* 
		 ******************************************************
		 * delay of data is up to 300ns from rising edge so 
		 * better sample behind falling edge 
		 ******************************************************
		 */
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
/**
 ************************************************************************
 * \fn void MDIO_Address(uint16_t phy_addr,uint16_t devType,uint16_t addr)
 * Send the register address to the MDIO device. This command is
 * required before a read or write command because these include no
 * address.
 ************************************************************************
 */

void
MDIO_Address(uint16_t phy_addr,uint16_t devType,uint16_t addr)
{
	uint8_t i;
	uint16_t outval;
	/* Preamble */
	SetMDO(1);
	SetDirection(DIR_OUT);
	mdio_delay();
	for(i = gMDIOPort.preambleLen; i > 0; i--) {
		SetMDC(1);
		mdio_delay();
		SetMDC(0);
		mdio_delay();
	}
	outval = 0x0002;
	outval |= ((phy_addr & 0x1f) << 7);
	outval |= (devType & 0x1f) << 2;	
	for(i = 0; i < 16; i++,  outval <<= 1) {
		uint8_t mdo = (outval & 0x8000) ? 1 : 0; 
		SetMDO(mdo);		
		SetMDC(1);
		mdio_delay();
		SetMDC(0);
		mdio_delay();
	}
	outval = addr; 
	for(i = 0; i < 16; i++,  outval <<= 1) {
		uint8_t mdo = (outval & 0x8000) ? 1 : 0; 
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

void
MDIO_Write(uint8_t phy_addr,uint8_t devType,uint16_t value)
{
	uint8_t i;
	uint16_t outval;
	/* Preamble */
	SetMDO(1);
	SetDirection(DIR_OUT);
	mdio_delay();
	for(i = gMDIOPort.preambleLen; i > 0; i--) {
		SetMDC(1);
		mdio_delay();
		SetMDC(0);
		mdio_delay();
	}
	outval = 0x1002;
	outval |= (phy_addr << 7);
	outval |= (devType & 0x1f) << 2;	
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
		uint8_t mdo = (outval & 0x8000) ? 1 : 0; 
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

static uint32_t bit_errs = 0;

static int8_t
cmd_mdio(Interp * interp, uint8_t argc, char *argv[])
{
	uint8_t phyaddr,devtype;
	uint16_t val;
	uint16_t addr;
	if(argc == 4) {
		phyaddr = astrtoi16(argv[1]);
		devtype = astrtoi16(argv[2]);
		addr = astrtoi16(argv[3]);
		MDIO_Address(phyaddr,devtype,addr);
		//MDIO_Write(phyaddr,devtype,val);
		return 0;
	} else if((argc == 3) && (strcmp(argv[1],"preamble") == 0)) {
		gMDIOPort.preambleLen = astrtoi16(argv[2]);
	} else if(argc == 3) {
		phyaddr = astrtoi16(argv[1]);
		devtype = astrtoi16(argv[2]);
		val = MDIO_Read(phyaddr,devtype);
		Con_Printf("%u.%u: 0x%x\n",phyaddr,devtype,val);
		return 0;
	} else if(argc == 5) {
		phyaddr = astrtoi16(argv[1]);
		devtype = astrtoi16(argv[2]);
		addr = astrtoi16(argv[3]);
		val = astrtoi16(argv[4]);
		MDIO_Address(phyaddr,devtype,addr);
		MDIO_Write(phyaddr,devtype,val);
	}
	Con_Printf("%08lu\n",bit_errs);
	return 0;
}

INTERP_CMD(mdioCmd, "mdio", cmd_mdio, "mdio <addr> <register> ?<value>?   # read write to/from mdio");

void pollProc(void *eventData);
TIMER_DECLARE(pollTimer,pollProc,NULL)

void pollProc(void *eventData)
{
	uint16_t val;
	Timer_Start(&pollTimer,1);
	val = MDIO_Read(1,30);
	bit_errs += val;
}

void
MDIO_Init(void)
{
	MDC_DIROUT;
	MDIO_DIRIN;
	SetDirection(DIR_IN);
	DIRCTRL_DIROUT;
	Interp_RegisterCmd(&mdioCmd);
	gMDIOPort.preambleLen = 32;
//	Timer_Start(&pollTimer,1);
}
