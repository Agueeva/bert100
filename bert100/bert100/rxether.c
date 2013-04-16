/*
 * Renesas RX Ethernet controller driver
 */

#include "iodefine.h"
#include "types.h"
#include "rxether.h"

void
RX_EtherInit()
{
	volatile uint32_t i;
	MSTP(EDMAC) = 0;
	for(i = 0; i < 10000; i++) {

	}
	EDMAC.EDMR.BIT.SWR = 1;
	for(i = 0; i < 10000; i++) {

	}
	/* This is a locally assigned unicast address */
	ETHERC.MAHR = 0x12345678;
	ETHERC.MALR.LONG = 0x9abc;
}
