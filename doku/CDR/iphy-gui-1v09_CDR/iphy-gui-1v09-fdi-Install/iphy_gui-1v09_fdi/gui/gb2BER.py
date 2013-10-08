#Dec 6, 2012 - Enable Force/Lock CTLE
import sys
import wx
import re
import ipvar
import ipallreg
import time



class BERT():
  def __init__(self, *args, **kwargs):
    self.accumulateError = 0
    self.syncLoss = 0
    self.errorSaturation = 0
    self.regs = {}
    self.eye_x_val = 0
    self.eye_y_val = 0
    #CTLE
    self.ctle = 0
    self.piLock = 0
    self.ctleState = 0
    self.ctleLock = 0
    
    #PI Codes
    self.cdrpiLatest = 256  # last value read from the PI
    self.cdrHist = [0]*513  # zero init the histogram of the PI
    self.cdr2Hist = [0]*10  # history of cdr2 values  
    self.trip = 0    
    
  def setRate(self,String):
    print "Rate set to",String    
  
class BERPanel(wx.Panel):
  """  """
  def __init__(self, parent, interface,GB1_sim=False):
    """   """
    wx.Panel.__init__(self, parent)
    self.parent = parent
    self.panel = self

    self.Bind(wx.EVT_PAINT,self.OnPaint)
    self.time = 0
    self.numberOfBits = 0
    self.haltRead = 0

    #Interface Mac, Line
    #
    #print 'gb2BER - Interface: %s' % interface
    if(interface == 'Host'):
      instance_id = 'GB2-H'
      choice_list=['10.3125','11.18']
      pat_ctrl_offset = 0x20
      errcnt_offset = 1600
      self.base_offset = 0x2080
      TIMER_ID = 3000


      

    elif(interface == 'Opt'):
      instance_id = 'GB2-O'
      choice_list=['25.78125','27.952']
      pat_ctrl_offset = 0x10    
      errcnt_offset = 1700
      self.base_offset = 0x1080
      TIMER_ID = 4000
      #----- Test Mode ----
      if('-GB1_sim' in sys.argv[1:]):
        caption = 'Warning!'
        message = 'BERT-Opt TEST MODE - Use GB1 base offset (0x180)'
        dlg = wx.MessageDialog(parent, message, caption, wx.OK| wx.ICON_WARNING)
        dlg.ShowModal()   
        dlg.Destroy()
        self.base_offset = 0x180
        print 'TEST MODE BER-Optical - Base Offset: 0x%04x'% self.base_offset
          
 
      
    self.adapt_IDs = [-1]*10
    self.pattern_labels = ['PRBS31','PRBS9','res','res','PRBS31','PRBS23','PRBS15','PRBS7']
    
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
    
    
    self.forceButton = wx.Button(self,-1,label="TBD",pos=(340,10),size=(75,24))
    self.forceButton.controls = self
    self.forceButton.Show(False)

    self.adaptButton = wx.Button(self,-1,label="TBD",pos=(405,10),size=(75,24))
    self.adaptButton.controls = self
    self.adaptButton.Show(False)
    
    self.statusButton = wx.Button(self,-1,label="TBD",pos=(405+75,10),size=(75,24))
    self.statusButton.controls = self
    self.statusButton.Show(False)
    
    self.eyeButton = wx.Button(self,-1,label="TBD",pos=(340,40),size=(120,24))
    self.eyeButton.controls = self
    self.eyeButton.Show(False)
    
    self.goButton.Bind(wx.EVT_BUTTON,self.OnGo)
    self.stopButton.Bind(wx.EVT_BUTTON,self.OnStop)
    #self.adaptButton.Bind(wx.EVT_BUTTON,self.OnAdaptAll)
    #self.statusButton.Bind(wx.EVT_BUTTON,self.OnEqStatus)    
    #self.forceButton.Bind(wx.EVT_BUTTON,self.OnEqForce)    
    #self.eyeButton.Bind(wx.EVT_BUTTON,self.OnEyeRefresh)    
    # *** END BUTTONS  ***********************************************
    
    rx=0
    ry=-15
    font=wx.Font(10, wx.FONTFAMILY_DEFAULT,wx.NORMAL,wx.BOLD)

    self.timeText = wx.StaticText(self,-1,'0d 00:00:00',size=(110,24),pos=(rx+120,ry+25),style=wx.RAISED_BORDER|wx.ALIGN_CENTRE|wx.ST_NO_AUTORESIZE)
    self.timeText.SetFont(font)
    
    #----- ORIG ----
    #self.checkboxDevice=[]
    #Devices = ['GB0']
    #self.device = Devices[0]    
    #self.deviceRB = wx.RadioBox(self, label="Device", pos=(rx+250, ry+20), choices=Devices,  majorDimension=0)
    #---------------
    #print '-' * 80
    #print ipvar.System.keys()    
    self.checkboxDevice=[]

    
	  #Select GB2 Devices only
    self.GB2_list = []
    
   
    #gjl - version
    #for eachDevice in ipvar.System.keys():
    #  if (re.search(r'\w+-\w+',eachDevice)):
    #    pass
    #  else:
    #    gb2 = re.search(r'(GB2\w*)',eachDevice)
    #    if(gb2) :
    #      #print 'GB2: %s'%gb2.group(1)
    #      self.GB2_list.append(gb2.group(1))               
    #rw - version
    for eachDevice in ipvar.System.keys():
      if "_" in eachDevice:
        pass
      else:
        if (re.search(r'\w+-\w+',eachDevice)):
          pass
        else:
          gb2 = re.search(r'(GB2\w*)',eachDevice)
          if(gb2) :
            #print 'GB2: %s'%gb2.group(1)
            self.GB2_list.append(gb2.group(1))      

      
    #print 'gb2BER: %s'%self.GB2_list
                
    self.device = self.GB2_list[0]    	
	
    self.m_deviceBox = wx.RadioBox(self, label="Device", pos=(rx+250, ry+20), choices=self.GB2_list,  majorDimension=1)
    self.Bind(wx.EVT_RADIOBOX, self.ChangeDevice,id=self.m_deviceBox.GetId())
    #--------------------------------------------------------------------------
    #Rate Choices
    choices=choice_list
    self.rateBox  = wx.ComboBox(self.panel,-1,value=choices[0],choices=choices,pos=(rx+120,ry+55),style=wx.ALIGN_RIGHT|wx.ST_NO_AUTORESIZE,name='rateCombo',size=(75,27))
    rateText = wx.StaticText(self.panel,-1,"(Gbps)",pos=(rx+200,ry+57),size=(-1,24))
    font=wx.Font(9, wx.FONTFAMILY_DEFAULT,wx.NORMAL,wx.BOLD)
    rateText.SetFont(font)
    bitsText = wx.StaticText(self.panel,-1,"Bits",pos=(rx+200,ry+82))
    bitsText.SetFont(font)
    self.bitsWidget = wx.StaticText(self,-1,'0',size=(75,22),pos=(rx+120,ry+80),style=wx.RAISED_BORDER|wx.ALIGN_RIGHT|wx.ST_NO_AUTORESIZE)

    
    #self.button_all  = wx.Button(self,-1,'All',pos=(rx+540,ry+82),size=(24,22))
    #rx=rx+ 26
    #self.button_none = wx.Button(self,-1,'None',pos=(rx+540,ry+82),size=(32,22)) 
    
    #self.Bind(wx.EVT_BUTTON,self.select_all,id=self.button_all.GetId())
    #self.Bind(wx.EVT_BUTTON,self.select_none,id=self.button_none.GetId())
    
    self.button_all  = wx.Button(self,-1,'All',pos=(545,ry+82),size=(24,22))
    #self.button_all  = wx.Button(self,-1,'All',pos=(500,ry+82),size=(24,22))
    #self.button_locked  = wx.Button(self,-1,'Locked',pos=(526,ry+82),size=(42,22))    
    self.button_none = wx.Button(self,-1,'None',pos=(570,ry+82),size=(33,22)) 
    
    self.Bind(wx.EVT_BUTTON,self.select_all,id=self.button_all.GetId())
    #self.Bind(wx.EVT_BUTTON,self.select_locked,id=self.button_locked.GetId())
    self.Bind(wx.EVT_BUTTON,self.select_none,id=self.button_none.GetId())
    
    
    #self.stopButton.Enable(False)
    self.stop = 0  
    #self.widgets={} # a list of widgets that get updated from reg values
    #self.regs={}    # a list of registers required to be read for update of this panel
    self.BERT = []
           
    self.rx=5
    self.ry=90
    self.i_y_offset = 94
    self.pattern_id_map={} 
    self.piLock_id_map={} 
    self.ctleState_id_map={} 
    self.ctleLock_id_map={} 
    self.checkbox_id_map={}
    self.device = self.m_deviceBox.GetStringSelection() 
    j = [0,2,4,6,8,1,3,5,7,9]    
    for i in range (10):
      #Shift to second column for BERT 5-9
      #if (i>4):
      if (i%2):
        self.rx=307
      else:
        self.rx=5
       
      self.BERT.append(BERT())
      self.BERT[i].pattern_id = wx.NewId()
      self.BERT[i].piLock_id = wx.NewId()
      self.BERT[i].ctleState_id = wx.NewId()
      self.BERT[i].ctleLock_id = wx.NewId()
      
      self.pattern_id_map[self.BERT[i].pattern_id] = i
      
      self.piLock_id_map[self.BERT[i].piLock_id]       = i
      self.ctleState_id_map[self.BERT[i].ctleState_id] = i
      self.ctleLock_id_map[self.BERT[i].ctleLock_id]   = i

      self.BERT[i].widgets = {}
      #if self.device.find("GB") == 0:
      self.BERT[i].regs['liveErrCount']  = self.device + '::8.' + str(errcnt_offset  + (i)) + '.15:0'
      self.BERT[i].regs['prbsLock']      = self.device + '::30.' + str(pat_ctrl_offset  + (i))
      #self.BERT[i].regs['ctle']          = self.device + '::30.' + str(self.base_offset  + (i*0x100)+37) +'.3:0'
      #self.BERT[i].regs['cdrLock']       = self.device + '::30.1546.' + str(i)
      #self.BERT[i].regs['pattern']       = self.device + '::30.' + str(20 + i)
      #else:
      #  self.BERT[i].regs['liveErrCount']  = self.device + '::30.' + str(48  + (i)) + '.15:0'
      #  self.BERT[i].regs['prbsLock']        = self.device + '::30.' + str(16  + (i)) + '.15'
      #self.BERT[i].regs['cdrLock']         = self.device + '::30.1546.' + str(i)
      

      self.staticBox3   = wx.StaticBox(self,-1,label='RX'+str(i),pos=(self.rx, self.ry+(i/2)*self.i_y_offset),style=wx.RAISED_BORDER|wx.BOLD,size=(297,self.i_y_offset-2))

      wx.StaticText(self.panel,-1,"Errors", pos=(self.rx+19,self.ry+20+(i/2)*self.i_y_offset)).SetFont(font)
      wx.StaticText(self.panel,-1,"Pattern",pos=(self.rx+9, self.ry+43+(i/2)*self.i_y_offset)).SetFont(font)
      wx.StaticText(self.panel,-1,"CTLE",   pos=(self.rx+25,self.ry+65+(i/2)*self.i_y_offset)).SetFont(font)
      #wx.StaticText(self.panel,-1,"ppm",    pos=(self.rx+100,self.ry+63+(i/2)*self.i_y_offset)).SetFont(font)


      
      self.BERT[i].widgets['liveErrCount'] = wx.StaticText(self,-1,'0',size=(45,22),pos=(self.rx+58,self.ry+17+(i/2)*self.i_y_offset),style=wx.RAISED_BORDER|wx.ALIGN_RIGHT|wx.ST_NO_AUTORESIZE)
      
      self.BERT[i].widgets['liveErrCount'].regStr = self.BERT[i].regs['liveErrCount']
      self.BERT[i].widgets['liveErrCount'].name = 'liveErrCount'
      
      self.BERT[i].widgetAccumulateError = wx.StaticText(self,-1,'0',size=(60,22),pos=(self.rx+103,self.ry+17+(i/2)*self.i_y_offset),style=wx.RAISED_BORDER|wx.ALIGN_RIGHT|wx.ST_NO_AUTORESIZE)


      #gjl - pattern button
      self.BERT[i].widgets['prbsLock'] = wx.Button(self,self.BERT[i].pattern_id,label=" ",size=(60,22),pos=(self.rx+58,self.ry+40+(i/2)*self.i_y_offset),style=wx.RAISED_BORDER|wx.ALIGN_RIGHT|wx.ST_NO_AUTORESIZE)
      self.BERT[i].widgets['prbsLock'].regStr = self.BERT[i].regs['prbsLock']
      self.BERT[i].widgets['prbsLock'].name = 'prbsLock'
      self.BERT[i].widgets['prbsLock'].Bind(wx.EVT_BUTTON,self.OnPattern,id=self.BERT[i].pattern_id)

      #self.BERT[i].widgets['ctle'] = wx.StaticText(self,-1,'',size=(25,22),pos=(self.rx+58,self.ry+63+(i/2)*self.i_y_offset),style=wx.RAISED_BORDER|wx.ALIGN_CENTER|wx.ST_NO_AUTORESIZE)
      #self.BERT[i].widgets['ctle'].regStr = self.BERT[i].regs['ctle']
      
      choices=['0','1','2','3','4','5','6','7','8','9','10','11','12','13','14']
      self.BERT[i].widgets['ctleState']  = wx.ComboBox(self,self.BERT[i].ctleState_id,value=choices[0],choices=choices,pos=(self.rx+58,self.ry+63+(i/2)*self.i_y_offset),style=wx.RAISED_BORDER|wx.ALIGN_CENTER|wx.ST_NO_AUTORESIZE,name='rateCombo',size=(35,22))
      self.BERT[i].widgets['ctleState'].name = 'ctleState'   
      self.BERT[i].widgets['ctleState'].Bind(wx.EVT_COMBOBOX,self.OnCtleForce,id=self.BERT[i].ctleState_id)        

      
      self.BERT[i].widgets['ctleLock']   = wx.CheckBox(self,self.BERT[i].ctleLock_id,label='Lock',pos=(self.rx+58+37,self.ry+67+(i/2)*self.i_y_offset))
      self.BERT[i].widgets['ctleLock'].name = 'ctleLock'      
      self.BERT[i].widgets['ctleLock'].Bind(wx.EVT_CHECKBOX,self.OnCtleLock,id=self.BERT[i].ctleLock_id)        
      
      
      self.BERT[i].widgets['ppm'] = wx.StaticText(self,-1,'ppm',size=(50,22),pos=(self.rx+140,self.ry+63+(i/2)*self.i_y_offset),style=wx.RAISED_BORDER|wx.ALIGN_CENTER|wx.ST_NO_AUTORESIZE)
      self.BERT[i].widgets['ppm'].name = 'ppm'     
      
      self.BERT[i].widgets['piLock'] = wx.Button(self,self.BERT[i].piLock_id,'PI Lock',size=(50,22),pos=(self.rx+210,self.ry+63+(i/2)*self.i_y_offset))
      self.BERT[i].widgets['piLock'].name = 'piLock'   
      self.BERT[i].widgets['piLock'].Bind(wx.EVT_BUTTON,self.OnpiLock,id=self.BERT[i].piLock_id)
      
      
      #self.BERT[i].widgets['cdrLock'] = wx.StaticText(self,-1,'CDR',size=(27,22),pos=(self.rx+120,self.ry+40+(i/2)*self.i_y_offset),style=wx.RAISED_BORDER|wx.ALIGN_CENTER|wx.ST_NO_AUTORESIZE)
      #self.BERT[i].widgets['cdrLock'].regStr = self.BERT[i].regs['cdrLock']
      #self.BERT[i].widgets['cdrLock'].name = 'cdrLock'
      
      font=wx.Font(12, wx.FONTFAMILY_DEFAULT,wx.NORMAL,wx.BOLD)  
      berText = wx.StaticText(self.panel,-1,"BER",pos=(self.rx+170,self.ry+17+(i/2)*self.i_y_offset))
      berText.SetFont(font)
      self.BERT[i].ber        = wx.StaticText(self,-1,'',size=(85,26),pos=(self.rx+205,self.ry+16+(i/2)*self.i_y_offset),style=wx.RAISED_BORDER|wx.ALIGN_RIGHT|wx.ST_NO_AUTORESIZE)
      self.BERT[i].ber.SetFont(font)
      self.BERT[i].ber.SetLabel(str(0))
      font=wx.Font(8, wx.FONTFAMILY_DEFAULT,wx.NORMAL,wx.BOLD)
      self.BERT[i].saturationWarning = wx.StaticText(self,-1,'Saturated',size=(70,22),pos=(self.rx+149,self.ry+40+(i/2)*self.i_y_offset),style=wx.RAISED_BORDER|wx.ALIGN_CENTRE|wx.BOLD|wx.ST_NO_AUTORESIZE)


      self.BERT[i].saturationWarning.SetBackgroundColour(wx.Colour(200,50,0))
      self.BERT[i].saturationWarning.SetForegroundColour('WHITE')
      self.BERT[i].saturationWarning.SetFont(font)
      self.BERT[i].saturationWarning.Show(False)
      #self.prbsLock.SetBackgroundColour('WHITE')

      #self.BERT[i].eye_x = wx.StaticText(self,-1,str(self.BERT[i].eye_x_val)+'UI',size=(60,22),pos=(self.rx+58,self.ry+63+(i/2)*self.i_y_offset),style=wx.RAISED_BORDER|wx.ALIGN_RIGHT|wx.ST_NO_AUTORESIZE)
      #self.BERT[i].eye_y = wx.StaticText(self,-1,str(self.BERT[i].eye_y_val)+'mV',size=(60,22),pos=(self.rx+58+62,self.ry+63+(i/2)*self.i_y_offset),style=wx.RAISED_BORDER|wx.ALIGN_RIGHT|wx.ST_NO_AUTORESIZE)
      
      
      self.BERT[i].checkbox_id = wx.NewId()
      self.checkbox_id_map[self.BERT[i].checkbox_id] = i
      self.BERT[i].checkbox = wx.CheckBox(self,self.BERT[i].checkbox_id,'',pos=(self.rx+280,self.ry+75+(i/2)*self.i_y_offset))
      self.Bind(wx.EVT_CHECKBOX,self.pattern_ver,id=self.BERT[i].checkbox_id)
      #self.Bind(wx.EVT_CHECKBOX,self.pattern_ver,id=self.BERT[i].GetId())


      #Enable Pattern Verification for All
      self.BERT[i].checkbox.SetValue(1)
      #address = self.device+'::30.'+ str(20+i)+'.12:12'
      #ipallreg.regWrite(address,1)
      
    #self.parent.cdrMonitorPanel = cdrMonitorPanel(self.notebook, -1)
    #
    #print "Updating BERT[i].widgets"
    #self.UpdateBERT[i].widgets()
    self.parent.Refresh()
 
  def ChangeDevice(self,event):
    global regs
    #print "Device Changed",event.String 
    #self.device = event.String
    self.device = self.m_deviceBox.GetStringSelection()   
    print "Device Changed",self.device 
    for i in range(0,10):
      self.BERT[i].regs = {}    
      if self.device.find("GB") == 0:
        self.BERT[i].regs['liveErrCount']  = self.device + '::8.' + str(1600  + (i)) + '.15:0'   # Host (Tx lane)
        self.BERT[i].regs['prbsLock']      = self.device + '::30.' + str(32  + (i))        # Host (Tx lane)
        #self.BERT[i].regs['cdrLock']       = self.device + '::30.1546.' + str(i)
      else:
        self.BERT[i].regs['liveErrCount']  = self.device + '::30.' + str(48  + (i)) + '.15:0'
        self.BERT[i].regs['prbsLock']        = self.device + '::30.' + str(16  + (i)) + '.15'


      self.BERT[i].widgets['liveErrCount'].regStr = self.BERT[i].regs['liveErrCount']
      self.BERT[i].widgets['prbsLock'].regStr = self.BERT[i].regs['prbsLock']
      #self.BERT[i].widgets['cdrLock'].regStr = self.BERT[i].regs['cdrLock']
    self.OnStop(True)
    
  def OnTimer(self,event):
      self.ReadTheWidgetRegisters()      
      self.UpdateWidgets()
      self.Refresh()
 
  def qphase2pcode(self,quad,phase,device="None"):
    """converts a quad/phase value into a 0-512 wheel value"""
    #if device.find("A0")>-1:
    if(False):
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
    #if device.find("A0")>-1: 
    if(False):
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
    
  def ReadTheWidgetRegisters(self,zeroThem=False,debug=False):
    #self.device = self.m_deviceBox.GetStringSelection()
    for i in range (0,10):
      if(self.BERT[i].checkbox.GetValue()):
      
        #Read CTLE
        #Read RxEq Override bit
        ctleOVR = ipallreg.regRead(self.device + '::30.' + str(self.base_offset  + (i*0x100)+36) +'.0')
        if(ctleOVR):
          ctleState  = ipallreg.regRead(self.device + '::30.' + str(self.base_offset  + (i*0x100)+38) +'.3:0')
        else:
          ctleState  = ipallreg.regRead(self.device + '::30.' + str(self.base_offset  + (i*0x100)+37) +'.3:0')
        self.BERT[i].ctleState  = (ctleOVR<<16)+ctleState

        #Read EqAdapt
        self.BERT[i].ctleLock = ipallreg.regRead(self.device + '::30.' + str(self.base_offset  + (i*0x100)+3) +'.0')
        
        #Get PI codes to calculate ppm offset

        piCodeStr  = self.device + '::30.' + str(self.base_offset+(i*0x100)+40) + '.9:0'
        cdr2CodeStr= self.device + '::30.' + str(self.base_offset+(i*0x100)+42) + '.4:0'
        trip       = self.device + '::30.' + str(self.base_offset+(i*0x100)+39) + '.13:0'
      
        #Read PI Lock
        self.BERT[i].piLock = ipallreg.regRead(self.device + '::30.' + str(self.base_offset+(i*0x100)+32) + '.1')
        #if(i==1): print 'i: piLock = %d'%self.BERT[i].piLock
      
      
      
        for eachReg in self.BERT[i].regs:
          #print "eachReg: ",eachReg (cdrLock,liveErrorCount,prbsLock)
          #print "read",eachReg,self.regs[eachReg]
          #if(debug): 
          #  if(i==0): print '%s'%(self.BERT[i].regs)
          (device, reg, hiLsb, loLsb) = re.search('(.*)::(\d+\.\d+)\.*(\d*):*(\d*)',self.BERT[i].regs[eachReg]).groups()
          #if(debug): print '%d::%d %d %d' %(device,reg,hiLsb,loLsb)
          address = device+'::'+reg
          val = ipallreg.regRead(address)

          if (eachReg == 'liveErrorCount'): 
            print self.BERT[i].regs[eachReg],"came back with",val,device,reg
          #print 'Reg %s[%s:%s] = 0x%x' % (reg,hiLsb,loLsb,val)
          if(debug): print '%s %s: 0x%x'%(device,reg,val)
          if zeroThem:
            ipvar.System[device].regDefTab[reg].lastRead = 0
          else:
            ipvar.System[device].regDefTab[reg].lastRead = val
          if(debug): print 'LastRead: 0x%x'%ipvar.System[device].regDefTab[reg].lastRead
          
          
        #----
        # Get the 1st order Hist data
        self.BERT[i].cdrMin = 512
        self.BERT[i].cdrMax = 0

        codeBin = ipallreg.regRead(piCodeStr)
        #Support B0 device only
        #if ipvar.System[self.device].regDefTab['30.3'].defVec.value & 0xf == 0:   # equals A0 device
        if(False):
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
        

    
  def UpdateWidgets(self,debug=False): 
    now = time.time()
    timeSpan = now - self.time
    secs  = int(timeSpan % 60)
    mins  = (int((timeSpan - secs)/60) % 60)
    hours = (int(timeSpan/(60*60))) % 24
    days  = (int(timeSpan/(60*60*24)))    
    self.timeText.SetLabel('%dd %02d:%02d:%02d' % (days,hours,mins,secs))
         
    for rxn in range(0,10):
      if(self.BERT[rxn].checkbox.GetValue()):
        val = self.getLastRead(self.BERT[rxn].widgets['prbsLock'].regStr)
        if(debug): print '%s: 0x%x'%(self.BERT[rxn].widgets['prbsLock'].regStr,val)
        ver_enable = (val & 0x1000) >> 12
        
        for eachWidget in self.BERT[rxn].widgets:
          #val = self.getLastRead(self.BERT[rxn].widgets[eachWidget].regStr)
          #if(debug): print '%s %s 0x%x'%(eachWidget,self.BERT[rxn].widgets[eachWidget].regStr,val)
  
          if self.BERT[rxn].widgets[eachWidget].name.find('liveErrCount') == 0: 
            val = self.getLastRead(self.BERT[rxn].widgets[eachWidget].regStr)
            self.BERT[rxn].widgets[self.BERT[rxn].widgets[eachWidget].name].SetLabel(str(val))
            if int(val) == 65535:
              self.BERT[rxn].errorSaturation = 1
              self.BERT[rxn].saturationWarning.Show(True)
              #self.BERT[rxn].ber.SetBackgroundColour(wx.Colour(200,50,0))
              #print "counter saturated"
              #self.BERT[rxn].errorSaturation = 1         
            if(ver_enable):   
              livecount = int(self.BERT[rxn].widgets['liveErrCount'].GetLabel())
            
              val = int(self.BERT[rxn].widgetAccumulateError.GetLabel()) + int(self.BERT[rxn].widgets['liveErrCount'].GetLabel())
            
              self.BERT[rxn].widgetAccumulateError.SetLabel(str(val))
              bits = (time.time() - self.time) * float(self.BERT[rxn].rate)*1e9
              self.bitsWidget.SetLabel("%05.03g" % bits)
              ber = int(self.BERT[rxn].widgetAccumulateError.GetLabel())/bits
              if ber == 0:
                ber = 1.0/bits
              self.BERT[rxn].ber.SetLabel("%05.03g" % ber)
              #print 'SyncLoss: %d, Saturation: %d' % (self.BERT[rxn].syncLoss,self.BERT[rxn].errorSaturation == 0)
              if (val == 0) and (self.BERT[rxn].syncLoss == 0) and (self.BERT[rxn].errorSaturation == 0):
                self.BERT[rxn].ber.SetBackgroundColour('PALE GREEN')
              elif (self.BERT[rxn].syncLoss == 1) or (self.BERT[rxn].errorSaturation == 1):
                self.BERT[rxn].ber.SetBackgroundColour(wx.Colour(200,50,0)) 
              else:
                self.BERT[rxn].ber.SetBackgroundColour(wx.Colour(250,250,100)) 
             
            else :
              self.BERT[rxn].ber.SetBackgroundColour(wx.NullColour)
              #self.BERT[rxn].ber.SetBackgroundColour(wx.Colour(250,250,100))          
              
          elif self.BERT[rxn].widgets[eachWidget].name.find('prbsLock') == 0: 
            val = self.getLastRead(self.BERT[rxn].widgets[eachWidget].regStr)
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
            
          elif self.BERT[rxn].widgets[eachWidget].name.find('ctleState') == 0: 
            if(self.BERT[rxn].ctleState>15):
              color = 'Red'
              value = self.BERT[rxn].ctleState & 0xf
            else:
              color = 'Black'
              value = self.BERT[rxn].ctleState
              
            self.BERT[rxn].widgets[self.BERT[rxn].widgets[eachWidget].name].SetForegroundColour(color)            
            self.BERT[rxn].widgets[self.BERT[rxn].widgets[eachWidget].name].SetLabel(str(value))
            
          elif self.BERT[rxn].widgets[eachWidget].name.find('ctleLock') == 0:  
            #ctleLock - eqAdapt 1=lock, 0=not lock
            if(self.BERT[rxn].ctleLock):
              self.BERT[rxn].widgets['ctleLock'].SetValue(False)
            else:
              self.BERT[rxn].widgets['ctleLock'].SetValue(True)
              
            
          elif self.BERT[rxn].widgets[eachWidget].name.find('ppm') == 0: 
            cdr2 = self.BERT[rxn].cdr2Hist[0]
            trip = self.BERT[rxn].trip
            real_ppm = cdr2*20+trip/5.5
            self.BERT[rxn].widgets[self.BERT[rxn].widgets[eachWidget].name].SetLabel(str('%3.0f ppm'%real_ppm))     

          elif self.BERT[rxn].widgets[eachWidget].name.find('piLock') == 0: 
            if(self.BERT[rxn].piLock):
              self.BERT[rxn].widgets[self.BERT[rxn].widgets[eachWidget].name].SetForegroundColour('Red')  
            else:              
              self.BERT[rxn].widgets[self.BERT[rxn].widgets[eachWidget].name].SetForegroundColour('Black')
            
                  
 
          '''         
          elif self.BERT[rxn].widgets[eachWidget].name.find('prbsLock') == 0: 
            #[12] - Verification Enable
            #[11] - Invert
            #[10:8] - Patttern 
            #Extract bits [10:8] to determine pattern
            
            #Check Pattern Verification Enable [12]
            if (ver_enable):
              #Verification Pattern 30.20-29[10:8]
              self.BERT[rxn].widgets[self.BERT[rxn].widgets[eachWidget].name].SetLabel(self.pattern_labels[((val&(7<<8))/(1<<8))]) 
              #self.BERT[rxn].widgets[self.BERT[rxn].widgets[eachWidget].name].SetLabel('PRBS31')
              
              #PRBS Lock 30.20-29[15]
              if val&(1<<15) == 1<<15:
                self.BERT[rxn].widgets[self.BERT[rxn].widgets[eachWidget].name].SetBackgroundColour('PALE GREEN')
              else:
                self.BERT[rxn].widgets[self.BERT[rxn].widgets[eachWidget].name].SetBackgroundColour(wx.Colour(200,50,0))
                #self.BERT[rxn].widgets[self.BERT[rxn].widgets[eachWidget].name].SetLabel('SYNC Loss')
                self.BERT[rxn].syncLoss = 1  
            else :
              self.BERT[rxn].widgets[self.BERT[rxn].widgets[eachWidget].name].SetBackgroundColour(wx.NullColour)
              self.BERT[rxn].widgets[self.BERT[rxn].widgets[eachWidget].name].SetLabel('Off')
          '''

 

          #elif self.BERT[rxn].widgets[eachWidget].name.find('cdrLock') == 0: 
          #  if val == 1:
          #    self.BERT[rxn].widgets[self.BERT[rxn].widgets[eachWidget].name].SetBackgroundColour('PALE GREEN')
          #  else:
          #    self.BERT[rxn].widgets[self.BERT[rxn].widgets[eachWidget].name].SetBackgroundColour(wx.Colour(200,50,0))
            

      else :
        pass

    #End For rx in range (0,10) 
    self.Refresh()    
    self.parent.Refresh()  

  def OnGo(self,event):
    self.ReadTheWidgetRegisters()
    for rxn in range(0,10):
      self.BERT[rxn].widgetAccumulateError.SetLabel(str(0))
      self.BERT[rxn].ber.SetBackgroundColour('WHITE')
      self.BERT[rxn].rate = self.rateBox.GetValue()
      self.BERT[rxn].saturationWarning.Show(False)
      self.BERT[rxn].syncLoss = 0
      self.BERT[rxn].errorSaturation = 0


    self.timer.Start(500)
    self.time = time.time()
    self.goButton.SetLabel('Restart Monitor')      

    
    #self.UpdateWidgets()
    
    

    
  def OnEyeRefresh(self,event,debug=False):
    self.device = self.m_deviceBox.GetStringSelection() 
    print "Refresh the 4 point Eye Captures"
    self.haltRead = 1
    for i in range(3):
      print 'x_offset(%d): %d'%(i, 32*memRead('0x1b5[7:6]','common%d'%i,self.device)+memRead('0x1b6[4:0]','common%d'%i,self.device))
    
    for i in range(10):
      if(self.BERT[i].checkbox.GetValue()):
      #if(True):
        if(debug) : print 'Eye Instance: ',i
        #x_val  = get_caui_4pt(i,debug=0,X=1,Y=0) 
        #y_val = get_caui_4pt(i,debug=0,X=0,Y=1) 
          #xadjust offset, center of the eye =59; cal=67; (67+ 2**5+32) (bit0 to bit5)


        (self.BERT[i].eye_x_val,self.BERT[i].eye_y_val) = get_caui_eye_xy(i,self.device)
        (x_val,y_val) = get_caui_eye_xy(i,self.device)

        print 'Eye Instance: %d x:%d, y:%d'%(i, x_val, y_val)
        self.BERT[i].eye_x_val = x_val/32.0
        self.BERT[i].eye_y_val = y_val/62.0*500
        self.BERT[i].eye_x.SetLabel('%5.3f UI' % self.BERT[i].eye_x_val)
        self.BERT[i].eye_y.SetLabel('%d mV' % self.BERT[i].eye_y_val)
    self.haltRead = 0
 
 
  #----------------------------------------------------------------------------
  def OnCtleForce(self,event,debug=False):
    self.device = self.m_deviceBox.GetStringSelection()
    #print 'ctleForce: %d'%event.Id
    #print self.ctleState_id_map

 
    lane = self.ctleState_id_map[event.Id]
    value = event.GetSelection()
    #self.BERT[lane].widgets[self.BERT[lane].widgets['ctleState'].GetLabel
    if(debug): print "CTLE Force %d - Lane: %d Value:%s" % (event.Id,lane,value)
 
    #Freeze EqAdapt
    ipallreg.regWrite(self.device + '::30.' + str(self.base_offset+(lane*0x100)+3) + '.0',0)
    #Set Eq Value
    ipallreg.regWrite(self.device + '::30.' + str(self.base_offset+(lane*0x100)+38) + '.3:0',value)
    #Set Eq Override
    ipallreg.regWrite(self.device + '::30.' + str(self.base_offset+(lane*0x100)+36) + '.0',1)
    
    self.BERT[lane].widgets['ctleLock'].SetValue(True)
    
  #----------------------------------------------------------------------------

  def OnCtleLock(self,event,debug=False):
    self.device = self.m_deviceBox.GetStringSelection()
    #print 'ctleLock: %d'%event.Id
    #print self.ctleLock_id_map
    
    widget_id = event.GetId()
    widget = self.FindWindowById(widget_id)


 
    lane = self.ctleLock_id_map[event.Id]
    lock = widget.GetValue()
    
    if(debug): print "CTLE lock checkbox: %d - Lane: %d Lock:%s" % (event.Id,lane,lock)
    
    if(lock): 
      #Freeze EqAdapt
      ipallreg.regWrite(self.device + '::30.' + str(self.base_offset+(lane*0x100)+3) + '.0',0)
    else:
      #Enable EqAdapt
      ipallreg.regWrite(self.device + '::30.' + str(self.base_offset+(lane*0x100)+3) + '.0',1)    

      #Disable Eq Override
      ipallreg.regWrite(self.device + '::30.' + str(self.base_offset+(lane*0x100)+36) + '.0',0)



  #----------------------------------------------------------------------------
  def OnpiLock(self,event,debug=False):
    self.device = self.m_deviceBox.GetStringSelection()
    lane = self.piLock_id_map[event.Id]
    if(debug): print "PI button hit: %d - Lane: %d Lock:%s" % (event.Id,lane,self.BERT[lane].piLock)
    if(self.BERT[lane].piLock):
      ipallreg.regWrite(self.device + '::30.' + str(self.base_offset+(lane*0x100)+32) + '.1',0)
    else:
      ipallreg.regWrite(self.device + '::30.' + str(self.base_offset+(lane*0x100)+32) + '.1',1)
    #print "PI Lock: RX%d" % (lane)
  #----------------------------------------------------------------------------
  def OnPattern(self,event):
    self.device = self.m_deviceBox.GetStringSelection()    
    button_id = event.GetId()
    button = self.FindWindowById(button_id)
    lane = self.pattern_id_map[event.Id]
    label = button.GetLabel()
    
    print "Pattern button hit: %d Lane:%d" % (event.Id, lane)
    try: 
      index = self.pattern_labels.index(label)
    except:
      index = -1

    print "Button Id:", button_id
    print "Bert : RX%s Label: %s(%d)" % (self.pattern_id_map[button_id],label,index)
    
    if ((index == 1)) :
      index = 5
    elif (index == 7) :
      index = -1
    else:
      index = index + 1
    if (index == -1):
      button.SetLabel('')
      address = self.device+'::30.'+ str(20+lane)+'.12:12'
      ipallreg.regWrite(address,0)
    else:
      button.SetLabel(self.pattern_labels[index])
      address = self.device+'::30.'+ str(20+lane)+'.12:12'
      ipallreg.regWrite(address,1)
      address = self.device+'::30.'+ str(20+lane)+'.10:8'
      ipallreg.regWrite(address,index) 
    
    
  def select_all(self,event,debug=False):
    if(debug):
      print 'Button All'
    for i in range (10):
      self.BERT[i].checkbox.SetValue(1)

  def select_locked(self,event,debug=False):
    self.device = self.m_deviceBox.GetStringSelection()   
	# CAUI Input Ready[9:0]
    #address = self.device+'::30.1546'	
    #cdr_lock = ipallreg.regRead(address)
    if(debug):
      print 'Button Locked '
	
      
  def select_none(self,event,debug=False):
    if(debug):
      print 'Button None'
    for i in range (10):
      self.BERT[i].checkbox.SetValue(0)      
      
      
  def pattern_ver(self,event,debug=False):
    self.device = self.m_deviceBox.GetStringSelection()
    checkbox_id = event.GetId()
    checkbox = self.FindWindowById(checkbox_id)
    rx = self.checkbox_id_map[checkbox_id]
    address = self.device+'::30.'+ str(20+rx)+'.12:12'
    if(debug):
      print "Check Box: ", checkbox_id,rx, address, checkbox.GetValue(),checkbox.IsChecked(),

    #Set Clear Pattern Ver Enable [12]
    #if (checkbox.IsChecked()):
    #  ipallreg.regWrite(address,1)
    #else:
    #  ipallreg.regWrite(address,0)
      
    

      
  def OnStop(self,event):
    self.timer.Stop()
    self.goButton.SetLabel('Start Monitor')
    
  def OnPaint(self,event):
    dc = wx.PaintDC(self)
    font = wx.Font(8, wx.MODERN, wx.NORMAL, wx.NORMAL)
    dc.SetFont(font)
    #dc.SetPen(wx.Pen("GRAY",1))
    #for i in range(0,4):
    #  dc.DrawRectangle(self.rx+100,self.ry+self.i_y_offset*i+10,513,50)
    #for x in range(0,513,64):
    #  dc.DrawLine(self.rx+x,self.ry,self.rx+x,self.ry+256)
    #for y in range(0,100,10):
    #  dc.DrawLine(self.rx,self.ry+256-y,self.rx+512,self.ry+256-y)
    #dc.SetPen(wx.Pen("BLACK",1))            


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
  
    valPassing = ipvar.System[device].regDefTab[reg].lastRead
    myBin = bin(valPassing)[2:].zfill(16) + 'x'
    
    subregVal = int(myBin[16 - loLsb - nBits:16 - loLsb],2) 
    return(subregVal)

    