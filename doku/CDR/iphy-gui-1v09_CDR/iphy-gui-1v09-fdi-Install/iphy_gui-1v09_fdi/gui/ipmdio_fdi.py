# ------------------------------------------------------------------------------
#
# $RCSfile: validation#python#ipmdio.py,v $ - MDIO interface: iPHY <-> FDI
#
# Disclaimer:
# Inphi Confidential
# Copyright(c) Inphi Corp. 2011
#
# All rights reserved.
# This is unpublished, confidential Inphi proprietary information.
# Do not reproduce or redistribute without written permission.
#
# $Header: /projects/wlv/iphy/sos/repository/iphy_A.rep/TEMP/rbemra_1274230893/validation#python#ipmdio.py,v 1.5 2011-07-28 15:11:27-07 rbemra Exp $
# $Log: validation#python#ipmdio.py,v $
# Revision 1.5  2011-07-28 15:11:27-07  rbemra
# Added regWriteAck() workaround for missing MDC last clock rising edge
#
# Revision 1.4  2011-07-14 16:33:37-07  rbemra
# Installed event-handlers for Main window's tabs
# Updated AllRegisters to handshake w/ Main window updates
#
# Revision 1.3  2011-07-06 18:19:38-07  rbemra
# Update for real/dummy MDIO interface at startup.
#
# Revision 1.2  2011-07-05 18:43:11-07  rbemra
# Updates to get more things working: user scripts w/ options/threading, DDF+CSV writers, MDIO R/W updates, ipallreg addition
#
# Revision 1.1  2011-07-01 09:02:55-07  rbemra
# First, prelim. runnable version
#
# ------------------------------------------------------------------------------
#
# From USBMPC_MDIODlg.cpp FDI example
# 
import re
import ipvar
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

NWR = 1

def ExecuteVendorCmd(Command, TimeoutMS):
  VendorCmdStatRegAddr = 'CPAK::1.'+str(0x9001)
  VendorStatMask = 0xff00
  VendorStatIdle = 0x0000
  VendorStatComplete = 0x0100
  VendorStatInProgress = 0x0200
  VendorStatError = 0x0300
  VendorStatInvalidCmd = 0x0400

  VendorParamsBaseAddr = 'CPAK::1.'+str(0x9200)
  VendorParamsBaseAddr1 = 'CPAK::1.'+str(0x9201)
  VendorParamsBaseAddr2 = 'CPAK::1.'+str(0x9202)
  
  VendorResultsBaseAddr = 'CPAK::1.'+str(0x9300)

  VendorCmdWriteCdrMdio = 0x0023
  VendorCmdReadCdrMdio = 0x0024
  StepMS = 100
  
  # check that the status is idle
  Reg = regValRead(VendorCmdStatRegAddr)
  Reg = regValRead(VendorCmdStatRegAddr)  

  if ((Reg & VendorStatMask) != VendorStatIdle):
    print "Cannot execute command ", Command," vendor status is not idle"
    
  # write the command
  regWriteAck(VendorCmdStatRegAddr, Command)
  
  # wait for command not in progress
  Reg = regValRead(VendorCmdStatRegAddr)
  cmdStat = (Reg & 0xff00) >> 8
 
  while ((Reg & VendorStatMask) == VendorStatInProgress):   
    if (TimeoutMS <= 0):
      print "Timeout waiting for vendor command to complete: ", Command
      TimeoutMS -= StepMS
    sleep(StepMS/1000.0)
    Reg = regValRead(VendorCmdStatRegAddr)
    
  if ((Reg & VendorStatMask) == VendorStatComplete):
    return True
  else:
    print "Vendor command", Command,"failed with status ", Reg & VendorStatMask

def cdr_read(dev,addr,TimeoutMS=1000,debug=0):
  global ExecuteVendorCmd
  if debug == 1:
    print 'cdr_read',dev,addr,
    
  VendorCmdStatRegAddr = 'CPAK::1.'+str(0x9001)
  VendorStatMask = 0xff00
  VendorStatIdle = 0x0000
  VendorStatComplete = 0x0100
  VendorStatInProgress = 0x0200
  VendorStatError = 0x0300
  VendorStatInvalidCmd = 0x0400

  VendorParamsBaseAddr  = 'CPAK::1.'+str(0x9200)
  VendorParamsBaseAddr1 = 'CPAK::1.'+str(0x9201)
  VendorParamsBaseAddr2 = 'CPAK::1.'+str(0x9202)
  
  VendorResultsBaseAddr = 'CPAK::1.'+str(0x9300)

  VendorCmdWriteCdrMdio = 0x0023
  VendorCmdReadCdrMdio = 0x0024

  regWriteAck(VendorParamsBaseAddr,dev)
  regWriteAck(VendorParamsBaseAddr1,addr)
  regWriteAck(VendorParamsBaseAddr2,1)     # 1 word

  if ExecuteVendorCmd(VendorCmdReadCdrMdio, TimeoutMS) == False:
    print "cdr read failed"
    return False
  else:
    val = regValRead(VendorResultsBaseAddr)
    if debug == 1:
      print "result=",val
    return(val)

def cdr_write(dev,addr,data,TimeoutMS=1000,debug=0):
  if debug == 1:
    print 'cdr_write',dev,addr,data
  VendorParamsBaseAddr  = 'CPAK::1.'+str(0x9200)
  VendorParamsBaseAddr1 = 'CPAK::1.'+str(0x9201)
  VendorParamsBaseAddr2 = 'CPAK::1.'+str(0x9202)
  VendorParamsBaseAddr3 = 'CPAK::1.'+str(0x9203)
  VendorCmdWriteCdrMdio = 0x0023


  regWrite(VendorParamsBaseAddr,dev)
  regWrite(VendorParamsBaseAddr1,addr)
  regWrite(VendorParamsBaseAddr2,1)     # 1 word
  regWrite(VendorParamsBaseAddr3,data)
  
  if ExecuteVendorCmd(VendorCmdWriteCdrMdio, TimeoutMS) == False:
    print "cdr write failed"
    return False
  else:
    return True
    
def setVendorTestMode(data,TimeoutMS=1000,debug=0):
  if debug == 1:
    print 'set vendor test mode',data
  VendorParamsBaseAddr  = 'CPAK::1.'+str(0x9200)
  VendorWriteVendorTestMode = 0x002a

  regWrite(VendorParamsBaseAddr,data)
  
  if ExecuteVendorCmd(VendorWriteVendorTestMode, TimeoutMS) == False:
    print "vendor test mode write failed"
    return False
  else:
    return True
  
def mdio_wr(portAddr, devAddr, regAddr, wVal):
  valC = c_int(wVal)
  #
  # seems to be an iCDR bug where one has to write twice or write+read
  # in order to apply the register change
  #
  for nTimes in range(NWR):
    wError = lmdio.Mdio45WriteWord(c_int(portAddr), c_int(devAddr),
                                   c_int(regAddr), valC)
    if (wError!=MDIO_NO_ERROR):
      raise Exception("MDIO Write Error!")
#-------------------------------------------------------------------------------

def mdio_rd(portAddr, devAddr, regAddr):
  valC = c_int(-1)

  readError = lmdio.Mdio45ReadWord(c_int(portAddr), c_int(devAddr),
                                   c_int(regAddr), byref(valC))
  if (readError!=MDIO_NO_ERROR):
    raise Exception("MDIO ReadError!")
  return valC.value
#-------------------------------------------------------------------------------

def mdio_wa(portAddr, devAddr, regAddr, wVal, waitSec=0,addrRef=''):
  if portAddr == -1:  # catch the optics module and use the tunnel
    cdrRef = int(re.match('M-CDR(\d)::.*',addrRef).group(1))
    cdr_write(cdrRef,regAddr,wVal)
  else:
    mdio_wr(portAddr, devAddr, regAddr, wVal)
    if '-ATE_log' in sys.argv:
      fid = open('ate_log.csv','a')
      #print regAddr,wVal
      fid.write(("%s.%s,0x%04x\n") %(devAddr,regAddr,wVal))
      fid.close()
  if waitSec>0:
    sleep(waitSec)
  if portAddr == -1:  # catch the optics module and use the tunnel
    cdrRef = int(re.match('M-CDR(\d)::.*',addrRef).group(1))
    rVal = cdr_read(cdrRef,regAddr)
  else:
    rVal = mdio_rd(portAddr, devAddr, regAddr)
  #rVal = mdio_rd(portAddr, devAddr, regAddr)
  return (rVal==wVal)
#-------------------------------------------------------------------------------

def mdio_set_speed(mhzBusSpeed):
  SpeedDict = {1:100, 2:200, 3:300, 4:400, 6:600}

  rVal = lmdio.MDIOSetSpeed(c_int(SpeedDict[mhzBusSpeed]))
  return True if rVal else False
#-------------------------------------------------------------------------------

def mdio_init(khzI2C_Speed=400, portNum=0):
  I2C_SpeedDict = {100: USBMPC_I2C_SPEED_100KHZ, 400: USBMPC_I2C_SPEED_400KHZ}

  rVal = lmdio.SetupHardware(c_int(0), c_int(khzI2C_Speed), c_int(portNum))
  assert(rVal==I2C_NO_ERROR)
  return True
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

def mdio_comms_setup():
  ipvar.MDIO_INIT = False
  mdio_init()
  if not mdio_set_speed(1):
    print "Initial MDIO reponse failed. Attempting to reestablish connection..."
    if '-nohardware' in sys.argv:
      print '\n*** No hardware mode - skipping MDIO check ***\n'
    else:  
      for i in range(1,11):
        print str(i) + ',',
        sleep(1)
        if mdio_set_speed(1):
          print "Established."
          ipvar.MDIO_INIT = True
          break
        if i == 10:
          print
          print "***********************************************************"
          print "MDIO communicaton not established. Using dummy register R/W"
          print "***********************************************************"
  else:
    ipvar.MDIO_INIT = True
  for kSpeed in [4, 3, 2, 1]: # set max. possible MHz speed
    if mdio_set_speed(kSpeed):
      break

  
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
  devAddr = 4
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

import bv
from ipreg import *
import ipvar

def find_address(): # find TI xcvr address
  waitSec = 0.25 # 0.5
  # try reading device/vendor identifier status registers
  addrId1 = 0x2
  addrId2 = 0x3
  printHead = False
  for devAddr in range(4,6,1): # (32) 4=PHY, 5-DTE
    for phyAddr in range(2): # 0=XGXS_A 1=XGXS_B
      id1 = mdio_rd(phyAddr, devAddr, addrId1)
      sleep(waitSec)
      id2 = mdio_rd(phyAddr, devAddr, addrId2)
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
      mdio_wr(phyAddr, devAddr, addrCtrl1, valCtrl1)
      sleep(waitSec)
      valRd = mdio_rd(phyAddr, devAddr, addrCtrl1)
      if valRd!=0xFFFF:
        if not printHead:
          print('--- Write/Read Ctrl1, Cfg: DevAddr, PhyAddr, regAddr, regVal ---')
          printHead = True
        print("0x%x, 0x%x, 0x%x, 0x%x"%(devAddr, phyAddr, addrCtrl1, valRd))
      mdio_wr(phyAddr, devAddr, addrCfg, valCfg)
      sleep(waitSec)
      valRd = mdio_rd(phyAddr, devAddr, addrCfg)
      if valRd!=0xFFFF:
        if not printHead:
          print('--- Write/Read Ctrl1, Cfg: DevAddr, PhyAddr, regAddr, regVal ---')
          printHead = True
        print("0x%x, 0x%x, 0x%x, 0x%x"%(devAddr, phyAddr, addrCfg, valRd))
#-------------------------------------------------------------------------------
# <hierRef>.<regAddr>[.<bitRange>]
# hierRef = <prtAd>::<MMD>
# Return (portAddr, devAddr, regAddr, nBits, start)
#
def mdio_addr_decode(addrRef):
  addrList = addrRef.split('.')
  nItems = len(addrList)
  retList = [None, None, None, 0, 0] # (portAddr, devAddr, regAddr, nBits, start)
  while True:
    if nItems<2:
      break # return
    else:
      # do hierRef
      refList = addrList[0].split(':')
      if len(refList)<2: # insufficient hierRef
        break
      # take first, last: [0], [-1]
      try:
        retList[0] = int(refList[0]) # already a PRTAD
      except:
        devDef = ipvar.System[refList[0]] # devRef, 'iPHY-GB', 'iPHY-CDR' etc.
        retList[0] = devDef.prtAddr
      retList[1] = int(refList[-1]) # <MMD> or devAddr
      # now get regAddr, bitRange
      retList[2] = int(addrList[1])
      if nItems==2: # <hierRef>.<regAddr>
        retList[3] = REG_LEN # nBits
        retList[4] = 0 # start
      else: # nItems>=3: # <hierRef>.<regAddr>.<bitRange>...
        bitList = map(int, addrList[2].split(':')) # [endBit, start]
        if len(bitList)<2: # <bitRange> = bitNum
          retList[3] = 1 # nBits
          retList[4] = bitList[0] # start
        else: # <endBit>:<start>...
          retList[3] = bitList[0] - bitList[1] + 1 # nBits
          retList[4] = bitList[1] # start
    break
  # end while True
  return retList


#-------------------------------------------------------------------------------
# Return register value as an int
#
def regValRead(addrRef):
  [portAddr, devAddr, regAddr, nBits, start] = mdio_addr_decode(addrRef)
  #print addrRef,portAddr, devAddr, regAddr, nBits, start

  if ipvar.MDIO_INIT:
    if portAddr == -1:  # catch the optics module and use the tunnel
      cdrRef = int(re.match('M-CDR(\d)::.*',addrRef).group(1))
      rVal = cdr_read(cdrRef,regAddr)
    else:
      rVal = mdio_rd(portAddr, devAddr, regAddr)
  else:
    from random import randint
    #print "regValRead('%s')" %(addrRef)
    rVal = randint(0, 2**nBits-1) # slice it: 2**nBits-1?
  rVec = bv.BitVector(REG_LEN, rVal)
  rVal = rVec.getbits(start, nBits)
  return int(rVal)
#-------------------------------------------------------------------------------

def reg_format(rVal, format=None, nBits=None):
  from math import log, ceil
  if format==None:
    retStr = str(rVal)
  else:
    format = format.lower()
    if format=='x':
      retStr = hex(int(rVal)).lower()[2:] # lower-case, chop trailing 'l'
    elif format=='b':
      retStr = bin(rVal)[2:]
    elif format=='n':
      if nBits==None: # use minimum
        nBits = int(ceil(log(rVal, 2)))
      rVec = bv.BitVector(nBits, rVal)
      retStr = rVec.get_rbits()
    else: # 'd' or default decimal
      retStr = str(rVal)
  return retStr
#-------------------------------------------------------------------------------
# Return register value as a string
# myReg = regRead('iPHY-GB::8.0')			# returns a 16b decimal value
# myReg = regRead('CDR2::8.0.15')			# returns a 1b decimal value
# myReg = regRead('iPHY-GB::8.0.15', format='x')	# returns a 1b hex value
# myReg = regRead('CDR1::8.0.10:7')		# returns a 4b decimal value
# myReg = regRead('1::30.0.0', format='b')		# returns a 1b binary from PRTAD=1
#
def regRead(addrRef, format=None):
  [portAddr, devAddr, regAddr, nBits, start] = mdio_addr_decode(addrRef)
  #print portAddr
  if ipvar.MDIO_INIT:
    if portAddr == -1:  # catch the optics module and use the tunnel
      cdrRef = int(re.match('M-CDR(\d)::.*',addrRef).group(1))
      rVal = cdr_read(cdrRef,regAddr)
    else:
      rVal = mdio_rd(portAddr, devAddr, regAddr)
  else:
    from random import randint
    #print "regRead('%s')" %(addrRef)
    rVal = randint(0, 2**nBits-1) # slice it: 2**nBits-1?

  rVec = bv.BitVector(REG_LEN, rVal)
  rVal = rVec.getbits(start, nBits)
  return reg_format(rVal, format, nBits)
#-------------------------------------------------------------------------------
# regWrite('iPHY-GB::8.0', "16'h5EB2") : 16b hex
# regWrite('iPHY-GB::8.0', 0x5C) : Free-form hex, to be expanded based on the bit-range
# regWrite('CDR2::8.0.15', "7'd14") : a 7b decimal number
# regWrite('iPHY-GB::8.0.15', "8'b10110110") : an 8b binary value
# regWrite('CDR1::8.0.10:7', "12'b1011_0110_1110") : a 12b nibble separated binary value
# regWrite('1::30.0.0', 010110) : octal value = d4168
#
def regWrite(addrRef, wVal):
  #print "The regWrite function has been replaced by regWriteAck - please use that."
  if not ipvar.MDIO_INIT:
    #print "regWrite('%s', " % addrRef, wVal, ")"
    return
  [portAddr, devAddr, regAddr, nBits, start] = mdio_addr_decode(addrRef)
  wVec = bv.BitVector(REG_LEN, wVal)
  if nBits<REG_LEN: # bitslice: read reg. first
    if portAddr == -1:  # catch the optics module and use the tunnel
      cdrRef = int(re.match('M-CDR(\d)::.*',addrRef).group(1))
      rVal = cdr_read(cdrRef,regAddr)
    else:
      rVal = mdio_rd(portAddr, devAddr, regAddr)    
    #rVal = mdio_rd(portAddr, devAddr, regAddr) # READ full reg.
    rVec = bv.BitVector(REG_LEN, rVal) # for class.copybits() next
    rVec.copybits(wVec.value, destStart=start, srcStart=0, nBits=nBits) # MODIFY
    mdio_wr(portAddr, devAddr, regAddr, rVec.value) # WRITE
  else:
    mdio_wr(portAddr, devAddr, regAddr, wVec.value) # WRITE
#-------------------------------------------------------------------------------

def regWriteAck(addrRef, wVal,debug=False):
  if not ipvar.MDIO_INIT:
    #print "regWriteAck( '%s'," % addrRef, wVal, ")"
    return True
  [portAddr, devAddr, regAddr, nBits, start] = mdio_addr_decode(addrRef)
  wVec = bv.BitVector(REG_LEN, wVal)
  if (debug): 
    print '-'*70
    print 'regWriteAck'
    print 'PRTADR:%d, DEVADR:%d, REGADR:0x%x, nbits: %d, REG_LEN: %d, wVal: 0x%x' % (portAddr,devAddr,regAddr,nBits,REG_LEN,wVal)


    
  if nBits<REG_LEN: # bitslice: read reg. first
    if portAddr == -1:  # catch the optics module and use the tunnel
      cdrRef = int(re.match('M-CDR(\d)::.*',addrRef).group(1))
      rVal = cdr_read(cdrRef,regAddr)
    else:
      rVal = mdio_rd(portAddr, devAddr, regAddr)
    #rVal = mdio_rd(portAddr, devAddr, regAddr) # READ full reg.
    rVec = bv.BitVector(REG_LEN, rVal) # for class.copybits() next
    rVec.copybits(wVec.value, destStart=start, srcStart=0, nBits=nBits) # MODIFY
    wStat = mdio_wa(portAddr, devAddr, regAddr, rVec.value,addrRef=addrRef) # WRITE
  else:
    wStat = mdio_wa(portAddr, devAddr, regAddr, wVec.value,addrRef=addrRef) # WRITE
  return wStat
#-------------------------------------------------------------------------------

def ipmdio_wr(portAddr, devAddr, regAddr, wVal):
  if ipvar.MDIO_INIT:
    mdio_wr(portAddr, devAddr, regAddr, wVal)
  else:
    print "mdio_wr(%d, %d, 0X%04X, 0X%04X)" %(portAddr, devAddr, regAddr, wVal)
#-------------------------------------------------------------------------------

def ipmdio_rd(portAddr, devAddr, regAddr):
  if ipvar.MDIO_INIT:
    rVal = mdio_rd(portAddr, devAddr, regAddr)
  else:
    from random import randint
    rVal = randint(0, 0xffff) # slice it: 2**nBits-1?
    print "0X%04X <- mdio_rd(%d, %d, 0X%04X)" %(rVal, portAddr, devAddr, regAddr)
  return rVal
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
