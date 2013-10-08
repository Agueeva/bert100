import wx
import re
import ipvar
import ipallreg
import time
from membus import memWrite, memRead

def get_caui_4pt(cLane,X=1,Y=1,device='GB0',debug=False):
  #global memWrite, memRead

  if cLane < 3:
    common = 'common0'
  elif cLane < 7:
    common = 'common1'
  else:
    common = 'common2'
  #4 pt eye setup
  ################increase RXCALROAMEYEMEAS_COUNT (RXCALROAMEYEMEAS_COUNT * bitwidth < 2**16-1)####################
  #Disable Roam Adjust for 4 point Eye
  memWrite('0xd',cLane,0x3,device)
  memWrite('0x25',cLane,0x40,device)
    
  memWrite('0x1bf[2:0]',common,0,device)
  memWrite('0x1be[7:0]',common,0xf4,device)
  memWrite('0x1bd[7:3]',common,0x1,device)
  #print "RXCALROAMEYEMEAS_COUNT:",memRead('0x1bd[7:3]',common)*2**11+memRead('0x1be[7:0]',common)*2**3+memRead('0x1bf[2:0]',common)

  #422[7:6],423[7:0],424[5:0]	RXCALEYEDIAGFSM_BERTHRESHOLD	4  0x400
  memWrite('0x1a6[7:6]',common,0x0,device)
  memWrite('0x1a7[7:0]',common,0x10,device)
  memWrite('0x1a8[5:0]',common,0x0,device)
  

  #print  "RXCALEYEDIAGFSMIN_LOCWREN:",memRead('0x25[3]',cLane,device)

  #set X and Y weight, by default, they are both 0x1. 
  #when they are both 1, the max 4-point eye number is 64(eye height) + 32(eye width) = 96 (4-point eye)
  #If you just want to capture eye height alone, set XVALWEIGHT = 0
  #if you want to capture eye width alone, set YVALWEIGHT = 0
  if(debug) :
     print  "RXCALROAM (0xd): 0x%x" % memRead('0xd',cLane,device)
     print  "RXCALEYE (0x25): 0x%x" % memRead('0x25',cLane,device)
     print  "RXCALROMXYADJUST(0xd[1:0])",memRead('0xd[1:0]',cLane,device)
     print  "RXROAM_XORBITSEL(0xd[3])",memRead('0xd[3]',cLane,device)
     print  "RXRXSCOPE_EN(0xd[4])",memRead('0xd[4]',cLane,device)

     print  "RXCALROAMEYEMEASIN(0x25[6])",memRead('0x25[6]',cLane,device)
     print  "RXCALROAMEYEMEASIN(0x25[6])",memRead('0x25[6]',cLane,device) 
     
     print "RXCALROAMEYEMEAS_COUNT_0x1bf: 0x%x" % memRead('0x1bf',common,device)
     print "RXCALROAMEYEMEAS_COUNT_0x1be: 0x%x" % memRead('0x1be',common,device)
     print "RXCALROAMEYEMEAS_COUNT_0x1bf[2:0]: ",memRead('0x1bf[2:0]',common,device)
     print "RXCALROAMEYEMEAS_COUNT_0x1be[7:0]: ",memRead('0x1be[7:0]',common,device)
     print "RXCALROAMEYEMEAS_COUNT_0x1bd[7:3]: ",memRead('0x1bd[7:3]',common,device)
     print "RXCALROAMEYEMEAS_COUNT:",memRead('0x1bd[7:3]',common,device)*2**11+memRead('0x1be[7:0]',common,device)*2**3+memRead('0x1bf[2:0]',common,device)

     
     #422[7:6],423[7:0],424[5:0]	RXCALEYEDIAGFSM_BERTHRESHOLD	4  0x400
     print "RXCALEYEDIAG_BERTHRESHOLD(0x1a6): 0x%x" % memRead('0x1a6',common,device)
     print "RXCALEYEDIAG_BERTHRESHOLD(0x1a8): 0x%x" % memRead('0x1a8',common,device)
     
     print "RXCALEYEDIAG_BERTHRESHOLD(0x1a6[7:6]):",memRead('0x1a6[7:6]',common,device)
     print "RXCALEYEDIAG_BERTHRESHOLD(0x1a7[7:0]): 0x%x" % memRead('0x1a7[7:0]',common,device)
     print "RXCALEYEDIAG_BERTHRESHOLD(0x1a8[5:0]): 0x%x"%memRead('0x1a8[5:0]',common,device)
     print "RXCALEYEDIAG_BERTHRESHOLD: 0x%x"%((memRead('0x1a6[7:6]',common,device)<<14) + (memRead('0x1a7[7:0]',common,device)<<6) + (memRead('0x1a8[5:0]',common,device)))
  
  memWrite('0x1ac[1:0]',common,X,device) # RXCALEYEDIAGFSM_XVALWEIGHT
  if debug:
    print  "RXCALEYEDIAGFSM_XVALWEIGHT:",memRead('0x1ac[1:0]',common,device)

  memWrite('0x1ad[7:4]',common,Y,device) # RXCALEYEDIAGFSM_YVALWEIGHT
  if debug:
    print  "RXCALEYEDIAGFSM_YVALWEIGHT:",memRead('0x1ad[7:4]',common,device)


  #enable 4 point eye measurement
  memWrite('0x1a8[7]',common,1,device) # RXCALEYEDIAGFSM_EN
  if debug:
    print  "RXCALEYEDIAGFSM_EN:",memRead('0x1a8[7]',common,device)

  #start 4-point eye measurement
  #memWrite('0x25[2]',cLane,1,device) # RXCALEYEDIAGFSMIN_START
  memWrite('0x25',cLane,0x44,device) # RXCALEYEDIAGFSMIN_START
  if debug:
    print  "RXCALEYEDIAGFSMIN_START:",memRead('0x25[2]',cLane,device)

  #check done signal
  #memWrite('0x1a8[7]',common,1,device) # RXCALEYEDIAGFSM_EN
  if debug:
    print  "RXCALEYEDIAGFSM_EN:",memRead('0x1a8[7]',common,device)
    print  "RXCALEYEDIAGFSM_DONE:",memRead('0x25[0]',cLane,device)

  #stop 4-point eye measurement
  #memWrite('0x25[2]',cLane,0,device) # RXCALEYEDIAGFSMIN_START  (0=stop)
  memWrite('0x25',cLane,0x40,device) # RXCALEYEDIAGFSMIN_START  (0=stop)
  if debug:
    print  "RXCALEYEDIAGFSMIN_START:",memRead('0x25[2]',cLane,device)

  #check done signal
  #print  "RXCALEYEDIAGFSM_DONE:",memRead('0x25[0]',cLane,device)


  #read back 4-point eye value
  if debug:
    print  "RXCALEYEDIAGFSM_EYESUM_0x26[7:0]:",memRead('0x26[7:0]',cLane,device)
    print  "RXCALEYEDIAGFSM_EYESUM_0x27[5:0]:",memRead('0x27[5:0]',cLane,device)
    print  "4-point eye value: ", memRead('0x26[7:0]',cLane,device)*2**6 + memRead('0x27[5:0]',cLane,device)

  return (memRead('0x26[7:0]',cLane,device)*2**6 + memRead('0x27[5:0]',cLane,device))


def get_caui_eye_xy(cLane,device='GB0',debug=False):
  global get_caui_4pt
  eye_4pt_x = get_caui_4pt(cLane,X=1,Y=0,device=device) 
  eye_4pt_y = get_caui_4pt(cLane,X=0,Y=1,device=device) 
  #return (random.randint(0,32)/32.0,random.randint(0,64)/64.0*500) # dumby code for offline testing
  #return (eye_4pt_x/32.0,eye_4pt_y/62.0*500)
  return (eye_4pt_x,eye_4pt_y)

class BERT():
  def __init__(self, *args, **kwargs):
    self.accumulateError = 0
    self.syncLoss = 0
    self.errorSaturation = 0
    self.regs = {}
    self.eye_x_val = 0
    self.eye_y_val = 0
    
    
  def setRate(self,String):
    print "Rate set to",String    
  
class BERPanel(wx.Panel):
  """  """
  def __init__(self, parent, *args, **kwargs):
    """   """
    wx.Panel.__init__(self, parent, *args, **kwargs)
    self.parent = parent
    self.panel = self

    self.Bind(wx.EVT_PAINT,self.OnPaint)
    self.time = 0
    self.numberOfBits = 0
    self.haltRead = 0


    self.adapt_IDs = [-1]*10
    self.pattern_labels = ['PRBS31','PRBS9','res','res','PRBS31','PRBS23','PRBS15','PRBS7']
    
    TIMER_ID = 2000  # pick a number
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
    
    self.forceButton = wx.Button(self,-1,label="TBD",pos=(310,10),size=(75,24))
    self.forceButton.controls = self

    self.adaptButton = wx.Button(self,-1,label="TBD",pos=(385,10),size=(75,24))
    self.adaptButton.controls = self

    self.statusButton = wx.Button(self,-1,label="TBD",pos=(385+75,10),size=(75,24))
    self.statusButton.controls = self

    self.eyeButton = wx.Button(self,-1,label="TBD",pos=(310,40),size=(120,24))
    self.eyeButton.controls = self

    
    self.goButton.Bind(wx.EVT_BUTTON,self.OnGo)
    self.stopButton.Bind(wx.EVT_BUTTON,self.OnStop)
    self.adaptButton.Bind(wx.EVT_BUTTON,self.OnAdaptAll)
    self.statusButton.Bind(wx.EVT_BUTTON,self.OnEqStatus)    
    self.forceButton.Bind(wx.EVT_BUTTON,self.OnEqForce)    
    self.eyeButton.Bind(wx.EVT_BUTTON,self.OnEyeRefresh)    
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
    #Devices = ipvar.System.keys()    
	#GJl - 09/06/2012 
	#Select GB Devices only
    self.GB_list = []
    for eachDevice in ipvar.System.keys():
      if (eachDevice.find("GB")==0):
        self.GB_list.append(eachDevice)
                
    self.device = self.GB_list[0]    	
	
    self.m_deviceBox = wx.RadioBox(self, label="Device", pos=(rx+250, ry+20), choices=self.GB_list,  majorDimension=1)
    self.Bind(wx.EVT_RADIOBOX, self.ChangeDevice,id=self.m_deviceBox.GetId())
    #--------------------------------------------------------------------------
    #Rate Choices
    choices=['10.3125','11.18']
    self.rateBox  = wx.ComboBox(self.panel,-1,value=choices[0],choices=choices,pos=(rx+120,ry+55),style=wx.ALIGN_RIGHT|wx.ST_NO_AUTORESIZE,name='rateCombo',size=(75,27))
    rateText = wx.StaticText(self.panel,-1,"(Gbps)",pos=(rx+200,ry+57),size=(-1,24))
    font=wx.Font(9, wx.FONTFAMILY_DEFAULT,wx.NORMAL,wx.BOLD)
    rateText.SetFont(font)
    bitsText = wx.StaticText(self.panel,-1,"Bits",pos=(rx+200,ry+82))
    bitsText.SetFont(font)
    self.bitsWidget = wx.StaticText(self,-1,'0',size=(75,22),pos=(rx+120,ry+80),style=wx.RAISED_BORDER|wx.ALIGN_RIGHT|wx.ST_NO_AUTORESIZE)

    
    self.button_all  = wx.Button(self,-1,'All',pos=(rx+540,ry+82),size=(24,22))
    rx=rx+ 26
    self.button_none = wx.Button(self,-1,'None',pos=(rx+540,ry+82),size=(32,22)) 
    
    self.Bind(wx.EVT_BUTTON,self.select_all,id=self.button_all.GetId())
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
      self.pattern_id_map[self.BERT[i].pattern_id] = i
      self.BERT[i].widgets = {}
      #if self.device.find("GB") == 0:
      self.BERT[i].regs['liveErrCount']  = self.device + '::8.' + str(1700  + (i)) + '.15:0'
      self.BERT[i].regs['prbsLock']      = self.device + '::30.' + str(16  + (i))
      #self.BERT[i].regs['cdrLock']       = self.device + '::30.1546.' + str(i)
      #self.BERT[i].regs['pattern']       = self.device + '::30.' + str(20 + i)
      #else:
      #  self.BERT[i].regs['liveErrCount']  = self.device + '::30.' + str(48  + (i)) + '.15:0'
      #  self.BERT[i].regs['prbsLock']        = self.device + '::30.' + str(16  + (i)) + '.15'
      #self.BERT[i].regs['cdrLock']         = self.device + '::30.1546.' + str(i)
      

      self.staticBox3   = wx.StaticBox(self,-1,label='RX'+str(i),pos=(self.rx, self.ry+(i/2)*self.i_y_offset),style=wx.RAISED_BORDER|wx.BOLD,size=(297,self.i_y_offset-2))

      wx.StaticText(self.panel,-1,"Errors", pos=(self.rx+19,self.ry+20+(i/2)*self.i_y_offset)).SetFont(font)
      wx.StaticText(self.panel,-1,"Pattern",pos=(self.rx+9,self.ry+43+(i/2)*self.i_y_offset)).SetFont(font)
      #wx.StaticText(self.panel,-1,"Eye4+",pos=(self.rx+19,self.ry+66+(i/2)*self.i_y_offset)).SetFont(font)
      
      self.BERT[i].widgets['liveErrCount'] = wx.StaticText(self,-1,'0',size=(45,22),pos=(self.rx+58,self.ry+17+(i/2)*self.i_y_offset),style=wx.RAISED_BORDER|wx.ALIGN_RIGHT|wx.ST_NO_AUTORESIZE)
      
      self.BERT[i].widgets['liveErrCount'].regStr = self.BERT[i].regs['liveErrCount']
      self.BERT[i].widgets['liveErrCount'].name = 'liveErrCount'
      
      self.BERT[i].widgetAccumulateError = wx.StaticText(self,-1,'0',size=(60,22),pos=(self.rx+103,self.ry+17+(i/2)*self.i_y_offset),style=wx.RAISED_BORDER|wx.ALIGN_RIGHT|wx.ST_NO_AUTORESIZE)


      #gjl - pattern button
      self.BERT[i].widgets['prbsLock'] = wx.Button(self,self.BERT[i].pattern_id,label=" ",size=(60,22),pos=(self.rx+58,self.ry+40+(i/2)*self.i_y_offset),style=wx.RAISED_BORDER|wx.ALIGN_RIGHT|wx.ST_NO_AUTORESIZE)
      self.BERT[i].widgets['prbsLock'].regStr = self.BERT[i].regs['prbsLock']
      self.BERT[i].widgets['prbsLock'].name = 'prbsLock'
      self.BERT[i].widgets['prbsLock'].Bind(wx.EVT_BUTTON,self.OnPattern)


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
      self.BERT[i].checkbox = wx.CheckBox(self,self.BERT[i].checkbox_id,'',pos=(self.rx+275,self.ry+70+(i/2)*self.i_y_offset))
      self.Bind(wx.EVT_CHECKBOX,self.pattern_ver)

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
        self.BERT[i].regs['liveErrCount']  = self.device + '::8.' + str(1700  + (i)) + '.15:0'   # Host (Tx lane)
        self.BERT[i].regs['prbsLock']      = self.device + '::30.' + str(16  + (i))        # Host (Tx lane)
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
 
  def ReadTheWidgetRegisters(self,zeroThem=False,debug=False):
    #self.device = self.m_deviceBox.GetStringSelection()
    for i in range (0,10):
      if(self.BERT[i].checkbox.GetValue()):
      #if(True):
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
      #if (True):
        val = self.getLastRead(self.BERT[rxn].widgets['prbsLock'].regStr)
        if(debug): print '%s: 0x%x'%(self.BERT[rxn].widgets['prbsLock'].regStr,val)
        ver_enable = (val & 0x1000) >> 12
        for eachWidget in self.BERT[rxn].widgets:
          val = self.getLastRead(self.BERT[rxn].widgets[eachWidget].regStr)
          #if(debug): print '%s %s 0x%x'%(eachWidget,self.BERT[rxn].widgets[eachWidget].regStr,val)
  
          if self.BERT[rxn].widgets[eachWidget].name.find('liveErrCount') == 0: 
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
  def OnAdaptAll(self,event):
    print "Setting RxEq - Backplane Mode"
    #locals = {"device_name":"GB0"}
    #execfile("../scripts/CAUI_RxEq-Backplane.py",locals)  
    #execfile('../scripts/rx_caui_adapt_all.py')
    # need to check is the BER tool is running and if it is - restart it
    self.adaptButton.Enable(False)
    self.device_name = self.m_deviceBox.GetStringSelection()
    locals = {"device_name":self.device_name}
    try:
      file="../scripts/CAUI_RxEq-Backplane.py"  
      execfile(file, locals)         
    except:
      print 'Error executing file %s' % file              
    self.adaptButton.Enable(True)    
 
  def OnEqForce(self,event):
    print "Setting RxEq - CFP Mode"
    #execfile('../scripts/rx_caui_set_cfp.py')
    #execfile("../scripts/CAUI_RxEq-8888.py", locals)
    # need to check is the BER tool is running and if it is - restart it
    self.forceButton.Enable(False)
    self.device_name = self.m_deviceBox.GetStringSelection()
    locals = {"device_name":self.device_name}
    try:
      file="../scripts/CAUI_RxEq-8888.py"  
      execfile(file, locals)         
    except:
      print 'Error executing file %s' % file              
    self.forceButton.Enable(True)   
    
    
    
  def OnEqStatus(self,event):
    #execfile('../scripts/rx_caui_status.py')
    self.statusButton.Enable(False)
    self.device_name = self.m_deviceBox.GetStringSelection()
    locals = {"device_name":self.device_name}
    try:
      file="../scripts/CAUI_RxEq-Status.py"  
      execfile(file, locals)         
    except:
      print 'Error executing file %s' % file              
    self.statusButton.Enable(True)        
     
    
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
    
    
  def select_all(self,event,debug=True):
    if(debug):
      print 'Button All'
    for i in range (10):
      self.BERT[i].checkbox.SetValue(1)
      
  def select_none(self,event,debug=True):
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

    