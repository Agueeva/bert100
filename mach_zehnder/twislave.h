#ifndef _TWISLAVE_H
#define _TWISLAVE_H
void TWISlave_Init(void);
typedef uint8_t TWIDataSourceProc(uint8_t dw_addr);
typedef void TWIDataSinkProc(uint8_t dw_addr,  uint8_t * buf, uint8_t cnt);
void TWI_RegisterSink(TWIDataSinkProc * proc);
void TWI_RegisterSource(TWIDataSourceProc * proc);

/**
 ************************************************************************
 * Interface to the terminal/shell
 ************************************************************************
 */
typedef void TWITermSinkProc(void *clientData, uint8_t c);
void TWITerm_RegisterSink(TWITermSinkProc * proc,void *sinkData);
void TWITerm_TransmitChar(uint8_t c);

#endif
