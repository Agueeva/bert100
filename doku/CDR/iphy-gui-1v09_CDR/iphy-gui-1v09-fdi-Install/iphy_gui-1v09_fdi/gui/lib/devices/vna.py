#
# $Header: /projects/wlv/iphy/sos/repository/iphy_A.rep/TEMP/case_1285613542/#alidation#iphy-gui#gui#lib#devices#vna.py,v 1.1 2011-09-02 11:43:52-07 case Exp $
# OldHeader: /inphi/inlab/pylab/devices/vna.py,v 1.2 2008/07/23 22:12:45 rbemra Exp 
# $Log: #alidation#iphy-gui#gui#lib#devices#vna.py,v $
# Revision 1.1  2011-09-02 11:43:52-07  case
# ...No comments entered during checkin...
#
# Revision 1.2  2011-08-31 19:38:50-07  rbemra
# Cannot recall ... maybe just a rearrange
#
# Revision 1.1  2011-05-04 18:10:34-07  rbemra
# Initial python+pyvisa for instrumentation (from ExacTik/882)
#
# Revision 1.2  2008/07/23 22:12:45  rbemra
# Added proper VecNwkAn class & measmt. functions.
# Moved S2P class, related code to pylab.utils.labutils
#
# Revision 1.1.1.1  2008/07/21 18:20:28  rbemra
# Initial Version
#
#
# Vector Network Analyzer remote interface
# Packaged under pylab 7/21/2008: RSB
#
# from var import *
# import sys
import time
# import math
# from types import *
from labdev import *
from meas_utils import *

#-----------------------------------------------------------------------------

VNA_8510 = 1
VNA_4395A = 2
VNA_8361A = 3
PNA_N5245A = 4
VnaDict = {
  '8510':VNA_8510,
  '4395':VNA_4395A,
  '8361':VNA_8361A,
  '5245':PNA_N5245A
}

class VecNwkAn(LabDev):
  def __init__(self, name):
    LabDev.__init__(self, name)
    LabDev.set_model(self, VnaDict, DEV_VNA)
  #-----------------------------------------------------------------------------

  def get_loss_db(self, lossList, param, nPts, SP_RI=None):
    # vna.write("HOLD;")
    SP_RI = self.viDev.ask_for_values("%s; OUTPDATA;"%param)
    nParam = len(SP_RI)
    if not(nParam/2):
      raise Exception("Odd %d values for complex %s!\n"%(nParam, param))
    if (nParam != (2*nPts)):
      raise Exception("Length(%s)=%d != Length(Freq)=%d!\n"%(param, nParam/2, nPts))
    for k in range(0, 2*nPts, 2):
      sRI = complex(SP_RI[k], SP_RI[k+1])
      loss = -20*math.log10(abs(sRI))
      lossList.append(loss)
    vna.write("CONT;")
  #-----------------------------------------------------------------------------

  def get_vna_delay(self, param):
    self.viDev.write("%s; DELA; WAIT;"%param)
    usleep(10e6, False) # wait 10s
    gdStrg = self.viDev.ask("OUTPFORM;") # gets " real, 0.\n real, 0.\n..."
    csvStrg = gdStrg.replace("\n", ",") # turn into csv
    gdsList = csvStrg.split(",") # list of ["real" "0" "real" "0"....]
    gDelay = map(float, gdsList[0::2]) # [real real ....]
    return gDelay
  #-----------------------------------------------------------------------------

  def get_cspar(self, param, nPts):
    # self.viDev.write("HOLD;")
    self.viDev.write("%s; LOGM; WAIT;"%param)
    usleep(10e6, False) # wait 10s
    SP_RI = self.viDev.ask_for_values("OUTPDATA;")
    nVal = len(SP_RI)
    if not(nVal/2):
      raise Exception("Odd %d values for complex %s!\n"%(nVal, param))
    if (nVal != (2*nPts)):
      raise Exception("Length(%s)=%d != Length(Freq)=%d!\n"%(param, nVal/2, nPts))
    self.viDev.write("CONT;")
    cSP = []
    for k in range(0, nVal, 2):
      cSP.append(complex(SP_RI[k], SP_RI[k+1]))
    return cSP
#-----------------------------------------------------------------------------

  def save_snp_file(self,
    basePath,
    portOrder, # "1,2,3,4" or permutation
    sFmt, # if None, AUTO, else one of MA|DB|RI|AUTO
    doCITI):
  
    nPorts = 1

    if (basePath==None or portOrder==None):
      return
    if 0: # use split() instead
      pc = portOrder
      sLen = pc.__len__()
      while True:
        kIndx = pc.find(',')
        if (kIndx<0):
          break
        nPorts += 1
        pc = portOrder[kIndx+1:sLen-1]
    else:
      nPorts = len(portOrder.split(','))
    if (sFmt==None):
      sFmt = "AUTO"
    sBigBuf = "MMEM:STOR:TRAC:FORM:SNP %s"%sFmt
    self.viDev.write(sBigBuf)
    if (nPorts <= 2):
      if (doCITI):
        sBigBuf = 'MMEM:STOR \"%s.cti\"'%basePath
      else:
        sBigBuf = 'MMEM:STOR \"%s.s%1dp\"'%(basePath, nPorts)
    else:
      if (doCITI):
        sBigBuf = 'MMEM:STOR:CIT:DATA \"%s.cti\"'%basePath
      else:
        sBigBuf = 'CALC:DATA:SNP:PORT:SAVE \"%s\", \"%s.s%1dp\"'%(portOrder, basePath, nPorts)

    self.viDev.write(sBigBuf)
#-----------------------------------------------------------------------------

def vna_test_run(s2pName=None):
  from datetime import datetime
  import sys
  CsvFyl = sys.stdout
  il = get_instruments_list()
  ix = get_index(il, "VNA_HP8510", False)
  if ix >= 0:
    vna = VecNwkAn(il[ix])
  else:
    raise Exception("VNA not found")

  # vna.dev_print()

  vna.viDev.write("DEBUON;")
  vna.viDev.write("FORM4;")  # ASCII output
  dzS2P = S2P()
  [NPts] = vna.viDev.ask_for_values("POIN;OUTPACTI;") # #of freq. pts
  NPts = int(NPts)
  dzS2P.fqList = vna.viDev.ask_for_values("OUTPFREL;")
  if len(dzS2P.fqList)!=NPts:
    raise Exception("len(FreqList)=%d != NPts=%d!\n"%(len(dzS2P.fqList), NPts))
  #
  # ok, get the results
  #
  dzS2P.S11 = vna.get_cspar("S11", NPts)
  S11_RL = loss_db(dzS2P.S11,"S11")

  dzS2P.S12 = vna.get_cspar("S12", NPts)

  dzS2P.S21 = vna.get_cspar("S21", NPts)
  S21_Gain = loss_db(dzS2P.S21, "S21", False)

  # gd_S21 = map(pico, dzS2P.group_delay()) # post-process: no span-smoothing
  gd_S21 = map(pico, vna.get_vna_delay("S21")) # use vna smoothing

  dzS2P.S22 = vna.get_cspar("S22", NPts)
  S22_RL = loss_db(dzS2P.S22, "S22")

  if s2pName != None:
    dzS2P.write_s2p(s2pName+".s2p", "vna_test_run")

  # write this freq set
  for k in range(0, NPts):
    vnaLine = "%.2f, %.2f, %.2f, %.2f, %.2f"%\
      (dzS2P.fqList[k], S11_RL[k], S22_RL[k], S21_Gain[k], gd_S21[k])
    CsvFyl.write(vnaLine+"\n")
    CsvFyl.flush()
  
  now = datetime.today()
  CsvFyl.write(now.strftime("Completed on %d %b %Y %H:%M:%S."))
  CsvFyl.write("\n")
  print "\nTests Complete!\n"
  CsvFyl.flush()
  # CsvFyl.close()
#-----------------------------------------------------------------------

def gdtest():
  # from pylab.utils.labutils import gd_one, pico
  from labutils import gd_one, pico
  S21 = [
    100.3242 + (-9.035155j),
    90.44529 + (-36.13281j),
    75.97264 + (-58.82812j),
    56.23046 + (-77.90624j),
    31.91796 + (-89.26561j),
    5.316405 + (-94.28123j),
    -22.65625 + (-92.3164j),
    -45.24218 + (-84.12889j),
    -66.58983 + (-65.41796j),
    -81.78514 + (-47.17577j),
    -90.23436 + (-22.25781j),
    -90.70702 + (3.253906j),
    -87.1953 + (29.53906j),
    -72.90624 + (50.85546j),
    -56.49218 + (72.43749j),
    -33.07812 + (81.5078j),
    -8.691405 + (92.14062j),
    13.37109 + (91.06249j),
    34.57031 + (84.43357j),
    55.58984 + (71.78905j),
    68.06249 + (55.85546j)
  ]
  fqList = []
  delFq = 200e6
  cFreq = 45e6
  fqList.append(cFreq)
  for k in range(len(S21)-1):
    cFreq = cFreq + delFq
    fqList.append(cFreq)

  gdList = gd_one(S21, fqList)
  gd_ps = map(pico, gdList) # 
  print "GD(ps):\n", gd_ps
#-------------------------------------------------------------------------------

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
