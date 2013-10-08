import wx
import os

class ButtonFrame(wx.Frame):
  def __init__(self):
    wx.Frame.__init__(self, None, -1, 'Run Script', size=(600, 200))
    panel = wx.Panel(self, -1)
    vertSizer  = wx.BoxSizer(wx.VERTICAL)
    horizSizer = wx.BoxSizer(wx.HORIZONTAL)
    headerText = wx.StaticText(panel,label=" "*60+"<script_file>"+" "*40+"<args>",style=0)
    self.button = wx.Button(panel, -1, "Run Script:", size=(90,25),style=wx.RAISED_BORDER)
    self.Bind(wx.EVT_BUTTON, self.OnClick, self.button)
    self.button.SetDefault()
    self.fileDialog = wx.Button(panel, -1, "Browse...", size=(30,25),style=wx.RAISED_BORDER)
    self.Bind(wx.EVT_BUTTON, self.OnFileDialog, self.fileDialog)
    self.entryScript = wx.TextCtrl(panel,-1,value="C:/workspace/default.py",size=(200,25),style=wx.RAISED_BORDER)    
    self.entryArgs   = wx.TextCtrl(panel,-1,value="-d test -l log.txt",size=(200,25),style=wx.RAISED_BORDER)    

    # Add the components the Horiz Sizer
    horizSizer.Add(self.button)
    horizSizer.Add(self.fileDialog)
    horizSizer.Add(self.entryScript)
    horizSizer.Add(self.entryArgs)
    vertSizer.Add(headerText)
    vertSizer.Add(horizSizer)
    panel.SetSizer(vertSizer)

  def OnClick(self, event):
    print "Executing the script:",self.entryScript.Value,self.entryArgs.Value
    os.system("python " + self.entryScript.Value + ' ' + self.entryArgs.Value)
 
  def OnFileDialog(self,event):
    wildcard = "Python source (*.py)|*.py|" "All files (*.*)|*.*"
    dialog = wx.FileDialog(None, "Choose a file", os.getcwd(), "", wildcard, wx.OPEN)
    if dialog.ShowModal() == wx.ID_OK:
      self.entryScript.Value = dialog.GetPath() 
      dialog.Destroy()
#-------------------------------------------------------------------------------

def main():
  app = wx.PySimpleApp()
  frame = ButtonFrame()
  frame.Show()
  app.MainLoop()
#-------------------------------------------------------------------------------

if __name__=='__main__':
  main()
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
