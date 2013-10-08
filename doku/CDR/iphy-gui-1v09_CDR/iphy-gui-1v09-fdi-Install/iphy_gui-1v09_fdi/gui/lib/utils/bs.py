#
# $Header: /projects/wlv/iphy/sos/repository/iphy_A.rep/TEMP/case_1285613542/#alidation#iphy-gui#gui#lib#utils#bs.py,v 1.1 2011-09-02 11:44:11-07 case Exp $
# $Log: #alidation#iphy-gui#gui#lib#utils#bs.py,v $
# Revision 1.1  2011-09-02 11:44:11-07  case
# ...No comments entered during checkin...
#
# Revision 1.1  2011-05-04 18:10:34-07  rbemra
# Initial python+pyvisa for instrumentation (from ExacTik/882)
#
#-------------------------------------------------------------

def bin_search(
  func,
  fnArgs, # [0]:stimInstHdl, [1]:measInstHdl, [2..]:addl.optional
  lo, # lower bracketing value
  hi, # upper bracket
  yVal, # desired value yv = fn(returnVal)
  yTol) # rel. f(x) tolerance

  pVec = fnArgs->pv.ppVec
  pStimDev = (InstType *)(pVec? pVec[0]: NULL) # stimulus instrument
  pMeasDev = (InstType *)(pVec? pVec[1]: NULL) # measmt. instrument
  iArg2 = iArg1 = 0
  pFlt2 = pFlt1 = NULL
  pVecNew = NULL
  switch (fnArgs->nVal):
   case 2:
    break
   default: # >= 7
    pVecNew = (VecType *)pVec[6]
   case 6:
    pFlt2 = (float *)pVec[5]
   case 5:
    pFlt1 = (float *)pVec[4]
   case 4:
    iArg2 = pVec[3]? *(int *)pVec[3]: 0 # int arg.
   case 3:
    iArg1 = pVec[2]? *(int *)pVec[2]: 0 # int arg.
  
  if (pFlt1)
    faVal = *pFlt1
  else:
    faVal = func(pStimDev, lo, pMeasDev, iArg1, iArg2, pVecNew)
  if (pFlt2)
    fbVal = *pFlt2
  else:
    fbVal = func(pStimDev, hi, pMeasDev, iArg1, iArg2, pVecNew)

  fprintf(stderr, "bin_search: f(%d)=%f, f(%d)=%f\n", lo, faVal, hi, fbVal)
  monoIncFn = (fbVal > faVal)? TRUE: FALSE
  faVal -= yVal
  fbVal -= yVal
  
  absTol = yTol*fabs(yVal)
  iter = 1
  index = lo
  while (lo<=hi) :
    index = (lo+hi) >> 1
    fVal = func(pStimDev, index, pMeasDev, iArg1, iArg2, pVecNew)
    fprintf(stderr, "%d) f(%d)=%f\n", iter++, index, fVal)
    fVal -= yVal
    vDiff = fabs(fVal)
    if (monoIncFn) : # mono. increasing
      if (vDiff < absTol) #
        break
      elif (fVal<0) :
        lo = index # +1?
        faVal = fVal
      else: : # (fVal>0)
        hi = index # -1?
        fbVal = fVal
      if ((hi-lo)<2) :
        index = fabs(faVal) < fabs(fbVal)? lo: hi
        break
    else: # mono. decreasing
      if (vDiff < absTol) #
        break
      elif (fVal<0) :
        hi = index # +1?
        fbVal = fVal
      else: # (fVal>0)
        lo = index # -1?
        faVal = fVal
      
      if ((hi-lo)<2) :
        index = fabs(faVal) < fabs(fbVal)? lo: hi
        break
  
  return index
 # bin_search
