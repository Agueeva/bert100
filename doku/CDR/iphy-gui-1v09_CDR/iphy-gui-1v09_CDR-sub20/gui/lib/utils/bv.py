#
# $Header: /projects/wlv/iphy/sos/repository/iphy_A.rep/TEMP/case_1285613542/#alidation#iphy-gui#gui#lib#utils#bv.py,v 1.1 2011-09-02 11:44:11-07 case Exp $
# Old Header: /inphi/inlab/pylab/utils/bv.py,v 1.2 2008/09/10 19:22:51 rbemra Exp
# $Log: #alidation#iphy-gui#gui#lib#utils#bv.py,v $
# Revision 1.1  2011-09-02 11:44:11-07  case
# ...No comments entered during checkin...
#
# Revision 1.4  2011-08-09 11:49:22-07  rbemra
# "flt" length fix in token_value()
#
# Revision 1.3  2011-06-30 21:28:25-07  rbemra
# Some BitVector method fixes/updates
# Removed *Reg0
#
# Revision 1.2  2011-05-04 19:06:43-07  rbemra
# Some updates for hex/bin printing
#
# Revision 1.1  2011-05-04 18:10:34-07  rbemra
# Initial python+pyvisa for instrumentation (from ExacTik/882)
#
# Revision 1.2  2008/09/10 19:22:51  rbemra
# 1. Added vector patterns: walk_1bit, indep_prbs, sso_prbs, const.
# 2. Added BitVector.odd_parity()
# 3. Moved 882-specific register definitions to 882LV_A application
#
# Revision 1.1  2008/08/19 20:08:01  rbemra
# Initial version
#
# Register & Bit vector utility module
# Initial Version
#

#------------------------------------------------------------

bin2hex = {
  '0000': '0', '0001': '1', '0010': '2', '0011':'3',
  '0100': '4', '0101': '5', '0110': '6', '0111':'7',
  '1000': '8', '1001': '9', '1010': 'a', '1011':'b',
  '1100': 'c', '1101': 'd', '1110': 'e', '1111':'f'
}
hex2bin = {
  '0': '0000', '1': '0001', '2': '0010', '3': '0011',
  '4': '0100', '5': '0101', '6': '0110', '7': '0111',
  '8': '1000', '9': '1001', 'a': '1010', 'b': '1011',
  'c': '1100', 'd': '1101', 'e': '1110', 'f': '1111'
}
#------------------------------------------------------------

def bs2xs(ebs): # convert string with '0','1' into hex string
  if ebs[0:2]=='0b' or ebs[0:2]=='0B':
    bs = ebs[2::]
  else:
    bs = ebs
  ln = len(bs)
  nPrepad = ln%4
  if nPrepad:
    nPrepad = 4 - nPrepad
    zPad = ''
    for k in range(nPrepad):
      zPad = zPad+'0'
    newBs = zPad+bs
    ln = ln + nPrepad
  else:
    newBs = bs
# print 'newBs:', newBs, 'ln:', ln
  sHex = ''
  for k in range(0, ln, 4):
    nibble = newBs[k:k+4]
    try:
      cHex = bin2hex[nibble]
    except:
      pass # print "Ignoring:", nibble
    else:
      sHex = sHex + cHex
      
  sHex = '0x'+sHex
  return sHex
#------------------------------------------------------------

def xs2bs(exs, fLen=None): # convert hex string to binary string
  if exs[0:2]=='0x' or exs[0:2]=='0X':
    xs = exs[2::]
  else:
    xs = exs
  sBin = ''
  xLen = len(xs)
  x2bLen = 4*xLen
  locLen = x2bLen if (fLen==None or fLen<x2bLen) else fLen
  for hx in xs:
    sBin = hex2bin[hx] + sBin
  for k in range(0, locLen-x2bLen):
    sBin = '0' + sBin
  return sBin[-fLen:] if (fLen!=None) else sBin
#------------------------------------------------------------
def is2xs(ins, fLen=None): # convert integer string to hex string
  if fLen!=None:
    fmtStrg = "%%0%dx"%fLen
  else:
    fmtStrg = "%x"
  return fmtStrg%eval(ins)
#------------------------------------------------------------

def i2xs(iVal, fLen=None): # convert integer to hex string
  if fLen!=None:
    fmtStrg = "%%0%dx"%fLen
  else:
    fmtStrg = "%x"
  return fmtStrg%iVal
#------------------------------------------------------------

def xs2is(xs): # convert hex string to int string
  if xs[0:2]=='0x' or xs[0:2]=='0X':
    exs = xs
  else:
    exs = '0x'+xs
  return "%d"%eval(exs)
#------------------------------------------------------------

def bs2is(bs): # binary string to int string
  xs = bs2xs(bs)
  return xs2is(xs)
#------------------------------------------------------------

def is2bs(ins): # int string to binary string
  xs = is2xs(ins)
  return xs2bs(xs)
#------------------------------------------------------------

def i2bs(iVal, fLen=None): # int to binary string
  xs = i2xs(iVal)
  return xs2bs(xs, fLen)
#------------------------------------------------------------

def xdelim(bs, delim=' \t_'): # pack bitstream, strip delims
  import string
  xlate = string.maketrans('lLhH', '0011') # convert l, H if present
  return bs.translate(xlate, delim)
#------------------------------------------------------------
# verilog string:
# 12'b1011_0101_1100, 32'habCd, (not yet: 10'd123)
# 12'B1011100, 40'H1Ed_5, (not yet: 3'D1_23)
#
def vs2value(vs): # verilog string to value
  vs = vs.lower()
  ls = vs.split("'")
  vLen = len(ls)
  if vLen==1: # no ' 1-quote
    vvs = xdelim(ls[0]) # remove delimiters
    vsVal = long(eval(vvs))
    nBits = 0
    while vsVal > pow(2, nBits):
      nBits += 1
  else:
    import re
    rex = re.compile("([0-9]+)[ \t]*'([bdhx])[ \t]*([0-9a-f_]+)")
    match = rex.match(vs)
    if match==None:
      raise Exception("%s has syntax error"%vs)
    mGrp = match.groups()
    nBits = eval(mGrp[0])
    fmt = mGrp[1]
    # vvs = mGrp[2]
    vvs = xdelim(mGrp[2]) # remove delimiters
    if fmt=='d':
      vsVal = long(eval(vvs))
    elif fmt=='b': # 'b'
      vsVal = long(eval(bs2xs(vvs)))
    else: # 'h' or 'x'
      vsVal = long(eval(xs2is(vvs)))
    if vsVal >= pow(2, nBits):
      raise Exception("%s specifies insufficient bits"%vs)
  return [nBits, vsVal]
# ----------------------------------------------------------------------------------------

FLOAT_VCTL = 100.0

def token_value(pToken, pVal): # DataType *pVal):
  lToken = pToken.lower()
  if lToken[0:2]=="0x":
    retVal = eval(lToken)
  elif (lToken[0:3]=="flt"):
    retVal = FLOAT_VCTL
  elif (lToken.find('.')>=0 or lToken.find('e')>0):
    try:
      retVal = float(pToken)
    except ValueError: # string
      try:
        [_, retVal] = vs2value(pToken)
      except:
        retVal = str(pToken)
  else: # integer
    try:
      retVal = int(pToken)
    except ValueError:
      try:
        [_, retVal] = vs2value(pToken)
      except:
        retVal = str(pToken)
  return retVal
#------------------------------------------------------------

def makebv(nBits, value=None):
  if value != None:
    rVal = long(value)
  else:
    rVal = 0L
  return rVal
#----------------------------------------------------------

def setbit(bv, kBit, value=True):
  if value:
    bv |= (0x1 << kBit)
  else:
    bv &= ~(0x1 << kBit)
  return bv
#----------------------------------------------------------

def getbit(bv, kBit):
  rBit = bv & (0x1 << kBit)
  return rBit
#----------------------------------------------------------

def print_bv(bv, fmt='x'):
  print "%x"%bv
#----------------------------------------------------------

class BitVector:
  # nBytesPerInt = type(0).__basicsize__
  # nBitsPerInt = 8 * nBytesPerInt
  #----------------------------------------------------------
  def __init__(self, nBits, value=None):
    # from math import ceil
    # nInt = int(ceil(nBits/float(self.nBitsPerInt)))
    # self.bvList = []
    # for k in range(nInt):
    #  self.bvList.append(0)
    zVal = 0
    zValL = 0L
    sVal = '0'
    if value != None:
      if type(value)==type(zVal) or type(value)==type(zValL):
        lVal = long(value)
      elif type(value)==type(sVal):
        [nBval, lVal] = vs2value(value)
      else:
        raise Exception("Unknown type")
      if lVal > pow(2, nBits)-1:
        raise Exception("%ld too big for %d bits"%(lVal, nBits))
      self.value = lVal
    else: # default = clear all bits
      self.value = zValL
    self.bvLen = nBits
  #----------------------------------------------------------

  def setbit(self, k, value=True):
    if k >= self.bvLen:
      return
    # kInt = k/self.nBitsPerInt
    # kBit = k - kInt*self.nBitsPerInt
    if value:
      # self.bvList[kInt] |= (0x1 << kBit)
      self.value |= (0x1 << k)
    else:
      # self.bvList[kInt] &= ~(0x1 << kBit)
      self.value &=  ~(0x1 << k)
  #----------------------------------------------------------

  def clrbit(self, k):
    self.setbit(k, False)
  #----------------------------------------------------------

  def getbit(self, k):
    # kInt = k/self.nBitsPerInt
    # kBit = k - kInt*self.nBitsPerInt
    # rBit = self.bvList[kInt] & (0x1 << kBit)
    rBit = self.value & (0x1 << k)
    if rBit:
      rBit = 0x1
    else:
      rBit = 0
    return rBit
  #----------------------------------------------------------

  def getbits(self, start=0, nBits=None):
    if nBits==None:
      nBits = self.bvLen-start
    rVal = (self.value >> start) & (pow(2, nBits)-1)
    return rVal
  #----------------------------------------------------------

  def setbits(self, start=0, nBits=None, ones=True):
    if nBits==None:
      nBits = self.bvLen-start
    if ones:
      bMask = (long((pow(2, nBits)-1)) << start)
      self.value |= bMask
    else:
      bMask = ~(long((pow(2, nBits)-1)) << start)
      self.value &= bMask
    return self
  #----------------------------------------------------------

  def clrbits(self, start=0, nBits=None):
    self.setbits(start, nBits, False)
  #----------------------------------------------------------

  def invert(self, start=0, nBits=None):
    self.setbits(start, nBits, False)
  #----------------------------------------------------------

  def copybits(self, srcVal, destStart=0, srcStart=0, nBits=None):
    if nBits==None: # all from destStart to MSB
      nBits = self.bvLen-destStart # min(self.bvLen-destStart, srcVal.bvLen-srcStart)
    oneBits = pow(2, nBits)-1
    zMask = ~(long(oneBits) << destStart)
    self.value &= zMask # zero'd out destination
    copyMask = oneBits & (srcVal >> srcStart)
    copyMask = copyMask << destStart
    self.value |= copyMask
  #----------------------------------------------------------

  def getbyte(self, k):
    bytStart = 8*k
    endBit = bytStart+8
    if k<0 or endBit > self.bvLen:
      raise Exception("Invalid byte index %d"%k)
    val = 0
    for kBit in range(bytStart, min(self.bvLen, endBit)):
      if self.getbit(kBit):
        val |= (0x1 << (kBit-bytStart))
    return val
  #----------------------------------------------------------

  def odd_parity(self, start, stop):
    sum = 0
    for k in range(start, stop+1):
      if self.getbit(k):
        sum += 1
    if sum%2:
      return True
    else:
      return False
  #----------------------------------------------------------

  def make_rand(self):
    randVal = irand_lh(0, pow(2, self.bvLen)-1)
    return BitVector(self.bvLen, randVal)
  #----------------------------------------------------------

  def print_int(self):
    print self.value
  #----------------------------------------------------------

  def old_print_bits(self, delim='_'):
    sBits = ''
    nBits = 0
    for kBit in range(self.bvLen-1, -1, -1):
      sBits = sBits + '%1d'%self.getbit(kBit)
      nBits = nBits + 1
      if not nBits%32:
        sBits = sBits + '\n'
      elif delim!=None and not nBits%4 and nBits!=self.bvLen:
        sBits = sBits + '_'
    print(sBits+'\n')
  #----------------------------------------------------------

  def get_rbits(self, maxBitsPerLine=None, delim='_'):
    sBits = '' # "%d'b"%self.bvLen
    nBits = 0
    # nLineCh = len(sBits)
    for kBit in range(0, self.bvLen, 1):
      sBits = '%1d'%self.getbit(kBit) + sBits
      nBits = nBits + 1
      if (type(maxBitsPerLine)==int and not nBits%maxBitsPerLine):
        sBits = '\n' + sBits
      if delim!=None and not nBits%4 and nBits!=self.bvLen:
        sBits = '_' + sBits
    return sBits
  #----------------------------------------------------------

  def print_rbits(self, maxBitsPerLine=32, fylHdl=None, delim='_'):
    sBits = self.get_rbits(maxBitsPerLine, delim)
    if fylHdl!=None:
      fylHdl.write(sBits)
    else:
      print sBits
  #----------------------------------------------------------

  def print_rhex(self, maxCharsPerLine=16, fylHdl=None):
    nbvChar = int(round(self.bvLen/4.0))
    sFmt = "%d'h%%0%dx"%(self.bvLen, nbvChar)
    sHex = sFmt%self.value
    nLineCh = 0
    sLine = ''
    for ch in sHex:
      sLine += ch
      nLineCh += 1
      if nLineCh >= maxCharsPerLine:
        print sLine
        sLine = ''
        nLineCh = 0
    if len(sLine):
      if fylHdl!=None:
        fylHdl.write(sLine)
      else:
        print sLine
 #----------------------------------------------------------

  def print_bv(self, fmt='x'): # needs work
    nInt = len(self.bvList)
    for kByte in range(self.bvLen/8, -1, -1):
      byteVal = 0
      bitTop = min(kByte*8-1, self.bvLen-1)
      for kBit in range(bitTop, bitTop-8, -1):
        if getbit(self, kBit):
          byteVal = byteVal + 1
      print('%x'%byteVal)
    nIntPerLine = 2
    nOut = 0
    for k in range(nInt-1, -1, -1):
      iVal = self.bvList[k]
      print_int(iVal)
      nOut = nOut+1
      if not nOut%nIntPerLine:
        print '\n'
#----------------------------------------------------------

class BitMatrix:
  def __init__(self, nRow, nCol):
    self.bmRowHead = []
    for k in range(nRow):
      row = BitVector(nCol)
      self.bmRowHead.append(row)
    self.nRow = nRow
    self.nCol = nCol
  #-------------------------------------------------------

  def getrow(self, kRow):
    return self.bmRowHead[kRow]

  #-------------------------------------------------------

  def print_bmx(self, fmt='b'):
    for kRow in range(self.nRow):
      vRow = self.bmRowHead[kRow]
      if fmt=='x' or fmt=='h':
        vRow.print_rhex(None, 1+self.nCol/4) # /4 to ensure whole row/line printed
      elif fmt=='b':
        vRow.print_rbits(vRow.bvLen+1)
      else: # decimal
        vRow.print_int()
  #-------------------------------------------------------

  def foreach_row(self, fun, arg1=None, arg2=None):
    for kRow in range(self.nRow):
      vRow = self.bmRowHead[kRow]
      fun(vRow, arg1, arg2)

  #-------------------------------------------------------
  #def setrv(self, kRow, rVec):
    
#-----------------------------------------------------------
# Some code snippets from CTC/AMB:
# def writeb(addr, offset, byte):
# def writed(addr, off, dword):
# def readb(addr, off):
# def readw(addr, off):
# def readd(addr, off):
# # We require a 16 digit, hexadecimal number!
#         regExp = re.compile("^[0-9][A-F]")
#         if len(value) != 16 or regExp.match(value):
#             self.invalidValue("must be 16 digit hexadecimal number", value)
#             return None
#           
# #-----------------------------------------------------------
# 
# # read_ascii10
#  -- any # of delimiters
# 
# write_creg(bv, fmt='b'|'x', delim=None)
# 
# class(CntrlReg):
#   value = BitVector()
#   addr = BitVector()
#   Name = ""
#------------------------------------------------------------

class SubReg:
  def __init__(self, name, lsbStart, nBits, pList):
    self.purposeName = name
    self.nBits = nBits
    self.lsbStart = lsbStart # position of LSBit in register
    self.purposeList = pList
    if pList!=None:
      self.pLenDefined = len(pList)
    else:
      self.pLenDefined = 0
  #---------------------------------------------------------
  def sreg_svalue(self, value):
    if value >= self.pLenDefined:
      sVal = "Arb/Unkwn/Rsvd"
    else:
      sVal = self.purposeList[value]
    return [self.purposeName, sVal]
#------------------------------------------------------------

class CfgRegister:
  def __init__(self, cfgDict, regAddr, value=None):
    self.regAddr = regAddr
    regEntry = cfgDict[regAddr]
    # [0]=regName [1]=defVal [2]=subList
    nBits = 16
    if value==None:
      value = regEntry[1] # default
    self.cfgDict = cfgDict
    self.bits = BitVector(nBits, value)
    self.uwMask = BitVector(nBits, 0) # initialize mask: RO
    self.dutMask = BitVector(nBits, 0) # initialize mask: RO
    subList = regEntry[2::]
    # subList = [[subName, lsbStart, nBits, wMask, [p1, ...]] [...] ]
    self.subRegs = [] # list of SubReg class objects
    for sub in subList:
      pList = sub[4]
      sReg = SubReg(sub[0], sub[1], sub[2], pList)
      self.dutMask.copybits(sub[3], sub[1], 0, sub[2])
      self.subRegs.append(sReg)
  #--------------------------------------------------------
  def wreg(self, value):
    if (value >= pow(2, self.bits.bvLen)):
      regEntry = self.cfgDict[self.regAddr]
      name = regEntry[0]
      raise Exception("%s: %d out of range"%(name, value))
    self.bits.value = value
  #--------------------------------------------------------
  def rreg(self, start, nBits):
    rVal = (self.bits.value >> start) & (pow(2, nBits)-1)
    return rVal
  #--------------------------------------------------------
  def reg_decode(self, fmt='b'):
    regEntry = self.cfgDict[self.regAddr]
    nameLine = regEntry[0] + '\tAddress\tuwMask\tdutMask'
    rVal = self.bits.value # '%d'%self.bits.value
    umVal = self.uwMask.value # '%d'%self.uwMask.value
    dmVal = self.dutMask.value # '%d'%self.dutMask.value
    fmt = fmt.lower()

    if fmt=='x':
      fLen = self.bits.bvLen/4
      valLine = ("%d'h"%self.bits.bvLen) + i2xs(rVal, fLen)
      valLine += '\t' + '0x%04x'%(self.regAddr)
      fLen = self.uwMask.bvLen/4
      valLine += '\t' + ("%d'h"%self.uwMask.bvLen) + i2xs(umVal, fLen)
      fLen = self.dutMask.bvLen/4
      valLine += '\t' + ("%d'h"%self.dutMask.bvLen) + i2xs(dmVal, fLen)
    else: # let binary
      fLen = self.bits.bvLen
      valLine = ("%d'b"%self.bits.bvLen) + i2bs(rVal, fLen)
      valLine += '\t' + '%d'%(self.regAddr)
      fLen = self.uwMask.bvLen
      valLine += '\t' + ("%d'b"%self.uwMask.bvLen) + i2bs(umVal, fLen)
      fLen = self.dutMask.bvLen
      valLine += '\t' + ("%d'b"%self.dutMask.bvLen) + i2bs(dmVal, fLen)
    for sReg in self.subRegs:
      # nameAdd = sReg.purposeName
      subVal = self.rreg(sReg.lsbStart, sReg.nBits) # purposeList[index]
      [nameAdd, valAdd] = sReg.sreg_svalue(subVal)
      nameLine += '\t' + nameAdd + '[%d:%d]'%(sReg.lsbStart+sReg.nBits-1, sReg.lsbStart)
      valLine += '\t' + valAdd
    return [nameLine, valLine]
#------------------------------------------------------------
  def reg_print(self, fmt='b'):
    [name, val] = self.reg_decode(fmt)
    print '%s'%name
    print '%s'%val
#------------------------------------------------------------
# 
# # Think about EnDecoder, csv file, write, read(into Excel)
#
#------------------------------------------------------------

def irand_lh(low=0, high=15):
  from random import random
  # low = int(low)
  # high = int(high)
  fRng = float(high-low+1)
  fVal = random()*fRng
  iVal = long(fVal)+long(low)
  return iVal
#------------------------------------------------------------

def bytrand(nByte=1):
  sVal = ''
  for k in range(2*nByte):
    sVal = sVal + "%1x"%(irand_lh())
  return '0x'+sVal
#----------------------------------------------------------
# verilog string:
# 12'b1011_0101_1100, 32'habCd, 10'd123
# 12'B1011100, 40'H1Ed_5, 3'D1_23
#
def vs2bv(vs): # verilog string to BitVector
  [nBits, vsVal] = vs2value(vs)
  bVec = BitVector(nBits, vsVal)
  return bVec
#------------------------------------------------------------
# Walking-1 matrix, with nRows x nCols, start/end column indices
#
def walk_1bit(nRows, nCols, begCol, endCol, invert=False):
  if nRows%2:
    raise Exception("Even |nRows| required")
  e2b = endCol-begCol
  nPer = nRows/(2*e2b)
  if 2*nPer*e2b != nRows: # suggest (e-b) possibilities here
    print("|walking_bits| should be %d, %d, ...1"%(nRows/2, nRows/4))
    raise Exception("Walk-1Bit period error")
  walkBitMx = BitMatrix(nRows, nCols)
  if invert:
    initVal = ~0x0L
    bVal = 0
  else:
    initVal = 0x0L
    bVal = 1
  initVec = BitVector(nCols, initVal)
  initVec.setbit(begCol, bVal)
  walkBitMx.bmRowHead[0].copybits(initVec.value)
  kRow = 1

  for kSeg in range(nPer):
    for kBit in range(begCol, endCol):
      rSrc = walkBitMx.bmRowHead[kRow-1]
      rDest = walkBitMx.bmRowHead[kRow]
      rDest.copybits(rSrc.value)
      bVal = rSrc.getbit(kBit)
      rDest.setbit(kBit, not bVal)
      rDest.setbit(kBit+1, bVal)
      kRow += 1
    for kBit in range(endCol, begCol, -1):
      rSrc = walkBitMx.bmRowHead[kRow-1]
      rDest = walkBitMx.bmRowHead[kRow]
      rDest.copybits(rSrc.value)
      bVal = rSrc.getbit(kBit)
      rDest.setbit(kBit, not bVal)
      rDest.setbit(kBit-1, bVal)
      kRow += 1
      if kRow==nRows: # done: last pt.==first by circular buffer
        return walkBitMx # maybe not good practice, but nested
  if kRow != nRows:
    raise Exception("Walk-1Bit prog. error")
  return walkBitMx
#------------------------------------------------------------
# Constant matrix
#
def const_bits(initVec, nRows):
  constMx = BitMatrix(nRows, initVec.bvLen)
  for kRow in range(nRows):
    # constMx.bmRowHead[kRow].copybits(initVec.value)
    constMx.bmRowHead[kRow] = initVec
  return constMx
#------------------------------------------------------------
# IID-PRBS matrix: prbs in columns & rows
#
def indep_prbs_bits(nRows, nCols):
  # nRows-long array of nCols-wide random bits
  prbsMx = BitMatrix(nRows, nCols)
  for kRow in range(nRows):
    for kCol in range(nCols):
      if irand_lh(0, 1):
        prbsMx.bmRowHead[kRow].setbit(kCol)
  return prbsMx
#------------------------------------------------------------
# SSO-PRBS matrix: identical columns, prbs wrt rows/clock
#
def sso_prbs_bits(nRows, nCols):
  ssoMx = BitMatrix(nRows, nCols)
  randVec = BitVector(nCols)
  for kCol in range(nCols):
    if irand_lh(0, 1):
      randVec.setbit(kCol)
  for kRow in range(nRows):
    # ssoMx.bmRowHead[kRow].copybits(randVec.value)
    ssoMx.bmRowHead[kRow] = randVec
  return ssoMx
#------------------------------------------------------------
# 1-bit switching matrix, with nRows, start/end column indices
#
def switch_1bit(initVec, nRows, begCol, endCol):
  if nRows%2:
    raise Exception("Even |nRows| required")
  e2b = endCol-begCol
  nPer = nRows/(2*(e2b+1))
  if 2*nPer*(e2b+1) != nRows: # suggest (e-b) possibilities here
    print("|switching_bits| should be %d, %d, ...1"%(nRows/2, nRows/4))
    raise Exception("1-BitSW period error")
  oneBitMx = BitMatrix(nRows, initVec.bvLen)
  oneBitMx.bmRowHead[0].copybits(initVec.value)
  kRow = 1

  for kSeg in range(nPer):
    for kBit in range(begCol, endCol+1):
      rSrc = oneBitMx.bmRowHead[kRow-1]
      rDest = oneBitMx.bmRowHead[kRow]
      rDest.copybits(rSrc.value)
      getBit = rSrc.getbit(kBit)
      rDest.setbit(kBit, not getBit)
      kRow += 1
    for kBit in range(endCol, begCol-1, -1):
      rSrc = oneBitMx.bmRowHead[kRow-1]
      rDest = oneBitMx.bmRowHead[kRow]
      rDest.copybits(rSrc.value)
      getBit = rSrc.getbit(kBit)
      rDest.setbit(kBit, not getBit)
      kRow += 1
      if kRow==nRows: # done: last pt.==first by circular buffer
        return oneBitMx # maybe not good practice, but nested
  if kRow != nRows:
    raise Exception("OneBitSw prog. error")
  return oneBitMx
#----------------------------------------------------------

def odd_parity(bVec, start, stop):
  sum = 0
  for k in range(start, stop+1):
    if bVec.getbit(k):
      sum += 1
  return sum%2
#----------------------------------------------------------
# Example application: STE882 specific registers
#
RC0 = [
  'RC0', [
    [ # RC0_0
      'RC0[DA3]', 0, 1, # name, lsbStart, nBits
      ['O/P Inversion:EN', 'O/P Inversion:DIS']
    ],
    [ # RC0_1
      'RC0[DA4]', 1, 1, # lsbStart, nBits
      ['O/P Float:EN', 'O/P Float:DIS']
    ],
    [ # RC0_2
      'RC0[DBA0]', 2, 1, # lsbStart, nBits
      ['O/P QxA:EN', 'O/P QxA:DIS']
    ],
    [ # RC0_3
      'RC0[DBA1]', 3, 1, # lsbStart, nBits
      ['O/P QxB:EN', 'O/P QxB:DIS']
    ],
  ]
]
RC6 = [
  'RC6', [
    [ # RC6_1_0
      'RC6[DA4:DA3]', 0, 2, # lsbStart, nBits
      [
        'Extra strong drive En', # 00
        'R/C D', # 01
        'R/C E', # 10
        'R/C F', # 11
      ]
    ]
  ]
]
RC13 = [
  'RC13', [
    [ # RC13_2_0
      'RC13[2:0]', 0, 3, # lsbStart, nBits
      [
        'Zout=14', # 000
        'Zout=17', # 001
        'Zout=19', # 010
        'Zout=25', # 011
        'Zout=30', # 100
        'Zout=50', # 101
        'Zout=75', # 110
        'Zout=off', # 111
      ]
    ],
    [ # RC13_3,
      'RC13[DBA1]', 3, 1, # lsbStart, nBits
      [
        'SlRate=3', # 0
        'SlRate=5' # 1
      ]
    ]
  ]
]
#----------------------------------------------------------------
# 36-bit control + command + address input "register"
CmdAddr = [
  'C/A_Reg', [
    [ # Row/Column Address
      'A[15:0]', 0, 16, # name, lsbStart, nBits
      [] # empty for arb. values
    ],
    [ # Bank Address
      'BA[2:0]', 16, 3, # lsbStart, nBits
      [] # empty for arb. values
    ],
    [ # WE/RAS/CAS commands
      'WE#_CAS#_RAS#', 19, 3, # lsbStart, nBits
      []
    ],
    [ # CS[3:0] controls
      'CS[3:0]', 22, 4, # lsbStart, nBits
      []
    ],
    [ # CKE commands
      'CKE[1:0]', 26, 2, # lsbStart, nBits
      []
    ],
    [ # ODT commands
      'ODT[1:0]', 28, 2, # lsbStart, nBits
      []
    ],
    [ # QCSEN control
      'QCSEN#', 30, 1, # lsbStart, nBits
      []
    ],
    [ # PAR check
      'PAR', 31, 1, # lsbStart, nBits
      []
    ],
    [ # MIR control
      'MIR', 32, 1, # lsbStart, nBits
      []
    ],
    [ # RST control
      'RST#', 33, 1, # lsbStart, nBits
      []
    ],
    [ # Rsvd
      'RSVD[1:0]', 34, 2, # lsbStart, nBits
      []
    ]
  ]
]
#------------------------------------------------------------
#
# Read a value into CfgRegister, from string
# 
def cfg2reg_882(cfgLine, rcReg):
  bvLine = BitVector(36, cfgLine) # new, local vector
  bv4 = BitVector(4) # compacted 4-bit local vector
  bv4.copybits(bvLine.value, 0, 3, 2) # [A4:A3]
  bv4.copybits(bvLine.value, 2, 16, 2) # [BA1:BA0]
  bv4.print_rbits()
  # now extract the bits from positions in register definition
  for sReg in rcReg.subRegs:
    rcReg.defVal.copybits(bv4.value, sReg.lsbStart, sReg.lsbStart, sReg.nBits)
#------------------------------------------------------------

def run():
  # 36-bit vector initialized to all 1s
  allOnes36 = BitVector(36, 0xfffffffff)
  print("--------- 36-bit all 1s ---------")
  allOnes36.print_rbits(40)

  # 36-bit vector with 1010 pattern
  pat1010 = BitVector(36, "36'xaaaa_aaaa_a")
  print("---- 36-bit 1010 pattern --------")
  pat1010.print_rhex(20)

  prbs_bmx = indep_prbs_bits(128, 36)
  print("---- 128 x 36-bit random pattern ----")
  prbs_bmx.print_bmx()

  zeros = BitVector(36)
  mx1BitSw = switch_1bit(zeros, 128, 0, 15)
  print("---- 128 x 36-bit 1-bit switch pattern, initial 0s ----")
  mx1BitSw.print_bmx()

  # 16-long array of 36-bit vectors, walking 1
  nWalk = 16
  walkOnes = []
  oneL = 1L
  for k in range(nWalk):
    walkOnes.append(BitVector(36, oneL))
    oneL = oneL << 1

  print("---- 16 x 36-bit walking 1s, non-periodic --------")
  for wVec in walkOnes:
    wVec.print_rbits(40)

  walk1_bmx = walk_1bit(128, 36, 0, 8)
  print("---- 128 x 36-bit walking-1 pattern ----")
  walk1_bmx.print_bmx()

  sso_bmx = sso_prbs_bits(10, 20)
  print("---- 10 x 20-bit SSO pattern ----")
  sso_bmx.print_bmx()
#------------------------------------------------------------
# 
#  Local Variables:
#  tab-width: 2
#  indent-tabs-mode: nil
#  scroll-step: 1
#  line-number-mode: t
#  column-number-mode: t
#  End:
# 
