/*
 * Renesas RX Ethernet controller driver
 */

#include "iodefine.h"
#include "types.h"
#include "rxether.h"
#include "phy.h"

/**
 *********************************************************************
 * Setup IO Ports for Ethernet RMII
 *********************************************************************
 */
static void
RX_EtherSetupIoPorts(void)
{
	MPC.PWPR.BIT.B0WI = 0;
	MPC.PWPR.BIT.PFSWE = 1;
	MPC.PFENET.BIT.PHYMODE = 0; /* RMII mode */
	MPC.P71PFS.BYTE = 0x11; /* MDIO */
	MPC.P72PFS.BYTE = 0x11;	/* MDC */
	/* P73 is WOL */
	MPC.P74PFS.BYTE = 0x12;	/* RXD1 */ 
	MPC.P75PFS.BYTE = 0x12; /* RXD0 */
	MPC.P76PFS.BYTE = 0x12; /* RXCLK */
	MPC.P77PFS.BYTE = 0x12; /* RXER input */
	PORT7.PMR.BYTE = 0xf6;
	PORT7.PDR.BYTE = 0x06;
	/* Port 8, TX */
	MPC.P80PFS.BYTE = 0x12;	/* TXEN */
	MPC.P81PFS.BYTE = 0x12; /* TXD0 */
	MPC.P82PFS.BYTE = 0x12; /* TXD1 */
	MPC.P83PFS.BYTE = 0x12; /* CRS input */
	PORT8.PMR.BYTE = 0x0f;
	PORT8.PDR.BYTE = 7;

	MPC.PWPR.BIT.PFSWE = 0;
	MPC.PWPR.BIT.B0WI = 1;
}
void
RX_EtherInit()
{
	volatile uint32_t i;
	RX_EtherSetupIoPorts();
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
