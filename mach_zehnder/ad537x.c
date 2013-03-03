#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "irqflags.h"
#include "console.h"
#include "events.h"
#include "interpreter.h"
#include "hex.h"
#include "ad537x.h"

#define array_size(x) (sizeof(x) / sizeof((x)[0]))


#define PORT_SYNC	PORTD
#define	PIN_SYNC	7
#define PINCTRL_SYNC    PORTD.PIN7CTRL

#define PORT_SDI	PORTE
#define	PIN_SDI		0
#define PINCTRL_SDI	PORTE.PIN0CTRL

#define PORT_SCLK	PORTE
#define	PIN_SCLK	1
#define PINCTRL_SCLK	PORTE.PIN1CTRL

#define PORT_SDO	PORTC
#define	PIN_SDO		5
#define PINCTRL_SDO	PORTC.PIN5CTRL

#if 0
#define PORT_BUSY	PORTE
#define	PIN_BUSY	4
#define PINCTRL_BUSY	PORTE.PIN4CTRL
#endif

#define PORT_RESET	PORTD
#define	PIN_RESET	6	
#define PINCTRL_RESET	PORTD.PIN6CTRL

#if 0
#define PORT_CLR	PORTE
#define	PIN_CLR		6
#define PINCTRL_CLR	PORTE.PIN6CTRL
#endif

static const char str_X1A[] PROGMEM = "x1a";
static const char str_X1B[] PROGMEM = "x1b";
static const char str_C[] PROGMEM = "c";
static const char str_M[] PROGMEM = "m";
static const char str_CONTROL[] PROGMEM = "control";
static const char str_OFS0[] PROGMEM = "ofs0";
static const char str_OFS1[] PROGMEM = "ofs1";
static const char str_ABSELECT0[] PROGMEM = "abselect0";
static const char str_ABSELECT1[] PROGMEM = "abselect1";
static const char str_ABSELECT2[] PROGMEM = "abselect2";
static const char str_ABSELECT3[] PROGMEM = "abselect3";

typedef struct ReadbackVar {
        PGM_P   name;
	uint16_t addrCode;
        uint8_t nrArgs;
} ReadbackVar;

static ReadbackVar PROGMEM rbvars[] = {
	{
		.name = str_X1A,
		.addrCode = 0,
		.nrArgs = 1,
	},
	{
		.name = str_X1B,
		.addrCode = 0x2000,
		.nrArgs = 1,
	},
	{
		.name = str_C,
		.addrCode = 0x4000,
		.nrArgs = 1,
	},
	{
		.name = str_M,
		.addrCode = 0x6000,
		.nrArgs = 1,
	},
	{
		.name = str_CONTROL,
		.addrCode = 0x8080,
		.nrArgs = 0,
	},
	{
		.name = str_OFS0,
		.addrCode = 0x8100,
		.nrArgs = 0,
	},
	{
		.name = str_OFS1,
		.addrCode = 0x8180,
		.nrArgs = 0,
	},
	{
		.name = str_ABSELECT0,
		.addrCode = 0x8300,
		.nrArgs = 0,
	},
	{
		.name = str_ABSELECT1,
		.addrCode = 0x8380,
		.nrArgs = 0,
	},
	{
		.name = str_ABSELECT2,
		.addrCode = 0x8400,
		.nrArgs = 0,
	},
	{
		.name = str_ABSELECT3,
		.nrArgs = 0,
		.addrCode = 0x8480,
	}
};

typedef struct WriteVar {
        PGM_P   name;
	uint16_t sfCode;
        uint8_t nrArgs;
} WriteVar;

static WriteVar PROGMEM wrvars[] = {
	{
		.name = str_CONTROL,
		.sfCode = 1,
	},
	{
		.name = str_OFS0,
		.sfCode = 2,
	},
	{
		.name = str_OFS1,
		.sfCode = 3,
	},
	{
		.name = str_ABSELECT0,
		.sfCode = 6,
	},
	{
		.name = str_ABSELECT1,
		.sfCode = 7,
	},
	{
		.name = str_ABSELECT2,
		.sfCode = 8,
	},
	{
		.name = str_ABSELECT3,
		.sfCode = 9,
	}
};

static uint32_t  
AD537x_Write(uint32_t value)
{
	int8_t i;
	uint32_t inval;
	inval = 0;
	PORT_SYNC.OUTCLR = (1 << PIN_SYNC);
	for(i = 23; i >= 0;i--) {
		if(PORT_SDO.IN & (1 << PIN_SDO)) { 
			inval = (inval << 1) | 1;
		} else {
			inval = (inval << 1);
		}
		if((value >> i) & 1) {
			//Con_Printf_P("1");
			PORT_SDI.OUTSET = (1 << PIN_SDI);
		} else {
			//Con_Printf_P("0");
			PORT_SDI.OUTCLR = (1 << PIN_SDI);
		}
		asm("nop");
		asm("nop");
		asm("nop");
		PORT_SCLK.OUTCLR = (1 << PIN_SCLK);
		asm("nop");
		asm("nop");
		asm("nop");
		PORT_SCLK.OUTSET = (1 << PIN_SCLK);
	}		
	PORT_SYNC.OUTSET = (1 << PIN_SYNC);
	//Con_Printf_P("\n");
	return inval;
}

static inline void 
AD537x_SFWrite(uint8_t sfc,uint16_t value)
{
	uint32_t wrval;
	wrval = value | ((uint32_t)sfc << 16); 
	Con_Printf_P("Write %06lx\n",wrval);
	AD537x_Write(wrval);
}

/*
 * Readback  
 */
static uint32_t 
AD537x_Readback(uint16_t addrCode,uint8_t channelAddr) 
{
	uint32_t spiCmd;
	uint32_t result;
	spiCmd = addrCode | (UINT32_C(5) << 16) | (((uint16_t)channelAddr) << 7);
	AD537x_Write(spiCmd);
	spiCmd = 0; /* NOP */	
	result = AD537x_Write(spiCmd);
	return result;	
}

static int8_t
cmd_dac(Interp *interp,uint8_t argc,char *argv[])
{
	uint8_t i;
	uint8_t channel;
	ReadbackVar rbvar; 
	WriteVar wrvar;
	uint16_t value;
	if(argc < 2) {
		for(i = 0; i < array_size(rbvars); i++) {
			memcpy_P(&rbvar,rbvars + i,sizeof(rbvar));
			if(rbvar.nrArgs) {
				value = AD537x_Readback(rbvar.addrCode,0);
				Con_Printf_P("%S(%04x): %04x\n",rbvar.name,rbvar.addrCode,value);
			} else {
				value = AD537x_Readback(rbvar.addrCode,0);
				Con_Printf_P("%S(%04x): %04x\n",rbvar.name,rbvar.addrCode,value);
			}		
		}
		return 0;
	}
	for(i = 0; i < array_size(rbvars); i++) {
		memcpy_P(&rbvar,rbvars + i,sizeof(rbvar));
		if((strcmp_P(argv[1],rbvar.name) == 0) 
			&& (rbvar.nrArgs + 2) == argc) 
		{
			if(rbvar.nrArgs) {
				channel = astrtoi16(argv[2]);
			} else {
				channel = 0;
			}		
			value = AD537x_Readback(rbvar.addrCode,channel);
			Con_Printf_P("%04x\n",value);
			return 0;
		}
	}
	/* Now try to write the register */
	for(i = 0; i < array_size(wrvars); i++) {
		memcpy_P(&wrvar,wrvars + i,sizeof(wrvar));
		if((argc > 2) && (strcmp_P(argv[1],wrvar.name) == 0)) {
			value = astrtoi16(argv[2]);	
			AD537x_SFWrite(wrvar.sfCode,value);
			return 0;
		} 
	}
	return -EC_BADARG;
}


static int8_t
cmd_dacx(Interp *interp,uint8_t argc,char *argv[])
{
	uint8_t channel;
	uint16_t value;
	if(argc < 3) {
		uint8_t i;
		uint16_t value;
		for(i = 0; i < 32; i++) {
			value = AD537x_Readback(0,i + 8);
			Con_Printf_P("%04x ",value);
			if((i % 16) == 15) {
				Con_Printf_P("\n");
			}
		}
		Con_Printf_P("\n");
		for(i = 0; i < 32; i++) {
			value = AD537x_Readback(1 << 13,i + 8);
			Con_Printf_P("%04x ",value);
			if((i % 16) == 15) {
				Con_Printf_P("\n");
			}
		}
		return 0;
	}
	channel = astrtoi16(argv[1]);
	value = astrtoi16(argv[2]);
	AD537x_Write(value | ((uint32_t)(channel + 8) << 16) | (UINT32_C(3) << 22));
	return 0;
}

static int8_t
cmd_dacc(Interp *interp,uint8_t argc,char *argv[])
{
	uint8_t channel;
	uint16_t value;
	if(argc < 3) {
		uint8_t i;
		for(i = 0; i < 32; i++) {
			value = AD537x_Readback(2 << 13,i + 8);
			Con_Printf_P("%04x ",value);
			if((i % 16) == 15) {
				Con_Printf_P("\n");
			}
		}
		return 0;
	}
	channel = astrtoi16(argv[1]);
	value = astrtoi16(argv[2]);
	AD537x_Write(value | ((uint32_t)(channel + 8) << 16) | (UINT32_C(2) << 22));
	return 0;
}

static int8_t
cmd_dacm(Interp *interp,uint8_t argc,char *argv[])
{
	uint8_t channel;
	uint16_t value;
	if(argc < 3) {
		uint8_t i;
		for(i = 0; i < 32; i++) {
			value = AD537x_Readback(3 << 13,i + 8);
			Con_Printf_P("%04x ",value);
			if((i % 16) == 15) {
				Con_Printf_P("\n");
			}
		}
		return 0;
	}
	channel = astrtoi16(argv[1]);
	value = astrtoi16(argv[2]);
	AD537x_Write(value | ((uint32_t)(channel + 8) << 16) | (UINT32_C(1) << 22));
	return 0;
}

INTERP_CMD(dac, cmd_dac,
           "dac <regname> ?<channel>? ?<value>? # ");

INTERP_CMD(dacx, cmd_dacx,
           "dacx <channel> <value> # ");

INTERP_CMD(dacc, cmd_dacc,
           "dacc <channel> <value> # ");

INTERP_CMD(dacm, cmd_dacm,
           "dacm <channel> <value> # ");

void
AD537x_Init(void)
{
	PINCTRL_RESET = PORT_OPC_TOTEM_gc;
	PORT_RESET.DIRCLR = (1 << PIN_RESET);
	PORT_RESET.OUTCLR = (1 << PIN_RESET);
	
	PINCTRL_SYNC = PORT_OPC_TOTEM_gc;
	PORT_SYNC.DIRSET = (1 << PIN_SYNC);
	PORT_SYNC.OUTSET = (1 << PIN_SYNC);

	PINCTRL_SDI = PORT_OPC_TOTEM_gc;
	PORT_SDI.DIRSET = (1 << PIN_SDI);

	PINCTRL_SCLK = PORT_OPC_TOTEM_gc;
	PORT_SCLK.DIRSET = (1 << PIN_SCLK);
	PORT_SCLK.OUTSET = (1 << PIN_SYNC);

	PINCTRL_SDO = PORT_OPC_PULLUP_gc; 
	PORT_SDO.DIRCLR = (1 << PIN_SDO);
	
#if 0
	PINCTRL_BUSY = PORT_OPC_PULLUP_gc;
	PORT_BUSY.DIRCLR = (1 << PIN_BUSY);
#endif


#if 0
	PINCTRL_CLR = PORT_OPC_TOTEM_gc;
	PORT_CLR.DIRSET = (1 << PIN_CLR);
#endif
	PORT_RESET.OUTSET = (1 << PIN_RESET);
}
