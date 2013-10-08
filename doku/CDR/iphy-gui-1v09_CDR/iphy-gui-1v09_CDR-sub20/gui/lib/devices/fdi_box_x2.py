#
# $Header: /projects/wlv/iphy/sos/repository/iphy_A.rep/TEMP/case_1285613542/#alidation#iphy-gui#gui#lib#devices#fdi_box_x2.py,v 1.1 2011-09-02 11:43:54-07 case Exp $
# $Log: #alidation#iphy-gui#gui#lib#devices#fdi_box_x2.py,v $
# Revision 1.1  2011-09-02 11:43:54-07  case
# ...No comments entered during checkin...
#
# Revision 1.1  2011-05-04 18:30:17-07  rbemra
# Initial python for MDIO interface
#
#
# Test interface to 2 separate FDI USB_MPC boxes from 1 program
#
from mdio_fdi import *
from mytoken import *

MPC_Handle0 = 0
MPC_Handle1 = 1

mdio_init(portNum=MPC_Handle0) # initialize kit0
mdio_init(portNum=MPC_Handle1) # initialize kit1

# Let's check some dummy writes to the MMD DUT
# on the RTO

def run():
  while True:
    uEntry = raw_input("Enter 0=MPC0, 1=MPC1(none to break):")
    tList = token_list(uEntry)
    if len(tList)<1:
      break
    mpcHdl = token_value(tList[0])
    mdio_select_hw(mpcHdl)
    while True:
      uEntry = raw_input("MPC%d: Enter 16-bit <addr>, <value> to write (none to break):"%mpcHdl)
      tList = token_list(uEntry)
      if len(tList)<2:
        break
      rAddr = token_value(tList[0])
      wVal = token_value(tList[1])
      mdio_wr(wVal, rAddr)
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
