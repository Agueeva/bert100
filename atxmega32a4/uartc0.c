/**
 ***************************************************************************************
 * \file uartc0.c
 * Device driver for th Uart on Port C of the ATxmega32A4 
 ***************************************************************************************
 */
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "uartc0.h"
#include "events.h"

#define RXFIFO_SIZE (32)        				/**< The fifo size for the receiver. */
#define RXFIFO_RP(uart) ((uart)->rxbuf_rp & (RXFIFO_SIZE-1))    /**< The fifo readpointer. */
#define RXFIFO_WP(uart) ((uart)->rxbuf_wp & (RXFIFO_SIZE-1))    /**< The fifo writepointer. */

/**
 *******************************************
 * \typedef Uart
 * The ATXMega Uart structure.
 *******************************************
 */
typedef struct UartC0 {
        UartC0SinkProc *sinkProc;   /**< The data sink for received data. */
        void *sinkData;		    /**< Pointer used by the data sink procedure. */
        uint8_t rxbuf[RXFIFO_SIZE]; /**< Receive fifo, filled by the interrupthandler, read by an eventhandler. */
        uint8_t rxbuf_rp;           /**< Readpointer for the RX-Fifo used by the RX-Event handler. */
        uint8_t rxbuf_wp;           /**< Writepointer for the RX-Fifo. */
        uint8_t enabled;            /**< Enabled flag for the UART. Used to only enable UART in debug mode. */
} UartC0;

static UartC0 g_uartc0;

/**
 ***************************************************************
 * \fn static void usartc0_sink_data(void *clientData)
 *      Forward the data received in the interrupt handler
 *      to the consumer. This is an eventhandler procedure.
 ***************************************************************
 */

static void 
uartc0_sink_data(void *clientData)
{
        UartC0 *uart = (UartC0 *) clientData;
        uint8_t rxchar;
        while (uart->rxbuf_rp != uart->rxbuf_wp) {
                rxchar = uart->rxbuf[RXFIFO_RP(uart)];
                uart->rxbuf_rp++;
                if (uart->sinkProc) {
                        uart->sinkProc(uart->sinkData, rxchar);
                }
        }

}

static EVENT_DECLARE(euartc0_sink_data,uartc0_sink_data,&g_uartc0);

/**
 *********************************************************
 *
 * \fn ISR(USART0_RX_vect)
 *      This RX-Interrupthandler puts the received data
 *      into the fifo and triggers an event.
 *
 *********************************************************
 */
ISR(USARTC0_RXC_vect)
{
        g_uartc0.rxbuf[RXFIFO_WP(&g_uartc0)] = USARTC0.DATA;
        g_uartc0.rxbuf_wp++;
        EV_Trigger(&euartc0_sink_data);
}


/**
 * Register the data sink for the UART
 */
void 
UartC0_RegisterSink(UartC0SinkProc * proc,void *sinkData)
{
        g_uartc0.sinkData = sinkData;
        /* Should be atomic */
        g_uartc0.sinkProc = proc;
}

/**
 *******************************************************************
 * \fn void Uart_TransmitChar(uint8_t c);
 * Transmit one char. Call event queue when waiting is necessary.
 * Do nothing if the UART is not enabled (Device is not in
 * debug mode.
 *******************************************************************
 */
void 
UartC0_TransmitChar(uint8_t c)
{
        if(g_uartc0.enabled == 0) {
                return;
        }
 	while((USARTC0.STATUS & USART_DREIF_bm) == 0) {
		//PORTC.OUTTGL = 0x40;
                EV_Yield();
	}
    	USARTC0.DATA = c;  
}

#define USART_BSCALE	(0)
#define USART_BAUDRATE 38400
#define USART_BSEL ((F_CPU / (USART_BAUDRATE * UINT32_C(16))) - 1)

void UartC0_Init(void) 
{

	PORTC.OUTSET = PIN3_bm;
	PORTC.DIRSET = PIN3_bm;
	PORTC.DIRCLR = PIN2_bm;
	/* set baud rate, frame format */
	USARTC0.BAUDCTRLA = USART_BSEL;
	USARTC0.BAUDCTRLB = (USART_BSCALE << 4) | (USART_BSEL >> 8);
	/* set mode (asynchronous), 8-bit character, 1 stop bit, no parity */
	USARTC0.CTRLC = 0x03;
	/* configure interrupts levels */
	USARTC0.CTRLA |= USART_RXCINTLVL_LO_gc;
	//  USART_DREINTLVL_LO_gc;
	/* enable rx and tx */
	USARTC0.CTRLB |= USART_TXEN_bm | USART_RXEN_bm; 
	g_uartc0.enabled = 1;
}

