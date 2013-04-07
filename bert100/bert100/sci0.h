#ifndef _SCI0_H
#define _SCI0_H
#include "types.h"
#include "interrupt_handlers.h"
#define SCI0_FER	(0x8000)

void Sci0_Configure(uint32_t baud, uint8_t rxbits, uint8_t txbits, uint8_t par, uint8_t rx_stop, uint8_t tx_stop);
void Sci0_TransmitChar(uint8_t c);
typedef void Sci0SinkProc(void *clientData, uint8_t c);
void Sci0_Init(uint32_t baudrate);
void Sci0_RegisterSink(Sci0SinkProc * proc, void *sinkData);
uint8_t Sci0_Read(uint8_t * data);
void Sci0_Flush(void);
#endif
