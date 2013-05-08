/*
 *****************************************************************************
 * Uart SCI0
 *****************************************************************************
 */
#include <string.h>
#include <stdlib.h>
#include "iodefine.h"
#include "events.h"
#include "types.h"
#include "sci0.h"
#include "sci.h"
#include "atomic.h"
#include "config.h"
#include "interpreter.h"
#include "interrupt_handlers.h"
#include "console.h"

#define SCI0_IPL 2
#define RXFIFO_SIZE	(16)
#define RXFIFO_RP(uart) ((uart)->rxbuf_rp & (RXFIFO_SIZE-1))	/**< The fifo readpointer.  */
#define RXFIFO_WP(uart) ((uart)->rxbuf_wp & (RXFIFO_SIZE-1))	/**< The fifo writepointer. */
#define TXFIFO_SIZE	(16)
#define TXFIFO_RP(uart) ((uart)->txbuf_rp & (TXFIFO_SIZE-1))
#define TXFIFO_WP(uart) ((uart)->txbuf_wp & (TXFIFO_SIZE-1))
#define TXFIFO_CNT(uart) ((uart)->txbuf_wp - (uart)->txbuf_rp)

typedef struct Sci0 {
	Sci0SinkProc *dataSinkProc; /**< The data sink for received data. */
	void *sinkData;				/**< Pointer used by the data sink procedure. */
	uint16_t rxbuf[RXFIFO_SIZE]; /**< Receive fifo, filled by the interrupthandler, read by an eventhandler. */
	uint16_t rxbuf_rp;	    /**< Readpointer for the RX-Fifo used by the RX-Event handler. */
	uint16_t rxbuf_wp;	    /**< Writepointer for the RX-Fifo. */
	uint16_t txbuf[TXFIFO_SIZE];
	uint16_t txbuf_wp;
	uint16_t txbuf_rp;
	uint8_t tx_idle;
	bool nineBit;
	uint8_t rx_bits;
	uint8_t tx_bits;
	uint8_t rx_stopbits;
	uint8_t tx_stopbits;
	uint8_t tx_teie;
	uint32_t baud;
	uint32_t stat_txints;
	uint32_t stat_rxints;
	uint32_t stat_errints;
	uint32_t stat_ferints;
	uint32_t stat_overints;
	uint32_t stat_perints;
} Sci0;

static Sci0 g_uart;

/**
 **********************************************************************************
 * \fn void Sci0_RegisterSink(Sci0SinkProc *proc,void *sinkData);
 **********************************************************************************
 */
void
Sci0_RegisterSink(Sci0SinkProc * proc, void *sinkData)
{
	g_uart.sinkData = sinkData;
	g_uart.dataSinkProc = proc;
}

/**
 *********************************************************************
 * \fn static void uart2_data_sink(void *eventData);
 *********************************************************************
 */
static void
sci0_data_sink(void *eventData)
{
	Sci0 *sci0 = (Sci0 *) eventData;
	uint16_t rxchar;
	while (sci0->rxbuf_rp != sci0->rxbuf_wp) {
		rxchar = sci0->rxbuf[RXFIFO_RP(sci0)];
		sci0->rxbuf_rp++;
		if (sci0->dataSinkProc) {
			sci0->dataSinkProc(sci0->sinkData, rxchar);
		}
	}
}

static EVENT_DECLARE(e_sci0_data_avail, sci0_data_sink, &g_uart);

/**
 *****************************************************************
 * \fn void Sci0_TxIrq(void)
 *****************************************************************
 */
void
Excep_SCI0_TXI0(void)
{
	Sci0 *ua = &g_uart;
	uint16_t c;
	if (ua->txbuf_rp != ua->txbuf_wp) {
		c = ua->txbuf[TXFIFO_RP(ua)];
		if (ua->nineBit) {
			SCI0.SSR.BIT.MPBT = ! !(c & 0x100);
		}
		SCI0.TDR = c;
		ua->txbuf_rp++;
	} else {
		ua->tx_idle = 1;
	}
	ua->stat_txints++;
}

/**
 ******************************************************
 * \fn void Sci0_TransmitChar(uint8_t c);
 ******************************************************
 */
void
Sci0_TransmitChar(uint8_t c)
{
	uint32_t flags;
	Sci0 *ua = &g_uart;
	SAVE_FLAGS_SET_IPL(flags, SCI0_IPL);
	while (TXFIFO_CNT(ua) == TXFIFO_SIZE) {
		RESTORE_FLAGS(flags);
		EV_Yield();
		SAVE_FLAGS_SET_IPL(flags, SCI0_IPL);
	}
	ua->txbuf[TXFIFO_WP(ua)] = c;
	ua->txbuf_wp++;
	if (ua->tx_idle) {
		ua->tx_idle = 0;
		if (ua->nineBit) {
			SCI0.SSR.BIT.MPBT = ! !(c & 0x100);
		}
		SCI0.TDR = c;
		ua->txbuf_rp++;
	}
	RESTORE_FLAGS(flags)
}

#define rx_avail(x) EV_Trigger(&e_sci0_data_avail);
/**
 ************************************************************************
 * \fn void Uart2_RxIrq(void)
 ************************************************************************
 */

void
Excep_SCI0_RXI0(void)
{
	uint16_t rxchar;
	if (g_uart.nineBit) {
		rxchar = ((uint16_t) (SCI0.SSR.BIT.MPB)) << 8;
		rxchar |= (uint16_t) (SCI0.RDR);
	} else {
		rxchar = (uint16_t) (SCI0.RDR);
	}
	g_uart.rxbuf[RXFIFO_WP(&g_uart)] = rxchar;
	g_uart.stat_rxints++;
	g_uart.rxbuf_wp++;
	rx_avail(&g_uart);
}
#if 0
/**
 *********************************************************************
 * The receiver seems to be stopped until errors are acknowledged.
 *********************************************************************
 */
void
Excep_SCI0_ERI0(void)
{
	g_uart.stat_errints++;
	if (SCI0.SSR.BIT.ORER) {
		SCI0.SSR.BIT.ORER = 0;
		g_uart.stat_overints++;
	}
	if (SCI0.SSR.BIT.PER) {
		SCI0.SSR.BIT.PER = 0;
		g_uart.stat_perints++;
	}
	if (SCI0.SSR.BIT.FER) {
		g_uart.stat_ferints++;
		g_uart.rxbuf[RXFIFO_WP(&g_uart)] = (uint16_t) (SCI0.RDR) | SCI0_FER;
		g_uart.rxbuf_wp++;
		g_uart.stat_ferints++;
		SCI0.SSR.BIT.FER = 0;
		rx_avail(&g_uart);
	}
}
#endif
/**
 ****************************************************************************
 * \fn static int8_t cmd_sci0(Interp * interp, uint8_t argc, char *argv[])
 ****************************************************************************
 */
static int8_t
cmd_sci0(Interp * interp, uint8_t argc, char *argv[])
{
	Sci0 *sci0 = &g_uart;
	unsigned int i;
	if (argc == 1) {
		Interp_Printf_P(interp, "RX  Ints %lu\n", sci0->stat_rxints);
		Interp_Printf_P(interp, "TX  Ints %lu\n", sci0->stat_txints);
		Interp_Printf_P(interp, "ERR Ints %lu\n", sci0->stat_errints);
		Interp_Printf_P(interp, "FER Ints %lu\n", sci0->stat_ferints);
		Interp_Printf_P(interp, "OR  Ints %lu\n", sci0->stat_overints);
		Interp_Printf_P(interp, "PER Ints %lu\n", sci0->stat_perints);
		Interp_Printf_P(interp, "SSR 0x%02x\n", SCI0.SSR.BYTE);
		Interp_Printf_P(interp, "SCR 0x%02x\n", SCI0.SCR.BYTE);
		Interp_Printf_P(interp, "Baud %lu\n", sci0->baud);
		Interp_Printf_P(interp, "tx_teie %u\n", sci0->tx_teie);
		Interp_Printf_P(interp, "rx_bits %u\n", sci0->rx_bits);
		Interp_Printf_P(interp, "tx_bits %u\n", sci0->tx_bits);
		Interp_Printf_P(interp, "rx_stopbits %u\n", sci0->rx_stopbits);
		Interp_Printf_P(interp, "tx_stopbits %u\n", sci0->tx_stopbits);
	} else {
		for (i = 0; i < strlen(argv[1]); i++) {
			Sci0_TransmitChar(argv[1][i]);
		}
		Sci0_TransmitChar('\r');
		Sci0_TransmitChar('\n');
	}
	return 0;
}

INTERP_CMD(sci0Cmd, "sci0", cmd_sci0, "sci0 ?<String>? # Get status of SCI0 / Send Teststring");

void
Sci0_Flush(void)
{
	Sci0 *ua = &g_uart;
	while (!ua->tx_idle) {
		EV_Yield();
	}
	while (SCI0.SSR.BIT.TEND == 0) {
		EV_Yield();
	}
}

/**
 **********************************************************************************************
 * Configure the SCI0 interface.
 **********************************************************************************************
 */
void
Sci0_Configure(uint32_t baud, uint8_t rx_bits, uint8_t tx_bits, uint8_t parity, uint8_t rx_stopbits,
	       uint8_t tx_stopbits)
{
	Sci0 *ua = &g_uart;
	Sci0_Flush();
	/* RE and TE can only be cleared together */
	SCI0.SCR.BYTE = 0;
	if (baud >= 57600) {
		SCI0.SEMR.BIT.ABCS = 1;
		SCI0.BRR = (F_PCLK / 16 / baud) - 1;
	} else if (baud >= 9600) {
		SCI0.SEMR.BIT.ABCS = 0;
		SCI0.BRR = (F_PCLK / 32 / baud) - 1;
	} else {
		Con_Printf("SCI0: Baudrate below 9600 not implemented: %ld\n", baud);
	}
	if (rx_bits == 9) {
		SCI0.SMR.BIT.MP = 1;
		SCI0.SMR.BIT.CHR = 0;
		ua->nineBit = true;
	} else if (rx_bits == 8) {
		SCI0.SMR.BIT.MP = 0;
		SCI0.SMR.BIT.CHR = 0;
		ua->nineBit = false;
	} else if (rx_bits == 7) {
		SCI0.SMR.BIT.MP = 0;
		SCI0.SMR.BIT.CHR = 1;
		ua->nineBit = false;
	} else {
		Con_Printf("SCI0 bad bits %d\n", rx_bits);
	}
	if (rx_stopbits == 2) {
		SCI0.SMR.BIT.STOP = 1;
	} else if (rx_stopbits == 1) {
		SCI0.SMR.BIT.STOP = 0;
	} else {
		Con_Printf("SCI0 bad stopbits %d\n", rx_stopbits);
	}
	if (parity == PAR_NONE) {
		SCI0.SMR.BIT.PE = 0;
	} else if (parity == PAR_EVEN) {
		SCI0.SMR.BIT.PE = 1;
		SCI0.SMR.BIT.PM = 0;
	} else if (parity == PAR_ODD) {
		SCI0.SMR.BIT.PE = 1;
		SCI0.SMR.BIT.PM = 1;
	} else {
		Con_Printf("SCI0 bad parity %d\n", parity);
	}
	ua->tx_idle = 1;
	if (rx_bits != tx_bits) {
		/* not implemented */
		//ua->tx_teie = 1;
		ua->tx_teie = 0;
	} else {
		ua->tx_teie = 0;
	}
	ua->rx_bits = rx_bits;
	ua->tx_bits = tx_bits;
	ua->rx_stopbits = rx_stopbits;
	ua->tx_stopbits = tx_stopbits;
	ua->baud = baud;
	/* RE and TE can only be set together */
	if (ua->tx_teie) {
		SCI0.SCR.BYTE = 0x70;
	} else {
		SCI0.SCR.BYTE = 0xf0;
	}
}

/**
 *********************************************************************************
 * \fn void Sci_Init(void)
 *********************************************************************************
 */
void
Sci0_Init(uint32_t baudrate)
{
	Sci0 *ua = &g_uart;
	MSTP(SCI0) = 0;
	SCI0.SCR.BIT.TIE = 0;
	SCI0.SCR.BIT.RIE = 0;
	SCI0.SCR.BIT.RE = 0;
	SCI0.SCR.BIT.TE = 0;


	
	MPC.P21PFS.BIT.PSEL = 0x0a;
	MPC.P20PFS.BIT.PSEL = 0x0a;
	BSET(0,PORT2.PDR.BYTE);
	BSET(1,PORT2.PDR.BYTE);
	PORT2.PMR.BYTE |= 3;
	//BSET(1, PORT2.ICR.BYTE);

	if (baudrate >= 57600) {
		SCI0.SEMR.BIT.ABCS = 1;
		SCI0.BRR = (F_PCLK / 16 / baudrate) - 1;
	} else if (baudrate >= 9600) {
		SCI0.SEMR.BIT.ABCS = 0;
		SCI0.BRR = (F_PCLK / 32 / baudrate) - 1;
	} else {

	}
	SCI0.SMR.BIT.CKS = 0;
	SCI0.SMR.BIT.STOP = 0;
	SCI0.SMR.BIT.PE = 0;
	SCI0.SMR.BIT.PM = 0;
	SCI0.SMR.BIT.CHR = 0;
	SCI0.SMR.BIT.CM = 0;
	SCI0.SCMR.BYTE = 0xf2; /* Serial */

	IPR(SCI0, TXI0) = SCI0_IPL;
	IEN(SCI0, TXI0) = 1;
	IPR(SCI0, RXI0) = SCI0_IPL;
	IEN(SCI0, RXI0) = 1;
	IR(SCI0,RXI0) = 0x0;
	SCI0.SCR.BYTE = 0xf0;
	
	ua->tx_idle = 1;
	Interp_RegisterCmd(&sci0Cmd);
}
