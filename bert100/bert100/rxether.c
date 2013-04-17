/*
 * Renesas RX Ethernet controller driver
 */

#include "iodefine.h"
#include "types.h"
#include "rxether.h"
#include "phy.h"

void
RX_EtherInit()
{
	volatile uint32_t i;
	MSTP(EDMAC) = 0;
	for(i = 0; i < 10000; i++) {

	}
	EDMAC.EDMR.BIT.SWR = 1;
	for(i = 0; i < 10000; i++) {

	}
	/* This is a locally assigned unicast address */
	ETHERC.MAHR = 0x12345678;
	ETHERC.MALR.LONG = 0x9abc;
	
	ETHERC.ECSR.LONG = 0x37;	/* Clear all status bits */
	EDMAC.EESIPR.BIT.TCIP = 1; 	/* Enable the transmission Complete interrupt */
	EDMAC.EESIPR.BIT.FRIP = 1;	/* Enable the Frame reception interrupt */
	ETHERC.RFLR.LONG = 1522;	/* Standard Ethernet MAXPS + 4 Byte VLAN Header */
	ETHERC.IPGR.LONG = 0x14;	
	EDMAC.EESR.LONG = 0x47ff0f9f;	/* Clear all status bits, should not be necessary, cleared by reset */
	EDMAC.EDMR.BIT.DE = 1;		/* Little endian DMA */	
	//EDMAC.RDLAR = bla;
	//EDMAC.TDLAR = blub;
	EDMAC.TRSCER.LONG = 0;		/* Should not be necessary because cleared by reset */
	EDMAC.TFTR.LONG = 0;		/* Store and forward mode */
	EDMAC.FDR.LONG = 5;		/* Fifo 1536 bytes */
	EDMAC.RMCR.LONG = 0;		/* Should not be necessary because cleared by reset */
	EDMAC.FCFTR.LONG = 0x00070005; 
#if 0
	IPR(ETHER,EINT) = 2;
	IEN(ETHER,EINT = 1;
#endif
	Phy_Init();
}
