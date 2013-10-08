# ------------------------------------------------------------------------------
#
# $RCSfile: validation#python#iprtab.py,v $$ - GUI Tab for a tabbed group of iPHY registers
#
# Disclaimer:
# Inphi Confidential
# Copyright(c) Inphi Corp. 2011
#
# All rights reserved.
# This is unpublished, confidential Inphi proprietary information.
# Do not reproduce or redistribute without written permission.
#
# $Header: /projects/wlv/iphy/sos/repository/iphy_A.rep/LIBS/FILES258/SRC/validation#python#iprtab.py,v 1.3 2011-07-17 18:02:53-07 rbemra Exp $
# $Log: validation#python#iprtab.py,v $
# Revision 1.3  2011-07-17 18:02:53-07  rbemra
# More updates to Main & AllRegs frames to keep in sync.
# Added Show/Hide item to Menu->File in both frames.
# Fixed Alt+X, Ctrl+H in Main Menu that had typos from iMB
#
# Revision 1.2  2011-07-14 16:33:39-07  rbemra
# Installed event-handlers for Main window's tabs
# Updated AllRegisters to handshake w/ Main window updates
#
# Revision 1.1  2011-07-11 12:24:19-07  rbemra
# First version of all register tabbed pages: event handlers partial/tbd
#
# ------------------------------------------------------------------------------
import wx
import wx.lib.inspection
from ipreg import *
import ipmdio
#------------------------------------------------------------------------------

# class wxRegRef():
#   def __init__(self, regDef, wxDef):
#     self.wxWrite = wxWidget
#   def add

class SubRef:
  def __init__(self, subDef, cbDes, tcBits):
    self.subDef = subDef
    self.cbDes = cbDes
    self.tcBits = tcBits
    self.stDes = None
    self.stBits = None
  def add_st_xref(self, stDes, stBits):
    self.stDes = stDes
    self.stBits = stBits

class RegXref():
  def __init__(self, regDef, tcWrite=None, stRead=None):
    self.regDef = regDef
    self.refSubTab = OrderedDict()
  def add_xref(self, tcWrite, stRead):
    self.tcWrite = tcWrite
    self.stRead = stRead
  def add_sub_xref(self, subKey, subDef, cbDes, tcBits, stDes, stBits):
    self.refSubTab[subKey] = SubRef(subDef, cbDes, tcBits, stDes, stBits)
#------------------------------------------------------------------------------

def update_write(regXref, doAllRegs=False):
  regDef = regXref.regDef
  refTab = regXref.refSubTab
  wVec = bv.BitVector(REG_LEN, regDef.lastWrite)
  for (subAddr, subReg) in regDef.regSubTab.items():
    pTab = subReg.purposeTab
    nBits = subReg.nBits
    subVal = int(wVec.getbits(subReg.lsbStart, nBits))
    try:
      subDes = pTab[subVal] if len(pTab)>0 else ""
    except:
      subDes = ""
    bv1 = bv.BitVector(nBits, subVal)
    subBits = bv1.get_rbits(delim=None) # make the key
    subRef = refTab[subAddr]
    if subDes != "":
      subRef.cbDes.SetStringSelection(subDes)
    subRef.tcBits.ChangeValue(subBits)
  regXref.tcWrite.ChangeValue('0x%04x' % regDef.lastWrite)
  if doAllRegs:
    try:
      from ipallreg import frameAR
      if frameAR.IsShownOnScreen():
        frameAR.UpdateWriteTextFromHex(regDef, regDef.lastWrite, False)
        frameAR.UpdateComboFromHex(regDef, regDef.lastWrite, False)
    except:
      pass
#-----------------------------------------------------------------------------

def update_read(regXref, doAllRegs=False):
  regDef = regXref.regDef
  refTab = regXref.refSubTab
  rVec = bv.BitVector(REG_LEN, regDef.lastRead)
  for (subAddr, subReg) in regDef.regSubTab.items():
    pTab = subReg.purposeTab
    nBits = subReg.nBits
    subVal = int(rVec.getbits(subReg.lsbStart, nBits))
    try:
      subDes = pTab[subVal] if len(pTab)>0 else ""
    except:
      subDes = ""
    bv1 = bv.BitVector(nBits, subVal)
    subBits = bv1.get_rbits(delim=None) # make the key
    subRef = refTab[subAddr]
    subRef.stDes.SetLabel(subDes)
    subRef.stBits.SetLabel(subBits)
  regXref.stRead.SetLabel('0x%04x' % regDef.lastRead)
  if doAllRegs:
    try:
      from ipallreg import frameAR
      if frameAR.IsShownOnScreen():
        frameAR.UpdateReadTextFromHex(regDef, regDef.lastRead, False)
    except:
      pass
#------------------------------------------------------------------------------

class MyCB(wx.ComboBox):
  def __init__(self, parent, value, choices, style, subAddr):
    wx.ComboBox.__init__(self, parent, -1, value = value, choices = choices,
                         style = style)
    self.subAddr = subAddr
#------------------------------------------------------------------------------

class MyTC(wx.TextCtrl):
  def __init__(self, parent, value, style, subAddr):
    wx.TextCtrl.__init__(self, parent, -1, value = value, style = style)
    self.subAddr = subAddr
#------------------------------------------------------------------------------
# Register e.g. iGBR30.16-19 have 12 subRegs
# Reg25GList: ['30.16', '30.17', '30.18', '30.19']
# For kLane in range(4):
#         prbs_lock <15> rx_disable <14> Lback_en <13> pat_ver_en <12> prbs_ver_inv <11> pat_ver_sel <10:8> : StaticText
#         rsvd <7> tx_disable <6> error_insert <5> pat_gen_en <4> prbs_gen_inv <3> pat_gen_sel <2:0> : StaticText
# Wbutton prbs_lock <15> rx_disable <14> Lback_en <13> pat_ver_en <12> prbs_ver_inv <11> pat_ver_sel <10:8> : ComboBox/StaticText
#         rsvd <7> tx_disable <6> error_insert <5> pat_gen_en <4> prbs_gen_inv <3> pat_gen_sel <2:0> : ComboBox/StaticText
# Rbutton prbs_lock <15> rx_disable <14> Lback_en <13> pat_ver_en <12> prbs_ver_inv <11> pat_ver_sel <10:8> : StaticText
#         rsvd <7> tx_disable <6> error_insert <5> pat_gen_en <4> prbs_gen_inv <3> pat_gen_sel <2:0> : StaticText
# Lane 0  Address  30.16  25G Lane 0 Pattern Control  W: TextCtrl  R: StaticText
#------------------------------------------------------------------------------

# device.devName::regKey, e.g. CDR0::30.16

class MyRegTab(wx.Panel):
  def __init__(self, parent, name, device, regKeyList):
    self.evtXref = {} # event handler cross-reference
    regAddrXref = parent.regAddrXref # hier-address -> RegXref
    wx.Panel.__init__(self, parent, -1, name=name)
    scroll3 = wx.ScrolledWindow(self, -1) #] 5 sw -> nbp
    panel4 = wx.Panel(scroll3, -1, pos = wx.Point(15,5)) #] 6 panel -> sw

    # find # of subRegs, # of columns needed, for GridSizer
    # nRows = len(regKeyList) * 4 # headerText, comboBoxWvals, RvalText, addr/des/hex R/W
    currReg = device.regDefTab[regKeyList[0]]
    # nCols =  2 * len(currReg.regSubTab) # wx_name/des wx_value
    gs4 = wx.GridBagSizer(2, 2) # hgap, vgap

    wxColorEven = wx.Colour(150, 200, 80) # green
    wxColorOdd = wx.Colour(0, 180, 200) # blue
    kRow = 0
    for regKey in regKeyList:
      kCol = 0
      currReg = device.regDefTab[regKey]
      regName = currReg.regName # '25G Lane # Pattern Control'

      regXref = RegXref(currReg)
      # Row 1. Add StaticText headers
      # [sReg.purposeName/ST1] [<sReg.MSB:sReg.LSB>/ST2] ...
      for (subAddr, subReg) in currReg.regSubTab.items():
        bgColor = wxColorOdd if (kCol/2)%2 else wxColorEven
        style = wx.ALIGN_CENTER | (wx.BORDER_SUNKEN if (kCol/2)%2 else wx.BORDER_RAISED)
        srName = subReg.purposeName

        wx1_ST1 = wx.StaticText(panel4, -1, srName)
        wx1_ST1.SetBackgroundColour(bgColor)
        # print kRow, kCol, wx1_ST1.Name
        gs4.Add(wx1_ST1, pos=(kRow, kCol))
        kCol += 1

        if subReg.nBits==1:
          srBits = '<%d>' % subReg.lsbStart
        else:
          srBits = '<%d:%d>' % (subReg.lsbStart+subReg.nBits-1, subReg.lsbStart)
        wx1_ST2 = wx.StaticText(panel4, -1, srBits) # , name="wx1_ST2_"+regName)
        wx1_ST2.SetBackgroundColour(bgColor)
        # print kRow, kCol, wx1_ST2.Name
        gs4.Add(wx1_ST2, pos=(kRow, kCol))
        kCol += 1
      nCols = kCol
      # Row2 = ComboBox|StaticText=sR.
      # [sReg[subKey]/CB1] [subKey/TC2]
      kRow += 1
      kCol = 0
      lwVec = bv.BitVector(REG_LEN, currReg.lastWrite)
      for (subAddr, subReg) in currReg.regSubTab.items():
        bgColor = wxColorOdd if (kCol/2)%2 else wxColorEven
        bitMask = subReg.bitMask # R|W|RC|RW
        subVal = int(lwVec.getbits(subReg.lsbStart, subReg.nBits))
        pTab = subReg.purposeTab
        try:
          subDes = pTab[subVal] if len(pTab)>0 else ""
        except:
          subDes = ""
        else:
          pass
        bv1 = bv.BitVector(subReg.nBits, subVal)
        subBits = bv1.get_rbits(delim=None) # make the key

        # Add ComboBox+TextCtrl for [desc|v] [bits|v]
        wx2_srCB1 = MyCB(panel4, subDes, pTab.values(),
                         wx.CB_DROPDOWN|wx.TE_PROCESS_ENTER|wx.ALIGN_RIGHT, subAddr)
        wx2_srCB1.SetBackgroundColour(bgColor)
        self.Bind(wx.EVT_COMBOBOX, self.OnSubDesCB, wx2_srCB1)
        self.Bind(wx.EVT_TEXT_ENTER, self.OnSubDesCB, wx2_srCB1)
        self.evtXref[wx2_srCB1] = regXref # todo: use [wx.GetId()] as key?
        if bitMask=='R' or bitMask=='RC':
          wx2_srCB1.Disable()

        # print kRow, kCol, wx2_srCB1.Name
        gs4.Add(wx2_srCB1, pos=(kRow, kCol))
        kCol += 1

        wx2_srTC2 = MyTC(panel4, subBits, wx.TE_PROCESS_ENTER|wx.ALIGN_LEFT, subAddr)
        wx2_srTC2.SetBackgroundColour(bgColor)
        self.Bind(wx.EVT_TEXT_ENTER, self.OnSubBitsTC, wx2_srTC2)
        self.evtXref[wx2_srTC2] = regXref # todo: use [wx.GetId()] as key?
        if bitMask=='R' or bitMask=='RC':
          wx2_srTC2.Disable()

        # print kRow, kCol, wx2_srTC2.Name
        gs4.Add(wx2_srTC2, pos=(kRow, kCol))
        kCol += 1

        regXref.refSubTab[subAddr] = SubRef(subReg, wx2_srCB1, wx2_srTC2)

      # Row 3. Add StaticText [desc] [bits] pairs from currReg.lastRead
      # [sReg[sReg.lR.subKey]/ST1] [sReg.lR.subKey/ST2]
      kRow += 1
      kCol = 0
      lrVec = bv.BitVector(REG_LEN, currReg.lastRead)
      for (subAddr, subReg) in currReg.regSubTab.items():
        bgColor = wxColorOdd if (kCol/2)%2 else wxColorEven
        subVal = int(lrVec.getbits(subReg.lsbStart, subReg.nBits))
        bv1 = bv.BitVector(subReg.nBits, subVal)
        subBits = bv1.get_rbits(delim=None) # make the key
        pTab = subReg.purposeTab
        try:
          subDes = pTab[subVal] if len(pTab)>0 else ""
        except:
          subDes = ""
        else:
          pass

        wx3_srST1 = wx.StaticText(panel4, -1, subDes) # , name="wx3_srST1_"+regName)
        wx3_srST1.SetBackgroundColour(bgColor)
        # print kRow, kCol, wx3_srST1.Name
        gs4.Add(wx3_srST1, pos=(kRow, kCol))
        kCol += 1

        wx3_srST2 = wx.StaticText(panel4, -1, subBits) # , name="wx3_ST2_"+regName)
        wx3_srST2.SetBackgroundColour(bgColor)
        # print kRow, kCol, wx3_srST2.Name
        gs4.Add(wx3_srST2, pos=(kRow, kCol))
        kCol += 1

        regXref.refSubTab[subAddr].add_st_xref(wx3_srST1, wx3_srST2)

      # Row 4: 16-bit hexAddr, regName, lastWrite, lastRead
      # [hexAddr/ST1] [Reg. desc/ST2] [W:/ST3][hexVal/TC4] [R:/ST5] [hexVal/ST6]
      kRow += 1
      kCol = 0
      regKeyAddr = 'R' + regKey + ': ' + '0x%04x' % currReg.regAddr # R30.16: 0x0010
      wx4_ST1 = wx.StaticText(panel4, -1, regKeyAddr)
      # print kRow, kCol, wx4_ST1.Name
      gs4.Add(wx4_ST1, pos=(kRow, kCol), span=(1, 1))
      kCol += 1

      wx4_ST2 = wx.StaticText(panel4, -1, regName)
      # print kRow, kCol, wx4_ST2.Name
      gs4.Add(wx4_ST2, pos=(kRow, kCol), span=(1, 1))
      kCol += 1

      wx4_ST3 = wx.StaticText(panel4, -1, '    Write:')
      # print kRow, kCol, wx4_ST3.Name
      gs4.Add(wx4_ST3, pos=(kRow, kCol), span=(1, 1))
      kCol += 1

      wx4_TC4 = wx.TextCtrl(panel4, -1,  '0x%04x' % currReg.lastWrite,
                  style=wx.TE_PROCESS_ENTER|wx.ALIGN_LEFT)
      self.Bind(wx.EVT_TEXT_ENTER, self.OnRegHexTC, wx4_TC4)
      self.evtXref[wx4_TC4] = regXref # todo: use [wx.GetId()] as key?
      # print kRow, kCol, wx4_TC4.Name
      gs4.Add(wx4_TC4, pos=(kRow, kCol), span=(1, 1))
      kCol += 1

      wx4_ST5 = wx.StaticText(panel4, -1, '     Read:')
      # print kRow, kCol, wx4_ST5.Name
      gs4.Add(wx4_ST5, pos=(kRow, kCol), span=(1, 1))
      kCol += 1

      wx4_ST6 = wx.StaticText(panel4, -1, '0x%04x' % currReg.lastRead)
      # print kRow, kCol, wx4_ST6.Name
      gs4.Add(wx4_ST6, pos=(kRow, kCol), span=(1, 1))
      regXref.add_xref(wx4_TC4, wx4_ST6)
      kCol += 1

      # Row 5: -------------spacer-------------------------------------
      kRow += 1
      wx5_ST1 = wx.StaticText(panel4, -1, '-'*nCols*25,
                  name="spacer", style=wx.ALIGN_CENTER)
      gs4.Add(wx5_ST1, pos=(kRow, 0), span=(1, nCols))
      kRow += 1
      kCol = 0
      regAddrXref[device.devName+'::'+regKey] = regXref
      #dbug print 'Added', device.devName+'::'+regKey
    # --- end for currReg --------

    panel4 # self
    panel4.SetSizerAndFit(gs4) #] 9.1
    panel4.SetAutoLayout(True) #] 9.2
    panel4.Layout() #] 9.3

    bs2 = wx.BoxSizer(wx.VERTICAL) #] 10.1
    bs2.Add(scroll3, 1, wx.EXPAND) #] 10.2
    self.SetSizer(bs2) #] 10.3

    unit = 20
    width, height = panel4.GetSize() #] 11.1
    scroll3.SetVirtualSize((width+unit, height+unit)) #] 11.2
    scroll3.SetScrollRate(unit, unit) #] 11.3

    self.Show(1)
  #-----------------------------------------------------------------------------
  def OnRegHexTC(self, evt):
    evtStrg = evt.GetString()
    #dbug print "OnRegHexTC:", evtStrg
    tcHex = evt.GetEventObject()
    regXref = self.evtXref[tcHex]
    regDef = regXref.regDef

    lastVal = regDef.lastWrite
    try:
      wVec = bv.BitVector(REG_LEN, evtStrg)
    except:
      tcHex.ChangeValue('0x%04x' % regDef.lastWrite) # ignore bad input; restore prev. value
      evt.Skip()
    else:
      regDef.lastWrite = int(wVec.value)
      ipmdio.ipmdio_wr(regDef.prtAddr, regDef.devAddr, regDef.regAddr, regDef.lastWrite)
      update_write(regXref, True)
      # time.sleep(0.01) # wait 10ms before read?
      regDef.lastRead = ipmdio.ipmdio_rd(regDef.prtAddr, regDef.devAddr, regDef.regAddr)
      update_read(regXref, True)
  #-----------------------------------------------------------------------------
  # When the user selects a drop-down combo. entry, or enter we go here.
  def OnSubDesCB(self, evt):
    evtStrg = evt.GetString()
    #dbug print "OnSubDesCB:", evtStrg
    cb = evt.GetEventObject()
    regXref = self.evtXref[cb]
    regDef = regXref.regDef
    #dbug print 'R'+cb.subAddr+':', regDef.regName
    subDef = regDef.regSubTab[cb.subAddr]
    nBits = subDef.nBits

    refTab = regXref.refSubTab
    subRef = refTab[cb.subAddr]
    tc = subRef.tcBits
    try:
      kIndex = cb.Items.index(evtStrg)
    except:
      evt.Skip()
    else:
      cIndex = subDef.purposeTab.keys()[kIndex]
      # bitStrg = bv.BitVector(nBits, kIndex).get_rbits()[-nBits:]
      bitStrg = bv.BitVector(nBits, cIndex).get_rbits()[-nBits:]
      tc.ChangeValue(bitStrg)
      wVec = bv.BitVector(REG_LEN, regDef.lastWrite)
      wVec.copybits(cIndex, subDef.lsbStart, 0, nBits)
      regDef.lastWrite = int(wVec.value)
      ipmdio.ipmdio_wr(regDef.prtAddr, regDef.devAddr, regDef.regAddr, regDef.lastWrite)
      update_write(regXref, True)
      # time.sleep(0.01) # wait 10ms before read?
      regDef.lastRead = ipmdio.ipmdio_rd(regDef.prtAddr, regDef.devAddr, regDef.regAddr)
      update_read(regXref, True)
  #-----------------------------------------------------------------------------
  # When the user types something into the control then hits ENTER.
  def OnSubBitsTC(self, evt):
    evtStrg = evt.GetString()
    #dbug print "OnSubBitsTC:", evtStrg
    tc = evt.GetEventObject()
    regXref = self.evtXref[tc]
    regDef = regXref.regDef
    #dbug print 'R'+tc.subAddr+':', regDef.regName
    subDef = regDef.regSubTab[tc.subAddr]
    nBits = subDef.nBits

    refTab = regXref.refSubTab
    subRef = refTab[tc.subAddr]
    cb = subRef.cbDes
    try:
      fmt = "%d'b" % nBits
      bcdVal = bv.BitVector(nBits, fmt+evtStrg).value
    except:
      evt.Skip() # illegal bit syntax
    else:
      try:
        cb.SetStringSelection(subDef.purposeTab[bcdVal])
      except:
        pass # could be a large counter/thermal code
      wVec = bv.BitVector(REG_LEN, regDef.lastWrite)
      wVec.copybits(bcdVal, subDef.lsbStart, 0, nBits)
      regDef.lastWrite = int(wVec.value)
      ipmdio.ipmdio_wr(regDef.prtAddr, regDef.devAddr, regDef.regAddr, regDef.lastWrite)
      update_write(regXref, True)
      # time.sleep(0.01) # wait 10ms before read?
      regDef.lastRead = ipmdio.ipmdio_rd(regDef.prtAddr, regDef.devAddr, regDef.regAddr)
      update_read(regXref, True)
#------------------------------------------------------------------------------
#
# update_reg('CDR0::30.16', 'R'|'W'|'RW')
#
def update_reg(hierRef, regAddrXref, doTgt='RW', doAllRegs=False):
  # [portAddr, devAddr, regAddr, nBits, start] = mdio_addr_decode(hierRef)
  try:
    xRef = regAddrXref[hierRef]
  except:
    #dbug print hierRef, ': not found'
    return
  #dbug print 'Updating ', hierRef, doTgt, 'field(s)'
  if doTgt=='R':
    update_read(xRef, doAllRegs)
  elif doTgt=='W':
    update_write(xRef, doAllRegs)
  else:
    update_read(xRef, doAllRegs)
    update_write(xRef, doAllRegs)
#------------------------------------------------------------------------------
#
def update_device(regAddrXref, doTgt='RW', doAllRegs=False):
  for xRef in regAddrXref.values():
    if doTgt=='R':
      update_read(xRef, doAllRegs)
    elif doTgt=='W':
      update_write(xRef, doAllRegs)
    else:
      update_read(xRef, doAllRegs)
      update_write(xRef, doAllRegs)
#------------------------------------------------------------------------------
# 
#  Local Variables:
#  tab-width: 2
#  indent-tabs-mode: nil
#  scroll-step: 1
#  line-number-mode: t
#  column-number-mode: t
#  End:
#
