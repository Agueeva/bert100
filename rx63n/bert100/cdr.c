/***
 **************************************************************************************
 * Interface to CDR Registers
 **************************************************************************************
 */

#include <string.h>
#include "types.h"
#include "mdio.h"
#include "cdr.h"
#include "interpreter.h"
#include "hex.h"
#include "console.h"
#include "timer.h"
#include "tpos.h"

/* Register definitions for INPHY-100 CDR */
#define CDR_VS_DEVICE_CONTROL                   0
#define CDR_VS_DEVICE_IDENTIFIER2               2
#define CDR_VS_DEVICE_IDENTIFIER3               3
#define CDR_PMA_PMD_DEVICES_IN_PACKAGE5         5
#define CDR_PMA_PMD_DEVICES_IN_PACKAGE6         6
#define CDR_VS_STATUS                           8
#define CDR_VS_PACKAGE_ID14                     0xe     // read as 0
#define CDR_VS_PACKAGE_ID15                     0xf
#define CDR_LANE_PATTERN_CONTROL0               16      
#define CDR_LANE_PATTERN_CONTROL1               17
#define CDR_LANE_PATTERN_CONTROL2               18
#define CDR_LANE_PATTERN_CONTROL3               19
#define CDR_CUSTOM_TEST_PATTERN30               30
#define CDR_CUSTOM_TEST_PATTERN31               31
#define CDR_CUSTOM_TEST_PATTERN32               32
#define CDR_LOL_STATUS                          33
#define CDR_FIFO_STATUS                         37
#define CDR_TXPLL_CONFIG1                       39      
#define CDR_TXPLL_CONFIG2                       40
#define CDR_RXPLL_CONFIG1                       41
#define CDR_RXPLL_CONFIG2                       42
#define CDR_MANUAL_RESET_CONTROL                44
#define CDR_LANE0_ERROR_COUNTER                 48
#define CDR_LANE1_ERROR_COUNTER                 49
#define CDR_LANE2_ERROR_COUNTER                 50
#define CDR_LANE3_ERROR_COUNTER                 51
#define CDR_LANE0_SAMPLE0_READ                  64
#define CDR_LANE0_SAMPLE1_READ                  65      
#define CDR_LANE0_SAMPLE2_READ                  66
#define CDR_LANE1_SAMPLE0_READ                  0x44    
#define CDR_LANE1_SAMPLE1_READ                  0x45
#define CDR_LANE1_SAMPLE2_READ                  0x46
#define CDR_LANE2_SAMPLE0_READ                  0x48
#define CDR_LANE2_SAMPLE1_READ                  0x49
#define CDR_LANE2_SAMPLE2_READ                  0x4a
#define CDR_LANE3_SAMPLE0_READ                  0x4c
#define CDR_LANE3_SAMPLE1_READ                  0x4d
#define CDR_LANE3_SAMPLE2_READ                  0x4e
/* 0x100-0x17f, 0x200- 0x27f , 0x300 - 0x37f , 0x400 - 0x47f Transmitter register spaces */
#define CDR_TX_MAIN_CONTROL(lane)               (0x100 + (0x100 * (lane)))
#define CDR_TX_EQ_CONTROL(lane)                 (0x101 + (0x100 * (lane)))
#define CDR_TX_SWING_CONTROL(lane)              (0x102 + (0x100 * (lane)))
/* Receive lane register spaces */
#define CDR_RX_MAIN_CONTROL(lane)               (384 + (0x100 * (lane)))
#define CDR_RX_1ST_ORDER_CONTROL(lane)          (385 + (0x100 * (lane)))
#define CDR_RX_2ND_ORDER_CONTROL(lane)          (386 + (0x100 * (lane)))
#define CDR_RX_EQ_CONTROL(lane)                 (387 + (0x100 * (lane)))
#define CDR_RX_EYESCAN_CONTROL1(lane)           (391 + (0x100 * (lane)))
#define CDR_RX_EYESCAN_CONTROL2(lane)           (392 + (0x100 * (lane)))
#define CDR_RX_EYESCAN_RESULT(lane)             (393 + (0x100 * (lane)))
#define CDR_RX_EQ_CONTROL2(lane)                (420 + (0x100 * (lane)))
#define CDR_RX_EQ_OBSERVE1(lane)                (421 + (0x100 * (lane)))
#define CDR_RX_EQ_SETTING(lane)                 (422 + (0x100 * (lane)))
#define CDR_RX_2ND_ORDER_CDR_OBSERVE(lane)      (423 + (0x100 * (lane)))

/* The INPHY CDR's are of devive type 0x30 which means "Vendor specific" */
#define DEVTYPE		(30)

typedef struct CdrRegister {
	const char *name;
	uint16_t regNo;
	uint8_t firstBit;
	uint8_t nrBits;
} CdrRegister;

/**
 ******************************************************************
 * This is the list of the exported CDR registers with name
 * and address.  
 ******************************************************************
 */
static const CdrRegister gCdrRegisters[] = {
	{
	 .name = "regBla",
	 .regNo = 0,
	 .firstBit = 8,
	 .nrBits = 8,
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
Cdr_Write(uint8_t phyAddr, uint16_t regAddr, uint16_t value)
{
	MDIO_Address(phyAddr, DEVTYPE, regAddr);
	MDIO_Write(phyAddr, DEVTYPE, value);
}

/**
 ************************************************************************
 * \fn void Cdr_Read(uint8_t phyAddr,uint16_t regAddr,uint16_t value)
 * Write to the CDR by setting up the Address with the MDIO Address cmd.
 * and then writing a 16bit word.
 ************************************************************************
 */
uint16_t
Cdr_Read(uint8_t phyAddr, uint16_t regAddr)
{
	MDIO_Address(phyAddr, DEVTYPE, regAddr);
	return MDIO_Read(phyAddr, DEVTYPE);
}


/**
 ************************************************************************
 * \fn void Cdr_Init_cdr(uint16_t phy_addr=1)
 * from Python
 ************************************************************************
 */
static void
Cdr_InitCdr(uint16_t phy_addr)
{
	Cdr_Write(phy_addr, 0, 0X0100);
	Cdr_Write(phy_addr, 2, 0X0210);
	Cdr_Write(phy_addr, 3, 0X7401);
	Cdr_Write(phy_addr, 5, 0X0000);
	Cdr_Write(phy_addr, 6, 0X4000);
	Cdr_Write(phy_addr, 8, 0X8000);
	Cdr_Write(phy_addr, 16, 0X1414);
	Cdr_Write(phy_addr, 17, 0X1414);
	Cdr_Write(phy_addr, 18, 0X1414);
	Cdr_Write(phy_addr, 19, 0X1414);
	Cdr_Write(phy_addr, 30, 0X0000);
	Cdr_Write(phy_addr, 31, 0X0000);
	Cdr_Write(phy_addr, 32, 0X0000);
	Cdr_Write(phy_addr, 33, 0XFFFF);
	Cdr_Write(phy_addr, 37, 0X4700);
	Cdr_Write(phy_addr, 38, 0X0000);
	Cdr_Write(phy_addr, 44, 0XFC00);
	Cdr_Write(phy_addr, 45, 0X0000);
	Cdr_Write(phy_addr, 48, 0X0000);
	Cdr_Write(phy_addr, 49, 0X0000);
	Cdr_Write(phy_addr, 50, 0X0000);
	Cdr_Write(phy_addr, 51, 0X0000);
	Cdr_Write(phy_addr, 256, 0X0006);
	Cdr_Write(phy_addr, 512, 0X0006);
	Cdr_Write(phy_addr, 768, 0X0006);
	Cdr_Write(phy_addr, 1024, 0X0006);
	Cdr_Write(phy_addr, 257, 0X0000);
	Cdr_Write(phy_addr, 513, 0X0000);
	Cdr_Write(phy_addr, 769, 0X0000);
	Cdr_Write(phy_addr, 1025, 0X0000);
	Cdr_Write(phy_addr, 258, 0X0000);
	Cdr_Write(phy_addr, 514, 0X0002);
	Cdr_Write(phy_addr, 770, 0X0000);
	Cdr_Write(phy_addr, 1026, 0X0000);
	Cdr_Write(phy_addr, 384, 0X0004);
	Cdr_Write(phy_addr, 640, 0X0004);
	Cdr_Write(phy_addr, 896, 0X0004);
	Cdr_Write(phy_addr, 1152, 0X0004);
	Cdr_Write(phy_addr, 387, 0X8405);
	Cdr_Write(phy_addr, 643, 0X8405);
	Cdr_Write(phy_addr, 899, 0X8405);
	Cdr_Write(phy_addr, 1155, 0X8405);
	Cdr_Write(phy_addr, 416, 0X0000);
	Cdr_Write(phy_addr, 672, 0X0000);
	Cdr_Write(phy_addr, 928, 0X0000);
	Cdr_Write(phy_addr, 1184, 0X0002);
	Cdr_Write(phy_addr, 421, 0X000E);
	Cdr_Write(phy_addr, 677, 0X000E);
	Cdr_Write(phy_addr, 933, 0X000E);
	Cdr_Write(phy_addr, 1189, 0X000E);
	Cdr_Write(phy_addr, 422, 0X0000);
	Cdr_Write(phy_addr, 678, 0X0000);
	Cdr_Write(phy_addr, 934, 0X0000);
	Cdr_Write(phy_addr, 1190, 0X0000);
	Cdr_Write(phy_addr, 423, 0X2043);
	Cdr_Write(phy_addr, 679, 0X2029);
	Cdr_Write(phy_addr, 935, 0X1FFA);
	Cdr_Write(phy_addr, 1191, 0X2000);
	Cdr_Write(phy_addr, 424, 0X025B);
	Cdr_Write(phy_addr, 680, 0X022B);
	Cdr_Write(phy_addr, 936, 0X0359);
	Cdr_Write(phy_addr, 1192, 0X03E2);
}

/**
************************************************************************
* \fn void iShuffleRead(unsigned* uOutp, unsigned uInp,unsigned uStartBit,unsigned uNumBit)
{
	* from Python Liest uNumBit aus  uInp von uStartBit
************************************************************************
*/

static uint16_t
iShuffleRead(uint16_t uInp, uint8_t uStartBit, uint8_t uNumBit)
{
	uint16_t uMask;
	if (uStartBit + uNumBit > 16) {
		Con_Printf("CDR Bug, startbit + numbit > 16, %u + %u \n",uStartBit,uNumBit);
		return 0;
	}
	uMask = UINT16_C(0xffff) >> (16 - uNumBit);
	uInp >>= uStartBit;
	return uInp & uMask;
}

/**
************************************************************************
* \fn void iShuffle(uint16_t* uOutp, uint16_t uInp,uint8_t uStartBit,uint8_t uNumBit,uint8_t uNumber)
{
* from Python: Ersetzt uNumBit in  uInp von uStartBit auf uNumber
************************************************************************
*/

static void
iShuffle(uint16_t * uOutp, uint16_t uInp, uint8_t uStartBit, uint8_t uNumBit, uint16_t uNumber)
{
	uint16_t uMask = 0;
	if (uStartBit + uNumBit > 16) {
		Con_Printf("CDR Bug, startbit + numbit > 16, %u + %u \n",uStartBit,uNumBit);
		return;
	}
	uMask = UINT16_C(0xffff) >> (16 - uNumBit);
	uMask <<= uStartBit;
	uNumber <<= uStartBit;
	*uOutp = (uInp & (~uMask)) | uNumber;
}

/**
************************************************************************
* \fn void Cdr_WritePart(uint16_t phy_addr,uint16_t uRegister,uint8_t uNumBit,uint8_t uStartBit,uint8_t uNumber)
* from Python startup_cdr.py
************************************************************************
*/

void
Cdr_WritePart(uint16_t phy_addr, uint16_t uRegister, uint8_t uLastBit, uint8_t uStartBit, uint16_t uNumber)
{
	uint16_t uNumBit;
	uint16_t uVal;
	uNumBit = uLastBit - uStartBit + 1;
	uVal = Cdr_Read(phy_addr, uRegister);
	iShuffle(&uVal, uVal, uStartBit, uNumBit, uNumber);
	Cdr_Write(phy_addr, uRegister, uVal);
}

/**
************************************************************************
* \fn void Cdr_ReadPart(uint16_t phy_addr,uint16_t uRegister,uint8_t uNumBit,uint8_t uStartBit)
* from Python startup_cdr.py
************************************************************************
*/

uint16_t
Cdr_ReadPart(uint16_t phy_addr, uint16_t uRegister, uint8_t uLastBit, uint8_t uStartBit)
{
	uint16_t uNumBit;
	uint16_t uVal;
	uNumBit = uLastBit - uStartBit + 1;
	uVal = Cdr_Read(phy_addr, uRegister);
	uVal = iShuffleRead(uVal, uStartBit, uNumBit);
	return uVal;
}

/**
************************************************************************
* \fn void Cdr_Startup(uint16_t phy_addr=1)
* from Python startup_cdr.py
************************************************************************
*/

static uint8_t
Cdr_Startup(uint16_t phy_addr)	// Olga
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
	uint8_t status = 1;
	uint8_t i;
	uVal = Cdr_ReadPart(phy_addr, 3, 15, 4);
	if (uVal == 0x740) {
		Con_Printf("CDR device detected\n");
	} else {
		Con_Printf("No CDR device detected at addr %u\n",phy_addr);
		return -1;
	}

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
	Cdr_Write(phy_addr, 0, 0x1020);	//Hard reset (bit 5) and MDIO init (bit 12)

//
//regWrite(device + "30.0",0x0200) # Datapath soft reset (bit 9)
	Cdr_Write(phy_addr, 0, 0x200);	//Datapath soft reset (bit 9)
//
//regWrite(device + "30.385.10:8",4)      # Set 128 steps for PI resolution - inside reset"
//mod_field(uint8_t *buf,uint8_t value,uint8_t bits,uint8_t firstbit)
//

	Cdr_WritePart(phy_addr, 385, 10, 8, 4);
//Cdr_WritePart(uint16_t phy_addr,uint16_t uRegister,uint8_t uNumBit,uint8_t uStartBit,uint8_t uNumber)

//regWrite(device + "30.641.10:8",4)
	Cdr_WritePart(phy_addr, 641, 10, 8, 4);

//regWrite(device + "30.897.10:8",4)
	Cdr_WritePart(phy_addr, 897, 10, 8, 4);

//regWrite(device + "30.1153.10:8",4)
	Cdr_WritePart(phy_addr, 1153, 10, 8, 4);

//regWrite(device + "30.0",0x0000)
//
//# CDR recal of TxPLL while PI3 is locked
//regWrite(device + "30.1184.1",1)        # Locking Rx3 PI
	Cdr_WritePart(phy_addr, 1184, 1, 0, 1);
//# Re-Calibrating Tx PLL
//regWrite(device + "30.39.15",0)         # normal mode
	Cdr_WritePart(phy_addr, 39, 15, 0, 0);

//regWrite(device + "30.39.15",1)         # re-calibrate
	Cdr_WritePart(phy_addr, 39, 15, 0, 1);

//regWrite(device + "30.39.15",0)         # normal mode
	Cdr_WritePart(phy_addr, 39, 15, 0, 0);
//time.sleep(0.1) 
	SleepMs(100);		// wait for it to lock
//regWrite(device + "30.1184.1",0)        # and unlock PI3
	Cdr_WritePart(phy_addr, 1184, 1, 0, 0);
	SleepMs(100);
//time.sleep(0.1)
//
//for lane in range(4):                   # EQ offset override is set
//regWrite(device + "30." + str(441 + 256 * lane) + ".0",1)
	for (i = 0; i < 4; i++) {
		Cdr_WritePart(phy_addr, 441 + 256 * i, 0, 0, 1);	// Olga  Achtung!!!!!! Die Länge ist 0
	}

//
//if A0_device == 1:
//print "Implemening swizzle sampler offset correction (only for A0/A1 stepping)"
//execfile("../scripts/correct_offsets.py",globals(),startup_dictionary)
//for lane in range(4):                 # Set 128 steps here for A0 (not inside reset)
//regWrite(device + "30." + str(385 + 256 * lane) + ".10:8",4)
	for (i = 0; i < 4; i++) {
		Cdr_WritePart(phy_addr, 385 + 256 * i, 10, 8, 4);
	}
//
//regWrite(device + "30.37.15",1) # Asserts FIFO reset

	Cdr_WritePart(phy_addr, 37, 15, 0, 1);
//regWrite(device + "30.37.14",1) # Enable auto-reset

	Cdr_WritePart(phy_addr, 37, 14, 0, 1);
//regWrite(device + "30.37.15",0) # De-Asserts FIFO reset

	Cdr_WritePart(phy_addr, 37, 15, 0, 0);
//
//# This section just checks things did complete are startup OK
//
//reply = regRead(device + "30.42.8")

	uVal = Cdr_ReadPart(phy_addr, 42, 8, 0);
//print "Rx PLL Lock Status should be 1:" + str(reply)
//if reply==0:
//print "***  Rx PLL not locked, is REFCLK present?"
//status = 0
//
	if (uVal == 0) {
		status = 0;
	}
//reply = regRead(device + "30.40.8")

	uVal = Cdr_ReadPart(phy_addr, 40, 8, 0);

//print "Tx PLL Lock Status should be 1:" + str(reply)
//if reply==0:
//print "***  Tx PLL not locked, is REFCLK present and Rx3 PI locked?"
//status = 0
//
	if (uVal == 0) {
		status = 0;
	}
//reply = regRead(device + "30.0.8")

	uVal = Cdr_ReadPart(phy_addr, 0, 8, 0);

//print "GB Tx or CDR Reset sequence done:" + str(reply)
//if reply == 0:
//print "***  Part did not complete GB Tx or CDR reset sequence"
//status = 0
//
	if (uVal == 0) {
		status = 0;
	}
//print "Enabling calibrated EQ offsets"
//for lane in range(4):
//regWrite(device + "30." + str(441 + 256 * lane) + ".0",1)
	for (i = 0; i < 4; i++) {
		Cdr_WritePart(phy_addr, 441 + 256 * i, 0, 0, 1);	// Achtung!!!!!! Die Länge ist 0
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
	if ((argc == 3) && (strcmp(argv[1], "startup") == 0)) {
		uint8_t cdr = astrtoi16(argv[2]);
		Con_Printf("Calling Olgas CDR_Startup for CDR %u\n", cdr);
		Cdr_Startup(cdr);
		return 0;
	}
	if ((argc == 3) && (strcmp(argv[1], "init") == 0)) {
		uint8_t cdr = astrtoi16(argv[2]);
		Con_Printf("Calling Olgas CDR_InitCdr for CDR %u\n", cdr);
		Cdr_InitCdr(cdr);
		return 0;
	} else if (argc == 3) {
		phyAddr = astrtoi16(argv[1]);
		regAddr = astrtoi16(argv[2]);
		val = Cdr_Read(phyAddr, regAddr);
		Con_Printf("%u.%u: 0x%x\n", phyAddr, regAddr, val);
		return 0;
	} else if (argc == 4) {
		phyAddr = astrtoi16(argv[1]);
		regAddr = astrtoi16(argv[2]);
		val = astrtoi16(argv[3]);
		Cdr_Write(phyAddr, regAddr, val);
	} else {
		return -EC_BADARG;
	}
	return 0;
}

INTERP_CMD(cdrCmd, "cdr", cmd_cdr, "cdr <cdrAddr> <regAddr> ?<value>?   # read write to/from cdr");


void
PVSignalQuality_Get (void *cbData, uint32_t adId, char *bufP,uint16_t maxlen)
{
//        uint32_t rawAdval;
//        uint8_t cnt;
//        ADCChan *ch = cbData;
//        rawAdval = ADC12_Read(ch->channelNr);
        bufP[uitoa16(value,bufP)] = 0;
}


/*
 ************************************************************************************
 * Zum Webinterface exportierte Variablen:
 * CDR status: PLL locks,
 * PRBS pattern, PRBS lock, LOL status , PRBS error counters.
 ************************************************************************************
 */
void
CDR_Init(const char *name)
{
	Interp_RegisterCmd(&cdrCmd);
	int lane;
	for(lane = 0; lane < 4; lane++) {
		PVar_New(PVSignalQuality_Get,NULL,&cdr,0,"%s.regEqState_%u",name,);
	}

}
