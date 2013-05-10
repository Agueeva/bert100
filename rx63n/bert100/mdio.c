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
#define DIR_OUT		(1)
#define DIR_IN		(0)

NOINLINE static void
mdio_delay200ns(void)
{
	asm("mov.l %0,r1"::"g"(F_CPU / 20000000) : "memory","r1");
	asm("label9279: ":::);
	asm("sub #1,r1":::"r1");
	asm("bpz label9279":::);
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
GetMDI(uint8_t value) 
{

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

static void
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
		SetMDC(1)
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
MDIO_Init(void)
{
	unsigned int i;
	for(i = 0; i < 32; i++) {
		Con_Printf("%u: 0x%04x\n",i,MDC_Read(i));
	}
}
