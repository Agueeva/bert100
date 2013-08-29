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
} SiXO;

static SiXO gSiXO[1];

static void
set_frequency(const char *strFreq) 
{
		
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
	Con_Printf("Not implemented\n");
        return 0;
}
INTERP_CMD(xoCmd,"xo", cmd_xo, "xo  ?<freq>? # Set/Get frequency of XO");

void
XO_Init(const char *name,uint8_t i2cAddr) 
{
	SiXO *si = &gSiXO[0];	
	si->i2cAddr = i2cAddr;	
	Interp_RegisterCmd(&xoCmd);
}
