#include <string.h>
#include "types.h"
#include "console.h"
#include "events.h"
#include "interpreter.h"
#include "hex.h"
#include "ad537x.h"
#include "atomic.h"
#include "iodefine.h"
#include "config.h"
#include "timer.h"
#include "pvar.h"

#define array_size(x) (sizeof(x) / sizeof((x)[0]))

#define SYNC_DIROUT	BSET(0,PORT5.PDR.BYTE)
#define SYNC_HIGH	BSET(0,PORT5.PODR.BYTE)
#define SYNC_LOW	BCLR(0,PORT5.PODR.BYTE)	

#define SDI_DIROUT	BSET(6,PORTC.PDR.BYTE)
#define SDI_HIGH	BSET(6,PORTC.PODR.BYTE)	
#define SDI_LOW		BCLR(6,PORTC.PODR.BYTE)

#define SCLK_DIROUT	BSET(1,PORT5.PDR.BYTE)
#define SCLK_HIGH	BSET(1,PORT5.PODR.BYTE)
#define SCLK_LOW	BCLR(1,PORT5.PODR.BYTE)

#define SDO_DIRIN	BCLR(5,PORTC.PDR.BYTE)
#define SDO_READ	PORTC.PIDR.BIT.B5	

#define RESET_DIROUT	BSET(2,PORTC.PDR.BYTE)
#define RESET_HIGH	BSET(2,PORTC.PODR.BYTE)
#define RESET_LOW	BCLR(2,PORTC.PODR.BYTE)

#define CLR_DIROUT	BSET(3,PORTC.PDR.BYTE)
#define CLR_HIGH	BSET(3,PORTC.PODR.BYTE)
#define CLR_LOW		BCLR(3,PORTC.PODR.BYTE)

#define LDAC_DIROUT	BSET(4,PORTC.PDR.BYTE)
#define LDAC_HIGH	BSET(4,PORTC.PODR.BYTE)
#define LDAC_LOW	BCLR(4,PORTC.PODR.BYTE)

#define NR_CHANNELS	(32)

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

typedef struct DacAlias {
	const char *name;
	uint8_t flags;
} DacAlias;

#define FLG_READABLE 	(1)
#define FLG_WRITABLE 	(2)

const DacAlias dac0AliasesEml[NR_CHANNELS] = {
	{NULL,0},
	{NULL,0},
	{NULL,0},
	{NULL,0},
	{"emlAmp1.vg1", FLG_READABLE | FLG_WRITABLE },
	{"emlAmp2.vg1", FLG_READABLE | FLG_WRITABLE },
	{"emlAmp3.vg1", FLG_READABLE | FLG_WRITABLE },
	{"emlAmp4.vg1", FLG_READABLE | FLG_WRITABLE },
	{"emlAmp1.vg2", FLG_READABLE | FLG_WRITABLE },
	{"emlAmp2.vg2", FLG_READABLE | FLG_WRITABLE },
	{"emlAmp3.vg2", FLG_READABLE | FLG_WRITABLE },
	{"emlAmp4.vg2", FLG_READABLE | FLG_WRITABLE },
	{NULL, 0},
	{NULL, 0},
	{NULL, 0},
	{NULL, 0},
	{"emlAmp1.vd1", FLG_READABLE },
	{"emlAmp2.vd1", FLG_READABLE },
	{"emlAmp3.vd1", FLG_READABLE },
	{"emlAmp4.vd1", FLG_READABLE },
	{"emlAmp1.vd2", FLG_READABLE },
	{"emlAmp2.vd2", FLG_READABLE },
	{"emlAmp3.vd2", FLG_READABLE },
	{"emlAmp4.vd2", FLG_READABLE },
	{NULL, 0},
	{NULL, 0},
	{NULL, 0},
	{NULL, 0},
	{NULL, 0},
	{NULL, 0},
	{NULL, 0},
	{NULL, 0},
};	

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
			SDI_HIGH;
		} else {
			SDI_LOW;
		}
		SCLK_HIGH;
		asm("nop");
		asm("nop");
		asm("nop");
		SCLK_LOW;
		if(SDO_READ) { 
			inval = (inval << 1) | 1;
		} else {
			inval = (inval << 1);
		}
	}		
	SYNC_HIGH;
	return inval;
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
	DelayNs(270);
	result = AD537x_Write(spiCmd);
	return result;	
}

/**
 * Per channel gain 
 */ 
static void
DACM_Set(uint16_t channel,uint16_t value)    
{
	AD537x_Write(value | ((uint32_t)(channel + 8) << 16) | (UINT32_C(1) << 22));
}
/**
 * Per channel offset
 */ 
static void
DACC_Set(uint16_t channel,uint16_t value)    
{
	/* 2 is an offset register */
	AD537x_Write(value | ((uint32_t)(channel + 8) << 16) | (UINT32_C(2) << 22));
}

/**
 * Per channel input code 
 */ 
static void
DACX_Set(uint16_t channel,uint16_t value)    
{
	AD537x_Write(value | ((uint32_t)(channel + 8) << 16) | (UINT32_C(3) << 22));
}

static void
DACOFS0_Write(uint16_t value) {
	uint16_t addrCode = 2;
	AD537x_SFWrite(addrCode,value);
}

static void
DACOFS1_Write(uint16_t value) {
	uint16_t addrCode = 3;
	AD537x_SFWrite(addrCode,value);
}

bool 
DAC_Set(uint8_t chNr,float value)
{
	int32_t inputCode = ((value * 65536 + 10) / 20) + 32768;  		
	if(chNr > 31) {
		return false;
	}
	if(inputCode > 65535) {
		if(inputCode > 65540) {
			return false;
		} else {
			inputCode = 65535;
		}
	} else if(inputCode < 0){
		if(inputCode < -5) {
			return false;
		} else {
			inputCode = 0;
		}
	}
	DACX_Set(chNr,inputCode);
	return true;
}

static bool 
DAC_Get(uint8_t chNr,float *valRet)
{
	float anaVOut;
	uint8_t channelAddr = chNr + 8;
	uint16_t inputCode; 
	uint16_t M = 65535;
	uint16_t C = 32768;
	int32_t dacCode;
	int32_t offsCode = 8192;
	if(chNr > 31) {
		*valRet = 0;
		return false;
	}
	inputCode = AD537x_Readback(0,channelAddr); /* Read X1A of channel */
	dacCode = (int64_t)inputCode * ((uint32_t)M + 1) / 65536 + C - 32768;
	anaVOut = (4 * 5.0 * (dacCode - (offsCode * 4))) / 65536 /* + ad->anaVSigGnd */;
	*valRet = anaVOut;
	return true;
}
static int8_t
cmd_dac(Interp *interp,uint8_t argc, char *argv[]) 
{
	float val;	
	bool result;
	uint8_t channel;
	if(argc == 3) {
		channel = astrtoi16(argv[1]);
		val = astrtof32(argv[2]);
	 	result = DAC_Set(channel,val);
		if(result == false) {
			Con_Printf("Failed to set DAC %u\n",channel);
			return 0;
		} else {
			return 0;
		}
	} else if(argc == 2) {
		channel = astrtoi16(argv[1]);
		result = DAC_Get(channel,&val);
		if(result == true) {
			Con_Printf("DAC Ch%u: %f\n",channel,val);
		} else {
			Con_Printf("DAC Ch%u not readable\n",channel);
		}
		return 0;
	}
	return -EC_BADNUMARGS;
}

static int8_t
cmd_dacrreset(Interp *interp,uint8_t argc,char *argv[])
{
	RESET_LOW;
	DelayNs(1000);
	RESET_HIGH;
	return 0;
}

static int8_t
cmd_dacreg(Interp *interp,uint8_t argc,char *argv[])
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

INTERP_CMD(dacregCmd, "dacreg", cmd_dacreg,
           "dacreg <regname> ?<channel>? ?<value>? # ");

INTERP_CMD(dacxCmd, "dacx", cmd_dacx,
           "dacx <channel> <value> # ");

INTERP_CMD(daccCmd, "dacc" , cmd_dacc,
           "dacc <channel> <value> # ");

INTERP_CMD(dacmCmd, "dacm", cmd_dacm,
           "dacm <channel> <value> # ");

INTERP_CMD(dacresetCmd, "dacreset", cmd_dacrreset,
           "dacrreset                   # reset dac");

INTERP_CMD(dacCmd, "dac", cmd_dac,
           "dac                  # write to dac");

static bool 
PVDac_Set (void *cbData, uint32_t chNr, const char *strP)
{
	float val;	
	val = astrtof32(strP);
	return DAC_Set(chNr,val);
}

static bool 
PVDac_Get (void *cbData, uint32_t chNr, char *bufP,uint16_t maxlen)
{
	float dacval;
        uint8_t cnt;
	bool retval;
	retval = DAC_Get(chNr,&dacval);
        cnt = f32toa(dacval,bufP,maxlen);
        bufP[cnt] = 0;
        return true;
}

void
AD537x_ModInit(const char *name)
{
	int ch;
	RESET_HIGH;
	RESET_DIROUT;

	SCLK_DIROUT;
	SCLK_LOW;

	SYNC_DIROUT;
	SYNC_HIGH;

	SDI_DIROUT;

	SDO_DIRIN;

	CLR_HIGH;
	CLR_DIROUT;

	LDAC_HIGH;
	LDAC_DIROUT;
	DACOFS0_Write(0x2000);
	DACOFS1_Write(0x2000);
 	for(ch = 0; ch < NR_CHANNELS; ch++) {
		uint16_t value;
		const DacAlias *alias;
		value = 0xffff;
		DACM_Set(ch,value);
		value = 0x8000;
		DACX_Set(ch,value);
		value = 0x8000;
		DACC_Set(ch,value);
		alias = &dac0AliasesEml[ch];
		if(alias->name) {
			if(alias->flags == (FLG_READABLE | FLG_WRITABLE)) {
				PVar_New(PVDac_Get,PVDac_Set,NULL,ch,alias->name);
			} else if(alias->flags == FLG_READABLE) {
				PVar_New(PVDac_Get,NULL,NULL,ch,alias->name);
			}
		}
		PVar_New(PVDac_Get,PVDac_Set,NULL,ch,"dac0.ch%d",ch);
	}
		
	LDAC_LOW;
	LDAC_DIROUT;

	Interp_RegisterCmd(&dacregCmd);
	Interp_RegisterCmd(&dacxCmd);
	Interp_RegisterCmd(&daccCmd);
	Interp_RegisterCmd(&dacmCmd);
	Interp_RegisterCmd(&dacCmd);
	Interp_RegisterCmd(&dacresetCmd);
}
