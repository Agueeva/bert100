#
# $Header: /projects/wlv/iphy/sos/repository/iphy_A.rep/TEMP/case_1285613542/#alidation#iphy-gui#gui#lib#devices#labdev.py,v 1.1 2011-09-02 11:43:54-07 case Exp $
# Old Header: /inphi/inlab/pylab/devices/labdev.py,v 1.2 2008/07/21 19:34:51 rbemra
# $Log: #alidation#iphy-gui#gui#lib#devices#labdev.py,v $
# Revision 1.1  2011-09-02 11:43:54-07  case
# ...No comments entered during checkin...
#
# Revision 1.2  2011-08-31 19:31:38-07  rbemra
# Added BERT type & untested opc_wait()
#
# Revision 1.1  2011-05-04 18:10:36-07  rbemra
# Initial python+pyvisa for instrumentation (from ExacTik/882)
#
# Revision 1.2  2008/07/21 19:34:51  rbemra
# Removed CR chars added during cvs import
#
# Revision 1.1.1.1  2008/07/21 18:20:28  rbemra
# Initial Version
#
#
# Packaged under inlab 7/21/2008: RSB
# Inphi Lab Instrument GPIB/LAN Interface
#
# Revision 1.7  2008/04/12 01:11:01  rbemra
# Added temp acq. for DAQ; added RAM pattern load for BertScope
#
# Revision 1.6  2008/02/02 01:06:48  zhua
# no message
#
# Revision 1.4  2008/02/01 20:08:28  zhua
# Added controls for Keithley
#
# Revision 1.3  2008/02/01 18:46:54  rbemra
# Added channel dict/name for SrcMeter & Meter: untested!
#
# Revision 1.2  2008/01/24 21:46:28  rbemra
# Added some srcMeter, sigGen methods
#
# Revision 1.1  2008/01/21 00:32:54  rbemra
# Instrument Interface: first cut
#
#
from visa import *
#---------------------------------------------------------------------------------

DeviceTypes = {
  0:"SCOPE", 1:"SIG_GEN", 2:"SOURCE", 3:"METER", 4:"PTFS", 5:"VNA"
}
DEV_SCOPE = 0
DEV_GEN = 1
DEV_SRC = 2
DEV_METER = 3
DEV_PTFS = 4
DEV_VNA = 5
DEV_BERT = 6

class LabDev:
  name = None
  devType = None
  devTypeName = None
  modelType = None
  modelName = None
  modelDict = None
  viDev = None # visa device, GpibInstrument
  def __init__(self, name):
    self.viDev = instrument(name)
    self.name = name
  def set_model(self, modelDict, devType):
    # id = self.IDN() # take the first model that's part of device IDN
    id = self.name
    id = id.upper() # long(er) user name
    for mdlName in modelDict.keys():
      upName = mdlName.upper() # short, long names
      if id.find(upName) >= 0: # user/alias name must contain one of predefined names
        self.modelType = modelDict[mdlName] 
        self.modelName = mdlName
        self.modelDict = modelDict
        print("Yes, modelType=%d, modelName=%s"%(self.modelType, self.modelName))
        self.devType = devType
        self.devTypeName = DeviceTypes[devType]
  def IDN(self):
    return self.viDev.ask("*IDN?")
  def dev_print(self):
    if self.viDev == None:
      return
    print("devTypeName=%s, devType=%d"%(self.devTypeName, self.devType))
    print("modelName=%s, modelType=%d"%(self.modelName, self.modelType))
    print("IDN=%s"%self.IDN())
  def opc_wait(self, toutSec): # 0:forever, else timeout in seconds
    waitTimeUs = 100000 # wait 0.1 sec between query
    tDoneUs = 0L
    toutUs = toutSec * 1000000L
    done = False

    while (not done):
      usleep(waitTimeUs)
      OPC = int(self.viDev.ask("*OPC?"))
      done = OPC==1
      if (done):
        break
      if (not toutSec):
        continue # forever
      tDoneUs += waitTimeUs
      if (tDoneUs > toutUs):
        break
    return done
#-----------------------------------------------------------------------------------------
def run():
  from meas_utils import *
  devList = []
  nameList = get_instruments_list()

  ix = get_index(nameList, "E3631A_VddVS")
  if ix >= 0:
    dev = SrcMeter(nameList[ix])
    if dev.modelType != None:
      print("Ok, found E3631A")
      devList.append(dev)

  ix = get_index(nameList, "Instek")
  if ix >= 0:
    dev = SrcMeter(nameList[ix])
    if dev.modelType != None:
      print("Ok, found Instek")
      devList.append(dev)

  ix = get_index(nameList, "Keithley")
  if ix >= 0:
    dev = SrcMeter(nameList[ix])
    if dev.modelType != None:
      print("Found Keithley")
      devList.append(dev)
    dev.return_to_local()

  ix = get_index(nameList, "34970A_VddDVM")
  if ix >= 0:
    dev = Meter(nameList[ix])
    if dev.modelType != None:
      print("Ok, found 34970A")
      devList.append(dev)

  for dev in devList:
    dev.dev_print() 
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
