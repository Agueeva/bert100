/**
 ***********************************************************************************
 * Module for Shift Register with 74HC594
 * Atmel: PC0 für Shift Clock
 *        PC1 für Data 
 * 	  PC4 für Latch  
 ***********************************************************************************
 */

#include "iodefine.h"
#include "atomic.h"
#include "interpreter.h"
#include "hex.h"

#define DATA_SR_DIROUT		BSET(2,PORT1.PDR.BYTE)
#define DATA_SR(val)		BMOD(2,PORT1.PODR.BYTE,(val))
#define DATA_SR_HIGH		BSET(2,PORT1.PODR.BYTE)
#define DATA_SR_LOW		BCLR(2,PORT1.PODR.BYTE)

#define CLK_SR_DIROUT		BSET(3,PORT1.PDR.BYTE)
#define CLK_SR_HIGH		BSET(3,PORT1.PODR.BYTE)
#define CLK_SR_LOW		BCLR(3,PORT1.PODR.BYTE)
#define	CLK_SR(val)		BMOD(3,PORT1.PODR.BYTE,(val))

#define LATCH_SR_DIROUT		BSET(5,PORT1.PDR.BYTE)
#define LATCH_SR_HIGH		BSET(5,PORT1.PODR.BYTE)
#define LATCH_SR_LOW		BCLR(5,PORT1.PODR.BYTE)
#define LATCH_SR(val)		BMOD(5,PORT1.PODR.BYTE,(val))

void
ShiftReg_Out(uint16_t shiftval) 
{
	unsigned int i;	
	LATCH_SR_LOW;
	for(i = 0; i < 16; i++) {
		if(shiftval & 0x8000) {
			DATA_SR_HIGH;
		} else {
			DATA_SR_LOW;
		}
		CLK_SR_HIGH;
		shiftval <<= 1;
		CLK_SR_LOW;
	} 
	LATCH_SR_HIGH;
}

/**
 ***************************************************************************************
 * \fn static int8_t cmd_shift(Interp * interp, uint8_t argc, char *argv[])
 ***************************************************************************************
 */
static int8_t
cmd_shift(Interp * interp, uint8_t argc, char *argv[])
{
        uint16_t val;
	if(argc < 2) {
		return -EC_BADNUMARGS;
	}
	val = astrtoi16(argv[1]);
	ShiftReg_Out(val);
        return 0;
}

INTERP_CMD(shiftCmd, "shift", cmd_shift, "shift value  # shift out a value");

void
ShiftReg_Init(uint16_t initval)
{	
	CLK_SR_DIROUT;
	DATA_SR_DIROUT;
	LATCH_SR_DIROUT;
	CLK_SR_LOW;
	LATCH_SR_LOW;
	ShiftReg_Out(initval);
	Interp_RegisterCmd(&shiftCmd);
}
