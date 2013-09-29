/**
 * Watchdog module
 */
#include "atomic.h"
#include "iodefine.h"
#include "wdta.h"
#include "interpreter.h"
#include "hex.h"
#include "console.h"

void 
WDTA_Reboot()
{
	WDT.WDTCR.WORD = 0x3300; /* Divider 4, Timeout 1023 */
	DISABLE_IRQ();
	WDT.WDTRR = 0;	
	WDT.WDTRR = 0xff;	
	while(1);
}
/**
 ****************************************************************************
 * \fn static int8_t cmd_sci0(Interp * interp, uint8_t argc, char *argv[])
 ****************************************************************************
 */
static int8_t
cmd_wdta(Interp * interp, uint8_t argc, char *argv[])
{
	WDT.WDTRR = 0;	
	WDT.WDTRR = 0xff;	
	return 0;
}

INTERP_CMD(wdtaCmd, "wdta", cmd_wdta, "wdta # Watchdog Timer A");

static int8_t
cmd_reboot(Interp * interp, uint8_t argc, char *argv[])
{
	WDTA_Reboot();
	return 0;
}

INTERP_CMD(rebootCmd, "reboot", cmd_reboot, "reboot # Reboot by using the Watchdog timer");

void
WDTA_Init(void)
{
	WDT.WDTCR.WORD = 0x3383;
	Interp_RegisterCmd(&wdtaCmd);
	Interp_RegisterCmd(&rebootCmd);
}
