#include <string.h>
#include "types.h"
#include "console.h"
#include "events.h"
#include "interpreter.h"
#include "hex.h"
#include "ad537x.h"
#include "atomic.h"
#include "iodefine.h"

#define array_size(x) (sizeof(x) / sizeof((x)[0]))


#define SYNC_DIROUT	BSET(5,PORTC.PDR.BYTE)
#define SYNC_HIGH	BSET(5,PORTC.PODR.BYTE)
#define SYNC_LOW	BCLR(5,PORTC.PODR.BYTE)	

#define SDI_DIROUT	BSET(3,PORTC.PDR.BYTE)
#define SDI_HIGH	BSET(3,PORTC.PODR.BYTE)	
#define SDI_LOW		BCLR(3,PORTC.PODR.BYTE)

#define SCLK_DIROUT	BSET(2,PORTC.PDR.BYTE)
#define SCLK_HIGH	BSET(2,PORTC.PODR.BYTE)
#define SCLK_LOW	BCLR(2,PORTC.PODR.BYTE)

#define SDO_DIRIN	BCLR(4,PORTC.PDR.BYTE)
#define SDO_READ	PORTC.PIDR.BIT.B4	

#define RESET_DIROUT	BSET(6,PORTC.PDR.BYTE)
#define RESET_HIGH	BSET(6,PORTC.PODR.BYTE)
#define RESET_LOW	BCLR(6,PORTC.PODR.BYTE)

#define CLR_DIROOUT	BSET(1,PORT5.PDR.BYTE)
#define CLR_HIGH	BSET(1,PORT5.PODR.BYTE)
#define CLR_LOW		BCLR(1,PORT5.PODR.BYTE)

#define LDAC_DIROUT	BSET(0,PORT5.PDR.BYTE)
#define LDAC_HIGH	BSET(0,PORT5.PODR.BYTE)
#define LDAC_LOW	BCLR(0,PORT5.PODR.BYTE)


#if 0
#define PORT_BUSY	PORTE
#define	PIN_BUSY	4
#define PINCTRL_BUSY	PORTE.PIN4CTRL
#endif

typedef struct ReadbackVar {
        const char   *name;
	uint16_t addrCode;
        uint8_t nrArgs;
} ReadbackVar;

static ReadbackVar rbvars[] = {
	{
		.name = "x1a",
		.addrCode = 0,
		.nrArgs = 1,
	},
	{
		.name = "x1b",
		.addrCode = 0x2000,
		.nrArgs = 1,
	},
	{
		.name = "c",
		.addrCode = 0x4000,
		.nrArgs = 1,
	},
	{
		.name = "m",
		.addrCode = 0x6000,
		.nrArgs = 1,
	},
	{
		.name = "control", 
		.addrCode = 0x8080,
		.nrArgs = 0,
	},
	{
		.name = "ofs0",
		.addrCode = 0x8100,
		.nrArgs = 0,
	},
	{
		.name = "ofs1", 
		.addrCode = 0x8180,
		.nrArgs = 0,
	},
	{
		.name = "abselect0",
		.addrCode = 0x8300,
		.nrArgs = 0,
	},
	{
		.name = "abselect1",
		.addrCode = 0x8380,
		.nrArgs = 0,
	},
	{
		.name = "abselect2",
		.addrCode = 0x8400,
		.nrArgs = 0,
	},
	{
		.name = "abselect3",
		.nrArgs = 0,
		.addrCode = 0x8480,
	}
};

typedef struct WriteVar {
        const char  *name;
	uint16_t sfCode;
        uint8_t nrArgs;
} WriteVar;

static WriteVar wrvars[] = {
	{
		.name = "control",
		.sfCode = 1,
	},
	{
		.name = "ofs0",
		.sfCode = 2,
	},
	{
		.name = "ofs1",
		.sfCode = 3,
	},
	{
		.name = "abselect0",
		.sfCode = 6,
	},
	{
		.name = "abselect1",
		.sfCode = 7,
	},
	{
		.name = "abselect2",
		.sfCode = 8,
	},
	{
		.name = "abselect3",
		.sfCode = 9,
	}
};


static uint32_t  
AD537x_Write(uint32_t value)
{
	uint32_t i;
	uint32_t inval;
	inval = 0;
	SYNC_LOW;
	for(i = UINT32_C(0x00800000); i > 0;i >>= 1) {
		if(value & i) {
			//Con_Printf("1");
			SDI_HIGH;
		} else {
			//Con_Printf("0");
			SDI_LOW;
		}
		if(SDO_READ) { 
			inval = (inval << 1) | 1;
		} else {
			inval = (inval << 1);
		}
		//asm("nop");
		//asm("nop");
		//asm("nop");
		SCLK_LOW;
		asm("nop");
		asm("nop");
		asm("nop");
		SCLK_HIGH;
	}		
	SYNC_HIGH;
	//Con_Printf("\n");
	return inval;
}

void
DAC_Set(uint16_t channel,uint16_t value)
{
	AD537x_Write(value | ((uint32_t)(channel + 8) << 16) | (UINT32_C(3) << 22));
}

static inline void 
AD537x_SFWrite(uint8_t sfc,uint16_t value)
{
	uint32_t wrval;
	wrval = value | ((uint32_t)sfc << 16); 
	Con_Printf("Write %06lx\n",wrval);
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
cmd_dacr(Interp *interp,uint8_t argc,char *argv[])
{
	RESET_LOW;
	//_delay_us(1);
	RESET_HIGH;
	//_delay_us(300);
	return 0;
}

static int8_t
cmd_dac(Interp *interp,uint8_t argc,char *argv[])
{
	uint8_t i;
	uint8_t channel;
	ReadbackVar *rbvar; 
	WriteVar *wrvar;
	uint16_t value;
	if(argc < 2) {
		for(i = 0; i < array_size(rbvars); i++) {
			rbvar = &rbvars[i];
			if(rbvar->nrArgs) {
			//	value = AD537x_Readback(rbvar.addrCode,0);
			//	Con_Printf("%S(%04x): 0x%04x\n",rbvar.name,rbvar.addrCode,value);
			} else {
				value = AD537x_Readback(rbvar->addrCode,0);
				Con_Printf("%s(%04x): 0x%04x\n",rbvar->name,rbvar->addrCode,value);
			}		
		}
		return 0;
	}
	for(i = 0; i < array_size(rbvars); i++) {
		rbvar = &rbvars[i];
		if((strcmp(argv[1],rbvar->name) == 0) 
			&& (rbvar->nrArgs + 2) == argc) 
		{
			if(rbvar->nrArgs) {
				channel = astrtoi16(argv[2]);
			} else {
				channel = 0;
			}		
			value = AD537x_Readback(rbvar->addrCode,channel);
			Con_Printf("0x%04x\n",value);
			return 0;
		}
	}
	/* Now try to write the register */
	for(i = 0; i < array_size(wrvars); i++) {
		wrvar = &wrvars[i];
		if((argc > 2) && (strcmp(argv[1],wrvar->name) == 0)) {
			value = astrtoi16(argv[2]);	
			AD537x_SFWrite(wrvar->sfCode,value);
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
	if(argc == 1) {
		uint8_t i;
		uint16_t value;
		for(i = 0; i < 32; i++) {
			value = AD537x_Readback(0,i + 8);
			Con_Printf("%04x ",value);
			if((i % 16) == 15) {
				Con_Printf("\n");
			}
		}
		Con_Printf("\n");
		for(i = 0; i < 32; i++) {
			value = AD537x_Readback(1 << 13,i + 8);
			Con_Printf("%04x ",value);
			if((i % 16) == 15) {
				Con_Printf("\n");
			}
		}
		return 0;
	} else if(argc == 2) {
		channel = astrtoi16(argv[1]);
		value = AD537x_Readback(0,channel + 8);
		Con_Printf("0x%04x,",value);
		value = AD537x_Readback(1 << 13,channel + 8);
		Con_Printf("0x%04x\n",value);
		return 0;
	} else if(argc == 3) {
		channel = astrtoi16(argv[1]);
		value = astrtoi16(argv[2]);
		AD537x_Write(value | ((uint32_t)(channel + 8) << 16) | (UINT32_C(3) << 22));
		return 0;
	}
	return -EC_BADNUMARGS;
}

static int8_t
cmd_dacc(Interp *interp,uint8_t argc,char *argv[])
{
	uint8_t channel;
	uint16_t value;
	if(argc == 1) {
		uint8_t i;
		for(i = 0; i < 32; i++) {
			value = AD537x_Readback(2 << 13,i + 8);
			Con_Printf("%04x ",value);
			if((i % 16) == 15) {
				Con_Printf("\n");
			}
		}
		return 0;
	} else if(argc == 2) {
		channel = astrtoi16(argv[1]);
		value = AD537x_Readback(2 << 13,channel + 8);
		Con_Printf("0x%04x\n",value);
		return 0;
	} else if(argc == 3) {
		channel = astrtoi16(argv[1]);
		value = astrtoi16(argv[2]);
		AD537x_Write(value | ((uint32_t)(channel + 8) << 16) | (UINT32_C(2) << 22));
		return 0;
	}
	return -EC_BADNUMARGS;
}

static int8_t
cmd_dacm(Interp *interp,uint8_t argc,char *argv[])
{
	uint8_t channel;
	uint16_t value;
	if(argc == 1) {
		uint8_t i;
		for(i = 0; i < 32; i++) {
			value = AD537x_Readback(3 << 13,i + 8);
			Con_Printf("%04x ",value);
			if((i % 16) == 15) {
				Con_Printf("\n");
			}
		}
		return 0;
	} else if(argc == 2) {
		channel = astrtoi16(argv[1]);
		value = AD537x_Readback(3 << 13,channel + 8);
		Con_Printf("0x%04x\n",value);
		return 0;
	} else if(argc == 3) {
		channel = astrtoi16(argv[1]);
		value = astrtoi16(argv[2]);
		AD537x_Write(value | ((uint32_t)(channel + 8) << 16) | (UINT32_C(1) << 22));
	}
	return 0;
}

INTERP_CMD(dacCmd, "dac", cmd_dac,
           "dac <regname> ?<channel>? ?<value>? # ");

INTERP_CMD(dacxCmd, "dacx", cmd_dacx,
           "dacx <channel> <value> # ");

INTERP_CMD(daccCmd, "dacc" , cmd_dacc,
           "dacc <channel> <value> # ");

INTERP_CMD(dacmCmd, "dacm", cmd_dacm,
           "dacm <channel> <value> # ");

INTERP_CMD(dacrCmd, "dacr", cmd_dacr,
           "dacr                   # reset dac");

void
AD537x_Init(void)
{
	RESET_HIGH;
	RESET_DIROUT;

	SYNC_DIROUT;
	SYNC_HIGH;

	SDI_DIROUT;

	SCLK_DIROUT;
	SCLK_HIGH;

	SDO_DIRIN;

	CLR_HIGH;
	CLR_DIROOUT;

	LDAC_HIGH;
	LDAC_DIROUT;

	Interp_RegisterCmd(&dacCmd);
	Interp_RegisterCmd(&dacxCmd);
	Interp_RegisterCmd(&daccCmd);
	Interp_RegisterCmd(&dacmCmd);
	Interp_RegisterCmd(&dacrCmd);
}
