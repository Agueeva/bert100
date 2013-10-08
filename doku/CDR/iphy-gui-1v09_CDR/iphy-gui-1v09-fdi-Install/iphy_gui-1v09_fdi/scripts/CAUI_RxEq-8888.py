from membus import memRead,memWrite
print device_name

#----------------------------------------------------------------- 
# Set xadjust offset for 4pt Eye
print 'Set Xadjust offset for 4pt Eye'
for cLane in range(0,10):
  if cLane < 3:
    common = 'common0'
  elif cLane < 7:
    common = 'common1'
  else:
    common = 'common2'
    
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
  
#--------------------------------------------------------------------------------------------
  
for cLane in range(0,10):

  print "setting lane",cLane
  #memWrite('0x7[0:0]',cLane,0x1)  #LANEPCSP STATE Override Enable (page 117)
  #print "PCSRXEQ_LOCWREN:",memRead('0x1a[6]',cLane)  
  #memWrite('0x1a[6]',cLane,1)

  memWrite('0x15[6:6]',cLane,1,device_name) 
  memWrite('0x15[6:6]',cLane,0,device_name) # RXCALEQ_LOCWREN
  memWrite('0x15[2:0]',cLane,7,device_name) # RXCALEQ_DCGAIN
  memWrite('0x15[5:3]',cLane,0,device_name) # RXCALEQ_DFEPSTAPF3DB
  
  memWrite('0x18[6:4]',cLane,7,device_name) # ATT,lfAGC
  memWrite('0x19[4:0]',cLane,2,device_name) # BOOST,hfAGC


  memWrite('0x16[2:0]',cLane,0,device_name) # DFETAPGAIN
  
  memWrite('0x16[6:3]',cLane,8,device_name) # DFETAP1GAIN
  memWrite('0x17[3:0]',cLane,8,device_name) # DFETAP2GAIN
  memWrite('0x17[7:4]',cLane,8,device_name) # DFETAP3GAIN
  memWrite('0x18[3:0]',cLane,8,device_name) # DFETAP4GAIN


print 'done'
#--------------------------------------------------------------------------------------------
