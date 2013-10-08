#
# $Header: /projects/wlv/iphy/sos/repository/iphy_A.rep/TEMP/case_1285613542/#alidation#iphy-gui#gui#lib#devices#dca.py,v 1.1 2011-09-02 11:43:54-07 case Exp $
# $Log: #alidation#iphy-gui#gui#lib#devices#dca.py,v $
# Revision 1.1  2011-09-02 11:43:54-07  case
# ...No comments entered during checkin...
#
# Revision 1.2  2011-08-31 19:24:54-07  rbemra
# Ported more methods from C: untested!
#
# Revision 1.1  2011-05-04 18:10:36-07  rbemra
# Initial python+pyvisa for instrumentation (from ExacTik/882)
#
# Revision 1.2  2008/07/23 02:03:19  rbemra
# Deleted RTO stuff
# inlab -> inlab/pylab
#
# Revision 1.1.1.1  2008/07/21 18:20:28  rbemra
# Initial Version
#
#
# Eqvt.-time/DCA (Dig. Comm. Analyzer) scope remote interface
# Packaged under pylab 7/21/2008: RSB
#
import time
from labdev import *
from meas_utils import *

DCA_WAIT_US = 4e6 # wait time for DCA result

ETOSC_DCA = 2
ETOSC_BERT = 4
EtOscScopeDict = {
  '86100':ETOSC_DCA,
  'Bertscope':ETOSC_BERT
}
class EtScope(LabDev):
  def __init__(self, name):
    LabDev.__init__(self, name)
    LabDev.set_model(self, EtOscScopeDict, DEV_SCOPE)
  def save_screen(self, jpgName, doAsk=True, doPause=True):
    if doPause:
      self.viDev.write(":STOP")
    if doAsk:
      yesNo = raw_input("\nScreenshot %s.jpg? (n):"%jpgName)
      if yesNo=='y' or yesNo=='1':
        yesNo = True
      else:
        yesNo = False
    else:
      yesNo = True
    if yesNo:
      cmd = ":DISK:SIM \"%s.jpg\", SCR"%(jpgName)
      self.viDev.write(cmd)
    if doPause:
      self.viDev.write(":RUN")
#------------------------------------------------------------

DCA_86100_TO = 10.0 # to hopefully cure MEAS:CROS exception
class HP86100(EtScope):
  # def __init__(self, name):
  #   LabDev.__init__(self, name)
  #   LabDev.set_model(self, MeterDict, DEV_METER)
  #  # self.viDev.timeout = DCA_86100_TO
  #  print "DCA init..."
  def HP86100A_channel_setup(self,
    chan, mode, atten, trig_src, trig_bw,
    trig_lev, hor_scale, ver_scale, offset):

    # Sets Sampler Bandwidth
    # OPTIONS: <HIGH> <LOW>

    self.viDev.write(":CHAN%i:BAND HIGH"%chan)

    # Sets Attenuation Factors
    # OPTIONS: <DEC> <RAT>

    self.viDev.write(":CHAN%i:PROB %.1f,DEC"%(chan,atten))

    # Sets Trigger Source
    # OPTIONS: <FPAN> <FRUN> <LMOD> <RMOD>

    self.viDev.write(":TRIG:SOUR %s"%trig_src)

    # Sets Trigger Bandwidth
    # OPTIONS:  <HIGH> <LOW> <DIV>

    if trig_bw!=None:
      self.viDev.write(":TRIG:BWL %s"%trig_bw)
    if trig_lev!=None:
      self.viDev.write(":TRIG:LEV %.4f"%trig_lev)
    if hor_scale!=None and hor_scale>0.:
      self.viDev.write(":TIM:SCAL %e"%hor_scale)
    if ver_scale!=None and ver_scale>0.:
      self.viDev.write(":CHAN%i:SCAL %f"%(chan, ver_scale))

    # Sets Channel Display mode
    # OPTIONS: <EYE> <OSC>

    self.viDev.write(":SYST:MODE %s"%mode)
    self.viDev.write(":CHAN%i:OFFS %f"%(chan,offset))
    self.viDev.write(":MEAS:ANN OFF")

    # Sets Channel Display ON
    # OPTIONS: <ON> <OFF>

    self.viDev.write(":CHAN1:DISP OFF") # self.set_display(1, 0)
    self.viDev.write(":CHAN2:DISP OFF")
  #------------------------------------------------------------

  def HP86100A_Setup_Meas(self, chan=1):
    self.viDev.write("CDIS")
    self.viDev.write(":MEAS:CLE")
    self.viDev.write(":CHAN1:DISP ON")
    self.viDev.write(":CHAN2:DISP ON, APP") # Turn On Both Chans Simultaneously!!
    self.viDev.write(":MEAS:CGR:COMP 10")
  #------------------------------------------------------------

  def HP86100A_meas_amp(self, chan, minMax=None):
    rList = []
    # self.viDev.write(":MEAS:CGR:SOUR CHAN%d"%chan)
    usleep(DCA_WAIT_US) # wait for DCA result
    [amp] = self.viDev.ask_for_values(":MEAS:CGR:AMPLITUDE? CHAN%d"%chan) # for A
    if (amp >= 1E+10):
      amp = 0.0
      rStat = -1
    elif (minMax!=None and ((amp > minMax[1]) or (amp < minMax[0]))):
      rStat = False
    else: rStat = True

    return [rStat, amp]
  #------------------------------------------------------------

  def HP86100A_meas_edge(self, chan, doRise=True, minMax=None):
    if doRise:
      cmd = ":MEAS:RIS? %d"
    else:
      cmd = ":MEAS:FALL? %d"
    usleep(DCA_WAIT_US) # wait for DCA result
    [tEdge] = self.viDev.ask_for_values(cmd%chan)
    tEdge = tEdge*1e+12
    if (tEdge >= 1E+10):
      tEdge = 0.0
      rStat = -1
    elif (minMax!=None and ((tEdge > minMax[1]) or (tEdge < minMax[0]))):
      rStat = False
    else: rStat = True

    return [rStat, tEdge]
  #------------------------------------------------------------

  def HP86100A_meas_jit(self, chan, doRMS=True, minMax=None):
    if doRMS:
      jMode = "RMS"
    else:
      jMode = "PP"
    usleep(DCA_WAIT_US) # wait for DCA result
    [tJit] = self.viDev.ask_for_values(":MEAS:CGR:JITT? %s, CHAN%d"%(jMode, chan))
    tJit = tJit*1e+12
    if (tJit >= 1E+10):
      tJit = 0.0
      rStat = -1
    elif (minMax!=None and ((tJit > minMax[1]) or (tEdge < minMax[0]))):
      rStat = False
    else: rStat = True

    return [rStat, tJit]
  #------------------------------------------------------------

  def HP86100A_meas_crosspct(self, chan, minMax=None):
    # while True: # Exception NEEDS TESTING!
    try:
      usleep(DCA_WAIT_US) # wait for DCA result
      [cross] = self.viDev.ask_for_values(":MEAS:CGR:CROS? CHAN%d"%chan)
    except ValueError:
      #userKey = raw_input("Eye-Crossing failed:'n' to skip, other repeat:")
      #if userKey == 'n':
      cross = 1E10 # let's continue, so DCA will not get stuck
    if (cross >= 1E+10):
      cross = 0.0
      rStat = -1
    elif (minMax!=None and ((cross > minMax[1]) or (cross < minMax[0]))):
      rStat = False
    else: rStat = True

    return [rStat, cross]
  #------------------------------------------------------------

  def HP86100A_Meas_Low_Amptd_chX(self, chan, test_num):
    rList = []
    self.viDev.write(":MEAS:CGR:SOUR CHAN%i"%chan)
    usleep(1000000) # Needed sleep for accurate data!!!!
	
    [amp] = self.viDev.ask_for_values(":MEAS:CGR:AMPLITUDE?")
    if (amp >= 1E+10):
      amp = 0.0

    [rise] = self.viDev.ask_for_values(":MEAS:RIS?")
    rise = rise*1e+12
    if (rise >= 1E+10):
      rise = 0.0

    [fall] = self.viDev.ask_for_values(":MEAS:FALL?")
    fall = fall*1e+12
    if (fall >= 1E+10):
      fall = 0.0

    [jit_rms] = self.viDev.ask_for_values(":MEAS:CGR:JITT? RMS")
    jit_rms = jit_rms*1e+12
    if (jit_rms >= 1E+10):
      jit_rms = 0.0

    [jit_pp] = self.viDev.ask_for_values(":MEAS:CGR:JITT? PP")
    jit_pp = jit_pp*1e+12
    if (jit_pp >= 1E+10):
      jit_pp = 0.0

    [cross] = self.viDev.ask_for_values(":MEAS:CGR:CROS?")
    if (cross >= 1E+10):
      cross = 0.0

    rList.append(amp) # log_data("%i = %.3f\n", test_num, amp)
    rList.append(jit_rms) # log_data("%i = %.3f\n", test_num+1, jit_rms)
    rList.append(jit_pp) # log_data("%i = %.3f\n", test_num+2, jit_pp)
    rList.append(rise) # log_data("%i = %.3f\n", test_num+3, rise)
    rList.append(fall) # log_data("%i = %.3f\n", test_num+4, fall)
    rList.append(cross) # log_data("%i = %.3f\n", test_num+5, cross)

    if ((amp > LOW_AMP_MAX_SPEC) or (amp < LOW_AMP_MIN_SPEC)):
      rVal = 4
    elif ((rise > RISE_MAX_SPEC) or (rise < RISE_MIN_SPEC)):
      rVal = 7
    elif ((fall > FALL_MAX_SPEC) or (fall < FALL_MIN_SPEC)):
      rVal = 8
    elif ((jit_rms > JIT_RMS_MAX_SPEC) or (jit_rms < JIT_RMS_MIN_SPEC)):
      rVal = 5
    elif ((jit_pp > JIT_PP_MAX_SPEC) or (jit_pp < JIT_PP_MIN_SPEC)):
      rVal = 6
    elif ((cross > CROSS_MAX_SPEC) or (cross < CROSS_MIN_SPEC)):
      rVal = 9
    else:
      rVal = 1
    return [rVal, rList]

  #------------------------------------------------------------	

  def HP86100A_Meas_High_Amptd_chX(self, chan, test_num):
    self.viDev.write(cmd, ":MEAS:CGR:SOUR CHAN%i", chan)
    usleep(1000000) # Needed sleep for accurate data!!!! */

    [amp] = self.viDev.ask_for_values(":MEAS:CGR:AMPLITUDE?")
    if (amp >= 1E+10):
      amp = 0.0
    amp = get_scaled_limited(self, ":MEAS:CGR:AMPLITUDE?", 1e12, 1e10)

    rise = get_scaled_limited(self, ":MEAS:RIS?", 1e12, 1e10)
    fall = get_scaled_limited(self, ":MEAS:FALL?", 1e12, 1e10)
    jit_rms = get_scaled_limited(self, ":MEAS:CGR:JITT?", 1e12, 1e10)
    jit_pp = get_scaled_limited(self, ":MEAS:CGR:JITT? PP", 1e12, 1e10)

    cross = get_scaled_limited(self, ":MEAS:CGR:CROS?", 1.0, 1e10)

    rList.append(amp) # log_data("%i = %.3f\n", test_num, amp)
    rList.append(jit_rms) # log_data("%i = %.3f\n", test_num+1, jit_rms)
    rList.append(jit_pp) # log_data("%i = %.3f\n", test_num+2, jit_pp)
    rList.append(rise) # log_data("%i = %.3f\n", test_num+3, rise)
    rList.append(fall) # log_data("%i = %.3f\n", test_num+4, fall)
    rList.append(cross) # log_data("%i = %.3f\n", test_num+5, cross)

    if ((amp > HIGH_AMP_MAX_SPEC) or (amp < HIGH_AMP_MIN_SPEC)):
      rVal = 4
    elif ((rise > RISE_MAX_SPEC) or (rise < RISE_MIN_SPEC)):
      rVal = 7
    elif ((fall > FALL_MAX_SPEC) or (fall < FALL_MIN_SPEC)):
      rVal = 8
    elif ((jit_rms > JIT_RMS_MAX_SPEC) or (jit_rms < JIT_RMS_MIN_SPEC)):
      rVal = 5
    elif ((jit_pp > JIT_PP_MAX_SPEC) or (jit_pp < JIT_PP_MIN_SPEC)):
      rVal = 6
    elif ((cross > CROSS_MAX_SPEC) or (cross < CROSS_MIN_SPEC)):
      rVal = 9
    else:
      rVal = 1
    return [rVal, rList]

  #------------------------------------------------------------	

  def HP86100A_Meas_Xing_Only_chX(self, chan, test_num):
    self.viDev.write(cmd, ":MEAS:CGR:SOUR CHAN%i"%chan)
    usleep(1000000)  #	Needed Sleep for accurate Xing!!!!!
	
    cross = get_scaled_limited(self, ":MEAS:CGR:CROS?", 1.0, 1e10)
		
    log_data("%i = %.3f\n", test_num, cross)
    if ((cross > CROSS_MAX_SPEC) or (cross < CROSS_MIN_SPEC)):
      rVal = 9
    else:
      rVal = 1
    return [rVal, cross]
  #------------------------------------------------------------	
  # chan = CHANNEL #
  # freq = EXPECTED FREQUENCY
  # min_amp = EXPECTED MINIMUM AMP OF SIGNAL
  # accuracy = PERCENT ACCURACY ex. 5% = 0.05
  # test_num = TEST NUMBER

  def HP86100A_Div_Search(self, chan, freq, min_amp, accuracy, test_num):
    # static char cmd[256],reply[256]
    # static float amp, scope_freq, freq_left, freq_right

    freq_left = (1.0 - accuracy) * (freq)
    freq_right = (1.0 + accuracy) * (freq)

    self.viDev.write(cmd, ":CHAN%i:DISP ON",chan)
    usleep(600000)
    self.viDev.write(cmd, ":MEAS:SOUR CHAN%i",chan)
	
    [amp] = self.viDev.ask_for_values(cmd, ":MEAS:VAMP? CHAN%i",chan)

    if (amp >= min_amp):
      [scope_freq] = self.viDev.ask_for_values(":MEAS:FREQ? CHAN%i"%chan)
      if ((scope_freq >= freq_left) and (scope_freq <= freq_right)):
        log_data("%i = %.3e\n", test_num, scope_freq)
        self.viDev.write(":CHAN%i:DISP OFF"%chan)
        return(0)
      else:
        log_data("%i = %.3e\n", test_num, scope_freq)
        self.viDev.write(":CHAN%i:DISP OFF"%chan)
        return(1)
    else:
      log_data("%i = 0.000\n", test_num)
      self.viDev.write(":CHAN%i:DISP OFF"%chan)
      return(1)
  #------------------------------------------------------------	
  def HP86100A_get_gif(self, name):
    # static char cmd[256],data[100000]
    # char img_name[256],bytes[10]
    # static int i,j,num_bytes,tot_bytes
    # static char *tok
    # FILE *infile
    # FILE *outfile

    if (FLOW==2):
      self.viDev.write("%s/GIF/%s_%i,%i,%i,%i,%s.gif"%
        (DATA_PATH,name,retx,rety,diex,diey,wfr_num))
    elif (FLOW==1):
      self.viDev.write("%s/GIF/%s_%i.gif"%(DATA_PATH,name,serial))

    self.viDev.ask_for_values("DISP:DATA? GIF,SCR,ON,NORM")
    time.sleep(2)
    HP86100A_rd(data, 1)
    HP86100A_rd(bytes,1)
    num_bytes = atoi(bytes)

    HP86100A_rd(bytes,num_bytes)
    tot_bytes = atoi(bytes)

    gpib_rdf(hp86100a_dev, img_name)

    self.viDev.write(":CHAN1:DISP OFF")
    self.viDev.write(":CHAN2:DISP OFF")
  #------------------------------------------------------------

  # from EyeAnalysis.c::EA_AutoSet_Vert()
  def HP86100A_auto_vrange(self, chan, src=None):
    scale = 1.0

    # Two stage vertical autoscale
    # Get the amplitude of channel chan and set scale so 
    # wform covers about half of screen. Then remeasure amplitude
    # and return scale such that waveform covers 3/4 of screen.

    if (src=="FUNC"):
      self.viDev.write(":FUNC%d:VERT AUTO"%chan)
      return

    [atten] = self.viDev.ask_for_values(":CHAN%d:PROB?"%chan)
    scalemx = 0.999999 * pow(10.0, (atten/20.0)) / 8.0
    scalemn = 0.002 * pow(10.0, (atten/20.0)) / 8.0

    scale = scalemx
    offset = 0.0
    for i in [0, 2]:
      self.viDev.write(":CHAN%i:SCAL %f"%(chan, scale))
      self.viDev.write(":CHAN%i:OFFS %f"%(chan, offset))
      self.viDev.write(":CHAN%d:DISP ON, APP"%chan)

      self.viDev.write(":MEAS:SOUR CHAN%d"%chan)
      self.opc_wait(0)
      [vmn] = self.viDev.ask_for_values(":MEAS:VMIN?")
      self.opc_wait(0)
      [vmx] = self.viDev.ask_for_values(":MEAS:VMAX?")
      self.opc_wait(0)

      scale = (vmx-vmn)/4.0
      if (scale<scalemn):
        scale = scalemn
      if (scale>scalemx):
        scale = scalemx
      offset = (vmx + vmn)/2.0

    scale = (vmx-vmn)/4.5
    if (scale<scalemn):
      scale = scalemn
    if (abs(offset) < scale/2.0):
      offset = 0.0

    self.viDev.write(":CHAN%i:SCAL %f"%(chan, scale))
    self.viDev.write(":CHAN%i:OFFS %f"%(chan, offset))

  #------------------------------------------------------------

  def HP86100A_hist_ppos(self, srcStrg=None, chNum=None):
    if (srcStrg!=None and chan!=None): # may actually be ignored per online alg.
      self.viDev.write("MEAS:SOUR " + srcStrg + "%d"%chNum)
    [pkPos] = self.viDev.ask_for_values(":MEAS:HIST:PPOS")
    return pkPos
  #-------------------------------------------------------------------------------
  def HP86100A_EA_MakeHist(self, ch, ori, tmn, tmx,	vmn, vmx):
    if (ch<1 or ch>4):
      raise Exception("EA_MakeHist::Illegal channel %d\n"%ch)
    elif (length(ori)>4):
      raise("EA_MakeHist:ori char array too long")
    self.viDev.write(":HIST:SOUR CHAN%d"%ch)
    self.viDev.write(":HIST:AXIS " + ori)
 
    # self.viDev.write(":HIST:SCALE:SIZE 6.0")*/
    self.viDev.write(":HIST:WIND:BORD ON")
    self.viDev.write(":HIST:WIND:X1P %e"%tmn)
    self.viDev.write(":HIST:WIND:X2P %e"%tmx)
    self.viDev.write(":HIST:WIND:Y1P %e"%vmn)
    self.viDev.write(":HIST:WIND:Y2P %e"%vmx)
    self.viDev.write(":HIST:MODE ON")

  #-------------------------------------------------------------------------------

  def HP86100A_CenterEye(self, chan, mode, tUI):
    # mode: 0: eye parameter measurements, 1: centers cross in middle for zooming

    self.viDev.write(":ACQ:RUNT OFF")
    dcaMode = self.viDev.ask(":SYST:MODE?")

    self.viDev.write(":SYST:MODE OSC")
    self.viDev.write(":TIM:REF LEFT")

    self.viDev.write(":TIM:RANG %.6e"%(2.0*tUI)) # Set to 2 UI width every time

    self.viDev.write(":TIM:POS 0."); # Get min. delay setting by forcing to 0.0
    [dcaRef] = self.viDev.ask_for_values(":TIM:POS?")

    self.viDev.write(":CHAN%i:DISP ON"%chan)
    self.viDev.write(":MEAS:SOUR CHAN%i"%chan)
    self.viDev.write("RUN")  # Don't forget to run!! 
    usleep(100000)

    [VMax] = self.viDev.ask_for_values(":MEAS:VMAX?")
    if (VMax >= 1E+10):
      VMax = 0.0
 
    [VMin] = self.viDev.ask_for_values(":MEAS:VMIN?")
    if (VMin >= 1E+10):
      VMin = 0.0
 
    VHistMin = (VMax + VMin)/2.0 - 0.003 
    VHistMax = (VMax + VMin)/2.0 + 0.003 

    self.viDev.write(":HIST:AXIS HOR")
    self.viDev.write(":HIST:SCALE:SIZE 6.0")

    self.viDev.write(":HIST:SOUR CHAN%i"%chan)

    self.viDev.write(":HIST:WIND:BORD ON")
    self.viDev.write(":HIST:WIND:X1P %e"%(dcaRef + 0.01*tUI))
    self.viDev.write(":HIST:WIND:X2P %e"%(dcaRef + 0.91*tUI))

    self.viDev.write(":HIST:WIND:Y1P %e"%VHistMin)
    self.viDev.write(":HIST:WIND:Y2P %e"%VHistMax)
    self.viDev.write(":HIST:MODE ON")

    for itry in range(8):
      usleep(100000)
      [xTime] = self.viDev.ask_for_values(":MEAS:HIST:MED? HIST")
      if (xTime<1.E-7):
        break
    if (xTime>1.0):
      xTime = dcaRef #  If you can't find a crossing, it must be near left-side

    self.viDev.write(":HIST:MODE OFF")

    # Center Crossing on the scope
    xDel = (xTime - dcaRef)
    halfUI = 0.5*tUI
    if (mode==0):
      if (xDel > halfUI):
        newRef = xTime - halfUI
      else:
        newRef = xTime + halfUI
    else:
      newRef = xTime

    self.viDev.write(":TIM:POS %.6e"%newRef)
    self.viDev.write(":SYST:MODE "+dcaMode)
  #-------------------------------------------------------------------------------

  def HP86100A_HandshakeSta(self, mode, ch, nac):
    if (ch==0):
      ichsta=1
      ichfin=2
    else:
      ichsta=ch
      ichfin=ch

    self.viDev.write("*CLS")

    # Probably unnecessary
    self.viDev.write("*ESE 1")             # Enable Evnt Stat. Reg. OPC Bit
    self.viDev.write("*SRE 32")            # Enable Stat. Byt. Reg. ESB Bit

    self.viDev.write("CDIS")
    self.viDev.write(":MEAS:CLE")
    if ("EYE"==mode):
      self.viDev.write(":MEAS:CGR:COMP 1")

    for ich in range(ichsta, ichfin+1, 1):
      self.viDev.write(":CHAN%i:DISP ON, APP"%ich)
 
    self.viDev.write(":ACQ:RUNT WAV, %d"%nac)
    self.viDev.write(":RUN")
    self.viDev.write("*SRE 32")            # Enable Stat. Byt. Reg. ESB Bit
    self.viDev.write("*OPC")
  #-------------------------------------------------------------------------------

  def HP86100A_HandshakeFin(self):
    DCA_FINISH_MEAS_SEC = 5
    if ( not self.opc_wait(DCA_FINISH_MEAS_SEC)):
      print("DCA: HandshakeFin()->OPC incomplete after %dsec"%DCA_FINISH_MEAS_SEC)
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
