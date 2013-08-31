/**
 **********************************************************************************
 * Interface to the Silabs Si570 Programmable Oscilator XO
 **********************************************************************************
 */

#include "types.h"
#include "pvar.h"
#include "interpreter.h"
#include "console.h"
#include "hex.h"
#include "xo.h"

typedef struct SiXO {
	uint8_t i2cAddr;
	uint32_t fXTAL;
} SiXO;

static SiXO gSiXO[1];

/**
 *************************************************************************
 * the list of possible hsdiv dividers (not the register values) .
 *************************************************************************
 */
static const uint8_t hsdiv_tab[] = {
	4, 5, 6, 7, 9, 11
};

static bool 
set_frequency(SiXO *xo,uint64_t freq) 
{
	int i;
	uint32_t n1,hs_div;
	uint64_t rfreq;
	uint32_t fxtal = xo->fXTAL;
	uint64_t maxdco = UINT64_C(5670000000);
	uint64_t mindco = UINT64_C(4850000000);
	uint64_t optdco = (maxdco + mindco) >> 1; 
	//uint64_t optrfreq = optdco / fxtal;
	uint64_t optDividers = optdco / freq;
	Con_Printf("Optimal dividers %lu\n",(uint32_t)optDividers);
	for(i = 0; i < array_size(hsdiv_tab); i++) {
		hs_div = hsdiv_tab[i];
		n1 = optDividers / hs_div;	
		if((n1 >= 1) && (n1 < 129)) {
			break;
		}
	}
	if(i == array_size(hsdiv_tab)) {
		Con_Printf("No hsdiv_match\n");
		return false;
	}
	Con_Printf("n1 %lu hsdiv %lu\n",n1,hs_div);
	rfreq = (1 << 28) * freq / fxtal * hs_div * n1;
	Con_Printf("rfreq %llx\n",rfreq);
	return true;
}
/**
 ******************************************************************************
 * \fn static int8_t cmd_vxco(Interp * interp, uint8_t argc, char *argv[])
 * Command shell interface for controlling the Silabs Si570 Oscillator 
 ******************************************************************************
 */

static int8_t
cmd_xo(Interp * interp, uint8_t argc, char *argv[])
{
	SiXO *xo = &gSiXO[0];	
	uint64_t freq;		
	if(argc < 2) {
		Con_Printf("Not implemented\n");
		return 0;
	}
	freq = astrtoi64(argv[1]);
	set_frequency(xo,freq);
        return 0;
}
INTERP_CMD(xoCmd,"xo", cmd_xo, "xo  ?<freq>? # Set/Get frequency of XO");

void
XO_Init(const char *name,uint8_t i2cAddr) 
{
	SiXO *xo = &gSiXO[0];	
	xo->i2cAddr = i2cAddr;	
	xo->fXTAL = 114285000;
	Interp_RegisterCmd(&xoCmd);
}
