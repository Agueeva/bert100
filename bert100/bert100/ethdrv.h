#ifndef _ETHDRV_H
#define _ETHDRV_H
#define MAX_ETH_DRIVERS	1

#define ETHIF_RXETH0    0

typedef struct EthControlCmd {
	unsigned int cmd;
	void *cmdArg;
} EthControlCmd;

typedef void Eth_TxProc(void *driverData,const uint8_t *buf,uint16_t pktlen);
typedef void Eth_PktSinkProc(void *eventData,int ifaceID,const uint8_t *buf,uint16_t pktlen);
typedef void Eth_ControlProc(void *driverData,EthControlCmd *ctrlCmd);

/*
 ****************************************************************************
 * \struct EthDriver
 * This is the interface implemented by an Ethernet driver.
 * Only the Receive callback proc is indirectly accessible by the
 * user of the Ethernet driver.
 ****************************************************************************
 */
typedef struct EthDriver {
	int ifID;
	Eth_TxProc *transmitProc;
	void *driverData;	
	Eth_ControlProc *ctrlProc;

	Eth_PktSinkProc *pktSinkProc;
	void *pktSinkData;
} EthDriver;

EthDriver g_EthDrivers[MAX_ETH_DRIVERS];

static inline void 
Eth_TransmitPacket(int ifaceID,uint8_t *buf,uint16_t pktlen)
{
	EthDriver *ethDriver;
	if((ifaceID < MAX_ETH_DRIVERS) && (ethDriver = &g_EthDrivers[ifaceID])) {
		ethDriver->transmitProc(ethDriver->driverData,buf,pktlen);
	}
}

void Eth_RegisterPktSink(int ifaceID,Eth_PktSinkProc *proc,void *cbData); 
void Eth_Control(int ifaceID,EthControlCmd *ctrlCmd);

#endif
