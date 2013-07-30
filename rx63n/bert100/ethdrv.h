#ifndef _ETHDRV_H
#define _ETHDRV_H
#include "skb.h"
#define MAX_ETH_DRIVERS	1

#define ETHIF_RXETH0    0

#define ETHCTL_SET_MAC	0
#define ETHCTL_GET_MAC	1

typedef struct EthControlCmd {
	unsigned int cmd;
	void *cmdArg;
} EthControlCmd;

#if 0
/**
 *************************************************************************
 * Pakets are split into two halfs. A header and a Data part. 
 * The header is written from the end. It end-first may not get
 * bigger than maxlen; The header grows downward up to maxlen
 * on composition of packet. Header grows upward on decomposition
 * For decomposition all data needs to be in one databuf and
 * hdr_end moves. hdr_start has a reserve of 32 Bytes for 
 * the case that the sent header is bigger than the received header.
 *************************************************************************
 */
typedef struct Skb {
        uint8_t *hdrBuf;
        uint8_t *hdrStart;
        uint8_t *hdrEnd;
        uint16_t hdrAvailLen;
        uint16_t hdrBufSize;

        uint8_t *dataBuf;
        uint8_t *dataStart;
        uint16_t dataLen;
        uint16_t dataAvailLen;
        uint16_t dataBufSize;
} Skb;
#endif

typedef struct EthDriver EthDriver;
typedef void Eth_TxProc(void *driverData,const uint8_t *buf,uint16_t pktlen);
typedef void EthDrv_PktSinkProc(void *eventData,Skb *sbk);
typedef void EthDrv_RegPktSinkProc(EthDriver *,EthDrv_PktSinkProc *cbProc,void *evData);
typedef void Eth_ControlProc(void *driverData,EthControlCmd *ctrlCmd);
/*
 ****************************************************************************
 * \struct EthDriver
 * This is the interface implemented by an Ethernet driver.
 * Only the Receive callback proc is indirectly accessible by the
 * user of the Ethernet driver.
 ****************************************************************************
 */
struct EthDriver {
	int ifID;
	Eth_TxProc *txProc;
	Eth_ControlProc *ctrlProc;
	EthDrv_RegPktSinkProc *regPktSink;
	void *driverData;	

	//Eth_PktSinkProc *pktSinkProc;
	void *pktSinkData;
};

//EthDriver g_EthDrivers[MAX_ETH_DRIVERS];

#if 0
static inline void 
Eth_TransmitPacket(int ifaceID,uint8_t *buf,uint16_t pktlen)
{
	EthDriver *ethDriver;
	if((ifaceID < MAX_ETH_DRIVERS) && (ethDriver = &g_EthDrivers[ifaceID])) {
		ethDriver->transmitProc(ethDriver->driverData,buf,pktlen);
	}
}
#endif

void EthDrv_RegisterPktSink(int ifaceID,EthDrv_PktSinkProc *proc,void *cbData); 
void EthDrv_Control(int ifaceID,EthControlCmd *ctrlCmd);

#endif
