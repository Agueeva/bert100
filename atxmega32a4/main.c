#include <stdint.h>
#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "timer.h"
#include "events.h"
#include "console.h"
#include "interpreter.h"
#include "editor.h"
#include "adc.h"
#include "pwm.h"
#include "i2cmaster.h"
#include "twislave.h"
#include "tx.h"
#include "control.h"
#include "threads.h"
#include "tpos.h"
#include "ad537x.h"
#include "sweep.h"
#include "mdio.h"
#include "cdr.h"
#include "shiftreg.h"

extern void _switch_context(uint16_t *newStack,uint16_t *oldStack);

int
main()
{
	Interp *interp;
	OSC.PLLCTRL = OSC_PLLSRC_RC32M_gc | 16;
	CCP = 0xd8; /* Enable write to protected IO register */
	CLK.PSCTRL = CLK_PSADIV_1_gc | CLK_PSBCDIV_2_2_gc;
	OSC.CTRL = 0x13; 
	while((OSC.STATUS & 0x13) != 0x13) {
		/* Wait until everything is stable */
	}
	CCP = 0xd8; /* Enable write to protected IO register */
	CLK.CTRL = 4;
	
	Timers_Init();
	PMIC.CTRL = 7;
	sei();
	ShiftReg_Init(0xffff);
	UartC0_Init();
	ADC_Init();
	Con_Printf_P("\nWelcome to Mach-Zehnder\n" 
		"Build " __DATE__ " " __TIME__ "\n");
	I2CM_Init();
	interp = Interp_Init(Con_OutStr, _Con_Printf_P);
	Editor_Init(Interp_Feed, interp);
	PWM_Init();
	TX_Init();
	//TWISlave_Init();
	Control_Init();
	Threads_Init();
	TPOS_Init();
	//_switch_context(0x1000,0x2000);
	Sweep_Init();
	AD537x_Init();
	MDIO_Init();
	Cdr_Init();
	EV_Loop();	
}
