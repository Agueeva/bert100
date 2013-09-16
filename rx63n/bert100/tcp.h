#ifndef _TCP_H
#define _TCP_H
#include "types.h"
#include "ip.h"
#include "ethernet.h" /* For Skb , should be moved */
/* The Transmission control block (TCB) */
typedef struct Tcb Tcb;
typedef bool Tcp_AcceptProc(void *eventData,Tcb *con,uint8_t ip[4],uint16_t port_nr);
/* The data sink returns the bytes sunken */
typedef uint16_t Tcp_DataSink(void *eventData,uint32_t fpos,const uint8_t *buf,uint16_t count);
typedef uint16_t Tcp_DataSrc(void *eventData,uint32_t fpos,void **buf,uint16_t maxlen);
typedef void Tcp_CloseProc(void *eventData);

typedef struct Tcb_Operations {
	Tcp_DataSink *sinkProc;
	Tcp_DataSrc *srcProc;
	Tcp_CloseProc *closeProc;	
} Tcb_Operations;

void Tcb_RegisterOps(Tcb *tcb,Tcb_Operations *tcops,void *eventData);
void Tcb_SetMaxWinSize(Tcb *tcb,uint16_t maxWinSz);
void Tcp_ProcessPacket(IpHdr *ipHdr,Skb *skb);
void Tcp_ServerSocket(uint16_t port,Tcp_AcceptProc *proc,void *eventData);
//void TcpCon_RegisterDataProcs(Tcb *tcon,Tcp_DataSink *sink,Tcp_DataSrc *src,void *eventData) ;
void TcpCon_ControlTx(Tcb *tcon,bool enable);
void Tcp_Close(Tcb *tcb);
void Tcp_Init(void);

#endif
