/***
 **************************************************************************************
 * Interface to CDR Registers
 **************************************************************************************
 */

#include <stdint.h> 
#include "mdio.h"
#include "cdr.h"
#include "interpreter.h"
#include "hex.h"
#include "console.h"

/* The INPHY CDR's are of devive type 0x30 which means "Vendor specific" */
#define DEVTYPE		(0x30)

typedef struct CdrRegister {
	uint16_t regNo;	
} CdrRegister;

#define NAME(x,y)	 

/**
 ******************************************************************
 * This is the list of all CDR registers with name
 * and address.  
 ******************************************************************
 */
static const CdrRegister gCdrRegisters[] = 
{
	{
		NAME("bla",)
		.regNo = 0,
	}
};

/**
 ************************************************************************
 * \fn void Cdr_Write(uint8_t phyAddr,uint16_t regAddr,uint16_t value)
 * Write to the CDR by setting up the Address with the MDIO Address cmd.
 * and then writing a 16bit word.
 ************************************************************************
 */
void
Cdr_Write(uint8_t phyAddr,uint16_t regAddr,uint16_t value)
{
	MDIO_Address(phyAddr,DEVTYPE,regAddr);
	MDIO_Write(phyAddr,DEVTYPE,value);
}

/**
 ************************************************************************
 * \fn void Cdr_Read(uint8_t phyAddr,uint16_t regAddr,uint16_t value)
 * Write to the CDR by setting up the Address with the MDIO Address cmd.
 * and then writing a 16bit word.
 ************************************************************************
 */
uint16_t 
Cdr_Read(uint8_t phyAddr,uint16_t regAddr)
{
	MDIO_Address(phyAddr,DEVTYPE,regAddr);
	return MDIO_Read(phyAddr,DEVTYPE);
}

/**
 ************************************************************************
 * \fn static int8_t cmd_cdr(Interp * interp, uint8_t argc, char *argv[])
 * Allows to read or write registers of a CDR from the command shell
 **************************************************************************
 */
static int8_t
cmd_cdr(Interp * interp, uint8_t argc, char *argv[])
{
	uint8_t phyAddr;
        uint16_t val;
        uint16_t regAddr;
        if(argc == 3) {
                phyAddr = astrtoi16(argv[1]);
                regAddr = astrtoi16(argv[2]);
                val = Cdr_Read(phyAddr,regAddr);
                Con_Printf_P("%u.%u: 0x%x\n",phyAddr,regAddr,val);
                return 0;
        } else if(argc == 4) {
                phyAddr = astrtoi16(argv[1]);
                regAddr = astrtoi16(argv[2]);
                val = astrtoi16(argv[3]);
		Cdr_Write(phyAddr,regAddr,val);
        }
        return 0;

}

INTERP_CMD(cdr, cmd_cdr, "cdr <cdrAddr> <regAddr> ?<value>?   # read write to/from cdr");

void
Cdr_Init() 
{
		
}
