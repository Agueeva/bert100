#10/3 - Individual file naming
#11/1 - Increase EyeScan Speed
#11/13 - Added wx.Yield() in DoVeriticalSteps/F to keep gui from freezing during EyeScans

import ipallreg
#from ipallreg import regWrite,regRead
from membus import memRead,memWrite
import ipvar

import wx
import time

class CauiEyeScan():
#--------------------------------------------------------------------
    def DoVerticalStepsF(self,controls,myRange,myX,refreshWindow=True,debug=False):
      lasterror = 0
      try:
        wx.Yield()
      except:
        pass
      for vertical in myRange:
        valymap = range(63,31,-1)+range(1,32)
        valy = valymap[vertical]
        memWrite('0xf',self.rxn,valy,self.device)    
        memWrite('0x25',self.rxn,0x28,self.device)
        memWrite('0x25',self.rxn,0x8,self.device) 
        lasterror = memRead('0x28',self.rxn,self.device)*2**8+memRead('0x29',self.rxn,self.device)
        self.eye[vertical][myX] = lasterror   
        if(debug &(myX%4==0)&(vertical==32)): 
          print '(%2d,%2d(%d)): %d' %(myX,vertical,valy,lasterror)
      if refreshWindow:
        controls.panel.Refresh()  
        controls.panel.Update()          
#--------------------------------------------------------------------
    def DoVerticalSteps(self,controls,myRange,myX,forceFull=False,refreshWindow=True,debug=False):
      lasterror = 0
      for vertical in myRange:
        valymap = range(63,31,-1)+range(1,32)
        valy = valymap[vertical]
        wx.Yield()
        if self.stop:
          print "STOP"
          break
        if lasterror == 0 or controls.cbMode['Full Scan'].Value or forceFull:
          memWrite('0xf[5:0]',self.rxn,valy,self.device)
          memWrite('0x25[5]',self.rxn,0x1,self.device)
          memWrite('0x25[5]',self.rxn,0x0,self.device)
          lasterror = memRead('0x28',self.rxn,self.device)*2**8+memRead('0x29',self.rxn,self.device)
          
          #ipallreg.regWrite(self.device+'::'+self.esVertOffset,vertical)
          #lasterror = ipallreg.regRead(self.device+'::'+self.esResult)
  
          self.eye[vertical][myX] = lasterror
          self.fid.write('%s,%s,%s,%s,%d\n' %(myX,vertical,vertical, lasterror,-1))
          if(debug): print '(%d,%d(%d)): %d' %(myX,vertical,valy,lasterror)
              
      if refreshWindow:
        controls.panel.Refresh()  
        controls.panel.Update()                     
            
#--------------------------------------------------------------------

    def startup_rx(self):

        #device = 'GB0::'
        cLane = self.rxn
        self.outputFilename2 = 'cauieye-Rx%d.csv'%cLane
        print "current RX lane= %s: %d" %(self.device,self.rxn)

   
        print "Running eye-scan..."
        # Roam Eye
        # Overwrite
      
        #Roam Adjust Enable
        memWrite('0xd[0]',cLane,0,self.device)
        memWrite('0xd[1]',cLane,0,self.device)
        #Roam Eye Enable
        memWrite('0x25[6]',cLane,0,self.device)

        #XORBITSEL - Select Bit Countour Mode
        memWrite('0xd[3]',cLane,1,self.device)
        
        #Common Registers
        if cLane < 3:
          common = 'common0'
        elif cLane < 7:
          common = 'common1'
        else:
          common = 'common2'
        ################increase RXCALROAMEYEMEAS_COUNT (RXCALROAMEYEMEAS_COUNT * bitwidth < 2**16-1)####################
        memWrite('0x1bf[2:0]',common,0,self.device)
        #print "RXCALROAMEYEMEAS_COUNT_0x1bf[2:0]:",memRead('0x1bf[2:0]',common)
        memWrite('0x1be[7:0]',common,0xf4,self.device)
        #print "RXCALROAMEYEMEAS_COUNT_0x1be[7:0]:",memRead('0x1be[7:0]',common)
        memWrite('0x1bd[7:3]',common,0x1,self.device)
        #print "RXCALROAMEYEMEAS_COUNT_0x1bd[7:3]:",memRead('0x1bd[7:3]',common)
        #print "RXCALROAMEYEMEAS_COUNT:",memRead('0x1bd[7:3]',common)*2**11+memRead('0x1be[7:0]',common)*2**3+memRead('0x1bf[2:0]',common)

        #422[7:6],423[7:0],424[5:0]	RXCALEYEDIAGFSM_BERTHRESHOLD	4  0x400
        memWrite('0x1a6[7:6]',common,0x0,self.device)
        memWrite('0x1a7[7:0]',common,0x10,self.device)
        memWrite('0x1a8[5:0]',common,0x0,self.device)

        #print "DONE"  

    def __init__(self,controls):
        self.input = []
        self.output = []
        self.eye = {}
        #self.outputFilename = 'cauieye_out.csv'

        self.fid = -1
        self.fid2 = -1
        self.rectSize=5
        self.origX = 30
        self.origY = 135
        self.panel = controls.panel
        self.rxn = -1
        self.txn = -1
        self.device = 'Undefined'
        self.offset = -1
        self.debug = 0
        self.stop = 0
        self.controls = controls
        self.saOffset = 0
        self.UpdateFastDiamond = False
        self.PostPlotSummary = False
        #self.frame.Bind(wx.EVT_CHECKBOX,self.OnCheckBox)
        self.minV = 32
        self.maxV = 32
        self.minH = 64
        self.maxH = 64
        self.UI = 0
        self.myX_left = 0
        self.myX_right = 0
        self.height = 0
        self.update = 0
        self.colour = True
        self.colourmap = []
        self.colourmap.append([255,255,255])
        # create the colourmap for the eye scans...
        red = 255
        green = 255
        blue = 255
        # for i in range(1,2500):
          # if i < 201:
            # red = 100+int(i*0.75)
            # green = int(i)
            # blue = 0
          # elif i<501:
            # red = 250-int((i-200)/1.2)
            # green = 255-int((i-200)/1.2)
            # blue = int((i-200)/2)
          # else:
            # red = int((i-500)/8)
            # green = int((i-500)/8)
            # blue = int((i-500)/20)+150
          #print i,red,green,blue
          # self.colourmap.append([red,green,blue])  
        self.maxError = 2500
        for i in range(1,self.maxError):
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
            blue = int((i-500)/40)+150
          #print i,red,green,blue
          self.colourmap.append([red,green,blue])  
          
        self.caui_gauge = wx.Gauge(self.panel,-1,range=31,pos=(self.origX,self.origY+256+10),size=(128*2,8))
        self.caui_gauge.Show(False)        

    def OnCheckBox(self,event):
      print "Eye checkbox"
      
    def OnPaintSweep(self,event,debug=False):
        scaleY=4
        scaleX=8
        #if(debug): print 'PaintSweep'
        dc = wx.PaintDC(self.panel)
        font = wx.Font(8, wx.MODERN, wx.NORMAL, wx.NORMAL)
        dc.SetFont(font)
        #dc.Clear()
        #dc.BeginDrawing()
        self.DrawChart(self,self.origX,self.origY)
        

        if True: #self.controls.cbMode['Full Scan'].Value:
            for eachY in range(0,64):
                for eachX in range(0, 32):
                    error_cnt = self.eye[eachY][self.myX_left+eachX]
                    if (error_cnt < 1):
                        pass
                    else:
                        if self.eye[eachY][self.myX_left+eachX] > 25000:
                            pass
                            #dc.SetPen(wx.Pen(wx.Colour(255, 255, 255),1))
                        else:
                            dc.SetPen(wx.Pen(self.SetColour(self.eye[eachY][self.myX_left+eachX]),1))
                            dc.SetBrush(wx.Brush(self.SetColour(self.eye[eachY][self.myX_left+eachX]), wx.SOLID))
                            #dc.DrawLine(self.origX+eachX*3,self.origY+256-eachY,self.origX+eachX*3+3,self.origY+256-eachY)
                            if (self.controls.cbMode['Full Scan'].Value):                            
                              dc.DrawRectangle(self.origX+eachX*scaleX,self.origY+(256-eachY*scaleY),scaleX,scaleY)
                            else :
                              dc.DrawRectangle(self.origX+eachX*scaleX,self.origY+(256-eachY*4),2,2)
                            #print "Drew",eachX*3,eachY  

        if self.UpdateFastDiamond and self.controls.cbMode['Fast Diamond'].Value:
          self.UpdateFastDiamond = False
          self.update = self.update + 1
          if (debug): print "Update Fast Diamond: %d" % self.update
          myPen = wx.Pen("BLUE",1)
          #myPen.SetWidth(2)
          dc.SetPen(myPen)
          myBrush = wx.Brush("WHITE",wx.TRANSPARENT)
          dc.SetBrush(myBrush)
          if(debug):
            print 'Min/MaxH: %d-%d' % (self.minH,self.maxH)
            print 'Min/MaxV: %d-%d' % (self.minV,self.maxV)
            dc.SetPen(wx.Pen("Blue",1)) 
            dc.DrawPolygon([(center-64,128),(center, 256),
                            (center+64,128),(center, 0)],
                             self.origX,self.origY)
            
          center = (self.minH+self.maxH)*4/2 
          dc.DrawPolygon([(self.minH*4,128),(center, 256-self.maxV*4),
                          (self.maxH*4,128),(center, 256-self.minV*4)],
                           self.origX,self.origY)
       
                           
          dc.SetPen(wx.Pen("Black",1)) 
          dc.DrawRectangle(center+self.origX, self.origY+128,2,2)
          
        if(debug):                   
          line = 'Horiz: (%d-%d) UI: %.3f' %(self.minH,self.maxH,self.UI)
          #self.controls.hztxt = wx.StaticText(self.panel,-1,line,pos=(self.origX,self.origY+315))
          dc.DrawText(line, self.origX+5,self.origY+285)
          line = 'Vert: (%d-%d) : ~%d mV' %(self.minV,self.maxV,self.height)
          #self.controls.vrttxt = wx.StaticText(self.panel,-1,line,pos=(self.origX,self.origY+330))
          dc.DrawText(line, self.origX+5,self.origY+300)

                           

     
    def DrawChart(self,event,origX,origY,debug=False):
      if (debug): print 'DrawChart'
      dc = wx.PaintDC(self.panel)
      font = wx.Font(8, wx.MODERN, wx.NORMAL, wx.NORMAL)
      dc.SetFont(font)
      maxX = 64
      maxY = 63
      #dc.Clear()
      #dc.BeginDrawing()
      dc.SetPen(wx.Pen("GRAY",1))
     
      dc.SetBrush(wx.Brush("WHITE",style=wx.SOLID))
      
      #Draw 256 x 256 Diagram
      dc.DrawRectangle(origX-1,origY,259,256)
      
      #Draw Veritcal/Horizontal gridlines
      for i in range(0,256,16):
        dc.DrawLines([(i,0),(i,256)],origX,origY)
        dc.DrawLines([(0,i),(256,i)],origX,origY)

 
          
      #Draw Center axis    
      dc.SetPen(wx.Pen("BLACK",1))      
      dc.DrawLines([(0,127),(256,127)],origX,origY)
      dc.DrawLines([(127,0),(127,256)],origX,origY)
      
      
      if(debug):
        dc.SetPen(wx.Pen("Blue",1)) 
        dc.SetBrush(wx.Brush("WHITE",wx.TRANSPARENT))
        dc.DrawRectangle(origX,origY,3,3)
        dc.DrawPolygon([(0,maxY*4/2+2),(maxX*4/2,maxY*4+2),
                        (maxX*4,maxY*4/2+2),(maxX*4/2,0+2)],
                         origX,origY)

        
    def Refresh(self):
      self.controls.panel.Refresh()  
      self.controls.panel.Update()   
            
    def SetColour(self,value):
      if self.controls.eyeColour.GetValue():
        try:
          myColour = wx.Colour(self.colourmap[value/10][0],self.colourmap[value/10][1],self.colourmap[value/10][2])
        except:
          myColour = wx.Colour(255,255,255)
        return(myColour)
      else:
        myColour = int(value/10)+10
        return(wx.Colour(myColour, myColour, myColour))

    def ClearEye(self):
      for eachY in range(0,64):
        self.eye[eachY] = {}
        for eachX in range(0,64):
          self.eye[eachY][eachX] = -1      
        
    def Do(self,debug=False):   
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
        
        
        try:
            pass
            #self.fid       = open(self.outputFilename,'w')
            #print "Eye data saved to: %s" %(self.outputFilename)
        except:
            print "Can't open %s - maybe it's currently open?" %(self.outputFilename)
            self.controls.goButton.Enable(True)
            self.controls.stopButton.Enable(False)
            dlg = wx.MessageDialog(self.controls.panel, "Can't open %s! - maybe it's currently open?" %(self.outputFilename), "Warning!", wx.OK | wx.ICON_WARNING)
            dlg.ShowModal()
            dlg.Destroy()
        
        try:
            self.fid2       = open(self.outputFilename2,'w')
            print "Eye data saved to: %s" %(self.outputFilename2)
        except:
            print "Can't open %s - maybe it's currently open?" %(self.outputFilename2)

            return
        
        self.controls.panel.Bind(wx.EVT_PAINT,self.OnPaintSweep)
        
        
        # Build the space and empty results
        self.ClearEye()
                
        self.controls.panel.Refresh()  
        self.controls.panel.Update()        
        #headerStr = 'voltageOffset,phaseOffset,errors'
        #self.fid.write(headerStr+'\n')


        self.caui_gauge.SetValue(0)
        self.caui_gauge.Show(True)
        start_time = time.time()
        if True:                   
          # Find X-Openning
          left = 0
          right = 64
          self.offset=0
          y0 = 31 # 31 = center (0)
          y1 = 32
          for myX in range (64):
            #RXCALROAMXADJUST(0xe[6:0]) 
            memWrite('0xe[6:0]',self.rxn,myX,self.device)
            #self.DoVerticalSteps(self.controls,range(y0, y1),myX,refreshWindow=False,forceFull=True,debug=False) 
            #self.DoVerticalSteps(self.controls,range(y0, y1),myX,refreshWindow=False,debug=False)           
            self.DoVerticalStepsF(self.controls,range(y0, y1),myX,refreshWindow=False,debug=False)    
            width = 0
          centerY = 0
          for i in range(y0,y1):
            #print '%d: %d'%(i,self.eye[i].values().count(0))
            if self.eye[i].values().count(0) > width:
              centerY = i
              width = self.eye[i].values().count(0)
              
          x_line0 = 'Center,Width - (%d: %d)' % (centerY,width)
          
         
         
         
          for myX in range(64):
            value = self.eye[centerY][myX]
            if(value == 0): 
              left = myX
              break
          
          for myX in range(63,-1,-1):
            value = self.eye[centerY][myX]
            if(value == 0): 
              right = myX
              break              
          
          center = (left+right)/2
       
          self.minH = left
          self.maxH = right
          self.UI = (float(right-left+1)/32)
          self.offset = 32-center
          x_line = 'Left: %d, Right: %d, Center: %d, Offset: %d, UI: %.3f' % (left,right, center, self.offset ,self.UI)  
          
          #----------------------------------------------
          # Step through the Eye and do the vertical scan
          
          #for myX in range(0,64):
          if self.controls.cbMode['Full Scan'].Value:
             self.myX_left = center - 16
             self.myX_right = center + 16
             self.ClearEye()
          elif self.controls.cbMode['Fast Diamond'].Value:
            self.myX_left = center - 2
            self.myX_right = center + 2
          
          for myX in range(self.myX_left,self.myX_right):
            if self.stop:
              print "Stopping...."
              break
              
              
            #RXCALROAMXADJUST(0xe[6:0]) 
            memWrite('0xe[6:0]',self.rxn,myX,self.device)

            if self.controls.cbMode['Full Scan'].Value:
              #self.DoVerticalSteps(self.controls,range(24,40),myX)
              #self.DoVerticalSteps(self.controls,range(0,63),myX)
              self.DoVerticalStepsF(self.controls,range(0,63),myX)
              
            elif self.controls.cbMode['Fast Diamond'].Value:
              #self.DoVerticalSteps(self.controls,range(127+self.saOffset,128+self.saOffset+1),64-myX,offset)
              self.DoVerticalStepsF(self.controls,range(127+self.saOffset,128+self.saOffset+1),64-myX,offset)
            #print 'Gauge: %d' %self.caui_gauge.GetValue()
            self.caui_gauge.SetValue(pos=self.caui_gauge.GetValue() + 1)
          
          #print Eye,cvs
          #self.eye[vertical][myX+offset] = lasterror
          
          scan_time = time.time() - start_time
          for y in range(62, -1, -1):
            line = '%d,%d,'%(y,y-31)
            
            for x in range (64):
              comma=','
              line = line + '%s%s'%(comma,self.eye[y][x])
              
            line = line +'\n'
            self.fid2.write(line)
            
            lastline=',,'
            lastline1=',,'
            for x in range (64):
              lastline = lastline+',%d'%(x-31)
              lastline1 = lastline1+',%d'% x
              
            lastline  = lastline+'\n'
            lastline1 = lastline1+'\n'
              
          self.fid2.write(lastline)
          self.fid2.write(lastline1)
          

          
          
          #if self.controls.cbMode['Fast Diamond'].Value:
          #Find Top
          for y in range(63,-1, -1):
            if(debug): print 'y:%d = %d  '% (y,self.eye[y][32])   
            top = y
            if (self.eye[y][center] == 0) :  
              if(debug): print 'Top: %d'% top    
              self.maxV = top
              break
          #Find Bottom
          for y in range(64):
            if(debug): print 'y:%d = %d  '% (y,self.eye[y][32])   
            bottom = y
            if (self.eye[y][center] == 0) : 
              if(debug): print 'Bottom: %d'% bottom 
              self.minV = bottom
              break
           
          self.height = int(float(top-bottom)/62 * 500)
           
          y_line = 'Top: %d, Bottom: %d, Height: %d(~%dmV)'% (top,bottom, top-bottom, self.height)
          
          print x_line
          print y_line
          print 'Time: %d'%scan_time
          self.fid2.write(x_line0+'\n')          
          self.fid2.write(y_line+'\n')
          self.fid2.close()
                
                


        # Find max H and V
#        self.minV = 32
#        self.maxV = 32
#        self.minH = 32
#        self.maxH = 32
#        
#        for findH in range(64,128):
#            if self.eye[128+self.saOffset][findH] == 0:
#                self.maxH = findH
#
#        for findH in range(63,-1,-1):
#            if self.eye[128+self.saOffset][findH] == 0:
#                self.minH = findH
#        for findV in range(128+self.saOffset,256):
#            if self.eye[findV][64] == 0:
#                self.maxV = findV
#        for findV in range(128+self.saOffset,-1,-1):
#            if self.eye[findV][64] == 0:
#                self.minV = findV
#
#        self.controls.pps1 = wx.StaticText(self.panel,-1,'Horiz:'+str(self.minH)+'-'+str(self.maxH)+' (/128)',pos=(self.origX+128*3+10,self.origY))
#        valStrH = '(%5.3f UI)' % ((self.maxH-self.minH)/128.0)
#        self.controls.pps2 = wx.StaticText(self.panel,-1,valStrH,pos=(self.origX+128*3+10,self.origY+15))
#        self.controls.pps3 = wx.StaticText(self.panel,-1,'Vert: '+str(self.minV)+'-'+str(self.maxV)+' (/256)',pos=(self.origX+128*3+10,self.origY+30))
#        valStrV = '(~%.0f mV)' % ((self.maxV-self.minV)*1.85)
#        self.controls.pps4 = wx.StaticText(self.panel,-1,valStrV,pos=(self.origX+128*3+10,self.origY+45))
#                
#        print "min/max H",self.minH,self.maxH,valStrH,"min/max V",self.minV,self.maxV,valStrV
        self.UpdateFastDiamond = True
        self.PostPlotSummary = True
        self.controls.panel.Refresh()  
        self.controls.panel.Update()                     

        #self.fid.close()
        self.controls.goButton.Enable(True)
        self.controls.stopButton.Enable(False)
  

    def populate(self):
        # go through the checkboxes and find the Rx on
        for checkboxRx in self.controls.checkboxRx:
          self.rxn=self.rxn+1
          if checkboxRx.GetValue():
            break
        print "Rx%d selected" %(self.rxn)
        
        #i = -1
        #for checkboxDevice in self.controls.checkboxDevice:
        #  i=i+1
        #  if checkboxDevice.GetValue():
        #    self.device = self.controls.checkboxDevice[i].GetLabel()
			
		#GJL 9/6/2012 - Use new RadioBox
        self.device = self.controls.m_deviceBox.GetStringSelection() 		
        print "%s selected" %(self.device)
        self.startup_rx()
        
        

