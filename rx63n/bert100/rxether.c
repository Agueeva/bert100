/*
 * Renesas RX Ethernet controller driver
 */

#include <string.h>
#include "iodefine.h"
#include "types.h"
#include "ethdrv.h"
#include "rxether.h"
#include "phy.h"
#include "interrupt_handlers.h"
#include "interpreter.h"
#include "hex.h"
#include "console.h"
#include "timer.h"
#include "events.h"
#include "skb.h"
#include "tpos.h"

#define RX_DESCR_NUM	(4U)
#define TX_DESCR_NUM	(2U)

#define EMAC_NUM_RX_BUFS	RX_DESCR_NUM
#define EMAC_RX_BUFSIZE		(1536) /* Aligned to multiples of 32 Bytes, not necessary ? */

#define RX_DESCR_WP(rxeth)	((rxeth)->rxDescrWp % RX_DESCR_NUM)
#define RX_DESCR_RP(rxeth)	((rxeth)->rxDescrRp % RX_DESCR_NUM)
#define TX_DESCR_WP(rxeth)	((rxeth)->txDescrWp % TX_DESCR_NUM)
#define TX_DESCR_RP(rxeth)	((rxeth)->txDescrRp % TX_DESCR_NUM)
#define RX_DESCR_INC_RP(rxeth)	((rxeth)->rxDescrRp = ((rxeth)->rxDescrRp + 1) % (RX_DESCR_NUM << 1))
//#define RX_DESCR_INC_WP(rxeth)	((rxeth)->rxDescrWp = ((rxeth)->rxDescrWp + 1) % (RX_DESCR_NUM << 1))
#define TX_DESCR_INC_RP(rxeth)	((rxeth)->txDescrRp = ((rxeth)->txDescrRp + 1) % (TX_DESCR_NUM << 1))
#define TX_DESCR_INC_WP(rxeth)	((rxeth)->txDescrWp = ((rxeth)->txDescrWp + 1) % (TX_DESCR_NUM << 1))

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

#define TFS_DTA		(UINT32_C(1) << 8) 	/* Detect Transmission Abort	*/
#define TFS_DNC		(UINT32_C(1) << 3)	/* Detect No Carrier		*/	
#define TFS_DLC		(UINT32_C(1) << 2)	/* Detect Loss of Carrier	*/
#define TFS_DDC		(UINT32_C(1) << 1)	/* Detect Delayed Collision	*/
#define	TFS_TRO		(UINT32_C(1) << 0)	/* Transmit Retry Over */

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

#define RFS_OVL		(UINT32_C(1) << 9)
#define RFS_DRA		(UINT32_C(1) << 8)
#define RFS_RMAF	(UINT32_C(1) << 7)
#define	RFS_RRF		(UINT32_C(1) << 4)
#define RFS_RTLF	(UINT32_C(1) << 3)
#define	RFS_RTSF	(UINT32_C(1) << 2)
#define RFS_PRE		(UINT32_C(1) << 1)
#define RFS_CERF	(UINT32_C(1) << 0)

typedef struct Descriptor {
	volatile uint32_t status;
	volatile uint16_t size;	  /**< Only relevant in receive descriptor */
	volatile uint16_t bufsize; /**< TX: number of bytes to transmit, RX: Maxium bytes to receive */
	uint8_t  *bufP;
	Skb *skb;	  /**< not used by hardware, used for skb here */
} Descriptor;

typedef struct RxEth {
	EthDriver ethDrv;
	Descriptor txDescr[TX_DESCR_NUM] __attribute__((aligned (16))); 
	Descriptor rxDescr[RX_DESCR_NUM] __attribute__((aligned (16)));
	Skb	rxSkb;
	EthDrv_PktSinkProc *pktSinkProc;
	void *pktSinkData;
	Timer   rxWatchdogTimer;
	uint8_t rxBuf[EMAC_NUM_RX_BUFS * EMAC_RX_BUFSIZE];
	uint8_t ethMAC[6];
	uint16_t txDescrWp;
	uint16_t txDescrRp;
	uint16_t rxDescrRp;
	bool evRxInUse;
	bool evTxInUse;
	Event    evRxEvent;
	Event    evTxEvent;
	CSema	 cSemaTxDescr;
	uint32_t statTxInts;
	uint32_t statRxInts;
	uint32_t statRxBadFrame;
	uint32_t statRxFrameOk;
	uint32_t statRRRClear;
	uint32_t statRxWatchdog;
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
}

__attribute__((unused)) static void
RX_EtherSetupIoPortsMII(void)
{
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
}
/**
 ***********************************************************
 * Version for 100 Pin variant of the CPU using port A/B
 ***********************************************************
 */
__attribute__((unused)) static void
RX_EtherSetupIoPorts100_RMII(void)
{
	MPC.PFENET.BIT.PHYMODE = 0; /* RMII mode */
	MPC.PA3PFS.BYTE = 0x11; /* PA3 MDIO	*/
	MPC.PA4PFS.BYTE = 0x11; /* PA4 MDC	*/
	PORTA.PMR.BYTE |= 0x18;
	PORTA.PDR.BYTE |= 0x18;
	MPC.PB0PFS.BYTE = 0x12;	/* PB0 RMII_RXD1 */ 
	MPC.PB1PFS.BYTE = 0x12; /* PB1 RMII_RXD0 */
	MPC.PB2PFS.BYTE = 0x12; /* PB2 RMII_REF50CK */
	MPC.PB3PFS.BYTE = 0x12; /* PB3 RMII_RX_ER input */
	MPC.PB4PFS.BYTE = 0x12;	/* PB4 RMII_TXD_EN */
	MPC.PB5PFS.BYTE = 0x12; /* PB5 RMII_TXD0 */
	MPC.PB6PFS.BYTE = 0x12; /* PB6 RMII_TXD1 */
	MPC.PB7PFS.BYTE = 0x12; /* PB7_RMII_CRS input */
	PORTB.PMR.BYTE = 0xff;
	PORTB.PDR.BYTE = 0x70; 
}

/**
 ************************************************************************
 * \fn void Excep_ETHER_EINT(void) 
 ************************************************************************
 */

void 
Excep_ETHER_EINT(void) 
{  
	RxEth *re = &gRxEth;
	uint32_t status;
	status = EDMAC.EESR.LONG;
	EDMAC.EESR.LONG = status; /* Ack all pending interrupts */
	if(status & EMAC_FR_INT) {
		re->statRxInts++;
		EV_Trigger(&re->evRxEvent);
	} 
	if(status & EMAC_TC_INT) {
		EV_Trigger(&re->evTxEvent);
		re->statTxInts++;
	}
}

static void
RXEth_RxEventProc(void *eventData)
{
	RxEth *re = eventData;
	uint16_t rp;
	uint32_t status;
	volatile Descriptor *rxDescr;
	Skb *skb;
	re->evRxInUse = 1;
	do { 
		rp = RX_DESCR_RP(re);
		rxDescr = &re->rxDescr[rp];
		status = rxDescr->status;
		if(status & RXDS_ACT) {
			break;
		}
		/* Check for errors */
		if(status & RXDS_FE) {
			re->statRxBadFrame++;
		} else {
			//Con_Printf("Received a frame\n");
			re->statRxFrameOk++;
			skb = &re->rxSkb;
			skb->hdrBuf = NULL;
			skb->dataStart = skb->dataBuf = skb->hdrStart = skb->hdrEnd = rxDescr->bufP;
			skb->dataLen = rxDescr->size;
        		skb->hdrAvailLen = 0;
        		skb->dataAvailLen =  EMAC_RX_BUFSIZE;
			if(re->pktSinkProc) {
				re->evRxInUse = 2;
				re->pktSinkProc(re->pktSinkData,skb);
				re->evRxInUse = 1;
			}
		}
		rxDescr->status = RXDS_ACT | (status & RXDS_DLE);
		RX_DESCR_INC_RP(re);
		/* 
 		 ****************************************************
		 * Restart receiver if necessary 
		 * Should not be necessary if RNC is set
 		 ****************************************************
		 */ 
		if(EDMAC.EDRRR.LONG == 0) {
			re->statRRRClear++;
			EDMAC.EDRRR.LONG = 1;
		}
	} while(1);
	re->evRxInUse = 0;
}

static void
RXEth_TxEventProc(void *eventData)
{
	RxEth *re = eventData;
	volatile Descriptor *txDescr;
	re->evTxInUse = 1;
	do {
		txDescr = &re->txDescr[TX_DESCR_RP(re)];
		if((txDescr->status & TXDS_ACT)) {
			break;
		} 
		if((txDescr->skb == NULL)) {
			break;
		} 
		Skb_Free(txDescr->skb);
		txDescr->skb = NULL;
		TX_DESCR_INC_RP(re);
		//Con_Printf("Up\n");
		CSema_Up(&re->cSemaTxDescr);
	} while(1);
	re->evTxInUse = 0;
}

/**
 ****************************************************************************
 * \fn static void RXEth_Transmit(void *driverData,uint8_t *buf,uint16_t len)
 * Transmit a paket. This is the interface to the outside.
 ****************************************************************************
 */
static void 
RXEth_Transmit(EthDriver *drv,Skb *skb)
{
	RxEth *re = container_of(drv,RxEth,ethDrv); 
	volatile Descriptor *txDescr;
	CSema_Down(&re->cSemaTxDescr);
	txDescr = &re->txDescr[TX_DESCR_WP(re)];
	txDescr->bufP = (uint8_t *)skb->hdrStart;
	txDescr->bufsize = skb->hdrEnd - skb->hdrStart + skb->dataLen;
	if(txDescr->bufsize < 64)  {
		txDescr->bufsize = 64;
	}
	//Con_Printf("Bufsize %u\n",txDescr->bufsize);
	txDescr->skb = skb;
	barrier();
	txDescr->status |= (TXDS_FP1 | TXDS_FP0 | TXDS_ACT); /* activate Single buffer frame */
	TX_DESCR_INC_WP(re);
	if(EDMAC.EDTRR.LONG == 0) {
		EDMAC.EDTRR.LONG = 1;
	}
}

/**
 **********************************************************************
 * \fn static void RXEth_SetMAC(RxEth *re,const uint8_t *mac)
 * Set the MAC address of the ethernet Interface.
 **********************************************************************
 */
static void
RXEth_SetMAC(RxEth *re,const uint8_t *mac)
{
	int i;
	for(i = 0; i < 6; i++) {
		re->ethMAC[i] = mac[i];
	}
	ETHERC.MAHR = ((uint32_t)mac[0] << 24) | ((uint32_t)mac[1] << 16) 
		      | ((uint32_t)mac[2] << 8) | mac[3];
	ETHERC.MALR.LONG = ((uint32_t)mac[4] << 8) | mac[5];
}

/**
 *********************************************************************
 * \fn static void RXEth_Control(void driverData,EthControlCmd *ctrl)
 *********************************************************************
 */
static void
RXEth_Control(void *driverData,EthControlCmd *ctrl)
{
	RxEth *re = driverData; 
	uint8_t *mac;
	switch(ctrl->cmd) {
		case ETHCTL_SET_MAC: 
			mac = ctrl->cmdArg;	
			RXEth_SetMAC(re,mac);
			break;
				
		case ETHCTL_GET_MAC: 
			mac = ctrl->cmdArg;
			memcpy(mac,re->ethMAC,6);
			break;
	}
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
	volatile Descriptor *descr;
	CSema_Init(&re->cSemaTxDescr);
	for(i = 0; i < TX_DESCR_NUM; i++) {
		descr = &re->txDescr[i];	
		descr->status = 0;
		descr->skb = NULL;
		CSema_Up(&re->cSemaTxDescr);
	}
	descr->status |= TXDS_DLE; /* Mark as last Descriptor in the ring */
	
	for(i = 0; i < RX_DESCR_NUM; i++) {
		descr = &re->rxDescr[i];
		descr->bufP = re->rxBuf + i * EMAC_RX_BUFSIZE; 
		descr->bufsize = EMAC_RX_BUFSIZE;
		descr->size = 0;
		descr->status = RXDS_ACT;
		descr->skb = NULL;
	}
	descr->status |= RXDS_DLE; /* Mark as last descriptor in the ring */
	
}

void 
RxEth_RegisterPktSink(EthDriver *drv,EthDrv_PktSinkProc *p,void *evData)
{
	RxEth *re = container_of(drv,RxEth,ethDrv);
	re->pktSinkProc = p;
	re->pktSinkData = evData;
}

static void
RxEth_RxWatchdogTimerProc(void *eventData) 
{
	RxEth *re = eventData;
	bool ok = false;
	int i;
	Timer_Start(&re->rxWatchdogTimer,1100);
#if 1
	if(EDMAC.EDRRR.LONG == 1) {
		return;
	}
#endif
	for(i = 0; i < RX_DESCR_NUM; i++) {
		uint32_t status;
		volatile Descriptor *rxDescr = &re->rxDescr[i];
		status = rxDescr->status;
		if(status & RXDS_ACT) {
			ok = true;
		}
	}
	if(ok == false) {
		re->statRxWatchdog++;
	}
	EV_Trigger(&re->evRxEvent);
}
/**
 ***********************************************************************************
 * \fn static int8_t cmd_ethtx(Interp * interp, uint8_t argc, char *argv[])
 * Shell interface for testing the ethernet transmission
 ***********************************************************************************
 */
static int8_t
cmd_ethstat(Interp * interp, uint8_t argc, char *argv[])
{
	RxEth *re = &gRxEth;
	uint16_t i;
	for(i = 0; i < RX_DESCR_NUM; i++) {
		uint32_t status;
		volatile Descriptor *rxDescr = &re->rxDescr[i];
		status = rxDescr->status;
		Con_Printf("RxDescr %u status 0x%08lx\n",i,status);
	}
	Con_Printf("TX Ints:  %lu\n",re->statTxInts);
	Con_Printf("RX Ints:  %lu\n",re->statRxInts);
	Con_Printf("RxGood:   %lu\n",re->statRxFrameOk);
	Con_Printf("RxBad:    %lu\n",re->statRxBadFrame);
	Con_Printf("RRRClear: %lu\n",re->statRRRClear);
	Con_Printf("EDRRR:    %lu\n",EDMAC.EDRRR.LONG);
	Con_Printf("RxWdg:    %lu\n",re->statRxWatchdog);
	Con_Printf("evRxInUse %u\n",re->evRxInUse);
	Con_Printf("evTxInUse %u\n",re->evTxInUse);
	if(argc > 1) {
		EDMAC.EDRRR.LONG = 1;
	}

	return 0;
}

INTERP_CMD(ethstatCmd, "ethstat", cmd_ethstat, "ethstat      # Ethernet driver statistics");

EthDriver *
RX_EtherInit(void)
{
	/* This is a locally assigned unicast address */
	const uint8_t mac[6] = { 0x12,0x34,0x56,0x78,0xab,0xcd };
	RxEth *re = &gRxEth;
	EthDriver *ethDrv;
	ethDrv = &re->ethDrv;

	EV_Init(&re->evRxEvent, RXEth_RxEventProc, re);
	EV_Init(&re->evTxEvent, RXEth_TxEventProc, re);
//	RX_EtherSetupIoPortsMII();
	RX_EtherSetupIoPorts100_RMII();
	MSTP(EDMAC) = 0;
	DelayUs(2);
	EDMAC.EDMR.BIT.SWR = 1;
	DelayUs(2); /* The documentation says 64 bus clock cycles */ 
	RXEth_InitDescriptors(re);
	RXEth_SetMAC(re,mac);
	
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
	EDMAC.TRSCER.BIT.RMAFCE = 1;	/* Multicast is not an error */
	EDMAC.TFTR.LONG = 0;		/* Store and forward mode */
	EDMAC.FDR.LONG = 5;		/* Fifo 1536 bytes */
	EDMAC.RMCR.LONG = 0;		/* Should not be necessary because cleared by reset */
	EDMAC.RMCR.BIT.RNR = 1;
	EDMAC.FCFTR.LONG = 0x00070005; 
	/* Now start it */
	ETHERC.ECMR.BIT.DM = 1;		/* Full duplex */
	ETHERC.ECMR.BIT.RTM = 1; 	/* 100 MBit */
	ETHERC.APR.LONG = 1;
	ETHERC.TPAUSER.LONG = 0;
	ETHERC.ECMR.BIT.TXF = 0; /* TXPause */
	ETHERC.ECMR.BIT.RXF = 0; /* RXPause */
	ETHERC.ECMR.BIT.RE = 1;	
	ETHERC.ECMR.BIT.TE = 1;	
	EDMAC.EDRRR.LONG = 1;	 /* Enable receive */

	IPR(ETHER,EINT) = 2;
	IEN(ETHER,EINT) = 1;
	Phy_Init();
	ethDrv->txProc = RXEth_Transmit;
	ethDrv->ctrlProc = RXEth_Control; 
	ethDrv->regPktSink = RxEth_RegisterPktSink;
	Timer_Init(&re->rxWatchdogTimer,RxEth_RxWatchdogTimerProc,re);
	Timer_Start(&re->rxWatchdogTimer,2000);
	Interp_RegisterCmd(&ethstatCmd);
	return ethDrv;
}
