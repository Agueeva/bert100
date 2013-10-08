# ------------------------------------------------------------------------------
#
# $RCSfile: #alidation#iphy-gui#gui#ipuser.py,v $ - User-defined scripts for iPHY
#
# Disclaimer:
# Inphi Confidential
# Copyright(c) Inphi Corp. 2011
#
# All rights reserved.
# This is unpublished, confidential Inphi proprietary information.
# Do not reproduce or redistribute without written permission.
#
# $Header: /projects/wlv/iphy/sos/repository/iphy_A.rep/TEMP/case_1285613542/#alidation#iphy-gui#gui#ipuser.py,v 1.6 2011-09-03 08:32:37-07 case Exp $
# From: /projects/wlv/iphy/sos/repository/iphy_A.rep/TEMP/rbemra_1274230893/validation#python#ipuser.py,v 1.3 2011-08-05 11:36:48-07 rbemra
# $Log: #alidation#iphy-gui#gui#ipuser.py,v $
# Revision 1.6  2011-09-03 08:32:37-07  case
# Calibration ony using userfunc4, only set supplies for BG cal routine
#
# Revision 1.5  2011-09-02 14:25:37-07  case
# moved temp directory to calibration/temp
#
# Revision 1.4  2011-09-02 13:59:09-07  case
# added cdr startup
#
# Revision 1.3  2011-08-31 19:14:17-07  rbemra
# Updates for eFuse, imporovements to DC cal. user interaction
#
# Revision 1.2  2011-08-12 16:44:45-07  rbemra
# Updated for GB calibration: device name from AllRegs, OPZ is thermometer code
#
# Revision 1.1  2011-08-07 22:01:36-07  rbemra
# First iCDR100, iPHY100 release version
#
# Revision 1.3  2011-08-05 11:36:48-07  rbemra
# Added iCDR100 Vbg, Vreg, ODT, OPZ calibration.
#
# Revision 1.2  2011-07-05 18:43:10-07  rbemra
# Updates to get more things working: user scripts w/ options/threading, DDF+CSV writers, MDIO R/W updates, ipallreg addition
#
# Revision 1.1  2011-07-01 09:02:52-07  rbemra
# First, prelim. runnable version
#
import iputils
import ipvar
import time
import sys

global DUT_ID
OutDir = "../calibration/temp/"
# OutDir = "../../iPHY100/"
global OutPrefix # initially empty, OutDir + "gbcal_"
#-------------------------------------------------------------------------------

def UserFunc1(verbose=0):
  nop(func="1")
def UserFunc2(verbose=0):
  nop(func="2")
def UserFunc3(verbose=0):
  nop(func="3")
def UserFunc4(verbose=0):
  nop(func="4")
#-------------------------------------------------------------------------------

def curr_dev_id():
  from ipallreg import frameAR
  devId = 'dut'
  for (key, item) in frameAR.notebook.pages.items():
    if item.panel.IsShown():
      devId = key
      break
  # print 'curr_dev_id()=', devId
  return devId
#-------------------------------------------------------------------------------

def get_dev_type():
  devId = curr_dev_id().lower()
  if devId[0:2]=='gb':
    devType = 'gb'
  elif devId[0:3]=='cdr':
    devType = 'cdr'
  else:
    devType = 'dut'
  # print 'get_dev_type()=', devType
  return devType
#-------------------------------------------------------------------------------

def get_id():
  global DUT_ID, OutPrefix
  DUT_ID = raw_input("Enter Part ID:")
  devType = get_dev_type()
  OutPrefix = OutDir + 'cal_' + devType + '_'
  print "Output will be saved in", OutPrefix+DUT_ID+'.py and', OutPrefix+DUT_ID+'.csv', "files"
#-------------------------------------------------------------------------------

GB_STARTUP = "../scripts/GB_startup2.py"
CDR_STARTUP = "../scripts/cdr_startup.py"

def do_startup():
  if get_dev_type()=='gb':
    execfile(GB_STARTUP)
  if get_dev_type()=='cdr':
    execfile(CDR_STARTUP)
#-------------------------------------------------------------------------------

def UserFunc1(verbose=0):
  print("Please use UserFunc4 for calibration")
  # import ipcaldc as cal
  #import ipcaldcall as cal
  #global DUT_ID, OutPrefix
  #reload(cal)
  #get_id()
  #try:
    #calScript = OutPrefix + DUT_ID + '.py'
    #calLog = OutPrefix + DUT_ID + '.csv'
    #do_startup()
    # cal.write_vddr_vdd(DUT_ID, calLog, False) # VDCs set by next
    #cal.rx_bg_cal_all(DUT_ID, calScript, calLog)
    #cal.write_vddr_vdd(DUT_ID, calLog, False)
    #print 'UserFunc1() Done:', calScript, calLog
  #except Exception:
    #print sys.exc_info()
    #print "Exception caught during UserFunc1 execution"
#-------------------------------------------------------------------------------

def UserFunc2(verbose=0):
  print("Please use UserFunc4 for calibration")
  # import ipcaldc as cal
  #import ipcaldcall as cal
  #global DUT_ID, OutPrefix
  #reload(cal)
  #get_id()
  #try:
    #calScript = OutPrefix + DUT_ID + '.py'
    #calLog = OutPrefix + DUT_ID + '.csv'
    #"""
    #rxTxDict = {}
    #do_startup()
    #cal.write_vddr_vdd(DUT_ID, calLog, True)
    #cal.rx_vr_cal_all(rxTxDict, calScript, calLog)
    #cal.tx_vr_cal_all(rxTxDict, calScript, calLog)
    #cal.rx_tx_pll_vr_cal(rxTxDict, calScript, calLog)
    #cal.write_vddr_vdd(DUT_ID, calLog, False)
    #"""
    #cal.vr_cal_all(DUT_ID, calScript, calLog)
    #print 'UserFunc2() Done:', calScript, calLog
  #except Exception:
    #print sys.exc_info()
    #print "Exception caught during UserFunc2 execution"
#-------------------------------------------------------------------------------

def UserFunc3(verbose=0):
  print("Please use UserFunc4 for calibration")
  # import ipcaldc as cal
  #import ipcaldcall as cal
  #global DUT_ID, OutPrefix
  #reload(cal)
  #get_id()
  #try:
    #calScript = OutPrefix + DUT_ID + '.py'
    #calLog = OutPrefix + DUT_ID + '.csv'
    #do_startup()
    #cal.write_vddr_vdd(DUT_ID, calLog, True)
    #cal.rx_odt_cal_all(calScript, calLog)

    #cal.write_vddr_vdd(DUT_ID, calLog, True)
    #cal.tx_opz_cal_all(calScript, calLog)

    #cal.write_vddr_vdd(DUT_ID, calLog, False)
    #print 'UserFunc3() Done:', calScript, calLog
  #except Exception:
    #print sys.exc_info()
    #print "Exception caught during UserFunc2 execution"
#-------------------------------------------------------------------------------

def UserFunc4(verbose=0):
  # import ipcaldc as cal
  import ipcaldcall as cal
  global DUT_ID, OutPrefix
  reload(cal)
  get_id()
  try:
    calScript = OutPrefix + DUT_ID + '.py'
    calLog = OutPrefix + DUT_ID + '.csv'
    rxTxDict = {}

    do_startup()
    # cal.write_vddr_vdd(DUT_ID, calLog, True)
    cal.rx_bg_cal_all(DUT_ID, calScript, calLog)
    cal.vr_cal_all(DUT_ID, calScript, calLog)
    """
    cal.write_vddr_vdd(DUT_ID, calLog, True)
    cal.rx_vr_cal_all(rxTxDict, calScript, calLog)
    cal.tx_vr_cal_all(rxTxDict, calScript, calLog)
    cal.rx_tx_pll_vr_cal(rxTxDict, calScript, calLog)
    """
    cal.write_vddr_vdd(DUT_ID, calLog, False)
    cal.rx_odt_cal_all(calScript, calLog)
    cal.tx_opz_cal_all(calScript, calLog)
    cal.write_vddr_vdd(DUT_ID, calLog, False)

    # Append cal_main.py contents to calScript
    cal.append_script(calScript, '../gui/cal_main.py')
    print 'UserFunc4() Done:', calScript, calLog
  except Exception:
    print sys.exc_info()
    print "Exception caught during UserFunc4 execution"
#-------------------------------------------------------------------------------

def UserFunc1_bg_ansys(verbose=0):
  import ansys_bg_vr as bgcal
  reload(bgcal)
  try:
    # csvFile = '../iCDR100/cdr_evb2_dut7_bg.csv'
    csvFile = '../iPHY100/gb_evb4_dut1_bg.csv'
    bgcal.do_rx_bg_all(csvFile)
    print 'UserFunc1() Done:', csvFile
  except Exception:
    print sys.exc_info()
    print "Exception caught during UserFunc1 execution"
#-------------------------------------------------------------------------------

def UserFunc2_rx_ansys(verbose=0):
  import ansys_bg_vr as bgcal
  try:
    csvFile = '../iCDR100/cdr_evb2_dut7_vrf.csv'
    bgcal.do_freq_vs_vr_rx_all(csvFile)
    print 'UserFunc2() Done:', csvFile
  except Exception:
    print sys.exc_info()
    print "Exception caught during UserFunc2 execution"
#-------------------------------------------------------------------------------

def UserFunc3_tx_ansys(verbose=0):
  import ansys_bg_vr as bgcal
  reload(bgcal)
  try:
    csvFile = '../iCDR100/cdr_evb2_dut7_vrf.csv'
    bgcal.do_freq_vs_vr_tx_all(csvFile)
    print 'UserFunc3() Done:', csvFile
  except Exception:
    print sys.exc_info()
    print "Exception caught during UserFunc3 execution"
#-------------------------------------------------------------------------------

def UserFuncA(verbose=0):
  nop(func="A")

def UserFuncB(verbose=0):
  nop(func="B")

def UserFuncC(verbose=0):
  nop(func="C")

def UserFuncD(verbose=0):
  nop(func="D")

def UserFuncE(verbose=0):
  nop(func="E")

def UserFuncF(verbose=0):
  nop(func="F")
#-------------------------------------------------------------------------------

def nop(func=""):
  print "No UserFunc%s defined in user.py"%func
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
