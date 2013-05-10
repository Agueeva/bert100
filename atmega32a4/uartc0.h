#ifndef _UARTC0_H
#define _UARTC0_H
void UartC0_Init(void);
typedef void UartC0SinkProc(void *clientData, uint8_t c);
void UartC0_RegisterSink(UartC0SinkProc * proc,void *sinkData);
void UartC0_TransmitChar(uint8_t c);
#endif
