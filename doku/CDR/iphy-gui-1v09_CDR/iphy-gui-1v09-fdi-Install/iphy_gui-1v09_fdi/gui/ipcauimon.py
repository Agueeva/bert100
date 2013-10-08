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
    
    TIMER_ID = 1000  # pick a number
    self.timer = wx.Timer(self, TIMER_ID)  # message will be sent to the panel
    #self.timer.Start(50)  # x100 milliseconds
    wx.EVT_TIMER(self, TIMER_ID, self.OnTimer)  # call the on_timer function  
   
    rx=250
    ry=20
    self.goButton   = wx.Button(self,-1,label="Start Monitor",pos=(5,25),size=(100,24))
    self.goButton.controls = self
    
    self.stopButton = wx.Button(self,-1,label="Stop Monitor",pos=(5,55),size=(100,24))
    self.stopButton.controls = self

    self.goButton.Bind(wx.EVT_BUTTON,self.OnGo)
    self.stopButton.Bind(wx.EVT_BUTTON,self.OnStop)
    rx=0
    ry=0
    font=wx.Font(10, wx.FONTFAMILY_DEFAULT,wx.NORMAL,wx.BOLD)  
    self.timeText = wx.StaticText(self,-1,'0d 00:00:00',size=(110,22),pos=(rx+120,ry+30),style=wx.RAISED_BORDER|wx.ALIGN_RIGHT|wx.ST_NO_AUTORESIZE)
    self.timeText.SetFont(font)
    
    #self.staticBoxDev = wx.StaticBox(self.panel,-1,label='Device',pos=(rx+250,ry+20),style=wx.RAISED_BORDER,size=(150,40))

    self.checkboxDevice=[]
    Devices = ['GB0']
    #for eachDevice in ipvar.System.keys():
    #  Devices.append(eachDevice)
    self.device = Devices[0]
    self.deviceRB = wx.RadioBox(self, label="Device", pos=(rx+250, ry+20), choices=Devices,  majorDimension=0)
    self.Bind(wx.EVT_RADIOBOX, self.ChangeDevice,self.deviceRB)

     
    #self.stopButton.Enable(False)
    self.stop = 0  
    #self.widgets={} # a list of widgets that get updated from reg values
    #self.regs={}    # a list of registers required to be read for update of this panel
    self.BERT = []
           
    self.rx=30
    self.ry=100
    self.i_y_offset = 105
    for i in range(0,10):
      if i>4:
        self.rx=360
      self.BERT.append(BERT())
      self.BERT[i].widgets = {}
      if self.device.find("GB") == 0:
        self.BERT[i].regs['liveErrCount']  = self.device + '::8.' + str(1600  + (i)) + '.15:0'
        self.BERT[i].regs['prbsLock']      = self.device + '::30.' + str(20  + (i)) + '.15'
        self.BERT[i].regs['cdrLock']       = self.device + '::30.1546.' + str(i)
      else:
        self.BERT[i].regs['liveErrCount']  = self.device + '::30.' + str(48  + (i)) + '.15:0'
        self.BERT[i].regs['prbsLock']      = self.device + '::30.' + str(16  + (i))
      

      self.staticBox3   = wx.StaticBox(self,-1,label='RX'+str(i),pos=(self.rx,self.ry+(i%5)*self.i_y_offset),style=wx.RAISED_BORDER|wx.BOLD,size=(315,self.i_y_offset-5))
      rateText = wx.StaticText(self.panel,-1,"Data Rate",pos=(self.rx+12,self.ry+15+(i%5)*self.i_y_offset))

      font=wx.Font(9, wx.FONTFAMILY_DEFAULT,wx.NORMAL,wx.BOLD)
      rateText.SetFont(font)
      choices=['10.3125','11.18']
      self.BERT[i].rateBox  = wx.ComboBox(self.panel,-1,value=choices[0],choices=choices,pos=(self.rx+10,self.ry+35+(i%5)*self.i_y_offset),style=wx.ALIGN_RIGHT|wx.ST_NO_AUTORESIZE,name='rateCombo',size=(90,22))
      errText = wx.StaticText(self.panel,-1,"Errors",pos=(self.rx+105,self.ry+15+(i%5)*self.i_y_offset))
      errText.SetFont(font)
      
      self.BERT[i].widgets['liveErrCount'] = wx.StaticText(self,-1,'0',size=(45,22),pos=(self.rx+105,self.ry+35+(i%5)*self.i_y_offset),style=wx.RAISED_BORDER|wx.ALIGN_RIGHT|wx.ST_NO_AUTORESIZE)
      
      
      self.BERT[i].widgets['liveErrCount'].name = 'liveErrCount'
      accText = wx.StaticText(self.panel,-1,"Accumulation",pos=(self.rx+151,self.ry+15+(i%5)*self.i_y_offset)) 
      accText.SetFont(font)           
      self.BERT[i].widgetAccumulateError = wx.StaticText(self,-1,'0',size=(80,22),pos=(self.rx+150,self.ry+35+(i%5)*self.i_y_offset),style=wx.RAISED_BORDER|wx.ALIGN_RIGHT|wx.ST_NO_AUTORESIZE)

      self.BERT[i].widgets['prbsLock']    = wx.StaticText(self,-1,' ',size=(60,22),pos=(self.rx+100,self.ry+63+(i%5)*self.i_y_offset),style=wx.RAISED_BORDER|wx.ALIGN_CENTER|wx.ST_NO_AUTORESIZE)

      self.BERT[i].widgets['prbsLock'].name = 'prbsLock'

      self.BERT[i].widgets['cdrLock']    = wx.StaticText(self,-1,'CDR',size=(27,22),pos=(self.rx+160,self.ry+63+(i%5)*self.i_y_offset),style=wx.RAISED_BORDER|wx.ALIGN_CENTER|wx.ST_NO_AUTORESIZE)

      self.BERT[i].widgets['cdrLock'].name = 'cdrLock'
      
      
      
      self.BERT[i].widgets['liveErrCount'].regStr = self.BERT[i].regs['liveErrCount']
      self.BERT[i].widgets['prbsLock'].regStr = self.BERT[i].regs['prbsLock']
      self.BERT[i].widgets['cdrLock'].regStr = self.BERT[i].regs['cdrLock']   
      
         
      bitsText = wx.StaticText(self.panel,-1,"Bits",pos=(self.rx+10,self.ry+67+(i%5)*self.i_y_offset))
      bitsText.SetFont(font)
      self.BERT[i].bitsWidget = wx.StaticText(self,-1,'0',size=(62,22),pos=(self.rx+38,self.ry+63+(i%5)*self.i_y_offset),style=wx.RAISED_BORDER|wx.ALIGN_RIGHT|wx.ST_NO_AUTORESIZE)
      font=wx.Font(12, wx.FONTFAMILY_DEFAULT,wx.NORMAL,wx.BOLD)  
      berText = wx.StaticText(self.panel,-1,"BER",pos=(self.rx+190,self.ry+65+(i%5)*self.i_y_offset))
      berText.SetFont(font)
      self.BERT[i].ber        = wx.StaticText(self,-1,'',size=(85,28),pos=(self.rx+225,self.ry+61+(i%5)*self.i_y_offset),style=wx.RAISED_BORDER|wx.ALIGN_RIGHT|wx.ST_NO_AUTORESIZE)
      self.BERT[i].ber.SetFont(font)
      self.BERT[i].ber.SetLabel(str(0))
      font=wx.Font(8, wx.FONTFAMILY_DEFAULT,wx.NORMAL,wx.BOLD)
      self.BERT[i].saturationWarning = wx.StaticText(self,-1,'Saturated',size=(70,22),pos=(self.rx+230,self.ry+35+(i%5)*self.i_y_offset),style=wx.RAISED_BORDER|wx.ALIGN_CENTRE|wx.BOLD|wx.ST_NO_AUTORESIZE)
      self.BERT[i].saturationWarning.SetBackgroundColour(wx.Colour(200,50,0))
      self.BERT[i].saturationWarning.SetForegroundColour('WHITE')
      self.BERT[i].saturationWarning.SetFont(font)
      self.BERT[i].saturationWarning.Show(False)
      #self.prbsLock.SetBackgroundColour('WHITE')

    #self.parent.cdrMonitorPanel = cdrMonitorPanel(self.notebook, -1)
    #
    #print "Updating BERT[i].widgets"
    #self.UpdateBERT[i].widgets()
    self.parent.Refresh()
 
  def ChangeDevice(self,event):
    global regs
    print "Device Changed",event.String 
    self.device = event.String

    for i in range(0,10):
      self.BERT[i].regs = {}    
      if self.device.find("GB") == 0:
        self.BERT[i].regs['liveErrCount']  = self.device + '::8.' + str(1600  + (i)) + '.15:0'
        self.BERT[i].regs['prbsLock']      = self.device + '::30.' + str(16  + (i)) + '.15'
        self.BERT[i].regs['cdrLock']       = self.device + '::30.1546.' + str(i)
      else:
        self.BERT[i].regs['liveErrCount']  = self.device + '::30.' + str(48  + (i)) + '.15:0'
        self.BERT[i].regs['prbsLock']        = self.device + '::30.' + str(16  + (i))


      self.BERT[i].widgets['liveErrCount'].regStr = self.BERT[i].regs['liveErrCount']
      self.BERT[i].widgets['prbsLock'].regStr = self.BERT[i].regs['prbsLock']
      self.BERT[i].widgets['cdrLock'].regStr = self.BERT[i].regs['cdrLock']
    self.OnStop(True)
    
  def OnTimer(self,event):
      self.ReadTheWidgetRegisters()      
      self.UpdateWidgets()
      self.Refresh()
 
  def ReadTheWidgetRegisters(self,zeroThem=False):
    for i in range (0,10):
      for eachReg in self.BERT[i].regs:
        #print "read",eachReg,self.regs[eachReg]
        (device, reg, hiLsb, loLsb) = re.search('(.*)::(\d+\.\d+)\.*(\d*):*(\d*)',self.BERT[i].regs[eachReg]).groups()
        device = self.device
        val = ipallreg.regRead(device+'::'+reg)
        #print self.BERT[i].regs[eachReg],"came back with",val,device,reg
        if zeroThem:
          ipvar.System[device].regDefTab[reg].lastRead = 0
        else:
          ipvar.System[device].regDefTab[reg].lastRead = val
          
    
  def UpdateWidgets(self): 
    now = time.time()
    timeSpan = now - self.time
    secs  = int(timeSpan % 60)
    mins  = (int((timeSpan - secs)/60) % 60)
    hours = (int(timeSpan/(60*60))) % 24
    days  = (int(timeSpan/(60*60*24)))    
    self.timeText.SetLabel('%dd %02d::%02d::%02d' % (days,hours,mins,secs))
         
    for rxn in range(0,10):
      for eachWidget in self.BERT[rxn].widgets:
        val = self.getLastRead(self.BERT[rxn].widgets[eachWidget].regStr)
        #print eachWidget,self.BERT[i].widgets[eachWidget].regStr,val
  
        if self.BERT[rxn].widgets[eachWidget].name.find('liveErrCount') == 0: 
          self.BERT[rxn].widgets[self.BERT[rxn].widgets[eachWidget].name].SetLabel(str(val))
          if int(val) == 65535:
            self.BERT[rxn].errorSaturation = 1
            self.BERT[rxn].saturationWarning.Show(True)
            #self.BERT[rxn].ber.SetBackgroundColour(wx.Colour(200,50,0))
            #print "counter saturated"
            self.BERT[rxn].errorSaturation = 1
          
        elif self.BERT[rxn].widgets[eachWidget].name.find('prbsLock') == 0: 
          if val == 1:
            self.BERT[rxn].widgets[self.BERT[rxn].widgets[eachWidget].name].SetBackgroundColour('PALE GREEN')
            self.BERT[rxn].widgets[self.BERT[rxn].widgets[eachWidget].name].SetLabel('PRBS')
          else:
            self.BERT[rxn].widgets[self.BERT[rxn].widgets[eachWidget].name].SetBackgroundColour(wx.Colour(200,50,0))
            self.BERT[rxn].widgets[self.BERT[rxn].widgets[eachWidget].name].SetLabel('SYNC Loss')
            self.BERT[rxn].syncLoss = 1

        elif self.BERT[rxn].widgets[eachWidget].name.find('cdrLock') == 0: 
          if val == 1:
            self.BERT[rxn].widgets[self.BERT[rxn].widgets[eachWidget].name].SetBackgroundColour('PALE GREEN')
          else:
            self.BERT[rxn].widgets[self.BERT[rxn].widgets[eachWidget].name].SetBackgroundColour(wx.Colour(200,50,0))
            
                
      val = int(self.BERT[rxn].widgetAccumulateError.GetLabel()) + int(self.BERT[rxn].widgets['liveErrCount'].GetLabel())
      self.BERT[rxn].widgetAccumulateError.SetLabel(str(val))
      bits = (time.time() - self.time) * float(self.BERT[rxn].rate)*1e9
      self.BERT[rxn].bitsWidget.SetLabel("%05.03g" % bits)
      ber = int(self.BERT[rxn].widgetAccumulateError.GetLabel())/bits
      if ber == 0:
        ber = 1.0/bits
      self.BERT[rxn].ber.SetLabel("%05.03g" % ber)
      if (val == 0) and (self.BERT[rxn].syncLoss == 0) and (self.BERT[rxn].errorSaturation == 0):
        self.BERT[rxn].ber.SetBackgroundColour('PALE GREEN')
      else:
        self.BERT[rxn].ber.SetBackgroundColour(wx.Colour(250,250,100))    
    self.Refresh()    
    self.parent.Refresh()  

  def OnGo(self,event):
    self.ReadTheWidgetRegisters()
    for rxn in range(0,10):
      self.BERT[rxn].widgetAccumulateError.SetLabel(str(0))
      self.BERT[rxn].ber.SetBackgroundColour('Blue')
      self.BERT[rxn].rate = self.BERT[rxn].rateBox.GetValue()
      self.BERT[rxn].saturationWarning.Show(False)
      self.BERT[rxn].syncLoss = 0
      self.BERT[rxn].errorSaturation = 0

    self.timer.Start(200)
    self.time = time.time()
    self.goButton.SetLabel('Restart Monitor')
    
    #self.UpdateWidgets()
    
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

    