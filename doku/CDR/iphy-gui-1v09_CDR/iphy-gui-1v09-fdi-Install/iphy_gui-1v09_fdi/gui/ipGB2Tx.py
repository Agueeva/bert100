import wx
import re
import ipvar
import ipallreg
import time


from GEN import GEN

class GEN(GEN):
  def __init__(self,parent,label):
    super(GEN, self).__init__(parent)
    self.staticBox.SetLabel(label)
  
#class GEN():
#  def __init__(self, *args, **kwargs):
#    self.regs = {}
#    self.post  = 0
#    self.pre   = 0
#    self.swing = 0
    
class gb2TxPanel(wx.Panel):
  """  """
  def __init__(self, parent, interface):
    """   """
    wx.Panel.__init__(self, parent)
    self.parent = parent
    self.panel = self


    #Interface Mac, Line
    #
    #print 'Interface: %s' % interface
    if(interface == 'Host'):
      instance_id = 'GB2-H'
      choice_list=['10.3125','11.18']
      pat_ctrl_offset = 0x20
      errcnt_offset = 1600
      base_offset = 0x2080
      
      TIMER_ID = 1000
    elif(interface == 'Opt'):
      instance_id = 'GB2-O'
      choice_list=['25.78125','27.952']
      pat_ctrl_offset = 0x10    
      errcnt_offset = 1700
      base_offset = 0x1080
      TIMER_ID = 2000

          
    base_offset = 0x180  
      
    self.adapt_IDs = [-1]*10
    self.pattern_labels = ['PRBS31','PRBS9','res','res','PRBS31','PRBS23','PRBS15','PRBS7']
    
    #self.timer = wx.Timer(self, TIMER_ID)  # message will be sent to the panel
    #self.timer.Start(50)  # x100 milliseconds
    #wx.EVT_TIMER(self, TIMER_ID, self.OnTimer)  # call the on_timer function  
   
    rx=250
    ry=10
    # ***   BUTTONS  *************************************************

    self.panel = wx.Panel(self,-1)

    self.goButton   = wx.Button(self.panel,-1,label="Start M---",pos=(5,10),size=(100,24))
    self.stopButton = wx.Button(self.panel,-1,label="Stop M---",pos=(5,40),size=(100,24))
    self.forceButton = wx.Button(self.panel,-1,label="TBD0",pos=(340,10),size=(75,24))
    self.adaptButton = wx.Button(self.panel,-1,label="TBD1",pos=(405,10),size=(75,24))
    self.statusButton = wx.Button(self.panel,-1,label="TBD2",pos=(405+75,10),size=(75,24))
    self.eyeButton = wx.Button(self.panel,-1,label="TBD3",pos=(340,40),size=(120,24))


    
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
    #Devices = ipvar.System.keys()    
	  #GJl - 09/06/2012 
	  #Select GB Devices only

    #Select GB2 Devices only
    self.GB2_list = []
    for eachDevice in ipvar.System.keys():
      if "_" in eachDevice or (re.search(r'\w+-\w+',eachDevice)):
        pass
      else:
        gb2 = re.search(r'(GB2\w*)',eachDevice)
        if(gb2) :
          #print 'ipGB2Tx2: %s'%gb2.group(1)
          self.GB2_list.append(gb2.group(1))               
        

      
    print 'ipGB2Tx: %s'%self.GB2_list
         
    #Create Device for each GB2 - Host + Optical       
    self.Device_list = []
    for eachDevice in self.GB2_list:
      self.Device_list.append(eachDevice+'-Host')
      self.Device_list.append(eachDevice+'-Opt')      
    self.device = self.Device_list[0] 
    
	
    self.m_deviceBox = wx.RadioBox(self.panel, label="Device", pos=(rx+250, ry+20), choices=self.Device_list,  majorDimension=1)
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
    self.bitsWidget = wx.StaticText(self.panel,-1,'0',size=(75,22),pos=(rx+120,ry+80),style=wx.RAISED_BORDER|wx.ALIGN_RIGHT|wx.ST_NO_AUTORESIZE)

    
    #self.button_all  = wx.Button(self,-1,'All',pos=(rx+540,ry+82),size=(24,22))
    #rx=rx+ 26
    #self.button_none = wx.Button(self,-1,'None',pos=(rx+540,ry+82),size=(32,22)) 
    
    #self.Bind(wx.EVT_BUTTON,self.select_all,id=self.button_all.GetId())
    #self.Bind(wx.EVT_BUTTON,self.select_none,id=self.button_none.GetId())
    
    #self.button_all  = wx.Button(self,-1,'All',pos=(500,ry+82),size=(24,22))
    #self.button_locked  = wx.Button(self,-1,'Locked',pos=(526,ry+82),size=(42,22))    
    #self.button_none = wx.Button(self,-1,'None',pos=(570,ry+82),size=(33,22)) 
    
    #self.Bind(wx.EVT_BUTTON,self.select_all,id=self.button_all.GetId())
    #self.Bind(wx.EVT_BUTTON,self.select_locked,id=self.button_locked.GetId())
    #self.Bind(wx.EVT_BUTTON,self.select_none,id=self.button_none.GetId())
    
    
    #self.stopButton.Enable(False)
    self.stop = 0  
    #self.widgets={} # a list of widgets that get updated from reg values
    #self.regs={}    # a list of registers required to be read for update of this panel
    #self.GEN = []
           
    self.tx=5
    self.ty=90
    self.i_y_offset = 94
    self.pattern_id_map={} 
    self.checkbox_id_map={}
    self.device = self.m_deviceBox.GetStringSelection() 
    j = [0,2,4,6,8,1,3,5,7,9]    
    
    self.txCTRL = []
    for i in range (10):
      self.txCTRL.append(GEN(self,'Tx%d'%i))
    for i in range (10):
      #Shift to second column for GEN 5-9
      #if (i>4):
      if (i%2):
        self.tx=307
      else:
        self.tx=5
       
      #self.GEN.append(GEN())
      #self.GEN[i].pattern_id = wx.NewId()
      #self.pattern_id_map[self.GEN[i].pattern_id] = i
      #self.GEN[i].widgets = {}
      #self.GEN[i].regs['liveErrCount']  = self.device + '::8.' + str(errcnt_offset  + (i)) + '.15:0'
      #self.GEN[i].regs['prbsLock']      = self.device + '::30.' + str(pat_ctrl_offset  + (i))
      #self.GEN[i].regs['ctle']          = self.device + '::30.' + str(base_offset  + (i*100)+37) +'.3:0'
      #self.GEN[i].regs['cdrLock']       = self.device + '::30.1546.' + str(i)
      #self.GEN[i].regs['pattern']       = self.device + '::30.' + str(20 + i)
      #else:
      #  self.GEN[i].regs['liveErrCount']  = self.device + '::30.' + str(48  + (i)) + '.15:0'
      #  self.GEN[i].regs['prbsLock']        = self.device + '::30.' + str(16  + (i)) + '.15'
      #self.GEN[i].regs['cdrLock']         = self.device + '::30.1546.' + str(i)
      

      #self.staticBox3   = wx.StaticBox(self,-1,label='TX'+str(i),pos=(self.tx, self.ty+(i/2)*self.i_y_offset),style=wx.RAISED_BORDER|wx.BOLD,size=(297,self.i_y_offset-2))

      
      #wx.StaticText(self.panel,-1,"Pattern",pos=(self.tx+9, self.ty+20+(i/2)*self.i_y_offset))
      #self.GEN[i].swing=wx.SpinCtrl(self.panel,-1,"Swing", pos=(self.tx+19,self.ty+43+(i/2)*self.i_y_offset),size=(50,22))
      #self.GEN[i].swing.SetRange(0,3)
    
      #self.GEN[i].widgets['liveErrCount'] = wx.StaticText(self,-1,'0',size=(45,22),pos=(self.tx+58,self.ty+17+(i/2)*self.i_y_offset),style=wx.RAISED_BORDER|wx.ALIGN_RIGHT|wx.ST_NO_AUTORESIZE)   
      #self.GEN[i].widgets['liveErrCount'].regStr = self.GEN[i].regs['liveErrCount']
      #self.GEN[i].widgets['liveErrCount'].name = 'liveErrCount'
      
      #wx.StaticText(self.panel,-1,"CTLE",   pos=(self.tx+140,self.ty+43+(i/2)*self.i_y_offset)).SetFont(font)
      #wx.StaticText(self.panel,-1,"Eye4+",pos=(self.tx+19,self.ty+66+(i/2)*self.i_y_offset)).SetFont(font)
      

      
      #self.GEN[i].widgetAccumulateError = wx.StaticText(self,-1,'0',size=(60,22),pos=(self.tx+103,self.ty+17+(i/2)*self.i_y_offset),style=wx.RAISED_BORDER|wx.ALIGN_RIGHT|wx.ST_NO_AUTORESIZE)


      #gjl - pattern button
      #self.GEN[i].widgets['prbsLock'] = wx.Button(self,self.GEN[i].pattern_id,label=" ",size=(60,22),pos=(self.tx+58,self.ty+40+(i/2)*self.i_y_offset),style=wx.RAISED_BORDER|wx.ALIGN_RIGHT|wx.ST_NO_AUTORESIZE)
      #self.GEN[i].widgets['prbsLock'].regStr = self.GEN[i].regs['prbsLock']
      #self.GEN[i].widgets['prbsLock'].name = 'prbsLock'

      
      font=wx.Font(12, wx.FONTFAMILY_DEFAULT,wx.NORMAL,wx.BOLD)  
      #GENext = wx.StaticText(self.panel,-1,"BER",pos=(self.tx+170,self.ty+17+(i/2)*self.i_y_offset))
      #berText.SetFont(font)
      #self.GEN[i].ber        = wx.StaticText(self,-1,'',size=(85,26),pos=(self.tx+205,self.ty+16+(i/2)*self.i_y_offset),style=wx.RAISED_BORDER|wx.ALIGN_RIGHT|wx.ST_NO_AUTORESIZE)
      #self.GEN[i].ber.SetFont(font)
      #self.GEN[i].ber.SetLabel(str(0))
      #font=wx.Font(8, wx.FONTFAMILY_DEFAULT,wx.NORMAL,wx.BOLD)
      #self.GEN[i].saturationWarning = wx.StaticText(self,-1,'Saturated',size=(70,22),pos=(self.tx+149,self.ty+40+(i/2)*self.i_y_offset),style=wx.RAISED_BORDER|wx.ALIGN_CENTRE|wx.BOLD|wx.ST_NO_AUTORESIZE)


      #self.GEN[i].saturationWarning.SetBackgroundColour(wx.Colour(200,50,0))
      #self.GEN[i].saturationWarning.SetForegroundColour('WHITE')
      #self.GEN[i].saturationWarning.SetFont(font)
      #self.GEN[i].saturationWarning.Show(False)
      #self.prbsLock.SetBackgroundColour('WHITE')

      #self.GEN[i].eye_x = wx.StaticText(self,-1,str(self.GEN[i].eye_x_val)+'UI',size=(60,22),pos=(self.tx+58,self.ty+63+(i/2)*self.i_y_offset),style=wx.RAISED_BORDER|wx.ALIGN_RIGHT|wx.ST_NO_AUTORESIZE)
      #self.GEN[i].eye_y = wx.StaticText(self,-1,str(self.GEN[i].eye_y_val)+'mV',size=(60,22),pos=(self.tx+58+62,self.ty+63+(i/2)*self.i_y_offset),style=wx.RAISED_BORDER|wx.ALIGN_RIGHT|wx.ST_NO_AUTORESIZE)
      
      
      #self.GEN[i].checkbox_id = wx.NewId()
      #self.checkbox_id_map[self.GEN[i].checkbox_id] = i
      #self.GEN[i].checkbox = wx.CheckBox(self,self.GEN[i].checkbox_id,'',pos=(self.tx+275,self.ty+70+(i/2)*self.i_y_offset))
      #self.Bind(wx.EVT_CHECKBOX,self.pattern_ver)

      #Enable Pattern Verification for All
      #self.GEN[i].checkbox.SetValue(1)
      #address = self.device+'::30.'+ str(20+i)+'.12:12'
      #ipallreg.regWrite(address,1)
      
    #self.parent.cdrMonitorPanel = cdrMonitorPanel(self.notebook, -1)
    #
    #print "Updating BERT[i].widgets"
    #self.UpdateBERT[i].widgets()
    
    
    
    # Sizers
    box = wx.BoxSizer(wx.VERTICAL)
    box.Add(self.panel, 0, wx.EXPAND)  
    gSizer = wx.GridSizer(5,2,1,1)
    for i in range(10):
      gSizer.Add(self.txCTRL[i],0, wx.ALL,0)
    box.Add(gSizer)
    #hSizer = wx.BoxSizer(wx.HORIZONTAL)
    #Draw Window + Gauge    
    #vSizer0 = wx.BoxSizer(wx.VERTICAL)
    #vSizer0.Add(self.draw, 0, wx.EXPAND)    
    #vSizer0.Add(self.gauge, 0, wx.ALIGN_RIGHT)    
    #hSizer.Add(vSizer0,0,wx.EXPAND)  
    
    #Space hSizwer
    #hSizer.Add((5,5),0,wx.EXPAND)
    
    #Text Panel
    #vSizer1 = wx.BoxSizer(wx.VERTICAL)
    #vSizer1.Add((10,20),0,wx.EXPAND)
    #vSizer1.Add(self.pps1,0, wx.EXPAND)
    #vSizer1.Add(self.pps2,0, wx.EXPAND)
    #vSizer1.Add(self.pps3,0, wx.EXPAND)
    #vSizer1.Add(self.pps4,0, wx.EXPAND)
    #hSizer.Add(vSizer1,0,wx.EXPAND)    
    
    
    #box.Add(hSizer, 0, wx.EXPAND)


    self.SetAutoLayout(True)
    self.SetSizer(box)
    self.Layout()   
        
    self.parent.Refresh()
 
  def ChangeDevice(self,event):
    global regs
    #print "Device Changed",event.String 
    #self.device = event.String
    self.device = self.m_deviceBox.GetStringSelection()   
    print "Device Changed",self.device 
    for i in range(0,10):
      self.GEN[i].regs = {}    
      if self.device.find("GB") == 0:
        self.GEN[i].regs['liveErrCount']  = self.device + '::8.' + str(1600  + (i)) + '.15:0'   # Host (Tx lane)
        self.GEN[i].regs['prbsLock']      = self.device + '::30.' + str(32  + (i))        # Host (Tx lane)
        #self.GEN[i].regs['cdrLock']       = self.device + '::30.1546.' + str(i)
      else:
        self.GEN[i].regs['liveErrCount']  = self.device + '::30.' + str(48  + (i)) + '.15:0'
        self.GEN[i].regs['prbsLock']        = self.device + '::30.' + str(16  + (i)) + '.15'


      self.GEN[i].widgets['liveErrCount'].regStr = self.GEN[i].regs['liveErrCount']
      self.GEN[i].widgets['prbsLock'].regStr = self.GEN[i].regs['prbsLock']
      #self.GEN[i].widgets['cdrLock'].regStr = self.GEN[i].regs['cdrLock']
    self.OnStop(True)
    


          
    
  def UpdateWidgets(self,debug=False): 
    pass

  def OnGo(self,event):
    pass

    
    #self.UpdateWidgets()  
  

 
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
      self.GEN[i].checkbox.SetValue(1)

  def select_locked(self,event,debug=True):
    self.device = self.m_deviceBox.GetStringSelection()   
	# CAUI Input Ready[9:0]
    #address = self.device+'::30.1546'	
    #cdr_lock = ipallreg.regRead(address)
    if(debug):
      print 'Button Locked '
	
      
  def select_none(self,event,debug=True):
    if(debug):
      print 'Button None'
    for i in range (10):
      self.GEN[i].checkbox.SetValue(0)      
      
      

      
  def OnStop(self,event):
    self.timer.Stop()
    self.goButton.SetLabel('Start Monitor')
    


    