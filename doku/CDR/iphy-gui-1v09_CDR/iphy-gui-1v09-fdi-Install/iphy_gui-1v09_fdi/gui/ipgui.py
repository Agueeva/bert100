# ------------------------------------------------------------------------------
#
# $RCSfile: validation#python#ipgui.py,v $ - Top-level GUI module for iPHY program
#
# Disclaimer:
# Inphi Confidential
# Copyright(c) Inphi Corp. 2011
#
# All rights reserved.
# This is unpublished, confidential Inphi proprietary information.
# Do not reproduce or redistribute without written permission.
#
# $Header: /projects/wlv/iphy/sos/repository/iphy_A.rep/LIBS/FILES258/SRC/validation#python#ipgui.py,v 1.6 2011-07-17 18:02:55-07 rbemra Exp $
# $Log: validation#python#ipgui.py,v $
# Revision 1.6  2011-07-17 18:02:55-07  rbemra
# More updates to Main & AllRegs frames to keep in sync.
# Added Show/Hide item to Menu->File in both frames.
# Fixed Alt+X, Ctrl+H in Main Menu that had typos from iMB
#
# Revision 1.5  2011-07-14 16:43:41-07  rbemra
# Installed event-handlers for Main window's tabs
# Updated AllRegisters to handshake w/ Main window updates
#
# Revision 1.4  2011-07-11 13:42:15-07  rbemra
# Used ordict to ensure reg. tabs created in sequential order
#
# Revision 1.3  2011-07-11 12:28:20-07  rbemra
# Updates for real iPHY registers (tabbed pages & all registers frame)
#
# Revision 1.2  2011-07-05 18:43:11-07  rbemra
# Updates to get more things working: user scripts w/ options/threading, DDF+CSV writers, MDIO R/W updates, ipallreg addition
#
# Revision 1.1  2011-07-01 09:02:56-07  rbemra
# First, prelim. runnable version
#
# ------------------------------------------------------------------------------

import os
import sys
import time
from types import *
import string,re
import wx
import wx.combo
import wx.lib.colourdb
import wx.lib.delayedresult as delayedresult
from wxPython.wx import *
from ordict import *
# from ftdi import FTDI

import threading
import ctypes
w32 = ctypes.windll.kernel32
THREAD_TERMINATE = 1    # Privilege level for termination

import ipreg
import ipvar
import iputils
import ipconfig
import ipuser
import ipallreg
import iprtab

configAlso = False
ding = None
scriptname = None
threadList=[]
GlobalDoThread = False

#var.TopDir = os.pardir

import  thread
import  wx
import  wx.lib.newevent

# import usrPat

delayLines = [12, 0, 0, 0, 0, 0, 0, 0]
#----------------------------------------------------------------------

# This creates a new Event class and a EVT binder function
(UpdateTextEvent, EVT_REFRESH_RT_DISPLAY) = wx.lib.newevent.NewEvent()


class onTick_Thread:
  def __init__(self,win):
    self.keepGoing = self.running = False
      
  def Start(self):
    self.keepGoing = self.running = True
    thread.start_new_thread(self.ioThread, ())

  def Stop(self):
    self.keepGoing = False

  def IsRunning(self):
    return self.running

  def ioThread(self):
    global cntr
    cntr=0
    while self.keepGoing:
        print"cntr=",cntr
        cntr+=1
        dispData=['$$$$$$','@@@@@@@','!!!!!!!','%%%%%%','&&&&&&','*******','######','^^^^^^']
        evt = UpdateTextEvent(dispData)

        wx.PostEvent(self.win, evt)
        time.sleep(0.2)
    self.running = False
#----------------------------------------------------------------------

class DelaySlider(wx.Slider):
  def __init__(self, *args, **kw):
    wx.Slider.__init__(self, *args, **kw)
    wx.EVT_SCROLL(self, self.OnScroll)
    self.SetPageSize(10)
    self.prevValue = self.GetValue()
    
  def OnScroll(self, event):
    value = self.GetValue()
    if value==self.prevValue: return
    self.prevValue = value
    ipvar.frame.stDelay.SetLabel("Delay %.02f ns"%(value/100.0))
    iputils.Delay(value)
#----------------------------------------------------------------------

class ipDelaySlider(wx.Slider):
  def __init__(self, *args, **kw):
    wx.Slider.__init__(self, *args, **kw)
    wx.EVT_SCROLL(self, self.OnScroll)
    self.SetPageSize(10)
    self.prevValue = self.GetValue()
    self.SetValue(0)
  def setLabel(self, label):
    self.label = label
    value = self.GetValue()
    self.label.SetLabel("%.02f ns"%(value/100.0))

  def OnScroll(self, event):
    value = self.GetValue()
    if value==self.prevValue: return
    self.prevValue = value
    self.label.SetLabel("%.02f ns"%(value/100.0))
    iputils.Delay882(value)
    ipvar.ip882Delay = value
#----------------------------------------------------------------------

class ipMiscDelaySlider(wx.Slider):
  def __init__(self, *args, **kw):
    wx.Slider.__init__(self, *args, **kw)
    wx.EVT_SCROLL(self, self.OnScroll)
    self.SetPageSize(10)
    self.prevValue = self.GetValue()
    self.SetValue(0)

  def setLabel(self, label):
    self.label = label
    value = self.GetValue()
    self.label.SetLabel("%.02f ns"%(value/200.0))

  def OnScroll(self, event):
    value = self.GetValue()
    if value==self.prevValue: return
    self.prevValue = value
    #print self.ID,value
    if self.ID=="DQS": ipvar.DQSDelay = value
    if self.ID=="882": ipvar.DQDelay = value
    if self.ID=="Y0":  ipvar.Y0Delay = value
    if self.ID=="Y1":  ipvar.Y1Delay = value
    iputils.DQS_Yx_Delay()
    #self.label.SetLabel("%.02f ns"%(value/200.0))
#----------------------------------------------------------------------

class ScriptThread(threading.Thread):
  def __init__(self, pyScript, pyArgs):
    threading.Thread.__init__(self)
    self.runArgs = pyArgs # scriptName.py arg1 arg2 ...
    self.scriptName = pyScript # just scriptName.py
    self.setDaemon(1)

  def run(self):
    self.tid = w32.GetCurrentThreadId()
    if self.scriptName==None: return
    saveArgs = sys.argv
    sys.argv = self.runArgs
    # print "Thread execfile(%s) argv=:" % self.scriptName
    # print sys.argv
    execfile(self.scriptName)
    sys.argv = saveArgs

  def kill_thread(threadobj):
    handle = w32.OpenThread(THREAD_TERMINATE, False, threadobj.tid)
    result = w32.TerminateThread(handle, 0)
    w32.CloseHandle(handle)
    return result
#---------------------------------------------------------------------------

class AbortDlg(wx.Dialog):
  def __init__(self, parent, th):
    wx.Dialog.__init__(self, parent=parent, title='Running',
                     size=wx.Size(220,100), style=wx.DEFAULT_DIALOG_STYLE)
    self.th = th
    self.CreateDlg()
    self.CentreOnParent(wx.BOTH)
    self.didAbort = False

  def CreateDlg(self):
    wx.StaticText(self, -1, scriptname,
                pos=(0, 10), size=(220,-1),
                style=wx.ALIGN_CENTRE|wx.ST_NO_AUTORESIZE)
    self.btn = wx.Button(self,-1,"Abort",pos=wx.Point(65,35))
    self.Bind(wx.EVT_BUTTON, self.OnAbort, self.btn)

  def OnAbort(self, event):
    kill_thread(self.th)
    print "Script aborted!"
    ding.Play()
    self.didAbort = True
#----------------------------------------------------------------------

def is_cmd_arg(option):
  from sys import argv
  try:
    isArg = True if (argv!=None and argv.index(option)>0) else False
  except:
    isArg = False
  else:
    pass
  return isArg
#----------------------------------------------------------------------

def wx_inspect(turnOn):
  wxInspect = is_cmd_arg('+wi')
  if wxInspect and turnOn:
    import wx.lib.inspection
    wx.lib.inspection.InspectionTool().Show()
  return wxInspect
#----------------------------------------------------------------------

class ipgui(wx.Frame):
  OnIdleEvent=0
    
  def postIdleEvent(self):
    wx.PostEvent(ipgui, self.OnIdleEvent)  
  
  def __init__(self, parent):
    global GlobalDoThread
    self.doThread = GlobalDoThread = False
    self.argv = None # Script & Script Rerun
    self.shell = None
    self.InitCtrls(parent)
    self.icon = wx.Icon('../gui/inphi.ico', wx.BITMAP_TYPE_ICO)
    self.SetIcon(self.icon)
    self
  def InitCtrls(self, prnt):
    button = {}
    #                                Tab      Size Col   Row  Mod (optional)
    button['UFunc1']            = [ 'Bottom',  'h', 0.00, 0 ]
    button['UFunc2']            = [ 'Bottom',  'h', 0.50, 0 ]
    button['UFunc3']            = [ 'Bottom',  'h', 1.00, 0 ]
    button['UFunc4']            = [ 'Bottom',  'h', 1.50, 0 ]
    button['UFuncA']            = [ 'Bottom',  'h', 0.00, 1 ]
    button['UFuncB']            = [ 'Bottom',  'h', 0.50, 1 ]
    button['UFuncC']            = [ 'Bottom',  'h', 1.00, 1 ]
    button['UFuncD']            = [ 'Bottom',  'h', 1.50, 1 ]
    button['Shell']             = [ 'Bottom',  'h', 2.00, 0 ]
    button['Quit']              = [ 'Bottom',  'h', 2.50, 0 ]

    FM_XSIZE = 660
    FM_YSIZE = 720
    wx.Frame.__init__(self, name='', parent=prnt,
                   pos=(20, 0), size=(FM_XSIZE, FM_YSIZE),
                   style=wx.DEFAULT_FRAME_STYLE,
                   title='Inphi iPHY')
    Root = wx.Panel(parent=self, size=(FM_XSIZE, FM_YSIZE), pos=(0, 0), name="Root")

    image = wx.Image('../gui/logo.jpg', wx.BITMAP_TYPE_JPEG)
    self.bmp1 = wx.StaticBitmap(parent=Root,bitmap=image.ConvertToBitmap())

    Top = wx.Panel(parent=Root,size=(300,120),pos=(420,0), name="Top")

    self.nb = wx.Notebook(parent=Root, size=(640,420), pos=(0,150), name="Notebook")
    self.nb.regAddrXref = {} # key=['CDR0::30.16'], v=iprtab.RegXref
    #rb self.Bind(wx.EVT_IDLE, self.OnUIdleEvent)
    wx.EVT_NOTEBOOK_PAGE_CHANGED(self.nb, self.nb.GetId(), self.OnPageChanged)

    Bottom = wx.Panel(parent=Root,size=(400,60),pos=(10,580), name="Bottom")
    #
    # Read myregs.py if exists, else read ipgroup.py. Allow empty file
    # 
    try:
      import myregs as reginc
    except:
      import ipgroup as reginc
    try:
      devTab = reginc.DeviceTabs
    except:
      devTab = []

    regTabs = OrderedDict()
    for (devId, devRegTab) in devTab:
      try:
        devDef = ipvar.System[devId]
      except:
        pass # skip this device
      else:
        for grpNameList in devRegTab:
          pageId = devId + ':' + grpNameList[0]
          pageTab = iprtab.MyRegTab(self.nb, pageId, ipvar.System[devId], grpNameList[1])
          self.nb.AddPage(pageTab, pageId)

    Misc = wx.Panel(self.nb, -1, name="Misc")
    self.nb.AddPage(Misc, "Misc")
    # Make a menu bar
    self.mainmenu = wx.MenuBar()

    # Make the File menu
    menu = wx.Menu()
    item = menu.Append(wx.NewId(),'&Redirect Output',
                       'Redirect print statements to a window',
                       wx.ITEM_CHECK)
    wx.EVT_MENU(self, item.GetId(), self.OnToggleRedirect)
 
    item = menu.Append(wx.NewId(),'&AllRegs\tAlt+A', 'Show All Regs', wx.ITEM_CHECK)
    # item = menu.AppendCheckItem(wx.NewId(),'&AllRegs\tAlt+A', 'Show All Regs')
    wx.EVT_MENU(self, item.GetId(), self.OnToggle_AllRegs)

    item = menu.Append(wx.NewId(),'&Exit\tAlt+X', 'Exit application')
    wx.EVT_MENU(self, item.GetId(), self.OnButton_quit)

    wx.App_SetMacExitMenuItemId(wx.NewId())

    self.mainmenu.Append(menu, '&File')
    
    # Make the Help menu
    menu = wx.Menu()
    item = menu.Append(wx.NewId(), '&About\tCtrl+H', 'About iPHY')
    wx.EVT_MENU(self, item.GetId(), self.OnHelpAbout)
    wx.App_SetMacAboutMenuItemId(item.GetId())

    self.mainmenu.Append(menu, '&Help')
    self.SetMenuBar(self.mainmenu)

    # Make the status bar
    self.statusBar = self.CreateStatusBar(1, wx.ST_SIZEGRIP)

    originX = 10
    originY = 10

    # make the Script Button
    self.button_script = wx.Button(Top, label="Script",
                           pos=(5, 5), size=(75,-1), name = "Script")
    wx.EVT_BUTTON(self.button_script, self.button_script.GetId(), self.OnButton_script)

    # make the script rerun Button
    self.button_rerun = wx.Button(Top, label="Script Rerun",
                          pos=(5, 30), size=(150,-1), name = "button_rerun")
    wx.EVT_BUTTON(self.button_rerun, self.button_rerun.GetId(), self.OnButton_rerun)
    if ipvar.configurationFile=="": self.button_rerun.Enable(0)

    # Make the Temperature combo box
    temperatures = []
    for t in range (-50, 30, 10): temperatures.append("%dC"%t)
    temperatures.append("25C")
    for t in range (30, 60, 10): temperatures.append("%dC"%t)
    temperatures.append("55C")
    for t in range (60, 130, 10): temperatures.append("%dC"%t)
    temperatures.append("125C")
    self.buttonVoltageTemp = wx.ComboBox(choices=temperatures,
                               value='25C', name='Temp', parent=Top,
                               pos=(5, 60), size=(75, 20),
                               style=wx.CB_DROPDOWN|wx.CB_READONLY)
    wx.EVT_TEXT(self.buttonVoltageTemp, self.buttonVoltageTemp.GetId(), iputils.OnButton_Temp)

    # Make the "Title" static text
    self.stTitle = wx.StaticText(Top, size=(170, -1),
                               pos=(385/2-150/2, originY+25*0+8),
                               style=wx.ALIGN_CENTER|wx.ST_NO_AUTORESIZE)
                               
    # Make the "Ext Clk" checkbox
    self.cbConfig = wx.CheckBox(Top, label="Ext Clk",
                                  pos=(150, 5), size=(-1, 20))
    self.cbConfig.SetValue(configAlso)
    wx.EVT_CHECKBOX(self.cbConfig, self.cbConfig.GetId(), self.OnButton_cbConfig)

    # Add debug mode
    self.cbDebugMode = wx.CheckBox(Misc, label="Debug mode",
                                 pos=(originX+125*0, originY+25*0),
                                 size=(-1, 20))
    wx.EVT_CHECKBOX(self.cbDebugMode, self.cbDebugMode.GetId(), self.OnButton_cbDebugMode)

  # Add log file function
    self.cbLogFile = wx.CheckBox(Misc, label="Log to file",
                                 pos=(originX+125*0, originY+25*1),
                                 size=(-1, 20))
    wx.EVT_CHECKBOX(self.cbLogFile, self.cbLogFile.GetId(), self.OnButton_cbLogFile)

    #-------------------------------------------------------------------

    # Make all remaining the buttons
    for b in button.keys():
      bflags = 0
      if len(button[b])==4:
        (btab, bsize, bcol, brow) = button[b]
        module = "self"
      else:
        (btab, bsize, bcol, brow, module) = button[b]
      sizeY=20
      posY=originY+25*brow
      if (btab == 'Top'):
        posY += 5
      if bsize=='t':
        sizeX=31
        posX=originX+133*bcol
        if (bcol>0.6): posX += 11
        if (bcol>1.6): posX += 11
      elif bsize=='h':
        sizeX=52
        posX=originX+114*bcol
        if (bcol>0.5): posX += 11
        if (bcol>1.5): posX += 11
      else:
        sizeX=110
        posX=originX+125*bcol
      brefname = b.translate(string.maketrans(' ()/-.:','_______')).lower()
      sFmt = "self.button_%s = wx.Button(label='%s', name='%s', parent=%s, pos=(%d,%d), size=(%d,%d), style=0)"
      s = sFmt % (brefname, b, brefname, btab, posX, posY, sizeX, sizeY)
      # print s
      exec(s)
      sFmt = "wx.EVT_BUTTON(self.button_%s, self.button_%s.GetId(), %s.OnButton_%s)"
      func_suffix = "ufunc" if b.startswith('UFunc') else brefname
      s = sFmt % (brefname, brefname, module, func_suffix)
      try:
        exec(s)
      except:
        sFmt = "wx.EVT_BUTTON(self.button_%s, self.button_%s.GetId(), %s.OnButton_Undefined)"
        s = sFmt % (brefname, brefname, module)
        exec(s)
        print "Warning: No callback for %s"%b

    # Some last things..
    global ding
    ding = wx.Sound('ding.wav')
  #----------------------------------------------------

  def OnToggleRedirect(self, event):
    if event.Checked():
      ipvar.AppGui.RedirectStdio(None)
      print "Print statements and other standard output will now be directed to this window."
    else:
      ipvar.AppGui.RestoreStdio()
      print "Print statements and other standard output will now be sent to the usual location."
  #----------------------------------------------------

  def OnToggle_AllRegs(self, event):
    import ipallreg
    try:
      frame = ipallreg.frameAR
      if event.Checked() and frame.IsShownOnScreen():
        frame.Show(False)
      elif (not event.Checked()) and (not frame.IsShownOnScreen()):
        frame.UpdateAllRegs('RW', False) # tbd: update all tabs
        frame.Show(True)
    except:
      pass
  #----------------------------------------------------

  def OnHelpAbout(self, event):
    from about import MyAboutBox
    about = MyAboutBox(self)
    about.ShowModal()
    about.Destroy()
  #----------------------------------------------------
  def OnPageChanged(self, event):
    oldPageNum = event.GetOldSelection()
    pageNum    = event.GetSelection()
    event.Skip()
  #----------------------------------------------------
  def OnButton_Undefined(self, event):
    print "That button has no callback defined!"
  #----------------------------------------------------
  def OnButton_quit(self, event):
    sys.exit()
  #----------------------------------------------------
  def OnButton_shell(self, evt):
    if self.shell:
      # if it already exists then just make sure it's visible
      s = self.shell
      if s.IsIconized():
          s.Iconize(False)
      s.Raise()
    else:
      # Make a PyShell window
      from wx import py
      reload(iputils)
      namespace = { 'wx'          : wx,
                    'ipgui'         : self,
                    'iputils'   : iputils,
                    }
      self.shell = py.shell.ShellFrame(None, locals=namespace)
      self.shell.SetSize((640,480))
      self.shell.Show()

      # Hook the close event of the main frame window so that we
      # close the shell at the same time if it still exists            
    def CloseShell(evt):
      if self.shell:
        self.shell.Close()
        evt.Skip()
      #self.Bind(wx.EVT_CLOSE, CloseShell)
  #----------------------------------------------------
  def OnButton_reload_py(self, event):
    for mod in ["iputils", "user"]:
      print "Reloading %s.py"%mod
      exec("reload(%s)"%mod)
  #----------------------------------------------------
  def OnButton_ufunc(self, event):
    funDispatch = {
      'UFunc1': ipuser.UserFunc1,
      'UFunc2': ipuser.UserFunc2,
      'UFunc3': ipuser.UserFunc3,
      'UFunc4': ipuser.UserFunc4,
      'UFuncA': ipuser.UserFuncA,
      'UFuncB': ipuser.UserFuncB,
      'UFuncC': ipuser.UserFuncC,
      'UFuncD': ipuser.UserFuncD
    }
    buttonLabel = event.EventObject.Label
    try:
      user_fun = funDispatch[buttonLabel]
    except:
      print "Missing user function:", buttonLabel
    else:
      reload(ipuser)
      user_fun()
  #----------------------------------------------------
  def OnButton_cbConfig(self, event):
    # Disable the frequency drop down menu while in ext clock mode

    if (event.IsChecked()):
      ipvar.intClk = 0
      ipvar.extClk = 1
      print "External clock selected"
    else:
      ipvar.intClk = 1
      ipvar.extClk = 0
      print "External clock deselected"
  #----------------------------------------------------
  def OnButton_cbForceExtCk(self, event):
    ipvar.forceExtCk = event.IsChecked()
    self.cbConfig.Enable(not event.IsChecked())
    if (event.IsChecked()):
      ipvar.intClk = 0
      ipvar.extClk = 0
      ipvar.si5326Clk = 1
      config = ipvar.config | (0x1 << 37)
      print "0x%x"%config
    else:
      ipvar.intClk = 1
      ipvar.extClk = 0
    wx_inspect(True)
  #----------------------------------------------------
  def OnButton_cbDebugMode(self, event):
    if (event.IsChecked()):
     ipvar.debug = 1
     print ipvar.debug
    else:
     ipvar.debug = 0
     print ipvar.debug
  #----------------------------------------------------
  def OnButton_cbLogFile(self, event):
    if (event.IsChecked()):
      ipvar.logFile = True
    else: ipvar.logFile = False
  #----------------------------------------------------
  def OnButton_reapply(self, event):
    iputils.LoadConfiguration(ipvar.configurationFile, not configAlso)
  #----------------------------------------------------
  def OnButton_rerun(self, event):
    """
    run_script():  Reruns the script that has been run previously

    Arguments:  None
    Return:    None
    """
    global GlobalDoThread
    if self.argv!=None:
      scriptAndArgs = self.button_rerun.GetLabelText()
      if GlobalDoThread: # self.doThread:
        print("Rerun/Parent: execfile(%s), Executing %s" %
              (self.argv[0], ' '.join(self.argv[1:])))
        th = ScriptThread(self.argv[0], self.argv)
        th.start()
      else:
        # print "execfile(%s), argv:" % (self.argv[0])
        # print self.argv
        saveArgs = sys.argv # save
        sys.argv = self.argv
        execfile(self.argv[0])
        sys.argv = saveArgs # restore
  #----------------------------------------------------
  def OnButton_script(self, event):
    global scriptname
    wx_inspect(True)
    btnFrame = ScriptArgsFrame(self)
    btnFrame.Show()
  #----------------------------------------------------
  def OnButton_ping(self, event):
    iputils.pingFpga(forever=False)
  #----------------------------------------------------
  def OnButton_ping_inf(self, event):
    iputils.pingFpga(forever=True)
  #----------------------------------------------------
  def OnButton_cfg_chk(self, event):
    iputils.readFpgaConfig()
  #----------------------------------------------------
  def OnButton_ProgramRC(self, event):
    wx_inspect(True)
  #----------------------------------------------------

#CC real time thread will refresh data array
# then kick OnUpdate event        
  def OnUpdate(self, evt, displData):
    assert(1) #rb 
#------------------------------------------------------------------------------

class ScriptArgsFrame(wx.Frame):
  def __init__(self, parent):
    global GlobalDoThread
    wx.Frame.__init__(self, parent, -1, 'Run Script', pos=(20,720), size=(600, 90))
    self.parent = parent
    panel = wx.Panel(self, -1)
    vertSizer = wx.BoxSizer(wx.VERTICAL)
    horizSizer = wx.BoxSizer(wx.HORIZONTAL)
    headerText = wx.StaticText(panel, label = " "*80 + "<script_file>" + " "*50 + "<args>", style=0)
    self.runButton = wx.Button(panel, -1, "Run Script:", size=(90,25), style=wx.RAISED_BORDER)
    self.threadBox = wx.CheckBox(panel, -1, label="Separate Thread", size=(-1,20))
    self.threadBox.SetValue(GlobalDoThread) # parent.doThread)
    # print "Thread CheckBox=%s" % ("True" if GlobalDoThread else "False")
    wx.EVT_CHECKBOX(self.threadBox, self.threadBox.GetId(), self.OnButton_doThread)

    self.Bind(wx.EVT_BUTTON, self.OnClickRun, self.runButton)
    self.runButton.SetDefault()
    self.fileDialog = wx.Button(panel, -1, "Browse...", size=(100,25), style=wx.RAISED_BORDER)
    self.Bind(wx.EVT_BUTTON, self.OnFileDialog, self.fileDialog)
    if parent.argv: # show the previous script run
      scriptTxt = parent.argv[0]
      argsTxt = ' '.join(parent.argv[1:])
    else:
      scriptTxt = "user.py"
      argsTxt = "-d test -l user.log"
    self.entryScript = wx.TextCtrl(panel, -1, value = scriptTxt, size=(200,25),
                                   style=wx.RAISED_BORDER)    
    self.entryArgs = wx.TextCtrl(panel, -1, value = argsTxt,
                                 size=(200,25), style=wx.RAISED_BORDER)

    # Add the components the Horiz Sizer
    horizSizer.Add(self.runButton)
    horizSizer.Add(self.fileDialog)
    horizSizer.Add(self.entryScript)
    horizSizer.Add(self.entryArgs)
    vertSizer.Add(headerText)
    vertSizer.Add(horizSizer)
    vertSizer.Add(self.threadBox)
    panel.SetSizer(vertSizer)     
  #--------------------------------------------------------------
  def OnClickRun(self, event):
    global GlobalDoThread
    # print "Executing the script:", self.entryScript.Value, self.entryArgs.Value
    # os.system("python " + self.entryScript.Value + ' ' + self.entryArgs.Value)
    if self.entryScript.Value==None or self.entryScript.Value=='':
      return
    scriptAndArgs = self.entryScript.Value + ' ' + self.entryArgs.Value
    self.parent.argv = scriptAndArgs.split(' ')
    scriptname = self.entryScript.Value # global for execfile()
    if GlobalDoThread: # self.parent.doThread:
      # print "Parent: execfile(%s), Executing %s" % (self.entryScript.Value, self.entryArgs.Value)
      th = ScriptThread(self.entryScript.Value, self.parent.argv)
      th.start()
      # print "Executing ", scriptAndArgs
    else:
      #reload(ipconfig)
      # namespace = {'wx':wx, 'ipgui':self.parent,
      #              'ipconfig':ipconfig
      #             }
      # print "execfile(%s), argv:" % (self.entryScript.Value)
      #dbug print self.parent.argv
      saveArgs = sys.argv # save
      sys.argv = self.parent.argv
      execfile(self.entryScript.Value)
      sys.argv = saveArgs # restore
    wxRerun = self.parent.button_rerun
    wxRerun.SetLabel(scriptAndArgs)
    wxRerun.Size = wxRerun.BestSize
    wxRerun.Enable(1)
  #--------------------------------------------------------------
  def OnFileDialog(self, event):
    wildcard = "Python source (*.py)|*.py|" "All files (*.*)|*.*"
    dialog = wx.FileDialog(None, "Choose a file", os.getcwd(), "", wildcard, wx.OPEN)
    if dialog.ShowModal() == wx.ID_OK:
      self.entryScript.Value = dialog.GetPath() 
      dialog.Destroy()
  #----------------------------------------------------
  def OnButton_doThread(self, event):
    global GlobalDoThread
    doThread = event.IsChecked()
    self.parent.doThead = GlobalDoThread = doThread
    self.threadBox.SetValue(doThread)
    # print "Thread CheckBox=%s" % ("True" if GlobalDoThread else "False")
# -----------------------------------------------------------------------------

def main(argv=None):
  # global app
  if (ipvar.AppGui==None): ipvar.AppGui = wx.PySimpleApp()
  # if (ipvar.AppGui==None): ipvar.AppGui = wx.App(redirect=1, filename='stdouterr.txt')
  # ipvar.AppGui = app
  print "Loading.........."
  wx.InitAllImageHandlers()

  iputils.create_system()

  ipvar.frame = ipgui(None)
  ipvar.frame.Bind(EVT_REFRESH_RT_DISPLAY, ipgui.OnUpdate)
  if not is_cmd_arg('-mw'): # suppress main window
    ipvar.frame.Show()

  if not is_cmd_arg('-ar') and not wx_inspect(False): # Under WI, AllRegs crashes
    ipallreg.frameAR = ipallreg.MyFrame(None, -1, 'Inphi MDIO System Controller', ipvar.System)

  # Status bar information display
  iputils.initStatusBar()

  ipvar.IAVDD_Offset = 0
  ipvar.IDD_Offset = 0
  ipvar.IPVDD_Offset =0

  print "Initialization complete!"
  wx.lib.colourdb.updateColourDB()

  ipvar.AppGui.MainLoop()
  print "Exiting main()"
#-------------------------------------------------------------------------------

if __name__=='__main__':
  main(sys.argv)
#-------------------------------------------------------------------------------
# 
#  Local Variables:
#  tab-width: 2
#  indent-tabs-mode: nil
#  scroll-step: 1
#  line-number-mode: t
#  column-number-mode: t
#  End:
# 
