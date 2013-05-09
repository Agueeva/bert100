/**
 ***********************************************************************************
 * Ethernet Drivers
 ***********************************************************************************
 */
#include "types.h"
#include "ethdrv.h"
#include "console.h"

void 
Eth_RegisterPktSink(int ifaceID,Eth_PktSinkProc *proc,void *cbData)
{
	EthDriver *ethDriver;
	if((ifaceID >= MAX_ETH_DRIVERS) || !(ethDriver = &g_EthDrivers[ifaceID])) {
		Con_Printf("Illegal Ethernet driver ID\n");
		return;
        }
	ethDriver->pktSinkProc = proc;
	ethDriver->pktSinkData = cbData;
}

/**
 ***********************************************************************
 * \fn void Eth_Control(int ifaceID,EthControlCmd *ctrlCmd)
 ***********************************************************************
 */
void
Eth_Control(int ifaceID,EthControlCmd *ctrlCmd)
{
        EthDriver *ethDriver;
        if((ifaceID < MAX_ETH_DRIVERS) && (ethDriver = &g_EthDrivers[ifaceID])) {
                ethDriver->ctrlProc(ethDriver->driverData,ctrlCmd);
        }
}

