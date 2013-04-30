/*
 * Renesas RX Ethernet controller driver
 */

#include "iodefine.h"
#include "types.h"
#include "rxether.h"
#include "phy.h"
#include "interrupt_handlers.h"

#define NUM_TXBUFS	(2)
#define EMAC_TC_INT	(UINT32_C(1) << 21)
#define EMAC_FR_INT	(UINT32_C(1) << 18)

typedef struct RxEth {
	uint8_t txBuffer[1522 * NUM_TXBUFS] __attribute__((aligned (4)));	
} RxEth;

static RxEth gRxEth;

/**
 *********************************************************************
 * Setup IO Ports for Ethernet RMII
 *********************************************************************
 */
__attribute__((unused)) static void
RX_EtherSetupIoPortsRMII(void)
{
	MPC.PWPR.BIT.B0WI = 0;
	MPC.PWPR.BIT.PFSWE = 1;
	MPC.PFENET.BIT.PHYMODE = 0; /* RMII mode */
	MPC.P71PFS.BYTE = 0x11; /* MDIO */
	MPC.P72PFS.BYTE = 0x11;	/* MDC */
	/* P73 is WOL */
	MPC.P74PFS.BYTE = 0x12;	/* RXD1 */ 
	MPC.P75PFS.BYTE = 0x12; /* RXD0 */
	MPC.P76PFS.BYTE = 0x12; /* ETHCLK */
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

__attribute__((unused)) static void
RX_EtherSetupIoPortsMII(void)
{
	MPC.PWPR.BIT.B0WI = 0;	
	MPC.PWPR.BIT.PFSWE = 1;
	MPC.PFENET.BIT.PHYMODE = 1;
	MPC.P71PFS.BYTE = 0x11; /* P71 MDIO	*/
	MPC.P72PFS.BYTE = 0x11; /* P72 MDC	*/
	MPC.P74PFS.BYTE = 0x11; /* P74 RXD1	*/
	MPC.P75PFS.BYTE = 0x11;	/* P75 RXD0	*/
	MPC.P76PFS.BYTE = 0x11; /* P76 RXCLK	*/
	MPC.P77PFS.BYTE = 0x11; /* P77 RXER	*/
	PORT7.PMR.BYTE = 0xf6;  /* Port Mode Register to Ether */
	PORT7.PDR.BYTE = 0x06;  /* P71 and P72 are output */
	/* Port 8 */	
	MPC.P80PFS.BYTE = 0x11;	/* TXEN */
	MPC.P81PFS.BYTE = 0x11;	/* TXD0 */
	MPC.P82PFS.BYTE = 0x11;	/* TXD1 */
	MPC.P83PFS.BYTE = 0x11;	/* CRS input */
	PORT8.PMR.BYTE = 0x0f;
	PORT8.PDR.BYTE = 0x07;
	
	/* Port C */
	MPC.PC0PFS.BYTE = 0x11; /* PC0 RXD3 */
	MPC.PC1PFS.BYTE = 0x11;	/* PC1 RXD2 */
	MPC.PC2PFS.BYTE = 0x11;	/* PC2 RXDV */
	MPC.PC3PFS.BYTE = 0x11; /* PC3 TXER */
	MPC.PC4PFS.BYTE = 0x11;	/* PC4 TXCLK */
	MPC.PC5PFS.BYTE = 0x11;	/* PC5 TXD2 */
	MPC.PC6PFS.BYTE = 0x11;	/* PC6 TXD3 */
	MPC.PC7PFS.BYTE = 0x11; /* PC7 COL  */
	PORTC.PMR.BYTE = 0xff;	
	PORTC.PDR.BYTE = 0x68;
	
	MPC.PWPR.BIT.PFSWE = 0;
	MPC.PWPR.BIT.B0WI = 1;
}

void Excep_ETHER_EINT(void) 
{  
	uint32_t status;
	status = EDMAC.EESR.LONG;
	if(status & EMAC_FR_INT) {
	} 
	if(status & EMAC_TC_INT) {
		EDMAC.EESR.BIT.TC = 1; /* Clear the transmission complete bit */
	}
}

void 
RXEth_Transmit(uint8_t *buf,uint16_t len)
{

}

void
RX_EtherInit()
{
	volatile uint32_t i;
	RX_EtherSetupIoPortsMII();
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
