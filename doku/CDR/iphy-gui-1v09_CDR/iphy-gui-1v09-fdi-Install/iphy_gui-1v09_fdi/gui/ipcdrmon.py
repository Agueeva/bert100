#10/18 - Check for Auto-verify. Display enhancements
import wx
import re
import time
import math

import ipvar
import ipallreg


class BERT():
  def __init__(self, *args, **kwargs):
    self.accumulateError = 0
    self.syncLoss = 0
    self.errorSaturation = 0
    self.regs = {}

    self.cdrpiLatest = 256  # last value read from the PI
    self.cdrHist = [0]*513  # zero init the histogram of the PI
    self.cdr2Hist = [0]*10  # history of cdr2 values  
    self.trip = 0
    
  def setRate(self,String):
    print "Rate set to",String    
  
class cdrMonitorPanel(wx.Panel):
  """  """
  def __init__(self, parent, *args, **kwargs):
    """   """
    wx.Panel.__init__(self, parent, *args, **kwargs)
    self.parent = parent
    self.panel = self

    self.Bind(wx.EVT_PAINT,self.OnPaint)
    self.time = 0
    self.numberOfBits = 0


    self.go1hr = 0
	
    self.adapt_IDs = [-1]*10
    self.pattern_labels = ['PRBS31','PRBS9','res','res','PRBS31','PRBS23','PRBS15','PRBS7']

	
    TIMER_ID = 1000  # pick a number
    self.timer = wx.Timer(self, TIMER_ID)  # message will be sent to the panel
    #self.timer.Start(50)  # x100 milliseconds
    wx.EVT_TIMER(self, TIMER_ID, self.OnTimer)  # call the on_timer function  
   
    rx=250
    ry=10
    # ***   BUTTONS  *************************************************
    self.goButton   = wx.Button(self,-1,label="Start Monitor",pos=(5,10),size=(100,24))
    self.goButton.controls = self
    
    self.stopButton = wx.Button(self,-1,label="Stop Monitor",pos=(5,40),size=(100,24))
    self.stopButton.controls = self

    #self.go1hrButton = wx.Button(self,-1,label="1 Hour Test",pos=(5,70),size=(100,24))
    #self.go1hrButton.controls = self
    #self.go1hrButton.Show(False)

    self.goButton.Bind(wx.EVT_BUTTON,self.OnGo)
    self.stopButton.Bind(wx.EVT_BUTTON,self.OnStop)
    #self.go1hrButton.Bind(wx.EVT_BUTTON,self.OnGo1hr)
    
    rx=0
    ry=-15
    font=wx.Font(10, wx.FONTFAMILY_DEFAULT,wx.NORMAL,wx.BOLD)

    self.timeText = wx.StaticText(self,-1,'0d 00:00:00',size=(110,24),pos=(rx+120,ry+25),style=wx.RAISED_BORDER|wx.ALIGN_RIGHT|wx.ST_NO_AUTORESIZE)
    self.timeText.SetFont(font)
    
    #self.staticBoxDev = wx.StaticBox(self.panel,-1,label='Device',pos=(rx+250,ry+20),style=wx.RAISED_BORDER,size=(150,40))
    self.checkboxDevice=[]
   
    #Devices = []
    #for eachDevice in ipvar.System.keys():
    #  Devices.append(eachDevice)
    #self.device = Devices[0]
    
   
    #Identify devices in create_system.txt
    #CDR, GB1 and not GB2
    
    self.CDR_list = []
    self.GB1_list = []

    for eachDevice in ipvar.System.keys():
      if (re.search(r'\w+-\w+',eachDevice)):
        pass
      else:
        cdr = re.search(r'(CDR\w*)',eachDevice)
        gb1 = re.search(r'(GB[^2]+)',eachDevice)
          
        if(cdr) :
          #print 'ipcdrmon - CDR: %s'%cdr.group(1)
          self.CDR_list.append(cdr.group(1)) 
        if(gb1) :
          #print 'ipcdrmon GB1: %s'%gb1.group(1)
          self.GB1_list.append(gb1.group(1))
                            
    self.Device_list = self.CDR_list + self.GB1_list 
    self.device = self.Device_list[0]
 
    
    self.deviceRB = wx.RadioBox(self, label="Device", pos=(rx+250, ry+20), choices=self.Device_list,majorDimension=0)
    self.Bind(wx.EVT_RADIOBOX, self.ChangeDevice,self.deviceRB)
    choices=['25.78125','27.952']
    self.rateBox  = wx.ComboBox(self.panel,-1,value=choices[0],choices=choices,pos=(rx+120,ry+55),style=wx.ALIGN_RIGHT|wx.ST_NO_AUTORESIZE,name='rateCombo',size=(75,27))
    rateText = wx.StaticText(self.panel,-1,"(Gbps)",pos=(rx+200,ry+57),size=(-1,24))
    font=wx.Font(9, wx.FONTFAMILY_DEFAULT,wx.NORMAL,wx.BOLD)
    rateText.SetFont(font)
    bitsText = wx.StaticText(self.panel,-1,"Bits",pos=(rx+200,ry+82))
    bitsText.SetFont(font)
    self.bitsWidget = wx.StaticText(self,-1,'0',size=(75,22),pos=(rx+120,ry+80),style=wx.RAISED_BORDER|wx.ALIGN_RIGHT|wx.ST_NO_AUTORESIZE)

     
    #self.stopButton.Enable(False)
    self.stop = 0  
    #self.widgets={} # a list of widgets that get updated from reg values
    #self.regs={}    # a list of registers required to be read for update of this panel
    self.BERT = []
           
    self.rx=5
    self.ry=95
    self.i_y_offset = 105
    self.pattern_id_map={} 
       
    for i in range(0,4):
      if i>4:
        self.rx=307
      self.BERT.append(BERT())
      self.BERT[i].pattern_id = wx.NewId()
      self.pattern_id_map[self.BERT[i].pattern_id] = i
      self.BERT[i].widgets = {}
      if self.device.find("GB") == 0:
        self.BERT[i].regs['liveErrCount']  = self.device + '::8.' + str(1700  + (i)) + '.15:0'
      else:
        self.BERT[i].regs['liveErrCount']  = self.device + '::30.' + str(48  + (i)) + '.15:0'
        
      self.BERT[i].regs['prbsLock']        = self.device + '::30.' + str(16  + (i))
      self.BERT[i].regs['piLock']          = self.device + '::30.' + str(416 + (i*256)) + '.1'
      self.BERT[i].regs['cdr2Trip']        = self.device + '::30.' + str(423 + (i*256)) # + '.13:0'
      self.BERT[i].regs['ctle_state']      = self.device + '::30.' + str(421 + (i*256)) + '.3:0'
      self.BERT[i].regs['ctle_lock']       = self.device + '::30.' + str(387 + (i*256)) + '.0'
      #self.BERT[i].regs['ctle_override']   = self.device + '::30.' + str(420 + (i*256)) + '.0'

      self.staticBox3   = wx.StaticBox(self,-1,label='RX'+str(i),pos=(self.rx,self.ry+(i%5)*self.i_y_offset),style=wx.RAISED_BORDER|wx.BOLD,size=(297,self.i_y_offset-2))

      wx.StaticText(self.panel,-1,"Errors", pos=(self.rx+19,self.ry+20+(i%5)*self.i_y_offset)).SetFont(font)
      wx.StaticText(self.panel,-1,"Pattern",pos=(self.rx+9, self.ry+43+(i%5)*self.i_y_offset)).SetFont(font)
      wx.StaticText(self.panel,-1,"CTLE",   pos=(self.rx+23,self.ry+66+(i%5)*self.i_y_offset)).SetFont(font)
      
      self.BERT[i].widgets['liveErrCount'] = wx.StaticText(self,-1,'0',size=(45,22),pos=(self.rx+58,self.ry+17+(i%5)*self.i_y_offset),style=wx.RAISED_BORDER|wx.ALIGN_RIGHT|wx.ST_NO_AUTORESIZE)

      self.BERT[i].widgets['liveErrCount'].name = 'liveErrCount'
      #accText = wx.StaticText(self.panel,-1,"Accumulation",pos=(self.rx+151,self.ry+15+(i%5)*self.i_y_offset)) 
      #accText.SetFont(font)           
      self.BERT[i].widgetAccumulateError = wx.StaticText(self,-1,'0',size=(60,22),pos=(self.rx+103,self.ry+17+(i%5)*self.i_y_offset),style=wx.RAISED_BORDER|wx.ALIGN_RIGHT|wx.ST_NO_AUTORESIZE)

      #self.BERT[i].widgets['prbsLock']    = wx.StaticText(self,-1,' ',size=(60,22),pos=(self.rx+58,self.ry+40+(i%5)*self.i_y_offset),style=wx.RAISED_BORDER|wx.ALIGN_CENTER|wx.ST_NO_AUTORESIZE)
      self.BERT[i].widgets['prbsLock'] = wx.Button(self,self.BERT[i].pattern_id,label=" ",size=(60,22),pos=(self.rx+58,self.ry+40+(i%5)*self.i_y_offset),style=wx.RAISED_BORDER|wx.ALIGN_RIGHT|wx.ST_NO_AUTORESIZE)

      self.BERT[i].widgets['prbsLock'].name = 'prbsLock'
      self.BERT[i].widgets['prbsLock'].Bind(wx.EVT_BUTTON,self.OnPattern)
      
      #self.rx+360,self.ry+self.i_y_offset*i+self.i_y_offset/2
      self.BERT[i].widgets['piLock'] = wx.Button(self,self.BERT[i].pattern_id,label=" ",size=(12,12),pos=(self.rx+360-6,self.ry+46+(i%5)*self.i_y_offset),style=wx.NO_BORDER)

      self.BERT[i].widgets['piLock'].name = 'piLock'
      self.BERT[i].widgets['piLock'].Bind(wx.EVT_BUTTON,self.OnPiLock)
      self.BERT[i].widgets['piLock'].Show(False)
      if i == 3:
        self.BERT[i].widgets['piLock'].Show(True) 
        
      choices=['0','1','2','3','4','5','6','7','8','9','10','11','12','13','14']
      self.BERT[i].widgets['ctle_state']  = wx.ComboBox(self,-1,value=choices[0],choices=choices,pos=(self.rx+58,self.ry+65+i*self.i_y_offset),style=wx.RAISED_BORDER|wx.ALIGN_RIGHT|wx.ST_NO_AUTORESIZE,name='rateCombo',size=(35,22))
      self.BERT[i].widgets['ctle_lock']   = wx.CheckBox(self,-1,label='Lock',pos=(self.rx+58+37,self.ry+64+i*self.i_y_offset))
      #self.BERT[i].widgets['ctle_override']   = wx.CheckBox(self,-1,label='Override',pos=(self.rx+58+37,self.ry+77+i*self.i_y_offset))
      self.BERT[i].widgets['ctle_lock'].Bind(wx.EVT_CHECKBOX,self.OnCtleLock)  
      #self.BERT[i].widgets['ctle_state']  = wx.ComboBox(self,-1,value='1',choices=[0,1,2,3],pos=(self.rx+58,self.ry+65+i*self.i_y_offset),style=wx.ALIGN_RIGHT|wx.ST_NO_AUTORESIZE,name='ctle',size=(18,22))
      
      #self.BERT[i].widgets['ctle_state'] = wx.StaticText(self,-1,label=" ",size=(18,22), pos=(self.rx+58,self.ry+65+i*self.i_y_offset),style=wx.RAISED_BORDER|wx.ALIGN_RIGHT|wx.ST_NO_AUTORESIZE)
      self.BERT[i].widgets['ctle_state'].name = 'ctle_state'
      self.BERT[i].widgets['ctle_lock'].name = 'ctle_lock'+str(i)
      #self.BERT[i].widgets['ctle_override'].name = 'ctle_override'+str(i)

      self.BERT[i].widgets['liveErrCount'].regStr = self.BERT[i].regs['liveErrCount']
      self.BERT[i].widgets['prbsLock'].regStr = self.BERT[i].regs['prbsLock']
      self.BERT[i].widgets['piLock'].regStr = self.BERT[i].regs['piLock']
      self.BERT[i].widgets['ctle_state'].regStr = self.BERT[i].regs['ctle_state']
      self.BERT[i].widgets['ctle_lock'].regStr = self.BERT[i].regs['ctle_lock']
      #self.BERT[i].widgets['ctle_override'].regStr = self.BERT[i].regs['ctle_override']
      
      
      #wx.StaticText(self,-1,label=" ",size=(76,22),pos=(self.rx+58+78,self.ry+63+(i%5)*self.i_y_offset),style=wx.RAISED_BORDER|wx.ALIGN_RIGHT|wx.ST_NO_AUTORESIZE)
      #wx.StaticText(self,-1,label=" ",size=(76,22),pos=(self.rx+58+78+78,self.ry+63+(i%5)*self.i_y_offset),style=wx.RAISED_BORDER|wx.ALIGN_RIGHT|wx.ST_NO_AUTORESIZE)

      #self.BERT[i].widgets['cdrLock']    = wx.StaticText(self,-1,'CDR',size=(27,22),pos=(self.rx+120,self.ry+40+(i%5)*self.i_y_offset),style=wx.RAISED_BORDER|wx.ALIGN_CENTER|wx.ST_NO_AUTORESIZE)
      #self.BERT[i].widgets['cdrLock'].regStr = self.BERT[i].regs['cdrLock']
      #self.BERT[i].widgets['cdrLock'].name = 'cdrLock'
      
      font=wx.Font(12, wx.FONTFAMILY_DEFAULT,wx.NORMAL,wx.BOLD)  
      berText = wx.StaticText(self.panel,-1,"BER",pos=(self.rx+170,self.ry+17+(i%5)*self.i_y_offset))
      berText.SetFont(font)
      self.BERT[i].ber        = wx.StaticText(self,-1,'',size=(85,26),pos=(self.rx+205,self.ry+15+(i%5)*self.i_y_offset),style=wx.RAISED_BORDER|wx.ALIGN_RIGHT|wx.ST_NO_AUTORESIZE)
      self.BERT[i].ber.SetFont(font)
      self.BERT[i].ber.SetLabel(str(0))
      font=wx.Font(8, wx.FONTFAMILY_DEFAULT,wx.NORMAL,wx.BOLD)
      self.BERT[i].saturationWarning = wx.StaticText(self,-1,'Saturated',size=(70,22),pos=(self.rx+149,self.ry+40+(i%5)*self.i_y_offset),style=wx.RAISED_BORDER|wx.ALIGN_CENTRE|wx.BOLD|wx.ST_NO_AUTORESIZE)
      self.BERT[i].saturationWarning.SetBackgroundColour(wx.Colour(200,50,0))
      self.BERT[i].saturationWarning.SetForegroundColour('WHITE')
      self.BERT[i].saturationWarning.SetFont(font)
      self.BERT[i].saturationWarning.Show(False)
    self.parent.Refresh()


  def qphase2pcode(self,quad,phase,device="None"):
    """converts a quad/phase value into a 0-512 wheel value"""
    if device.find("A0")>-1:
        codeBin = (quad << 8) + phase
        codeBin1 = bin(codeBin)[2:].zfill(10)
        codeBin2 = codeBin1[0] + codeBin1[2:]
        code=int(codeBin2,2)
    else:
        if quad==0:
          if phase==247:
            code=128
          elif phase>247:
            code=phase-248
          else:
            code=int(phase/8)*4+4+phase%4
        elif quad==1:
          if phase>247:
            code=504-phase
          else:
            code=252-4*int(phase/8)-phase%4
        elif quad==3:
          if phase==247:
            code=384
          elif phase>248:
            code=phase+8
          else:
            code=4*int(phase/8)+phase%4+260
        else:
          if phase>248:
            code=760-phase
          else:
            code=508-4*int(phase/8)-phase%4

    return(code)

  def pcode2qp(self,code,device="None"):
    """returns a q p  code from reading the code input"""
    if device.find("A0")>-1: 
      codeBin1 = bin(code)[2:].zfill(10)
      codeBin2 = codeBin1[0] + codeBin1[2:]
      phase=int(codeBin2,2)
      quad=int(codeBin1[0:2],2)

    else:
      phase1=bin(code)[2:].zfill(10)
      phase=int(phase1[2:],2)
      quad1=phase1[0:2]
      quad=int(quad1,2)
    return(quad,phase)

  def pcode2code (self,code):
    (q,p) = self.pcode2qp(code)
    return(self.qphase2pcode(q,p))
    
  def ChangeDevice(self,event):
    global regs
    print "Device Changed to",event.String 
    self.device = event.String

    for i in range(0,4):
      self.BERT[i].regs = {}    
      if self.device.find("GB") == 0:
        self.BERT[i].regs['liveErrCount']  = self.device + '::8.' + str(1700  + (i)) + '.15:0'
      else:
        self.BERT[i].regs['liveErrCount']  = self.device + '::30.' + str(48  + (i)) + '.15:0'
        
      self.BERT[i].regs['prbsLock']        = self.device + '::30.' + str(16  + (i))
      self.BERT[i].regs['piLock']          = self.device + '::30.' + str(416 + (i*256)) + '.1'
      self.BERT[i].regs['cdr2Trip']        = self.device + '::30.' + str(423 + (i*256)) # + '.13:0'
      self.BERT[i].regs['ctle_state']      = self.device + '::30.' + str(421 + (i*256)) + '.3:0'
      self.BERT[i].regs['ctle_lock']      = self.device + '::30.' + str(387 + (i*256)) + '.0'      
      #self.BERT[i].regs['ctle_override']   = self.device + '::30.' + str(420 + (i*256)) + '.0'

      self.BERT[i].widgets['liveErrCount'].regStr = self.BERT[i].regs['liveErrCount']      
      self.BERT[i].widgets['prbsLock'].regStr = self.BERT[i].regs['prbsLock']
      self.BERT[i].widgets['piLock'].regStr = self.BERT[i].regs['piLock']
      self.BERT[i].widgets['ctle_state'].regStr = self.BERT[i].regs['ctle_state']
      self.BERT[i].widgets['ctle_lock'].regStr = self.BERT[i].regs['ctle_lock']
      #self.BERT[i].widgets['ctle_override'].regStr = self.BERT[i].regs['ctle_override']

      self.OnStop(True)
    
  def OnTimer(self,event):
      self.ReadTheWidgetRegisters()      
      self.UpdateWidgets()
      self.Refresh()
 
  def ReadTheWidgetRegisters(self,zeroThem=False):
    for i in range (0,4):
      piCodeStr  = self.device + '::30.' + str(424+(i*256)) + '.9:0'
      cdr2CodeStr= self.device + '::30.' + str(426+(i*256)) + '.4:0'
      trip       = self.device + '::30.' + str(423+(i*256)) + '.13:0'
      for eachReg in self.BERT[i].regs:
        #print "read",eachReg,self.device,reg
		
        (device, reg, hiLsb, loLsb) = re.search('(.*)::(\d+\.\d+)\.*(\d*):*(\d*)',self.BERT[i].regs[eachReg]).groups()
        device = self.device
        #print "read",eachReg,self.device,reg
        val = ipallreg.regRead(device+'::'+reg)
        if zeroThem:
          ipvar.System[device].regDefTab[reg].lastRead = 0
        else:
          ipvar.System[device].regDefTab[reg].lastRead = val
# Get the 1st order Hist data
      self.BERT[i].cdrMin = 512
      self.BERT[i].cdrMax = 0

      codeBin = ipallreg.regRead(piCodeStr)
      if ipvar.System[self.device].regDefTab['30.3'].defVec.value & 0xf == 0:   # equals A0 device
        codeBin1 = bin(codeBin)[2:].zfill(10)
        codeBin2 = codeBin1[0] + codeBin1[2:]
        code = int(codeBin2,2)
      else:                           # everything else (not A0)
        code = self.pcode2code(codeBin)
      if code > self.BERT[i].cdrMax:
        self.BERT[i].cdrMax = code
      if code < self.BERT[i].cdrMin:
        self.BERT[i].cdrMin = code
      self.BERT[i].cdrpiLatest = code
      self.BERT[i].cdrHist[code] = self.BERT[i].cdrHist[code]+1  
# Get the 2nd order data
      self.BERT[i].trip = 8192-ipallreg.regRead(trip)
      code = ipallreg.regRead(cdr2CodeStr)
      if code < 16:
        code = -code
      else:
        code = code-16
        
      (self.BERT[i].cdr2Hist[0],self.BERT[i].cdr2Hist[1:10]) = (code,self.BERT[i].cdr2Hist[0:9])
            
  def UpdateWidgets(self): 
    now = time.time()
    timeSpan = now - self.time
    if (self.go1hr == 1) and timeSpan > 60*60:
      print "Timeout: 1 hour test complete"
      self.timer.Stop()
      self.go1hr = 0
      self.goButton.SetLabel('Start Monitor')
      
    secs  = int(timeSpan % 60)
    mins  = (int((timeSpan - secs)/60) % 60)
    hours = (int(timeSpan/(60*60))) % 24
    days  = (int(timeSpan/(60*60*24)))    
    self.timeText.SetLabel('%dd %02d:%02d:%02d' % (days,hours,mins,secs))
         
    for rxn in range(0,4):
      for eachWidget in self.BERT[rxn].widgets:
        val = self.getLastRead(self.BERT[rxn].widgets[eachWidget].regStr)
        #print eachWidget,self.BERT[i].widgets[eachWidget].regStr,val
  
        if self.BERT[rxn].widgets[eachWidget].name.find('liveErrCount') == 0: 
          self.BERT[rxn].widgets[self.BERT[rxn].widgets[eachWidget].name].SetLabel(str(val))
          if int(val) == 65535:
            self.BERT[rxn].errorSaturation = 1
            self.BERT[rxn].saturationWarning.Show(True)
          
        elif self.BERT[rxn].widgets[eachWidget].name.find('prbsLock') == 0: 
          #verifiy enable 16-19[12]
          if(val & 0x1000):
            verify=True
            #auto verify 16-19[14]
            if (val & 0x4000):
              label = "A-"
            else:
              label = ""
            #ver invert 16-19[11]
            if (val & 0x800):
              label = label+'!'
            #ver pattern 16-19[10:8]
            label = label + self.pattern_labels[(val&0x700) >> 8]
            #label = self.pattern_labels[((val&(7<<8))/(1<<8))]
            self.BERT[rxn].widgets[self.BERT[rxn].widgets[eachWidget].name].SetLabel(label) 
            #self.BERT[rxn].widgets[self.BERT[rxn].widgets[eachWidget].name].SetLabel('PRBS31')
            #PRBS Lock [15]
            if val&(1<<15) == 1<<15:
              self.BERT[rxn].widgets[self.BERT[rxn].widgets[eachWidget].name].SetBackgroundColour('PALE GREEN')
            else:
              self.BERT[rxn].widgets[self.BERT[rxn].widgets[eachWidget].name].SetBackgroundColour(wx.Colour(200,50,0))
              #self.BERT[rxn].widgets[self.BERT[rxn].widgets[eachWidget].name].SetLabel('SYNC Loss')
              self.BERT[rxn].syncLoss = 1
          else:  #[12] - ver off
            verify=False
            label = 'Off'
            self.BERT[rxn].widgets[self.BERT[rxn].widgets[eachWidget].name].SetBackgroundColour(wx.NullColor)
          #Pattern Label
          self.BERT[rxn].widgets[self.BERT[rxn].widgets[eachWidget].name].SetLabel(label)             
        elif self.BERT[rxn].widgets[eachWidget].name.find('ctle_state') == 0: 
          self.BERT[rxn].widgets[self.BERT[rxn].widgets[eachWidget].name].SetLabel(str(val))
        elif self.BERT[rxn].widgets[eachWidget].name.find('ctle_lock') == 0: 
          if val == 0:
            #print 'val is 0, setting False',rxn,self.BERT[rxn].widgets['ctle_lock'].name,eachWidget
            self.BERT[rxn].widgets['ctle_lock'].SetValue(True)
          else:
            #print 'val is 1, setting True',rxn,self.BERT[rxn].widgets['ctle_lock'].name,eachWidget
            self.BERT[rxn].widgets['ctle_lock'].SetValue(False)
                
      val = int(self.BERT[rxn].widgetAccumulateError.GetLabel()) + int(self.BERT[rxn].widgets['liveErrCount'].GetLabel())
      # Update Total Errors
      self.BERT[rxn].widgetAccumulateError.SetLabel(str(val))
      bits = (time.time() - self.time) * float(self.BERT[rxn].rate)*1e9
      self.bitsWidget.SetLabel("%05.03g" % bits)
      #Update BER
      if(verify):
        ber = int(self.BERT[rxn].widgetAccumulateError.GetLabel())/bits
        if ber == 0:
          ber = 1.0/bits
        self.BERT[rxn].ber.SetLabel("%05.03g" % ber)
        if (val == 0) and (self.BERT[rxn].syncLoss == 0) and (self.BERT[rxn].errorSaturation == 0):
          self.BERT[rxn].ber.SetBackgroundColour('PALE GREEN')
        elif (self.BERT[rxn].syncLoss == 1) or (self.BERT[rxn].errorSaturation == 1):
          self.BERT[rxn].ber.SetBackgroundColour(wx.Colour(200,50,0)) 
        else:
          self.BERT[rxn].ber.SetBackgroundColour(wx.Colour(250,250,100)) 
      else:
        self.BERT[rxn].ber.SetLabel('')
        self.BERT[rxn].ber.SetBackgroundColour(wx.NullColour)
        
    self.Refresh()    
    self.parent.Refresh()  
  def OnCtleLock(self,event):
    val = self.getLastRead(event.EventObject.regStr)
    if event.EventObject.GetValue():
      ipallreg.regWrite(event.EventObject.regStr,0)
    else:
      ipallreg.regWrite(event.EventObject.regStr,1)
    self.ReadTheWidgetRegisters()
      
  def OnGo(self,event):
    self.ReadTheWidgetRegisters()
    for rxn in range(0,4):
      self.BERT[rxn].widgetAccumulateError.SetLabel(str(0))
      self.BERT[rxn].ber.SetBackgroundColour('WHITE')
      self.BERT[rxn].rate = self.rateBox.GetValue()
      self.BERT[rxn].saturationWarning.Show(False)
      self.BERT[rxn].syncLoss = 0
      self.BERT[rxn].errorSaturation = 0
      self.BERT[rxn].cdrpiLatest = 256  # last value read from the PI
      self.BERT[rxn].cdrHist = [0]*513  # zero init the histogram of the PI
      self.BERT[rxn].cdr2Hist = [0]*10
      self.BERT[rxn].trip = 0
    self.go1hr = 0  
    self.timer.Start(400)
    self.time = time.time()
    self.goButton.SetLabel('Restart Monitor')
    
    #self.UpdateWidgets()
    
  def OnGo1hr(self,event):
    self.ReadTheWidgetRegisters()
    for rxn in range(0,4):
      self.BERT[rxn].widgetAccumulateError.SetLabel(str(0))
      self.BERT[rxn].ber.SetBackgroundColour('WHITE')
      self.BERT[rxn].rate = self.rateBox.GetValue()
      self.BERT[rxn].saturationWarning.Show(False)
      self.BERT[rxn].syncLoss = 0
      self.BERT[rxn].errorSaturation = 0
      self.BERT[rxn].cdrpiLatest = 256  # last value read from the PI
      self.BERT[rxn].cdrHist = [0]*513  # zero init the histogram of the PI
      self.BERT[rxn].cdr2Hist = [0]*10
      self.BERT[rxn].trip = 0
    self.go1hr = 1  
    self.timer.Start(400)
    self.time = time.time()
    self.goButton.SetLabel('Restart Monitor')
    
  def OnPattern(self,event):
    print "Pattern button hit",event.Id,self.pattern_id_map[event.Id]
    
  def OnPiLock(self,event):
    val = ipallreg.regRead(self.BERT[3].regs['piLock'])
    print "Lock/unlock PI3. Was",val,'changing to',1-val
    ipallreg.regWrite(self.BERT[3].regs['piLock'],1-val)
    
  def OnStop(self,event):
    self.timer.Stop()
    self.go1hr = 0
    self.goButton.SetLabel('Start Monitor')
    
  def OnPaint(self,event):
    dc = wx.PaintDC(self)
    font=wx.Font(9, wx.FONTFAMILY_DEFAULT,wx.NORMAL,wx.NORMAL)
    dc.SetFont(font)

    if self.TopLevelParent.frame.show_phaseInt.IsChecked():
      radius = self.i_y_offset/3.0
      for i in range(0,4):
        val = ipallreg.regRead(self.BERT[i].regs['piLock'])
        if val == 1:
          dc.SetPen(wx.Pen(wx.Colour(200,50,0),3)) 
        else:
          dc.SetPen(wx.Pen(wx.Colour(200,200,200),3))
        dc.DrawCircle(self.rx+360,self.ry+self.i_y_offset*i+self.i_y_offset/2,radius)
        dc.DrawCircle(self.rx+360,self.ry+self.i_y_offset*i+self.i_y_offset/2,10)
        
        for pi in range(0,513):
          if self.BERT[i].cdrHist[pi] > 0:
            x1 = 10*math.sin((pi/512.0)*math.pi * 2.0)
            y1 = 10*math.cos((pi/512.0)*math.pi * 2.0)            
            x2 = 30*math.sin((pi/512.0)*math.pi * 2.0)
            y2 = 30*math.cos((pi/512.0)*math.pi * 2.0)    
            dc.SetPen(wx.Pen(wx.Colour(180,180,180),2)) 
            dc.DrawLine(self.rx+360+x1,self.ry+self.i_y_offset*i+self.i_y_offset/2+y1,self.rx+360+x2,self.ry+self.i_y_offset*i+self.i_y_offset/2+y2)
        x1 = 10*math.sin((self.BERT[i].cdrpiLatest/512.0)*math.pi * 2.0)
        y1 = 10*math.cos((self.BERT[i].cdrpiLatest/512.0)*math.pi * 2.0)  
        x2 = 30*math.sin((self.BERT[i].cdrpiLatest/512.0)*math.pi * 2.0)
        y2 = 30*math.cos((self.BERT[i].cdrpiLatest/512.0)*math.pi * 2.0)     
        dc.SetPen(wx.Pen("BLACK",2)) 
        dc.DrawLine(self.rx+360+x1,self.ry+self.i_y_offset*i+self.i_y_offset/2.0+y1,self.rx+360+x2,self.ry+self.i_y_offset*i+self.i_y_offset/2.0+y2)          
# 2nd Order CDR
        #val = ipallreg.regRead(self.BERT[i].regs['cdr2Trip'])
        cdr2 = self.BERT[i].cdr2Hist[0]
        trip = self.BERT[i].trip
        dc.SetPen(wx.Pen(wx.Colour(0,200,0),3))
        dc.DrawRectangle(self.rx+405,self.ry+self.i_y_offset*i+self.i_y_offset*0.2,15,self.i_y_offset*0.6)

        dc.SetPen(wx.Pen(wx.Colour(200,0,0),3))
        dc.DrawRectangle(self.rx+405,self.ry+self.i_y_offset*i+self.i_y_offset*0.1,15,self.i_y_offset*0.15)
        dc.DrawRectangle(self.rx+405,self.ry+self.i_y_offset*i+self.i_y_offset*0.75,15,self.i_y_offset*0.15)
        
        real_ppm = cdr2*20+trip/5.5
        dc.DrawText('%3.0f ppm' % real_ppm,self.rx+425,self.ry+self.i_y_offset*i+self.i_y_offset/2.0)
        dc.SetPen(wx.Pen(wx.Colour(180,180,180),2)) 
        #for cdr2 in self.BERT[i].cdr2Hist:
        #  myY = self.ry+self.i_y_offset*i+self.i_y_offset/2.0+(-cdr2/16.0)*self.i_y_offset*0.4
        #  dc.DrawLine(self.rx+405,myY,self.rx+420,myY)
        dc.SetPen(wx.Pen("BLACK",2))

        myY = self.ry+self.i_y_offset*i+self.i_y_offset/2.0+ (-real_ppm/200.0)*self.i_y_offset*0.25 
        dc.DrawLine(self.rx+403,myY,self.rx+422,myY)

    else:
      pass
    
  def getLastRead(self,registerStr):
    (device, reg, hiLsb, loLsb) = re.search('(.*)::(\d+\.\d+)\.*(\d*):*(\d*)',registerStr).groups()
    nBits = 16
    if loLsb:    # the register is a sub-bit range
      nBits = int(hiLsb) - int(loLsb) + 1
    elif hiLsb:
      nBits = 1
      loLsb = hiLsb
    else:
      loLsb = 0
      hiLsb = 15
       
    loLsb = int(loLsb)
    hiLsb = int(hiLsb)
    #print 'get last read:',device, reg, hiLsb, loLsb
    valPassing = ipvar.System[device].regDefTab[reg].lastRead
    myBin = bin(valPassing)[2:].zfill(16) + 'x'
    
    subregVal = int(myBin[16 - loLsb - nBits:16 - loLsb],2) 
    return(subregVal)

    