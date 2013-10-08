#
# $Header: /projects/wlv/iphy/sos/repository/iphy_A.rep/TEMP/case_1285613542/#alidation#iphy-gui#gui#lib#devices#sgen.py,v 1.1 2011-09-02 11:43:52-07 case Exp $
# Old Header: /inphi/inlab/pylab/devices/sgen.py,v 1.2 2008/07/21 19:34:51 rbemra Exp
# $Log: #alidation#iphy-gui#gui#lib#devices#sgen.py,v $
# Revision 1.1  2011-09-02 11:43:52-07  case
# ...No comments entered during checkin...
#
# Revision 1.2  2011-08-31 19:24:18-07  rbemra
# Added partial BERT device & methods
#
# Revision 1.1  2011-05-04 18:10:35-07  rbemra
# Initial python+pyvisa for instrumentation (from ExacTik/882)
#
# Revision 1.2  2008/07/21 19:34:51  rbemra
# Removed CR chars added during cvs import
#
#
# Signal Generator remote interface module:
# Usage: from inlab.devices import sgen
#
from labdev import *

GEN_BERT = 0
GEN_83650 = 1
GEN_83630 = 2
GEN_SQA_BERT = 3
SigGenDict = {
  'BertScope':GEN_BERT,
  '12500':GEN_BERT,
  '83650':GEN_83650,
  '83630L':GEN_83630,
  '1800
}
class SigGen(LabDev):
  def __init__(self, name):
    LabDev.__init__(self, name)
    LabDev.set_model(self, SigGenDict, DEV_GEN)
    # self = iDev
    # self.viDev = iDev.viDev
    # self.name = iDev.name
  def on_off(self, On=False):
    if On:
      swVal = "ON"
    else:
      swVal = "OFF"

  def set_freq(self, freqHz):
    if self.modelType == GEN_83650:
      self.viDev.write(":FREQ:CW %.3f"%freqHz)
      usleep(50000)
      self.viDev.write(":POW:STAT ON")
    elif self.modelType == GEN_BERT:
      self.viDev.write("SOUR:FREQ:MODE CW")
      self.viDev.write("GEN:ICLOCK %f"%freqHz)
  def set_power(self, pwr_dBm):
    if self.modelType == GEN_83650:
      self.viDev.write(":POW:LEV %.2f DBM"%pwr_dBm)
      usleep(250000)
      self.viDev.write(":POW:STAT ON")
    elif self.modelType == GEN_BERT:
      self.viDev.write("SOUR:POW:LEVEL %f"%pwr_dBm)

  def set_vpp(self, vpp):
    if self.modelType == GEN_83650:
      self.viDev.write("TODO")
    elif self.modelType == GEN_BERT:
      self.viDev.write("TODO")

  def get_freq(self):
    if self.modelType == GEN_BERT:
      freqHz = float(self.viDev.ask("GEN:ICLOCK?"))
    return freqHz

  def load_ram_pat(self, ramPatFile):
    if self.modelType == GEN_BERT:
      self.viDev.write("GEN:UPLoad \"%s\""%ramPatFile)

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
