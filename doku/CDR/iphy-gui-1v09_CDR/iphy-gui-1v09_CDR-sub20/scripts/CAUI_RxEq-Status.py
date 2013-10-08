from time import sleep
from ordict import *
from membus import memRead,memWrite

device = device_name+'::'
reg_list = OrderedDict()

reg_list['RXEQ_PRECAL_CODE']   ={'REG':('0x1a[4:2]', [])}
reg_list['RXEQ_BEST_EYE_VAL_0x34[7]']   ={'REG':('0x34[7]', [])}
reg_list['RXEQ_BEST_EYE_VAL_0x35[7:0]'] ={'REG':('0x35[7:0]', [])}
reg_list['RXEQ_BEST_EYE_VAL_0x36[4:0]'] ={'REG':('0x36[4:0]', [])}
reg_list['RXCALEQ_DCGAIN']              ={'REG':('0x15[2:0]', [])}
reg_list['RXCALEQ_DFEPSTAPF3DB']        ={'REG':('0x15[5:3]', [])}
reg_list['lfAGC(ATT)']                         ={'REG':('0x18[6:4]', [])}
reg_list['hfAGC(BOOST)']                       ={'REG':('0x19[4:0]', [])}
reg_list['DFETAPGAIN']                  ={'REG':('0x16[2:0]', [])}
reg_list['DFETAP1GAIN']                 ={'REG':('0x16[6:3]', [])}
reg_list['DFETAP2GAIN']                 ={'REG':('0x17[3:0]', [])}
reg_list['DFETAP3GAIN']                 ={'REG':('0x17[7:4]', [])}
reg_list['DFETAP4GAIN']                 ={'REG':('0x18[3:0]', [])}
reg_list['RXCALEQ_LOCWREN']             ={'REG':('0x15[6:6]', [])}
reg_list['RXCALEYEDIAGFSM_LOCWREN']     ={'REG':('0x42[1]', [])}
reg_list['RXCALEYEDIAGFSMIN_LOCWREN']   ={'REG':('0x25[3]', [])}

print '%30s' % device_name,
for cLane in range(0,10):
  print '%3d' % cLane,
print '\n','-'*70
for cLane in range(0,10):
  #print 'RX',cLane
  if cLane < 3:
    common = 'common0'
  elif cLane < 7:
    common = 'common1'
  else:
    common = 'common2'
    
  #rx_err     = device +'8.' + str(1600+cLane)
  #patlock    = device +'30.' + str(16+cLane)+ '.15'

  #regRead(rx_err),regRead(patlock)
  #time.sleep(0.1)
  #print 'RX',cLane,'Err=',regRead(rx_err),'Lock=',regRead(patlock)

  for eachReg in reg_list.keys():
#    print eachReg, reg_list[eachReg]['REG'][0],
    val = memRead(reg_list[eachReg]['REG'][0],cLane,device_name)
    reg_list[eachReg]['REG'][1].append(val)
    
  #print  "PCSRXEQ_START:",memRead('0x1a[5]',cLane)
  #print  "RXEQ_DONE:",memRead('0x36[5]',cLane)

  #print  "RXCALEYEDIAGFSM_XVALWEIGHT']={'0x1ac[1:0]',common)
  #print  "RXCALEYEDIAGFSM_YVALWEIGHT']={'0x1ad[7:4]',common)
#print '%30s%3d%3d%3d%3d%3d%3d%3d%3d%3d%3d' % (' ',0,1,2,3,4,5,6,7,8,9)
for eachReg in reg_list.keys():
  print '%30s' % eachReg,
  for cLane in range(0,10):
    print '%3d' % reg_list[eachReg]['REG'][1][cLane],
  print
