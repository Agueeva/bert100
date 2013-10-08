# ------------------------------------------------------------------------------
#
# $RCSfile: validation#python#ipconfig.py,v $ - iPHY register definitions CSV file reader, writer
#
# Disclaimer:
# Inphi Confidential
# Copyright(c) Inphi Corp. 2011
#
# All rights reserved.
# This is unpublished, confidential Inphi proprietary information.
# Do not reproduce or redistribute without written permission.
#
# $Header: /projects/wlv/iphy/sos/repository/iphy_A.rep/LIBS/FILES258/SRC/validation#python#ipconfig.py,v 1.2 2011-07-05 18:43:12-07 rbemra Exp $
# $Log: validation#python#ipconfig.py,v $
# Revision 1.2  2011-07-05 18:43:12-07  rbemra
# Updates to get more things working: user scripts w/ options/threading, DDF+CSV writers, MDIO R/W updates, ipallreg addition
#
# Revision 1.1  2011-07-01 09:02:53-07  rbemra
# First, prelim. runnable version
#
# ------------------------------------------------------------------------------

import bv
from ipreg import *

#-------------------------------------------------------------------------------

# charge-pump current 30.41.11:9
# "000=lowest current; 001=001; 010=010; 011=011; 100=100; 101=101; 110=110; 111=highest current"
# vctrlref 30.42.15:14
# " 00= 0.4*VDDA_1_0;  01= 0.45*VDDA_1_0; 01= 0.45*VDDA_1_0= 10: 0.5*VDDA_1_0= 11: 0.55*VDDA_1_0"
# txa_eqpst 30.257.10:8
# "000 = 0.00; 001 = 0.05; 010 = 0.10; 011 = 0.15; 100 = 0.20; 101 = 0.25; 110 = 0.30; 111 = 0.35;"

#-------------------------------------------------------------------------------

def parse_bpat_des(bdStr, pTab): # bit-patterns -> descriptor table
  bvList = bdStr.split(';')
  for assignStr in bvList:
    assignList = assignStr.split('=')
    if len(assignList)<2:
      continue
    try:
      patVal = int(bv.bs2is(assignList[0].strip()))
    except:
      continue # ignore if cannot reckon int
    pTab[patVal] = assignList[1].strip()
    # print "%s -> %s" % (bin(patVal)[2::], pTab[patVal])
#-------------------------------------------------------------------------------

def strip_comment(lineList):
  COMMENT = '!' # A field/xls cell starting with '!' implies beginning of comment
  lenList = len(lineList)
  for kItem in range(lenList):
    if (lineList[kItem].find('!') == 0): # start of commet
      break
  if kItem==0:
    retList = []
  elif kItem==lenList:
    retList = lineList # no comment found
  else:
    retList = lineList[0:kItem]
  return retList
#-------------------------------------------------------------------------------

def load_regdefs_from_csv(devTab, filename):
  import csv
  regDefTab = devTab.regDefTab
  try:
    fid = open(filename, 'r')
  except:
    print "IOError: %s for %s not accessible ... skipping" % (filename, devTab.devName)
    return
  insideRegDef = None # 'ZZZ'
  csvObj = csv.reader(fid)
  for lineList in csvObj: # iterator
    lineList = strip_comment(lineList)
    if len(lineList)==0:
      continue
    if (lineList[0].find('#') == 0): # # header line
      continue
    if (lineList[0].find('R') == 0): # 'R8.0', 'PMA Control1', 16'h204C, '', ''
      lineList[0] = lineList[0][1:] # Remove the leading 'R'
      regLineList = lineList
      nItems = len(regLineList)
      insideRegDef = regLineList[0] # '8.0'
      # build RegisterDef
      addrList = insideRegDef.split('.')
      devAd = int(addrList[0])
      regAd = int(addrList[1])
      defVal = regLineList[2].strip('"')
      defVal = defVal.strip("'")
      [_, regDefVal] = bv.vs2value(defVal)
      rDefList = [regLineList[1], regDefVal, []] # regAddr, ['Reg. Name', defVal, emptySubRegList]
      desStr = None
      if nItems>4 and regLineList[4]!='': # skip 4th column == regLineList[3]
        desStr = regLineList[4]
      regDef = RegisterDef(regLineList[1], devAd, devTab.prtAddr, regAd, regDefVal, None, desStr)
      regDefTab[insideRegDef] = regDef
    elif (insideRegDef!=None and
          lineList[0].find(insideRegDef)>=0): # '8.0.15', 'Reset', 'RW', '0=normal; 1=reset;', "Asserting ..."
      #print "Found @",line.find(insideRegDef),line.split(',')[0],'inside',insideRegDef
      subLineList = lineList # line.split(',')
      nItems = len(subLineList)
      subRegAddr = subLineList[0]
      subAddrList = subRegAddr.split('.')
      # print subAddrList
      subEndBegin = map(int, subAddrList[2].split(':')) # '8.0.10:7' -> [10, 7]
      if len(subEndBegin)>1:
        lsbStart = subEndBegin[1]
        nBits = subEndBegin[0] - lsbStart + 1
      else:
        lsbStart = subEndBegin[0]
        nBits = 1
      # tbd: get self.rMask, self.wMask from subLineList[1]
      bitMask = subLineList[2].upper()
      # parse description & build the purposeList
      pTab = OrderedDict()
      subDes = None
      if nItems>3: # use 4th entry only for 'pat0=des0;pat1=des1'
        parse_bpat_des(subLineList[3], pTab) # bit-patterns -> descriptor table
        subDes = subLineList[4] if (nItems>4 and subLineList[4]!='') else None
      subReg = SubReg(subLineList[1], lsbStart, nBits, bitMask, pTab, subDes)
      regDef.regSubTab[subRegAddr] = subReg
    else:
      insideRegDef = None # 'ZZZ'
  fid.close()
  devTab.regDefTab = regDefTab
  return True
#-------------------------------------------------------------------------------

def store_regs_to_csv(filename, regDict, doScan=True, valFmt='x'):
  oFile = open(filename, 'w')
  for (regKey, regDef) in regDict.items():
    oFile.write("R" + regKey + ',')
    regDef.write_reg(oFile, doScan, valFmt)
  oFile.close()
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
