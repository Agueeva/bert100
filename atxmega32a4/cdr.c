/***
 **************************************************************************************
 * Interface to CDR Registers
 **************************************************************************************
 */

#include <string.h>
#include <stdint.h> 
#include "mdio.h"
#include "cdr.h"
#include "interpreter.h"
#include "hex.h"
#include "console.h"
#include "timer.h"

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


void
Cdr_Init() 
{
		
}


/**
 ************************************************************************
 * \fn void Cdr_Init_cdr(uint16_t phy_addr=1)
 * from Python
 ************************************************************************
 */
void
Cdr_Init_cdr(uint16_t phy_addr)
{
phy_addr=1;
//regWrite('CDR1::30.0',0X0100)
Cdr_Write(phy_addr,0,0X0100);
//regWrite('CDR1::30.2',0X0210)
Cdr_Write(phy_addr,2,0X0210);
//regWrite('CDR1::30.3',0X7401)
Cdr_Write(phy_addr,3,0X7401);
//regWrite('CDR1::30.5',0X0000)
Cdr_Write(phy_addr,5,0X0000);
//regWrite('CDR1::30.6',0X4000)
Cdr_Write(phy_addr,6,0X4000);
//regWrite('CDR1::30.8',0X8000)
Cdr_Write(phy_addr,8,0X8000);
//regWrite('CDR1::30.16',0X1414)
Cdr_Write(phy_addr,16,0X1414);
//regWrite('CDR1::30.17',0X1414)
Cdr_Write(phy_addr,17,0X1414);
//regWrite('CDR1::30.18',0X1414)
Cdr_Write(phy_addr,18,0X1414);
//regWrite('CDR1::30.19',0X1414)
Cdr_Write(phy_addr,19,0X1414);
//regWrite('CDR1::30.30',0X0000)
Cdr_Write(phy_addr,30,0X0000);
//regWrite('CDR1::30.31',0X0000)
Cdr_Write(phy_addr,31,0X0000);
//regWrite('CDR1::30.32',0X0000)
Cdr_Write(phy_addr,32,0X0000);
//regWrite('CDR1::30.33',0XFFFF)
Cdr_Write(phy_addr,33,0XFFFF);
//regWrite('CDR1::30.37',0X4700)
Cdr_Write(phy_addr,37,0X4700);
//regWrite('CDR1::30.38',0X0000)
Cdr_Write(phy_addr,38,0X0000);
//regWrite('CDR1::30.44',0XFC00)
Cdr_Write(phy_addr,44,0XFC00);
//regWrite('CDR1::30.45',0X0000)
Cdr_Write(phy_addr,45,0X0000);
//regWrite('CDR1::30.48',0X0000)
Cdr_Write(phy_addr,48,0X0000);
//regWrite('CDR1::30.49',0X0000)
Cdr_Write(phy_addr,49,0X0000);
//regWrite('CDR1::30.50',0X0000)
Cdr_Write(phy_addr,50,0X0000);
//regWrite('CDR1::30.51',0X0000)
Cdr_Write(phy_addr,51,0X0000);
//regWrite('CDR1::30.256',0X0006)
Cdr_Write(phy_addr,256,0X0006);
//regWrite('CDR1::30.512',0X0006)
Cdr_Write(phy_addr,512,0X0006);
//regWrite('CDR1::30.768',0X0006)
Cdr_Write(phy_addr,768,0X0006);
//regWrite('CDR1::30.1024',0X0006)
Cdr_Write(phy_addr,1024,0X0006);
//regWrite('CDR1::30.257',0X0000)
Cdr_Write(phy_addr,257,0X0000);
//regWrite('CDR1::30.513',0X0000)
Cdr_Write(phy_addr,513,0X0000);
//regWrite('CDR1::30.769',0X0000)
Cdr_Write(phy_addr,769,0X0000);
//regWrite('CDR1::30.1025',0X0000)
Cdr_Write(phy_addr,1025,0X0000);
//regWrite('CDR1::30.258',0X0000)
Cdr_Write(phy_addr,258,0X0000);
//regWrite('CDR1::30.514',0X0002)
Cdr_Write(phy_addr,514,0X0002);
//regWrite('CDR1::30.770',0X0000)
Cdr_Write(phy_addr,770,0X0000);
//regWrite('CDR1::30.1026',0X0000)
Cdr_Write(phy_addr,1026,0X0000);
//regWrite('CDR1::30.384',0X0004)
Cdr_Write(phy_addr,384,0X0004);
//regWrite('CDR1::30.640',0X0004)
Cdr_Write(phy_addr,640,0X0004);
//regWrite('CDR1::30.896',0X0004)
Cdr_Write(phy_addr,896,0X0004);
//regWrite('CDR1::30.1152',0X0004)
Cdr_Write(phy_addr,1152,0X0004);
//regWrite('CDR1::30.387',0X8405)
Cdr_Write(phy_addr,387,0X8405);
//regWrite('CDR1::30.643',0X8405)
Cdr_Write(phy_addr,643,0X8405);
//regWrite('CDR1::30.899',0X8405)
Cdr_Write(phy_addr,899,0X8405);
//regWrite('CDR1::30.1155',0X8405)
Cdr_Write(phy_addr,1155,0X8405);
//regWrite('CDR1::30.416',0X0000)
Cdr_Write(phy_addr,416,0X0000);
//regWrite('CDR1::30.672',0X0000)
Cdr_Write(phy_addr,672,0X0000);
//regWrite('CDR1::30.928',0X0000)
Cdr_Write(phy_addr,928,0X0000);
//regWrite('CDR1::30.1184',0X0002)
Cdr_Write(phy_addr,1184,0X0002);
//regWrite('CDR1::30.421',0X000E)
Cdr_Write(phy_addr,421,0X000E);
//regWrite('CDR1::30.677',0X000E)
Cdr_Write(phy_addr,677,0X000E);
//regWrite('CDR1::30.933',0X000E)
Cdr_Write(phy_addr,933,0X000E);
//regWrite('CDR1::30.1189',0X000E)
Cdr_Write(phy_addr,1189,0X000E);
//regWrite('CDR1::30.422',0X0000)
Cdr_Write(phy_addr,422,0X0000);
//regWrite('CDR1::30.678',0X0000)
Cdr_Write(phy_addr,678,0X0000);
//regWrite('CDR1::30.934',0X0000)
Cdr_Write(phy_addr,934,0X0000);
//regWrite('CDR1::30.1190',0X0000)
Cdr_Write(phy_addr,1190,0X0000);
//regWrite('CDR1::30.423',0X2043)
Cdr_Write(phy_addr,423,0X2043);
//regWrite('CDR1::30.679',0X2029)
Cdr_Write(phy_addr,679,0X2029);
//regWrite('CDR1::30.935',0X1FFA)
Cdr_Write(phy_addr,935,0X1FFA);
//regWrite('CDR1::30.1191',0X2000)
Cdr_Write(phy_addr,1191,0X2000);
//regWrite('CDR1::30.424',0X025B)
Cdr_Write(phy_addr,424,0X025B);
//regWrite('CDR1::30.680',0X022B)
Cdr_Write(phy_addr,680,0X022B);
//regWrite('CDR1::30.936',0X0359)
Cdr_Write(phy_addr,936,0X0359);
//regWrite('CDR1::30.1192',0X03E2)	
Cdr_Write(phy_addr,1192,0X03E2);
}
/**
************************************************************************
* \fn void iShuffleRead(unsigned* uOutp, unsigned uInp,unsigned uStartBit,unsigned uNumBit)
{
	* from Python Liest uNumBit aus  uInp von uStartBit
************************************************************************
*/

void iShuffleRead(uint16_t* uOutp, uint16_t uInp,uint8_t uStartBit,uint8_t uNumBit)
 {
  uint16_t uMask = 0;
  volatile uint8_t i;
  if(uStartBit + uNumBit > 16)
  {
	  uNumBit=16-uStartBit; // bits numbers from 0
  }
 for(i=0;i<uNumBit;i++)
 {
    uMask=(uMask<<1)|1;
   }

 
  uInp>>=uStartBit;
  *uOutp=(uInp&uMask);
 }

/**
************************************************************************
* \fn void iShuffle(uint16_t* uOutp, uint16_t uInp,uint8_t uStartBit,uint8_t uNumBit,uint8_t uNumber)
{
* from Python: Ersetzt uNumBit in  uInp von uStartBit auf uNumber
************************************************************************
*/

void iShuffle(uint16_t* uOutp, uint16_t uInp,uint8_t uStartBit,uint8_t uNumBit,uint8_t uNumber)
{
	uint16_t uMask = 0;
	volatile uint8_t i;
	if(uStartBit + uNumBit > 16)
	{
		uNumBit=16-uStartBit; // bits numbers from 0
	}
	for(i=0;i<uNumBit;i++)
	{
		uMask=(uMask<<1)|1;
	}
	
	uMask<<=uStartBit;
	uNumber<<=uStartBit;
	
	*uOutp=(uInp&(~uMask))|uNumber;
	
}
/**
************************************************************************
* \fn void Cdr_Write_Part(uint16_t phy_addr,uint16_t uRegister,uint8_t uNumBit,uint8_t uStartBit,uint8_t uNumber)
* from Python startup_cdr.py
************************************************************************
*/

void Cdr_Write_Part(uint16_t phy_addr,uint16_t uRegister,uint8_t uNumBit,uint8_t uStartBit,uint8_t uNumber)
{
	uint16_t uVal;
	uVal=Cdr_Read(phy_addr,uRegister);
	iShuffle(&uVal, uVal,uStartBit,uNumBit,uNumber);
	Cdr_Write(phy_addr,uRegister,uVal);   
}
/**
************************************************************************
* \fn void Cdr_Read_Part(uint16_t phy_addr,uint16_t uRegister,uint8_t uNumBit,uint8_t uStartBit)
* from Python startup_cdr.py
************************************************************************
*/

void Cdr_Read_Part(uint16_t* uOutp,uint16_t phy_addr,uint16_t uRegister,uint8_t uNumBit,uint8_t uStartBit)
{
uint16_t uVal;
uVal=Cdr_Read(phy_addr,uRegister);
iShuffleRead(&uVal, uVal,uStartBit,uNumBit);
 *uOutp=uVal;
}

/**
************************************************************************
* \fn void Cdr_startup(uint16_t phy_addr=1)
* from Python startup_cdr.py
************************************************************************
*/

uint8_t
Cdr_startup(uint16_t phy_addr)  // Olga
{
//# Startup for CDR A0 and B0
//
//A0_device = 0
//
//startup_dictionary = locals()
//
//device = startup_dictionary["target"] + "::"
//print "Using device ",device
//
//if regRead(device + '30.3.15:4') == 0x740:
//print "CDR device detected"
//else:
//raise Exception("No CDR device detected at the specified PHYADDR")
uint16_t uVal;
uint8_t status=1;
//phy_addr=1;
Cdr_Read_Part(&uVal,phy_addr,3,15,4);
if (uVal==0x740) {
	
	
}
else {}


//
//if regRead(device + '30.3.3:0') == 0:             # A0 device found, use the read after write fix for clock park
//print "A0 device detected. Clock park correction enabled."
//A0_device = 1
//from ipmdio import regWriteAck as regWrite
//else:
//from ipmdio import regWrite
//
//status = 1
//
//print "Resetting the CDR device using startup "+VERSION+", initiate bring-up..."
//
//# Reset the device, assume normal bring-up
//
//regWrite(device + "30.0",0x1020) # Hard reset (bit 5) and MDIO init (bit 12)
Cdr_Write(phy_addr,0,0x1020);  //Hard reset (bit 5) and MDIO init (bit 12)

//
//regWrite(device + "30.0",0x0200) # Datapath soft reset (bit 9)
Cdr_Write(phy_addr,0,0x200); //Datapath soft reset (bit 9)
//
//regWrite(device + "30.385.10:8",4)      # Set 128 steps for PI resolution - inside reset"
//mod_field(uint8_t *buf,uint8_t value,uint8_t bits,uint8_t firstbit)
//

Cdr_Write_Part(phy_addr,385,10,8,4);
//Cdr_Write_Part(uint16_t phy_addr,uint16_t uRegister,uint8_t uNumBit,uint8_t uStartBit,uint8_t uNumber)

//regWrite(device + "30.641.10:8",4)
Cdr_Write_Part(phy_addr,641,10,8,4);

//regWrite(device + "30.897.10:8",4)
Cdr_Write_Part(phy_addr,897,10,8,4);

//regWrite(device + "30.1153.10:8",4)
Cdr_Write_Part(phy_addr,1153,10,8,4);

//regWrite(device + "30.0",0x0000)
//
//# CDR recal of TxPLL while PI3 is locked
//regWrite(device + "30.1184.1",1)        # Locking Rx3 PI
Cdr_Write_Part(phy_addr,1184,1,0,1);
//# Re-Calibrating Tx PLL
//regWrite(device + "30.39.15",0)         # normal mode
Cdr_Write_Part(phy_addr,39,15,0,0);

//regWrite(device + "30.39.15",1)         # re-calibrate
Cdr_Write_Part(phy_addr,39,15,0,1);

//regWrite(device + "30.39.15",0)         # normal mode
Cdr_Write_Part(phy_addr,39,15,0,0);
//time.sleep(0.1) 
SleepMs(100);                        // wait for it to lock
//regWrite(device + "30.1184.1",0)        # and unlock PI3
Cdr_Write_Part(phy_addr,1184,1,0,0);
SleepMs(100);
//time.sleep(0.1)
//
//for lane in range(4):                   # EQ offset override is set
//regWrite(device + "30." + str(441 + 256 * lane) + ".0",1)
volatile uint8_t i;
for(i = 0; i < 4; i++) {
	Cdr_Write_Part(phy_addr,441+256*i,0,0,1);  //Olga  Achtung!!!!!! Die Länge ist 0
}

//
//if A0_device == 1:
//print "Implemening swizzle sampler offset correction (only for A0/A1 stepping)"
//execfile("../scripts/correct_offsets.py",globals(),startup_dictionary)
//for lane in range(4):                 # Set 128 steps here for A0 (not inside reset)
//regWrite(device + "30." + str(385 + 256 * lane) + ".10:8",4)
for(i = 0; i < 4; i++) {
	Cdr_Write_Part(phy_addr,385+256*i,10,8,4);
}
//
//regWrite(device + "30.37.15",1) # Asserts FIFO reset

Cdr_Write_Part(phy_addr,37,15,0,1);
//regWrite(device + "30.37.14",1) # Enable auto-reset

Cdr_Write_Part(phy_addr,37,14,0,1);
//regWrite(device + "30.37.15",0) # De-Asserts FIFO reset

Cdr_Write_Part(phy_addr,37,15,0,0);
//
//# This section just checks things did complete are startup OK
//
//reply = regRead(device + "30.42.8")

Cdr_Read_Part(&uVal,phy_addr,42,8,0);
//print "Rx PLL Lock Status should be 1:" + str(reply)
//if reply==0:
//print "***  Rx PLL not locked, is REFCLK present?"
//status = 0
//
if (uVal==0)
{
	status = 0;
}

//reply = regRead(device + "30.40.8")

Cdr_Read_Part(&uVal,phy_addr,40,8,0);

//print "Tx PLL Lock Status should be 1:" + str(reply)
//if reply==0:
//print "***  Tx PLL not locked, is REFCLK present and Rx3 PI locked?"
//status = 0
//
if (uVal==0)
{
	status = 0;
}

//reply = regRead(device + "30.0.8")

Cdr_Read_Part(&uVal,phy_addr,0,8,0);

//print "GB Tx or CDR Reset sequence done:" + str(reply)
//if reply == 0:
//print "***  Part did not complete GB Tx or CDR reset sequence"
//status = 0
//
if (uVal==0)
{
	status = 0;
}
//print "Enabling calibrated EQ offsets"
//for lane in range(4):
//regWrite(device + "30." + str(441 + 256 * lane) + ".0",1)
for(i = 0; i < 4; i++) {
	Cdr_Write_Part(phy_addr,441+256*i,0,0,1);  // Achtung!!!!!! Die Länge ist 0
}

//
//if status == 1:
//print "The part is now ready for testing."
//else:
//print "The part is not ready, expect limited or no functionality"
//

return status;
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
	if((argc == 3) && (strcmp(argv[1],"startup")  == 0)) {
		uint8_t cdr = astrtoi16(argv[2]);
		Cdr_startup(cdr);
		Con_Printf_P("Called olgas CDR_Startup for CDR %u\n",cdr);
		return 0;
	} else if(argc == 3) {
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
