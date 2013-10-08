import time
from membus import memRead,memWrite

print "Running rx_adapt 2.0 on all CAUI lanes..."
print device_name

for cLane in range(0,10):
  if cLane < 3:
    common = 'common0'
  elif cLane < 7:
    common = 'common1'
  else:
    common = 'common2'
  #ovr
  #0x1a - common
  # 00001 - Power Down
  # 00010 - P2
  # 00100 - P1
  # 01000 - P0s
  # 10000 - P0
  
  
 #----------------------------------------------------------------- 
  memWrite('0x18[1:1]',common,0x0,device_name) #CMNPCSP STATE Override Enable (page 119)
  memWrite('0x7[0:0]',cLane,0x0,device_name)   #LANEPCSP STATE Override Enable (page 117)
  #set iddq
  memWrite('0x1a[4:0]',common,0x1,device_name) #CMNPCSPSTATE_SYNTH[4:0] - 00001 = {Power Down) (page 120)
  memWrite('0xa[4:0]',cLane,0x1,device_name)   #LANEPCSSTATE_TX[4:0] - 00001 (Power Down) 
  memWrite('0x9[4:0]',cLane,0x1,device_name)   #LANEPCSSTATE_RX[4:0] - 00001 (Power Down)

  #xadjust offset, center of the eye =59; cal=67; (67+ 2**5+32) (bit0 to bit5)
  memWrite('0x1b6[4:0]',common,0x1a,device_name) 
  memWrite('0x1b5[7:6]',common,0x1,device_name)

  #set p0
  memWrite('0x1a[4:0]',common,0x10,device_name) #CMNPCSPSTATE_SYNTH[4:0] - P0 
  memWrite('0xa[4:0]',cLane,0x10,device_name)   #LANEPCSSTATE_TX[4:0] - P0
  memWrite('0x9[4:0]',cLane,0x10,device_name)   #LANEPCSSTATE_RX[4:0] - P0
  ##--------------------------------------------------------------------------------------------
  
  #RXCALEQ_LOCWREN - Active Low
  memWrite('0x15[6:6]',cLane,1,device_name)  #Disable
  
  #RXEQ_CALEN - Active High
  memWrite('0x1d[2]',common,0,device_name) # Disable
  #PCSRXEQ_LOCWREN Active Low
  memWrite('0x1a[6]',cLane,1,device_name) # Disable
  #PCSRXEQ_START - Active High 
  memWrite('0x1a[5]',cLane,0,device_name) #Disable
 
  #RXCALEYEDIAGFSMIN_LOCWREN - Active Low
  memWrite('0x25[3]',cLane,1,device_name) #Disable
  
  # Roam Eye
  # Overwrite
  #RXCALROAMXADJUST_LOCWREN - Active Low
  memWrite('0xd[0]',cLane,1,device_name) #Disable
  
  #RXCALROAMYADJUST_LOCWREN - Active Low
  memWrite('0xd[1]',cLane,1,device_name) #Disable
  
  #RXCALROAMEEMEASIN_LOCWREN - Active Low
  memWrite('0x25[6]',cLane,1,device_name) #Disable
  
  ################increase RXCALROAMEYEMEAS_COUNT (RXCALROAMEYEMEAS_COUNT * bitwidth < 2**16-1)####################
  memWrite('0x1bf[2:0]',common,0,device_name)
  #print "RXCALROAMEYEMEAS_COUNT_0x1bf[2:0]:",memRead('0x1bf[2:0]',common)
  memWrite('0x1be[7:0]',common,0xf4,device_name)
  #print "RXCALROAMEYEMEAS_COUNT_0x1be[7:0]:",memRead('0x1be[7:0]',common)
  memWrite('0x1bd[7:3]',common,0x1,device_name)
  #print "RXCALROAMEYEMEAS_COUNT_0x1bd[7:3]:",memRead('0x1bd[7:3]',common)
  #print "RXCALROAMEYEMEAS_COUNT:",memRead('0x1bd[7:3]',common)*2**11+memRead('0x1be[7:0]',common)*2**3+memRead('0x1bf[2:0]',common)

  #422[7:6],423[7:0],424[5:0]	RXCALEYEDIAGFSM_BERTHRESHOLD	4  0x400
    
  memWrite('0x1a6[7:6]',common,0x0,device_name)
  memWrite('0x1a7[7:0]',common,0x10,device_name)
  memWrite('0x1a8[5:0]',common,0x0,device_name)
  
  ################################## change iteration number here##############################  
  

  #RXEQ settings - iteration #
  memWrite('0x1d3[4:0]',common,0xa,device_name)
  memWrite('0x1d2[7:6]',common,0x0,device_name)

  memWrite('0x1d4[7:6]',common,0x0,device_name)
  memWrite('0x1d5[2:0]',common,0x1,device_name)

  memWrite('0x1f1[7:4]',common,0x0,device_name)
  memWrite('0x1f2[2:0]',common,0x0,device_name)

  memWrite('0x1f4[7:6]',common,0x2,device_name)
  memWrite('0x1f5[2:0]',common,0x1,device_name)

  memWrite('0x1dc[6:4]',common,0x0,device_name)

  #469[7:6],470[0:0]	RXEQ_DCGAIN_LUP0	4
  memWrite('0x1d5[7:6]',common,0x2,device_name)
  memWrite('0x1d6[0:0]',common,0x1,device_name)

  #473[3:1]	RXEQ_DFEPSTAPF3DB_LUP0	4
  memWrite('0x1d9[3:1]',common,0x0,device_name)

  #510[3:1]	RXEQ_LOOKUP_LASTCODE	4
  memWrite('0x1fe[3:1]',common,0x0,device_name)


  #ovr

  #-----------------------------------------------------------------
  memWrite('0x18[1:1]',common,0x0,device_name) #CMNPCSP STATE Override Enable (page 119
  memWrite('0x7[0:0]',cLane,0x0,device_name)   #LANEPCSP STATE Override Enable (page 117)
  
  #Power Down
  memWrite('0x1a[4:0]',common,0x1,device_name) #CMNPCSPSTATE_SYNTH[4:0] - 00001 = {Power Down) (page 120)
  memWrite('0xa[4:0]',cLane,0x1,device_name)   #LANEPCSSTATE_TX[4:0] - 00001 (Power Down) 
  memWrite('0x9[4:0]',cLane,0x1,device_name)   #LANEPCSSTATE_RX[4:0] - 00001 (Power Down)

  memWrite('0x1d5[5:3]',common,0x5,device_name)
  memWrite('0x1d8[7:6]',common,0x0,device_name)
  memWrite('0x1d9[0:0]',common,0x0,device_name)
  memWrite('0x1dc[3:1]',common,0x0,device_name)
  memWrite('0x1dc[3:1]',common,0x0,device_name)
  memWrite('0x1df[7:4]',common,0x0,device_name)
  memWrite('0x1e4[3:0]',common,0x8,device_name)
  memWrite('0x1e8[7:4]',common,0x0,device_name)
  memWrite('0x1ed[3:0]',common,0x8,device_name)
  memWrite('0x1f4[5:1]',common,0x11,device_name)
  memWrite('0x1fa[0:0]',common,0x1,device_name)
  memWrite('0x1f9[7:6]',common,0x3,device_name)

  #set p0
  memWrite('0x1a[4:0]',common,0x10,device_name) #CMNPCSPSTATE_SYNTH[4:0] - P0 
  memWrite('0xa[4:0]',cLane,0x10,device_name)   #LANEPCSSTATE_TX[4:0] - P0
  memWrite('0x9[4:0]',cLane,0x10,device_name)   #LANEPCSSTATE_RX[4:0] - P0
  
  
  # enable boost, pulseshaping, and 3db
  memWrite('0x1d3[7:5]',common,0x0,device_name)
  memWrite('0x1d4[5:0]',common,0x1c,device_name)
  memWrite('0x1f2[7:3]',common,0x1,device_name)
  memWrite('0x1f3[3:0]',common,0xc,device_name)

  #-----------------------------------------------------------------
  #RXEQ_CALEN RX Equalization FSM enable (Active high) p133
  #RXEQ_CALEN (Active High)
  memWrite('0x1d[2]',common,0,device_name)
  
  #PCSRXEQ_LOCWREN (Active Low)
  memWrite('0x1a[6]',cLane,0,device_name)
  #PCSRXEQ_START (Active High)
  memWrite('0x1a[5]',cLane,0,device_name)

  #-----------------------------------------------------------------
  #RXEQ_CALEN RX Equalization FSM enable (Active high) p133
  memWrite('0x1d[2]',common,1,device_name)
  
  #PCSRXEQ_LOCWREN (Active Low)
  memWrite('0x1a[6]',cLane,0,device_name)
  #PCSRXEQ_HINT 
  #memWrite('0x1a[4:2]',cLane,0) # 2" FR4

  print 'Start RX:%d Hint:%d' %(cLane,memRead('0x1a[4:2]',cLane,device_name))
  #PCSRXEQ_START (Active High)
  memWrite('0x1a[5]',cLane,1,device_name)
  time.sleep (0.2)
  print "DONE = %d" % memRead('0x36[5:5]',cLane,device_name)
  memWrite('0x42[1]',cLane,1,device_name)  #????
  memWrite('0x25[3]',cLane,0,device_name)  #RXCALEYEDIAGFSMIN_LOCWREN (Active Low) p138

  memWrite('0x1ac[1:0]',common,1,device_name) 
  memWrite('0x1ad[7:4]',common,1,device_name)

  memWrite('0x1a8[7]',common,1,device_name)

  memWrite('0x25[2]',cLane,1,device_name) #RXCALEYEDIAGFSMIN_START (1 - Enable)

  memWrite('0x1a8[7]',common,1,device_name)

  memWrite('0x25[2]',cLane,0,device_name) #RXCALEYEDIAGFSMIN_START (0 - Disable)

  print "RX"+str(cLane)
  print "DONE = %d" % memRead('0x36[5:5]',cLane,device_name)
  print  "RXCALEYEDIAGFSM_EYESUM_0x26[7:0]:",memRead('0x26[7:0]',cLane,device_name)
  print  "RXCALEYEDIAGFSM_EYESUM_0x27[5:0]:",memRead('0x27[5:0]',cLane,device_name)
  
  memWrite('0x1d[2]',common,0,device_name) #RXEQ_CALEN RX Equalization FSM enable (Active high) p133
  memWrite('0x1a[5]',cLane,0,device_name)  #RXEQ_PCSRXEQ_START (0-Disable adaptive RX EQ)
