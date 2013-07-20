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

/* Configure the clock to 96MHz CPU / 48MHz Peripheral */
static void
ConfigureClocks(void)
{
	uint32_t i;
	SYSTEM.MOSCWTCR.BYTE = 0x0d;
	SYSTEM.PLLWTCR.BYTE = 0xE;
	SYSTEM.PLLCR.WORD = 0x0f00;
	SYSTEM.MOSCCR.BYTE = 0x00;
	SYSTEM.PLLCR2.BIT.PLLEN = 0; 
	for(i = 0; i < 3000;i++) {
		asm("nop":::"memory");	
	}
	SYSTEM.SCKCR.LONG = 0x21821211;
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
 	BMOD(3,PORT0.PODR.BYTE,toggle);
 //	BMOD(6,PORT8.PODR.BYTE,toggle);
}


int main(void)
{

	Interp *interp;
        Editor *editor;
	/* Disable register protection permanently */
	SYSTEM.PRCR.WORD = 0xa50b;
	/* Disable Pin function write protections permanently */	
	MPC.PWPR.BYTE = 0x00;
	MPC.PWPR.BYTE = 0x40;
	/*
 	***********************************************
 	* USB host VBus Power enable
 	***********************************************
 	*/
 	BSET(6,PORT1.PDR.BYTE);
 	BSET(6,PORT1.PODR.BYTE);

 	BSET(3,PORT0.PDR.BYTE);
 	BCLR(3,PORT0.PODR.BYTE);
 	BSET(5,PORT0.PDR.BYTE);
 	BCLR(5,PORT0.PODR.BYTE);

	/* Backlight LCD */
 	BSET(6,PORT8.PDR.BYTE);
 //	BSET(6,PORT8.PODR.BYTE);

	ConfigureClocks();
	Threads_Init();
	TPOS_Init();
	Timers_Init();
	Sci0_Init(115200);
	interp = Interp_Init(Con_OutStr, Con_PrintVA);
	editor = Editor_Init(Interp_Feed, interp);
	Con_RegisterSink(Editor_Feed, editor);
	ShiftReg_Init(0xffff);
	RxCRC_Init();
	MD5Lib_Init();
	Sha1Lib_Init();
	WDTA_Init();
	AD537x_Init();
	MDIO_Init();
	CDR_Init();

	Timer_Start(&blinkTimer,500);
	RX_EtherInit();
	Spi_Init();
	SDCard_ModuleInit();
	FatCmds_Init();
	EV_Loop();
}
