import wx
import re
import ipvar
import ipallreg
import time

from membus import memWrite, memRead

class BERT():
  def __init__(self, *args, **kwargs):
    self.accumulateError = 0
    self.syncLoss = 0
    self.errorSaturation = 0
    self.regs = {}
    
  def setRate(self,String):
    print "Rate set to",String  

    
class cauiRxPanel(wx.Panel):

  """  """
  def __init__(self, parent, *args, **kwargs):
    """   """
    wx.Panel.__init__(self, parent, *args, **kwargs)
    self.parent = parent
    self.panel = self
    self.time = 0
    self.adapt_IDs = [-1]*10
    Devices = ['GB0']
    self.device = Devices[0]
    self.stop = 0  

    self.parent.Refresh()
    self.rxcb = []
    rx = 0
    ry = 0
    wx.StaticText(self,-1,'CAUI Lane',pos=(rx+230,ry+5))
    self.button_all_id = wx.NewId()
    wx.Button(self,self.button_all_id,'All',pos=(rx+20,ry+23),size=(35,24))
    self.Bind(wx.EVT_BUTTON,self.set_all,id=self.button_all_id)
    
    self.button_none_id = wx.NewId()
    wx.Button(self,self.button_none_id,'None',pos=(rx+57,ry+23),size=(35,24))
    self.Bind(wx.EVT_BUTTON,self.set_none,id=self.button_none_id)
    
    for i in range(0,10):
      self.rxcb.append(wx.CheckBox(self, -1,label=str(i),pos=(rx+100+(i*30),ry+13+15)))
      self.rxcb[-1].rx = i
      
    self.memwrite_id = wx.NewId()
    self.memwrite = wx.TextCtrl(self,self.memwrite_id,'xx',pos=(410,ry+13+15),size=(80,24))
    #self.Bind(wx.EVT_TEXT_ENTER,self.memwrite_enter)
    
    self.buton_memaccess_id = wx.NewId()
    wx.Button(self,self.buton_memaccess_id,"Write",pos=(490,ry+13+15),size=(40,24))
    self.Bind(wx.EVT_BUTTON,self.doMem,id=self.buton_memaccess_id)

    wx.StaticText(self, -1, "CALEQ_LOCWREN", pos=(410, ry+13+15+35))
    self.button_locwren_id = wx.NewId()  
    self.button_locwren = wx.ToggleButton(self,self.button_locwren_id,"0",pos=(505,ry+13+15+30),size=(24,24))
    self.button_locwren.memaddr = '0x15[6:6]' 
    self.Bind(wx.EVT_TOGGLEBUTTON,self.doLocwrenToggle,id=self.button_locwren_id)

    
    offset = 0
    self.slider_dcgain = wx.Slider(self,-1,name='dcgain',value=0,minValue=0,maxValue=7,pos=(rx+30, ry+60),size=(-1, -1),style=wx.SL_VERTICAL|wx.SL_AUTOTICKS|wx.SL_LABELS)
    self.slider_dcgain.memaddr = '0x15[2:0]'
    self.slider_dcgain.SetTickFreq(1, 1)
    wx.StaticText(self, -1, "DCGAIN", pos=(rx+35, ry+180))
    
    offset = offset + 80
    self.slider_3db_freq = wx.Slider(self,-1,name='3db_freq',value=0,minValue=0,maxValue=7,pos=(rx+offset+30, ry+60),size=(-1, -1),style=wx.SL_VERTICAL|wx.SL_AUTOTICKS|wx.SL_LABELS)
    self.slider_3db_freq.memaddr = '0x15[5:3]'
    self.slider_3db_freq.SetTickFreq(1, 1)
    wx.StaticText(self, -1, "3DB_FREQ", pos=(rx+offset+35, ry+180))
       
    offset = offset + 80
    self.slider_lowf_agc = wx.Slider(self,-1,name='lowf_agc',value=0,minValue=0,maxValue=7,pos=(rx+offset+30, ry+60),size=(-1, -1),style=wx.SL_VERTICAL|wx.SL_AUTOTICKS|wx.SL_LABELS)
    self.slider_lowf_agc.memaddr = '0x18[6:4]'
    self.slider_lowf_agc.SetTickFreq(1, 1)
    wx.StaticText(self, -1, 'LOWF_AGC', pos=(rx+offset+35, ry+180))
    
    offset = offset + 80
    self.slider_lowf_agc = wx.Slider(self,-1,name='hif_agc',value=0,minValue=0,maxValue=31,pos=(rx+offset+30, ry+60),size=(-1, -1),style=wx.SL_VERTICAL|wx.SL_AUTOTICKS|wx.SL_LABELS)
    self.slider_lowf_agc.memaddr = '0x19[4:0]'
    self.slider_lowf_agc.SetTickFreq(1, 1)
    wx.StaticText(self, -1, 'HIF_AGC', pos=(rx+offset+35, ry+180))


    offset = 0
    self.slider_tapgain = wx.Slider(self,-1,name='tapgain',value=0,minValue=0,maxValue=7,pos=(rx+offset+30, ry+150+60),size=(-1, -1),style=wx.SL_VERTICAL|wx.SL_AUTOTICKS|wx.SL_LABELS)
    self.slider_tapgain.memaddr = '0x16[2:0]'
    self.slider_tapgain.SetTickFreq(1, 1)
    wx.StaticText(self, -1, "TAPGAIN", pos=(rx+offset+35, ry+250+80))
 
    offset = offset + 80
    self.slider_tap1gain = wx.Slider(self,-1,name='tap1gain',value=0,minValue=0,maxValue=15,pos=(rx+offset+30, ry+150+60),size=(-1, -1),style=wx.SL_VERTICAL|wx.SL_AUTOTICKS|wx.SL_LABELS)
    self.slider_tap1gain.memaddr = '0x16[6:3]'
    self.slider_tap1gain.SetTickFreq(1, 1)
    wx.StaticText(self, -1, "TAP1GAIN", pos=(rx+offset+35, ry+250+80))
    
    offset = offset + 80
    self.slider_tap2gain = wx.Slider(self,-1,name='tap2gain',value=0,minValue=0,maxValue=15,pos=(rx+offset+30, ry+150+60),size=(-1, -1),style=wx.SL_VERTICAL|wx.SL_AUTOTICKS|wx.SL_LABELS)
    self.slider_tap2gain.memaddr = '0x17[3:0]'
    self.slider_tap2gain.SetTickFreq(1, 1)
    wx.StaticText(self, -1, "TAP2GAIN", pos=(rx+offset+35, ry+250+80))
    
    offset = offset + 80
    self.slider_tap3gain = wx.Slider(self,-1,name='tap3gain',value=0,minValue=0,maxValue=15,pos=(rx+offset+30, ry+150+60),size=(-1, -1),style=wx.SL_VERTICAL|wx.SL_AUTOTICKS|wx.SL_LABELS)
    self.slider_tap3gain.memaddr = '0x17[7:4]'
    self.slider_tap3gain.SetTickFreq(1, 1)
    wx.StaticText(self, -1, "TAP3GAIN", pos=(rx+offset+35, ry+250+80))
    
    offset = offset + 80
    self.slider_tap4gain = wx.Slider(self,-1,name='tap4gain',value=0,minValue=0,maxValue=15,pos=(rx+offset+30, ry+150+60),size=(-1, -1),style=wx.SL_VERTICAL|wx.SL_AUTOTICKS|wx.SL_LABELS)
    self.slider_tap4gain.memaddr = '0x18[3:0]'
    self.slider_tap4gain.SetTickFreq(1, 1)
    wx.StaticText(self, -1, "TAP4GAIN", pos=(rx+offset+35, ry+250+80))
    
    
#    offset = 0
#    self.slider_tap4gain = wx.Slider(self,-1,name='rxterm_cal',value=0,minValue=0,maxValue=7,pos=(rx+offset+30, ry+300+60),size=(-1, -1),style=wx.SL_VERTICAL|wx.SL_AUTOTICKS|wx.SL_LABELS)
#    self.slider_tap4gain.memaddr = '38[6:4]'
#    self.slider_tap4gain.SetTickFreq(1, 1)
#    wx.StaticText(self, -1, "RXTERM_CAL", pos=(rx+offset+35, ry+400+80))
#    
#    
#    offset = offset + 80
#    self.slider_tap4gain = wx.Slider(self,-1,name='rxterm_course',value=0,minValue=0,maxValue=3,pos=(rx+offset+30, ry+300+60),size=(-1, -1),style=wx.SL_VERTICAL|wx.SL_AUTOTICKS|wx.SL_LABELS)
#    self.slider_tap4gain.memaddr = '39[1:0]'
#    self.slider_tap4gain.SetTickFreq(1, 1)
#    wx.StaticText(self, -1, "RXTERM_COURSE", pos=(rx+offset+35, ry+400+80))
#    
#    
#    offset = offset + 80
#    self.slider_tap4gain = wx.Slider(self,-1,name='rxterm_mode',value=0,minValue=0,maxValue=3,pos=(rx+offset+30, ry+300+60),size=(-1, -1),style=wx.SL_VERTICAL|wx.SL_AUTOTICKS|wx.SL_LABELS)
#    self.slider_tap4gain.memaddr = '39[3:2]'
#    self.slider_tap4gain.SetTickFreq(1, 1)
#    wx.StaticText(self, -1, "RXTERM_MODE", pos=(rx+offset+35, ry+400+80))
#    
#
#    
#    offset = offset + 80
#    self.slider_tap4gain = wx.Slider(self,-1,name='rxterm_locwen',value=0,minValue=0,maxValue=1,pos=(rx+offset+30, ry+300+60),size=(-1, -1),style=wx.SL_VERTICAL|wx.SL_AUTOTICKS|wx.SL_LABELS)
#    self.slider_tap4gain.memaddr = '38[7:7]'
#    self.slider_tap4gain.SetTickFreq(1, 1)
#    wx.StaticText(self, -1, "RXTERM_LOCWEN", pos=(rx+offset+35, ry+400+80))
#    


    
    self.Bind(wx.EVT_SLIDER, self.onChanged)
        
  def set_all(self,event):
    for eachRXCB in self.rxcb:
      eachRXCB.SetValue(1)
  def set_none(self,event):
    for eachRXCB in self.rxcb:
      eachRXCB.SetValue(0)
  def doMem(self,event):
    print "enter",self.memwrite
    pars = self.memwrite.GetValue().split(',')
    print pars[0],pars[1],pars[2]
    memWrite(pars[0],int(pars[1]),int(pars[2]))

  def onChanged(self, event):
      print 'changed: %d' % event.EventObject.GetValue(),event.EventObject.Name
      
      for eachRXCB in self.rxcb:
        if eachRXCB.GetValue():
          print eachRXCB.rx,'is checked'
          cLane = eachRXCB.rx
          memWrite(event.EventObject.memaddr,cLane,event.EventObject.GetValue())
      self.Refresh() 
      
  def doLocwrenToggle(self,event):
    val = int(event.EventObject.GetLabel())
    newval = 1-val
    event.EventObject.SetLabel(str(newval))
    print "button"
    for eachRXCB in self.rxcb:
      if eachRXCB.GetValue():
        print eachRXCB.rx,'is checked'
        cLane = eachRXCB.rx
        memWrite(event.EventObject.memaddr,cLane,newval)
    self.Refresh() 
    
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

   
#--------

class cauiTx():
  def __init__(self, *args, **kwargs):
    self.pattern_id = 0
    self.post_id = 0
    self.main_id = 0
    self.pre_id = 0
    self.amp_id = 0
  
class cauiTxPanel(wx.Panel):   

  """  """
  def __init__(self, parent,*args, **kwargs):
    """   """
    wx.Panel.__init__(self, parent, *args, **kwargs)
    self.parent = parent
    self.panel = self
    self.time = 0
    self.adapt_IDs = [-1]*10

    
    
    # Device Radiobox - Check for Gearbox only
    #print '-'*80
    #print ipvar.System.keys()
    #for eachDevice in ipvar.System.keys():
    #  if (eachDevice.find("GB")==0):
    #    self.GB_list.append(eachDevice)
                
    #11-20-2012
	  #Select GB1 Devices only
    self.GB1_list = []
    for eachDevice in ipvar.System.keys():                       
      if (re.search(r'\w+-\w+',eachDevice)):
        pass
      else:
        gb1 = re.search(r'(GB[^2]+)',eachDevice)
        if(gb1) :
          #print 'ipcauiTx: %s'%gb1.group(1)
          self.GB1_list.append(gb1.group(1))
                
                
    self.device = self.GB1_list[0]       
    
    rx = 0
    ry = 0
    self.m_deviceBox = wx.RadioBox ( self, wx.ID_ANY, label='Devices',pos=(rx+10,ry+5), choices = self.GB1_list, style = wx.RA_SPECIFY_COLS, )
    self.Bind(wx.EVT_RADIOBOX,self.refresh_status,id=self.m_deviceBox.GetId()) 

      
    self.stop = 0  

    self.parent.Refresh()
    self.pattern_labels = ['PRBS31','PRBS9','Square','1010','PRBS31','PRBS23','PRBS15','PRBS7']
    self.rxcb = []
    self.cauiTx =[]

    grid = False
    debug = False
 

      
 
    if grid :
      rx = 550
      wx.StaticText(self,-1,'.',pos=(rx,50))
      wx.StaticText(self,-1,'. 100',pos=(rx,100))
      wx.StaticText(self,-1,'.',pos=(rx,150))
      wx.StaticText(self,-1,'. 200',pos=(rx,200))
      wx.StaticText(self,-1,'.',pos=(rx,250))
      wx.StaticText(self,-1,'. 300',pos=(rx,300))
      wx.StaticText(self,-1,'.',pos=(rx,350))
      wx.StaticText(self,-1,'. 400',pos=(rx,400))
      wx.StaticText(self,-1,'.',pos=(rx,450))
      wx.StaticText(self,-1,'. 500',pos=(rx,500))
    
      wx.StaticText(self,-1,'.',pos=(000,500))    
      wx.StaticText(self,-1,'.',pos=(100,500))
      wx.StaticText(self,-1,'.',pos=(200,500))
      wx.StaticText(self,-1,'.',pos=(300,500))
      wx.StaticText(self,-1,'.',pos=(400,500))
      wx.StaticText(self,-1,'.',pos=(500,500))
      rx = 0


    
    ry = 30
    wx.StaticText(self,-1,'CAUI Lane',pos=(rx+230,ry+5))
    self.button_all_id = wx.NewId()
    wx.Button(self,self.button_all_id,'All',pos=(rx+20,ry+23),size=(35,24))
    self.Bind(wx.EVT_BUTTON,self.set_all,id=self.button_all_id)
    
    self.button_none_id = wx.NewId()
    wx.Button(self,self.button_none_id,'None',pos=(rx+57,ry+23),size=(35,24))
    self.Bind(wx.EVT_BUTTON,self.set_none,id=self.button_none_id)
    
    for i in range(0,10):
      self.rxcb.append(wx.CheckBox(self, -1,label=str(i),pos=(rx+100+(i*30),ry+13+15)))
      self.rxcb[-1].rx = i

    # PI Lock
    rx=0
    ry=55
    wx.StaticText(self, -1, "Lock 28G PI3", pos=(440, ry+2))
    self.button_lockpi = wx.Button(self,wx.ID_ANY,label='',pos=(510,ry-1),size=(24,24))
    self.Bind(wx.EVT_BUTTON,self.onLockPI,id=self.button_lockpi.GetId())
    #--------------------------------------------------------------------------------------
    #wx.StaticText(self, -1, "LOCK PI", pos=(410, ry+13+15+35))
    #self.button_locwren_id = wx.NewId()  
    #self.button_locwren = wx.ToggleButton(self,self.button_locwren_id,"0",pos=(505,ry+13+15+30),size=(24,24))
    #self.button_locwren.memaddr = '0x15[6:6]' 
    #self.Bind(wx.EVT_TOGGLEBUTTON,self.doLocwrenToggle,id=self.button_locwren_id)   

    
    rx=0
    ry=30
# Slider      
    offset = 0
    self.slider_post = wx.Slider(self,-1,name='post',value=3,minValue=0,maxValue=9,pos=(rx+30, ry+60),size=(-1, -1),style=wx.SL_VERTICAL|wx.SL_AUTOTICKS|wx.SL_LABELS)
    self.slider_post.memaddr = '0x1551[15:12]'
    self.slider_post.SetTickFreq(1, 1)
    wx.StaticText(self, -1, "POST", pos=(rx+35, ry+180))
    
    offset = offset + 80
    self.slider_main = wx.Slider(self,-1,name='main',value=12,minValue=0,maxValue=24,pos=(rx+offset+30, ry+60),size=(-1, -1),style=wx.SL_VERTICAL|wx.SL_AUTOTICKS|wx.SL_LABELS)
    self.slider_main.memaddr = '0x1551[10:6]'
    self.slider_main.SetTickFreq(1, 1)
    wx.StaticText(self, -1, "MAIN", pos=(rx+offset+35, ry+180))
       
    offset = offset + 80
    self.slider_pre = wx.Slider(self,-1,name='pre',value=0,minValue=0,maxValue=3,pos=(rx+offset+30, ry+60),size=(-1, -1),style=wx.SL_VERTICAL|wx.SL_AUTOTICKS|wx.SL_LABELS)
    self.slider_pre.memaddr = '0x1551[2:0]'
    self.slider_pre.SetTickFreq(1, 1)
    wx.StaticText(self, -1, 'PRE', pos=(rx+offset+35, ry+180))
    
    offset = offset + 80
    self.slider_amp = wx.Slider(self,-1,name='amp',value=3,minValue=0,maxValue=7,pos=(rx+offset+30, ry+60),size=(-1, -1),style=wx.SL_VERTICAL|wx.SL_AUTOTICKS|wx.SL_LABELS)
    self.slider_amp.memaddr = '0x1551[5:3]'
    self.slider_amp.SetTickFreq(1, 1)
    wx.StaticText(self, -1, 'AMP', pos=(rx+offset+35, ry+180))

    self.Bind(wx.EVT_SLIDER, self.onChanged)
    
    #------------------------------------------------------------------------------------   
    #Default
    self.button_default_id = wx.NewId()
    wx.Button(self,self.button_default_id,"Default",pos=(350,ry+75),size=(50,24))
    self.Bind(wx.EVT_BUTTON,self.set_default,id=self.button_default_id)
    #------------------------------------------------------------------------------------
    #Write Box
    self.box_reg_id = wx.NewId()
    self.box_reg = wx.TextCtrl(self,self.box_reg_id,'0x3318',pos=(350,ry+120),size=(60,24))
    #self.Bind(wx.EVT_TEXT_ENTER,self.reg_write_enter)
    #Write Button
    self.button_write_id = wx.NewId()
    wx.Button(self,self.button_write_id,"Write",pos=(425,ry+120),size=(40,24))
    self.Bind(wx.EVT_BUTTON,self.button_write,id=self.button_write_id)
    
    
    #------------------------------------------------------------------------------------    
    #Pattern Buttons - PRBS, Square, Hi-Freq
     
     
    rx=30
    ry=ry+210
    self.button_prbs7_id = wx.NewId()
    self.button_prbs7=wx.Button(self,self.button_prbs7_id,"PRBS7",pos=(rx,ry),size=(50,24))
    self.Bind(wx.EVT_BUTTON,self.button_pattern,id=self.button_prbs7_id)   
    
    rx = rx + 60
    self.button_prbs31_id = wx.NewId()
    self.button_prbs31=wx.Button(self,self.button_prbs31_id,"PRBS31",pos=(rx,ry),size=(50,24))
    self.Bind(wx.EVT_BUTTON,self.button_pattern,id=self.button_prbs31_id)  
    
    rx = rx + 60
    self.button_sqr_id = wx.NewId()
    self.button_sqr=wx.Button(self,self.button_sqr_id,"SQUARE",pos=(rx,ry),size=(50,24))
    self.Bind(wx.EVT_BUTTON,self.button_pattern,id=self.button_sqr_id)   

    rx = rx + 60
    self.button_hifreq_id = wx.NewId()
    self.button_hifreq=wx.Button(self,self.button_hifreq_id,"1010",pos=(rx,ry),size=(50,24))   
    self.Bind(wx.EVT_BUTTON,self.button_pattern,id=self.button_hifreq_id)   
    

    rx= rx + 60
    self.button_invert_id = wx.NewId()
    self.button_invert = wx.Button(self, self.button_invert_id,'Invert',pos=(rx,ry),size=(50,24))
    self.Bind(wx.EVT_BUTTON,self.button_invert_pattern,id=self.button_invert_id)

    rx= rx + 100
    self.button_injectErr_id = wx.NewId()
    self.button_injectErr = wx.Button(self, self.button_injectErr_id,'Inject Bit Error',pos=(rx,ry),size=(80,24))
    self.Bind(wx.EVT_BUTTON,self.injectErr,id=self.button_injectErr_id)
  
    ry = ry +40
    wx.StaticLine(self,-1,pos = (0,ry),size=(500,3))
    #----------------------------------------------------------------------------------------------------------------
    # Status Panel

    
    rx = 10
    ry = ry+20

    wx.StaticText(self, -1, 'Status', pos=(rx,ry))
    ry = ry + 20
    wx.StaticText(self, -1, 'Lane' , pos=(rx, ry))
    wx.StaticText(self, -1, 'Pattern' , size=(50,25),pos=(50, ry),style=wx.ALIGN_CENTRE)
    wx.StaticText(self, -1, 'Post/Main/Pre' , pos=(125, ry))

    wx.StaticText(self, -1, 'Amp'  , pos=(225, ry)) 
       
    
    ry = ry + 25
    self.button_refresh_id = wx.NewId()
    self.button_refresh = wx.Button(self, self.button_refresh_id, 'Refresh', pos=(275,ry))
    self.Bind(wx.EVT_BUTTON,self.refresh_status,id=self.button_refresh_id)
   
    for i in range (10):
      wx.StaticText(self, -1, str(i), pos=(rx, ry))
      self.cauiTx.append(cauiTx())
      self.cauiTx[i].pattern_id = wx.NewId()
      self.cauiTx[i].pattern = wx.Button(self,self.cauiTx[i].pattern_id,'',size=(50,22),pos=(50,ry),style=wx.RAISED_BORDER|wx.ALIGN_CENTRE|wx.ST_NO_AUTORESIZE)
      
      self.cauiTx[i].post = wx.StaticText(self,-1,'',size=(25,22),pos=(125,ry),style=wx.RAISED_BORDER|wx.ALIGN_CENTRE|wx.ST_NO_AUTORESIZE)
      self.cauiTx[i].main = wx.StaticText(self,-1,'',size=(25,22),pos=(150,ry),style=wx.RAISED_BORDER|wx.ALIGN_CENTRE|wx.ST_NO_AUTORESIZE)
      self.cauiTx[i].pre  = wx.StaticText(self,-1,'',size=(25,22),pos=(175,ry),style=wx.RAISED_BORDER|wx.ALIGN_CENTRE|wx.ST_NO_AUTORESIZE)

      self.cauiTx[i].amp  = wx.StaticText(self,-1,'',size=(25,22),pos=(225,ry),style=wx.RAISED_BORDER|wx.ALIGN_CENTRE|wx.ST_NO_AUTORESIZE)
      ry = ry + 20
    
    
    
  def set_all(self,event,debug=True):
    if(debug): print 'Set All'
    for eachRXCB in self.rxcb:
      eachRXCB.SetValue(1)
      
  def set_none(self,event):
    for eachRXCB in self.rxcb:
      eachRXCB.SetValue(0)
      
  def set_slider(self, post,main,pre,amp):
    self.slider_post.SetValue(post)
    self.slider_main.SetValue(main)
    self.slider_pre.SetValue(pre)
    self.slider_amp.SetValue(amp)
    
  
  def onChanged(self, event, debug=False):
    self.device = self.m_deviceBox.GetStringSelection()
    post = self.slider_post.GetValue()
    main = self.slider_main.GetValue()
    pre = self.slider_pre.GetValue()   
    amp = self.slider_amp.GetValue()  
    value = (post << 12) + (main  << 6) + (amp << 3) + pre
    if(debug): 
      print 'Post: %d ' % self.slider_post.GetValue()
      print 'Main: %d ' % self.slider_main.GetValue()
      print 'Pre: %d ' % self.slider_pre.GetValue()   
      print 'Amp: %d ' % self.slider_amp.GetValue()      
      print 'Value: 0x%x' % value
      
    for eachRXCB in self.rxcb:
      if eachRXCB.GetValue():
        if(debug): 
          print eachRXCB.rx,'is checked'
          #cLane = eachRXCB.rx
          #memWrite(event.EventObject.memaddr,cLane,event.EventObject.GetValue())
          print self.device+'::30.'
          print str(1552+eachRXCB.rx)
          print '0x%x' % value
        ipallreg.regWrite(self.device+"::30."+ str(1552+eachRXCB.rx),value)  
    self.update_Status()        
    self.Refresh() 
    

  #--------------------------------------------------------------------------
  def onLockPI(self,event,debug=False):
    self.device = self.m_deviceBox.GetStringSelection()
    address = self.device+'::30.1184.1:1'
    pi_value = ipallreg.regRead(address)
    
    if(debug) :
      print pi_value
      
      
    if(pi_value) :
      label = '0'
      pi_value = 0
    else:
      label = '1'
      pi_value = 1
    self.button_lockpi.SetLabel(label)
    
    ipallreg.regWrite(address,pi_value)

    self.update_Status() 
    
  #--------------------------------------------------------------------------
  #   Set Defalut Value    
  def set_default(self,event,debug=False):
    self.device = self.m_deviceBox.GetStringSelection()
    default_value = 0x3318
    self.set_slider(3,12,0,3)
    for eachRXCB in self.rxcb:
      if eachRXCB.GetValue():
        address = self.device+'::30.'+ str(1552+eachRXCB.rx)  
        ipallreg.regWrite(address, default_value)  
        value = ipallreg.regRead(address)
        if(debug): print '%s: %s' % (address,value)    
    self.update_Status()        
    self.Refresh()     
    
  #-------------------------------------------------------------------------------
  #   Set Reg Write Enter 
  #
  def button_write(self,event,debug=False):
    self.device = self.m_deviceBox.GetStringSelection()
    value = int(self.box_reg.GetValue(),16)
    post = value >> 12
    main =(value & 0x7c0) >> 6
    pre = (value & 0x7)
    amp = (value & 0x38) >> 3
    self.set_slider(post,main,pre,amp)
    for eachRXCB in self.rxcb:
      if eachRXCB.GetValue():
        address = self.device+'::30.'+ str(1552+eachRXCB.rx)  
        ipallreg.regWrite(address, value)  
        value = ipallreg.regRead(address)
        if(debug): print '%s: %s' % (address,value)           
    self.update_Status()   
    self.Refresh() 
   #----------------------------------------------------------------------------------
   # Patterns PRBS , Square, Hi-Freq
   #20[4:4] - enable
   #20[2:0]
   # PRBS31 - 000, 100
   # PRBS9  - 001
   # Square - 010
   # HiFreq - 011 , Also can be custom
   # PRBS23 - 101
   # PRBS15 - 110
   # PRBS7 -  111
   
   
  def button_pattern(self,event,debug=False):
    self.device = self.m_deviceBox.GetStringSelection()
    #Identify Button by Id
    button_id = event.GetId()
    button = self.FindWindowById(button_id)
    self.button_pattern_clear()
    #button.SetForegroundColour("Green")
    
    event_id = event.GetId()
        
    
    if (event.GetId() == self.button_prbs7_id) : pattern = 7
    elif (event.GetId() == self.button_prbs31_id) :pattern = 4
    elif (event.GetId() == self.button_sqr_id) :pattern = 2
    elif (event.GetId() == self.button_hifreq_id) : pattern = 3

    #Custom Pattern Default 101010
    ipallreg.regWrite(self.device+"::30."+str(30)+".15:0",0xaaaa)
    ipallreg.regWrite(self.device+"::30."+str(31)+".15:0",0xaaaa)     
    
    for eachRXCB in self.rxcb:
      if eachRXCB.GetValue():
        address = self.device+'::30.'+ str(20+eachRXCB.rx)
        ipallreg.regWrite(address+'.4:4', 1) 
        ipallreg.regWrite(address+'.3:0', pattern)   
        if(debug): print '%s: 0x%x' % (address,pattern)       
        
    self.update_Status()
    self.Refresh()    
  
  def button_invert_pattern(self,event,debug=False):
    self.device = self.m_deviceBox.GetStringSelection()
    for eachRXCB in self.rxcb:
      if eachRXCB.GetValue():
        address = self.device+'::30.'+ str(20+eachRXCB.rx)
        invert = ipallreg.regRead(address+'.3:3')
        if(debug) : print 'Invert: ',invert
        invert = int(not invert)
        if(debug) : print 'Invert: ',invert
        #if(invert) :
        #  self.button_invert.SetForegroundColour('Red')
        #else :
        #  self.button_invert.SetForegroundColour('Black')
        
          
        ipallreg.regWrite(address+'.3:3', invert)     
    self.update_Status()        
    self.Refresh()   

  def injectErr(self,event,debug=False):
    self.device = self.m_deviceBox.GetStringSelection()
    #Get and Save Custom Reg LSB
    address = self.device+'::30.'+ str(30)
    byte = ipallreg.regRead(address)
    #Inject 1 bit Error
    ipallreg.regWrite(address, 0x1)
    
    for eachRXCB in self.rxcb:
      if eachRXCB.GetValue():

        address = self.device+'::30.'+ str(20+eachRXCB.rx)          
        ipallreg.regWrite(address+'.5:5', 1)  
        ipallreg.regWrite(address+'.5:5', 0)      
        
    #Write Back Custom Reg LSB
    address = self.device+'::30.'+ str(30)
    #print 'Byte: 0x%x'%byte
    ipallreg.regWrite(address, byte)
    self.Refresh()      
    
  def button_pattern_clear(self):
    buttons = [self.button_prbs7, self.button_prbs31, self.button_sqr, self.button_hifreq]
    for button in buttons:
      button.SetForegroundColour('black')

  def update_Status(self,debug=False):
    self.device = self.m_deviceBox.GetStringSelection()
    address = self.device+'::30.1184.1:1'
    pi3_value = ipallreg.regRead(address)
    self.button_lockpi.SetLabel(str(pi3_value))
    for i in range (10):
      address = self.device+'::30.'+ str(20+i)

      value = ipallreg.regRead(address)
      enable = (value & 0x10) >> 4
      invert = (value & 0x8) >> 3       
      pattern = self.pattern_labels[value & 0x7]
      if(invert): pattern = '!'+pattern
      
      
      address = self.device+'::30.'+ str(1552+i)
      value = ipallreg.regRead(address)
      post = value >> 12
      main = (value & 0x7c0) >> 6
      pre = value & 0x7
      amp = (value & 0x38) >> 3
      
      self.cauiTx[i].pattern.SetLabel(pattern)
      if(enable):
        self.cauiTx[i].pattern.SetBackgroundColour('Pale Green')
      else:
        self.cauiTx[i].pattern.SetLabel('')
        self.cauiTx[i].pattern.SetBackgroundColour(wx.NullColour)
       
      self.cauiTx[i].post.SetLabel(str(post))
      self.cauiTx[i].main.SetLabel(str(main))
      self.cauiTx[i].pre.SetLabel(str(pre))
      self.cauiTx[i].amp.SetLabel(str(amp))
      
      if(debug):
         print i,':: ',pattern, post, main, pre, amp
      
   
  def refresh_status(self,event):
    self.update_Status()
    
  #------------------------------------------------------------------------------------    
  def doLocwrenToggle(self,event):
    val = int(event.EventObject.GetLabel())
    newval = 1-val
    event.EventObject.SetLabel(str(newval))
    print "button"
    for eachRXCB in self.rxcb:
      if eachRXCB.GetValue():
        print eachRXCB.rx,'is checked'
        cLane = eachRXCB.rx
        memWrite(event.EventObject.memaddr,cLane,newval)
    self.Refresh() 
    
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