#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include "console.h"
#include "timer.h"
#include "irqflags.h"
#include "events.h"
#include "twislave.h"

#define PINCTRL_SDA     PORTC.PIN0CTRL
#define PINCTRL_SCL     PORTC.PIN1CTRL

#define TWISTATE_IDLE	(0)
#define TWISTATE_ADDR	(1)
#define TWI_MAX_RX	(9)
#define array_size(x) (sizeof(x) / sizeof((x)[0]))

#define TERM_TX_BUF_SIZE	(128)
#define TERM_TX_BUF_WP(twi)	((twi)->term_tx_buf_wp % TERM_TX_BUF_SIZE)
#define TERM_TX_BUF_RP(twi)	((twi)->term_tx_buf_rp % TERM_TX_BUF_SIZE)
typedef struct TWISlave {
	uint8_t data;
	uint8_t state;
	uint8_t rcvd_addr;
	uint8_t data_addr;
	uint8_t rcv_buf[TWI_MAX_RX];
	uint8_t rx_cnt;
	uint8_t tx_cnt;
	uint8_t term_tx_buf[TERM_TX_BUF_SIZE];
	uint8_t term_tx_buf_wp;
	uint8_t term_tx_buf_rp;
	uint8_t term_enable;
	TWITermSinkProc *termSinkProc;
	void *termSinkData;

	TWIDataSinkProc *sink;
        TWIDataSourceProc *source;
} TWISlave;

TWISlave g_TWISlave;

/**
 * Register the data sink for the UART
 */
void
TWITerm_RegisterSink(TWITermSinkProc * proc,void *sinkData)
{
        g_TWISlave.termSinkData = sinkData;
        /* Should be atomic */
        g_TWISlave.termSinkProc = proc;
}

void TWITerm_TransmitChar(uint8_t c) 
{
	TWISlave *twi = &g_TWISlave;
	twi->term_tx_buf[TERM_TX_BUF_WP(twi)] = c;
	barrier();
	twi->term_tx_buf_wp++;
	if((twi->term_tx_buf_wp & 3) == 0) {
		SleepMs(1);	
	}
}
/**
 **************************************************************
 * \fn void TWI_RegisterSink(TWIDataSinkProc * proc)
 * Register a data sink for the TWI Slave.
 **************************************************************
 */
void TWI_RegisterSink(TWIDataSinkProc * proc)
{
        g_TWISlave.sink = proc;
}

/**
 **************************************************************
 * \fn void TWI_RegisterSource(TWIDataSinkProc * proc)
 * Register a data source for the TWI Slave.
 **************************************************************
 */
void TWI_RegisterSource(TWIDataSourceProc * proc)
{
        g_TWISlave.source = proc;
}

static void 
TWI_DataSink(void *evData) 
{
	TWISlave *twi = evData;
	uint8_t i;
	//Con_Printf_P("Twi Data at %d, cnt %d, data[1]: 0x%02x\n",twi->rcvd_addr,twi->transfer_cnt, twi->rcv_buf[1]);	
	if(twi->term_enable && (twi->data_addr >= 13) && (twi->data_addr < 22)) {
		if(twi->termSinkProc) {
			for(i = 1; i < twi->rx_cnt; i++) { 	
				twi->termSinkProc(twi->termSinkData,twi->rcv_buf[i]);
				twi->data_addr = (twi->data_addr & 0x80) | ((twi->data_addr + 1) & 0x7f);
			}
		}
		return;	
	}  
	for(i = 1; i < twi->rx_cnt; i++) { 	
		if(twi->sink) {
			twi->sink(twi->data_addr, twi->rcv_buf + i,1);
		}
		twi->data_addr = (twi->data_addr & 0x80) | ((twi->data_addr + 1) & 0x7f);
	}
}

EVENT_DECLARE(e_TWIDataAvail,TWI_DataSink,&g_TWISlave);

/**
 *****************************************************************************************************
 * \fn static uint8_t fetch_byte(TWISlave *twi); 
 *****************************************************************************************************
 */
static uint8_t
fetch_byte(TWISlave *twi) {
	uint8_t value;
	if(twi->term_enable && (twi->data_addr >= 13) && (twi->data_addr < 22)) {
		if(twi->term_tx_buf_rp == twi->term_tx_buf_wp) {
			value = 0;	
		} else {
			value = twi->term_tx_buf[TERM_TX_BUF_RP(twi)];
			twi->term_tx_buf_rp++;
		}
	} else if(twi->source) {
		value = twi->source(twi->data_addr);
		/* Increment with rollover in same 128 Byte page */
	} else {
		value = 0xab; /* Should better return non ready value from status reg. */
	}
	twi->data_addr = (twi->data_addr & 0x80) | ((twi->data_addr + 1) & 0x7f);
	return value;
}

ISR(TWIC_TWIS_vect)
{
	TWISlave *twi = &g_TWISlave;
	uint8_t status = TWIC.SLAVE.STATUS;	
	if((status & TWI_SLAVE_APIF_bm) && (status & TWI_SLAVE_AP_bm)) {
		twi->rcvd_addr = TWIC.SLAVE.DATA;
		/* Be careful with the stop interrupt */
        	TWIC.SLAVE.CTRLA &= ~TWI_SLAVE_PIEN_bm;
		TWIC.SLAVE.CTRLB = TWI_SLAVE_CMD_RESPONSE_gc;
		twi->rx_cnt = 0;
		twi->tx_cnt = 0;
        } else if (status & TWI_SLAVE_DIF_bm) {
		if(status & TWI_SLAVE_DIR_bm) {
			/* Master reads from me */
			if ((twi->tx_cnt > 0) && (status & TWI_SLAVE_RXACK_bm)) {
                		TWIC.SLAVE.CTRLB = TWI_SLAVE_CMD_COMPTRANS_gc;
			} else {
				/* Fetch the next byte */
				TWIC.SLAVE.DATA = fetch_byte(twi);
				twi->tx_cnt++;
			}
			TWIC.SLAVE.CTRLB = TWI_SLAVE_CMD_RESPONSE_gc;
		} else {
			/* Master Writes to me */
        		TWIC.SLAVE.CTRLA |= TWI_SLAVE_PIEN_bm;
			if(twi->rx_cnt < array_size(twi->rcv_buf)) {
				twi->rcv_buf[twi->rx_cnt] = TWIC.SLAVE.DATA;
				if(twi->rx_cnt == 0) {
					twi->data_addr = twi->rcv_buf[0];
				}
				twi->rx_cnt++;
				TWIC.SLAVE.CTRLB = TWI_SLAVE_CMD_RESPONSE_gc;
			} else {
				TWIC.SLAVE.CTRLB = TWI_SLAVE_ACKACT_bm |
                                              TWI_SLAVE_CMD_COMPTRANS_gc;
	
			}
		}
  	} else if (status & TWI_SLAVE_APIF_bm) {
		/* This only happens after a write to slave transaction */
        	TWIC.SLAVE.CTRLA &= ~TWI_SLAVE_PIEN_bm;
        	TWIC.SLAVE.STATUS |= TWI_SLAVE_APIF_bm;
		if(twi->rx_cnt > 1) {
			EV_Trigger(&e_TWIDataAvail);
		}
        }
	return;
}

void
TWISlave_Init(void) 
{
#if 1
	PINCTRL_SDA = PORT_OPC_WIREDANDPULL_gc;
	PINCTRL_SCL = PORT_OPC_WIREDANDPULL_gc;
	TWIC.SLAVE.ADDR = 0xA2;
	TWIC.SLAVE.CTRLA = TWI_SLAVE_INTLVL_LO_gc |
		TWI_SLAVE_APIEN_bm | TWI_SLAVE_DIEN_bm | TWI_SLAVE_ENABLE_bm;


	g_TWISlave.term_enable = 1;
#endif
	
}
