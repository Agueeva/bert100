#
# $Header: /projects/wlv/iphy/sos/repository/iphy_A.rep/TEMP/case_1285613542/#alidation#iphy-gui#gui#lib#devices#meter.py,v 1.1 2011-09-02 11:43:53-07 case Exp $
# Old $Header: /inphi/inlab/pylab/devices/meter.py,v 1.2 2008/07/23 02:07:53 rbemra Exp
# $Log: #alidation#iphy-gui#gui#lib#devices#meter.py,v $
# Revision 1.1  2011-09-02 11:43:53-07  case
# ...No comments entered during checkin...
#
# Revision 1.2  2011-08-10 17:59:13-07  rbemra
# Added get_freq() method
#
# Revision 1.1  2011-05-04 18:10:35-07  rbemra
# Initial python+pyvisa for instrumentation (from ExacTik/882)
#
# Revision 1.2  2008/07/23 02:07:53  rbemra
# Used small, unique id as infix to be part of device alias
# CVS header, log kwds
#
# Revision 1.1.1.1  2008/07/21 18:20:28  rbemra
# Initial Version
#
import time
from labdev import *
from meas_utils import *

METER_34970 = 0
METER_34401 = 1
MeterDict = {
  '34970':METER_34970,
  '34401':METER_34401
}
class Meter(LabDev):
  chDict = None
  swDict = None
  swTimeUs = 10000 # 20000=orig_vx.xx_C
  def __init__(self, name):
    LabDev.__init__(self, name)
    LabDev.set_model(self, MeterDict, DEV_METER)
    # self = iDev
    # self.viDev = iDev.viDev
    # self.name = iDev.name
  def add_ch(self, chNum, chanName):
    if self.chDict == None:
      self.chDict = {} # initialize
    self.chDict[chNum] = chanName
  def get_v(self, chNum):
    if self.modelType == METER_34970:
      rsL = self.viDev.ask("MEAS:VOLT:DC? DEF,DEF,(@%s)"%self.chDict[chNum])
    elif self.modelType == METER_34401:
      raise Exception("TODO: "+self.IDN())
    return float(rsL)
  def get_i(self, chNum):
    if self.modelType == METER_34970:
      raise Exception("TODO: "+self.IDN())
    elif self.modelType == METER_34401:
      raise Exception("TODO: "+self.IDN())
  def get_temp(self, chNum):
    if self.modelType == METER_34970:
      # rv = self.viDev.ask_for_values("MEAS:TEMP? TC,K,1,DEF,(@%s)"%self.chDict[chNum])
      rsL = self.viDev.ask("MEAS:TEMP? TC,K,1,DEF,(@%s)"%self.chDict[chNum])
    elif self.modelType == METER_34401:
      raise Exception("TODO: "+self.IDN())
    # return rv[0]
    return float(rsL)
  
  def get_freq(self, chNum):
    if self.modelType == METER_34970:
      rsL = self.viDev.ask("MEAS:FREQ? DEF,DEF,(@%s)"%self.chDict[chNum])
    elif self.modelType == METER_34401:
      raise Exception("TODO: "+self.IDN())
    # return rv[0]
    return float(rsL)
  
  def get_res(self, chNum):
    if self.modelType == METER_34970:
      rsL = self.viDev.ask("MEAS:RES? DEF,DEF,(@%s)"%self.chDict[chNum])
    elif self.modelType == METER_34401:
      raise Exception("TODO: "+self.IDN())
    # return rv[0]
    return float(rsL)
  
  def add_sw(self, swNum, swName):
    if self.swDict == None:
      self.swDict = {} # initialize
    self.swDict[swNum] = swName
  def sw_open(self, swNum):
    self.viDev.write("ROUT:OPEN (@%s)"%(self.swDict[swNum]))
    usleep(self.swTimeUs)
  def sw_close(self, swNum):
    self.viDev.write("ROUT:CLOS (@%s)"%(self.swDict[swNum]))
    usleep(self.swTimeUs)
  def sw_doit(self, swNum, doClose=True):
    if doClose:
      self.viDev.write("ROUT:CLOS (@%s)"%(self.swDict[swNum]))
    else:
      self.viDev.write("ROUT:OPEN (@%s)"%(self.swDict[swNum]))
    usleep(self.swTimeUs)
  def sw_is_open(self, swNum):
    isOpen = self.viDev.ask("ROUT:OPEN? (@%s)"%(self.swDict[swNum]))
    return isOpen=='1'
  def sw_toggle(self, swNum):
    if self.sw_is_open(swNum):
      self.sw_close(swNum)
    else:
      self.sw_open(swNum)
#------------------------------------------------------------------------
#
# Test:
# Assume SPDT @ 305 between VDD & GND, monitored @ 101 and 102
#
def run():
  # from visa import *
  # from labutils import *
  swMtrName = "DAQ_34970A_53"

  iList = get_instruments_list()
  ix = get_index(iList, swMtrName)
  if ix < 0:
    raise Exception("%s: not found\n"%swMtrName)
  swMeter = Meter(iList[ix])
  swMeter.add_sw(1, "305")
  swMeter.add_ch(1, "101")
  swMeter.add_ch(2, "102")
  swMeter.sw_open(1)
  if not(swMeter.sw_is_open(1)):
    raise Exception("could not open switch")
  v101 = swMeter.get_v(1)
  v102 = swMeter.get_v(2)
  print("OPEN: v101=%.3f, v102=%.3f\n"%(v101, v102))
  swMeter.sw_close(1)
  if swMeter.sw_is_open(1):
    raise Exception("could not close switch")
  v101 = swMeter.get_v(1)
  v102 = swMeter.get_v(2)
  print("CLOSED: v101=%.3f, v102=%.3f\n"%(v101, v102))
  swMeter.sw_toggle(1)
  v101 = swMeter.get_v(1)
  v102 = swMeter.get_v(2)
  print("TOGGLE: v101=%.3f, v102=%.3f\n"%(v101, v102))
# 
#  Local Variables:
#  tab-width: 2
#  indent-tabs-mode: nil
#  scroll-step: 1
#  line-number-mode: t
#  column-number-mode: t
#  End:
# 
