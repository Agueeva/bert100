# ------------------------------------------------------------------------------
#
# $RCSfile: validation#python#ipmdio.py,v $ - MDIO interface: iPHY <-> SUB-20
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
#GJL - 10/20 Add -ATE_log to mdio_wr only

import re
import ipvar
from time import sleep
from ctypes import *
libc = cdll.LoadLibrary("msvcrt") # or cdll.msvcrt  # linux: CDLL("libc.so.6")
## lmdio = cdll.LoadLibrary("USBMPC.dll") # cdecl calling convention
#lmdio = windll.LoadLibrary("USBMPC") # stdcall calling convention
lmdio = cdll.LoadLibrary("sub20.dll")

sub_mdio45 = lmdio.sub_mdio45
sub_mdio45.argtypes = [c_void_p, c_int, c_int, c_int, c_int, POINTER (c_int)]
#JT function protoype: int   sub_mdio_xfer_ex( sub_handle hndl, int channel, int count, union sub_mdio_frame* mdios )
sub_mdio_xfer_ex = lmdio.sub_mdio_xfer_ex
sub_mdio_xfer_ex.argtypes = [c_void_p, c_int, c_int, POINTER (c_int)]

MDIO_CH = 0 #JT default setting of MDIO ch0 (not 4MHz MSA mode)
SUB_CFP_MSA = 128 #JT - from libsub.h - flag value to bitwise-OR (or integer add) with SUB_MDIO_CH to enable 4MHz MSA mode
SUB_MDIO_CH = MDIO_CH + SUB_CFP_MSA #JT - Valid SUB_MDIO_CH values for slow-mode are 0 (MDIO0, SUB-20), 1 (MDIO1, SUB-20) and 4MHz mode valid values are 0x80 and 0x81 

# class sub_device(Structure):
  # pass
# sub_device._fields_ = [("micro", c_int), ("minor", c_int), ("major", c_int), ("boot", c_int)]

# class driver(Structure):
  # pass
# driver._fields_ = [("major", c_int), ("minor", c_int), ("micro", c_int), ("nano", c_int)]

# class dll(Structure):
  # pass
# dll._fields_ = [("major", c_int), ("minor", c_int), ("micro", c_int), ("nano", c_int)]
   
# class sub_version(Structure):
  # pass
# sub_version._fields_ = [("dll", dll), ("driver", driver), ("sub_device", sub_device)] 

# global sub20_info
# global sub20_info_p
# sub20_info = (sub_version)()
# sub20_info_p = pointer(sub20_info)

class clause22(Structure):
  pass
clause22._fields_ = [("op", c_int), ("phyad", c_int), ("regad", c_int), ("data", c_int)]

class clause45(Structure):
  pass
clause45._fields_ = [("op", c_int), ("prtad", c_int), ("devad", c_int), ("data", c_int)]

class sub_mdio_frame(Union):
  pass
sub_mdio_frame._fields_ = [("clause22", clause22), ("clause45", clause45)]

mdios = (sub_mdio_frame * 2)()
#cast(mdios, POINTER(c_int))
mdios[0].clause22.op = -1
mdios[0].clause22.phyad = -1
mdios[0].clause22.regad = -1
mdios[0].clause22.data = -1
mdios[1].clause22.op = -1
mdios[1].clause22.phyad = -1
mdios[1].clause22.regad = -1
mdios[1].clause22.data = -1


#JT - SUB-20 Definitions
#
# Opcodes for SUB-20 MDIO45 operations
SUB_MDIO45_ADDR = 0   #ADDRESS operation
SUB_MDIO45_WRITE = 1  #WRITE operation
SUB_MDIO45_READ = 3   #READ operation
SUB_MDIO45_PRIA = 2   #POST-READ-INCREMENT-ADDRESS operation

global sub20_handle
sub20_handle = None 

global MDIO_NO_ERROR
global MDIO_USB_OPEN_FAILURE
global MDIO_USB_COMM_FAILURE
global I2C_NO_ERROR

#MDIO_RST_ERROR = -19 #JT - added for SUB-20 errno... might want to take care of +19 as well. Frozen is negative?
MDIO_NO_ERROR = 0
MDIO_USB_OPEN_FAILURE = 1000
MDIO_USB_COMM_FAILURE = 1001
USBMPC_I2C_SPEED_100KHZ = 500
USBMPC_I2C_SPEED_400KHZ = 125
I2C_NO_ERROR = 0

NWR = 1

#JT - example of restype for defining sub_handler return type? lmdio.Mdio45WriteWord.restype = c_int


#JT - decodes the errno into text and prints it
def print_errmsg(errno):
  err_msg = create_string_buffer('\000' * 60)
  err_msg = c_char_p(lmdio.sub_strerror(errno))
  print format(err_msg.value), '|errno=',
  return errno
  
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

# def mdio_wr(portAddr, devAddr, regAddr, wVal):
# #  valC = c_int(wVal)
  # # i = c_int(1)
# #  returned = pointer(valC)
  
  # #sub_mdio45 = lmdio.sub_mdio45
  # #sub_mdio45.argtypes = [c_long, c_int, c_int, c_int, c_int, POINTER (c_int)]
  # #
  # # seems to be an iCDR bug where one has to write twice or write+read
  # # in order to apply the register change
  # #
  # for nTimes in range(NWR):
    # wError = sub_mdio45(sub20_handle, SUB_MDIO45_ADDR, c_int(portAddr), c_int(devAddr), c_int(regAddr), returned)
    # wError = sub_mdio45(sub20_handle, SUB_MDIO45_WRITE, c_int(portAddr), c_int(devAddr), wVal, returned)
    # #print 'WRITE attempt: P=', portAddr, '   D=', devAddr, '   R=', regAddr, '   V=', hex(wVal) #JT

    # if (wError!=MDIO_NO_ERROR):
      # raise Exception("MDIO Write Error!")
# #-------------------------------------------------------------------------------

def mdio_wr(portAddr, devAddr, regAddr, wVal):
#  valC = c_int(wVal)
  # i = c_int(1)
#  returned = pointer(valC)
  if '-ATE_log' in sys.argv:
    fid = open('ate_log.csv','a')
    fid.write(("W %s.%s 0x%04x\n") %(devAddr,regAddr,wVal))
    fid.close()  
  mdios[0].clause45.op = c_int(SUB_MDIO45_ADDR)
  mdios[0].clause45.prtad = c_int(portAddr) 
  mdios[0].clause45.devad = c_int(devAddr)
  mdios[0].clause45.data = c_int(regAddr)
  mdios[1].clause45.op = c_int(SUB_MDIO45_WRITE)
  mdios[1].clause45.prtad = c_int(portAddr) 
  mdios[1].clause45.devad = c_int(devAddr)
  mdios[1].clause45.data = c_int(wVal)
  mdio_ptr = cast(mdios, POINTER(c_int))  
  #sub_mdio45 = lmdio.sub_mdio45
  #sub_mdio45.argtypes = [c_long, c_int, c_int, c_int, c_int, POINTER (c_int)]
  #
  # seems to be an iCDR bug where one has to write twice or write+read
  # in order to apply the register change
  #
  for nTimes in range(NWR):
    #wError = sub_mdio45(sub20_handle, SUB_MDIO45_ADDR, c_int(portAddr), c_int(devAddr), c_int(regAddr), returned)
    #wError = sub_mdio45(sub20_handle, SUB_MDIO45_WRITE, c_int(portAddr), c_int(devAddr), wVal, returned)
    #print 'WRITE attempt: P=', portAddr, '   D=', devAddr, '   R=', regAddr, '   V=', hex(wVal) #JT
    wError = sub_mdio_xfer_ex(sub20_handle, SUB_MDIO_CH, 2, mdio_ptr)
    if (wError!=MDIO_NO_ERROR):
      raise Exception("MDIO Write Error!")
#-------------------------------------------------------------------------------
    
#JT - Performance Optimization - (1)try byref instead of pointer, (2)use sub_mdio_xfer_ex
def mdio_rd(portAddr, devAddr, regAddr):
  #print 'mdio_rd: sub20_handle=', sub20_handle
  
  #  valC = c_int(-1)
  # i = c_int(1)
#  returned = pointer(valC)
  mdios[0].clause45.op = c_int(SUB_MDIO45_ADDR)
  mdios[0].clause45.prtad = c_int(portAddr) 
  mdios[0].clause45.devad = c_int(devAddr)
  mdios[0].clause45.data = c_int(regAddr)
  mdios[1].clause45.op = c_int(SUB_MDIO45_READ)
  mdios[1].clause45.prtad = c_int(portAddr) 
  mdios[1].clause45.devad = c_int(devAddr)
  mdios[1].clause45.data = c_int(regAddr)
  mdio_ptr = cast(mdios, POINTER(c_int))
  

  #print 'READ attempt: P=', portAddr, '   D=', devAddr, '   R=', regAddr, #JT
  readError = sub_mdio_xfer_ex(sub20_handle, SUB_MDIO_CH, 2, mdio_ptr)
  #print hex(mdios[1].clause45.data);
  #print returned[0] #JT
  #print 'mdio_rd: ', print_errmsg(readError),
  if (readError!=MDIO_NO_ERROR):
    raise Exception("MDIO ReadError!")
  return mdios[1].clause45.data
  
# def mdio_rd(portAddr, devAddr, regAddr):
  # valC = c_int(-1)
  # # i = c_int(1)
  # returned = pointer(valC)
  
  # #sub_mdio45 = lmdio.sub_mdio45
  # #sub_mdio45.argtypes = [c_long, c_int, c_int, c_int, c_int, POINTER (c_int)]
  # #print 'sub20_handle=',sub20_handle
  
  # #JT - Send the SUB_MDIO45_ADDR, then call with SUB_MDIO45_READ opcode
  # #print 'ADDR attempt: P=', portAddr, '   D=', devAddr, '   R=', regAddr
  # readError = sub_mdio45(sub20_handle, SUB_MDIO45_ADDR, c_int(portAddr), c_int(devAddr), c_int(regAddr), returned)
  # #print 'mdio_rd: ', print_errmsg(readError), '  Returned:',returned[0]
  
  # #print 'READ attempt: P=', portAddr, '   D=', devAddr, '   R=', regAddr,
  # readError = sub_mdio45(sub20_handle, SUB_MDIO45_READ, c_int(portAddr), c_int(devAddr), c_int(regAddr), returned)
  # #print 'mdio_rd: ', print_errmsg(readError), '  Returned:',hex(returned[0])
  # #print '     ',hex(returned[0])
  # if (readError!=MDIO_NO_ERROR):
    # raise Exception("MDIO ReadError!")
  # return returned[0]
# #-------------------------------------------------------------------------------
 
####JT - scratchpad START
#
# ORIGINAL
#
# def mdio_rd(portAddr, devAddr, regAddr):
  # valC = c_int(-1)

  # readError = lmdio.Mdio45ReadWord(c_int(portAddr), c_int(devAddr),
                                   # c_int(regAddr), byref(valC))
  # if (readError!=MDIO_NO_ERROR):
    # raise Exception("MDIO ReadError!")
  # return valC.value
# #-------------------------------------------------------------------------------

def mdio_wa(portAddr, devAddr, regAddr, wVal, waitSec=0,addrRef=''):
  if portAddr == -1:  # catch the optics module and use the tunnel
    cdrRef = int(re.match('M-CDR(\d)::.*',addrRef).group(1))
    cdr_write(cdrRef,regAddr,wVal)
  else:
    mdio_wr(portAddr, devAddr, regAddr, wVal)
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

#########JT - fix sticky errno...

def mdio_sub_open():
  global sub20_handle
  sub20_handle = lmdio.sub_open(0)
  errno = c_int.in_dll (lmdio, "sub_errno")
#  print 'mdio_sub_open: ', print_errmsg(errno)
  return sub20_handle

def mdio_comms_setup(): #JT called from iputils.create_system from ipallreg
  global sub20_handle
  ipvar.MDIO_INIT = False
  sub20_handle = mdio_sub_open()
  #print "sub20_handle = ", sub20_handle, '\n'
 
  if (sub20_handle == 0):
    print "Initial MDIO reponse failed. Attempting to reestablish connection..."
    if '-nohardware' in sys.argv:
	  print '\n*** No hardware mode - skipping MDIO check ***\n'
    else:  
      for i in range(1,11):
        print str(i) + ',',
        sleep(1)

		  
        sub20_handle = mdio_sub_open()
        if not sub20_handle == 0:
          print "Established."
          ipvar.MDIO_INIT = True
          errno = get_sub20_info(sub20_handle)
          #return sub20_handle
          break
        if i == 10:
          print
          print "***********************************************************"
          print "MDIO communicaton not established. Using dummy register R/W"
          print "***********************************************************"
  else:
    ipvar.MDIO_INIT = True
    errno = get_sub20_info(sub20_handle)
    #return sub20_handle
  
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
  
##############################################################
# START SUB-20 EXAMPLE
#need to raise exception if dev_id is 0
def get_sub20_info (sub20_handle):
  #
  #Returnes a string which has the aaa product name and serial number
  #of the mdio interface device
  #
  #helper = cdll.LoadLibrary("C:\Windows\System32\sub20.dll")

  #JT: Check if a SUB-20 in connected to Host via USB
  #dev_id_try = 0
  #while( dev_id_try == lmdio.sub_find_devices(dev_id_try)):
  #  if dev_id_try != 0:
  #    break

#  dev_id = lmdio.sub_open (0) #JT - changed dev_id to sub20_handle
#  errno = c_int.in_dll (lmdio, "sub_errno")
#  print ("SUB-20 Device ID is: {0} sub_errno is: {1}".format (dev_id, errno.value))

  print '==== XDIMAX.COM SUB-20 MDIO<->USB ====='
#  #JT Decode errno.value to a string using sub_strerror
#  err_msg = create_string_buffer('\000' * 60)
#  err_msg = c_char_p(lmdio.sub_strerror(errno.value))
#  print ('Error decoded: {0}'.format(err_msg.value))
  
  #JT - Check that handle is valid before getting info - a return of MDIO_RST_ERROR(-19) means the SUB-20 is not open/init/available yet
  errno = lmdio.sub_reset(sub20_handle)
#  print 'get_sub20_info: sub_reset() returned...',  print_errmsg(errno)
  if errno != 0:
    return errno

  sub_get_product_id = lmdio.sub_get_product_id
  sub_get_product_id.argtypes = [c_int, c_char_p, c_int]
  s = create_string_buffer('\000' * 60)
  sub_get_product_id (sub20_handle, s, 60)
  print 'Device ID: ', s.value.decode("utf-8")

  sub_get_serial_number = lmdio.sub_get_serial_number
  sub_get_serial_number.argtypes = [c_int, c_char_p, c_int]
  s2 = create_string_buffer('\000' * 67)
  sub_get_serial_number (sub20_handle, s2, 67)
  print 'Serial No: ', s2.value.decode("utf-8")
  
  #JT - decode SUB_MDIO_CH for display and todo - add create_system definition for SUB/FDI and SUB_MDIO_CH 
  if (SUB_MDIO_CH == 128):
    print 'Channel  :  0'
    print 'MDC Freq :  4MHz CFP MSA mode'
  elif (SUB_MDIO_CH == 129):
    print 'Channel  :  1'
    print 'MDC Freq :  4MHz CFP MSA mode'
  elif (SUB_MDIO_CH == 0):
    print 'Channel  :  0'
    print 'MDC Freq :  1.5MHz'
  elif (SUB_MDIO_CH == 1):
    print 'Channel  :  1'
    print 'MDC Freq :  1.5MHz'
  else:
    print 'Channel  :  N/A'
    print 'MDC Freq :  (set SUB_MDIO_CH to valid value)'
  
  # global sub20_info
  # global sub20_info_p
  # # JT todo - get version numbers and display
  # sub_get_version = lmdio.sub_get_version
  # sub_get_version.argtypes = [c_void_p]
  # sub_get_version.restypes = [sub_version]
  # sub20_info_p = sub_get_version(sub20_handle)
  # # sub20_info = sub_version.in_dll (lmdio, "sub_version")
  # sub20_info = sub20_info_p
  # print 'Version Numbers - '
  # print 'DLL:',sub20_info.dll.major,'.', sub20_info.dll.minor, '.', sub20_info.dll.micro, '.', sub20_info.dll.nano
  # print 'DRV:',sub20_info.driver.major,'.', sub20_info.driver.minor, '.', sub20_info.driver.micro, '.', sub20_info.driver.nano
  # print 'SUB:',sub20_info.sub_device.micro,'.', sub20_info.sub_device.minor, '.', sub20_info.sub_device.major, '.', sub20_info.sub_device.boot
  
  print '======================================='
#JT add sub reset while device is open and check return value = 0?
#JT lmdio.sub_reset() returns -19 if SUB-20 is not open and reset is attempted		
#  errno = lmdio.sub_reset(sub20_handle)
#  print 'RESET:', print_errmsg(errno)	

  #lmdio.sub_close (dev_id)

  return errno

def transaction (op_code, phy_address, device_address, data):
  """
  Execute a mdio cluase 45 transaction.
  Returnes an integer on read or read post increment.
  op_code
  0 is for Address
  1 is for Write
  2 is for Read
  3 is for Read post increment
  """
  #helper = cdll.LoadLibrary("C:\Windows\System32\sub20.dll")
  #dev_id = lmdio.sub_open (0)
  #errno = c_int.in_dll (lmdio, "sub_errno")
  #print ("device id is: {0} sub_errno is: {1}".format (dev_id, errno.value))
  print 'sub20_handle=', sub20_handle, '  opcode=', op_code, '  phy_addr=',phy_address, '  dev_addr=', device_address, '  data=', data

  sub_mdio45 = lmdio.sub_mdio45
  sub_mdio45.argtypes = [c_int, c_int, c_int, c_int, c_int, POINTER (c_int)]

  i = c_int(1)
  returned = pointer(i)
  foo = sub_mdio45 (sub20_handle, op_code, phy_address, device_address, data, returned)
  errno = c_int.in_dll (lmdio, "sub_errno")
  print ("returned is: {0} sub_errno is: {1}".format (foo, errno.value))
  print 'MMD: ', hex(returned[0])

  #lmdio.sub_close (dev_id)

  return returned[0]

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
  #print 'regValRead: sub20_handle=', sub20_handle
  
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

def regWriteAck(addrRef, wVal):
  if not ipvar.MDIO_INIT:
    #print "regWriteAck( '%s'," % addrRef, wVal, ")"
    return True
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

#JT - remember to remove when running iPHY_GUI.bat
# mdio_comms_setup() #JT export global sub20_handle? instead of passing it back from mdio_comms_setup()
# print 'INITRUN_sub20_handle=',sub20_handle
# transaction(SUB_MDIO45_ADDR,2,8,3)
# transaction(SUB_MDIO45_READ,2,8,3)

# END SUB-20 EXAMPLE
#########################################################
# def transaction (op_code, phy_adrdess, device_address, data):
  # """
  # Execute a mdio cluase 45 transaction.
  # Returnes an integer on read or read post increment.
  # op_code
  # 0 is for Address
  # 1 is for Write
  # 2 is for Read
  # 3 is for Read post increment
  # """
  # #helper = cdll.LoadLibrary("C:\Windows\System32\sub20.dll")
  # #dev_id = lmdio.sub_open (0)
  # #errno = c_int.in_dll (lmdio, "sub_errno")
  # #print ("device id is: {0} sub_errno is: {1}".format (dev_id, errno.value))

  # sub_mdio45 = lmdio.sub_mdio45
  # sub_mdio45.argtypes = [c_int, c_int, c_int, c_int, c_int, POINTER (c_int)]

  # i = c_int(1)
  # returned = pointer(i)
  # foo = sub_mdio45 (sub20_handle, op_code, phy_adrdess, device_address, data, returned)
  # errno = c_int.in_dll (lmdio, "sub_errno")
  # print ("returned is: {0} sub_errno is: {1}".format (foo, errno.value))

  # #lmdio.sub_close (dev_id)

  # return returned[0]

