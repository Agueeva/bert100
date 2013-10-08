#
# $Header: /projects/wlv/iphy/sos/repository/iphy_A.rep/TEMP/case_1285613542/#alidation#iphy-gui#gui#lib#devices#rto.py,v 1.1 2011-09-02 11:43:53-07 case Exp $
# Old Header: /inphi/inlab/pylab/devices/rto.py,v 1.3 2008/07/23 02:09:06 rbemra Exp
# $Log: #alidation#iphy-gui#gui#lib#devices#rto.py,v $
# Revision 1.1  2011-09-02 11:43:53-07  case
# ...No comments entered during checkin...
#
# Revision 1.1  2011-05-04 18:10:35-07  rbemra
# Initial python+pyvisa for instrumentation (from ExacTik/882)
#
# Revision 1.3  2008/07/23 02:09:06  rbemra
# inlab -> inlab/pylab, just pylab w.r.t. python pkg
#
# Revision 1.2  2008/07/21 19:34:51  rbemra
# Removed CR chars added during cvs import
#
# Revision 1.1.1.1  2008/07/21 18:20:28  rbemra
# Initial Version
#
#
# Real-time Oscilloscope remote interface
# Packaged under inlab 7/21/2008: RSB
# First Rev.: Sept., 2007: RSB
#
# 
import time
from labdev import *
#------------------------------------------------------------------------------
class RTO:
  # common commands
  CMD_CLS = 0
  CMD_AUTO = 1
  CMD_RST = 2
  CMD_TRG = 3
  CMD_RCL = 4
  CMD_STOP = 5
  CMD_RUN = 6
  CMD_SAV = 7
  CMD_CDIS = 8
  def cmd_name(self, val):
    if val==0:
      name = "*CLS"
    elif val==1:
      name = ":AUT"
    elif val==2:
      name = "*RST"
    elif val==3:
      name = "*TRG"
    elif val==4:
      name = "*RCL"
    elif val==5:
      name = ":STOP"
    elif val==6:
      name = ":RUN"
    elif val==7:
      name = "*SAV"
    elif val==8:
      name = ":CDIS"
    else:
      name = ""
    return name

  # time reference
  TIME_REF_LEFT = 0
  TIME_REF_CENT = 1
  TIME_REF_RIGHT = 2
  time_ref_L = ["LEFT", "CENTER", "RIGHT"]
  def time_ref_name(self, val):
    return self.time_ref_L[val]
  def time_ref(self, name):
    return get_index(self.time_ref_L, name, False)

  # threshold type
  THR_STAN = 0
  THR_VOLT = 1
  THR_PERC = 2
  def thr_name(self, val):
    if val==0:
      name = "STAN"
    elif val==1:
      name = "VOLT"
    elif val==2:
      name = "PERC"
    else:
      name = ""
    return name

  # TopBase type
  TB_STAN = 0
  TB_MINMAX = 1
  TB_HIST = 2
  TB_CUSTOM = 3

  def tb_name(self, val):
    if val==0:
      name = "STAN"
    elif val==1:
      name = "MINMAX"
    elif val==2:
      name = "HISTONLY"
    else: # CUSTOM, or unknown
      name = ""
    return name


  # edge direction
  DIR_RISE = 0
  DIR_FALL = 1
  DIR_EITHER = 2
  def dir_name(self, val):
    if val==0:
      name = "RIS"
    elif val==1:
      name = "FALL"
    elif val==2:
      name = "EITH"
    else:
      name = ""
    return name

  # trigger slope
  SLP_POS = 0
  SLP_NEG = 1
  DIR_EITH = 2
  def slp_name(self, val):
    if val==0:
      name = "POS"
    elif val==1:
      name = "NEG"
    else:
      name = ""
    return name

  # position on edge
  POS_UPP = 0
  POS_MID = 1
  POS_LOW = 2
  def pos_name(self, val):
    if val==0:
      name = "UPP"
    elif val==1:
      name = "MIDD"
    elif val==2:
      name = "LOW"
    else:
      name = ""
    return name

  # measurement source
  SRC_CHAN = 0
  SRC_FUNC = 1
  SRC_WMEM = 2
  def src_name(self, val):
    if val==self.SRC_CHAN:
      name = "CHAN"
    elif val==self.SRC_FUNC:
      name = "FUNC"
    elif val==self.SRC_WMEM:
      name = "WMEM"
    else:
      name = ""
    return name

  # Math operators
  OP_INV = 0
  OP_AVER = 1
  OP_ADD = 2
  OP_SUBT = 3
  OP_COMM = 4
  OP_DIFF = 5
  OP_DIV = 6
  OP_FFTM = 7
  OP_FFTP = 8
  OP_MULT = 9
  op_L = ["INVERT", "AVERAGE", "ADD", "SUBTRACT", "COMMONMODE",
          "DIFF", "DIVIDE", "FFTM", "FFTP", "MULTIPLY"]
  def op_name(self, val):
    return self.op_L[val]
  def oper(self, name):
    return get_index(self.op_L, name, False)

  # File types
  FTYP_WFM = 0
  FTYP_CSV = 1
  FTYP_TSV = 2
  FTYP_TXT = 3
  FTYP_SET = 4
  def ftyp_name(self, val):
    if val==self.FTYP_WFM:
      name = "wfm"
    elif val==self.FTYP_CSV:
      name = "csv"
    elif val==self.FTYP_TSV:
      name = "tsv"
    elif val==self.FTYP_TXT:
      name = "txt"
    elif val==self.FTYP_SET:
      name = "set"
    else:
      name = ""
    return name

  # Trigger modes
  TRIG_EDGE = 0
  TRIG_GLITCH = 1
  TRIG_ADV_COMM = 2
  TRIG_ADV_DELAY = 3
  TRIG_ADV_PATT  = 4
  TRIG_ADV_STATE = 5
  TRIG_ADV_VIOL = 6
  trig_L = ["EDGE", "GLITCH", "COMM", "DELAY", "PATT",
          "STATE", "VIOL"]
  def trig_name(self, val):
    return self.trig_L[val]
  def trig(self, name):
    return get_index(self.trig_L, name, False)

  # Trigger sweep modes
  TRIGSWP_AUTO = 0
  TRIGSWP_TRIG = 1
  TRIGSWP_1SHOT = 2
  trigswp_L = ["AUTO", "TRIG", "SINGLE"]
  def trigswp_name(self, val):
    return self.trigswp_L[val]
  def trigswp(self, name):
    return get_index(self.trigswp_L, name, False)

  # constants
  V_OVFLOW = 100.0 # it's +/-5V for Agilent, clipping gives 9.9999e+37
  T_OVFLOW = 1.0 # some big time value


#---------------------------------------------------------------------------------
RTO1 = RTO() # global single instance
#
#     COMMON/ROOT COMMANDS
#---------------------------------------------------------------------------------

def send_cmd(scope, cmd, intArg=None, cmdStrg=None):
  if cmdStrg!=None:
    if intArg!=None:
      scope.viDev.write("%s %d"%(cmdStrg, intArg))
    else:
      scope.viDev.write(cmdStrg)
  else:
    cName = RTO1.cmd_name(cmd)
    if intArg!=None:
      scope.viDev.write("%s %d"%(cName, intArg))
    else:
      scope.viDev.write(cName)
#---------------------------------------------------------------------------------
#
#     CHANNEL/FUNCTION COMMANDS
#---------------------------------------------------------------------------------

def set_display(scope, chnl, OnOff=0, src=RTO.SRC_CHAN):
  srcName = RTO1.src_name(src)
  scope.viDev.write(":%s%d:DISP %d"%(srcName, chnl, OnOff))
#---------------------------------------------------------------------------------

def define_func(scope, num, operator, oper1, oper2, src1=RTO.SRC_CHAN, src2=RTO.SRC_CHAN):
  srcName = RTO1.src_name(src1)
  sCmd = ":FUNC%d:%s %s%d"%(num, RTO1.op_name(operator), srcName, oper1)
  if (operator==RTO.OP_INV or operator==RTO.OP_AVER or
      operator==RTO.OP_FFTM or operator==RTO.OP_FFTP):
    pass
  else:
    srcName = RTO1.src_name(src2)
    sCmd += ",%s%d"%(srcName, oper2)
  scope.viDev.write(sCmd)
#---------------------------------------------------------------------------------

def set_vrange(scope, chnl, vRng=None, vOffset=None, src=RTO.SRC_CHAN):
  if (src==RTO.SRC_CHAN):
    if vRng!=None:
      scope.viDev.write(":CHAN%d:RANG %e"%(chnl, vRng))
    if (vOffset != None):
      scope.viDev.write("CHAN%d:OFFSET %e"%(chnl, vOffset))
  elif (src==RTO.SRC_FUNC):
    if vRng!=None:
      scope.viDev.write(":FUNC%d:VERT:RANG %e"%(chnl, vRng))
    if (vOffset != None):
      scope.viDev.write("FUNC%d:VERT:OFFSET %e"%(chnl, vOffset))
#---------------------------------------------------------------------------------

def get_vrange(scope, chnl, src=RTO.SRC_CHAN):
  rList = []
  if (src==RTO.SRC_CHAN):
    vRng = float(scope.viDev.ask(":CHAN%d:RANG?"%chnl))
    vOff = float(scope.viDev.ask(":CHAN%d:OFFS?"%chnl))
  elif (src==RTO.SRC_FUNC):
    vRng = float(scope.viDev.ask(":FUNC%d:VERT:RANG?"%chnl))
    vOff = float(scope.viDev.ask(":FUNC%d:VERT:OFFS?"%chnl))
  else:
    return rList
  rList.append(vRng)
  rList.append(vOff)
  return rList
#---------------------------------------------------------------------------------
#
# Adjust to whole mV range, so that /div scale is whole mV
#
def set_vrange_min_max(scope, chnl, vMin, vMax, src=RTO.SRC_CHAN):
  mvRng = int(1000*(vMax-vMin))
  mvRng = mvRng + mvRng%8 # expand to multiple of 8 (divisions)
  set_vrange(scope, chnl, 0.001*mvRng, 0.5*(vMin+vMax), src)
#---------------------------------------------------------------------------------

def get_vrange_min_max(scope, chnl, src=RTO.SRC_CHAN):
  rList = []
  rngOff = get_vrange(scope, chnl, src)
  halfRng = 0.5*rngOff[0]
  osVal = rngOff[1]
  rList.append(osVal-halfRng)
  rList.append(osVal+halfRng)
  return rList
#---------------------------------------------------------------------------------
# Vertical Auto-scale
# Get [range, offset]
# get vMax, if MAXV, keep inc.ing offset by volts/div until vMax!=MAXV
# get vMin, if MAXV, keep dec.ing offset by volts/div until vMax!=MAXV
# Then use set_vrange_min_max()
#
OFFSET_MAX = 5.0 # max. allowed +/-offset we will expect
MAX_OFF_ITER = 20 #
SCALE_FACTOR = 2.0
MAX_SCALE_ITER = 5 # 1/2^5 = 1/32

def auto_vrange(scope, chnl, src=RTO.SRC_CHAN):
  set_display(scope, chnl, 1, src)
  [vRange, vOff] = get_vrange(scope, chnl, src)

  vOff1 = vOff
  offDelta = vRange/8.0
  nIter = 0
  stat = False
  restore = True
  while True:
    [vMax] = meas_single(scope, "VMAX", chnl, stat=False, nMeas=1, src=src)
    if vMax < RTO.V_OVFLOW:
      stat = True
      break # done
    if nIter > MAX_OFF_ITER:
      break
    vOff1 += offDelta
    if vOff1 > OFFSET_MAX: # maybe we're pushing offset in opp. direction
      # print "Ok, I'm pushing it down; let me try opposite direction ..."
      set_vrange(scope, chnl, vRange, vOff, src)
      nIter = 0 # start again
      offDelta = -offDelta
      continue
    elif vOff1 < -OFFSET_MAX:
      break
    set_vrange(scope, chnl, vRange, vOff1, src)
    nIter += 1

  if not stat and nIter > MAX_OFF_ITER: 
    nIter = 0
    vRange1 = vRange
    while True:
      vRange1 = SCALE_FACTOR*vRange1
      set_vrange(scope, chnl, vRange1, src)
      [vMax] = meas_single(scope, "VMAX", chnl, stat=False, nMeas=1, src=src)
      if vMax < RTO.V_OVFLOW:
        stat = True
        restore = False
        break # done
      if nIter > MAX_SCALE_ITER:
        break
      nIter += 1

  if not stat:
    print "* * Error * *: could not find VMAX"
    return stat

  if restore:
    set_vrange(scope, chnl, vRange, vOff, src) # restore

  # Repeat for vMin
  vOff1 = vOff
  offDelta = vRange/8.0
  nIter = 0
  stat = False
  while True:
    [vMin] = meas_single(scope, "VMIN", chnl, stat=False, nMeas=1, src=src)
    if vMin < RTO.V_OVFLOW:
      stat = True
      break # done
    if nIter > MAX_OFF_ITER:
      break
    vOff1 -= offDelta
    if vOff1 < -OFFSET_MAX: # maybe we're pushing offset in opp. direction
      # print "Ok, I'm pushing it up; let me try opposite direction ..."
      set_vrange(scope, chnl, vRange, vOff, src)
      offDelta = -offDelta
      continue
    elif vOff1 > OFFSET_MAX:
      stat = False
      break
    set_vrange(scope, chnl, vRange, vOff1, src)
    nIter += 1

  if not stat and nIter > MAX_OFF_ITER: 
    nIter = 0
    sFactor = 0.5
    vRange1 = vRange
    while True:
      vRange1 = SCALE_FACTOR*vRange1
      set_vrange(scope, chnl, vRange1, src)
      [vMin] = meas_single(scope, "VMIN", chnl, stat=False, nMeas=1, src=src)
      if vMin < RTO.V_OVFLOW:
        stat = True
        break # done
      if nIter > MAX_SCALE_ITER:
        break
      nIter += 1

  if not stat:
    print "* * Error * *: could not find VMIN"
    return stat

  vDel = (vMax-vMin)*0.05 # add 10% to range
  set_vrange_min_max(scope, chnl, vMin-vDel, vMax+vDel, src) # this v-autoscales chnl
  return True
#---------------------------------------------------------------------------------
#
#     ACQUISITION/DISPLAY COMMANDS
#---------------------------------------------------------------------------------

def set_aver(scope, OnOff=0, Count=1):
  scope.viDev.write(":ACQ:AVER %d"%OnOff)
  if OnOff:
    scope.viDev.write("ACQ:COUN %d"%Count)
#---------------------------------------------------------------------------------

def set_srate(scope, doAuto=True, sRate=None):
  if sRate != None:
    scope.viDev.write(":ACQ:SRATE %e"%sRate)
  elif not doAuto:
    scope.viDev.write(":ACQ:SRATE MAX")
  else:
    scope.viDev.write(":ACQ:SRATE:AUTO 1")
#---------------------------------------------------------------------------------

def set_points(scope, doAuto=True, nPts=None):
  if nPts != None:
    scope.viDev.write(":ACQ:POIN %d"%nPts)
  else:
    if doAuto: auto = 1
    else: auto = 0
    scope.viDev.write(":ACQ:POIN:AUTO %d"%auto)
#---------------------------------------------------------------------------------
#
#     TIMEBASE COMMANDS
#---------------------------------------------------------------------------------

def set_timebase_range(scope, tRng, refPos=RTO.TIME_REF_CENT):
  scope.viDev.write(":TIM:RANG %e"%tRng)
  sRef = RTO1.time_ref_name(refPos)
  scope.viDev.write(":TIM:REF %s"%sRef)
#---------------------------------------------------------------------------------

def set_timebase_pos(scope, tPos):
  scope.viDev.write(":TIM:POS %e"%tPos)
#---------------------------------------------------------------------------------

def get_trange(scope):
  rList = []
  rList.append(float(scope.viDev.ask(":TIM:RANG?")))
  rList.append(RTO1.time_ref(scope.viDev.ask(":TIM:REF?")))
  return rList
#---------------------------------------------------------------------------------
#
def meas_period(scope, chnl, stat=False, nMeas=1,
  src=RTO.SRC_CHAN, dir=RTO.DIR_RISE, doAppend=False):
  if (stat and (not doAppend)): # statistics on, use MEAS:RES? to get mean, min, max, rms
    scope.viDev.write(":MEAS:CLE") # clear out measmts
  nList = [] # numeric returned list
  srcName = RTO1.src_name(src)
  dirName = RTO1.dir_name(dir)
  set_display(scope, chnl, 1, src)
  scope.viDev.write(":MEAS:SOUR %s%d"%(srcName, chnl))
  if (stat): # statistics on, use MEAS:RES? to get mean, min, max, rms
    scope.viDev.write(":MEAS:STAT ON")
    scope.viDev.write(":MEAS:PER %s%d, %s"%(srcName, chnl, dirName))
    time.sleep(1.0) # pause a bit
    # wait till >= nMeas acquisitions complete
    mCount = 0
    while (mCount < nMeas):
      time.sleep(1.0) # pause a bit
      vResult = scope.viDev.ask_for_values(":MEAS:RES?") # 1.0,Curr,Min,Max,Mean,sDev,nMeas
      # sResult = scope.viDev.ask(":MEAS:RES?") # Name,Curr,Min,Max,Mean,sDev,nMeas
      # sList = sResult.split(',')
      if vResult[1] >= RTO.T_OVFLOW: # make all measVals=OVFLOW, nMeas=0
        for k in range(1, 6):
          vResult[k] = RTO.T_OVFLOW
        vResult[6] = 0.
        break
      mCount = vResult[6]
    for k in range(1, 7):
      nList.append(vResult[k])
  else: # current query
    nList.append(float(scope.viDev.ask(":MEAS:PER? %s%d, %s"%(srcName, chnl, dirName))))
  return nList # Numeric list: [Curr, Min, Max, Mean, sDev, nMeas]

#---------------------------------------------------------------------------------
#
#     MEASURE COMMANDS
#---------------------------------------------------------------------------------

def meas_vrange_min_max(scope, chnl, src=RTO.SRC_CHAN):
  rList = []
  srcName = RTO1.src_name(src)
  rVal = float(scope.viDev.ask(":MEAS:VMIN? %s%d"%(srcName, chnl)))
  rList.append(rVal)
  rVal = float(scope.viDev.ask(":MEAS:VMAX? %s%d"%(srcName, chnl)))
  rList.append(rVal)
  return rList
#---------------------------------------------------------------------------------
# Use this with measCmd =
# "VMAX", "VMIN", "VTOP", "VBAS", "VLOW", "VUPP", "VPP", "VRMS", "VAMP"
# returns numeric list: [Curr] or [Curr, Min, Max, Mean, sDev, nMeas]
#
def meas_single(scope, measCmd, chnl, stat=False, nMeas=1,
  src=RTO.SRC_CHAN, doAppend=False):
  if (stat and (not doAppend)): # statistics on, use MEAS:RES? to get mean, min, max, rms
    scope.viDev.write(":MEAS:CLE") # clear out measmts
  nList = [] # numeric returned list
  srcName = RTO1.src_name(src)
  scope.viDev.write(":MEAS:SOUR %s%d"%(srcName, chnl))
  if (stat): # statistics on, use MEAS:RES? to get mean, min, max, rms
    scope.viDev.write(":MEAS:STAT ON")
    scope.viDev.write(":MEAS:%s %s%d"%(measCmd, srcName, chnl))
    # wait till >= nMeas acquisitions complete
    mCount = 0
    while (mCount < nMeas):
      time.sleep(1.0) # pause a bit
      sResult = scope.viDev.ask(":MEAS:RES?")
      sList = sResult.split(',') # Name,Curr,Min,Max,Mean,sDev,nMeas
      mCount = float(sList[6])
    for k in range(1, 7):
      nList.append(float(sList[k]))
  else: # current query
    nList.append(float(scope.viDev.ask(":MEAS:%s? %s%d"%(measCmd, srcName, chnl))))
  return nList # Numeric list: [Curr, Min, Max, Mean, sDev, nMeas]
#---------------------------------------------------------------------------------
# returns numeric list: [Curr] or [Curr, Min, Max, Mean, sDev, nMeas]
#
def meas_vavg(scope, chnl, stat=False, nMeas=1,
  src=RTO.SRC_CHAN, doAppend=False):
  if (stat and (not doAppend)): # statistics on, use MEAS:RES? to get mean, min, max, rms
    scope.viDev.write(":MEAS:CLE") # clear out measmts
  nList = [] # numeric returned list
  srcName = RTO1.src_name(src)
  scope.viDev.write(":MEAS:SOUR %s%d"%(srcName, chnl))
  if (stat): # statistics on, use MEAS:RES? to get mean, min, max, rms
    scope.viDev.write(":MEAS:STAT ON")
    scope.viDev.write(":MEAS:VAV DISP, %s%d"%(srcName, chnl))
    # wait till >= nMeas acquisitions complete
    mCount = 0
    while (mCount < nMeas):
      time.sleep(1.0) # pause a bit
      sResult = scope.viDev.ask(":MEAS:RES?")
      sList = sResult.split(',') # Name,Curr,Min,Max,Mean,sDev,nMeas
      mCount = float(sList[6])
    for k in range(1, 7):
      nList.append(float(sList[k]))
  else: # current query
    nList.append(float(scope.viDev.ask(":MEAS:VAV? DISP, %s%d"%(srcName, chnl))))
  return nList # Numeric list: [Curr, Min, Max, Mean, sDev, nMeas]
#---------------------------------------------------------------------------------
#
def meas_vamp(scope, chnl, stat=False, nMeas=1, src=RTO.SRC_CHAN, doAppend=False):
  return meas_single(scope, "VAMP", chnl, stat, nMeas, src)
#---------------------------------------------------------------------------------
#
def meas_vtop(scope, chnl, stat=False, nMeas=1, src=RTO.SRC_CHAN, doAppend=False):
  return meas_single(scope, "VTOP", chnl, stat, nMeas, src)
#---------------------------------------------------------------------------------
#
def meas_vbase(scope, chnl, stat=False, nMeas=1, src=RTO.SRC_CHAN, doAppend=False):
  return meas_single(scope, "VBAS", chnl, stat, nMeas, src)
#---------------------------------------------------------------------------------
# Measure voltage in chnl at time tVal
# returns numeric list: [Curr] or [Curr, Min, Max, Mean, sDev, nMeas]
#
def meas_vtime(scope, tVal, chnl, stat=False, nMeas=1,
  src=RTO.SRC_CHAN, doAppend=False):
  if (stat and (not doAppend)): # statistics on, use MEAS:RES? to get mean, min, max, rms
    scope.viDev.write(":MEAS:CLE") # clear out measmts
  nList = [] # numeric returned list
  srcName = RTO1.src_name(src)
  scope.viDev.write(":MEAS:SOUR %s%d"%(srcName, chnl))
  if (stat): # statistics on, use MEAS:RES? to get mean, min, max, rms
    scope.viDev.write(":MEAS:STAT ON")
    scope.viDev.write(":MEAS:VTIM %e, %s%d"%(tVal, srcName, chnl))
    # wait till >= nMeas acquisitions complete
    mCount = 0
    while (mCount < nMeas):
      time.sleep(1.0) # pause a bit
      sResult = scope.viDev.ask(":MEAS:RES?")
      sList = sResult.split(',') # Name,Curr,Min,Max,Mean,sDev,nMeas
      mCount = float(sList[6])
    for k in range(1, 7):
      nList.append(float(sList[k]))
  else: # current query
    nList.append(float(scope.viDev.ask(":MEAS:VTIM? %e, %s%d"%(tVal, srcName, chnl))))
  return nList # Numeric list: [Curr, Min, Max, Mean, sDev, nMeas]
#---------------------------------------------------------------------------------
# Return list of displayed measmt.s, each entry being a list of 6 values:
# [Curr,Min,Max,Mean,sDev,nMeas]
#
def meas_results(scope):
  mList = []
  sResult = scope.viDev.ask(":MEAS:RES?")
  sList = sResult.split(',') # Name,Curr,Min,Max,Mean,sDev,nMeas
  nMeas = len(sList)/7
  for kMeas in range(0, len(sList)-1, 7):
    cList = []
    for k in range(1, 7):
      cList.append(float(sList[kMeas+k]))
    mList.append(cList)
  return mList
#---------------------------------------------------------------------------------

def set_trigger(scope, chnl, level=None, edge=RTO.SLP_POS):
  sEdge = RTO1.slp_name(edge)
  if level!=None:
    scope.viDev.write(":TRIG:LEV CHAN%d, %e;EDGE:SOUR CHAN%d;SLOP %s"%
                (chnl, level, chnl, sEdge))
  else:
    scope.viDev.write(":TRIG:EDGE:SOUR CHAN%d;SLOP %s"%(chnl, sEdge))
#---------------------------------------------------------------------------------

def set_trigswp(scope, mode=RTO.TRIGSWP_AUTO):
  sMode = RTO1.trigswp_name(mode)
  scope.viDev.write(":TRIG:SWE %s"%sMode)
#---------------------------------------------------------------------------------

def set_viol_width_trigger(scope, chnl, width, posPol=True, dirWide=True):
  sTrig = RTO1.trig_name(RTO.TRIG_ADV_VIOL)
  if posPol:
    sPole = "POS"
  else:
    sPole = "NEG"
  if dirWide:
    sDir = "GTH"
  else:
    sDir = "LTH"
  sViolCmd = ":TRIG:ADV:VIOL:PWID:"
  scope.viDev.write(sViolCmd+"SOUR CHAN%d"%chnl)
  scope.viDev.write(sViolCmd+"POL "+sPole)
  scope.viDev.write(sViolCmd+"DIR "+sDir)
  scope.viDev.write(sViolCmd+"WIDT %e"%width)
  
#---------------------------------------------------------------------------------
# Define measmt. threshold triple for given or all channel(s)
# thrList = [upVal, midVal, loVal]
#
def define_thr(scope, thrType, thrList=None, chnl=None, src=RTO.SRC_CHAN):
  sCmd = ":MEAS:DEF THR, "
  if (chnl==None):
    CHAN = "ALL"
  else:
    srcName = RTO1.src_name(src)
    CHAN = "%s%d"%(srcName, chnl)
  thrName = RTO1.thr_name(thrType)
  if thrType==RTO.THR_STAN: # STANdard
    sCmd += "%s,%s"%(thrName, CHAN)
  else:
    sCmd += "%s,%e,%e,%e, %s"%(thrName, thrList[0], thrList[1], thrList[2], CHAN)
  scope.viDev.write(sCmd)
#---------------------------------------------------------------------------------
# Define measmt. topbase pair for given or all channel(s)
# tbList = [topVolts, baseVolts]
#
def define_topbase(scope, tbType, tbList=None, chnl=None, src=RTO.SRC_CHAN):
  sCmd = ":MEAS:DEF TOPB, "
  if (chnl==None):
    CHAN = "ALL"
  else:
    srcName = RTO1.src_name(src)
    CHAN = "%s%d"%(srcName, chnl)
  tbName = RTO1.tb_name(tbType)
  if tbType==RTO.TB_CUSTOM:
    sCmd += "%e,%e,%s"%(tbList[0], tbList[1], CHAN)
  else:
    sCmd += "%s,%s"%(tbName, CHAN)
  scope.viDev.write(sCmd)
#---------------------------------------------------------------------------------
#
# List = [DIR_RISE|DIR_FALL|DIR_EITHER, edgeNum, POS_UPP|POS_MID|POS_LOW]
#
def define_deltime(scope, begList, endList):
  sCmd = ":MEAS:DEF DELT"
  for cList in [begList, endList]:
    sCmd += ", %s, %d, %s"%(RTO1.dir_name(cList[0]), cList[1], RTO1.pos_name(cList[2]))
  # print("DeltDef: %s"%sCmd)
  scope.viDev.write(sCmd)
#---------------------------------------------------------------------------------
# Delta-time measmt. from ch1 to ch2
# returns numeric list: [Curr] or [Curr, Min, Max, Mean, sDev, nMeas]
def meas_deltime(scope, ch1, ch2=None, stat=False, nMeas=1,
  src1=RTO.SRC_CHAN, src2=RTO.SRC_CHAN, doAppend=False):
  if (stat and (not doAppend)): # statistics on, use MEAS:RES? to get mean, min, max, rms
    scope.viDev.write(":MEAS:CLE") # clear out measmts
  nList = [] # numeric returned list
  src1Name = RTO1.src_name(src1)
  src2Name = RTO1.src_name(src2)
  mCmd = ":MEAS:SOUR %s%d"%(src1Name, ch1)
  if ch2!=None:
    mCmd += ", %s%d"%(src2Name, ch2)
  scope.viDev.write(mCmd)
  mCmd = ":MEAS:DELT"
  if (stat): # statistics on, use MEAS:RES? to get mean, min, max, rms
    # scope.viDev.write(":MEAS:CLE") # clear out measmts
    scope.viDev.write(":MEAS:STAT ON")
    mCmd += " %s%d"%(src1Name, ch1)
    if (ch2!=None):
      mCmd += ", %s%d"%(src2Name, ch2)
    scope.viDev.write(mCmd)
    # wait till >= nMeas acquisitions complete
    mCount = 0
    while (mCount < nMeas):
      time.sleep(2.0) # pause a bit, was 1.0
      sResult = scope.viDev.ask(":MEAS:RES?")
      sList = sResult.split(',') # Name,Curr,Min,Max,Mean,sDev,nMeas
      mCount = float(sList[6])
    for k in range(1, 7):
      nList.append(float(sList[k]))
  else: # current query
    mCmd += "? %s%d"%(src1Name, ch1)
    if (ch2!=None):
      mCmd += ", %s%d"%(src2Name, ch2)
    nList.append(float(scope.viDev.ask(mCmd)))
  return nList # Numeric list: [Curr, Min, Max, Mean, sDev, nMeas]
#---------------------------------------------------------------------------------

def get_vrng(lo, hi): # return range bracketing 100s of mV, plus 120mV
  dlo = floor(lo*1000)-120 # mV
  dhi = ceil(hi*1000)+120 #
  rng_mV = ceil(float(dhi-dlo)/8)*8*0.001 # ensure multiple of 8
  return rng_mV
#---------------------------------------------------------------------------------
# Get the time of src+chnl signal crossing val, edgDir from display left
#
def meas_tcross(scope, val, edgDir, chnl, stat=False, nMeas=1, src=RTO.SRC_CHAN):
  if (stat): # statistics on, use MEAS:RES? to get mean, min, max, rms
    scope.viDev.write(":MEAS:CLE") # clear out measmts
  nList = [] # numeric returned list
  srcName = RTO1.src_name(src)
  scope.viDev.write(":MEAS:SOUR %s%d"%(srcName, chnl))
  if (stat): # statistics on, use MEAS:RES? to get mean, min, max, rms
    scope.viDev.write(":MEAS:STAT ON")
    scope.viDev.write(":MEAS:TVOL %e, %d, %s%d"%(val, edgDir, srcName, chnl))
    # wait till >= nMeas acquisitions complete
    mCount = 0
    while (mCount < nMeas):
      time.sleep(1.0) # pause a bit
      sResult = scope.viDev.ask(":MEAS:RES?")
      sList = sResult.split(',') # Name,Curr,Min,Max,Mean,sDev,nMeas
      mCount = float(sList[6])
    for k in range(1, 7):
      nList.append(float(sList[k]))
  else: # current query
    nList.append(float(scope.viDev.ask(":MEAS:TVOL? %e, %d, %s%d"%(val, edgDir, srcName, chnl))))
  return nList # Numeric list: [Curr, Min, Max, Mean, sDev, nMeas]
#---------------------------------------------------------------------------------
#
# obtain pair of v-crossings of 2 channels, fNum=FUNC # for difference (chp-chn)
#
def vcross(scope, fNum, chp, chn, jpgPrefix=None):
  rList = []
  define_func(scope, fNum, RTO.OP_SUBT, chp, chn)
  auto_vrange(scope, chp)
  auto_vrange(scope, chn)
  auto_vrange(scope, fNum, RTO.SRC_FUNC)
  # set_display(scope, fNum, 1, RTO.SRC_FUNC)
  # pRng = get_vrange(scope, chp)
  # nRng = get_vrange(scope, chn)
  # set_vrange(scope, fNum, pRng[0]+nRng[0], pRng[1]-nRng[1], RTO.SRC_FUNC)
  # rising zero-xing(p-n)
  xList = meas_tcross(scope, 0., +1, fNum, True, 100, RTO.SRC_FUNC)
  xList = meas_vtime(scope, xList[3], chp, True, 100)
  if (jpgPrefix != None):
    jpgFile = jpgPrefix + '_Rise'
    save_screen(scope, jpgFile, doAsk=False, doPause=True)
  vxRise = xList[3]
  # falling zero-xing(p-n)
  xList = meas_tcross(scope, 0., -2, fNum, True, 100, RTO.SRC_FUNC)
  xList = meas_vtime(scope, xList[3], chp, True, 100)
  if jpgPrefix:
    jpgFile = jpgPrefix + '_Fall'
    save_screen(scope, jpgFile, doAsk=False, doPause=True)
  vxFall = xList[3]
  if (vxRise > vxFall):
    rList.append(vxFall)
    rList.append(vxRise)
  else:
    rList.append(vxRise)
    rList.append(vxFall)
  return rList  
#---------------------------------------------------------------------------------

def jitter_stat(scope, OnOff=True):
  on = 0
  if OnOff:
    on = 1
  scope.viDev.write(":MEAS:JITT:STAT %d"%on)
#---------------------------------------------------------------------------------
#
# Return [jitPerMin, jitPerMax, avePer]
#
def meas_jit_per(scope, chnl, nMeas=1, src=RTO.SRC_CHAN):
  perList = meas_period(scope, chnl, True, nMeas, src)
  avePer = perList[3]
  jitMin = perList[1] - avePer
  jitMax = perList[2] - avePer
  nList = [jitMin, jitMax, avePer]
  return nList # Numeric list: [Min, Max, avePer]
#---------------------------------------------------------------------------------
#
# Return [jitHperMin, jitHperMax]
#
def meas_jit_hper(scope, chnl, nMeas=1, src=RTO.SRC_CHAN):
  srcName = RTO1.src_name(src)
  scope.viDev.write(":MEAS:CLE") # clear out measmts
  scope.viDev.write(":MEAS:SOUR %s%d"%(srcName, chnl))
  # statistics on, use MEAS:RES? to get mean, min, max, rms
  scope.viDev.write(":MEAS:STAT ON")
  # Measure period, +W, -W, then min(+W,-W), max(+W, -W) for 1/2 period min/max
  scope.viDev.write(":MEAS:PER %s%d, RIS"%(srcName, chnl, ))
  scope.viDev.write(":MEAS:PWID %s%d"%(srcName, chnl))
  scope.viDev.write(":MEAS:NWID %s%d"%(srcName, chnl))
  # wait till >= nMeas acquisitions complete
  mCount = 0
  while (mCount < nMeas):
    time.sleep(1.0) # pause a bit
    nResult = scope.viDev.ask_for_values(":MEAS:RES?")
    # nResult has 7-valued triplet of -W, +W, PER in order:
    # Name,Curr,Min,Max,Mean,sDev,numMeas
    # Ensure min(numMeas) > nMeas
    mCount = min(nResult[6], nResult[7+6], nResult[2*7+6])
  minHper = min(nResult[2], nResult[7+2]) # min() of min()
  maxHper = max(nResult[3], nResult[7+3]) # max() of max()
  aveHper = 0.5*nResult[4+2*7] # ave. half-period
  jitHperMin = minHper - aveHper
  jitHperMax = maxHper - aveHper
  nList = [jitHperMin, jitHperMax]
  return nList # Numeric list: [Min, Max]
#---------------------------------------------------------------------------------
#
# Return [jccMin, jccMax]
#
def meas_jit_cc(scope, chnl, nMeas=1, src=RTO.SRC_CHAN):
  srcName = RTO1.src_name(src)
  scope.viDev.write(":MEAS:CLE") # clear out measmts
  scope.viDev.write(":MEAS:SOUR %s%d"%(srcName, chnl))
  # statistics on, use MEAS:RES? to get mean, min, max, rms
  scope.viDev.write(":MEAS:STAT ON")
  scope.viDev.write(":MEAS:CTCJ %s%d, RIS"%(srcName, chnl))
  # wait till >= nMeas acquisitions complete
  mCount = 0
  while (mCount < nMeas):
    time.sleep(1.0) # pause a bit
    sResult = scope.viDev.ask(":MEAS:RES?")
    sList = sResult.split(',') # Name,Curr,Min,Max,Mean,sDev,nMeas
    mCount = float(sList[6])
  jitMin = float(sList[2]) 
  jitMax = float(sList[3])
  nList = [jitMin, jitMax]
  return nList # Numeric list: [Min, Max]
#---------------------------------------------------------------------------------
#
#     DISK COMMANDS
#---------------------------------------------------------------------------------

def load_setup(scope, setupFile, fType=RTO.FTYP_SET, dest=1):
  if fType==RTO.FTYP_WFM:
    scope.viDev.write(":DISK:LOAD \"%s\",WMEM%d"%(setupFile, dest))
  else:
    scope.viDev.write(":DISK:LOAD \"%s\""%setupFile)
#---------------------------------------------------------------------------------

def save_screen(scope, jpgName, doAsk=True, doPause=True):
  if doPause:
    send_cmd(scope, RTO.CMD_STOP)
  if doAsk:
    yesNo = raw_input("\nScreenshot %s.jpg? (n):"%jpgName)
    if yesNo=='y' or yesNo=='1':
      yesNo = True
    else:
      yesNo = False
  else:
    yesNo = True
  if yesNo:
    cmd = ":DISK:SIM \"%s\", JPEG, SCR, ON"%(jpgName)
    scope.viDev.write(cmd)
  if doPause:
    send_cmd(scope, RTO.CMD_RUN)
#---------------------------------------------------------------------------------

def save_csv(scope, chnl, fileName, src=RTO.SRC_CHAN,
  doAsk=True, doPause=True):
  save_waveform(scope, chnl, fileName, src, doAsk, doPause)

#---------------------------------------------------------------------------------

def save_waveform(scope, chnl, fileName, src=RTO.SRC_CHAN,
  doAsk=True, doPause=True, tabDelim=False):
  if doPause:
    send_cmd(scope, RTO.CMD_STOP)
  if tabDelim: vType = "TSV"
  else: vType = "CSV"
  if doAsk:
    yesNo = raw_input("\nWaveform %s.%s? (n):"%(fileName, vType))
    if yesNo=='y' or yesNo=='1':
      yesNo = True
    else:
      yesNo = False
  else:
    yesNo = True
  if yesNo:
    if chnl <= 0: # do all channels, functions, waveform mems
      cmd = ":DISK:MST \"%s\", %s, OFF"%(fileName, vType)
    else:
      srcName = RTO1.src_name(src)
      cmd = ":DISK:STORE %s%d, \"%s\", TEXT, %s"%(srcName, chnl, fileName, vType)
    scope.viDev.write(cmd)
    # kludge to wait till file_save is done
    while True:
      time.sleep(5.0)
      id = scope.viDev.ask("*IDN?")
      if len(id) > 0:
        break
      print("Saving %s, please wait....\n"%fileName)
  if doPause:
    send_cmd(scope, RTO.CMD_RUN)


#---------------------------------------------------------------------------------

RTOSC_2p5G = 0
RTOSC_13G = 1
RTOSC_TEK = 2
RTOSC_12G = 3

RtScopeDict = {
  '5485':RTOSC_2p5G,
  '81304':RTOSC_13G,
  '7404':RTOSC_TEK,
  '81204':RTOSC_12G
}
class RtScope(LabDev):
  def __init__(self, name):
    LabDev.__init__(self, name)
    LabDev.set_model(self, RtScopeDict, DEV_SCOPE)
  # TODO: merge all scope_utils code here
  def set_display(self, chnl, OnOff=0, src=RTO.SRC_CHAN):
    srcName = RTO1.src_name(src)
    write(":%s%d:DISP %d"%(srcName, chnl, OnOff))
  # def meas_period():

  def send_cmd(self, cmd, intArg=None, cmdStrg=None):
    send_cmd(self, cmd, intArg, cmdStrg)

  def set_display(self, chnl, OnOff=0, src=RTO.SRC_CHAN):
    set_display(self, chnl, OnOff, src)

  def define_func(self, num, operator, oper1, oper2, src1=RTO.SRC_CHAN, src2=RTO.SRC_CHAN):
    define_func(self, num, operator, oper1, oper2, src1, src2)

  def set_vrange(self, chnl, vRng=None, vOffset=None, src=RTO.SRC_CHAN):
    set_vrange(self, chnl, vRng, vOffset, src)

  def get_vrange(self, chnl, src=RTO.SRC_CHAN):
    return get_vrange(self, chnl, src)

  def set_vrange_min_max(self, chnl, vMin, vMax, src=RTO.SRC_CHAN):
    set_vrange_min_max(self, chnl, vMin, vMax, src)

  def get_vrange_min_max(self, chnl, src=RTO.SRC_CHAN):
    return get_vrange_min_max(self, chnl, src)

  def auto_vrange(self, chnl, src=RTO.SRC_CHAN):
    return auto_vrange(self, chnl, src)

  def set_aver(self, OnOff=0, Count=1):
    set_aver(self, OnOff, Count)

  def set_srate(self, doAuto=True, sRate=None):
    set_srate(self, doAuto, sRate)

  def set_points(self, doAuto=True, nPts=None):
    set_points(self, doAuto, nPts)

  def set_timebase_range(self, tRng, refPos=RTO.TIME_REF_CENT):
    set_timebase_range(self, tRng, refPos)

  def set_timebase_pos(self, tPos):
    set_timebase_pos(self, tPos)


  def get_trange(self):
    return get_trange(self)

  def meas_period(self, chnl, stat=False, nMeas=1,
    src=RTO.SRC_CHAN, dir=RTO.DIR_RISE, doAppend=False):
    return meas_period(self, chnl, stat, nMeas, src, dir, doAppend)

  def meas_vrange_min_max(self, chnl, src=RTO.SRC_CHAN):
    return meas_vrange_min_max(self, chnl, src)

  def meas_single(self, measCmd, chnl, stat=False, nMeas=1,
    src=RTO.SRC_CHAN, doAppend=False):
    return meas_single(self, measCmd, chnl, stat, nMeas, src, doAppend)

  def meas_vavg(self, chnl, stat=False, nMeas=1,
    src=RTO.SRC_CHAN, doAppend=False):
    return meas_vavg(self, chnl, stat, nMeas, src, doAppend)

  def meas_vamp(self, chnl, stat=False, nMeas=1, src=RTO.SRC_CHAN, doAppend=False):
    return meas_vamp(self, chnl, stat, nMeas, src, doAppend)

  def meas_vtop(self, chnl, stat=False, nMeas=1, src=RTO.SRC_CHAN, doAppend=False):
    return meas_vtop(self, chnl, stat, nMeas, src, doAppend)

  def meas_vbase(self, chnl, stat=False, nMeas=1, src=RTO.SRC_CHAN, doAppend=False):
    return meas_vbase(self, chnl, stat, nMeas, src, doAppend)

  def meas_vtime(self, tVal, chnl, stat=False, nMeas=1, src=RTO.SRC_CHAN, doAppend=False):
    return meas_vtime(self, tVal, chnl, stat, nMeas, src, doAppend)

  def meas_results(self):
    return meas_results(self)

  def set_trigger(self, chnl, level=None, edge=RTO.SLP_POS):
    set_trigger(self, chnl, level, edge)

  def set_trigswp(self, mode=RTO.TRIGSWP_AUTO):
    set_trigswp(self, mode)

  def set_viol_width_trigger(self, chnl, width, posPol=True, dirWide=True):
    set_viol_width_trigger(self, chnl, width, posPol, dirWide)

  def define_thr(self, thrType, thrList=None, chnl=None, src=RTO.SRC_CHAN):
    define_thr(self, thrType, thrList, chnl, src)

  def define_topbase(self, tbType, tbList=None, chnl=None, src=RTO.SRC_CHAN):
    define_topbase(self, tbType, tbList, chnl, src)

  def define_deltime(self, begList, endList):
    define_deltime(self, begList, endList)

  def meas_deltime(self, ch1, ch2=None, stat=False, nMeas=1,
    src1=RTO.SRC_CHAN, src2=RTO.SRC_CHAN, doAppend=False):
    return meas_deltime(self, ch1, ch2, stat, nMeas, src1, src2, doAppend)

  def get_vrng(self, lo, hi): # return range bracketing 100s of mV, plus 120mV
    return get_vrng(self, lo, hi)

  def meas_tcross(self, val, edgDir, chnl, stat=False, nMeas=1, src=RTO.SRC_CHAN):
    return meas_tcross(self, val, edgDir, chnl, stat, nMeas, src)

  def vcross(self, fNum, chp, chn, jpgPrefix=None):
    return vcross(self, fNum, chp, chn, jpgPrefix)

  def jitter_stat(self, OnOff=True):
    jitter_stat(self, OnOff)

  def meas_jit_per(self, chnl, nMeas=1, src=RTO.SRC_CHAN):
    return meas_jit_per(self, chnl, nMeas, src)

  def meas_jit_hper(self, chnl, nMeas=1, src=RTO.SRC_CHAN):
    return meas_jit_hper(self, chnl, nMeas, src)

  def meas_jit_cc(self, chnl, nMeas=1, src=RTO.SRC_CHAN):
    return meas_jit_cc(self, chnl, nMeas, src)

  def load_setup(self, setupFile, fType=RTO.FTYP_SET, dest=1):
    load_setup(self, setupFile, fType, dest)

  def save_screen(self, jpgName, doAsk=True, doPause=True):
    save_screen(self, jpgName, doAsk, doPause)

  def save_csv(self, chnl, fileName, src=RTO.SRC_CHAN,
    doAsk=True, doPause=True):
    save_csv(self, chnl, fileName, src, doAsk, doPause)

  def save_waveform(self, chnl, fileName, src=RTO.SRC_CHAN, doAsk=True, doPause=True, tabDelim=False):
    save_waveform(self, chnl, fileName, src, doAsk, doPause, tabDelim)
    
#---------------------------------------------------------------------------------

def scope_utils_run():
  import time
  #  from pylab.utils.labutils import get_index
  from labutils import get_index
  
  il = get_instruments_list()
  dso = RtScope(il[get_index(il, "DSO81204")]) # '4f0705c"
  dso.dev_print()
  jper = dso.meas_jit_per(1, 10000)
  jper = [1e12 * x for x in jper] # ps
  print("CHAN1: jitMin=%e, jitMax=%e, avePer=%e ps"%
        (jper[0], jper[1], jper[2]))
  jcc = dso.meas_jit_cc(1, 10000)
  jcc = [1e12 * x for x in jcc] # ps
  print("CHAN1: jccMin=%e, jccMax=%e ps"%(jcc[0], jcc[1]))

  if (True):
    for kCh in range(1, 5):
      rList = dso.get_vrange(kCh)
      print("CHAN%d: RANGE=%e, OFFSET=%e"%(kCh, rList[0], rList[1]))
      rList = dso.get_vrange_min_max(kCh)
      print ("CHAN%d: Min=%e, Max=%e"%(kCh, rList[0], rList[1]))
      rList = dso.meas_vrange_min_max(kCh)
      print("CHAN%d: VMIN=%e, VMAX=%e"%(kCh, rList[0], rList[1]))

  dso.set_trigger(2, 0.75, RTO.SLP_NEG) # trigger on ch2 falling edge
  dso.define_thr(RTO.THR_VOLT, [0.2, 0., -0.2], 1)
  dso.define_thr(RTO.THR_VOLT, [0.75*2*0.8, 0.75, 0.75*2*0.2], 2)
  dso.define_thr(RTO.THR_VOLT, [0.1, 0., -0.1], 3)
  dso.define_thr(RTO.THR_PERC, [80, 50, 20], 4)
  dso.define_deltime([RTO.DIR_FALL, 1, RTO.POS_MID],
                     [RTO.DIR_RISE, 4, RTO.POS_MID])

  rList = dso.meas_deltime(2, 1)
  print("Current Delay(CHAN2, CHAN1)=%e"%rList[0])

  rList = dso.meas_deltime(2, 1, True, 100)
  print("Ave. Delay(CHAN2, CHAN1)=%e"%rList[3])
  

  dso.define_deltime([RTO.DIR_FALL, 1, RTO.POS_MID],
                 [RTO.DIR_FALL, 3, RTO.POS_MID])
  rList = dso.meas_deltime(2, 3, True, 100)
  print("Ave. Delay(CHAN2, CHAN3)=%e"%rList[3])

  dso.set_trigger(4, 0.75, RTO.SLP_POS) # trigger on ch4 rising edge
  dso.define_deltime([RTO.DIR_RISE, 1, RTO.POS_MID],
                     [RTO.DIR_RISE, 3, RTO.POS_MID])
  rList = dso.meas_deltime(4, 1, True, 100)
  print("Ave. Delay(CHAN4, CHAN1)=%e"%rList[3])

