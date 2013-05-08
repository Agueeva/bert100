/*
 * Renesas RX Ethernet controller driver
 */

#include <string.h>
#include "iodefine.h"
#include "types.h"
#include "rxether.h"
#include "phy.h"
#include "interrupt_handlers.h"
#include "interpreter.h"
#include "hex.h"

#define RX_DESCR_NUM	(2U)
#define TX_DESCR_NUM	(4U)

#define EMAC_NUM_RX_BUFS	RX_DESCR_NUM
#define EMAC_RX_BUFSIZE		(1536) /* Aligned to multiples of 32 Bytes, not necessary ? */

#define RX_DESCR_WP(rxeth)	((rxeth)->rxDescrWp % RX_DESCR_NUM)
#define RX_DESCR_RP(rxeth)	((rxeth)->rxDescrRp % RX_DESCR_NUM)
#define TX_DESCR_WP(rxeth)	((rxeth)->txDescrWp % TX_DESCR_NUM)
#define TX_DESCR_RP(rxeth)	((rxeth)->txDescrRp % TX_DESCR_NUM)

#define EMAC_TC_INT	(UINT32_C(1) << 21)
#define EMAC_FR_INT	(UINT32_C(1) << 18)

/**
 ************************************************************************
 * Status bits in the transmit descriptor
 ************************************************************************
 */
#define TXDS_ACT	(UINT32_C(1) << 31)
#define TXDS_DLE	(UINT32_C(1) << 30)
#define TXDS_FP1	(UINT32_C(1) << 29)
#define TXDS_FP0	(UINT32_C(1) << 28)
#define TXDS_FE		(UINT32_C(1) << 27)
#define TXDS_WBI	(UINT32_C(1) << 26)

/*
 *************************************************************************
 * Status bits in the receive descriptor
 *************************************************************************
 */
#define RXDS_ACT	(UINT32_C(1) << 31)
#define	RXDS_DLE	(UINT32_C(1) << 30)
#define RXDS_FP1	(UINT32_C(1) << 29)
#define RXDS_FP0	(UINT32_C(1) << 28)
#define RXDS_FE		(UINT32_C(1) << 27)

typedef struct Descriptor {
	uint32_t status;
	uint16_t size;	  /**< Only relevantin receive descriptor */
	uint16_t bufsize; /**< TX: number of bytes to transmit, RX: Maxium bytes to receive */
	uint8_t  *bufP;
	uint32_t padding;
} Descriptor;

typedef struct RxEth {
	Descriptor txDescr[TX_DESCR_NUM] __attribute__((aligned (16))); 
	Descriptor rxDescr[RX_DESCR_NUM] __attribute__((aligned (16)));
	uint8_t rxBuf[EMAC_NUM_RX_BUFS * EMAC_RX_BUFSIZE];
	uint16_t txDescrWp;
	uint16_t txDescrRp;
	uint16_t rxDescrWp;
	uint16_t rxDescrRp;
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
		Con_Printf("TCINT\n");
	}
}

void 
RXEth_Transmit(uint8_t *buf,uint16_t len)
{
	RxEth *re = &gRxEth;
	Descriptor *txDescr = &re->txDescr[TX_DESCR_WP(re)];
	txDescr->bufP = buf;
	txDescr->bufsize = len;
	txDescr->status |= (TXDS_FP1 | TXDS_FP0 | TXDS_ACT); /* activate Single buffer frame */
	re->txDescrWp++;
	if(EDMAC.EDTRR.LONG = 0) {
		EDMAC.EDTRR.LONG = 1;
	}
	SleepMs(200);
}

/**
 ***************************************************************************
 * Initialize the DMA descriptors.
 ***************************************************************************
 */
static void
RXEth_InitDescriptors(RxEth *re)
{
	int i;
	Descriptor *descr;
	for(i = 0; i < TX_DESCR_NUM; i++) {
		descr = &re->txDescr[i];	
		descr->status = 0;
	}
	descr->status |= TXDS_DLE; /* Mark as last Descriptor in the ring */
	
	for(i = 0; i < RX_DESCR_NUM; i++) {
		descr = &re->rxDescr[i];
		descr->bufsize = EMAC_RX_BUFSIZE;
		descr->size = 0;
		descr->status = RXDS_ACT;
	}
	descr->status = RXDS_DLE; /* Mark as last descriptor in the ring */
	
}
/**
 ***********************************************************************************
 * \fn static int8_t cmd_ethtx(Interp * interp, uint8_t argc, char *argv[])
 * Shell interface for testing the ethernet transmission
 ***********************************************************************************
 */
static int8_t
cmd_ethtx(Interp * interp, uint8_t argc, char *argv[])
{
	uint16_t i;
	uint8_t pkt[64] __attribute__((aligned (32)));
	memset(pkt,0,sizeof(pkt));
	for(i = 1; i < argc && (i <= array_size(pkt)); i++) {
		pkt[i - 1] = astrtoi16(argv[i]);
	}
	RXEth_Transmit(pkt,64);
	return 0;
}

INTERP_CMD(ethtxCmd, "ethtx", cmd_ethtx, "ethtx      # Raw ethernet transmit test command");

void
RX_EtherInit()
{
	volatile uint32_t i;
	RxEth *re = &gRxEth;
	RX_EtherSetupIoPortsMII();
	MSTP(EDMAC) = 0;
	for(i = 0; i < 1000; i++) {

	}
	EDMAC.EDMR.BIT.SWR = 1;
	for(i = 0; i < 1000; i++) {

	}
	RXEth_InitDescriptors(re);
	/* This is a locally assigned unicast address */
	ETHERC.MAHR = 0x12345678;
	ETHERC.MALR.LONG = 0x9abc;
	
	ETHERC.ECSR.LONG = 0x37;	/* Clear all status bits */
	EDMAC.EESIPR.BIT.TCIP = 1; 	/* Enable the transmission Complete interrupt */
	EDMAC.EESIPR.BIT.FRIP = 1;	/* Enable the Frame reception interrupt */
	ETHERC.RFLR.LONG = 1522;	/* Standard Ethernet MAXPS + 4 Byte VLAN Header */
	ETHERC.IPGR.LONG = 0x14;	
	EDMAC.EESR.LONG = 0x47ff0f9f;	/* Clear all status bits, should not be necessary, cleared by reset */
	EDMAC.EDMR.BIT.DE = 1;		/* Little endian DMA for descriptors */	
	EDMAC.RDLAR = &re->rxDescr[0]; 
	EDMAC.TDLAR = &re->txDescr[0];
	EDMAC.TRSCER.LONG = 0;		/* Should not be necessary because cleared by reset */
	EDMAC.TFTR.LONG = 0;		/* Store and forward mode */
	EDMAC.FDR.LONG = 5;		/* Fifo 1536 bytes */
	EDMAC.RMCR.LONG = 0;		/* Should not be necessary because cleared by reset */
	EDMAC.FCFTR.LONG = 0x00070005; 
	IPR(ETHER,EINT) = 2;
	IEN(ETHER,EINT) = 1;
	Phy_Init();
	Interp_RegisterCmd(&ethtxCmd);
}
