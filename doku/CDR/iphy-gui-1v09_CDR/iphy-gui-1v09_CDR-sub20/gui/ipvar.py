# ------------------------------------------------------------------------------
#
# $RCSfile: validation#python#ipvar.py,v $ - Global variables
#
# Disclaimer:
# Inphi Confidential
# Copyright(c) Inphi Corp. 2011
#
# All rights reserved.
# This is unpublished, confidential Inphi proprietary information.
# Do not reproduce or redistribute without written permission.
#
# $Header: /projects/wlv/iphy/sos/repository/iphy_A.rep/LIBS/FILES258/SRC/validation#python#ipvar.py,v 1.3 2011-07-11 12:32:21-07 rbemra Exp $
# $Log: validation#python#ipvar.py,v $
# Revision 1.3  2011-07-11 12:32:21-07  rbemra
# Added GB_Name = 'GB0', CDR_Name = 'CDR0', per rward
#
# Revision 1.2  2011-07-05 18:43:09-07  rbemra
# Updates to get more things working: user scripts w/ options/threading, DDF+CSV writers, MDIO R/W updates, ipallreg addition
#
# Revision 1.1  2011-07-01 09:02:50-07  rbemra
# First, prelim. runnable version
#
# ------------------------------------------------------------------------------

#rb from VA import *    # constant arrays of the predefined patterns

AppGui = None
System = {}
MDIO_INIT = False

#rb frame = None
debug = False
osname = None
myftdi = None
ftdiDriver = None
openFTDI = 0
ftdiTransferSize = 64
config = 0
delay = 0
configurationFile  = ""
configurationTitle = ""
busy = False

forceExtCk = False
centervalue = [0,0,0,0,0,0]
settings = None
RealTime = 1
IAVDDoffset = 0
IVDDoffset =  0
IPVDDoffset = 0
Freq = 400
currentVDD = 0
intClk =1 
extClk = 0
bertGenerator=None
si5326Clk = 0
ncount = 0
RCconfig =[ [-1,-1]]*31 #  [ Address, value]
#RCconfig[17] = [7,3]
RCconfig[18] = [16,0]
RCconfig[19] = [8,3]  # for read only
RCconfig[20] = [0,3] # for write only
DisSMBfixes = 0
DisableSMBPrint = 1
bypass_VTT = 0 # used by the tAON_tAOFF test when VTT is grounded


fpgaFeatures       = None
fpgaVersionMajor   = 0
fpgaVersionMinor   = 0
ResetDelay = 180  #  = delay after Reset vector applied to iMB.   units = 33ns   180 = 6us

# Possible config commands
CONFIG_WRITE  = 0  # default
CONFIG_READ   = 1
CONFIG_PING   = 2
CONFIG_ECHO3  = 3
CONFIG_ECHO4  = 4
CONFIG_ECHO5  = 5
CONFIG_ECHO6  = 6
CONFIG_ECHO7  = 7

# Bit positions of possible FPGA features
FEATURE_REV3BOARD  = (1<<10)
FEATURE_REV4BOARD  = (1<<11)

# Daughter card SI5326 narrow band mode indicator
si5326narrowband = 0

# LV mode flag: 0 - 1.5V; 1 - 1.35V
LV_MODE = 0

# Temperature controller state
tempController = 0

# Configuration output data
configData = [0,0,0,0,0]

# Temerpature calibration points
# DAC temperature voltage range for -50C-125C
DACTempVoltRange = [512, 4095]

t0C100CVolts = [0, 0]

logFile = False 
handleLogFile = None

#rb configDir = None
#rb TopDir    = ""
#rb ConfigDir = ""

CDR_Name = 'CDR0' # 'iPHY-CDR'
CDR1_Name = 'CDR1' # 'iPHY-CDR'
GB_Name = 'GB0' # 'iPHY-GB
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
