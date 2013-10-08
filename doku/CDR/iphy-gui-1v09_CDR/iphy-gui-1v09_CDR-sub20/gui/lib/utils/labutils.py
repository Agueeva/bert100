#
# $Header: /projects/wlv/iphy/sos/repository/iphy_A.rep/TEMP/case_1285613542/#alidation#iphy-gui#gui#lib#utils#labutils.py,v 1.1 2011-09-02 11:44:11-07 case Exp $
# Old Header: /inphi/inlab/pylab/utils/labutils.py,v 1.2 2008/07/23 02:01:35 rbemra Exp
# $Log: #alidation#iphy-gui#gui#lib#utils#labutils.py,v $
# Revision 1.1  2011-09-02 11:44:11-07  case
# ...No comments entered during checkin...
#
# Revision 1.1  2011-05-04 18:10:34-07  rbemra
# Initial python+pyvisa for instrumentation (from ExacTik/882)
#
# Revision 1.2  2008/07/23 02:01:35  rbemra
# Added S2P class, timeout code.
# Deleted CRs at EOLs
#
# Low-level, device-independent lab/measmt utility module
# Revision 1.1.1.1  2008/07/21 18:20:28  rbemra
# Initial Version
#
# Revision 1.1  2008/04/12 01:08:55  rbemra
# Moved instrument/product indep. Python utils here
#
#---------------------------------------------------------------------------------

import math
  
#---------------------------------------------------------------------------------
ITMAX = 20
EPS = 0.001 # 3.0e-8
# global FunArgs
# FunArgs = []
#---------------------------------------------------------------------------------

def tol_check(testVal, trueVal, RelTol=None, AbsTol=None):
  dVal = abs(testVal-trueVal)
  if (AbsTol!=None and AbsTol>0. and (dVal > AbsTol)):
    return False
  absT = abs(trueVal)
  if (RelTol!=None and RelTol>0. and absT!=0 and (dVal > RelTol*absT)):
    return False
  return True
#---------------------------------------------------------------------------------

def get_index(mList, key, caseSense=True):
  mLen = len(mList)
  cmpKey = key
  if not caseSense:
    cmpKey = cmpKey.upper()
  for ix in range(mLen):
    memVal = mList[ix]
    if not caseSense:
      memVal = memVal.upper()
    if (memVal.__contains__(cmpKey)):
      return ix
  return -1
#---------------------------------------------------------------------------------

def base_name(fileName):
  kDot = fileName.rfind('.')
  if kDot < 0:
    baseName = fileName
  else:
    baseName = fileName.__getslice__(0,kDot) # without dot
  return baseName
#---------------------------------------------------------------------------------

CFACTOR = 0.9 # creep factor, like Samanskii

def creep(func, FunArgs, x1, yv, tol=EPS):
  a = x1
  instStim = FunArgs[0] # stimulus instrument
  instMeas = FunArgs[1] # measmt. instrument
  chp = FunArgs[2] # channel 1
  chn = FunArgs[3] # channel 2
  if (len(FunArgs) >= 5): # [4]=f(x1)
    fa = FunArgs[4]
  else:
    fa = func(instStim, a, instMeas, chp, chn) # return measmt for input a

  nIter = 0
  absTol = tol*abs(yv)
  stat = True
  while (True):
    delV = yv-fa
    vDiff = abs(delV)
    if (vDiff < absTol): #
      # print("creep: root=%f, nIter=%d, Value=%f"%(a, nIter, fa))
      break
    a = a + CFACTOR*delV 
    fa = func(instStim, a, instMeas, chp, chn) # return measmt for input a
    # print("%d) Actual=%.2f, vPut=%.2f"%(nIter+1, fa, a))
    # if (vDiff > 0.010):
    #   time.sleep(1.0) # wait only 1 secs between checks
    # else:
    #   time.sleep(0.2) # wait 0.2 sec between checks
    nIter += 1
    if (nIter > ITMAX):
      print("Error: unable to set value=%d"%yv)
      stat = False
      break
  return [stat, a]

#---------------------------------------------------------------------------------

def ZB_SIGN(a,b):
  if (b >= 0.0):
    rv = a.__abs__()
  else:
    rv = -a.__abs__()
  return rv
#---------------------------------------------------------------------------------

def zbrent(func, FunArgs, x1, x2, yv, tol=EPS):
  a = x1
  b = x2
  c = x2
  # print "in zbrent, FunArgs:"
  # print FunArgs
  instStim = FunArgs[0] # stimulus instrument
  instMeas = FunArgs[1] # measmt. instrument
  chp = FunArgs[2] # channel 1
  chn = FunArgs[3] # channel 2
  if (len(FunArgs) >= 6): # [4]=f(x1), [5]=f(x2)
    fa = FunArgs[4]-yv
    fb = FunArgs[5]-yv
  else:
    fa = func(instStim, a, instMeas, chp, chn)-yv # return measmt for input a
    fb = func(instStim, b, instMeas, chp, chn)-yv # return measmt for input b
  if ((fa > 0.0 and fb > 0.0) or (fa < 0.0 and fb < 0.0)):
    print("Root is not bracketed in [%.3e, %.3e]"%(a, b))
    return [False, 0.0]
  fc = fb
  iter = 1
  for iter in range(1, ITMAX+1):
    if ((fb > 0.0 and fc > 0.0) or (fb < 0.0 and fc < 0.0)):
      c = a
      fc = fa
      e = d = b-a
    if (fc.__abs__() < fb.__abs__()):
      a = b
      b = c
      c = a
      fa = fb
      fb = fc
      fc = fa
    tol1 = 2.0*EPS*b.__abs__()+0.5*tol
    xm = 0.5*(c-b)
    if (xm.__abs__() <=  tol1 or fb == 0.0):
      # print("zbrent: root=%f, nIter=%d, Value=%f"%(b, iter, fb+yv))
      return [True, b]
    if (e.__abs__() >= tol1 and fa.__abs__() > fb.__abs__()):
      s = fb/fa
      if (a == c):
        p = 2.0*xm*s
        q = 1.0-s
      else:
        q = fa/fc
        r = fb/fc
        p = s*(2.0*xm*q*(q-r)-(b-a)*(r-1.0))
        q = (q-1.0)*(r-1.0)*(s-1.0)
      if (p > 0.0):
        q = -q
      p = p.__abs__()
      min1 = 3.0*xm*q - (tol1*q).__abs__()
      min2 = (e*q).__abs__()
      if (min1 < min2):
        min12 = min1
      else:
        min12 = min2
      if (2.0*p < min12):
        e = d
        d = p/q
      else:
        d = xm
        e = d
    else:
      d = xm
      e = d
    a = b
    fa = fb
    if (d.__abs__() > tol1):
      b += d
    else:
      b +=  ZB_SIGN(tol1, xm)
    if (b < x1) or (b > x2):
      print("zbrent: guess %f outside [%f, %f] bracket\n"%(b, x1, x2))
    fb = func(instStim, b, instMeas, chp, chn)-yv

  print ("Maximum number of iterations exceeded in zbrent")
  return [False, b]
#--------------------------------------------------------------------------------

def gd_one(sPar, fqList): # seconds
  gDelay = []
  nP = len(fqList)
  if len(sPar)!=nP:
    raise Exception("Mismatched lengths of fqList, S21")
  for k in range(nP-1):
    dFq = fqList[k+1] - fqList[k]
    if not(dFq):
      print "Warning: zero delta freq...setting gd=1E10s\n"
      gd = 1E10
    else:
      # IF((B3-B2)>180,(B3-B2-360),IF((B3-B2)<-180, (B3-B2+360), (B3-B2)))
      # -E2*1000000000000/(A3-A2)/360
      ph1 = math.atan2(sPar[k].imag, sPar[k].real)
      ph2 = math.atan2(sPar[k+1].imag, sPar[k+1].real)
      dDeg = (ph2-ph1)*180/math.pi
      while dDeg > 180.0:
        dDeg = dDeg - 360.0
      while dDeg < -180.0:
        dDeg = dDeg + 360.0
      gd = -dDeg/dFq/360.0
    gDelay.append(gd)
  return gDelay
#-----------------------------------------------------------------------------

class S2P:
  fqList = []
  S11 = []
  S12 = []
  S21 = []
  S22 = []
  #----------------------------------------------------
  def write_s2p(self, s2pName, comment):
    s2pFyl = open(s2pName, 'a')
    s2pFyl.write("! %s\n"%comment)
    s2pFyl.write("# HZ S RI R 50\n")
    for k in range(len(self.fqList)):
      s2pFyl.write("%e %e %e %e %e %e %e %e %e\n"%\
        (self.fqList[k],
         self.S11[k].real, self.S11[k].imag,
         self.S21[k].real, self.S21[k].imag,
         self.S12[k].real, self.S12[k].imag,
         self.S22[k].real, self.S22[k].imag))
    s2pFyl.flush()
    s2pFyl.close()
  #----------------------------------------------------
  def group_delay(self): # seconds
    return gd_one(self.S21, self.fqList)
#-----------------------------------------------------------------------------

def loss_db(cSP, name, doLoss=True):
  nVal = len(cSP)
  lossList = []
  if doLoss:
    mFactor = -20.0
  else:
    mFactor = 20.0 # gain
  for k in range(0, nVal):
    mc = abs(cSP[k])
    if mc > 0:
      loss = mFactor*math.log10(abs(cSP[k]))
    else:
      loss = -100
      print "warning: abs(%s[%d]) is 0!!\n"%(name, k)
    lossList.append(loss)
  return lossList
#-----------------------------------------------------------------------------

def usleep(uSec, doTell=False):
  import time
  tSec = 1e-6*uSec
  if doTell:
    print("Sleeping for %.2f seconds..."%tSec)
  time.sleep(tSec)
#--------------------------------------------------------------------------------

def pico(x):
  return (x*1e12)
#---------------------------------------------------------------------------------

def spico(x):
  s = "%.2f"%(x*1e12)
  return s
#---------------------------------------------------------------------------------
#
# Try timeout, if nobody available to respond
# Return user string, None if timeout occurred
#
def get_ui_to(prompt, toSec=None, tSleepSec=None):
  # import sys
  from time import time, sleep
  from msvcrt import getch, getche, kbhit

  if toSec==None: # wait forever
    userKey = raw_input(prompt)
    return userKey

  tElapsed = 0
  t0 = time()
  if tSleepSec == None:
    tSleep = 0.1*toSec
  else:
    tSleep = tSleepSec
  while True:
    if tElapsed > toSec:
      print "Timeout after tElapsed secs...%.3f"%tElapsed
      userKey = ''
      break
    print "\n", prompt,
    if kbhit():
      userKey = getche()
      while kbhit(): # flush input
        getch() # sys.stdin.flush()
      # userKey = raw_input(prompt)
      break
    # print "sleep tSleep secs...%.3f"%tSleep
    sleep(tSleep)
    tNow = time()
    tElapsed = tNow - t0

  return userKey

#---------------------------------------------------------------------------------
# 
#  Local Variables:
#  tab-width: 2
#  indent-tabs-mode: nil
#  scroll-step: 1
#  line-number-mode: t
#  column-number-mode: t
#  End:
# 
