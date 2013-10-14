/***********************************************************************/
/*                                                                     */
/*  FILE        :bert100.c                                             */
/*  DATE        :Tue, Apr 02, 2013                                     */
/*  DESCRIPTION :Main Program                                          */
/*  CPU TYPE    :RX63N                                                 */
/*                                                                     */
/*  This file is generated by KPIT GNU Project Generator.              */
/*                                                                     */
/***********************************************************************/
                    

#include "iodefine.h"
#include "types.h"
#include "timer.h"
#include "events.h"
#include "threads.h"
#include "console.h"
#include "sci0.h"
#include "interpreter.h"
#include "hex.h"
#include "editor.h"
#include "threads.h"
#include "tpos.h"
#include "rxether.h"
#include "md5.h"
#include "sha1.h"
#include "wdta.h"
#include "rx_crc.h"
#include "spi.h"
#include "sdcard.h"
#include "fatcmds.h"
#include "ad537x.h"
#include "mdio.h"
#include "cdr.h"
#include "shiftreg.h"
#include "i2cmaster.h"
#include "ethernet.h"
#include "skb.h"
#include "xy_web.h"
#include "pvar.h"
#include "pvar_websock.h"
#include "adc12.h"
#include "config.h"
#include "usbdev.h"
#include "usbstorage.h"
#include "xo.h"
#include "leds.h"
#include "version.h"
#include "dataflash.h"
#include "swupdate.h"
#include "bert.h"
#include "pat_trig.h"
#include "fanco.h"

/* Configure the clock to 96MHz CPU / 48MHz Peripheral */
static void
ConfigureClocks(void)
{
	uint32_t i;
	SYSTEM.SCKCR3.WORD = 0x0;
	SYSTEM.PLLCR2.BIT.PLLEN = 1; 
	SYSTEM.MOSCWTCR.BYTE = 0x0d;
	SYSTEM.PLLWTCR.BYTE = 0xE;
	SYSTEM.PLLCR.WORD = 0x0f00;
	SYSTEM.MOSCCR.BYTE = 0x00;
	SYSTEM.PLLCR2.BIT.PLLEN = 0; 
	for(i = 0; i < 3000;i++) {
		asm("nop":::"memory");	
	}
	//SYSTEM.SCKCR.LONG = 0x21821211;
	SYSTEM.SCKCR.LONG = 0x21821222;
	SYSTEM.SCKCR2.WORD = 0x033;
	SYSTEM.SCKCR3.WORD = 0x400;
}

static void blinkProc(void *eventData);
static TIMER_DECLARE(blinkTimer,blinkProc,NULL);

static void
blinkProc(void *eventData)
{
	static uint8_t toggle = 0;
	Timer_Start(&blinkTimer,500);
	toggle ^= 1;
#ifdef  BOARD_SAKURA
 	BSET(0,PORTA.PDR.BYTE);
 	BMOD(0,PORTA.PODR.BYTE,toggle);
#else 
 	BMOD(5,PORTA.PODR.BYTE,toggle);
#endif
}

int 
main(void)
{

	Interp *interp;
        Editor *editor;
	EthDriver *ethDrv;
	XY_WebServer *wserv;
	/* Disable register protection permanently */
	SYSTEM.PRCR.WORD = 0xa50b;
	/* Disable Pin function write protections permanently */	
	MPC.PWPR.BYTE = 0x00;
	MPC.PWPR.BYTE = 0x40;

	/* Alive LED */
	BSET(5,PORTA.PDR.BYTE);

	ConfigureClocks();
	Threads_Init();
	TPOS_Init();
	Timers_Init();
	Sci0_Init(115200);
	DelayUs(300);
	interp = Interp_Init(Con_OutStr, Con_PrintVA);
	editor = Editor_Init(Interp_Feed, interp);
	Con_RegisterSink(Editor_Feed, editor);
	RxCRC_Init(); /* Needed for the hash buckets of the PVARs */
	PVars_Init(); /* We need this early because som HW modules export objects */

	UsbStor_Init();
	Spi_Init();
	if(SDCard_ModuleInit() == true) {
		UsbStor_UnitReady(true);
	}
	FatCmds_Init();
	ShiftReg_Init(0xffff);
	I2CM_Init();
	MD5Lib_Init();
	Sha1Lib_Init();
	WDTA_Init();
#ifndef BOARD_SAKURA 
	AD537x_ModInit("dac0");
#endif
	MDIO_Init();
	PatTrig_Init("ptrig0");
	Synth_Init("synth0",0x10aa);
	CDR_Init("cdr0");
	Timer_Start(&blinkTimer,500);
	Skb_Init();
	Tcp_Init();
	ethDrv = RX_EtherInit();
	Ethernet_Init(ethDrv);
	wserv = XY_NewWebServer();
	PVarSocket_New(wserv);	
	ADC12_Init();
	DataFlash_Init();
	SWUpdate_Init();
	/* Now the higher level modules depending on hardware modules */
	Leds_Init();
	FanCo_Init();
	Version_Init();
	Bert_Init();
	Interp_StartScript(interp, "0:/bert100.scr");
	EV_Loop();
}
