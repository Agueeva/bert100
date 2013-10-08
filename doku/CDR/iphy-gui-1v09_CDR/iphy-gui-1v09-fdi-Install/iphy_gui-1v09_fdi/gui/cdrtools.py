import wx
import time
import math
from ipallreg import regRead
from ipallreg import regWrite


def qphase2pcode(quad,phase):
  """converts a quad/phase value into a 0-512 wheel value"""
  codeBin = (quad << 8) + phase
  codeBin1 = bin(codeBin)[2:].zfill(10)
  codeBin2 = codeBin1[0] + codeBin1[2:]
  return(int(codeBin2,2))
  
def phasereg2pcode(reg_val):
  """converts the raw register value to a 0-512 phase code"""
  codeBin1 = bin(int(reg_val))[2:].zfill(10)
  codeBin2 = codeBin1[0] + codeBin1[2:]
  return(int(codeBin2,2))
  
def readPI(regString):
  """returns a 0-512 phase wheel code from reading the regString input"""
  codeBin1 = bin(regRead(regString))[2:].zfill(10)
  codeBin2 = codeBin1[0] + codeBin1[2:]
  return(int(codeBin2,2))

def pcode2qphase(code):
  """returns the quad/phase as a tuple from a 0-512 phase wheel code input"""
  quad_mapping = {"00":128,"01":256,"11":384,"10":512}
  if code <128:
    quad = 0
    phase = code
  elif code <256:
    quad = 1
    phase = code
  elif code <384:
    quad = 3
    phase = code-256
  else:
    quad = 2
    phase = code-256
  return(quad,phase)
  
def doPhaseCapture(device,rxn,count=10,resultFile='pi_out.csv'):
  """expects a synchronous link setup, then captures the phase interpolator"""
  global readPI, pcode2qphase
  device = device+'::'
  ID_lo = regRead(device+'::30.2')
  if int(ID_lo) != 0x0210:
    print 
    print '***************************************'
    print '***   MDIO COMMUNICATION IS DOWN    ***'
    print '***************************************'

  # Setup the registers we'll need...
  piCodeStr  = device + '30.' + str(424 + (rxn*256)) + '.9:0'
  phase = []
  for eachPI in range(0,count):
    phase.append(readPI(piCodeStr))
    #print phase[-1]
  return(phase)
  
def doBathtub(device,rxn,resultFile='bath_out.csv'):
  """expects a synchronous link setup and pattern gen + ver running, then performs bathtub from current PI posistion and returns."""
  global readPI, pcode2qphase
  device = device+'::'
  ID_lo = regRead(device+'::30.2')
  if int(ID_lo) != 0x0210:
    print 
    print '***************************************'
    print '***   MDIO COMMUNICATION IS DOWN    ***'
    print '***************************************'

  # Setup the registers we'll need...
  piCodeStr  = device + '30.' + str(424 + (rxn*256)) + '.9:0'
  piLock     = device + '30.' + str(416 + (rxn*256)) + '.1'  
  piLock3    = device + '30.' + str(416 +   (3*256)) + '.1' # rx3 only 
  piOverride = device + '30.' + str(416 + (rxn*256)) + '.0'
  piquad0  = device + '30.' + str(416 + (rxn*256)) + '.9:8'
  piquad1  = device + '30.' + str(416 + (rxn*256)) + '.11:10'
  piquad2  = device + '30.' + str(416 + (rxn*256)) + '.13:12'
  piquad3  = device + '30.' + str(416 + (rxn*256)) + '.15:14'

  piOver0  = device + '30.' + str(417 + (rxn*256)) + '.7:0'
  piOver1  = device + '30.' + str(417 + (rxn*256)) + '.15:8'
  piOver2  = device + '30.' + str(418 + (rxn*256)) + '.7:0'
  piOver3  = device + '30.' + str(418 + (rxn*256)) + '.15:8'
  if device.find('GB') ==0: 
    rxError    = device + '8.' + str(1700 + rxn)
  else:
    rxError    = device + '30.' + str(48 + rxn)
  rxPatLock  = device + '30.' + str(16 + rxn) + '.15'

  # Steps for the bathtub...
  # check the links is synchronous and pattern locked OK
  # release override to let it adapt correctly
  regWrite(piOverride,0)
  time.sleep(0.1)
  print "Checking error free..."
  regRead(rxError)  # flush
  time.sleep(0.1)   # settle
  err = regRead(rxError)
  lock = regRead(rxPatLock)
  if lock == 0:
    print "Pattern verifier is not locked"
  elif err == 0:
    print "Pattern locked and error free at the start of the test"  
  else:
    print "Pattern locked, but Errors seen while doing initial link check"
    
  # capture the current PI value
  starting_phase = readPI(piCodeStr)
  (quad0,phase0)   = pcode2qphase(starting_phase)

  # setup the PI override values
  # main PI gets the real value 
  regWrite(piquad0,quad0)
  regWrite(piOver0,phase0)
  print "0:",quad0,phase0
  # next PI gets PI0 + 64
  (quad1,phase1)   = pcode2qphase(starting_phase+64)
  regWrite(piquad1,quad1)
  regWrite(piOver1,phase1)
  print "1:",quad1,phase1
  # +64 again
  (quad2,phase2)   = pcode2qphase(starting_phase+128)
  regWrite(piquad2,quad2)
  regWrite(piOver2,phase2)
  print "2:",quad2,phase2
  # +64 again
  (quad3,phase3)   = pcode2qphase(starting_phase+196)
  regWrite(piquad3,quad3)
  regWrite(piOver3,phase3)
  print "3:",quad3,phase3  
  # now overide to set these values in
  regWrite(piOverride,1)

# step backward 64 steps  

# step forward 128 steps  
# - each step record: phase, error, lock, EQ state
  

#doBathtub('CDR0',0,'bout,csv')

  
# for loop in [0.35]: #,3.5,35,350]:
	# fid1 = open('zpi_out_eqoff'+str(loop)+'.csv','w')
	# for i_pi in range(0,1000):
	  # codeBin = regRead(piCodeStr)
	  # codeBin1 = bin(codeBin)[2:].zfill(10)
	  # codeBin2 = codeBin1[0] + codeBin1[2:]
	  # code = int(codeBin2,2)
	  # outstring = str(code)
	  # fid1.write('%d\n'% code)
	# fid1.close()

	# fid2 = open('zbath_eqoff'+str(loop)+'.csv','w')
	# regWrite(piOverride,1)
	  
	# for basePhase in range(0,128):
	  # regWrite(piOver0,basePhase)
	  # regWrite(piOver1,basePhase+64)
	  # regWrite(piOver2,basePhase)
	  # regWrite(piOver3,basePhase+64)
	  # wx.Yield()
	  # time.sleep(0.05)  # settle
	  # regRead(rxError)  # flush
	  #time.sleep(loop)
	  # err = regRead(rxError)
	  # lock = regRead(rxPatLock)
	  # fid2.write('%d,%d,%d\n' % (basePhase,err,lock))
	  # wx.Yield()
	  # print 'basePhase=',basePhase,"ERR:", err,lock
	# fid2.close()
	# regWrite(piOverride,0)
	# time.sleep(0.5)


#print "Done"
