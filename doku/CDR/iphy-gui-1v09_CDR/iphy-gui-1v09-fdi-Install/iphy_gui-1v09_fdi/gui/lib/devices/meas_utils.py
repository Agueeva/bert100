#
# $Header: /projects/wlv/iphy/sos/repository/iphy_A.rep/TEMP/case_1285613542/#alidation#iphy-gui#gui#lib#devices#meas_utils.py,v 1.1 2011-09-02 11:43:53-07 case Exp $
# Old $Header: /inphi/inlab/pylab/devices/meas_utils.py,v 1.2 2008/07/23 02:05:28 rbemra Exp
# $Log: #alidation#iphy-gui#gui#lib#devices#meas_utils.py,v $
# Revision 1.1  2011-09-02 11:43:53-07  case
# ...No comments entered during checkin...
#
# Revision 1.1  2011-05-04 18:10:35-07  rbemra
# Initial python+pyvisa for instrumentation (from ExacTik/882)
#
# Revision 1.2  2008/07/23 02:05:28  rbemra
# Deleted CRs at EOLs
# inlab -> inlab/pylab
#
# Revision 1.1.1.1  2008/07/21 18:20:28  rbemra
# Initial Version
#
#
# From: /inphi/py882/meas_utils.py,v 1.11 2008/04/12
# Revision 1.11  2008/04/12 01:27:25  rbemra
# Moved some utils to pyutils.py
#
# Revision 1.10  2008/03/01 00:11:12  rbemra
# 1. Print msg. if guess outside bracket in zbrent()
# 2. Supply f(x) of bracket in set_vdc()
#
# Revision 1.9  2008/02/16 02:17:37  rbemra
# Bugfix in min_ave_max() for true Min, Max
#
# Revision 1.8  2008/02/03 22:07:55  rbemra
# Added some utilities for min/ave/max recording
#
# Revision 1.7  2008/01/16 02:00:31  rbemra
# 1. Added creep() to sneak up/down monotonically from initial guess.
# 2. Small fix in zbrent()
#
# Revision 1.6  2008/01/10 19:39:14  rbemra
# add Log key word
#
import time
# from pylab.utils.labutils import *
# from pylab.devices.srcmeter import *
from labutils import *
from srcmeter import *
#--------------------------------------------------------------------------------
# added 01/09/08 for VddMon feature, imported by scope_utils
def set_get_vdc(vSupp, vdc, dvm, chSrc, chMeas):
  vSupp.set_v(vdc, chSrc) # write("SOUR:VOLT:LEV %e"%vdc) # E3631A supply

  setVal = vSupp.get_v(chSrc) # float(vSupp.ask("MEAS:VOLT:DC?")) # for Agilent E3631A
  
  if (not tol_check(setVal, vdc, RelTol=0.01)):
    print("* * * Error * * * desired=%e, actual=%e"%(vdc, setVal))
    return setVal

  vSupp.get_i(chSrc) # vSupp.ask("MEAS:CURR:DC?") # E3631A supply, to restore display of actual current
  time.sleep(1.0) # settle
  if dvm != None:
    rVal = dvm.get_v(chMeas) # dvm.ask_for_values("MEAS:VOLT:DC? DEF,DEF,(@%s)"%chMeas)
    # rVal = rv[0]
    # rVal = dvm.get_v(chMeas)
  else:
    rVal = None
  return rVal

#-------------------------------------------------------------
# set vdc within +/-5mV of desired
def set_vdc(vSupply, dvm, vdc, chSrc, chMeas):
  if dvm == None: # no DVM, use Vsupply's value, no EVB/cable loss
    set_get_vdc(vSupply, vdc, None, chSrc, None)
    return True

  absTol_mV = 4.0
  # print "absTol_mV=%f"%absTol_mV
  # global dso.FunArgs
  FunArgs = []
  FunArgs.append(vSupply)
  FunArgs.append(dvm)
  FunArgs.append(chSrc)
  FunArgs.append(chMeas)
  # Bracket vdc:
  vSet = vdc
  while True:
    vSet = 0.95*vSet
    vLow = set_get_vdc(vSupply, vSet, dvm, chSrc, chMeas)
    if vLow < vdc:
      FunArgs.append(vLow)
      vLow = vSet # below x-root
      stat = True
      break
  # [stat, vSet] = creep(set_get_vdc, FunArgs, vLow, vdc, 1e-3*absTol_mV/vdc)
  # vGet = set_get_vdc(vSupply, vSet, dvm, chSrc, chMeas)
  # if not stat:
  #   print("creep() Error: unable to set VDC=%e, actual=%e"%(vdc, vGet))
  # else:
  #   print("creep(): Desired=%f, Actual=%f"%(vdc, vGet))

  vSet = vdc
  while True:
    vSet = 1.05*vSet
    vHi = set_get_vdc(vSupply, vSet, dvm, chSrc, chMeas)
    if vHi > vdc:
      FunArgs.append(vHi)
      vHi = vSet # above x-root
      stat = True
      break
  [stat, vSet] = zbrent(set_get_vdc, FunArgs, vLow, vHi, vdc, 1e-3*absTol_mV/vdc)
  vGet = set_get_vdc(vSupply, vSet, dvm, chSrc, chMeas)
  if not stat:
    print("Error: unable to set VDC=%e, actual=%e"%(vdc, vGet))
  else:
    print("zbrent(): Desired=%f, Actual=%f"%(vdc, vGet))

  return stat

#---------------------------------------------------------------------------------
# Return formatted, scaled string "min, ave, max"
#
def scale_fmt3(vList, mFactor=1.0, sFmt="%.1f"):
  s = (sFmt%(mFactor*vList[0]))+", "\
    + (sFmt%(mFactor*vList[1]))+", "\
    + (sFmt%(mFactor*vList[2]))
  return s
#---------------------------------------------------------------------------------
# Return list [min, ave, max]
#
def lo_ave_hi(dsoList, mFactor=1.0):
  dsoList.pop(0) # pop off Curr, [min, max, ave] left
  rL = []
  rL.append(mFactor*dsoList[0])
  rL.append(mFactor*dsoList[2])
  rL.append(mFactor*dsoList[1])
  return rL
#---------------------------------------------------------------------------------

def min_ave_max(L1, L2, offset=0., doInvert=False):
  aL = []
  if doInvert:
    aL.append(offset - max(L1[2], L2[2]))
    aL.append(offset - 0.5*(L1[1]+L2[1]))
    aL.append(offset - min(L1[0], L2[0]))
  else:
    aL.append(min(L1[0], L2[0])-offset)
    aL.append(0.5*(L1[1]+L2[1])-offset)
    aL.append(max(L1[2], L2[2])-offset)
  return aL
#---------------------------------------------------------------------------------

def get_scaled_limited(device, measCmd, scale=1.0, limit=0.0):
  [val] = device.viDev.ask_for_values(measCmd)
  val = val*scale
  if (limit and (val >= limit)):
    val = 0.0
  return val
