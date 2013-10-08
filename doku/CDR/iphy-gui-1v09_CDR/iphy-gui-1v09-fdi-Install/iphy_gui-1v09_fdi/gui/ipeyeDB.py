#11/6 - Individual file naming
#11/6 - Increase EyeScan Speed
#11/7 - Add legacy csv option
#11/13 - Added wx.Yield() in DoVeriticalSteps/F to keep gui from freezing during EyeScans
import wx
import re
import time

import ipallreg
import ipvar


class DrawWindow(wx.Window):
  def __init__(self, parent, ID):
    wx.Window.__init__(self, parent, ID, size=(3*128+40,270))
    
    #----------------------------------------------------------------
    self.origX = 40
    self.origY = 10
    #----- Default Color Map ------------------------------------------
    red = 255
    green = 255
    blue = 255
    self.colormap = []
    for i in range(2500):
      if i < 201:
        red = 100+int(i*0.75)
        green = int(i)
        blue = 0
      elif i<501:
        red = 250-int((i-200)/1.2)
        green = 255-int((i-200)/1.2)
        blue = int((i-200)/2)
      else:
        red = int((i-500)/8)
        green = int((i-500)/8)
        blue = int((i-500)/20)+150
      self.colormap.append(wx.Colour(red,green,blue))    
    self.colormap[0]=wx.Colour(255,255,255)
    
    self.InitBuffer()


    self.Bind(wx.EVT_SIZE, self.OnSize)
    self.Bind(wx.EVT_IDLE, self.OnIdle)
    self.Bind(wx.EVT_PAINT, self.OnPaint)

  def InitBuffer(self,saOffset=0,debug=False):
    size = self.GetClientSize()
    if(debug): 
      print 'InitBuffer: w=%d, h=%d saOffset=%d'%(size.width,size.height,saOffset)
    self.buffer = wx.EmptyBitmap(size.width, size.height)
    dc = wx.BufferedDC(None, self.buffer)
    dc.SetBackground(wx.Brush(self.GetBackgroundColour()))
    dc.Clear()
    self.DrawGraph(dc,saOffset)  
    self.reInitBuffer = False

  #----------------------------------------------------------------              
  def DrawGraph(self,dc,saOffset):
    #print 'Draw Graph'
    self.saOffset=saOffset
    dc.SetPen(wx.Pen("GRAY",1)) 
    dc.DrawRectangle(self.origX,self.origY,128*3,256)
    for x in range(0,129,8):
      dc.DrawLine(self.origX+x*3,self.origY,self.origX+x*3,self.origY+256)
    for y in range(0,256,16):
      dc.DrawLine(self.origX,self.origY+256-y,self.origX+128*3,self.origY+256-y)
    dc.SetPen(wx.Pen("BLACK",1))             
    dc.DrawLine(self.origX+64*3+1,self.origY+0,self.origX+64*3+1,self.origY+256)
    dc.DrawLine(self.origX,self.origY+128,self.origX+128*3,self.origY+128)
    if(self.saOffset != 0):
      dc.SetPen(wx.Pen("Red",1))            
      dc.DrawLine(self.origX,self.origY+256-(128+self.saOffset),self.origX+128*3,self.origY+256-(128+self.saOffset))

  #----------------------------------------------------------------
  def DrawEyeX(self,eye):
    self.EyeX = eye
    dc = wx.BufferedDC(wx.ClientDC(self), self.buffer)
    #dc.SetPen(wx.Pen("Blue",1))       
    #dc.DrawRectangle(self.origX+20,self.origY+20,128,128)
    for y in range(256):
      for x in range(128):      
        idx = int(self.EyeX[y][x])   
        if ((idx>0) and (idx<2500)):     
          try:
            dc.SetPen(wx.Pen(self.colormap[idx],1))    
          except:
            print 'Error - DrawEyeX, idx = %d out of range'%idx
          #dc.SetPen(wx.Pen(self.SetColour(self.eye[eachY][eachX]),1))
          #dc.DrawPoint(x,y)
          dc.DrawLine(self.origX+x*3,self.origY+256-y,self.origX+x*3+3,self.origY+256-y)  

  def DrawDiamond(self,minH,minV,maxH,maxV,saOffset):
  #----
  #if self.UpdateFastDiamond and self.controls.cbMode['Fast Diamond'].Value:

    self.minH=minH
    self.minV=minV
    self.maxH=maxH
    self.maxV=maxV
    self.saOffset=saOffset
    dc = wx.BufferedDC(wx.ClientDC(self), self.buffer)
    myPen = wx.Pen("BLUE",2)
    myPen.SetWidth(3)
    dc.SetPen(myPen)
    dc.DrawLine(self.origX+self.minH*3,self.origY+256-(128+self.saOffset),self.origX+64*3+2,self.origY+256-self.maxV)
    dc.DrawLine(self.origX+64*3+2,self.origY+256-self.maxV,self.origX+self.maxH*3,self.origY+256-(128+self.saOffset))
    dc.DrawLine(self.origX+self.maxH*3,self.origY+256-(128+self.saOffset),self.origX+64*3+2,self.origY+256-self.minV)
    dc.DrawLine(self.origX+64*3+2,self.origY+256-self.minV,self.origX+self.minH*3,self.origY+256-(128+self.saOffset))  

  def DrawLiveHeight(self,upper_vert,lower_vert,saOffset):

    eachX = 64
    self.saOffset = saOffset
    dc = wx.BufferedDC(wx.ClientDC(self), self.buffer)
    dc.SetBackground(wx.Brush(self.GetBackgroundColour()))   
    dc.Clear()
    self.DrawGraph(dc,saOffset)  
    
    dc.SetPen(wx.Pen("Blue",2))       
    #dc.DrawRectangle(self.origX+20,self.origY+20,128,128)
    #dc.SetPen(wx.Pen(self.SetColour(self.eye[eachY][eachX]),2))
    
    
    dc.DrawLine(self.origX+eachX*3-20,self.origY+256-upper_vert,self.origX+eachX*3+26,self.origY+256-upper_vert)   
    dc.DrawLine(self.origX+eachX*3-20,self.origY+256-lower_vert,self.origX+eachX*3+26,self.origY+256-lower_vert) 
    
  def OnSize(self, event):
    #print 'OnSize'
    #self.reInitBuffer = True
    self.Refresh(True)
    
  def OnIdle(self, event):
  
    if self.reInitBuffer:
      self.InitBuffer()
      self.Refresh(False)

  def OnPaint(self, event):
    dc = wx.BufferedPaintDC(self, self.buffer)
      


class eyeScanPanel(wx.Panel):
  """  """
  def __init__(self, parent, *args, **kwargs):
    """   """
    wx.Panel.__init__(self, parent, *args, **kwargs)
    self.parent = parent
    #self.panel = self
    
    self.input = []
    self.output = []
    self.eye = {}
    self.outputFilename = 'eye_out.csv'
    self.fid = -1
    self.rectSize=5
    self.origX = 30
    self.origY = 135

    self.rxn = -1
    self.txn = -1
    self.device = 'Undefined'
    self.offset = -1
    self.DEBUG = 0
    self.stop = 0

    self.saOffset = 0
    self.UpdateFastDiamond = False
    self.PostPlotSummary = False
    #self.frame.Bind(wx.EVT_CHECKBOX,self.OnCheckBox)
    self.minV = 128
    self.maxV = 128
    self.minH = 64
    self.maxH = 64
    #self.colour = True
    #self.colourmap = []
    #self.colourmap.append([255,255,255])   
    #---------
    # Build the space and empty results
    for eachY in range(0,256):
      self.eye[eachY] = {}
      for eachX in range(0,128):
        self.eye[eachY][eachX] = -1  
                        


    #--------------------------------------------------------------------------
    # Start/Stop Control
    #self.panel = wx.Panel(self,-1, style=wx.SUNKEN_BORDER)

    self.panel = wx.Panel(self,-1)
    self.startButton   = wx.Button(self.panel,-1,label="Start Eye Scan",pos=(5,25),size=(100,24))      
    self.stopButton = wx.Button(self.panel,-1,label="Stop Eye Scan",pos=(5,50),size=(100,22))
    self.stopButton.Enable(False)

    #self.refreshButton = wx.Button(self.panel,-1,label="Refresh Plot",pos=(5,75),size=(100,22))
    #self.refreshButton.controls = self 
    #self.refreshButton.Enable(False)
      
    # Modes
    rx=115
    ry=20      
    self.staticBox3 = wx.StaticBox(self.panel,-1,label='Modes',pos=(rx,ry),style=wx.RAISED_BORDER,size=(130,107))

    self.cbMode={}
    self.cbMode['Inner Eye']       = wx.RadioButton(self.panel,-1,"Inner Eye",pos=(rx+10,ry+20),style=wx.RB_GROUP)     
    self.cbMode['Inner Eye'].Enable(False)
    self.cbMode['Full Scan']       = wx.RadioButton(self.panel,-1,"Full Scan",pos=(rx+10,ry+40))
    self.cbMode['Full Scan'].Value = True
    self.cbMode['Fast Diamond'] = wx.RadioButton(self.panel,-1,'Fast Diamond',pos=(rx+10,ry+60))
    self.cbMode['Live Height']    = wx.RadioButton(self.panel,-1,"Live Height",pos=(rx+10,ry+80))
    self.cbMode['Live Height'].Enable(True)
    
    # Device
    rx=250
    ry=20
    #self.staticBoxDev = wx.StaticBox(self.panel,-1,label='Device',pos=(rx,ry),style=wx.RAISED_BORDER,size=(220,40))
      
    #self.checkboxDevice=[]
    #Devices = 0
    #for eachDevice in ipvar.System.keys():
    #  if Devices == 0:
    #    self.checkboxDevice.append(wx.RadioButton(self.panel,-1,eachDevice,pos=(rx+10+70*Devices,ry+16),style=wx.RB_GROUP))
    #    Devices = Devices + 1
    #  else:
    #    self.checkboxDevice.append(wx.RadioButton(self.panel,-1,eachDevice,pos=(rx+10+70*Devices,ry+16)))
    #    Devices = Devices + 1      
    #self.Device_list = ipvar.System.keys()
    #self.m_deviceBox = wx.RadioBox(self.panel, label="Device", pos=(rx, ry), choices=self.Device_list,  majorDimension=0)  
    #self.device = self.Device_list[0]    	
	  #----
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
          #print 'ipeyeDB - CDR: %s'%cdr.group(1)
          self.CDR_list.append(cdr.group(1)) 
        if(gb1) :
          #print 'ipeyeDB GB1: %s'%gb1.group(1)
          self.GB1_list.append(gb1.group(1))
                            
    self.Device_list = self.CDR_list + self.GB1_list 
    
    self.m_deviceBox = wx.RadioBox(self.panel, label="Device", pos=(rx, ry), choices=self.Device_list,  majorDimension=0)  
    self.device = self.Device_list[0]   
    
    # Receiver
    choice_list = ['Rx0','Rx1','Rx2','Rx3']
    self.m_checkBoxRx = wx.RadioBox(self.panel,-1,label='Receiver', pos=(rx,ry+44),choices=choice_list,majorDimension=0)
    #self.staticBoxRx = wx.StaticBox(self.panel,-1,label='Receiver',pos=(rx,ry+40),style=wx.RAISED_BORDER,size=(210,40))
    #self.checkboxRx=[]
    #self.checkboxRx.append(wx.RadioButton(self.panel,-1,"Rx0",pos=(rx+10,ry+56),style=wx.RB_GROUP)
    #self.checkboxRx.append(wx.RadioButton(self.panel,-1,"Rx1",pos=(rx+60,ry+56)))
    #self.checkboxRx.append(wx.RadioButton(self.panel,-1,"Rx2",pos=(rx+110,ry+56)))
    #self.checkboxRx.append(wx.RadioButton(self.panel,-1,"Rx3",pos=(rx+160,ry+56)))    
      
    # Options
    self.cdrSave = wx.CheckBox(self.panel,-1,label='Capture CDR PI',pos=(rx+10,ry+90))
    self.eyeColour = wx.CheckBox(self.panel,-1,label='ColourMap',pos=(rx+120,ry+90))
    self.eyeColour.SetValue(True)
    self.legacyCSV = wx.CheckBox(self.panel,-1,label='Legacy CSV',pos=(rx+200,ry+90))
    self.legacyCSV.SetValue(False)
    
    # Event Binding
    self.Bind(wx.EVT_BUTTON, self.Do, id=self.startButton.GetId())
    self.Bind(wx.EVT_BUTTON, self.Stop, id=self.stopButton.GetId())

        
    # Draw Window
    self.draw = DrawWindow(self, -1)
    
    # Eye Scan Info
    self.pps1 = wx.StaticText(self,-1,'')
    self.pps2 = wx.StaticText(self,-1,'')
    self.pps3 = wx.StaticText(self,-1,'')
    self.pps4 = wx.StaticText(self,-1,'')   
   
    # Gauge
    self.gauge = wx.Gauge(self,-1,range=127,size=(128*3,8))
    self.gauge.Show(True)
    
    # Sizers
    box = wx.BoxSizer(wx.VERTICAL)
    box.Add(self.panel, 0, wx.EXPAND)  
    hSizer = wx.BoxSizer(wx.HORIZONTAL)

    #Draw Window + Gauge    
    vSizer0 = wx.BoxSizer(wx.VERTICAL)
    vSizer0.Add(self.draw, 0, wx.EXPAND)    
    vSizer0.Add(self.gauge, 0, wx.ALIGN_RIGHT)    
    hSizer.Add(vSizer0,0,wx.EXPAND)  
    
    #Space hSizwer
    hSizer.Add((5,5),0,wx.EXPAND)
    
    #Text Panel
    vSizer1 = wx.BoxSizer(wx.VERTICAL)
    vSizer1.Add((10,20),0,wx.EXPAND)
    vSizer1.Add(self.pps1,0, wx.EXPAND)
    vSizer1.Add(self.pps2,0, wx.EXPAND)
    vSizer1.Add(self.pps3,0, wx.EXPAND)
    vSizer1.Add(self.pps4,0, wx.EXPAND)
    hSizer.Add(vSizer1,0,wx.EXPAND)    
    
    
    box.Add(hSizer, 0, wx.EXPAND)


    self.SetAutoLayout(True)
    self.SetSizer(box)
    self.Layout()   
    
  #--------------------------------------------------------------------
  def DoVerticalStepsF(self,myRange,myX,forceFull=False,refreshWindow=True,debug=False):
    if(debug):
      print '0x%04x: %d' %(myX,len(myRange))         
        
    #global pi,esVertOffset, esResult,ctle_state,saOffset
    lasterror = 0
    #maxVOffset = offset
    #minVOffset = offset
    piCode = ipallreg.regRead(self.device+'::'+self.pi)
    quad = (piCode & 0b1100000000)/256
    piCode   = piCode & 0b0011111111
    pi_string = str(piCode) + ',' + str(quad)
    wx.Yield()
    for vertical in myRange:
      if lasterror == 0 or self.cbMode['Full Scan'].Value or forceFull:
        self.maxVOffset = vertical      
        regVal = (vertical << 8) + myX
        #regWrite(device+'::'+esVertOffset,vertical)
        ipallreg.regWrite(self.device+'::'+self.esCTRL2,regVal)     
        lasterror = ipallreg.regRead(self.device+'::'+self.esResult)
        if(myX&0x80):
          x = 64+(myX&0x7f)
        else:
          x = 64-myX    
        self.eye[vertical][x] = lasterror
        #self.legacy_eye_csv
 
        if (self.legacyCSV.IsChecked()):
          self.fid.write('%s,%s,%s,%s,%s,%s\n' %(x,vertical,vertical-(self.saOffset+128), lasterror,pi_string,ipallreg.regRead(self.device+'::'+self.ctle_state)))
          

      else:
        #self.panel.Refresh()
        return(vertical)

      if(debug):
        if((vertical == 0)&(x%8 == 0)): 
          es_ctrl1 = ipallreg.regRead(self.device+'::'+self.esCTRL1)
          es_ctrl2 = ipallreg.regRead(self.device+'::'+self.esCTRL2)
          print '%d: %d,%d,(0x%04x,0x%04x) - %d' %(myX,x,vertical,es_ctrl1,es_ctrl2,lasterror)         
        
    #if(self.cbMode['Full Scan'].Value & ((myX%4) ==0)):
    if(self.cbMode['Full Scan'].Value):
      self.draw.DrawEyeX(self.eye)     
    #if refreshWindow:
    #  if ((myX % 4)==0):
    #    self.panel.Refresh()  
    #    self.panel.Update()                     
  

  #--------------------------------------------------------------------
  def clearEye(self):
    for y in range(256):
      for x in range(128):
        self.eye[y][x] = -1
  #--------------------------------------------------------------------
     
  #Returns Eye Horizontal offset to right Side    
  def rightEdge(self,debug=True):
    regVal=ipallreg.regRead(self.device+'::'+self.esCTRL2)
    myX = 0xff & regVal #Bit[7:0] direction, Horizontal Offset
    
    if(debug): print 'Right Edge: myX - 0x%02x'%myX    
    #Move from left side to Center 
    #esCTRL2[7] - 0, left side
    #esCTRL2[7] - 1, right side
    if (myX < 0x80):
      if(debug):
         print 'myX: 0x%02x Left to Center' % myX
      for x in range (myX,0,-1):
        ipallreg.regWrite(self.device+'::'+self.esCTRL2,x)
                  
      myX=0x80
      ipallreg.regWrite(self.device+'::'+self.esCTRL2,myX)

    if(debug):
      print 'myX: 0x%02x Walk to Right Edge' % myX           
    for x in range(myX, myX+65):
      ipallreg.regWrite(self.device+'::'+self.esCTRL2,x)      
      


  #------------------------------------------------------------------------------------   
  #Move Eye horizontal to center    
  def centerEye(self,debug=False):
    regVal=ipallreg.regRead(self.device+'::'+self.esCTRL2)
    myX = 0xff & regVal #Bit[7:0] direction, Horizontal Offset
      
    #Move from left side to Center 
    #esCTRL2[7] - 0, left side
    #esCTRL2[7] - 1, right side
    if (myX & 0x80):
      if(debug):
         print 'myX: 0x%02x Right to Center' % myX
      for x in range (myX,0x7f,-1):
        ipallreg.regWrite(self.device+'::'+self.esCTRL2,x)
          
    else:
      if(debug):
        print 'myX: 0x%02x Left to Center' % myX 
      for x in range(myX, 0,-1):
       
        ipallreg.regWrite(self.device+'::'+self.esCTRL2,x)    
        
      ipallreg.regWrite(self.device+'::'+self.esCTRL2,0x80)         
      
  #------------------------------------------------------------------
  
  def Do(self,event):     
    self.stop = 0
    self.startButton.Enable(False)
    self.stopButton.Enable(True)
    self.pps1.SetLabel('')
    self.pps2.SetLabel('')
    self.pps3.SetLabel('')
    self.pps4.SetLabel('')
    self.clearEye()
    #-----
    print '-'*70
    self.device = self.m_deviceBox.GetStringSelection()
    print "%s selected" %(self.device)
        
    self.rxn = self.m_checkBoxRx.GetSelection()
    print "Rx%d selected" %(self.rxn)


    #esEnable
    self.esCTRL1        = '30.' + str(391+(self.rxn*256))
    self.esCTRL2        = '30.' + str(392+(self.rxn*256))        
    self.esEnable       = '30.' + str(384+(self.rxn*256)+7) + '.0'
    self.esSenseOffset  = '30.' + str(384+(self.rxn*256)+7) + '.15:8'
    self.esVertOffset   = '30.' + str(384+(self.rxn*256)+8) + '.15:8'
    self.esHorizDir     = '30.' + str(384+(self.rxn*256)+8) + '.7'
    self.esHorizOffset  = '30.' + str(384+(self.rxn*256)+8) + '.6:0'
    self.esResult       = '30.' + str(384+(self.rxn*256)+9) + '.15:0'
    self.saAdpOffset6   = '30.' + str(437+(self.rxn*256)) + '.15:8'   # sense amp offset 6
    self.pi             = '30.' + str(424+(self.rxn*256)) + '.9:0'   # PI code
    self.ctle_state     = '30.' + str(421+(256*self.rxn)) + '.3:0'
    #-----    
    self.saOffset = ipallreg.regRead(self.device+'::'+self.saAdpOffset6)-128  # read the offset value        
    print "saOffset =",self.saOffset
    self.draw.InitBuffer(self.saOffset)
        
    # Do it. Set right/64 before entry
    ipallreg.regWrite(self.device+'::'+self.esHorizDir,1)      # set direction right
    ipallreg.regWrite(self.device+'::'+self.esHorizOffset,64)   
    ipallreg.regWrite(self.device+'::'+self.esEnable,1)    # enable Eye Scan mode
    self.offset = ipallreg.regRead(self.device+'::'+self.esSenseOffset)  # read the offset value

    self.gauge.SetValue(0)
    #if frame.legacy_eye_csv.IsChecked(): 
    #  print 'LEGACY'
    start_time = time.time()   
      
    #------
    if self.cbMode['Live Height'].Value: 


      #----- Live Height -------                 
      #for myX in range(64,0,-1):    # walk from right/64 to right/0
      #  ipallreg.regWrite(self.device+'::'+self.esHorizOffset,myX)
      self.cbMode['Full Scan'].Enable(False)        
      self.cbMode['Fast Diamond'].Enable(False)  
      self.cbMode['Full Scan'].Enable(False)    
      self.gauge.SetValue(64)
      self.centerEye()
      
          
      while True:
        wx.Yield()
        
        if self.stop:
          print "Stopping...."
          for myX in range(65):                                      # walk from center to right/0
           ipallreg.regWrite(self.device+'::'+self.esCTRL2,0x80|myX) #set direction right [7] = 1
          break           
        upper_vert = self.DoVerticalStepsF(range(128+self.saOffset,256),0x80)
        lower_vert = self.DoVerticalStepsF(range(127+self.saOffset,0,-1),0x80)	
        self.pps1.SetLabel('Upper: %d'%(upper_vert))  
        self.pps2.SetLabel('Lower: %d'%(lower_vert))        
        self.pps3.SetLabel('Height: ~%.0f mV'%((upper_vert-lower_vert)*1.85)) 
        self.draw.DrawLiveHeight(upper_vert,lower_vert,self.saOffset)          
        #print lower_vert,upper_vert         
          
    else: 
      #----- Full Scan or Fast Diamond -----
      self.cbMode['Live Height'].Enable(False)
      #----- Step from right to center to left edge ------- 
      #Right and walk to center         
      for myX in range(64,0,-1):    # walk from right/64 to right/0
        ipallreg.regWrite(self.device+'::'+self.esHorizOffset,myX)
          
      #----- step to the start of the Eye (left/64)
      ipallreg.regWrite(self.device+'::'+self.esHorizDir,0)      # set direction left
      for myX in range(0,64):  # move to the left edge of the eye
        ipallreg.regWrite(self.device+'::'+self.esHorizOffset,myX)
       
      #--------------------------------------------------------------       
      # Step through the Eye and do the vertical scan
      ipallreg.regWrite(self.device+'::'+self.esHorizDir,0)      # set direction left
      #----- Scan Left to Center (myX = 64->1)
      for myX in range(64,0,-1):
        if self.stop:
          #for my_x in range(myX,0,-1):                                      # walk from center to right/0
          #  ipallreg.regWrite(self.device+'::'+self.esCTRL2,myX) #set direction right [7] = 0
          print "Stopping....walk to center"
          self.centerEye()
          break
                            
        ipallreg.regWrite(self.device+'::'+self.esHorizOffset,myX)
        if self.cbMode['Full Scan'].Value:
          ##### Full Scan #########################################################
          self.DoVerticalStepsF(range(0,256),myX,debug=False)
        elif self.cbMode['Inner Eye'].Value:
          self.DoVerticalStepsF(range(128+self.saOffset,256),64-myX)
          self.DoVerticalStepsF(range(127+self.saOffset,0,-1),64-myX)	
        elif self.cbMode['Fast Diamond'].Value:
          self.DoVerticalStepsF(range(127+self.saOffset,128+self.saOffset+1),myX,debug=False)
        #self.gauge.SetValue(pos=self.gauge.GetValue() + 1)
        self.gauge.SetValue(64-myX)
        
      if self.cbMode['Fast Diamond'].Value:
        self.DoVerticalStepsF(range(128+self.saOffset,256),0x80,forceFull=True)
        self.DoVerticalStepsF(range(128+self.saOffset,0,-1),0x80,forceFull=True)
        
      #----- Scan Center -> Right (myX = 1,0 => 1,63)
      # *********************************************************************************
      #ipallreg.regWrite(self.device+'::'+self.esHorizDir,1)      # set direction right
      # GJL !!!!!!!! Need to stop and set direction before proceeding
      ipallreg.regWrite(self.device+'::'+self.esCTRL2,0x80) #set direction right [7] = 1
      #**********************************************************************************
      for myX in range(0,64):
        if self.stop:
          print "Stopping....walk to right"
          for x in range(myX,65):                                      # walk from center to right/0
            ipallreg.regWrite(self.device+'::'+self.esCTRL2,0x80|x) #set direction right [7] = 1
          self.gauge.SetValue(127)  
          #self.rightEdge()
          break
              
        ipallreg.regWrite(self.device+'::'+self.esHorizOffset,myX)
        if self.cbMode['Full Scan'].Value:
          ##### Full Scan #########################################################
         self.DoVerticalStepsF(range(0,256),myX|0x80,debug=False)
        elif self.cbMode['Inner Eye'].Value:
          self.DoVerticalSteps(range(128+self.saOffset,256),myX+64)
          self.DoVerticalSteps(range(128+self.saOffset,0,-1),myX+64)
        elif self.cbMode['Fast Diamond'].Value:
          #self.DoVerticalSteps(self,range(128+self.saOffset,128+self.saOffset+1),myX+64)
          self.DoVerticalStepsF(range(128+self.saOffset,128+self.saOffset+1),myX|0x80,debug=False)
        #self.gauge.SetValue(pos=self.gauge.GetValue() + 1)
        self.gauge.SetValue(myX+64)
        
        #print 'Offset: %s Min/Max Vertical: %s/%s (-%s/+%s)' %(self.offset,self.minVOffset,self.maxVOffset,self.offset-self.minVOffset,self.maxVOffset-self.offset)
    #FIXME ! - walk back! - you have!
    # Clean up at the end...
    ipallreg.regWrite(self.device+'::'+self.esHorizDir,1)      # set direction right        
    ipallreg.regWrite(self.device+'::'+self.esHorizOffset,64)  # set offset of 64
    ipallreg.regWrite(self.device+'::'+self.esEnable,0)        # Disable Eye Scan mode
    #Record Time
    scan_time = time.time() - start_time   

    # Find max H and V
    self.minV = 128+self.saOffset
    self.maxV = 128+self.saOffset
    self.minH = 64
    self.maxH = 64
        
    if not self.cbMode['Live Height'].Value:

      for findH in range(64,128):
        if self.eye[128+self.saOffset][findH] == 0:
          self.maxH = findH

      for findH in range(63,-1,-1):
        if self.eye[128+self.saOffset][findH] == 0:
          self.minH = findH
      for findV in range(128+self.saOffset,256):
        if self.eye[findV][64] == 0:
          self.maxV = findV
      for findV in range(128+self.saOffset,-1,-1):
        if self.eye[findV][64] == 0:
          self.minV = findV

      if self.cbMode['Fast Diamond'].Value:
        self.draw.DrawDiamond(self.minH,self.minV,self.maxH,self.maxV,self.saOffset)         

      if (not self.stop):    
        #self.controls.pps1 = wx.StaticText(self.panel,-1,'Horiz:'+str(self.minH)+'-'+str(self.maxH)+' (/128)',pos=(self.origX+128*3+10,self.origY))
        self.pps1.SetLabel('Horiz: %s - %s (/128)'%(self.minH,self.maxH))
      
        #self.controls.pps2 = wx.StaticText(self.panel,-1,valStrH,pos=(self.origX+128*3+10,self.origY+15))
        valStrH = '(%5.3f UI)' % ((self.maxH-self.minH)/128.0)
        self.pps2.SetLabel(valStrH)
      
        #self.controls.pps3 = wx.StaticText(self.panel,-1,'Vert: '+str(self.minV)+'-'+str(self.maxV)+' (/256)',pos=(self.origX+128*3+10,self.origY+30))
        self.pps3.SetLabel('Vert: %s - %s (/256)'%(self.minV,self.maxV))
  
        #self.controls.pps4 = wx.StaticText(self.panel,-1,valStrV,pos=(self.origX+128*3+10,self.origY+45))      
        valStrV = '(~%.0f mV)' % ((self.maxV-self.minV)*1.85)
        self.pps4.SetLabel(valStrV)
         
        print "min/max H",self.minH,self.maxH,valStrH,"min/max V",self.minV,self.maxV,valStrV
        
    print 'Time: %d'%scan_time    
    
    #----------------------------------------------------------------------
    #print Eye,cvs
    outputFilename = 'EyeF-Rx%d.csv'%self.rxn
    print "Saving RX lane= %d" %self.rxn
  
    try:
      fid       = open(outputFilename,'w')
      print "Eye data saved to: %s" %(outputFilename)
    except:
      print "Can't open %s - maybe it's currently open?" %(outputFilename)
 
    for y in range(0,256):
      line = '%d,'%(y)          
      for x in range(128):
        comma=','
        line = line + '%s%s'%(comma,self.eye[y][x])           
      line = line +'\n'
      fid.write(line)
            
      lastline=','

    for x in range (63,-1,-1):
      lastline = lastline+',%d'%(x)  
    for x in range (64):
      lastline = lastline+',%d'%(x)
    
      lastline1=','    
    for x in range (128):
      lastline1 = lastline1+',%d'% x
              
    lastline  = lastline+'\n'
    lastline1 = lastline1+'\n'
            
    fid.write(lastline)
    fid.write(lastline1)
    fid.write('Time,%d'%scan_time)
    print 'done'
    fid.close()  
    
    self.cbMode['Full Scan'].Enable(True)   
    self.cbMode['Fast Diamond'].Enable(True)    
    self.cbMode['Live Height'].Enable(True)
    self.startButton.Enable(True)
    self.stopButton.Enable(False)
  #--------------------------------------------------------------------
  def Stop(self,event):     
    self.stop = 1
    self.startButton.Enable(True)
    self.stopButton.Enable(False)




class EyeScan():
    def __init__(self,controls):
        self.input = []
        self.output = []
        self.eye = {}
        self.outputFilename = 'eye_out.csv'
        self.fid = -1
        self.rectSize=5
        self.origX = 30
        self.origY = 135
        self.panel = controls.panel
        self.rxn = -1
        self.txn = -1
        self.device = 'Undefined'
        self.offset = -1
        self.DEBUG = 0
        self.stop = 0
        self.controls = controls
        self.saOffset = 0
        self.UpdateFastDiamond = False
        self.PostPlotSummary = False
        #self.frame.Bind(wx.EVT_CHECKBOX,self.OnCheckBox)
        self.minV = 128
        self.maxV = 128
        self.minH = 64
        self.maxH = 64
        self.colour = True
        self.colourmap = []
        self.colourmap.append([255,255,255])

        #self.gauge = wx.Gauge(self.panel,-1,range=128,pos=(self.origX,self.origY+256+10),size=(128*3,8))
        #self.gauge.Show(False)

    def OnCheckBox(self,event):
      print "Eye checkbox"
    #------------------------------------------------------------------------------------

    #Move Eye Horizontal offset to left side    
    def leftEdge(self,debug=False):

      regVal=ipallreg.regRead(self.device+'::'+self.esCTRL2)
      myX = 0xff & regVal #Bit[7:0] direction, Horizontal Offset
      
      #Move from left side to Center 
      #esCTRL2[7] - 0, left side
      #esCTRL2[7] - 1, right side
      if (myX  & 0x80):
        if(debug):
           print 'myX: 0x%02x Right to Center' % myX
        for x in range (myX,-1,-1):
          ipallreg.regWrite(self.device+'::'+self.esCTRL2,x)
          
      
      for x in range(0, 64):
        if(debug):
           print 'myX: 0x%02x Walk to Left' % myX        
        ipallreg.regWrite(self.device+'::'+self.esCTRL2,x)  


   
    #------------------------------------------------------------------------------------    
    def Refresh(self):
      if self.controls.cbMode['Live Height'].Value:

        for eachY in range(0,256):
            self.eye[eachY] = {}
            for eachX in range(0,128):
                self.eye[eachY][eachX] = -1  
        
      self.controls.panel.Refresh()  
      self.controls.panel.Update()   
            
    def SetColour(self,value):
      if self.controls.eyeColour.GetValue():
        try:
          myColour = wx.Colour(self.colourmap[value][0],self.colourmap[value][1],self.colourmap[value][2])
        except:
          myColour = wx.Colour(255,255,255)
        return(myColour)
      else:
        myColour = int(value/10)+10
        return(wx.Colour(myColour, myColour, myColour))
        
    def DoCDR_Capture(self,count):
      for i in range(0,count):
        self.piCode       = '30.' + str(424+(self.rxn*256)) + '.7:0'
        self.piQuadrant   = '30.' + str(424+(self.rxn*256)) + '.9:8'

        piCode = ipallreg.regRead(self.device+'::'+self.piCode)
        piQuad = ipallreg.regRead(self.device+'::'+self.piQuadrant)
        print "PI",piCode,piQuad

    def saveToBmp(self):
        pass

          
