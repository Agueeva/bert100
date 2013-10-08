
import wx
from ipallreg import regRead, regWrite
import time
from ordict import *
#global stop

class SweepFrame(wx.Frame):
  global stop
  def __init__(self, parent, id, title,filename,wait_time=0.2,fmode='w'):
    wx.Frame.__init__(self, parent, id, title, pos=(10, 10), size=(300, 300))
    self.panel=wx.Panel(self,-1)
    self.input = []
    self.output = []
    self.N = []    # create N-space controller
    self.result = OrderedDict()
    self.outputFilename = filename
    self.wait_time = wait_time
    self.fmode = fmode
    self.rectSize=15
    
  def addInput(self, inputStr, range, label):
    self.input.append(OrderedDict()) #[inputStr,range,label])
    self.input[-1]['REG']=inputStr
    self.input[-1]['VALUES']=range
    self.input[-1]['LABEL']=label
    self.N.append(range)
    
  def addOutput(self, outputStr, label, comment=None):
    self.output.append(OrderedDict()) #[outputStr,label])
    self.output[-1]['REG']=outputStr
    self.output[-1]['LABEL']=label
    self.output[-1]['COMMENT']=comment
    
  def traverse(self,N): 
    sum = 1 
    for i in range(0,len(N)):
      sum=sum*len(N[i])
    state = []
    for i in range(0,sum):
      for n in range(0,len(N)):
        if n==0:
          state.append([])
        else:
          i=i/len(N[n-1])
        state[-1].append(i % len(N[n]))
    return(state)
    
  def Do(self,function=None):  
    global stop
    stop = 0
    print "Do the sweep with",len(self.input),"parameters"
    try:
      fid=open(self.outputFilename,self.fmode)
    except:
      print "* ERROR: Cannot open",self.outputFilename,"maybe it's already open?"
      return
    print "sweep data saved to: %s" %(self.outputFilename)
    
    state = self.traverse(self.N)
    
    outstr = ''
    str_join = ''
    for eachVarIndex in range(0,len(state[0])):
      outstr = outstr + str_join + str(self.input[eachVarIndex]['LABEL'])
      str_join = ','
    if function:
      outstr = outstr + ',function'
    else:      
      for eachOutput in self.output:
        if not eachOutput['COMMENT']:
          val = regRead(eachOutput['REG'])
          outstr = outstr + ',' + eachOutput['LABEL']
        
    if self.fmode.find('w')>-1:
      print outstr
      fid.write(outstr+'\n') 

    for eachState in state:
      if stop == 1:
        sys.exit()
      outstr = ''
      str_join = ''
      for eachVarIndex in range(0,len(eachState)):
        regWrite(self.input[eachVarIndex]['REG'],self.input[eachVarIndex]['VALUES'][eachState[eachVarIndex]])
        #print "write",self.input[eachVarIndex]['REG'],self.input[eachVarIndex]['VALUES'][eachState[eachVarIndex]]
        outstr = outstr + str_join + str(self.input[eachVarIndex]['VALUES'][eachState[eachVarIndex]])
        str_join = ','
        
      if function:
        pass
      else:
        for eachOutput in self.output:  
          if not eachOutput['COMMENT']:
            val = regRead(eachOutput['REG'])  # flush

      time.sleep(self.wait_time)             # wait
      
      if function:
        print outstr,
        fid.write(outstr) 
        #print "I would do it"
        function()
      else:
        for eachOutput in self.output:
          if not eachOutput['COMMENT']:
            val = regRead(eachOutput['REG'])
            outstr = outstr + ',' + str(val)
          else:
            outstr = outstr + ',' + eachOutput['COMMENT']
        print outstr
        fid.write(outstr+'\n')  
      
      try:
        wx.Yield()
      except:
        pass

    fid.close()
        
