# ------------------------------------------------------------------------------
#
# $RCSfile: validation#python#iputils.py,v $ - Utility methods for iPHY test scripts
#
# Disclaimer:
# Inphi Confidential
# Copyright(c) Inphi Corp. 2011
#
# All rights reserved.
# This is unpublished, confidential Inphi proprietary information.
# Do not reproduce or redistribute without written permission.
#
# $Header: /projects/wlv/iphy/sos/repository/iphy_A.rep/LIBS/FILES258/SRC/#alidation#iphy-gui#gui#iputils.py,v 1.3 2011-09-21 18:13:26-07 case Exp $
# $Log: #alidation#iphy-gui#gui#iputils.py,v $
# Revision 1.3  2011-09-21 18:13:26-07  case
# richard's latest change, dynamic devices
#
# Revision 1.4  2011-07-11 12:31:33-07  rbemra
# Updated device names (per rward), deleted unneeded IMB/ExacTik code
#
# Revision 1.3  2011-07-06 19:21:19-07  rbemra
# Fix for real FDI interface existence
#
# Revision 1.2  2011-07-05 18:43:10-07  rbemra
# Updates to get more things working: user scripts w/ options/threading, DDF+CSV writers, MDIO R/W updates, ipallreg addition
#
# Revision 1.1  2011-07-01 09:02:51-07  rbemra
# First, prelim. runnable version
#
# ------------------------------------------------------------------------------
import sys
#rb sys.path.append("N:/Validation/ExacTik/IP/scripts")

import time
import re
from ordict import *
from ctypes import *
import sys
import os

#import visa
#rb from ftdi import FTDI

from ipreg import *
import ipvar
import ipconfig
# import FPGAPacketCommand

from datetime import datetime

global last_voltage, last_freq
last_voltage = ""
last_freq = ""
last_VDD = ""
last_VTT = ""
last_VREF = ""
last_CLKOFF = ""
last_CLKOFFM = ""
last_temp = ""

# Packet command types
ConfigurationType = 0
AddressVectorType = 1
ReadVoltageType = 2
WriteVoltageType = 3
WriteEEPROMType = 4
ErroutMonitorType = 5
TempControllerType = 6
BoardResetType = 7
DelayAfterResetType = 8
fillerDQPacket = [0,0,0,0xf,0xc0,0,7,0xe0,0,3   ,0xf0,0,   1,0xf8,0,0   ]
FPGA_RUN = 0xff
debug=0
KeyPressNr = 0

# DAC temperature voltage range for -50C-125C
DACTempVoltRange = [512, 4095]

# --------------------------------------------------------------------

def create_system():
  print "create_system: Started with args:",sys.argv[1:]
  if(len(sys.argv) > 1):
    if(sys.argv[1] == '-fdi'):
      from ipmdio_fdi import *
  else:
    from ipmdio import *
  mdio_comms_setup()
  
  # Define the Top Level System Components
  if ipvar.System=={} or ipvar.System==None:
    # ipvar.System['ASIC-CAUI'] = DeviceDef(0, 'ASIC', 5) # ID=0, PRTAD=5
    try:
      ipvar.System = OrderedDict()
      fid = open(r'create_system.txt','r')
      print 'Loading system definitions from create_system.txt in',os.getcwd(),'...'
      for eachLine in fid:
        if eachLine.split(',')[0].find("SYS") == 0:
          eachLine = eachLine.rstrip('\n')
          print "Found",eachLine
          idNumber     = int(eachLine.split(',')[1])
          instanceName = eachLine.split(',')[2]
          PRTAD        = int(eachLine.split(',')[3])
          fileName     = eachLine.split(',')[4]

          ipvar.System[instanceName] = DeviceDef(idNumber,instanceName,PRTAD)
          ipconfig.load_regdefs_from_csv(ipvar.System[instanceName], fileName)
    except:
      ipvar.System = OrderedDict()
      print 'Loading system definitions from gui\iputils\create_system...'
      cdrInFile = "../dataconfig/iCDR100_internal_MDIO_flat.csv"
      ipvar.System[ipvar.CDR_Name] = DeviceDef(2, ipvar.CDR_Name, 0) # ID=2, PRTAD=0
      ipconfig.load_regdefs_from_csv(ipvar.System[ipvar.CDR_Name], cdrInFile)
  
      ipvar.System[ipvar.CDR1_Name] = DeviceDef(3, ipvar.CDR1_Name, 1) # ID=3, PRTAD=1
      ipconfig.load_regdefs_from_csv(ipvar.System[ipvar.CDR1_Name], cdrInFile)
      gbInFile = "../dataconfig/iPHY100_internal_MDIO_flat.csv"
      ipvar.System[ipvar.GB_Name] = DeviceDef(1, ipvar.GB_Name, 2) # ID=1, PRTAD=2
      ipconfig.load_regdefs_from_csv(ipvar.System[ipvar.GB_Name], gbInFile)

    # ipvar.System['Optical Module'] = DeviceDef(3, 'Optics', 7) # ID=3, PRTAD=7

    # XAUI_Name = 'TI-XAUI'
    # xauiInFile = "TLK3138_1v1.csv"
    # ipvar.System[XAUI_Name] = DeviceDef(4, XAUI_Name, 3) # ID=4, PRTAD=3
    # ipconfig.load_regdefs_from_csv(ipvar.System[XAUI_Name], xauiInFile)

    # initialize MDIO-FDI

    
    #ipvar.MDIO_INIT = False
    #mdio_init()
    #time.sleep(1)
    #mdio_set_speed(1)
    #time.sleep(1)
    #if True: #mdio_init():
    #  for kSpeed in [4, 3, 2, 1]: # set max. possible MHz speed
    #    if mdio_set_speed(kSpeed):
    #      ipvar.MDIO_INIT = True
    #     print "speed",kSpeed
    #      break
    #if not ipvar.MDIO_INIT:
    #  print "***********************************************************"
    #  print "MDIO interface not initialized ... using dummy register R/W"
    #  print "***********************************************************"
    #  time.sleep(2)

# --------------------------------------------------------------------

def printHex(inpData):  # printing list contents in hex 
  qty = len(inpData)
  sys.stdout.write('inHex->[')
  for i in xrange(qty-1):
    sys.stdout.write('%x, ' % inpData[i])
  sys.stdout.write('%x]\n' % inpData[qty-1])
# --------------------------------------------------------------------

def ppoCheckSum(vals):
  """
  ppoCheck(vals)
  Low level function for FPGA write
  Arguments:        vals - Array of bytes to be written
  Return:         None
  """
  
  calcChecksum = 0
  length=len(vals)
  for j in range(length):
    calcChecksum += vals[j]
  calcChecksum = 0xff ^ (0xff & calcChecksum)
  vals.append(calcChecksum)
  #rb ipvar.ftdiDriver.getExFTDI()
  retVal = None #rb ipvar.myftdi.writeFTDI(vals)
  #rb ipvar.ftdiDriver.releaseExFTDI()
  return 0 #rb retVal

  
def ContAccess_ppoCheckSum(vals):
  """
  ppoCheck(vals)
  Low level function for FPGA write
  Arguments:        vals - Array of bytes to be written
  Return:         None
  """
  
  calcChecksum = 0
  length=len(vals)
  for j in range(length):
    calcChecksum += vals[j]
  calcChecksum = 0xff ^ (0xff & calcChecksum)
  vals.append(calcChecksum)
  #rb retVal=ipvar.myftdi.writeFTDI(vals)
  return 0 #rb retVal

def ppo(vals):
  """
  ppo(vals)
  Low level function for FPGA write
  Arguments:        vals - Array of bytes to be written
  Return:       Packet read back as the result of the write operation. If the write doesn't require readback, it will contain invalid values, i.e. 0xff         
  """
  # Add a queue check for Rx

  #rb writeCount = ipvar.myftdi.writeFTDI(vals)
   # Parse the packet for ERROUT count
  #print "PPO out - Length =  ",   writeCount, "Packet =  ", vals
  #time.sleep(0.05)        #  print ppi()

  # return writeCount
  #rb ipvar.ftdiDriver.getExFTDI()
  #rb retVal=ipvar.myftdi.writeFTDI(vals)
  #rb ipvar.ftdiDriver.releaseExFTDI()
  return 0 #rb retVal

########################################################################
def checkParity(t=1):
    ppo([0x24, 0x2c, 0x24])  # clear parity: return true is parity is good
    time.sleep(t)
    if (ppi() & 0x40 ) == 0x40: parity =  True
    else: parity = False
    return parity
# --------------------------------------------------------------------

def VDD(volts):
  """
  VDD():   Sets the VDD value
  Arguments: volts - Voltage to be set 
  Return :   None
  """

  conversion_factor = 4095.0/500.0
    
  if ipvar.debug:
    print "The volts being passed to VDD(): "
    print volts
    print type(volts)

  #if (type(volts) is float):
  convertedVolts = "%1.03f"%volts + 'V'
  ipvar.currentVDD = volts
  #elif (type(volts) is str):
  #  convertedVolts = volts
  # ipvar.currentVDD = 1 + voltageParser(convertedVolts)/1000.0
  #  print "ipvar.currentVDD in VDD: %f"%ipvar.currentVDD
  #else:
  #  raise Exception("Illegal VDD type being passed!")
    
  fraction = voltageParser(convertedVolts)

  conversion = int((fraction - 100) * conversion_factor)

  if (ipvar.debug): 
    print "VDD cfg value"
    print conversion

    #rb ppo(FPGAPacketCommand.setVDD(conversion))
    print conversion
    
  ipvar.frame.buttonVoltage.SetValue("%s"%convertedVolts)
  ipvar.AppGui.Yield(True)

  VTT(ipvar.currentVDD/2)
  VREF(ipvar.currentVDD/2)
  if (ipvar.frame.cbClkOffEn.IsChecked() ==True): CLKOFF(ipvar.currentVDD/2)
# --------------------------------------------------------------------

def VTT(volts):
  """
  VTT():        Set the VTT voltage

  Arguments:        volts - Voltage value 
  Return:        None
  """
  conversion_factor = 4095.0/4500.0
    
  if (type(volts) is float):
    convertedVolts = "%1.04f"%volts + 'V'
  elif (type(volts) is str):
    convertedVolts = volts
  else:
    raise Exception("Illegal VTT type being passed!")

  fraction = voltageParser(convertedVolts)

  conversion = int((fraction - 4500) * conversion_factor)
  
  if (ipvar.debug):
    print "VTT cfg value"
    print conversion
 

  if ((conversion < 0) or (conversion > 4095)):
     raise Exception("Illegal VTT voltage request: %f"%conversion)
  
  #rb if (ipvar.debug):
  #rb   print FPGAPacketCommand.setVTT(conversion)     
  #rb else:
  #rb   ppo(FPGAPacketCommand.setVTT(conversion))

  ipvar.frame.buttonVoltageVTT.SetValue("%s"%convertedVolts)
  ipvar.AppGui.Yield(True)
# --------------------------------------------------------------------

def VREF(volts):
  """
  VREF():        Set the VREF value

  Arguments:        volts - Voltage value
  Return:        None
  """
  
  # DAC range for 0-3V, input 0-4095. To limit VREF in 0.45-0.9V, the input range is in 615-1228
  conversion_factor = 613/4500.0

  if (type(volts) is float):
    convertedVolts = "%1.04f"%volts + 'V'
  elif (type(volts) is str):
    convertedVolts = volts
  else:
    raise Exception("Illegal VREF type being passed!")

  fraction = voltageParser(convertedVolts)

  conversion = 615 + int((fraction - 4500) * conversion_factor)

  #print fraction
  if (ipvar.debug):
    print "VREF cfg value: "
    print conversion
 
  if ((conversion < 615 ) or (conversion > 1228)):
     raise Exception("Illegal VREF request: %f"%conversion)
  
  #rb if (ipvar.debug): print FPGAPacketCommand.setVREF(conversion)
  #rb else:
  #rb  ppo(FPGAPacketCommand.setVREF(conversion))

  ipvar.frame.buttonVoltageVREF.SetValue("%s"%convertedVolts)
  ipvar.AppGui.Yield(True)
# --------------------------------------------------------------------

def CLKOFF(volts, polarity=None):
  """
  CLKOFF():        Set the clock offset

  Arguments:        temp - Clock offset value
  Return:        None
  """
  # For conversion, see VREF
  conversion_factor = 613/4500.0

  if (type(volts) is float):
    convertedVolts = "%1.04f"%volts + 'V'
  elif ((type(volts) is str) or (type(volts) is unicode)):
    convertedVolts = volts
  else:
    raise Exception("Illegal CLKOFF type being passed!")

  fraction = voltageParser(convertedVolts)

  conversion = 615 + int((fraction - 4500) * conversion_factor)

  #print fraction
  if (ipvar.debug):
    print "Clk off cfg value: "
    print conversion

  if ((conversion < 615) or (conversion > 1228)):
     raise Exception("Illegal CLK offset request: %f"%conversion)


  if False: #rb
    if ((polarity==None) or (polarity=='+')): 
      ppo(FPGAPacketCommand.setCLKOFF(conversion))
      if (ipvar.debug): printHex(FPGAPacketCommand.setCLKOFF(conversion))
    elif (polarity == '-' ): 
      ppo(FPGAPacketCommand.setCLKOFFM(conversion))
      if (ipvar.debug): printHex(FPGAPacketCommand.setCLKOFFM(conversion)) 
    else:  raise Exception("CLKOFF polarity error!")

  if ((polarity==None) or (polarity=='+')): ipvar.frame.buttonVoltageClkOff.SetValue("%s"%convertedVolts)
  else:  ipvar.frame.buttonVoltageClkOffM.SetValue("%s"%convertedVolts)
  ipvar.AppGui.Yield(True)
# --------------------------------------------------------------------

def OnButton_Temp(event):
  """
  """
  global last_temp

  sel = event.GetString()
  
  if (sel == last_temp):
    last_temp = ""
    return

  last_temp = sel
  
  #rb TEMPERATURE(sel)
# --------------------------------------------------------------------

def OnButton_TempCoolSwitch(ON):
  """
  """
  tempCtrl = ipvar.tempController

  if (ON):
    tempCtrl = tempCtrl | 0x1
    if (ipvar.debug): print "Cool turned on"
  else:
    tempCtrl = tempCtrl & 0xFE
    if (ipvar.debug): print "Cool turned off"

  ipvar.tempController = tempCtrl
  print ipvar.tempController
  tempSwitch(tempCtrl)
# --------------------------------------------------------------------

def OnButton_TempHotSwitch(ON):
  """
  """
  tempCtrl = ipvar.tempController
  
  if (ON):
    tempCtrl = tempCtrl | 0x2
    if (ipvar.debug): print "Hot turned on"
  else:
    tempCtrl = tempCtrl & 0xFD
    if (ipvar.debug): print "Hot turned off"

  ipvar.tempController = tempCtrl
  print ipvar.tempController
# --------------------------------------------------------------------

def OnButton_ClkOff(event):
  """
  """
  global last_CLKOFF

  sel = event.GetString()
  
  if (sel == last_CLKOFF):
    last_CLKOFF = ""
    return

  last_CLKOFF = sel
  
  if (ipvar.frame.cbClkOffEn.IsChecked()): CLKOFF(sel, '-')
  CLKOFF(sel)
# --------------------------------------------------------------------

def OnButton_ClkOffM(event):
  print "Changing clock offset minus"
  global last_CLKOFFM

  sel = event.GetString()
  
  if (sel == last_CLKOFFM):
    last_CLKOFFM = ""
    return

  last_CLKOFFM = sel

  CLKOFF(sel, '-')  
# ----------------------------------------------------------------------

def voltageParser(volt):
  """
  voltageParser():        Parse the voltage selection string and retrun the fraction

  Arguments:        Voltage selection from GUI drop down menu
  Return:        The fraction of the voltage value
  """
  
  r1 = re.compile(r'[\.V]');
  parts = r1.split(volt)

  return int(parts[1]) 
# ----------------------------------------------------------------------

def initStatusBar():
  """
  initStatusBar():  Populate the status bar with information such as motherboard
                    rev/serial, daugther rev/serial and owner
  Arguments:            None
  Return:            None
  """
  
  # Initializes the status bar text string
  statusText = 'EVB Rev: PCB-174-A;'

  # EVB serial
  statusText += ' S/N: 1;'
  statusText += ' DUT ID: CDR0;'
  statusText += ' Clk: ??'

  # Display through the status bar
  ipvar.frame.statusBar.SetStatusText(statusText, 0)
# ----------------------------------------------------------------------

def initTemp():
  """
  initTemp():        Initializes the temperature by calibrating to the stored reference point values

  Arguments:        None
  Return:        True - Success; False - Failure
  """

  t0CHi = 2.0 #rb readEEPROM(0, 4) 
  t0CLo = 1.0 #rb readEEPROM(0, 5)
  t100CHi = 103 #rb readEEPROM(0, 6)
  t100CLo = 97 #rb readEEPROM(0, 7)

  if ( (t0CHi > 99) or (t0CLo > 990) or (t100CHi < 90) or (t100CLo > 990) ): #rb 
    print "Wrong temperature voltage reference value in EEPROM!"
    return False

  t0CVal = (t0CHi*100) + t0CLo
  t100CVal = (t100CHi*100) + t100CLo

  # Calibrate the temperature-voltage points using the 0C/4095 as base point to calibrate the 0C/125C point

  # Calibrate 0C
  ipvar.DACTempVoltRange[1] = int(((t0CVal - t100CVal)/1500.0)*4095)
  if (ipvar.DACTempVoltRange[1]<=4095): pass
  elif (ipvar.DACTempVoltRange[1] > 4095):  ipvar.DACTempVoltRange[1] = 4095
  else:  pass

  # Calibrate 125C
  ipvar.DACTempVoltRange[0] = int(((t0CVal - t100CVal)/1500.0)*((25/200.0)*4095))

  if ipvar.debug: 
    print t0CVal, t100CVal
    #rb oo
    print "DAC calibation base for 125C: %d"%ipvar.DACTempVoltRange[0]
    print "DAC calibation base for -50C: %d"%ipvar.DACTempVoltRange[1]

  return True
# ----------------------------------------------------------------------

def SleepRefreshDisplay(t):
  saveConfig = ipvar.RealTime
  ipvar.RealTime = 1
  ipvar.frame.dispMonitors()
  ipvar.frame.Refresh()
  time.sleep(t)
  ipvar.RealTime = saveConfig
# ---------------------------------------------------------------------

def logFileOpen():
  """
  logFileOpen:        Open a log file if not already
  """
  now = datetime.today()
  if (ipvar.logFile==True and ipvar.handleLogFile==None):
    ipvar.handleLogFile = open("vectorlog%s.txt"%now.strftime("-%H-%M-%S"), 'a')
  else: pass
# ---------------------------------------------------------------------

def logFileClose():
  """
  logFileClose():  Close the log file
  """
  ipvar.handleLogFile.close()
  ipvar.handleLogFile = None
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
