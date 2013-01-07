#include "calib.h"
#include <stdint.h>
#include <avr/io.h>
#include "interpreter.h"

uint8_t SP_ReadCalibrationByte(uint8_t index)
{
	uint8_t result;
	__asm__ __volatile__ (
		"ldi r20, %3    ; Load command into temp register." "\n\t"
		"mov r30, %2   ; Move index to Z pointer."         "\n\t"
		"clr r31        ; Clear ZH."                        "\n\t"
		"sts %1, r20    ; Store command to CMD register."   "\n\t"
		"lpm %0, Z      ; Load Program memory to result"    "\n\t"
		"ldi r20, %4    ; Clean up CMD register."           "\n\t"
		"sts %1, r20"
		: "=r" (result)
		: "m" (NVM_CMD),
		"r" (index),
		"M" (NVM_CMD_READ_CALIB_ROW_gc),
		"M" (NVM_CMD_NO_OPERATION_gc)
		: "r20", "r30", "r31"
        );
        return result;
}


/**
 ****************************************************************************
 * \fn static int8_t cmd_calib(Interp * interp, uint8_t argc, char *argv[])
 * Shell command for reading the Calibration data of the XMega. 
 ****************************************************************************
 */
static int8_t cmd_calib(Interp * interp, uint8_t argc, char *argv[])
{
	uint8_t i;
	for( i = 0 ;i < 0x3f; i++ ) {
		Interp_Printf_P(interp,"%02x ",SP_ReadCalibrationByte(i));
		if(((i & 15) == 15) ||  (i == 0x3e)) {
			Interp_Printf_P(interp,"\n");
		}
	}
        return 0;
}


INTERP_CMD(calib, cmd_calib,
           "calib       # dump the xmega calibration data");
                                  
