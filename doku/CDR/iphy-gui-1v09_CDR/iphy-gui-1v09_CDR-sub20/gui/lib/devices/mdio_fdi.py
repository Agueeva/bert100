#
# $Header: /projects/wlv/iphy/sos/repository/iphy_A.rep/TEMP/case_1285613542/#alidation#iphy-gui#gui#lib#devices#mdio_fdi.py,v 1.1 2011-09-02 11:43:54-07 case Exp $
# $Log: #alidation#iphy-gui#gui#lib#devices#mdio_fdi.py,v $
# Revision 1.1  2011-09-02 11:43:54-07  case
# ...No comments entered during checkin...
#
# Revision 1.1  2011-05-04 18:30:17-07  rbemra
# Initial python for MDIO interface
#
#
# From USBMPC_MDIODlg.cpp FDI example
# 
from time import sleep
from ctypes import *
libc = cdll.LoadLibrary("msvcrt") # or cdll.msvcrt  # linux: CDLL("libc.so.6")
# lmdio = cdll.LoadLibrary("USBMPC.dll") # cdecl calling convention
lmdio = windll.LoadLibrary("USBMPC") # stdcall calling convention

global MDIO_NO_ERROR
global MDIO_USB_OPEN_FAILURE
global MDIO_USB_COMM_FAILURE
global I2C_NO_ERROR

MDIO_NO_ERROR = 0
MDIO_USB_OPEN_FAILURE = 1000
MDIO_USB_COMM_FAILURE = 1001
USBMPC_I2C_SPEED_100KHZ = 500
USBMPC_I2C_SPEED_400KHZ = 125
I2C_NO_ERROR = 0

# int MDIOSetSpeed(int aSpeed) ;
# Parameters: aSpeed  Speed to run MDIO bus. Valid values are 100, 200, 300,
# 400, and 600 only (corresponding to speeds of 1 MHz, 2 MHz, 3MHz, 4 MHz, and 6 MHz).
# Returns: Returns TRUE if successful, else FALSE.
lmdio.MDIOSetSpeed.restype = c_int

# int SetupHardware(int nReserved, int nSpeed, int nPort)
# Function: This routine is called to set up the parameters to be used by the
# USB-MPC hardware. This routine must be executed once before calling the other commands.
# Parameters: nReserved = Currently always 0.
# nSpeed = USBMPC_I2C_SPEED_100KHZ or USBMPC_I2C_SPEED_400KHZ to declare which speed to run the I2C bus
# nPort = Usually 0 unless more than one USB-MPC device is used. Each device is sequentially numbered.
# Returns: Always returns I2C_NO_ERROR.
lmdio.SetupHardware.restype = c_int

# int  SelectHardware(int nPort)
# Function: When using multiple USB-MPC ports at the same time, call this
#   routine with the number of the previously initialized USB-MPC device (0 for the 1st, 1 for the 2nd
# Parameters: None
# Returns: Returns I2C_NO_ERROR if successful or I2C_BAD_PORT_ADDR if incorrect port number.
lmdio.SelectHardware.restype = c_int

# int  Mdio45ReadWord(int nPortAddr, int nDevAddr, int nRegAddr, int *nData);
# Function: This routine reads a single 32bit word from the MDIO (Clause 45) interface.
# The resultant word is returned in the variable nData.
# Parameters: nPortAddr = This value is the port number of the slave device on the MDIO bus.
# nDevAddr = This value is the device number of the slave device on the MDIO bus.
# nRegAddr = This is the address of the register within the slave device that is to be read.
# nData = This is the location that will receive the results of the read operation.
# Returns: Always MDIO_NO_ERROR. Also updates the value of nData with the value read from the bus.
lmdio.Mdio45ReadWord.restype = c_int
lmdio.Mdio45WriteWord.restype = c_int

# int  Mdio45WriteWord(int nPortAddr, int nDevAddr, int nRegAddr, int nData);
# Function: This routine writes a single 32bit word to the MDIO (Clause 45) interface.
# Parameters: nPortAddr = This value is the port number of the slave device on the MDIO bus.
# nDevAddr = This value is the device number of the slave device on the MDIO bus.
# nRegAddr = This is the address of the register within the slave device that is to be written.
# nData = This is the data that will be written to the slave device.
# Returns: Always returns MDIO_NO_ERROR.
#-------------------------------------------------------------------------------

def mdio_wr(wVal, regAddr, devAddr=0, portAddr=0):
  valC = c_int(wVal)
  wError = lmdio.Mdio45WriteWord(c_int(portAddr), c_int(devAddr),
                                 c_int(regAddr), valC)
  if (wError!=MDIO_NO_ERROR):
    raise Exception("MDIO Write Error!")
#-------------------------------------------------------------------------------

def mdio_rd(regAddr, devAddr=0, portAddr=0):
  valC = c_int(-1)

  readError = lmdio.Mdio45ReadWord(c_int(portAddr), c_int(devAddr),
                                   c_int(regAddr), byref(valC))
  if (readError!=MDIO_NO_ERROR):
    raise Exception("MDIO ReadError!")
  return valC.value
#-------------------------------------------------------------------------------

def mdio_wa(wVal, regAddr, devAddr=0, portAddr=0, waitSec=0):
  mdio_wr(wVal, regAddr, devAddr, portAddr)
  if waitSec>0:
    sleep(waitSec)
  rVal = mdio_rd(regAddr, devAddr, portAddr)
  return (rVal==wVal)
#-------------------------------------------------------------------------------

def mdio_set_speed(mhzBusSpeed):
  SpeedDict = {1:100, 2:200, 3:300, 4:400, 6:600}

  rVal = mdio.MDIOSetSpeed(c_int(SpeedDict[mhzBusSpeed]))
  return rVal
#-------------------------------------------------------------------------------

def mdio_init(khzI2C_Speed=400, portNum=0):
  I2C_SpeedDict = {100: USBMPC_I2C_SPEED_100KHZ, 400: USBMPC_I2C_SPEED_400KHZ}

  rVal = lmdio.SetupHardware(c_int(0), c_int(khzI2C_Speed), c_int(portNum))
  assert(rVal==I2C_NO_ERROR)
#-------------------------------------------------------------------------------

def mdio_select_hw(prevPort):
  rVal = lmdio.SelectHardware(c_int(prevPort))
  # pVal = crVal.value
  if rVal!=I2C_NO_ERROR: # I2C_BAD_PORT_ADDR
    raise Exception("Incorrect port number")
#-------------------------------------------------------------------------------
#  FDI
# 
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # /
#  CUSBMPC_MDIODlg message handlers
#-------------------------------------------------------------------------------

def OnInitDialog():
	#  TODO: Add extra initialization here
	#  FDI
	#  Setup the USBMPC I2C speed and use the first USB device
#  NOTE:  400 kHz cannot be used with all devices
	# 
# int SetupHardware(int nReserved, int nSpeed, int nPort)
	lmdio.SetupHardware(c_int(0), c_int(USBMPC_I2C_SPEED_100KHZ), c_int(0))
	return True   #  return TRUE  unless you set the focus to a control
#-------------------------------------------------------------------------------

def OnTest():
  portAddr = 0
  devAddr = 0
  valC = c_int(0)
  INC_THIS_REG = 0x16

  #  Read all 32 registers
  mText = "Phy Address:\t<%d, %d>\r\n"%(portAddr, devAddr)
  for reg in range(32):
    #  Show the register on the line
    vLine = "\t%02X:\t"%reg
    mText = mText + vLine
    if (lmdio.Mdio45ReadWord(c_int(portAddr), c_int(devAddr), c_int(reg), byref(valC))):
      mText = mText + "Error!" 
    else :
      vLine = "0x%04X"%valC.value
      mText = mText + vLine

    mText = mText + "\r\n"

  #  Increment one of the values
  if (lmdio.Mdio45ReadWord(c_int(portAddr), c_int(devAddr), c_int(INC_THIS_REG), byref(valC))):
    mText = mText + "Could not read register!\r\n" 

  if (lmdio.Mdio45WriteWord(c_int(portAddr), c_int(devAddr), c_int(INC_THIS_REG), c_int(valC.value+1))):
    mText = mText + "Error writing back register!" 
  else :
    vLine = "0x%04X"%valC.value

  mText = mText + "\r\n" 

  #  Show registers again
  for reg in range(32):
    #  Show the register on the line
    vLine = "\t%02X:\t"%reg
    mText = mText + vLine
    if (lmdio.Mdio45ReadWord(c_int(portAddr), c_int(devAddr), c_int(reg), byref(valC))):
      mText = mText + "Error!" 
    else:
      vLine = "0x%04X"%valC.value
      mText = mText + vLine

    mText = mText + "\r\n" 

  # UpdateData(False)
  print mText
  # return mText
#-------------------------------------------------------------------------------

def find_address(): # find TI xcvr address
  waitSec = 0.25 # 0.5
  # try reading device/vendor identifier status registers
  addrId1 = 0x2
  addrId2 = 0x3
  printHead = False
  for devAddr in range(4,6,1): # (32) 4=PHY, 5-DTE
    for phyAddr in range(2): # 0=XGXS_A 1=XGXS_B
      id1 = mdio_rd(addrId1, devAddr, phyAddr)
      sleep(waitSec)
      id2 = mdio_rd(addrId2, devAddr, phyAddr)
      sleep(waitSec)
      if (id1!=0xFFFF or id2!=0xFFFF):
        if not printHead:
          print('--- Read Vendor ID: DevAddr, PhyAddr, regAddr, regVal ---')
          printHead = True
        print("0x%x, 0x%x, 0x%x, 0x%x"%(devAddr, phyAddr, addrId1, id1))
        print("0x%x, 0x%x, 0x%x, 0x%x"%(devAddr, phyAddr, addrId2, id2))
  # try writing to control registers
  addrCtrl1 = 0x0
  valCtrl1 = 0x2040 # write default: normal, >= 10Gbps
  addrCfg = 0x8000
  valCfg = 0x3 # enable CRPAT, CJPAT test modes
  printHead = False
  for devAddr in range(4,6,1): # (32): # (32) 4=PHY, 5-DTE
    for phyAddr in range(2): # 0=XGXS_A 1=XGXS_B
      mdio_wr(valCtrl1, addrCtrl1, devAddr, phyAddr)
      sleep(waitSec)
      valRd = mdio_rd(addrCtrl1, devAddr, phyAddr)
      if valRd!=0xFFFF:
        if not printHead:
          print('--- Write/Read Ctrl1, Cfg: DevAddr, PhyAddr, regAddr, regVal ---')
          printHead = True
        print("0x%x, 0x%x, 0x%x, 0x%x"%(devAddr, phyAddr, addrCtrl1, valRd))
      mdio_wr(valCfg, addrCfg, devAddr, phyAddr)
      sleep(waitSec)
      valRd = mdio_rd(addrCfg, devAddr, phyAddr)
      if valRd!=0xFFFF:
        if not printHead:
          print('--- Write/Read Ctrl1, Cfg: DevAddr, PhyAddr, regAddr, regVal ---')
          printHead = True
        print("0x%x, 0x%x, 0x%x, 0x%x"%(devAddr, phyAddr, addrCfg, valRd))

#-------------------------------------------------------------------------------
# 
#  Local Variables:
#  tab-width: 2
#  indent-tabs-mode: nil
#  scroll-step: 1
#  line-number-mode: t
#  column-number-mode: t
#  End:
# 
