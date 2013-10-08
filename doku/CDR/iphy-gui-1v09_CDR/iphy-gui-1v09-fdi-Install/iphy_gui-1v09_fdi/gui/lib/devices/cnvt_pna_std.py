#
# $Header: /projects/wlv/iphy/sos/repository/iphy_A.rep/TEMP/case_1285613542/#alidation#iphy-gui#gui#lib#devices#cnvt_pna_std.py,v 1.1 2011-09-02 11:43:55-07 case Exp $
# $Log: #alidation#iphy-gui#gui#lib#devices#cnvt_pna_std.py,v $
# Revision 1.1  2011-09-02 11:43:55-07  case
# ...No comments entered during checkin...
#
# Revision 1.1  2011-05-04 18:10:36-07  rbemra
# Initial python+pyvisa for instrumentation (from ExacTik/882)
#
#
# Convert PNA mixed S-params to std S4P in Touchstone fmt
# 3 parts:
#   1. parser of each freq. matrix into complex type 2-d list:
#     CxMat == [[a11, a12 .. a1N], [a21, a22, ..], .. [aN1, aN2, .. aNN]]
#   2. convert CxMat from PNA order to true mixed-mode order
#   3. convert mixed-mode to std SE s4p
#   4. write (freq, std SE CxMat) in Touchstone fmt
# While 2->4 can be done directly, mm <-> std methods are useful to have
#-------------------------------------------------------------------------------
#
# convert PNA form:
#   [bd1]   [Sdd11 Sdc11 Sdd12 Sdc12] [ad1]
#   [bc1] = [Scd11 Scc11 Scd12 Scc12] [ac1]
#   [bd2]   [Sdd21 Sdc21 Sdd22 Sdc22] [ad2]
#   [bc2]   [Scd21 Scc21 Scd22 Scc22] [ac2]
# to mm:
#   [bd1]   [Sdd11 Sdc11 Sdd12 Sdc12] [ad1]
#   [bd2] = [Scd11 Scc11 Scd12 Scc12] [ad2]
#   [bc1]   [Sdd21 Sdc21 Sdd22 Sdc22] [ac1]
#   [bc2]   [Scd21 Scc21 Scd22 Scc22] [ac2]
#

from copy import deepcopy

def m4x4_pna2mm(cxMat):
  # swap rows (2, 3), then cols (2, 3)
  r1 = cxMat[1] # deepcopy(cxMat[1])
  r2 = cxMat[2] # deepcopy(cxMat[2])
  cxMat[2] = r1 # deepcopy(r1)
  cxMat[1] = r2 # deepcopy(r2)
  for kRow in range(4):
    zHold = cxMat[kRow][1]
    cxMat[kRow][1] = cxMat[kRow][2]
    cxMat[kRow][2] = zHold
#-------------------------------------------------------------------------------
#
# convert mm to std:
# mm:
#   [bd1]   [Sdd11 Sdc11 Sdd12 Sdc12] [ad1]
#   [bd2] = [Scd11 Scc11 Scd12 Scc12] [ad2]
#   [bc1]   [Sdd21 Sdc21 Sdd22 Sdc22] [ac1]
#   [bc2]   [Scd21 Scc21 Scd22 Scc22] [ac2]
# to std:
#   [b1]   1  [(m11+m13+m31+m33) (m13-m11+m33-m31) (m12+m14+m32+m34) (m14-m12+m34-m32)] [a1]
#   [b2] = -  [(m31+m33-m11-m13) (m33-m31+m11-m13) (m32+m34-m12-m14) (m34-m32+m12-m14)] [a2]
#   [b3]   2  [(m21+m23+m41+m43) (m23-m21+m43-m41) (m22+m24+m42+m44) (m24-m22+m44-m42)] [a3]
#   [b4]      [(m41+m43-m21-m23) (m43-m41+m21-m23) (m42+m44-m22-m24) (m44-m42+m22-m24)] [a4]
#
def m4x4_do_std(iM):
  k1, k2, k3, k4 = 0, 1, 2, 3 # easy readability
  oM = deepcopy(iM)
  iM[k1][k1] = 0.5 *(oM[k1][k1] + oM[k1][k3] + oM[k3][k1] + oM[k3][k3]) # (m11+m13+m31+m33)
  iM[k1][k2] = 0.5 *(oM[k1][k3] - oM[k1][k1] + oM[k3][k3] - oM[k3][k1]) # (m13-m11+m33-m31)
  iM[k1][k3] = 0.5 *(oM[k1][k2] + oM[k1][k4] + oM[k3][k2] + oM[k3][k4]) # (m12+m14+m32+m34)
  iM[k1][k4] = 0.5 *(oM[k1][k4] - oM[k1][k2] + oM[k3][k4] - oM[k3][k2]) # (m14-m12+m34-m32)

  iM[k2][k1] = 0.5 *(oM[k3][k1] + oM[k3][k3] - oM[k1][k1] - oM[k1][k3]) # (m31+m33-m11-m13)
  iM[k2][k2] = 0.5 *(oM[k3][k3] - oM[k3][k1] + oM[k1][k1] - oM[k1][k3]) # (m33-m31+m11-m13)
  iM[k2][k3] = 0.5 *(oM[k3][k2] + oM[k3][k4] - oM[k1][k2] - oM[k1][k4]) # (m32+m34-m12-m14)
  iM[k2][k4] = 0.5 *(oM[k3][k4] - oM[k3][k2] + oM[k1][k2] - oM[k1][k4]) # (m34-m32+m12-m14)

  iM[k3][k1] = 0.5 *(oM[k2][k1] + oM[k2][k3] + oM[k4][k1] + oM[k4][k3]) # (m21+m23+m41+m43)
  iM[k3][k2] = 0.5 *(oM[k2][k3] - oM[k2][k1] + oM[k4][k3] - oM[k4][k1]) # (m23-m21+m43-m41)
  iM[k3][k3] = 0.5 *(oM[k2][k2] + oM[k2][k4] + oM[k4][k2] + oM[k4][k4]) # (m22+m24+m42+m44)
  iM[k3][k4] = 0.5 *(oM[k2][k4] - oM[k2][k2] + oM[k4][k4] - oM[k4][k2]) # (m24-m22+m44-m42)

  iM[k4][k1] = 0.5 *(oM[k4][k1] + oM[k4][k3] - oM[k2][k1] - oM[k2][k3]) # (m41+m43-m21-m23)
  iM[k4][k2] = 0.5 *(oM[k4][k3] - oM[k4][k1] + oM[k2][k1] - oM[k2][k3]) # (m43-m41+m21-m23)
  iM[k4][k3] = 0.5 *(oM[k4][k2] + oM[k4][k4] - oM[k2][k2] - oM[k2][k4]) # (m42+m44-m22-m24)
  iM[k4][k4] = 0.5 *(oM[k4][k4] - oM[k4][k2] + oM[k2][k2] - oM[k2][k4]) # (m44-m42+m22-m24)
#-------------------------------------------------------------------------------

def m4x4_print(freqVal, CxMat):
  rowStrg = "%e"%freqVal
  for kRow in range(4):
    for kCol in range(4):
      rowStrg = rowStrg.__add__(" %e %e"%(CxMat[kRow][kCol].real, CxMat[kRow][kCol].imag))
    print(rowStrg)
    rowStrg = ""
#-------------------------------------------------------------------------------

def run(s4pFile):
  from copy import deepcopy
  CxNum = complex(0., 0.)
  CxRow = [CxNum, CxNum, CxNum, CxNum]
  CxMat = [deepcopy(CxRow), deepcopy(CxRow), deepcopy(CxRow), deepcopy(CxRow)]
  snpObj = open(s4pFile, 'r')
  kRow = -1 # 0 - 3 for each line of matrix
  lNum = 0
  for line in snpObj:
    line = line.strip(' \t,\r\n')
    lNum += 1
    if line[0]=='!':
      continue # comment
    elif line[0]=='#':
      print line # format line
      continue
    kRow += 1
    sList = line.split()
    len = sList.__len__()
    if kRow==0:
      if len<9:
        raise "line %d: Frequency line needs 9 values"%lNum
      freqVal = float(sList[0])
      kStart = 1
    else:
      if len<8:
        raise "line %d: Data line needs 8 values"%lNum
      kStart = 0
    for kCol in range(kStart, kStart+8, 2):
      CxMat[kRow][kCol/2] = complex(float(sList[kCol]), float(sList[kCol+1]))
    if kRow<3:
      continue
    # ok, we now have full 4x4 complex matrix
    m4x4_pna2mm(CxMat)
    m4x4_do_std(CxMat)
    m4x4_print(freqVal, CxMat)
    kRow = -1 # reset
  snpObj.close()
#-------------------------------------------------------------------------------

# s4pFile = "TT4_Room_Vdd1001_Eq14.s4p"
# s4pFile = "TT6_T025_Vdd0950_Eq00.s4p"
# s4pFile = "test_pna_std.s4p"
# run(s4pFile)

import sys

if __name__ == "__main__":
  if sys.argv[1:]:
    s4pFile = sys.argv[1]
    run(s4pFile)
#
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
