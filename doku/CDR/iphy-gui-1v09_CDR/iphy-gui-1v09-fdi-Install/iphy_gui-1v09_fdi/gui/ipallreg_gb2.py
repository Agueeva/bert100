# ------------------------------------------------------------------------------
#
# $RCSfile: validation#python#ipallreg.py,v $: - All registers' I/O of iPHY/other devices in System
#
# Disclaimer:
# Inphi Confidential
# Copyright(c) Inphi Corp. 2011
#
# All rights reserved.
# This is unpublished, confidential Inphi proprietary information.
# Do not reproduce or redistribute without written permission.

'''


'''
#GJL 10/17/2012 - Disable FullSystem Read2Write
#GJL 10/18/2012 - regWrite (no Ack)

import bv
import wx
import wx.aui
import re
import time
import iputils
import ipvar
import ipmdio
import os
import ipeyeGB2
import ipGB2Tx
import ipeye
import ipcauieye
import sys
import ipcdrmon
import ipcauimon2
import ipcauidebug
#import ipBER
#import ipBER_optical
import gb2BER
import string
from ordict import *
#from ireg import *

mySetup = 'GB_EVB'
stop = 0

MyGREY = wx.Colour(230, 230, 230)

def regRead(reg):
  retVal = ipmdio.regValRead(reg)
  return retVal

# def regRead(reg):
    # if ipvar.MDIO_INIT:
        # retVal = ipmdio.regValRead(reg)
        # #print "regRead:",reg,"=",retVal
    # else:
        # import random
        # #print "regRead('%s')" %(reg)
        # retVal = random.randint(0,0xffff)
    # return retVal

def regWrite(reg,val):
    if ipvar.MDIO_INIT:
        #ipmdio.regWriteAck(reg,val)
        ipmdio.regWrite(reg,val)
    else:
        pass
        #print "regWrite('%s',%s)" %(reg,val)

def regWriteAndRead(reg, value):
    regWrite(reg, value)
    return(regRead(reg))
    
class MyNotebookPage():
    
    def create(self,notebook,device,frame,blank=0):
        self.notebook = notebook
        self.device = device
        self.frame = frame
        self.blank = blank
        #print "Creating notebook page:",device
        self.panel = wx.Panel(notebook, -1)
        notebook.AddPage(self.panel,device)
        self.update()
        
    def update(self):        
        if self.blank == 0: 
          self.scroll = wx.ScrolledWindow(self.panel, -1)
          self.scrollPanel = wx.Panel(self.scroll, -1)
       
          self.sizer = wx.BoxSizer(wx.VERTICAL)
          self.scrollPanel.SetSizer(self.sizer)
          self.sizerBS = wx.BoxSizer(wx.VERTICAL)
          self.sizerBS.Add(self.scroll, 1, wx.EXPAND)
          self.panel.SetSizer(self.sizerBS)
          self.topHeader = wx.BoxSizer(wx.HORIZONTAL) 

          self.topHeader.AddSpacer((315, 22))
          self.bsHeader = wx.BoxSizer(wx.HORIZONTAL)
          self.bsHeader.AddSpacer(12)
          self.bsHeader.Add(wx.StaticText(self.scrollPanel,-1,' Register',            wx.DefaultPosition,wx.Size(90, -1)))
          self.bsHeader.Add(wx.StaticText(self.scrollPanel,-1,' Register Description',wx.DefaultPosition,wx.Size(150, -1)))
          self.bsHeader.Add(wx.StaticText(self.scrollPanel,-1,' Bit Description',     wx.DefaultPosition,wx.Size(227, -1)))
          self.bsHeader.Add(wx.StaticText(self.scrollPanel,-1,' Write',               wx.DefaultPosition,wx.Size(40, -1)))
          self.bsHeader.Add(wx.StaticText(self.scrollPanel,-1,' Read',                wx.DefaultPosition,wx.Size(50, -1)))
        
          self.sizer.Add(self.topHeader)
          self.sizer.Add(self.bsHeader)
          self.sizer.index = 3
        
          for eachRegisterKey in ipvar.System[self.device].regDefTab.keys():
            fieldBox().create(self, self.device, eachRegisterKey, self.scrollPanel, self.sizer, ipvar.System[self.device].regDefTab[eachRegisterKey])
          self.frame.refreshScrollWindowArea(self.scrollPanel, self.scroll, self.sizer)
        
    def addSweepControls(self,frame,device):         
      self.staticBox1 = wx.StaticBox(self.panel,-1,label='Common Sweeps',pos=(10,20),style=wx.RAISED_BORDER, size=(135,100)) #,
      self.boxSizer1 = wx.StaticBoxSizer(self.staticBox1,wx.VERTICAL)
      self.rb1 = wx.RadioButton(self.panel,-1,"Tx pre/pst",pos=(20,40),style=wx.RB_GROUP)
      self.rb2 = wx.RadioButton(self.panel,-1,"Tx pre/pst/swg",pos=(20,60))
      self.rb2.Enable(False)
      self.rb3 = wx.RadioButton(self.panel,-1,"Tx pre/pst/swg/RxEQ",pos=(20,80))
      self.rb3.Enable(False)
      rx=150
      ry=20
      self.staticBox2 = wx.StaticBox(self.panel,-1,label='Transmitter',pos=(rx,ry),style=wx.RAISED_BORDER,size=(370,50)) #,
      self.cbt=[]
      self.cbt.append(wx.RadioButton(self.panel,-1,"Tx0",pos=(rx+10,ry+20),style=wx.RB_GROUP))
      self.cbt.append(wx.RadioButton(self.panel,-1,"Tx1",pos=(rx+50,ry+20)))
      self.cbt.append(wx.RadioButton(self.panel,-1,"Tx2",pos=(rx+90,ry+20)))
      self.cbt.append(wx.RadioButton(self.panel,-1,"Tx3",pos=(rx+130,ry+20)))
      #self.cbt.append(wx.RadioButton(self.panel,-1,"Tx4",pos=(rx+170,ry+20)))
      #self.cbt.append(wx.RadioButton(self.panel,-1,"Tx5",pos=(rx+210,ry+20)))
      
      rx=150
      ry=70
      self.staticBox2 = wx.StaticBox(self.panel,-1,label='Receiver',pos=(rx,ry),style=wx.RAISED_BORDER,size=(370,50)) #,
      #self.boxSizer2 = wx.StaticBoxSizer(self.staticBox2,wx.VERTICAL)
      self.checkboxRx=[]
      self.checkboxRx.append(wx.RadioButton(self.panel,-1,"Rx0",pos=(rx+10,ry+20),style=wx.RB_GROUP))
      self.checkboxRx.append(wx.RadioButton(self.panel,-1,"Rx1",pos=(rx+50,ry+20)))
      self.checkboxRx.append(wx.RadioButton(self.panel,-1,"Rx2",pos=(rx+90,ry+20)))
      self.checkboxRx.append(wx.RadioButton(self.panel,-1,"Rx3",pos=(rx+130,ry+20)))
      #self.checkboxRx.append(wx.RadioButton(self.panel,-1,"Rx4",pos=(rx+170,ry+20)))
      #self.checkboxRx.append(wx.RadioButton(self.panel,-1,"Rx5",pos=(rx+210,ry+20)))
      
      choices=['35s (1e-12)','3.5s (1e-11)','1s','350ms (1e-10)','0s']
      self.timeCombo  = wx.ComboBox(self.panel,-1,value='350ms(1e-10)',choices=choices,pos=(210,133),style=wx.ALIGN_RIGHT,name='timeCombo')
      self.timeCombo.Enable(False)
      self.goButton   = wx.Button(self.panel,-1,label="Start Sweep",pos=(10,130))
      self.goButton.controls = self
      self.stopButton = wx.Button(self.panel,-1,label="Stop Sweep",pos=(110,130))
      self.stopButton.controls = self
      self.stop = 0
      #self.stopButton.Enable(False)
                 
      #self.sizer.Add(self.boxSizer1)
      #self.sizer.Add(self.boxSizer2)
      #self.sizer.Add(goButton)
      self.panel.Refresh()
      frame.Refresh()
      frame.Layout()
      #frame.refreshScrollWindowArea(self.scrollPanel, self.scroll, self.sizer)
      
    def addEyeScanControls(self,frame,device):            
      rx=250
      ry=20
      self.staticBoxDev = wx.StaticBox(self.panel,-1,label='Device',pos=(rx,ry),style=wx.RAISED_BORDER,size=(220,40))
      
      self.checkboxDevice=[]
      Devices = 0
      for eachDevice in ipvar.System.keys():
        if Devices == 0:
          self.checkboxDevice.append(wx.RadioButton(self.panel,-1,eachDevice,pos=(rx+10+70*Devices,ry+16),style=wx.RB_GROUP))
          Devices = Devices + 1
        else:
          self.checkboxDevice.append(wx.RadioButton(self.panel,-1,eachDevice,pos=(rx+10+70*Devices,ry+16)))
          Devices = Devices + 1
                  
      self.staticBoxRx = wx.StaticBox(self.panel,-1,label='Receiver',pos=(rx,ry+40),style=wx.RAISED_BORDER,size=(210,40))
      self.checkboxRx=[]
      self.checkboxRx.append(wx.RadioButton(self.panel,-1,"Rx0",pos=(rx+10,ry+56),style=wx.RB_GROUP))
      self.checkboxRx.append(wx.RadioButton(self.panel,-1,"Rx1",pos=(rx+60,ry+56)))
      self.checkboxRx.append(wx.RadioButton(self.panel,-1,"Rx2",pos=(rx+110,ry+56)))
      self.checkboxRx.append(wx.RadioButton(self.panel,-1,"Rx3",pos=(rx+160,ry+56)))

      self.cdrSave = wx.CheckBox(self.panel,-1,label='Capture CDR PI',pos=(rx+10,ry+90))
      self.eyeColour = wx.CheckBox(self.panel,-1,label='ColourMap',pos=(rx+120,ry+90))
      self.eyeColour.SetValue(True)
      self.legacyCSV = wx.CheckBox(self.panel,-1,label='Legacy CSV',pos=(rx+200,ry+90))
      self.legacyCSV.SetValue(False)
      
      self.goButton   = wx.Button(self.panel,-1,label="Start Eye Scan",pos=(5,25),size=(100,24))
      self.goButton.controls = self
      
      self.stopButton = wx.Button(self.panel,-1,label="Stop Eye Scan",pos=(5,50),size=(100,22))
      self.stopButton.controls = self 
      self.stopButton.Enable(False)

      self.refreshButton = wx.Button(self.panel,-1,label="Refresh Plot",pos=(5,75),size=(100,22))
      self.refreshButton.controls = self 
      self.refreshButton.Enable(False)
      
      #self.saveButton = wx.Button(self.panel,-1,label="Save to bmp",pos=(5,75),size=(100,22))
      #self.saveButton.controls = self
      
      self.stop = 0     
      rx=115
      ry=20      
      self.staticBox3 = wx.StaticBox(self.panel,-1,label='Modes',pos=(rx,ry),style=wx.RAISED_BORDER,size=(130,107)) #,

      self.cbMode={}
      self.cbMode['Inner Eye']       = wx.RadioButton(self.panel,-1,"Inner Eye",pos=(rx+10,ry+20),style=wx.RB_GROUP)
      self.cbMode['Inner Eye'].Enable(False)
      self.cbMode['Full Scan']       = wx.RadioButton(self.panel,-1,"Full Scan",pos=(rx+10,ry+40))
      self.cbMode['Full Scan'].Value = True
      self.cbMode['Fast Diamond'] = wx.RadioButton(self.panel,-1,'Fast Diamond',pos=(rx+10,ry+60))
      self.cbMode['Live Height']    = wx.RadioButton(self.panel,-1,"Live Height",pos=(rx+10,ry+80))
      self.cbMode['Live Height'].Enable(True)
      #self.cbMode['Live Diamond'].Enable(False)
      self.panel.Refresh()
      frame.Refresh()
      frame.Layout()
      #frame.refreshScrollWindowArea(self.scrollPanel, self.scroll, self.sizer)
      
    def addCauiEyeScanControls(self,frame,device): 
	
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
      
	  #GJl - 09/06/2012 
	  #Select GB Devices only
	  
      self.GB_list = []
      for eachDevice in ipvar.System.keys():
        if (eachDevice.find("GB")==0):
          self.GB_list.append(eachDevice)
      
	  self.m_deviceBox = wx.RadioBox(self.panel, label="Device", pos=(rx,ry), choices=self.GB_list,  majorDimension=0)
      self.device = self.GB_list[0]    	
	  
      ry = 27
      self.staticBoxRx = wx.StaticBox(self.panel,-1,label='Receiver',pos=(rx,ry+40),style=wx.RAISED_BORDER,size=(260,60))
      self.checkboxRx=[]
      self.checkboxRx.append(wx.RadioButton(self.panel,-1,"Rx0",pos=(rx+10,ry+56),style=wx.RB_GROUP))
      self.checkboxRx.append(wx.RadioButton(self.panel,-1,"Rx1",pos=(rx+60,ry+56)))
      self.checkboxRx.append(wx.RadioButton(self.panel,-1,"Rx2",pos=(rx+110,ry+56)))
      self.checkboxRx.append(wx.RadioButton(self.panel,-1,"Rx3",pos=(rx+160,ry+56)))
      self.checkboxRx.append(wx.RadioButton(self.panel,-1,"Rx4",pos=(rx+210,ry+56)))
      self.checkboxRx.append(wx.RadioButton(self.panel,-1,"Rx5",pos=(rx+10,ry+76)))
      self.checkboxRx.append(wx.RadioButton(self.panel,-1,"Rx6",pos=(rx+60,ry+76)))
      self.checkboxRx.append(wx.RadioButton(self.panel,-1,"Rx7",pos=(rx+110,ry+76)))
      self.checkboxRx.append(wx.RadioButton(self.panel,-1,"Rx8",pos=(rx+160,ry+76)))
      self.checkboxRx.append(wx.RadioButton(self.panel,-1,"Rx9",pos=(rx+210,ry+76)))

      #self.cdrSave = wx.CheckBox(self.panel,-1,label='Capture CDR PI',pos=(rx+10,ry+110))
      self.eyeColour = wx.CheckBox(self.panel,-1,label='ColourMap',pos=(rx+160,ry+110))
      self.eyeColour.SetValue(True)
      
      self.goButton   = wx.Button(self.panel,-1,label="Start CAUI Scan",pos=(5,25),size=(100,24))
      self.goButton.controls = self
      
      self.stopButton = wx.Button(self.panel,-1,label="Stop Eye Scan",pos=(5,50),size=(100,22))
      self.stopButton.controls = self 
      self.stopButton.Enable(False)

      self.refreshButton = wx.Button(self.panel,-1,label="Refresh Plot",pos=(5,75),size=(100,22))
      self.refreshButton.controls = self 
      self.refreshButton.Enable(False)
      
      #self.saveButton = wx.Button(self.panel,-1,label="Save to bmp",pos=(5,75),size=(100,22))
      #self.saveButton.controls = self
      
      self.stop = 0     
      rx=115
      ry=20      
      self.staticBox3 = wx.StaticBox(self.panel,-1,label='Modes',pos=(rx,ry),style=wx.RAISED_BORDER,size=(130,107)) #,
      self.cbMode={}
      self.cbMode['Inner Eye']       = wx.RadioButton(self.panel,-1,"Inner Eye",pos=(rx+10,ry+20),style=wx.RB_GROUP)
      self.cbMode['Inner Eye'].Enable(False)
      self.cbMode['Full Scan']       = wx.RadioButton(self.panel,-1,"Full Scan",pos=(rx+10,ry+40))
      self.cbMode['Full Scan'].Value = True
      self.cbMode['Fast Diamond'] = wx.RadioButton(self.panel,-1,'Fast Diamond',pos=(rx+10,ry+60))
      self.cbMode['Fast Diamond'].Enable(False)
      self.cbMode['Live Height']    = wx.RadioButton(self.panel,-1,"Live Height",pos=(rx+10,ry+80))
      self.cbMode['Live Height'].Enable(False)
      self.panel.Refresh()
      frame.Refresh()
      frame.Layout()
      #frame.refreshScrollWindowArea(self.scrollPanel, self.scroll, self.sizer)
         
class fieldBox():

    def create(self, parent, hierarchy, register, panel, panelSizer, regDict):
        staticbox_title = regDict.regName
        hzSizer = wx.BoxSizer(wx.HORIZONTAL) 
        regDict.hierarchy = hierarchy       
        regDict.expanded = 0
        regDict.expandButton = wx.Button(parent=panel, id= -1, label='+', size=(14, 20), name='expand register', style=wx.ST_NO_AUTORESIZE) #=wx.RAISED_BORDER|
        font=wx.Font(7, wx.FONTFAMILY_DEFAULT,wx.NORMAL,wx.BOLD)
        regDict.expandButton.SetFont(font)
        regDict.expandButton.parent = regDict        #expandButton[-1].expanded   = 0
        regDict.expandButton.hierarchy = hierarchy
        regDict.expandButton.register = register
        regDict.expandButton.panel = panel
        regDict.expandButton.panelSizer = panelSizer
        regDict.expandButton.regDict = regDict
        regDict.expandButton.hzSizer = hzSizer
        regDict.expandButton.index = panelSizer.index
        
        reg_sub = register.split('.')
        if reg_sub[0] == '1':
          self.staticTextS1 = wx.StaticText(id= -1, label=reg_sub[0]+'.'+str(hex(string.atoi(reg_sub[1]))[2:]).upper(), name='staticText1', parent=panel, size=wx.Size(90, 22), style=wx.RAISED_BORDER)
        else:
          self.staticTextS1 = wx.StaticText(id= -1, label=register, name='staticText1', parent=panel, size=wx.Size(90, 22), style=wx.RAISED_BORDER)
        self.staticTextS2 = wx.StaticText(id= -1, label=staticbox_title, name='staticText2', parent=panel, size=wx.Size(150, 22), style=wx.RAISED_BORDER)
        regDict.entryWrite = wx.TextCtrl(panel, -1, str(hex(int(regDict.defVec.value))[2:].zfill(4)), size=(43, 22), style=wx.RAISED_BORDER | wx.TE_PROCESS_ENTER | wx.ALIGN_RIGHT)
        regDict.lastWrite = int(regDict.defVec.value)
        regDict.entryWrite.parent = regDict
        regDict.entryWrite.isRead = 0
        regDict.register = register
        spacer1 = wx.TextCtrl(panel, -1, value=u"hex:", size=(222, 22), style=wx.ALIGN_RIGHT|wx.RAISED_BORDER) 
        spacer1.Enable(False)           
        regDict.entryRead = wx.TextCtrl(panel, -1, str(hex(int(regDict.defVec.value))[2:].zfill(4)), size=(43, 22), style=wx.RAISED_BORDER| wx.TE_PROCESS_ENTER|wx.ST_NO_AUTORESIZE)
        regDict.entryRead.SetBackgroundColour(MyGREY)
        regDict.entryRead.parent = regDict
        regDict.entryRead.isRead = 1
        myBlue     = wx.Colour(150, 200, 255)
        myBlueGrey = wx.Colour(200, 220, 255)
        myYellow   = wx.Colour(250, 250, 130)
        myGreen    = wx.Colour(170, 220, 170)
        if re.search('10G|Tx Error|CAUI',staticbox_title):
          self.staticTextS1.SetBackgroundColour(myGreen)
          self.staticTextS2.SetBackgroundColour(myGreen)          
        elif re.search('PMA|Device|Test P.* Ability|PRBS .* Control|Tx Square|MMD30|PLL|MemBus|Custom|LOL|FIFO|Manual|Sample|CDR|PI Control [0,2,3]|Impedance|Amux|Vreg|Trim|Eye Sc|Rx.*Eq |2nd order',staticbox_title):
          self.staticTextS1.SetBackgroundColour(myBlueGrey)
          self.staticTextS2.SetBackgroundColour(myBlueGrey)   
        elif re.search('trim|Fuse|Sense Amp',staticbox_title):
          self.staticTextS1.SetBackgroundColour(myBlueGrey)
          self.staticTextS2.SetBackgroundColour(myBlueGrey)                   
        else:
          self.staticTextS1.SetBackgroundColour(myBlue)
          self.staticTextS2.SetBackgroundColour(myBlue)
          
        #spacer1.SetBackgroundColour(wx.Colour(150, 200, 255))
        hzSizer.Add(regDict.expandButton)
        hzSizer.Add(self.staticTextS1)
        hzSizer.Add(self.staticTextS2)
        hzSizer.Add(spacer1)
        hzSizer.Add(regDict.entryWrite)
        hzSizer.AddSpacer(5)
        hzSizer.Add(regDict.entryRead)
        panelSizer.Add(hzSizer) 
        panelSizer.index = panelSizer.index + 1
       
    def ToggleSubReg(self, parent, hierarchy, register, panel, panelSizer, regDict, atIndex):  
        NumberOfSubRegs = len(regDict.regSubTab.keys())
        if regDict.expanded == 1:
            regDict.expandButton.SetLabel('+')
            for subreg in regDict.regSubTab:
              regDict.regSubTab[subreg].combo.Destroy()
              regDict.regSubTab[subreg].entryRead.Destroy()
              regDict.regSubTab[subreg].entryWrite.Destroy()
              regDict.regSubTab[subreg].staticTextS1.Destroy()
              regDict.regSubTab[subreg].staticTextS2.Destroy()
              panelSizer.Remove(regDict.regSubTab[subreg].hzSizer) 
            regDict.expanded = 0
            NumberOfSubRegs = -NumberOfSubRegs
            self.resequenceSizerIndex(atIndex, NumberOfSubRegs,hierarchy) 
            panel.Refresh()
            panel.Update()             
        else:
            regDict.expanded = 1         
            for subreg in regDict.regSubTab:          
                regDict.regSubTab[subreg].hzSizer = wx.BoxSizer(wx.HORIZONTAL)           
                choices = []
                bitPixelWidth = regDict.regSubTab[subreg].nBits           
                if len(regDict.regSubTab[subreg].purposeTab) == 0:
                    initialValue = " "
                    choices = [""]
                else:
                    index = regDict.regSubTab[subreg].purposeTab.keys()[0]
                    initialValue = regDict.regSubTab[subreg].purposeTab[index] 
                for items in regDict.regSubTab[subreg].purposeTab:
                    choices.append(regDict.regSubTab[subreg].purposeTab[items])
                           
                regDict.regSubTab[subreg].staticTextS1 = wx.StaticText(id= -1, label=subreg, name='staticText1', parent=panel, size=wx.Size(90, 22), style=wx.RAISED_BORDER)
                regDict.regSubTab[subreg].staticTextS2 = wx.StaticText(id= -1, label=regDict.regSubTab[subreg].purposeName, name='staticText1', parent=panel, size=wx.Size(150, 22), style=wx.RAISED_BORDER)

                regDict.regSubTab[subreg].combo = wx.ComboBox(value=initialValue, choices=choices, id= -1, name='comboBox1', parent=panel, style=wx.RAISED_BORDER, size=(250 - 7 * bitPixelWidth, 22))
                regDict.regSubTab[subreg].combo.parent = regDict
                regDict.regSubTab[subreg].combo.subregOrdDict = regDict.regSubTab[subreg]
                regDict.regSubTab[subreg].combo.hierarchy = hierarchy
                regDict.regSubTab[subreg].combo.subreg = subreg
                                
                regDict.regSubTab[subreg].entryWrite = wx.TextCtrl(panel, -1, value="0" * bitPixelWidth, size=(15 + 7 * bitPixelWidth, 22), style=wx.RAISED_BORDER | wx.TE_PROCESS_ENTER)
                regDict.regSubTab[subreg].entryWrite.parent = regDict
                regDict.regSubTab[subreg].entryWrite.subregOrdDict = regDict.regSubTab[subreg]
                regDict.regSubTab[subreg].entryWrite.hierarchy = hierarchy
                regDict.regSubTab[subreg].entryWrite.subreg = subreg
           
                regDict.regSubTab[subreg].entryRead = wx.StaticText(panel, -1, label="0" * bitPixelWidth, size=(15 + 7 * bitPixelWidth, 22), style=wx.RAISED_BORDER|wx.ST_NO_AUTORESIZE)
                regDict.regSubTab[subreg].staticTextS1.SetBackgroundColour(MyGREY)
                regDict.regSubTab[subreg].staticTextS2.SetBackgroundColour(MyGREY)         
                
                if regDict.regSubTab[subreg].bitMask.find("W") > -1:
                    regDict.regSubTab[subreg].entryWrite.SetBackgroundColour(wx.Colour(255, 255, 255))
                else:
                    regDict.regSubTab[subreg].entryWrite.SetBackgroundColour(MyGREY)
                    regDict.regSubTab[subreg].entryWrite.SetForegroundColour(wx.LIGHT_GREY)
                    regDict.regSubTab[subreg].entryWrite.Enable(False)
                    regDict.regSubTab[subreg].combo.Enable(False)
                                  
                regDict.regSubTab[subreg].entryRead.SetBackgroundColour(MyGREY)
                regDict.regSubTab[subreg].hzSizer.AddSpacer(14)           
                regDict.regSubTab[subreg].hzSizer.Add(regDict.regSubTab[subreg].staticTextS1)
                regDict.regSubTab[subreg].hzSizer.Add(regDict.regSubTab[subreg].staticTextS2)
                regDict.regSubTab[subreg].hzSizer.Add(regDict.regSubTab[subreg].combo) 
                regDict.regSubTab[subreg].hzSizer.Add(regDict.regSubTab[subreg].entryWrite)
                regDict.regSubTab[subreg].hzSizer.AddSpacer(5) 
                regDict.regSubTab[subreg].hzSizer.Add(regDict.regSubTab[subreg].entryRead) 
                panelSizer.Insert(atIndex, regDict.regSubTab[subreg].hzSizer)
                
                regDict.expandButton.SetLabel('-')
            panelSizer.Layout()
            self.resequenceSizerIndex(atIndex, NumberOfSubRegs,hierarchy)
            
    def resequenceSizerIndex(self, atIndex, indexMoves, hierarchy):
      for eachRegisterKey in ipvar.System[hierarchy].regDefTab:
          if ipvar.System[hierarchy].regDefTab[eachRegisterKey].expandButton.index > atIndex:
              ipvar.System[hierarchy].regDefTab[eachRegisterKey].expandButton.index += indexMoves
  
class sweepObj():
    def __init__(self,controls):
        self.input = []
        self.output = []
        self.result = {}
        self.outputFilename = 'sweep_out.csv'
        self.rectSize=15
        self.origX = 50
        self.origY = 200
        self.panel = controls.panel
        
    def OnPaintSweep(self,event):
        dc = wx.PaintDC(self.panel)
        font = wx.Font(8, wx.MODERN, wx.NORMAL, wx.NORMAL)
        dc.SetFont(font)
        #dc.Clear()
        #dc.BeginDrawing()
        dc.SetPen(wx.Pen("GRAY",1))
        for eachRegValY in self.input[1]['VALUES']:
            for eachRegValX in self.input[0]['VALUES']:
                if int(self.result[eachRegValY][eachRegValX][1]) == 0:
                    dc.SetBrush(wx.Brush('RED', wx.SOLID))
                elif int(self.result[eachRegValY][eachRegValX][0]) == 0:
                    dc.SetBrush(wx.Brush(('GREEN'), wx.SOLID))
                elif int(self.result[eachRegValY][eachRegValX][0]) == -1:
                    dc.SetBrush(wx.Brush(('WHITE'), wx.SOLID))                
                elif int(self.result[eachRegValY][eachRegValX][0]) == 65535:
                    dc.SetBrush(wx.Brush('RED', wx.SOLID))
                else:
                    dc.SetBrush(wx.Brush('YELLOW', wx.SOLID))
                dc.DrawRectangle(eachRegValX*self.rectSize+self.origX,eachRegValY*self.rectSize+self.origY,self.rectSize,self.rectSize)
        # Y axis
        for eachRegValY in self.input[1]['VALUES']:
            dc.DrawText(str(eachRegValY),self.origX-15,eachRegValY*self.rectSize+self.origY)
        # X axis
        for eachRegValX in self.input[0]['VALUES']:
            dc.DrawText(str(eachRegValX),eachRegValX*self.rectSize+self.origX+3,self.origY-15)
        # labels
        dc.DrawText(self.input[0]['LABEL'],self.origX+10,self.origY-30)
        dc.DrawRotatedText(self.input[1]['LABEL'],self.origX-35,self.origY+70, 90)
                    
#        for x in range(0,21):
#            for y in range(0,11):
#                if True: #self.sweepObj[x][y] == [-1,-1,-1]:
#                    dc.SetBrush(wx.Brush(('WHITE'), wx.SOLID))
#                else:
#                    colour = self.sweepObj[x][y]
#                    dc.SetBrush(wx.Brush((colour), wx.SOLID))
#                dc.DrawRectangle(x*self.rectSize+self.origX,y*self.rectSize+self.origY,self.rectSize,self.rectSize)
        #print "did a paint"
        #dc.EndDrawing()
        #del dc
        #event.Skip()

    def Do(self,controls,topFrame):       
        print "Do the sweep with",len(self.input),"parameters"
        controls.stop = 0
        fid=open(self.outputFilename,'w')
        print "sweep data saved to: %s" %(self.outputFilename)
        
        controls.panel.Bind(wx.EVT_PAINT,self.OnPaintSweep)
        # Build the space and empty results
        for eachRegValY in self.input[1]['VALUES']:
            self.result[eachRegValY] = {}
            for eachRegValX in self.input[0]['VALUES']:
                self.result[eachRegValY][eachRegValX] = []
                for eachOutput in self.output:
                  self.result[eachRegValY][eachRegValX].append(-1)  
                        
        controls.panel.Refresh()  
        controls.panel.Update()        
        headerStr = [] # just build the header string for the .csv
        for each in self.input:          
          headerStr.append(each['LABEL'])         
        for each in self.output:
            headerStr.append(each['LABEL'])            
        print ','.join(headerStr)
        fid.write(','.join(headerStr)+'\n')
        
        for eachRegValY in self.input[1]['VALUES']:
            if controls.stop == 0:
              regWrite(self.input[1]['REG'],eachRegValY)
              for eachRegValX in self.input[0]['VALUES']:
                  if controls.stop == 0:
                    regWrite(self.input[0]['REG'],eachRegValX)
                    for eachOutput in self.output:
                      val = regRead(eachOutput['REG'])
                    time.sleep(0.35)
                    ValStr = []
                    for eachOutput in self.output:
                        val = regRead(eachOutput['REG'])
                        ValStr.append(str(val)) 
                    controls.panel.Refresh() 
                    controls.panel.Update()
                    #topFrame.Update()
                    #topFrame.Refresh()
                    wx.Yield()
                    self.result[eachRegValY][eachRegValX] = ValStr
                    print eachRegValY,eachRegValX,ValStr
                    controls.panel.Refresh()          
                    fid.write('%s,%s,%s,%s\n' %(eachRegValY,eachRegValX,ValStr[0],ValStr[1]))
                  else:
                    break
        fid.close()
               
            
    def addInput(self, inputStr, range, label):
        self.input.append(OrderedDict()) #[inputStr,range,label])
        self.input[-1]['REG']=inputStr
        self.input[-1]['VALUES']=range
        self.input[-1]['LABEL']=label
      
    def addOutput(self, outputStr, label):
        self.output.append(OrderedDict()) #[outputStr,label])
        self.output[-1]['REG']=outputStr
        self.output[-1]['LABEL']=label

    def populate(self,controls):
        txn=-1                     # go through the checkboxes and find the Tx on
        for cbt in controls.cbt:
          txn=txn+1
          if cbt.GetValue():
            break
        rxn=-1                     # go through the checkboxes and find the Rx on
        for checkboxRx in controls.checkboxRx:
          rxn=rxn+1
          if checkboxRx.GetValue():
            break
        print "Tx%d / Rx%d selected" %(txn,rxn)
        
        if device.find("CDR") == 0:
          self.addInput(ipvar.CDR_Name+'::30.'+str(256*(txn+1)+1)+'.10:8' , range(0, 8), 'tx25_'+str(txn)+'_eqpst') # register, range, label
          self.addInput(ipvar.CDR_Name+'::30.'+str(256*(txn+1)+1)+'.1:0'  , range(0, 4), 'tx25_'+str(txn)+'_eqpre')  
          #self.addInput(ipvar.CDR_Name+'::30.'+str(256*(txn+1)+1)+'.2:0'  , range(0, 5), 'tx25_'+str(txn)+'_txa_swing')  
          #self.addInput(ipvar.CDR_Name+'::30.'+str(256*(txn+1)+1)+'.2:0'  , range(0, 5), 'tx25_'+str(txn)+'_txa_swing')  
          self.addOutput(ipvar.CDR_Name+'::30.'+str(48+rxn)+'.15:0' , 'rx25_'+str(rxn)+'_errcount')
          self.addOutput(ipvar.CDR_Name+'::30.'+str(421+rxn*256)+'.3:0', 'rx25_'+str(rxn)+'_eq_state')
        elif device.find("GB") == 0:
          self.addInput(device+'::30.'+ str(1552+txn) + '.15:12' , range(0, 16), 'tx10_'+str(txn)+'_eqpst') # register, range, label
          self.addInput(device+'::30.'+ str(1552+txn) + '.2:0'  , range(0, 8), 'tx10_'+str(txn)+'_eqpre')  
          #self.addInput(device+'::30.'+ str(256*(tx+1)+1)+'.2:0'  , range(0, 5), 'txXX_'+str(tx)+'_txa_swing')  
          #self.addInput(device+'::30.'+ str(256*(tx+1)+1)+'.2:0'  , range(0, 5), 'tx25_'+str(tx)+'_txa_swing')  
          self.addOutput(device+'::8.'+ str(1600+rxn)+'.15:0' , 'rx10_'+str(rxn)+'_errcount')
          self.addOutput(device+'::30.'+ str(20+rxn) + '.15', 'rx10_'+str(rxn)+'_patlock')
  
class MyFrame(wx.Frame):
    def __init__(self, parent, id, title, system):
        wx.Frame.__init__(self, parent, id, title, pos=(390, 0), size=(630, 770))
        self.SetTitle("Inphi MDIO System Controller - Version 1.09") # FIXME! - need global var        
        self.icon = wx.Icon('../gui/inphi.ico', wx.BITMAP_TYPE_ICO)
        self.frame = self
        self.SetIcon(self.icon)
        self.parent = parent
        debug=False
        self.InitialScanDone = 0
        filemenu  = wx.Menu()
        toolsmenu = wx.Menu()
        submenu   = wx.Menu()
        #init_menu = wx.Menu()
        #init_submenu = wx.Menu()
        init_menu = {}
        #init_menu['WXMENU'] = wx.Menu()
        
        helpmenu  = wx.Menu()
        saveAll = filemenu.Append(wx.ID_ANY, "&Save Register Setup", "Saves all registers in all devices")
        #toggleMain = filemenu.Append(wx.ID_ANY, "&Main", "Show/Hide Main window", wx.ITEM_CHECK)
        quit    = filemenu.Append(wx.ID_EXIT,'&Exit', 'Exit application')
        shell   = toolsmenu.Append(wx.ID_ANY,'Open Shell',"Opens a new command shell")
        detect  = toolsmenu.Append(wx.ID_ANY,'Detect Devices',"Finds MDIO devices")
        self.show_phaseInt = submenu.Append(wx.ID_ANY,'Show Phase Interpolators',"Shows the graphical PIs",kind=wx.ITEM_CHECK)
        submenu.Check(self.show_phaseInt.GetId(), True)
        #self.legacy_eye_csv = submenu.Append(wx.ID_ANY,'Legacy Eye CSV',"Output Legacy Eye CSV file",kind=wx.ITEM_CHECK)
        #submenu.Check(self.legacy_eye_csv.GetId(), False)
        init_menu['WXMENU'] = wx.Menu()
        for eachDevice in ipvar.System.keys():
          init_menu[eachDevice] = {}
          init_menu[eachDevice]['SUBMENU'] = wx.Menu()
          init_menu['WXMENU'].AppendMenu(wx.ID_ANY,eachDevice+' specific routines...',init_menu[eachDevice]['SUBMENU'])
          
          if eachDevice.find('GB0') == 0:
            init_menu[eachDevice]['power_up'] = init_menu[eachDevice]['SUBMENU'].Append(wx.ID_ANY,'Initialize on Power-up/Reset',"Gearbox Reset after Power On")
            gb0_gen25  = init_menu[eachDevice]['SUBMENU'].Append(wx.ID_ANY,'Set all 25G lanes to PRBS31'," ")
            gb0_fifo25 = init_menu[eachDevice]['SUBMENU'].Append(wx.ID_ANY,'Set all 25G lanes to FIFO mode'," ")
            gb0_ver25  = init_menu[eachDevice]['SUBMENU'].Append(wx.ID_ANY,'Set all 25G lanes to Auto Verify'," ")
            gb0_gen10  = init_menu[eachDevice]['SUBMENU'].Append(wx.ID_ANY,'Set all 10G lanes to PRBS31'," ")
            gb0_fifo10 = init_menu[eachDevice]['SUBMENU'].Append(wx.ID_ANY,'Set all 10G lanes to FIFO mode'," ")
            self.menu_device = eachDevice
            self.Bind(wx.EVT_MENU, self.OnDeviceSpecific_GB0, init_menu[eachDevice]['power_up'])
            self.Bind(wx.EVT_MENU, self.OnDeviceSpecific_GB0_gen25,  gb0_gen25)
            self.Bind(wx.EVT_MENU, self.OnDeviceSpecific_GB0_fifo25, gb0_fifo25)
            self.Bind(wx.EVT_MENU, self.OnDeviceSpecific_GB0_ver25,  gb0_ver25)
            self.Bind(wx.EVT_MENU, self.OnDeviceSpecific_GB0_gen10,  gb0_gen10)
            self.Bind(wx.EVT_MENU, self.OnDeviceSpecific_GB0_fifo10, gb0_fifo10)


          if eachDevice.find('GB1') == 0:
            init_menu[eachDevice]['power_up'] = init_menu[eachDevice]['SUBMENU'].Append(wx.ID_ANY,'Initialize on Power-up/Reset',"Gearbox Reset after Power On")
            gb1_gen25  = init_menu[eachDevice]['SUBMENU'].Append(wx.ID_ANY,'Set all 25G lanes to PRBS31'," ")
            gb1_fifo25 = init_menu[eachDevice]['SUBMENU'].Append(wx.ID_ANY,'Set all 25G lanes to FIFO mode'," ")
            gb1_ver25  = init_menu[eachDevice]['SUBMENU'].Append(wx.ID_ANY,'Set all 25G lanes to Auto Verify'," ")
            gb1_gen10  = init_menu[eachDevice]['SUBMENU'].Append(wx.ID_ANY,'Set all 10G lanes to PRBS31'," ")
            gb1_fifo10 = init_menu[eachDevice]['SUBMENU'].Append(wx.ID_ANY,'Set all 10G lanes to FIFO mode'," ")
            #gbver10  = init_menu[eachDevice]['SUBMENU'].Append(wx.ID_ANY,'Set all 10G lanes to PRBS31 verify'," ")
            self.menu_device = eachDevice
            self.Bind(wx.EVT_MENU, self.OnDeviceSpecific_GB1, init_menu[eachDevice]['power_up'])
            self.Bind(wx.EVT_MENU, self.OnDeviceSpecific_GB1_gen25,  gb1_gen25)
            self.Bind(wx.EVT_MENU, self.OnDeviceSpecific_GB1_fifo25, gb1_fifo25)
            self.Bind(wx.EVT_MENU, self.OnDeviceSpecific_GB1_ver25,  gb1_ver25)
            self.Bind(wx.EVT_MENU, self.OnDeviceSpecific_GB1_gen10,  gb1_gen10)
            self.Bind(wx.EVT_MENU, self.OnDeviceSpecific_GB1_fifo10, gb1_fifo10)
            
            
          if eachDevice.find('CDR0') == 0:
            init_menu[eachDevice]['power_up'] = init_menu[eachDevice]['SUBMENU'].Append(wx.ID_ANY,'Initialize on Power-up/Reset',"CDR Reset after Power On")
            #cdr0gen25  = init_menu[eachDevice]['SUBMENU'].Append(wx.ID_ANY,'Set all 25G lanes to PRBS31 generate'," ")
            #cdr0fifo25 = init_menu[eachDevice]['SUBMENU'].Append(wx.ID_ANY,'Set all 25G lanes to FIFO mode'," ")
            #cdr0ver25  = init_menu[eachDevice]['SUBMENU'].Append(wx.ID_ANY,'Set all 25G lanes to PRBS31 verify'," ")
            cdr0gen25  = init_menu[eachDevice]['SUBMENU'].Append(wx.ID_ANY,'Set all 25G lanes to PRBS31'," ")
            cdr0fifo25 = init_menu[eachDevice]['SUBMENU'].Append(wx.ID_ANY,'Set all 25G lanes to FIFO mode'," ")
            cdr0ver25  = init_menu[eachDevice]['SUBMENU'].Append(wx.ID_ANY,'Set all 25G lanes to Auto verify'," ")
            self.menu_device = eachDevice
            self.Bind(wx.EVT_MENU, self.OnDeviceSpecific_CDR0,init_menu[eachDevice]['power_up'])
            self.Bind(wx.EVT_MENU, self.OnDeviceSpecific_CDR0, init_menu[eachDevice]['power_up'])
            self.Bind(wx.EVT_MENU, self.OnDeviceSpecific_CDR0_gen25,  cdr0gen25)
            self.Bind(wx.EVT_MENU, self.OnDeviceSpecific_CDR0_fifo25, cdr0fifo25)
            self.Bind(wx.EVT_MENU, self.OnDeviceSpecific_CDR0_ver25,  cdr0ver25)

          if eachDevice.find('CDR1') == 0:
            init_menu[eachDevice]['power_up'] = init_menu[eachDevice]['SUBMENU'].Append(wx.ID_ANY,'Initialize on Power-up/Reset',"CDR Reset after Power On GB")
            cdr1gen25  = init_menu[eachDevice]['SUBMENU'].Append(wx.ID_ANY,'Set all 25G lanes to PRBS31'," ")
            cdr1fifo25 = init_menu[eachDevice]['SUBMENU'].Append(wx.ID_ANY,'Set all 25G lanes to FIFO mode'," ")
            cdr1ver25  = init_menu[eachDevice]['SUBMENU'].Append(wx.ID_ANY,'Set all 25G lanes to Auto verify'," ")

            self.menu_device = eachDevice
            self.Bind(wx.EVT_MENU, self.OnDeviceSpecific_CDR1,init_menu[eachDevice]['power_up'])
            self.Bind(wx.EVT_MENU, self.OnDeviceSpecific_CDR1, init_menu[eachDevice]['power_up'])
            self.Bind(wx.EVT_MENU, self.OnDeviceSpecific_CDR1_gen25,  cdr1gen25)
            self.Bind(wx.EVT_MENU, self.OnDeviceSpecific_CDR1_fifo25, cdr1fifo25)
            self.Bind(wx.EVT_MENU, self.OnDeviceSpecific_CDR1_ver25,  cdr1ver25)

          if eachDevice.find('M-CDR0') == 0:
            init_menu[eachDevice]['power_up'] = init_menu[eachDevice]['SUBMENU'].Append(wx.ID_ANY,'Initialize on Power-up/Reset',"CDR Reset after Power On")
            cdr0gen25  = init_menu[eachDevice]['SUBMENU'].Append(wx.ID_ANY,'Set all 25G lanes to PRBS31 generate'," ")
            cdr0fifo25 = init_menu[eachDevice]['SUBMENU'].Append(wx.ID_ANY,'Set all 25G lanes to FIFO mode'," ")
            cdr0ver25  = init_menu[eachDevice]['SUBMENU'].Append(wx.ID_ANY,'Set all 25G lanes to PRBS31 verify'," ")

            self.menu_device = eachDevice
            self.Bind(wx.EVT_MENU, self.OnDeviceSpecific_CDR0,init_menu[eachDevice]['power_up'])
            self.Bind(wx.EVT_MENU, self.OnDeviceSpecific_CDR0, init_menu[eachDevice]['power_up'])
            self.Bind(wx.EVT_MENU, self.OnDeviceSpecific_CDR0_gen25,  cdr0gen25)
            self.Bind(wx.EVT_MENU, self.OnDeviceSpecific_CDR0_fifo25, cdr0fifo25)
            self.Bind(wx.EVT_MENU, self.OnDeviceSpecific_CDR0_ver25,  cdr0ver25)

          if eachDevice.find('M-CDR1') == 0:
            init_menu[eachDevice]['power_up'] = init_menu[eachDevice]['SUBMENU'].Append(wx.ID_ANY,'Initialize on Power-up/Reset',"CDR Reset after Power On GB")
            cdr1gen25  = init_menu[eachDevice]['SUBMENU'].Append(wx.ID_ANY,'Set all 25G lanes to PRBS31 generate'," ")
            cdr1fifo25 = init_menu[eachDevice]['SUBMENU'].Append(wx.ID_ANY,'Set all 25G lanes to FIFO mode'," ")
            cdr1ver25  = init_menu[eachDevice]['SUBMENU'].Append(wx.ID_ANY,'Set all 25G lanes to PRBS31 verify'," ")

            self.menu_device = eachDevice
            self.Bind(wx.EVT_MENU, self.OnDeviceSpecific_CDR1,init_menu[eachDevice]['power_up'])
            self.Bind(wx.EVT_MENU, self.OnDeviceSpecific_CDR1, init_menu[eachDevice]['power_up'])
            self.Bind(wx.EVT_MENU, self.OnDeviceSpecific_CDR1_gen25,  cdr1gen25)
            self.Bind(wx.EVT_MENU, self.OnDeviceSpecific_CDR1_fifo25, cdr1fifo25)
            self.Bind(wx.EVT_MENU, self.OnDeviceSpecific_CDR1_ver25,  cdr1ver25)


        toolsmenu.AppendMenu(wx.ID_ANY,'Show debug hooks',submenu)

        about   = helpmenu.Append(wx.ID_ANY,'About','')

        menuBar = wx.MenuBar()
        menuBar.Append(filemenu ,"&File") # Adding the "filemenu" to the MenuBar
        menuBar.Append(toolsmenu,"&Tools")
        menuBar.Append(init_menu['WXMENU'],"&Device_Init")
        #menuBar.Append(helpmenu ,"&Help")
        
        self.CreateStatusBar()
        self.SetMenuBar(menuBar)  # Adding the MenuBar to the Frame content.
        self.Bind(wx.EVT_MENU, self.OnSaveAsFile, saveAll)
        #self.Bind(wx.EVT_MENU, self.OnToggle_Main, toggleMain)
        self.Bind(wx.EVT_MENU, self.OnButton_quit, quit)
        self.Bind(wx.EVT_MENU, self.OnButton_shell,shell)
        self.Bind(wx.EVT_MENU, self.OnButton_detect,detect)

        self.Bind(wx.EVT_MENU, self.OnButton_about,about)

        self.Bind(wx.EVT_COMBOBOX, self.OnSelectCombo) 
        self.Bind(wx.EVT_TEXT_ENTER, self.OnPressEnter)
        self.Bind(wx.EVT_BUTTON, self.OnAnyButton)
        self.Bind(wx.EVT_IDLE, self.OnFirstIdle)
        self.window_1 = wx.SplitterWindow(self, -1)
        #self.notebook = wx.Notebook(self.window_1, -1, style=0)
        self.notebook = wx.aui.AuiNotebook(self.window_1)
        self.notebook.panel = {}
        self.notebook.pages = {}
        
        for eachDevice in ipvar.System.keys():
            self.notebook.pages[eachDevice] = MyNotebookPage()
            self.notebook.pages[eachDevice].create(self.notebook,eachDevice,self)
            
        #nextTab = 'Sweep'
        #self.notebook.pages[nextTab] = MyNotebookPage()
        #self.notebook.pages[nextTab].create(self.notebook,nextTab,self,blank=1)
        #self.notebook.pages[nextTab].addSweepControls(self,ipvar.CDR_Name)
        
        #nextTab = 'Eye Scan'
        #self.notebook.pages[nextTab] = MyNotebookPage()
        #self.notebook.pages[nextTab].create(self.notebook,nextTab,self,blank=1)
        #self.notebook.pages[nextTab].addEyeScanControls(self,ipvar.CDR_Name)
        
        nextTab = 'Eye Scan(GB2)'
        self.eyeScanPanel = ipeyeGB2.eyeScanPanel(self.notebook, -1)
        self.notebook.AddPage(self.eyeScanPanel,nextTab)       
        nextTab = 'BER - Host'
        self.hostBERPanel = gb2BER.BERPanel(self.notebook,'Host')
        self.notebook.AddPage(self.hostBERPanel,nextTab)        
        nextTab = 'BER - Opt'
        self.optBERPanel = gb2BER.BERPanel(self.notebook,'Opt')
        self.notebook.AddPage(self.optBERPanel,nextTab)   

        nextTab = 'GB2 Tx'
        self.gb2TxPanel = ipGB2Tx.gb2TxPanel(self.notebook,'Host')
        self.notebook.AddPage(self.gb2TxPanel,nextTab)        
        
        #nextTab = 'BER Tool'
        #self.cdrMonitorPanel = ipcdrmon.cdrMonitorPanel(self.notebook, -1)
        #self.notebook.AddPage(self.cdrMonitorPanel,nextTab)
        
        #Check for GearBox in Device List
        self.GB_list = []
        for eachDevice in ipvar.System.keys():
          if (eachDevice.find("GB")==0):
            self.GB_list.append(eachDevice)
        #if len(self.GB_list): 
        #  nextTab = 'CAUI Eye Scan'
        #  self.notebook.pages[nextTab] = MyNotebookPage()
        #  self.notebook.pages[nextTab].create(self.notebook,nextTab,self,blank=1)
        #  self.notebook.pages[nextTab].addCauiEyeScanControls(self,ipvar.CDR_Name)
        
        #  nextTab = 'CAUI BER Tool'
        #  self.cauiMonitorPanel = ipcauimon2.cauiMonitorPanel(self.notebook, -1)
        #  self.notebook.AddPage(self.cauiMonitorPanel,nextTab)
        
        #  nextTab = 'CAUI Tx'
        #  self.cauiTxPanel = ipcauidebug.cauiTxPanel(self.notebook, -1)
        #  self.notebook.AddPage(self.cauiTxPanel,nextTab)
        
        #  if(debug) :
        #    nextTab = 'CAUI Rx'
        #    self.cauiRxPanel = ipcauidebug.cauiRxPanel(self.notebook, -1)
        #    self.notebook.AddPage(self.cauiRxPanel,nextTab)
        
        self.panelUpper = wx.Panel(self.window_1, -1)
        self.PushStatusText("Loading components...")
        
        sizer_1 = wx.BoxSizer(wx.HORIZONTAL)     
          
        self.buttons = []
        self.fieldBoxes = []
        
        #self.refreshScrollWindowArea(self.notebook_pane_4_scroll_panel, self.notebook_pane_4_scroll, self.grid_sizer_4)
                                                            
        self.window_1.SplitHorizontally(self.panelUpper, self.notebook, 100)
        sizer_1.Add(self.window_1, 1, wx.EXPAND, 0)
        self.SetSizer(sizer_1)
        # the background image
        image = wx.Image('../gui/new_logo.jpg', wx.BITMAP_TYPE_JPEG)
        self.bmp1 = wx.StaticBitmap(parent=self.panelUpper,bitmap=image.ConvertToBitmap())
        

        # Create a "Shell button"
        self.shell = None
        runOrigX = 440
        runOrigY = 20
        #headerText = wx.StaticText(self.panelUpper, label = " "*80 + "<script_file>" + " "*50 + "<args>", style=0)
        self.runButton = wx.Button(self.bmp1, -1, "Run Script", size=(80,25), pos=(runOrigX,runOrigY),style=wx.RAISED_BORDER)
        self.runButton.SetToolTipString('Run the script')
        self.stopButton = wx.Button(self.bmp1, -1, "Stop/Go", size=(50,25), pos=(runOrigX+100,runOrigY),style=wx.RAISED_BORDER)
        self.stopButton.SetToolTipString('Flag for the script to stop')
        
        #self.threadBox = wx.CheckBox(panel, -1, label="Separate Thread", size=(-1,20))
        #self.threadBox.SetValue(GlobalDoThread) # parent.doThread)
        # print "Thread CheckBox=%s" % ("True" if GlobalDoThread else "False")
        #wx.EVT_CHECKBOX(self.threadBox, self.threadBox.GetId(), self.OnButton_doThread)

        self.Bind(wx.EVT_BUTTON, self.OnClickRun, self.runButton)
        self.Bind(wx.EVT_BUTTON, self.OnStop, self.stopButton)
        
        #self.runButton.SetDefault()
        self.fileDialog = wx.Button(self.bmp1, -1, "...", size=(20,25), style=wx.RAISED_BORDER,pos=(runOrigX+80,runOrigY))
        self.fileDialog.SetToolTipString('Browse for a script file')
        self.Bind(wx.EVT_BUTTON, self.OnFileDialog, self.fileDialog)
        #if parent.argv: # show the previous script run
        #    scriptTxt = parent.argv[0]
        #    argsTxt = ' '.join(parent.argv[1:])
        #else:
        readX=runOrigX-83
        readY=runOrigY
        self.readAll = wx.Button(self.bmp1, -1, label='Read-All', name='staticText2', size=(80, 22),pos=(readX,readY))
        self.readAll.SetToolTipString('Read all registers')
        self.read2write = wx.Button(self.bmp1, -1, label='Read2Write', name='staticText2', size=(80, 22),pos=(readX,readY+25))
        self.read2write.SetToolTipString('Copy Read bits to Write fields')
        #self.readCONT = wx.Button(self.bmp1, -1, label='Read-CONT', name='staticText2', size=(80, 22),pos=(readX,readY+50))
        #self.readCONT.Enable(False)
        
        #self.reset = wx.Button(self.bmp1, -1, label='H/W Reset', name='staticText2', size=(80, 22),pos=(readX-90,readY+50))
        #self.reset.SetToolTipString('Reset Sequence')
 
          

        self.entryScript = wx.TextCtrl(self.bmp1, -1, value = 'user.py', size=(150,22),
                                   style=wx.RAISED_BORDER|wx.ALIGN_RIGHT,pos=(runOrigX,runOrigY+25))
        self.entryScript.SetToolTipString('Enter the script name')
        self.entryArgs = wx.TextCtrl(self.bmp1, -1, value = '',
                                 size=(150,22), style=wx.RAISED_BORDER,pos=(runOrigX,runOrigY+50))
        self.entryArgs.SetToolTipString('Enter any args the script requires')
        self.entryArgs.Raise()
        self.entryArgs.Refresh()
          
        self.FindCustomButtons()
        
        #x = 10
        #y = 10
        #wx.StaticText(self.panelUpper, -1, label='Load Cal:', name='staticText2', size=(46, 22),pos=(x+3,y+4))
        #self.loadCalNumber = wx.TextCtrl(self.panelUpper, -1, value = '0', size=(35,22),
        #                           style=wx.RAISED_BORDER|wx.ALIGN_RIGHT,pos=(x+50,y))
        #self.loadCal0       = wx.Button(self.panelUpper, -1, label=ipvar.CDR_Name, name='staticText2', size=(42, 22),pos=(x,y+25))
        #self.loadCal1       = wx.Button(self.panelUpper, -1, label=ipvar.CDR1_Name, name='staticText2', size=(42, 22),pos=(x+45,y+25))
        #self.loadCal.Bind(wx.EVT_TEXT_ENTER,self.OnLoadCal)
        
        self.Layout()
        self.Update()
        self.Refresh()
        self.Show()

        self.PushStatusText(" ")

    def FindCustomButtons(self):
      # find all files that start with gui_button_xxx.py
      self.custom_buttons = {}
      #import os
      local_files = os.listdir('.')
      x=220
      y=5
      for eachFile in local_files:
        if eachFile.find('gui_button_') == 0:
          buttonName = eachFile.replace('gui_button_','')
          buttonName = buttonName.replace('.py','')
          print "Installing custom gui button:",buttonName,'at',x,y
          self.custom_buttons[buttonName] = wx.Button(self.bmp1, -1, label=buttonName,size=(-1, 25),pos=(x,y))
          self.custom_buttons[buttonName].SetToolTipString('Run from '+ eachFile)
          self.custom_buttons[buttonName].run = eachFile
          #self.Bind(wx.EVT_BUTTON, execfile(eachFile), self.custom_buttons[buttonName])
          self.Bind(wx.EVT_BUTTON, self.onCustomButton(buttonName), self.custom_buttons[buttonName])
          #self.panelUpper.Refresh()
          y=y+30
          if y > 60:
            y = 10
            x = 350
    def onCustomButton(self,bb):
      print "do it",bb
      #for foundFiles in ['custom_button_28G_bathtub.py','custom_button_plot.py']:
      #  buttonName = foundFiles.replace('custom_button_','')
      #  buttonName = buttonName.replace('.py','')
        
      #  print "Created custom button:",buttonName,"(run from:"+foundFiles+')'

      # self.custom_buttons[buttonName] = wx.Button(self.panelUpper, -1, label=buttonName,size=(-1, 25),pos=(x,y))
      # y = y + 30
      #  self.custom_buttons[buttonName].SetToolTipString('Run from '+ foundFiles)
      #  self.custom_buttons[buttonName].run = foundFiles
        #self.Bind(wx.EVT_BUTTON, execfile(foundFiles), self.custom_buttons[buttonName])

                
      
    def OnButton_quit(self, event):
        sys.exit()

    def OnStop(self,event):
      global stop
      stop = 1
      print "Flagging the Stop signal to routines..."
      
    def OnClickRun(self, event):
    #global GlobalDoThread
        global stop
        stop = 0
        print "Executing the script:", self.entryScript.Value, self.entryArgs.Value
        if self.entryScript.Value==None or self.entryScript.Value=='':
            return
        #scriptAndArgs = self.entryScript.Value + ' ' + self.entryArgs.Value
    #self.parent.argv = scriptAndArgs.split(' ')
        #scriptname = self.entryScript.Value # global for execfile()
    #if GlobalDoThread: # self.parent.doThread:
      # print "Parent: execfile(%s), Executing %s" % (self.entryScript.Value, self.entryArgs.Value)
    #  th = ScriptThread(self.entryScript.Value, self.parent.argv)
    #  th.start()
      # print "Executing ", scriptAndArgs
    #else:
      #reload(ipconfig)
      # namespace = {'wx':wx, 'ipgui':self.parent,
      #              'ipconfig':ipconfig
      #             }
      # print "execfile(%s), argv:" % (self.entryScript.Value)
      #dbug print self.parent.argv
        saveArgs = sys.argv # save
        sys.argv = [self.entryScript.Value] + self.entryArgs.Value.split(' ')
        try:
          execfile(self.entryScript.Value)
        except SystemExit:
          print "The script exited early - but that's OK"
        sys.argv = saveArgs # restore
		#GJL 10/17/2012 - Disable FullSystem Read2Write
        #self.FullSystemRead2Write()

    def OnFileDialog(self, event):
        wildcard = "Python source (*.py)|*.py|" "All files (*.*)|*.*"
        dialog = wx.FileDialog(None, "Choose a file", os.getcwd(), "", wildcard, wx.OPEN)
        if dialog.ShowModal() == wx.ID_OK:
            self.entryScript.Value = dialog.GetPath() 
            dialog.Destroy()
                     
    def OnFirstIdle(self, event):
        if self.InitialScanDone == 0:
          self.InitialScanDone = 1          
          self.FullSystemRead()
          #self.refreshScrollWindowArea(self.notebook_pane_4_scroll_panel, self.notebook_pane_4_scroll, self.grid_sizer_4)
      
    def refreshScrollWindowArea(self, panel, scroll, sizer):
        panel.SetSizerAndFit(sizer)
        panel.SetAutoLayout(True) 
        panel.Layout() 
        panel.Refresh()        
        width, height = panel.GetSize() 
        scroll.SetVirtualSize((width + 20, height + 20)) 
        scroll.SetScrollRate(50, 50) 

    def OnPlotButton(self, event):
      pass

    def OnAnyButton(self, event):
      if event.EventObject.Label.find("Read-All") == 0:
          print "Read-All"
          self.FullSystemRead()
      elif event.EventObject.Label.find("Read2Write") == 0:
          print "Read2Write"
          self.FullSystemRead()
          self.FullSystemRead2Write()
      elif event.EventObject.Label.find("Start Sweep") == 0:   
        mySweep = sweepObj(event.EventObject.controls)
        mySweep.populate(event.EventObject.controls)      
        mySweep.Do(event.EventObject.controls,self) 
      elif event.EventObject.Label.find("Stop Sweep") == 0:
        event.EventObject.controls.stop = 1
        event.Skip()  
      elif event.EventObject.Label.find("Start Eye Scan") == 0:
        self.PushStatusText("Running Eye Scan...")
        self.myEye = ipeye.EyeScan(event.EventObject.controls)
        self.myEye.populate()               
        self.myEye.Do()
        self.PushStatusText("")
      elif event.EventObject.Label.find("Start CAUI Scan") == 0:
        self.PushStatusText("Running Eye Scan...")
        self.myEye = ipcauieye.CauiEyeScan(event.EventObject.controls)
        self.myEye.populate()               
        self.myEye.Do()
        self.PushStatusText("")
         
      elif event.EventObject.Label.find("Stop Eye Scan") == 0:
        self.myEye.stop = 1
      elif event.EventObject.Label.find("Refresh Plot") == 0:
        self.myEye.Refresh()
      elif event.EventObject.Label.find("Save to bmp") == 0:
        self.myEye.saveToBmp()  
      elif event.EventObject.Label.find(ipvar.CDR_Name) == 0:
        self.LoadCal(ipvar.CDR_Name,self.loadCalNumber.GetValue())
      elif event.EventObject.Label.find(ipvar.CDR1_Name) == 0:
        self.LoadCal(ipvar.CDR1_Name,self.loadCalNumber.GetValue())
        
      elif event.EventObject.Label.find("+") == 0:      
        fieldBox().ToggleSubReg(self, hierarchy=event.EventObject.hierarchy,
                                      register=event.EventObject.register,
                                      panel=event.EventObject.panel,
                                      panelSizer=event.EventObject.panelSizer,
                                      regDict=event.EventObject.regDict,
                                      atIndex=event.EventObject.index)
        regDict = event.EventObject.regDict
        self.UpdateWriteTextFromHex(regDict, int(regDict.entryWrite.GetValue(), 16), True)
        self.UpdateComboFromHex(regDict, int(regDict.entryWrite.GetValue(), 16), True)
        self.UpdateReadTextFromHex(regDict, regDict.lastRead, True)
        self.refreshScrollWindowArea(self.notebook.pages[event.EventObject.hierarchy].scrollPanel, self.notebook.pages[event.EventObject.hierarchy].scroll, self.notebook.pages[event.EventObject.hierarchy].sizer)
      elif event.EventObject.Label.find("-") == 0:      
        fieldBox().ToggleSubReg(self, hierarchy=event.EventObject.hierarchy,
                                      register=event.EventObject.register,
                                      panel=event.EventObject.panel,
                                      panelSizer=event.EventObject.panelSizer,
                                      regDict=event.EventObject.regDict,
                                      atIndex=event.EventObject.index)
        regDict = event.EventObject.regDict
        self.UpdateWriteTextFromHex(regDict, int(regDict.entryWrite.GetValue(), 16), True)
        self.UpdateComboFromHex(regDict, int(regDict.entryWrite.GetValue(), 16), True)
        self.UpdateReadTextFromHex(regDict, regDict.lastRead, True)
        self.refreshScrollWindowArea(self.notebook.pages[event.EventObject.hierarchy].scrollPanel, self.notebook.pages[event.EventObject.hierarchy].scroll, self.notebook.pages[event.EventObject.hierarchy].sizer)
      else:   # now go through all the custom buttons and execute the file if one is found
        for eachButton in self.custom_buttons.keys():
          if event.EventObject.Label.find(eachButton) == 0:
            execfile(self.custom_buttons[eachButton].run)
        
    def LoadCal(self,cdrName,dutNumber):
      filePath = r'..\calibration\cal_dut'+dutNumber+'.py'
      print "Load Cal for",cdrName,'DUT',dutNumber, 'from ('+filePath+')...'
      self.entryScript.Value = filePath
      self.entryArgs.Value = 'all'
      self.OnClickRun(None)


    #----------------------------------------------------------------
	#Initialize StartUp 
    def OnDeviceSpecific_GB0(self,evt):  
      startup_dictionary = {"target":"GB0", "28Gdata":"no", "10Gdata":"no","quiet":"yes"}
      execfile("../scripts/startup_gb.py",globals(),startup_dictionary)
      self.FullSystemRead2Write()
      
    def OnDeviceSpecific_GB1(self,evt):  
      startup_dictionary = {"target":"GB1", "28Gdata":"no", "10Gdata":"no","quiet":"yes"}
      execfile("../scripts/startup_gb.py",globals(),startup_dictionary)
      self.FullSystemRead2Write()
	  
    def OnDeviceSpecific_CDR0(self,evt): 
      startup_dictionary = {"target":"CDR0", "28Gdata":"no", "quiet":"yes"}
      execfile("../scripts/startup_cdr.py",globals(),startup_dictionary)
      self.FullSystemRead2Write()	
	  
	  
    def OnDeviceSpecific_CDR1(self,evt): 
      startup_dictionary = {"target":"CDR1", "28Gdata":"no", "quiet":"yes"}
      execfile("../scripts/startup_cdr.py",globals(),startup_dictionary)
      self.FullSystemRead2Write()	  
	  
    #----------------------------------------------------------------
    # 25/28G PRBS31    
    def OnDeviceSpecific_GB0_gen25(self,evt):  
      device='GB0'
      self.gen25(device)
    def OnDeviceSpecific_GB1_gen25(self,evt):  
      device='GB1'
      self.gen25(device)

    def OnDeviceSpecific_CDR0_gen25(self,evt):   
      device='CDR0'
      self.gen25(device)
    def OnDeviceSpecific_CDR1_gen25(self,evt):   
      device='CDR1'
      self.gen25(device)
	  
    def gen25(self,device='GB0'):    
      model = regRead(device+'::30.3')
      print 'Model/Rev: 0x%x' % model
      #7400/7410 - A0
      #7401/7411 - B0
      if ((model&0xffef) == 0x7401):
        isB0 = True
      else:
        isB0 = False
		
      for i in range(4):
	    #Pattern Verification
        regWrite(device+"::30."+str(16+i)+".12:8",0x14)
		
		#Pattern Generation
        regWrite(device+"::30."+str(16+i)+".6:0",0x14) #[6] TxDisable=0, [4]= Pattern Enable=1
		#Set Swing to 50% for B0
    #if(isB0):
    #      print 'B0 Detected - Tx%d Amplitude set to 50 percent'%i
    #      regWrite(device+"::30."+str(258+256*i)+".2:0",0x2)
          
      self.FullSystemRead2Write()
    #----------------------------------------------------------------
    # 25/28G FIFO
    def OnDeviceSpecific_GB0_fifo25(self,evt):     
      self.fifo25('GB0')
    def OnDeviceSpecific_GB1_fifo25(self,evt):     
      self.fifo25('GB1')
	  
    def OnDeviceSpecific_CDR0_fifo25(self,evt):   
      self.fifo25('CDR0')	    
    def OnDeviceSpecific_CDR1_fifo25(self,evt):   
      self.fifo25('CDR1')	

   
    def fifo25(self,device='GB0'):
      for i in range(0,4):
        regWrite(device+"::30."+str(16+i)+".6:0",0)
      self.FullSystemRead2Write()  
    #----------------------------------------------------------------
    # 25/28G Auto-verify      
    def OnDeviceSpecific_GB0_ver25(self,evt):   
      device='GB0'
      self.autover25(device)      
    def OnDeviceSpecific_GB1_ver25(self,evt):   
      device='GB1'
      self.autover25(device)

    def OnDeviceSpecific_CDR0_ver25(self,evt):
	  device='CDR0'
	  self.autover25(device)
    def OnDeviceSpecific_CDR1_ver25(self,evt):
	  device='CDR1'
	  self.autover25(device)
	  
    #Sets bit 14-autover, 12-enable checker
    def autover25(self,device='GB0'):
      for i in range(0,4):
        regWrite(device+"::30."+str(16+i)+".14:14",1)
        regWrite(device+"::30."+str(16+i)+".12:12",1)
      self.FullSystemRead2Write()  

    #----------------------------------------------------------------
    # CAUI PRBS31 
    def OnDeviceSpecific_GB0_gen10(self,evt):   
      device='GB0'
      self.gen10(device)
    def OnDeviceSpecific_GB1_gen10(self,evt):   
      device='GB1'
      self.gen10(device)      
      
    def gen10(self,device='GB0'):
      for i in range(0,10):
        regWrite(device+"::30."+str(20+i)+".12:8",0x14)
        regWrite(device+"::30."+str(20+i)+".4:0",0x14)
        regWrite(device+"::30."+str(1552+i)+".15:0",0x53b8)
      self.FullSystemRead2Write()
    #----------------------------------------------------------------
    # 10G FIFO     
	  
    def OnDeviceSpecific_GB0_fifo10(self,evt):   
      self.fifo10('GB0')
    def OnDeviceSpecific_GB1_fifo10(self,evt):   
      self.fifo10('GB1')
     	  
    def fifo10(self,device='GB0'):
      for i in range(0,10):
        regWrite(device+"::30."+str(20+i)+".4:0",0)
      self.FullSystemRead2Write()  
      
    #----------------------------------------------------------------  
    def OnButton_detect(self, evt):
      print 'Running Device Detect...'
      for PRTAD in range(32):
        for DEVAD in range(32):
          for regaddr in [2,3]:
            val = ipmdio.mdio_rd(PRTAD,DEVAD,regaddr)
            #print 'PRTAD',PRTAD,'DEVAD',DEVAD,'addr',addr,'=',val
            if regaddr == 2 and val == 0x0210:
              print '-'*60
              print 'Found Inphi manufacturer ID   ('+str(hex(val))+') at PRTAD',PRTAD,'DEVAD',DEVAD
            elif regaddr == 3 and val == 0x7410:
              print 'Found Inphi Gearbox A0 ID    ('+str(hex(val))+') at PRTAD',PRTAD,'DEVAD',DEVAD              
            elif regaddr == 3 and val == 0x7411:
              print 'Found Inphi Gearbox_B0 ID    ('+str(hex(val))+') at PRTAD',PRTAD,'DEVAD',DEVAD   
            elif regaddr == 3 and val == 0x7400:
              print 'Found Inphi CDR A0 ID        ('+str(hex(val))+') at PRTAD',PRTAD,'DEVAD',DEVAD    
            elif regaddr == 3 and val == 0x7401:
              print 'Found Inphi CDR B0 ID        ('+str(hex(val))+') at PRTAD',PRTAD,'DEVAD',DEVAD  
            elif val != 65535 and regaddr == 2:
              print 'Unknown device manufacture ID ('+str(val)+') found at PRTAD',PRTAD,'DEVAD',DEVAD
            elif val != 65535 and regaddr == 3:
              print 'Unknown device type           ('+str(val)+') found at PRTAD',PRTAD,'DEVAD',DEVAD
      print 'Detect Complete'          
    def OnButton_about(self, evt):
      print "Not implemented yet"
              
    def OnButton_shell(self, evt):
#      if self.shell!=None:
        # if it already exists then just make sure it's visible
      #s = self.shell
#        if s.IsIconized():
#            s.Iconize(False)
#            s.Raise()
#      else:
        # Make a PyShell window
      from wx import py
      # reload(iputils)
      namespace = {'wx'      : wx,
                  'ipallreg' : self,
                  'system'   : system,
                  'ipvat'    : ipvar,
                  'ipallreg' : self,
                  }
      self.shell = py.shell.ShellFrame(None, locals=namespace)
      self.shell.SetSize((640, 480))
      self.shell.Show()

    # Hook the close event of the main frame window so that we
    # close the shell at the same time if it still exists            
    def CloseShell(self, evt):
      if self.shell:
        self.shell.Close()
        evt.Skip()
        #self.Bind(wx.EVT_CLOSE, CloseShell)
        
    def OnSelectCombo(self, event):
      if event.EventObject.Name.find('rateCombo')==0:
        event.Skip()
      else:
        if event.EventObject.subregOrdDict.bitMask.find("W") > -1:  # This was selected from the ComboBox
          comboStr = event.EventObject.GetValue()
          #print "I see",comboStr
          for eachKey in event.EventObject.subregOrdDict.purposeTab:
            #if event.EventObject.subregOrdDict.purposeTab[eachKey].find(comboStr)==0:
            if event.EventObject.subregOrdDict.purposeTab[eachKey] == comboStr:

              #print "found combostr",comboStr,"as",eachKey,"not",event.GetSelection()
              realVal = eachKey
          event.EventObject.subregOrdDict.entryWrite.SetValue(bin(realVal)[2:].zfill(event.EventObject.subregOrdDict.nBits))
          regPassing = event.EventObject.hierarchy + '::' + event.EventObject.subreg
          valPassing = realVal 
          regWrite(regPassing, valPassing)        
          self.UpdateHexFromText(event.EventObject.subregOrdDict, True)
          self.FullSystemRead()

    def OnPressEnter(self, event):
      if hasattr(event.EventObject, 'subreg'):  # Then we are in a subreg field
        if event.EventObject.subregOrdDict.bitMask.find("W") > -1:
            #event.EventObject.subregOrdDict.combo.Selection = int(event.String, 2)
            if int(event.String, 2) in event.EventObject.subregOrdDict.purposeTab:
              event.EventObject.subregOrdDict.combo.SetValue(event.EventObject.subregOrdDict.purposeTab[int(event.String, 2)])
            else:
              event.EventObject.subregOrdDict.combo.SetValue('')

            #self.UpdateTextFromCombo()
            regPassing = event.EventObject.hierarchy + '::' + event.EventObject.subreg
            regWrite(regPassing, int(event.String, 2))         
            self.UpdateHexFromText(event.EventObject.subregOrdDict)
            self.FullSystemRead()
            
      else:
        # hasattr(event.EventObject.parent, 'regAddr'): # This is the 16b hex value entered
        if event.EventObject.isRead == 1:
            regPassing = event.EventObject.parent.hierarchy + '::' + event.EventObject.parent.register
            valRead = regRead(event.EventObject.parent.hierarchy + '::' + event.EventObject.parent.register)
            event.EventObject.parent.lastRead = valRead
            self.UpdateComboFromHex(event.EventObject.parent, valRead, True)
        else:  
            regPassing = event.EventObject.parent.hierarchy + '::' + event.EventObject.parent.register
            valPassing = int(event.String, 16)
            event.EventObject.parent.lastWrite = valPassing
            self.UpdateComboFromHex(event.EventObject.parent, valPassing, True)
            self.UpdateWriteTextFromHex(event.EventObject.parent, valPassing, True)
            valRead = regWriteAndRead(event.EventObject.parent.hierarchy + '::' + event.EventObject.parent.register, int(event.String, 16))

        event.EventObject.parent.lastRead = valRead
        self.UpdateReadTextFromHex(event.EventObject.parent, valRead, True)
                                      # This is a subReg entered with the writeEntry Textbox
    def UpdateHexFromCombo(self):
      print "Update Hex From Combo not implemented yet!"

    def HardwareResetAll(self):
      print "Performing hardware reset..."
      for eachDevice in ipvar.System.keys(): 
        print "Reseting %s..." % (eachDevice)
        regWrite(eachDevice + '::30.0','0x0000')
        self.FullSystemRead()
        regWrite(eachDevice + '::30.0','0x00a0')
        self.FullSystemRead()
        regWrite(eachDevice + '::30.0','0x0000') 
        self.FullSystemRead()     
      self.PushStatusText("")  
      self.Update()
      
    def FullSystemRead(self):
      self.PushStatusText("Running full scan...")
      for eachDevice in ipvar.System.keys():
        if eachDevice.find('M-')<0:
          for eachRegisterKey in ipvar.System[eachDevice].regDefTab.keys():
            valRead = regRead(eachDevice + '::' + eachRegisterKey)
            ipvar.System[eachDevice].regDefTab[eachRegisterKey].lastRead = valRead
            self.UpdateReadTextFromHex(ipvar.System[eachDevice].regDefTab[eachRegisterKey], valRead, True)
            self.UpdateComboFromHex(ipvar.System[eachDevice].regDefTab[eachRegisterKey], valRead, True)

      self.PushStatusText("")  
      self.Update()
        
    def FullSystemRead2Write(self):
      self.PushStatusText("Running full scan...")
      for eachDevice in ipvar.System.keys():
        if eachDevice.find('M-')<0:      
          for eachRegisterKey in ipvar.System[eachDevice].regDefTab.keys():
            valRead = regRead(eachDevice + '::' + eachRegisterKey)
            ipvar.System[eachDevice].regDefTab[eachRegisterKey].lastRead = valRead
            self.UpdateReadTextFromHex(ipvar.System[eachDevice].regDefTab[eachRegisterKey], valRead, True)

            regWrite(eachDevice + '::' + eachRegisterKey,valRead)
            ipvar.System[eachDevice].regDefTab[eachRegisterKey].lastWrite = valRead
            self.UpdateWriteTextFromHex(ipvar.System[eachDevice].regDefTab[eachRegisterKey], valRead, True)
            self.UpdateComboFromHex(ipvar.System[eachDevice].regDefTab[eachRegisterKey], valRead, True)
            ipvar.System[eachDevice].regDefTab[eachRegisterKey].entryWrite.parent.entryWrite.SetValue(hex(valRead)[2:].zfill(4))
      self.PushStatusText("")  
      self.Update()


    def UpdateHexFromText(self, eventObject, doMain=False):  # the eventObject here is the subreg regDefTab object
      from iprtab import update_reg # rb
      s = eventObject.lsbStart
      n = eventObject.nBits
      lastBin = bin(eventObject.entryWrite.parent.lastWrite)[2:].zfill(16) + 'x'
      myBin = lastBin[:16 - s - n] + eventObject.entryWrite.Label + lastBin[16 - s:]
      #print "s=",s,"n=",n,"lastBin=",lastBin,myBin, hex(int(myBin[:-1],2))
      myLastWrite = int(myBin[:-1], 2)
      eventObject.entryWrite.parent.entryWrite.SetValue(hex(myLastWrite)[2:].zfill(4))
      # This does the "lastWrite" action. This is probably better done INSIDE regWrite in future
      eventObject.entryWrite.parent.lastWrite = myLastWrite

      return(myLastWrite)

    def UpdateReadTextFromHex(self, regDict, valPassing, doMain=False):
      from iprtab import update_reg # rb
      #mySize = regDict.entryRead.GetSize()
      regDict.entryRead.SetLabel(hex(valPassing)[2:].zfill(4))
      #regDict.entryRead.SetSize(mySize)
      if regDict.expanded == 1:
        for eachSubReg in regDict.regSubTab:
          n = regDict.regSubTab[eachSubReg].nBits
          s = regDict.regSubTab[eachSubReg].lsbStart
          myBin = bin(valPassing)[2:].zfill(16) + 'x'
          valueToSet = myBin[16 - s - n:16 - s]
          #print "before",regDict.regSubTab[eachSubReg].entryRead.GetSize()
          regDict.regSubTab[eachSubReg].entryRead.SetLabel(valueToSet)
          #print "after",regDict.regSubTab[eachSubReg].entryRead.GetSize()

    def UpdateWriteTextFromHex(self, regDict, valPassing, doMain=False):
      from iprtab import update_reg # rb
      regDict.entryWrite.SetLabel(hex(valPassing)[2:].zfill(4)) # rb
      if regDict.expanded == 1:
        for eachSubReg in regDict.regSubTab:
          n = regDict.regSubTab[eachSubReg].nBits
          s = regDict.regSubTab[eachSubReg].lsbStart
          myBin = bin(valPassing)[2:].zfill(16) + 'x'                     
          valueToSet = myBin[16 - s - n:16 - s]
          regDict.regSubTab[eachSubReg].entryWrite.SetLabel(valueToSet)

   
    def UpdateComboFromText(self):
      pass
    
    def UpdateComboFromHex(self, regDict, valPassing, doMain=False):
      if regDict.expanded == 1: 
        for eachSubReg in regDict.regSubTab:
          if regDict.regSubTab[eachSubReg].bitMask.find("W") > -1:         
            myBin = bin(regDict.lastWrite)[2:].zfill(16) + 'x' 
            #myBin = bin(valPassing)[2:].zfill(16) + 'x'
            a = regDict.regSubTab[eachSubReg].nBits
            b = regDict.regSubTab[eachSubReg].lsbStart                    
            valueToSet = myBin[16 - b - a:16 - b]                        
            #regDict.regSubTab[eachSubReg].combo.SetValue(regDict.regSubTab[eachSubReg].purposeTab[str(valueToSet)])
            if int(valueToSet,2) in regDict.regSubTab[eachSubReg].purposeTab:
              regDict.regSubTab[eachSubReg].combo.SetValue(regDict.regSubTab[eachSubReg].purposeTab[int(valueToSet,2)])
            else:
              regDict.regSubTab[eachSubReg].combo.SetValue('')

          else:
            myBin = bin(regDict.lastRead)[2:].zfill(16) + 'x' 
            a = regDict.regSubTab[eachSubReg].nBits
            b = regDict.regSubTab[eachSubReg].lsbStart                    
            valueToSet = myBin[16 - b - a:16 - b]   
            #regDict.regSubTab[eachSubReg].combo.Selection = int(valueToSet, 2)
            if int(valueToSet,2) in regDict.regSubTab[eachSubReg].purposeTab:
              regDict.regSubTab[eachSubReg].combo.SetValue(regDict.regSubTab[eachSubReg].purposeTab[int(valueToSet,2)])
            else:
              regDict.regSubTab[eachSubReg].combo.SetValue('')


#    def UpdateAllRegs(self, doTgt='RW', doMain=False):
#      for device in ipvar.System.values():        
#        for regDef in device.regDefTab.values():
#          if doTgt=='R':
#            frameAR.UpdateReadTextFromHex(regDef, regDef.lastRead, doMain)
#          elif doTgt=='W':
#            frameAR.UpdateWriteTextFromHex(regDef, regDef.lastWrite, doMain)
#            frameAR.UpdateComboFromHex(regDef, regDef.lastWrite, doMain)
#          else:
#            frameAR.UpdateReadTextFromHex(regDef, regDef.lastRead, doMain)
#            frameAR.UpdateWriteTextFromHex(regDef, regDef.lastWrite, doMain)
#            frameAR.UpdateComboFromHex(regDef, regDef.lastWrite, doMain)

    def OnButtonClick(self, event):
        print 'You clicked the ' + event.EventObject.Name + event.EventObject.Label

    def OnTest(self,event):
      print "Just Testing!"
      for child in self.notebook.pages['CDR0'].panel.GetChildren():
        child.Destroy()
        #time.sleep(4)
      self.notebook.pages['CDR0'].update()
      
    def OnSaveAsFile(self, event):
      wcd='*.py'
      dir = os.getcwd()
      save_dlg = wx.FileDialog(self, message='Save file as...', defaultDir=dir, defaultFile='',
                      wildcard=wcd, style=wx.SAVE | wx.OVERWRITE_PROMPT)
      if save_dlg.ShowModal() == wx.ID_OK:
          path = save_dlg.GetPath()

          try:
              file = open(path, 'w')
              #text = self.text.GetValue()
              #file.write(text)
              #file.close()
              #self.last_name_saved = os.path.basename(path)
              #self.statusbar.SetStatusText(path + ' saved', 0)
              #self.modify = False
              #self.statusbar.SetStatusText('', 1)
     
              file.write("from ipallreg import regWrite\n")
              for eachDevice in ipvar.System.keys():
                for eachRegisterKey in ipvar.System[eachDevice].regDefTab.keys():
                  valRead = regRead(eachDevice + '::' + eachRegisterKey)
                  ipvar.System[eachDevice].regDefTab[eachRegisterKey].lastRead = valRead
                  self.UpdateReadTextFromHex(ipvar.System[eachDevice].regDefTab[eachRegisterKey], valRead, True)
                  self.UpdateComboFromHex(ipvar.System[eachDevice].regDefTab[eachRegisterKey], valRead, True)
                  file.write("regWrite('%s',0X%04X)\n" % (eachDevice + '::' + eachRegisterKey,valRead))
        
              self.PushStatusText("File "+path+" saved.")  
              self.Update()
              file.close()
              
          except IOError, error:
              dlg = wx.MessageDialog(self, 'Error saving file\n' + str(error))
              dlg.ShowModal()
      save_dlg.Destroy()
     
    def OnUnHide(self, e):
        """ Open a file"""
        print "Trying to hide something..."
        print self.buttons[0].Show(True)
        self.newobj1.boxSizer.Layout()
  #----------------------------------------------------

#    def OnToggle_Main(self, event):
#        import ipvar
#        try:
#          frame = ipvar.frame
#          if event.Checked() and frame.IsShownOnScreen():
#            frame.Show(False)
#          elif (not event.Checked()) and (not frame.IsShownOnScreen()):
#            from iprtab import update_device
#            frame.Show(True)
#            update_device(frame.nb.regAddrXref, 'RW', False)
#        except:
#          pass

class MyApp(wx.App):
  '''Our application class
  '''
  def OnInit(self):
    '''Initialize by creating the split window with the tree
    '''
    frameAR = MyFrame(None, -1, 'Inphi MDIO System Controller', system)
    frameAR.Show(True)
    self.SetTopWindow(frameAR)
    return True

def run():
  #print "Stated with args:",sys.argv[1:]
 
  app = MyApp(0)
  app.MainLoop()

if __name__ == '__main__':
  print "******************************"
  print "***  iphy-gui Version 1.09 ***"
  print "***                        ***"
  print "***  Inphi Corp. (c) 2012  ***"
  print "******************************"
  print " "
  system = iputils.create_system()
  run()


