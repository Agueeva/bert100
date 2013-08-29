/**
 **********************************************************************************
 * Interface to the Silabs Si570 Programmable Oscilator XO/VXCO
 **********************************************************************************
 */

#include "types.h"
#include "pvar.h"

typedef struct SiVXCO {
	uint8_t i2cAddr;
} SiVXCO;

static SiVXCO gSiVXCO[1];

void
VXCO_Init(const char *name,uint8_t i2cAddr) 
{
	SiVXCO *si = &gSiVXCO[0];	
	si->i2cAddr = i2cAddr;	
}
