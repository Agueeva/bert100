#
# $Header: /projects/wlv/iphy/sos/repository/iphy_A.rep/TEMP/case_1285613542/#alidation#iphy-gui#gui#lib#utils#regdict.py,v 1.1 2011-09-02 11:44:09-07 case Exp $
# Old Header: /inphi/882LV_A/validation/python/utils/regdefs.py,v 1.2 2008/09/25 23:13:29 rbemra Exp
# $Log: #alidation#iphy-gui#gui#lib#utils#regdict.py,v $
# Revision 1.1  2011-09-02 11:44:09-07  case
# ...No comments entered during checkin...
#
# Revision 1.1  2011-05-04 18:10:33-07  rbemra
# Initial python+pyvisa for instrumentation (from ExacTik/882)
#
# Revision 1.2  2008/09/25 23:13:29  rbemra
# Fix in cfg3reg_882(); added examples of cfg register set from row vector.
#
# Revision 1.1  2008/09/10 18:57:03  rbemra
# Initial Version
#
#
# STE882 register definitions
# Should probably be moved to var.py
#
RC0 = [
    [ # RC0_0
      'RC0[DA3] O/P Inv', 0, 1, # name, lsbStart, nBits
      ['O/P Inversion:EN', 'O/P Inversion:DIS']
    ],
    [ # RC0_1
      'RC0[DA4] O/P Float', 1, 1, # lsbStart, nBits
      ['O/P Float:EN', 'O/P Float:DIS']
    ],
    [ # RC0_2
      'RC0[DBA0] QnA En/Dis', 2, 1, # lsbStart, nBits
      ['O/P QAn:EN', 'O/P QAn:DIS']
    ],
    [ # RC0_3
      'RC0[DBA1] QnB En/Dis', 3, 1, # lsbStart, nBits
      ['O/P QBn:EN', 'O/P QBn:DIS']
    ]
]
RC1 = [
    [
      'RC1[DA3]', 0, 1,
      ['Y0 EN', 'Y0 DIS']
    ],
    [
      'RC1[DA4]', 1, 1,
      ['Y1 EN', 'Y1 DIS']
    ],
    [
      'RC1[DBA0]', 2, 1,
      ['Y2 EN', 'Y2 DIS']
    ],
    [
      'RC1[DBA1]', 3, 1,
      ['Y3 EN', 'Y3 DIS']
    ]
]
RC2 = [
    [
      'RC2[DA3] PreLaunch', 0, 1,
      ['2Q', '3Q']
    ],
    [
      'RC2[DA4]', 1, 1,
      ['1T', '3T']
    ],
    [
      'RC2[DBA0] IBT', 2, 1,
      ['IBT:100', 'IBT:150']
    ],
    [
      'RC2[DBA1]', 3, 1,
      ['fBand1', 'fBand2']
    ]
]
RC3 = [
    [
      'RC3[DA4:DA3]', 0, 2,
      [
        'QAn: CA Light', # 00
        'QAn: CA Moderate', # 01
        'QAn: CA Strong', # 10
        'see RC6' # 11
      ]
    ],
    [
      'RC3[DBA1:DBA0]', 2, 2,
      [
        'QBn: CA Light', # 00
        'QBn: CA Moderate', # 01
        'QBn: CA Strong', # 10
        'see RC6' # 11
      ]
    ]
]
RC4 = [
    [
      'RC4[DA4:DA3]', 0, 2,
      [
        'QAn: Ctrl Light', # 00
        'QAn: Ctrl Moderate', # 01
        'QAn: Ctrl Strong', # 10
        'see RC6' # 11
      ]
    ],
    [
      'RC4[DBA1:DBA0]', 2, 2,
      [
        'QnB: Ctrl Light', # 00
        'QnB: Ctrl Moderate', # 01
        'QnB: Ctrl Strong', # 10
        'see RC6' # 11
      ]
    ]
]
RC5 = [
    [
      'RC5[DA4:DA3]', 0, 2,
      [
        'Y1,Y3 Light', # 00
        'Y1,Y3 Moderate', # 01
        'Y1,Y3 Strong', # 10
        'see RC6' # 11
      ]
    ],
    [
      'RC5[DBA1:DBA0]', 2, 2,
      [
        'Y0,Y2 Light', # 00
        'Y0,Y2 Moderate', # 01
        'Y0,Y2 Strong', # 10
        'see RC6' # 11
      ]
    ]
]
RC6 = [
    [ # RC6_1_0
      'RC6[DA4:DA3] Rawcard ID', 0, 2, # lsbStart, nBits
      [
        'Extra strong drive En', # 00
        'R/C D', # 01
        'R/C E', # 10
        'R/C F', # 11
      ]
    ]
]
RC7 = [
    [
      'RC7[DA3] RefClk Delay', 0, 1,
      ['Normal', '100ps less tSU']
    ],
    [
      'RC7[DA4] PLL Bypass', 1, 1,
      ['En', 'Dis']
    ],
    [
      'RC7[DBA0] Parity', 2, 1,
      ['Normal', 'Dis']
    ],
    [
      'RC7[DBA1] Inphi ExtReg', 3, 1,
      ['Dis', 'En']
    ]
]
RC8 = [
    [
      'RC8[2:0] IBT Value', 0, 2,
      [
        'Use RC2', # 000
        '200', # 001
        '200', # 010
        '200', # 011
        '300', # 100
        '300', # 101
        '300', # 110
        'IBT off', # 111
      ]
    ],
    [
      'RC8[DBA1] IBT in Mirror', 3, 1,
      ['IBT off', 'IBT on']
    ]
]
RC9 = [
    [
      'RC9[DA3] Weak Drive', 0, 1,
      ['Normal', 'En']
    ],
    [
      'RC9[DBA0] CKE PwrDn', 2, 1,
      ['IBT On', 'IBT Off, QODT low']
    ],
    [
      'RC9[DBA1] CKE PwrDn En', 3, 1,
      ['En', 'Dis']
    ]
]
RC10 = [
    [
      'RC10[3:0]', 0, 4,
      []
    ]
]
RC11 = [
    [
      'RC11[3:0]', 0, 4,
      []
    ]
]
RC12 = [
    [
      'RC12[DA4:DA3] Eqlzn Level', 0, 2,
      [
        'Off', # 00
        '100 Ohms', # 01
        '60 Ohms', # 10
        '40 Ohms' # 11
      ]
    ],
    [
      'RC12[DBA1:DBA0] Eqlzn Time', 2, 2,
      [
        '85 ns', # 00
        '1 ns', # 01
        '1.15 ns', # 10
        '1.3 ns' # 11
      ]
    ]
]
RC13 = [
    [ # RC13_2_0
      'RC13[2:0] Zout', 0, 3, # lsbStart, nBits
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
RC14 = [
    [
      'RC14[DA3] PLL Backup Bias Enable', 0, 1,
      ['Off', 'Dis']
    ],
    [
      'RC14[DA4] Direct Tune Enable', 1, 1,
      ['Off', 'En']
    ],
    [
      'RC14[DBA0] Force Bypass', 2, 1,
      ['Off', 'Dis']
    ],
    [
      'RC14[DBA1] Standby Disable', 3, 1,
      ['Off', 'Dis']
    ]
]
RC15 = [
    [
      'RC15[DA4:DA3] Bandwidth', 0, 2,
      [
        '16MHz', # 00
        '24MHz', # 01
        '30MHz', # 10
        '38MHz' # 11
      ]
    ],
    [
      'RC15[DBA0] PLL Lock Detector Disable', 2, 1,
      ['En', 'Dis']
    ],
    [
      'RC15[DBA1] Register Read', 3, 1,
      ['Dis', 'En']
    ]
]
#----------------------------------------------------------------
# For convenience in loop iterations:
RegDefList = {
  'RC0':RC0, 'RC1':RC1, 'RC2':RC2, 'RC3':RC3,
  'RC4':RC4, 'RC5':RC5, 'RC6':RC6, 'RC7':RC7,
  'RC8':RC8, 'RC9':RC9, 'RC10':RC10, 'RC11':RC11,
  'RC12':RC12, 'RC13':RC13, 'RC14':RC14, 'RC15':RC15
}

#----------------------------------------------------------------
# 36-bit control + command + address input "register"
CmdAddr = [
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
CmdAddrRegList = {'C/A_Reg': CmdAddr}
#------------------------------------------------------------

# from pylab.utils import bv
import bv

#------------------------------------------------------------
# Return a STE882 CfgRegister instance, with given value
#
# TODO: WIP: NEEDS WORK: 3/21/2011
#
def make_reg(rDef, value=0L):
  subList = []
  for sDef in rDef[1]:
    sReg = bv.SubReg(sDef[0], sDef[1], sDef[2], sDef[3])
    subList.append(sReg)
  mReg = bv.CfgRegister(rDef[0], subList, value)
  return mReg
#------------------------------------------------------------
# Example use of CfgRegister
#
def rcreg():
  rc0 = make_reg(RC0, 0x1)
  rc6 = make_reg(RC6, 0x3)
  rc13 = make_reg(RC13, 0x5)
  caReg = make_reg(CmdAddr, "36'habcd")
  rc0.reg_print()
  rc6.reg_print()
  rc13.reg_print()
  rc13.reg_print('x')
  caReg.reg_print('x')
#------------------------------------------------------------
#
# Read a value into CfgRegister, from string
# 
def cfg2reg_882(cfgLine, cRegList):
  bvLine = bv.BitVector(36, cfgLine) # new, local vector
  bv4 = bv.BitVector(4) # compacted 4-bit local vector
  bv4.copybits(bvLine, 0, 3, 2) # [A4:A3]
  bv4.copybits(bvLine, 2, 16, 2) # [BA1:BA0]
  # bv4.print_rbits()

  # find register addressed in string
  addrVec = bv.BitVector(4)
  addrVec.copybits(bvLine, 0, 0, 3) # first 3: [DA2:DA0]: [2:0] <- [2:0]
  addrVec.copybits(bvLine, 3, 18, 1) # [DBA2]: [18] <- [3]
  regIndex = int(addrVec.value)
  # print 'regIndex=%d\n'%regIndex
  cReg = cRegList[regIndex]
  cReg.bits.copybits(bv4)

#------------------------------------------------------------
#
# Define & return array of 16 regs for 882 with default (0x0) settings
#
def make_default_regs():
  r882List = []
  for rc in RegDefList:
    reg = make_reg(rc, 0x0)
    r882List.append(reg)
  return r882List
#------------------------------------------------------------
# Test code
#
def run():
  cRegs = make_default_regs()

  # for reg in cRegs:
  #   reg.reg_print()

  # RC0=0011, inversion disabled, o/p float dis
  # RC1=0110, Y1, Y2 disabled
  # RC6=, R/C D
  # 0_0p = "36'b0000_0000_0000_ss00_0Arr_0000_0000_000r_rAAA"
  RC0_0p = "36'b0000_0000_0000_0000_0000_0000_0000_0001_1000"
  RC1_0p = "36'b0000_0000_0000_0000_0001_0000_0000_0001_0001"
  RC6_0p = "36'b0000_0000_0000_0000_0000_0000_0000_0000_1110"

  cfg2reg_882(RC0_0p, cRegs)
  cfg2reg_882(RC1_0p, cRegs)
  cfg2reg_882(RC6_0p, cRegs)

  print '------------------------------------------'

  cRegs[0].reg_print()
  cRegs[1].reg_print()
  cRegs[6].reg_print()

  print '------------------------------------------'
  # for reg in cRegs:
  #   reg.reg_print()
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
