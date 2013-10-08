# ------------------------------------------------------------------------------
#
# $RCSfile: validation#python#ipreg.py,v $ - Inphi's iPHY register class definitions
#
# Disclaimer:
# Inphi Confidential
# Copyright(c) Inphi Corp. 2011
#
# All rights reserved.
# This is unpublished, confidential Inphi proprietary information.
# Do not reproduce or redistribute without written permission.
#
# $Header: /projects/wlv/iphy/sos/repository/iphy_A.rep/LIBS/FILES258/SRC/validation#python#ipreg.py,v 1.3 2011-07-11 12:29:24-07 rbemra Exp $
# $Log: validation#python#ipreg.py,v $
# Revision 1.3  2011-07-11 12:29:24-07  rbemra
# Added lastRead, lastWrite attributes
#
# Revision 1.2  2011-07-05 18:43:10-07  rbemra
# Updates to get more things working: user scripts w/ options/threading, DDF+CSV writers, MDIO R/W updates, ipallreg addition
#
# Revision 1.1  2011-07-01 09:02:54-07  rbemra
# First, prelim. runnable version
#
# ------------------------------------------------------------------------------

import bv
import sys
from ordict import *

# ------------------------------------------------------------------------------

class SubReg:
  def __init__(self, name, lsbStart, nBits, bitMask, pTab, subStr=None):
    self.purposeName = name
    self.nBits = nBits
    self.lsbStart = lsbStart # position of LSBit in register
    self.bitMask = bitMask.strip() if type(bitMask)==str else bitMask
    self.purposeTab = pTab
    self.subStr = subStr
    #  self.pLenDefined = len(pList)
  #---------------------------------------------------------
  def sub_value(self, value):
    try:
      sVal = self.purposeTab[value]
    except:
      sVal = "Arb/Unkwn/Rsvd"
    return [self.purposeName, sVal]
  #---------------------------------------------------------
  def sub_print(self, oFile=sys.stdout):
    pass
#------------------------------------------------------------

REG_LEN = 16

class RegisterDef:
  def __init__(self, name, devAd, prtAd, regAd, defVal, rSubDict=None, desStr=None):
    self.regName = name
    self.regAddr = regAd
    self.devAddr = devAd
    self.prtAddr = prtAd
    nBits = REG_LEN
    self.defVec = bv.BitVector(nBits, defVal) # default
    rMask = bv.BitVector(nBits, 0)
    rMask.setbits(0, nBits, True)
    self.rMask = rMask # initialize Read mask: readable
    self.wMask = bv.BitVector(nBits, 0) # initialize mask: not writable
    self.scanVec = bv.BitVector(nBits, 0) # initialize all zeros?
    if rSubDict!=None:
      self.regSubTab = rSubDict
    else:
      self.regSubTab = OrderedDict()
    self.desStr = desStr
    self.lastWrite = 0
    self.lastRead = 0xFFFF
  #------------------------------------------------------------
  # R8.0,PMA Control1,"16'h204c",
  # 8.0.15,Reset,RW,0=normal; 1=reset;
  # 8.0.14,Reserved,R
  # 8.0.13,Speed selection (LSB),R
  # 8.0.12,Reserved,R
  # 8.0.11,Low Power,RW
  # 8.0.10:7,Reserved,R
  # 8.0.6,Speed selection (MSB),R
  # 8.0.5:2,Speed selection,R
  # 8.0.1,PMA remote loopback,RW,0=disable; 1=enable
  # 8.0.0,PMA local loopback,RW,0=disable; 1=enable
  # 
  def write_reg(self, oFile, doScan=True, valFmt='x'):
    if doScan:
      regVal = self.scanVec.value
    else:
      regVal = self.defVec.value
    if valFmt=='x':
      valStr = hex(regVal).rstrip('L')
    elif valFmt=='b':
      valStr = bin(regVal)[2::]
    else: # decimal
      valStr = str(regVal)
    oFile.write(self.regName + ',' + valStr)
    if self.desStr!=None and self.desStr!='':
      oFile.write(',,"' + self.desStr + '"') # write it as 1 d-quoted string in 5th column
    for (subRegAddr, subReg) in self.regSubTab.items():
      oFile.write("\n%s,%s,%s,"%(subRegAddr, subReg.purposeName, subReg.bitMask))
      for (patVal, patDes) in subReg.purposeTab.items():
        oFile.write("%s=%s;"%(bin(patVal)[2::], patDes))
      if subReg.subStr!=None and subReg.subStr!='':
        oFile.write(',"' + subReg.subStr + '"')
    oFile.write("\n")
    oFile.flush()
  #---------------------------------------------------------------------
  def write_ddf_reg(self, regKey, oFile):
    regBitMap = {'R':0b0001, 'RW':0b0010, 'RC':0b0100, 'W':0b1000}
    ddfTypeMap = {0b0000:'RSV', 0b0001:'RO', 0b0010:'RW', 0b0011:'RW',
                  0b0100: 'RO', 0b0101:'RO', 0b0110:'RW', 0b0111:'RW',
                  0b1000: 'WO', 0b1001:'RW', 0b1010:'RW', 0b1011:'RW',
                  0b1100: 'RW', 0b1101:'RW', 0b1110:'RW', 0b1111:'RW'}
    oFile.write('// Vendor Specific: ' + regKey + '\n')
    oFile.write('[REGISTER]\n')
    oFile.write(' [REG NAME] "%s"\n' % self.regName)
    oFile.write(' [REG SIZE] 0x02\n')
    oFile.write(' [REG ADDR] 0x%04X\n' % self.regAddr)
    ddfBits = 0b0000
    for (subRegAddr, subReg) in self.regSubTab.items():
      ddfBits |= regBitMap[subReg.bitMask]
    oFile.write(' [REG TYPE] "%s"\n\n' % ddfTypeMap[ddfBits])
    oFile.flush()
  #---------------------------------------------------------------------
  # send register's value to chip
  def mdio_wreg(self, doScan=True):
    wVal = self.scanVec.value if doScan else self.defVec.value
    ipmdio.mdio_wr(self.prtAddr, self.devAddr, self.regAddr, wVal)
  #---------------------------------------------------------------------
  # get register's value from chip
  def mdio_rreg(self, doUpdate=True):
    rVal = ipmdio.mdio_rd(self.prtAddr, self.devAddr, self.regAddr)
    if doUpdate:
      self.scanVec.value = rVal
    return rVal
  #--------------------------------------------------------
  def reg_decode(self, fmt='b'):
    nameLine = self.rDefList[0] + '\tAddress\tuwMask' # '\tdutMask'
    rVal = self.defVal.value # '%d'%self.defVal.value
    dmVal = self.dutMask.value # '%d'%self.dutMask.value
    fmt = fmt.lower()

    if fmt=='x':
      fLen = self.defVal.bvLen/4
      valLine = ("%d'h"%self.defVal.bvLen) + i2xs(rVal, fLen)
      valLine += '\t' + '0x%04x'%(self.regAddr)
      fLen = self.dutMask.bvLen/4
      valLine += '\t' + ("%d'h"%self.dutMask.bvLen) + i2xs(dmVal, fLen)
    else: # let binary
      fLen = self.defVal.bvLen
      valLine = ("%d'b"%self.defVal.bvLen) + i2bs(rVal, fLen)
      valLine += '\t' + '%d'%(self.regAddr)
      fLen = self.dutMask.bvLen
      valLine += '\t' + ("%d'b"%self.dutMask.bvLen) + i2bs(dmVal, fLen)
    for sReg in self.subRegs:
      # nameAdd = sReg.purposeName
      subVal = self.rreg(sReg.lsbStart, sReg.nBits) # purposeList[index]
      [nameAdd, valAdd] = sReg.sreg_svalue(subVal)
      nameLine += '\t' + nameAdd + '[%d:%d]'%(sReg.lsbStart+sReg.nBits-1, sReg.lsbStart)
      valLine += '\t' + valAdd
    return [nameLine, valLine]
#---------------------------------------------------------------------

class DeviceDef:
  def __init__(self, devId, devName, prtAd, regTab=None):
    self.devId = devId
    self.devName = devName
    self.prtAddr = prtAd
    if regTab!=None:
      self.regDefTab = regTab
    else:
      self.regDefTab = OrderedDict()
  #-------------------------------------------------------------------
  def write_ddf_head(self, devKey, oFile):
    oFile.write('// %s\n' % oFile.name)
    oFile.write('\
// This is the device definition file (DDF) for an iPHY-defined MDIO\n\
// clause 45 device.\n\
//\n')
    oFile.write('[DEVICE]\n')
    oFile.write(' [NAME] "%s"\n' % devKey)
    oFile.write(' [DESC] "%s"\n' % self.devName)
    oFile.write(' [TYPE] "MDIO_45"\n')
    oFile.flush()
  #-------------------------------------------------------------------
  # write DDF MDIO_45 definition file
  def write_ddf_device(self, devKey, ddfFileName=None):
    import sys
    if ddfFileName==None:
      oFile = sys.stdout
    else:
      try:
        oFile = open(ddfFileName, 'w')
      except:
        sys.stderr.write("Error writing %s: %s\n" % (ddfFileName, sys.exc_info()[0]))
        return
    self.write_ddf_head(devKey, oFile)
    for (regKey, regDef) in self.regDefTab.items():
      regDef.write_ddf_reg(regKey, oFile)
    if oFile!=sys.stdout:
      oFile.close()
  #-------------------------------------------------------------------
  # write iPHY-CSV-format definition/reg_state file
  def write_csv_device(self, doScan=True, valFmt='x', csvFileName=None):
    import sys
    if csvFileName==None:
      oFile = sys.stdout
    else:
      try:
        oFile = open(csvFileName, 'w')
      except:
        sys.stderr.write("Error writing %s: %s\n" % (csvFileName, sys.exc_info()[0]))
        return
    for (regKey, regDef) in self.regDefTab.items():
      oFile.write("R" + regKey + ',')
      regDef.write_reg(oFile, doScan, valFmt)
    if oFile!=sys.stdout:
      oFile.close()
  #-------------------------------------------------------------------
  #def load_regs(self, filename):
  #  return ipconfig.load_regdefs_from_csv(self, filename)
#---------------------------------------------------------------------
# 
#  Local Variables:
#  tab-width: 2
#  indent-tabs-mode: nil
#  scroll-step: 1
#  line-number-mode: t
#  column-number-mode: t
#  End:
# 
