#11/6 - Individual file naming
#11/6 - Increase EyeScan Speed
#11/7 - Add legacy csv option
#11/13 - Added wx.Yield() in DoVeriticalSteps/F to keep gui from freezing during EyeScans
import wx
import ipallreg
import ipvar
import time

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
        # create the colourmap for the eye scans...
        red = 255
        green = 255
        blue = 255
        for i in range(1,2500):
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
          #print i,red,green,blue
          self.colourmap.append([red,green,blue])   
        self.gauge = wx.Gauge(self.panel,-1,range=128,pos=(self.origX,self.origY+256+10),size=(128*3,8))
        self.gauge.Show(False)

    def OnCheckBox(self,event):
      print "Eye checkbox"
    #------------------------------------------------------------------------------------
    def DoVerticalSteps(self,controls,myRange,myX,forceFull=False,refreshWindow=True):
      lasterror = 0
      self.maxVOffset = self.offset
      self.minVOffset = self.offset
      pi = ipallreg.regRead(self.device+'::'+self.pi)
      quad = (pi & 0b1100000000)/256
      pi   = pi & 0b0011111111
      pi_string = str(pi) + ',' + str(quad)
      for vertical in myRange:
        try:
          wx.Yield()
        except:
          pass
        if self.stop:
          print "STOP"
          break
        if lasterror == 0 or controls.cbMode['Full Scan'].Value or forceFull:
          self.maxVOffset = vertical
          ipallreg.regWrite(self.device+'::'+self.esVertOffset,vertical)     
          lasterror = ipallreg.regRead(self.device+'::'+self.esResult)
  
          self.eye[vertical][myX] = lasterror
          self.fid.write('%s,%s,%s,%s,%s,%s\n' %(myX,vertical,vertical-(self.saOffset+128), lasterror,pi_string,ipallreg.regRead(self.device+'::'+self.ctle_state)))
          #print '%s,%s,%s' %(myX,vertical,lasterror)
        else:
          controls.panel.Refresh()
          return(vertical)
      if refreshWindow:
        controls.panel.Refresh()  
        controls.panel.Update()                     
      #return(vertical)
    #------------------------------------------------------------------------------------
    def DoVerticalStepsF(self,controls,myRange,myX,forceFull=False,refreshWindow=True,debug=False):
      #global pi,esVertOffset, esResult,ctle_state,saOffset
      lasterror = 0
      #maxVOffset = offset
      #minVOffset = offset
      piCode = ipallreg.regRead(self.device+'::'+self.pi)
      quad = (piCode & 0b1100000000)/256
      piCode   = piCode & 0b0011111111
      pi_string = str(piCode) + ',' + str(quad)
      try:
        wx.Yield()
      except:
        pass
      for vertical in myRange:
        if lasterror == 0 or controls.cbMode['Full Scan'].Value or forceFull:
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
 
          if (controls.legacyCSV.IsChecked()):
            self.fid.write('%s,%s,%s,%s,%s,%s\n' %(x,vertical,vertical-(self.saOffset+128), lasterror,pi_string,ipallreg.regRead(self.device+'::'+self.ctle_state)))
          
          if(debug):
            if((vertical == 0)&(x%8 == 0)): 
              es_ctrl1 = ipallreg.regRead(self.device+'::'+self.esCTRL1)
              es_ctrl2 = ipallreg.regRead(self.device+'::'+self.esCTRL2)
              print '%d: %d,%d,(0x%04x,0x%04x) - %d' %(myX,x,vertical,es_ctrl1,es_ctrl2,lasterror)   
        else:
          controls.panel.Refresh()
          return(vertical)
          
      if refreshWindow:
        if ((myX % 4)==0):
          controls.panel.Refresh()  
          controls.panel.Update()                     

    #------------------------------------------------------------------------------------   
    #Returns Eye Horizontal offset to right Side    
    def resetEye(self,controls,debug=False):

      regVal=ipallreg.regRead(self.device+'::'+self.esCTRL2)
      myX = 0xff & regVal #Bit[7:0] direction, Horizontal Offset
      
      if (myX < 0x80):
        if(debug):
           print 'myX: 0x%02x Walk to Center' % myX
        for x in range (myX,0,-1):
          ipallreg.regWrite(self.device+'::'+self.esCTRL2,x)
        myX=0x80
      
      for x in range(myX, 64):
        if(debug):
           print 'myX: 0x%02x Walk to Right' % myX        
        ipallreg.regWrite(self.device+'::'+self.esCTRL2,x)      
      
      ipallreg.regWrite(self.device+'::'+self.esCTRL2,64)      
      



   
    #------------------------------------------------------------------------------------    
    def OnPaintSweep(self,event):
        dc = wx.PaintDC(self.panel)
        font = wx.Font(8, wx.MODERN, wx.NORMAL, wx.NORMAL)
        dc.SetFont(font)
        #dc.Clear()
        #dc.BeginDrawing()
        dc.SetPen(wx.Pen("GRAY",1))
        dc.DrawRectangle(self.origX,self.origY,128*3,256)
        for x in range(0,129,8):
          dc.DrawLine(self.origX+x*3,self.origY,self.origX+x*3,self.origY+256)
        for y in range(0,256,16):
          dc.DrawLine(self.origX,self.origY+256-y,self.origX+128*3,self.origY+256-y)
        dc.SetPen(wx.Pen("BLACK",1))            
        dc.DrawLine(self.origX,self.origY+256-(128+self.saOffset),self.origX+128*3,self.origY+256-(128+self.saOffset))
        dc.DrawLine(self.origX+64*3+1,self.origY+0,self.origX+64*3+1,self.origY+256)

        if True: #self.controls.cbMode['Full Scan'].Value:
            for eachY in range(0,256):
                for eachX in range(1,128):
                    if self.eye[eachY][eachX] < 1:
                        pass
                    else:
                        if self.eye[eachY][eachX] > 2449:
                            pass
                            #dc.SetPen(wx.Pen(wx.Colour(255, 255, 255),1))
                        else:
                          if self.controls.cbMode['Live Height'].Value:
                            dc.SetPen(wx.Pen(self.SetColour(self.eye[eachY][eachX]),2))
                            dc.DrawLine(self.origX+eachX*3-20,self.origY+256-eachY,self.origX+eachX*3+26,self.origY+256-eachY)                        
                          
                          else:
                            dc.SetPen(wx.Pen(self.SetColour(self.eye[eachY][eachX]),1))
                            dc.DrawLine(self.origX+eachX*3,self.origY+256-eachY,self.origX+eachX*3+3,self.origY+256-eachY)                        
                            #print "Drew",eachX*3,eachY  
        if self.UpdateFastDiamond and self.controls.cbMode['Fast Diamond'].Value:
          myPen = wx.Pen("BLUE",1)
          myPen.SetWidth(3)
          dc.SetPen(myPen)
          dc.DrawLine(self.origX+self.minH*3,self.origY+256-(128+self.saOffset),self.origX+64*3+2,self.origY+256-self.maxV)
          dc.DrawLine(self.origX+64*3+2,self.origY+256-self.maxV,self.origX+self.maxH*3,self.origY+256-(128+self.saOffset))
          dc.DrawLine(self.origX+self.maxH*3,self.origY+256-(128+self.saOffset),self.origX+64*3+2,self.origY+256-self.minV)
          dc.DrawLine(self.origX+64*3+2,self.origY+256-self.minV,self.origX+self.minH*3,self.origY+256-(128+self.saOffset))
            
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
    def Do(self):     
        self.controls.stop = 0
        self.controls.goButton.Enable(False)
        self.controls.stopButton.Enable(True)
        self.controls.refreshButton.Enable(True)

        self.UpdateFastDiamond = False
        self.PostPlotSummary = False
        try:
            self.controls.pps1.Destroy()
            self.controls.pps2.Destroy()
            self.controls.pps3.Destroy()
            self.controls.pps4.Destroy()
            self.controls.panel.Refresh()  
            self.controls.panel.Update() 
        except:
            pass
        
        self.DoCDR_Capture(10)
        try:
            self.fid       = open(self.outputFilename,'w')
            print "Eye data saved to: %s" %(self.outputFilename)
        except:
            print "Can't open %s - maybe it's currently open?" %(self.outputFilename)
            self.controls.goButton.Enable(True)
            self.controls.stopButton.Enable(False)
            dlg = wx.MessageDialog(self.controls.panel, "Can't open %s! - maybe it's currently open?" %(self.outputFilename), "Warning!", wx.OK | wx.ICON_WARNING)
            dlg.ShowModal()
            dlg.Destroy()

            return
        
        self.controls.panel.Bind(wx.EVT_PAINT,self.OnPaintSweep)
        # Build the space and empty results
        for eachY in range(0,256):
            self.eye[eachY] = {}
            for eachX in range(0,128):
                self.eye[eachY][eachX] = -1  
                        
        self.controls.panel.Refresh()  
        self.controls.panel.Update()        
        #headerStr = 'voltageOffset,phaseOffset,errors'
        #self.fid.write(headerStr+'\n')

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
        
        self.saOffset = ipallreg.regRead(self.device+'::'+self.saAdpOffset6)-128  # read the offset value        
        print "saOffset =",self.saOffset
        
        # Do it. Set right/64 before entry
        ipallreg.regWrite(self.device+'::'+self.esHorizDir,1)      # set direction right
        ipallreg.regWrite(self.device+'::'+self.esHorizOffset,64)   
        ipallreg.regWrite(self.device+'::'+self.esEnable,1)    # enable Eye Scan mode
        offset = ipallreg.regRead(self.device+'::'+self.esSenseOffset)  # read the offset value

        self.offset = offset
        self.gauge.SetValue(0)
        self.gauge.Show(True)
        #if frame.legacy_eye_csv.IsChecked(): 
        #  print 'LEGACY'
        start_time = time.time()
        if self.controls.cbMode['Live Height'].Value: 
          #----- Live Height -------                 
          for myX in range(64,0,-1):    # walk from right/64 to right/0
            ipallreg.regWrite(self.device+'::'+self.esHorizOffset,myX)
          
          while True:
            wx.Yield()
            if self.stop:
              print "Stopping...."
              for myX in range(64):                                      # walk from center to right/0
                ipallreg.regWrite(self.device+'::'+self.esCTRL2,0x80|myX) #set direction right [7] = 1
              break           
            #upper_vert = self.DoVerticalSteps(self.controls,range(128+self.saOffset,256),64-myX)
            #lower_vert = self.DoVerticalSteps(self.controls,range(127+self.saOffset,0,-1),64-myX)	   
            upper_vert = self.DoVerticalStepsF(self.controls,range(128+self.saOffset,256),0x80)
            lower_vert = self.DoVerticalStepsF(self.controls,range(127+self.saOffset,0,-1),0x80)	                  
            #print lower_vert,upper_vert         
          
        else:          
          #----- Step from right to center to left edge ------- 
          #Right and walk to center         
          for myX in range(64,0,-1):    # walk from right/64 to right/0
            ipallreg.regWrite(self.device+'::'+self.esHorizOffset,myX)
          
          # step to the start of the Eye (left/64)
          ipallreg.regWrite(self.device+'::'+self.esHorizDir,0)      # set direction left
          for myX in range(0,64):  # move to the left edge of the eye
            ipallreg.regWrite(self.device+'::'+self.esHorizOffset,myX)
                 
          # Step through the Eye and do the vertical scan
          ipallreg.regWrite(self.device+'::'+self.esHorizDir,0)      # set direction lef
          for myX in range(64,0,-1):
            if self.stop:
              for my_x in range(myX,0,-1):                                      # walk from center to right/0
                ipallreg.regWrite(self.device+'::'+self.esCTRL2,myX) #set direction right [7] = 0
              print "Stopping....walk to center"
              break
              
              
            ipallreg.regWrite(self.device+'::'+self.esHorizOffset,myX)
            if self.controls.cbMode['Full Scan'].Value:
              ##### Full Scan #########################################################
              #self.DoVerticalSteps(self.controls,range(0,256),64-myX,debug=True)
              self.DoVerticalStepsF(self.controls,range(0,256),myX,debug=False)
            elif self.controls.cbMode['Inner Eye'].Value:
              self.DoVerticalSteps(self.controls,range(128+self.saOffset,256),64-myX)
              self.DoVerticalSteps(self.controls,range(127+self.saOffset,0,-1),64-myX)	
            elif self.controls.cbMode['Fast Diamond'].Value:
              #self.DoVerticalSteps(self.controls,range(127+self.saOffset,128+self.saOffset+1),64-myX) #x= 0=>63
              self.DoVerticalStepsF(self.controls,range(127+self.saOffset,128+self.saOffset+1),myX,debug=True)
            self.gauge.SetValue(pos=self.gauge.GetValue() + 1)
            
            
          #ipallreg.regWrite(self.device+'::'+self.esHorizDir,1)      # set direction right
          # GJL !!!!!!!! Need to stop and set direction before proceeding
          ipallreg.regWrite(self.device+'::'+self.esCTRL2,0x80) #set direction right [7] = 1            
          if self.controls.cbMode['Fast Diamond'].Value:
              #self.DoVerticalStepsF(self.controls,range(128+self.saOffset,256),64,forceFull=True)
              #self.DoVerticalStepsF(self.controls,range(128+self.saOffset,0,-1),64,forceFull=True)
              self.DoVerticalStepsF(self.controls,range(128+self.saOffset,256),0x80,forceFull=True)
              self.DoVerticalStepsF(self.controls,range(128+self.saOffset,0,-1),0x80,forceFull=True)
                

          for myX in range(0,64):
            if self.stop:
              for myX in range(64):                                      # walk from center to right/0
                ipallreg.regWrite(self.device+'::'+self.esCTRL2,0x80|myX) #set direction right [7] = 1
              print "Stopping....walk to right"
              break
              
            ipallreg.regWrite(self.device+'::'+self.esHorizOffset,myX)
            if self.controls.cbMode['Full Scan'].Value:
              ##### Full Scan #########################################################
              #self.DoVerticalSteps(self.controls,range(0,256),myX+64,debug=True)  #x=64-> 128
              self.DoVerticalStepsF(self.controls,range(0,256),myX|0x80,debug=False)
            elif self.controls.cbMode['Inner Eye'].Value:
              self.DoVerticalSteps(self.controls,range(128+self.saOffset,256),myX+64)
              self.DoVerticalSteps(self.controls,range(128+self.saOffset,0,-1),myX+64)
            elif self.controls.cbMode['Fast Diamond'].Value:
              #self.DoVerticalSteps(self.controls,range(128+self.saOffset,128+self.saOffset+1),myX+64)
              self.DoVerticalStepsF(self.controls,range(128+self.saOffset,128+self.saOffset+1),myX|0x80,debug=True)
            self.gauge.SetValue(pos=self.gauge.GetValue() + 1)

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
        
        if not self.controls.cbMode['Live Height'].Value:
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

          self.controls.pps1 = wx.StaticText(self.panel,-1,'Horiz:'+str(self.minH)+'-'+str(self.maxH)+' (/128)',pos=(self.origX+128*3+10,self.origY))
          valStrH = '(%5.3f UI)' % ((self.maxH-self.minH)/128.0)
          self.controls.pps2 = wx.StaticText(self.panel,-1,valStrH,pos=(self.origX+128*3+10,self.origY+15))
          self.controls.pps3 = wx.StaticText(self.panel,-1,'Vert: '+str(self.minV)+'-'+str(self.maxV)+' (/256)',pos=(self.origX+128*3+10,self.origY+30))
          valStrV = '(~%.0f mV)' % ((self.maxV-self.minV)*1.85)
          self.controls.pps4 = wx.StaticText(self.panel,-1,valStrV,pos=(self.origX+128*3+10,self.origY+45))
                  
          print "min/max H",self.minH,self.maxH,valStrH,"min/max V",self.minV,self.maxV,valStrV
          print 'Time: %d'%scan_time
          self.UpdateFastDiamond = True
          self.PostPlotSummary = True
        self.controls.panel.Refresh()  
        self.controls.panel.Update()                     

        self.fid.close()
        
        
        #----------------------------------------------------------------------
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
        
        self.controls.goButton.Enable(True)
        self.controls.stopButton.Enable(False)
    #------------------------------------------------------------------------------------
    def DoVerticalSteps(self,controls,myRange,myX,forceFull=False,refreshWindow=True):
      lasterror = 0
      self.maxVOffset = self.offset
      self.minVOffset = self.offset
      pi = ipallreg.regRead(self.device+'::'+self.pi)
      quad = (pi & 0b1100000000)/256
      pi   = pi & 0b0011111111
      pi_string = str(pi) + ',' + str(quad)
      for vertical in myRange:
        try:
          wx.Yield()
        except:
          pass
        if self.stop:
          print "STOP"
          break
        if lasterror == 0 or controls.cbMode['Full Scan'].Value or forceFull:
          self.maxVOffset = vertical
          ipallreg.regWrite(self.device+'::'+self.esVertOffset,vertical)
         
          lasterror = ipallreg.regRead(self.device+'::'+self.esResult)
  
          self.eye[vertical][myX] = lasterror
          self.fid.write('%s,%s,%s,%s,%s,%s\n' %(myX,vertical,vertical-(self.saOffset+128), lasterror,pi_string,ipallreg.regRead(self.device+'::'+self.ctle_state)))
          #print '%s,%s,%s' %(myX,vertical,lasterror)
        else:
          controls.panel.Refresh()
          return(vertical)
      if refreshWindow:
        controls.panel.Refresh()  
        #controls.panel.Update()                     
      #return(vertical)
      
    def populate(self):
        # go through the checkboxes and find the Rx on
        for checkboxRx in self.controls.checkboxRx:
          self.rxn=self.rxn+1
          if checkboxRx.GetValue():
            break
        print "Rx%d selected" %(self.rxn)
        
        i = -1
        for checkboxDevice in self.controls.checkboxDevice:
          i=i+1
          if checkboxDevice.GetValue():
            self.device = self.controls.checkboxDevice[i].GetLabel()
        print "%s selected" %(self.device)
        
          
