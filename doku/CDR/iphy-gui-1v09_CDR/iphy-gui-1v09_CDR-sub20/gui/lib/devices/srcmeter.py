#
# $Header: /projects/wlv/iphy/sos/repository/iphy_A.rep/TEMP/case_1285613542/#alidation#iphy-gui#gui#lib#devices#srcmeter.py,v 1.1 2011-09-02 11:43:52-07 case Exp $
# Old Header: /inphi/inlab/pylab/devices/srcmeter.py,v 1.2 2008/07/23 02:10:14 rbemra Exp
# $Log: #alidation#iphy-gui#gui#lib#devices#srcmeter.py,v $
# Revision 1.1  2011-09-02 11:43:52-07  case
# ...No comments entered during checkin...
#
# Revision 1.2  2011-08-31 19:34:52-07  rbemra
# Fix for Instek vsupply PROT.
#
# Revision 1.1  2011-05-04 18:10:35-07  rbemra
# Initial python+pyvisa for instrumentation (from ExacTik/882)
#
# Revision 1.2  2008/07/23 02:10:14  rbemra
# Added HP3631A code ported from solaris/Ops: partially tested
#
# Revision 1.1.1.1  2008/07/21 18:20:28  rbemra
# Initial Version
#
#
# Power Supply remote interface
# Packaged under pylab 7/21/2008: RSB

import time
from labdev import *
from meas_utils import *

SRC_MPS = 0
SRC_E3631A = 1
SRC_INSTEK = 2
SRC_KTHLY = 3
SrcDict = {
  'MPS':SRC_MPS,
  '3631A':SRC_E3631A,
  'PST3202':SRC_INSTEK,
  'Keithley':SRC_KTHLY,
}
# ChDict[chNum] is the channel name for GPIB, chNum is Python/user/device-dashboard channel number
ChName_E3631A_Dict = {
  1:'P6V',
  2:'P25V',
  3:'N25V'
}
class SrcMeter(LabDev):
  chDict = None
  vSet = None
  chSel = None
  def __init__(self, name):
    LabDev.__init__(self, name)
    LabDev.set_model(self, SrcDict, DEV_SRC)
    if self.modelType == SRC_E3631A:
      self.chDict = ChName_E3631A_Dict

  def return_to_local(self):
    if self.modelType == SRC_KTHLY:
      self.viDev.write(":SYST:LOC")
    else:
      raise Exception("Return to local control for Keithley only!")

  def set_autozero(self, On = True):
    if On:
      onValue = "On"
    else:
      onValue = "OFF"
    if self.modelType == SRC_KTHLY:
      self.viDev.write(":SYST:AZER "+onValue)
    else:
      raise Exception("Auto-zero not supported!")
	  
  def sel_ch(self, chNum=None):
    if chNum==None:
      return
    if (chNum<0 or chNum>3):
      raise Exception("Illegal supply channel")
    chSel = chNum
    if self.modelType == SRC_E3631A:
      self.viDev.write("INST:SEL "+self.chDict[chNum])

  def set_output_autorange(self):
    if self.modelType == SRC_KTHLY:
      self.viDev.write(":CURR:RANG:AUTO ON")
      self.viDev.write(":VOLT:RANG:AUTO ON")
    else:
      raise Exception("Output ranging settings for Keithley only")

  def set_v(self, vNext, chNum=None):
    self.sel_ch(chNum)
    if self.modelType == SRC_E3631A:
      self.viDev.write("SOUR:VOLT:LEV %e"%vNext)
    elif self.modelType == SRC_INSTEK:
      vProt = 1.1 * vNext
      self.viDev.write(":CHAN%d:PROT:VOLT %e"%(chNum, vProt)) # OVP on
      self.viDev.write(":CHAN%d:VOLT %e"%(chNum, vNext))
      # time.sleep(1e-3)
    elif self.modelType == SRC_MPS:
      self.viDev.write("SOUR:VOLT:LEV %e,(@%s)"%(vNext, self.chDict[chNum]))
    # Keithley set output voltage
    elif self.modelType == SRC_KTHLY:
      self.viDev.write(":SOUR:VOLT %e"%vNext)
    else:
      raise Exception("Unknown source model")

  def set_i(self, iNext, chan=None):
    self.sel_ch(chan)
    if self.modelType == SRC_E3631A:
      self.viDev.write("SOUR:CURR:LEV %e"%iNext)
    elif self.modelType == SRC_INSTEK:
      self.viDev.write(":CHAN%d:CURR %e"%(chan, iNext))
      self.viDev.write(":CHAN%d:PROT:CURR 1"%chan) # OCP on
    elif self.modelType == SRC_MPS:
      self.viDev.write("MEAS:CURR:DC %e DEF,DEF,(@%s)"%(iNext, chan))
    # Set current for Keithley
    elif self.modelType == SRC_KTHLY:
      self.viDev.write(":SOUR:CURR %e"%iNext)
    else:
      raise Exception("Unknown source model")

  def set_vlimit(self, vLimit, chan=None):
    self.sel_ch(chan)
    if self.modelType == SRC_E3631A:
      self.viDev.write("")
    elif self.modelType == SRC_INSTEK:
      self.set_v(vLimit, chan)
    # Set voltage limit for Keithley
    elif self.modelType == SRC_KTHLY:
      if vLimit > 1.575:
        vLimit = 1.575
      self.viDev.write(":VOLT:PROT %e"%vLimit)
    else:
      raise Exception("Unknown source model")

  def set_ilimit(self, iLimit, chan=None):
    self.sel_ch(chan)
    if self.modelType == SRC_E3631A:
      self.viDev.write("SOUR:CURR:LEV %e"%iLimit)
    elif self.modelType == SRC_INSTEK:
      self.viDev.write(":CHAN%d:CURR %e"%(chan, iLimit))
      self.viDev.write(":CHAN%d:PROT:CURR 1"%chan)
    elif self.modelType == SRC_KTHLY:
      if iLimit > 0.6:
        iLimit = 0.6 # max 600mA
      self.viDev.write(":CURR:PROT %e"%iLimit)
    else:
      raise Exception("Unknown source model")

  def get_v(self, chan=None):
    self.sel_ch(chan)
    if self.modelType == SRC_E3631A:
      vMeas = float(self.viDev.ask("MEAS:VOLT:DC?"))
      # vMeas = float(self.viDev.ask("SOUR:VOLT:LEV?"))
      self.viDev.ask("MEAS:CURR:DC?") # to restore display of current
    elif self.modelType == SRC_INSTEK:
      vMeas = float(self.viDev.ask(":CHAN%d:MEAS:VOLT?"%chan))
    elif self.modelType == SRC_MPS:
      vMeas = float(self.viDev.ask("MEAS:VOLT:DC? DEF,DEF,(@%s)"%chan))
    # Get voltage reading from Keithley
    elif self.modelType == SRC_KTHLY:
      self.viDev.write(":FORM:ELEM VOLT")
      vMeas = float(self.viDev.ask(":MEAS:VOLT?"))
    else:
      raise Exception("Unknown source model")
    return vMeas

  def get_i(self, chan=None):
    self.sel_ch(chan)
    if self.modelType == SRC_E3631A:
      iMeas = float(self.viDev.ask("MEAS:CURR:DC?"))
      self.viDev.ask("MEAS:CURR:DC?")
    elif self.modelType == SRC_INSTEK:
      iMeas = float(self.viDev.ask(":CHAN%d:MEAS:CURR?"%chan))
    elif self.modelType == SRC_MPS:
      iMeas = float(self.viDev.ask("MEAS:CURR:DC? DEF,DEF,(@%s)"%chan))
    elif self.modelType == SRC_KTHLY:
      self.viDev.write(":FORM:ELEM CURR")	    
      iMeas = float(self.viDev.ask(":MEAS:CURR?"))
    else:
      raise Exception("Unknown source model")
    return iMeas

  def on_off(self, On=False):
    if On:
      swVal = "ON"
    else:
      swVal = "OFF"
    if self.modelType == SRC_E3631A:
      self.viDev.write(":OUTPUT "+swVal)
    elif self.modelType == SRC_INSTEK:
      self.viDev.write(":OUTP:STAT %d"%On)
    # Enable/Disable Keithley output
    elif self.modelType == SRC_KTHLY:
      self.viDev.write(":OUTP:STAT "+swVal)
    else:
      raise Exception("Unknown source model")
#-------------------------------------------------------------------

class HP3631A(SrcMeter):
  def PwrOn (self): # no "supply_no" arg: each instance separate
    self.viDev.write("OUTP ON")
    return 0
  #--------------------------------------------------------------
  
  def PwrOff(self):
    self.viDev.write("OUTP OFF")

    [v, c] = PwrSet( 1, 0.0, 0.0, 0)
    [v, c] = PwrSet( 2, 0.0, 0.0, 0)
    [v, c] = PwrSet( 3, 0.0, 0.0, 0)
  
    return 0
  #-------------------------------------------------------------------------

  def PwrMeas(self, supply): #  float *v, float *c
    volt = self.get_v(supply)
    amp = self.get_i(supply)
    return [volt, amp]
  #-------------------------------------------------------------------------

  def PwrSet(self, supply, volts, amps, itry): # float *v, float *c
    self.set_v(volts, supply)
    self.set_i(amps, supply)

    if (itry <= 0):
      return 0

    istat = 10
    for i in range(itry):
      if not(istat):
        break
      [vMeas, cMeas] = self.PwrMeas(supply)
      if (abs(amps) > 0.001 and abs(cMeas/amps) > 0.95):
        istat = 1  # compliance
      elif (abs(volts) > 0.001 and abs(vMeas/volts) < 0.95):
        istat = 2  # voltage err
      else:
        istat = 0

    return [istat, vMeas, cMeas]
  #-------------------------------------------------------------------------
  
  def PwrStab(self, supply):
    maxtry = 4
    volt0 = -999
    itry = 0

    while True:
      itry = itry+1
      [volt, curr] = PwrMeas(supply)
      delta = abs(volt-volt0)
      volt0 = volt
      if not((delta > 1.0E-3) and (itry < maxtry)):
        break
  
    if (delta<=1.0E-3):
      return 0
    else:
      return itry
  #-------------------------------------------------------------------------
  # **********************************/
  # For compatibility with older     */
  #  programs                        */
  # **********************************/
  def PWR_OFF(self):
    self.viDev.write("OUTP OFF")
  #-------------------------------------------------------------------------
  
  def PWR_ON(self, supply, volts, amps):
    self.set_v(volts, supply)
    self.set_i(amps, supply)
    self.viDev.write("OUTP ON")
  #-------------------------------------------------------------------------
  
  def PWR_Meas_Current(
    self, supply, low_lim, high_lim, supply_lev,
    pwr_lim, test_num):
    curr = self.get_i(supply)
    if (curr <= low_lim):
      rVal = 1
    elif (curr >= high_lim):
      rVal = 2
    else:
      rVal = 0
    pwr = curr*supply_lev
    if curr <= low_lim: rVal = 1
    elif curr >= high_lim: rVal = 2
    elif pwr > pwr_lim: rVal = 3
    else: rVal = 0
    return [rVal, curr, pwr]
 #-------------------------------------------------------------------------
  
  def I_Meas_Current(self, supply, low_lim, high_lim, test_num):
    curr = self.get_i(supply)
    if (curr <= low_lim):
      rVal = 1
    elif (curr >= high_lim):
      rVal = 2
    else:
      rVal = 0
    return [rVal, curr]
  #-------------------------------------------------------------------------
    
  # ****************************************************************
  # pwr_set (int supply_no, int supply, float volts, float amps)   *
  #          supply_no = 1  -> 3631A GPIB #5                       *
  #                    = 2  -> 3631A GPIB #6                       *
  #                    = 3  -> 3631A GPIB #7                       *
  #                                                                *
  #          supply    = 1  ->   6 V output                        *
  #                    = 2  -> +25 V output                        *
  #                    = 3  -> -25 V output                        *
  #                                                                *
  #          volts       target voltage for output                 *
  #          amps        current compliance                        *
  #*****************************************************************/
  
  def pwr_set(self, supply, volts, amps):
    self.set_v(volts, supply)
    self.set_i(amps, supply)
  #-------------------------------------------------------------------------
  
  def pwr_stabilize (self, supply):
    volt0 = -999
    c = 0
  
    while True:
      err = 0.
      c = c+1
      volt = -1.0 * self.get_v(supply)

      delta = ( volt - volt0 )
      err = delta * delta
      volt0 = volt
  
      #     print(" %13.9f "%volt)
      #     printf("     %.3e\n "err)
  
      if not((err > 1.0E-6) and (c < 5)):
        break

    return c
  #-------------------------------------------------------------------------

  def pwr_Meas_Current (self, supply):
    return self.get_i(supply)
  #-------------------------------------------------------------------------
  
  def pwr_Meas_Volt(self, supply):
    return self.get_v(supply)
  #-------------------------------------------------------------------------

  def pwr_meas_current(self, supply):
    return self.get_i(supply)
#-------------------------------------------------------------------

def test_keithley():

  print "Testing Inphilab module against Keithley!\n"
  
  dev = SrcMeter("Keithley")
  print "Keithley Sourcemeter connected!"
  dev.dev_print()
  
  print "Seting Keithley auto zero"
  dev.set_autozero(On=True)
  print "Auto zero set\n"
  
  print "Seting output auto-range"
  dev.set_output_autorange()
  print "Auto range set\n"
  
  print "Set output voltage limit to be 1.575V"
  dev.set_vlimit(1.575)
  print "Done\n"
  
  print "Set output current limit to be 1.5A"
  dev.set_ilimit(1.5)
  print "Done\n"
  
  print "Set output voltage to be 1.4V"
  dev.set_v(1.4)
  print "Done\n"
  
  print "Set output current to be 0.5A"
  dev.set_i(0.5)
  print "Done\n"
  
  print "Read current back...."
  iReading  = dev.get_i(0.5)
  print "Current reading: %e"%iReading
  print ""
  
  print "Read voltage back .... "
  vReading = dev.get_v()
  print "Voltage reading: %e"%vReading
  print ""
  
  print "Testing the Keithley output on/off"
  print "Turning output on ..... "
  dev.on_off(On=True)
  print "Turning output off .... "
  dev.on_off(On=False)
  print ""
  print "Turn auto zero off"
  dev.set_autozero(On=False)
  print "Auto zero turned off\n"
  dev.return_to_local()
  print "Control returned to user"
  print ""
  print "Test completed!"
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
