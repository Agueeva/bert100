# Startup for Gearbox2 A0
VERSION = '0.6'
# Jan-04,2013
#
# Pass in the local dictionary to define what you want to startup:
#
# startup_dictionary = {"target":"GB", "28Gdata":"no", "10Gdata":"no", "quiet":"no"}
# execfile("../scripts/startup_gb2.py",globals(),startup_dictionary)
#
# "target" must be GB0, GB1, ---> GB7
# "28Gdata" can be yes or no
# "10Gdata" can be yes or no
# "quiet" can be yes or no

from ipmdio import regValRead as regRead
from ipmdio import regWrite
import time
  
debug = 1

startup_dictionary = locals()

device = startup_dictionary["target"] + "::"

if debug==1 or regRead(device + '30.3.15:4') == 0x742:
  print "GB2 device detected"
else:
  raise Exception("No GB device detected at the specified PHYADDR")

status = 1

reg_div = 0x0005
reg_trim1 = 0x6262
reg_trim2 = 0x6262

if startup_dictionary["mode"]=='10_10':
  print "Starting GB2 in 10:10 mode"
  pma_type = 0  # 0=10:10, 1=10:4
else:
  pma_type = 1  # 0=10:10, 1=10:4
  print "Starting GB2 in 10:4 mode"

print "Resetting the GB device using startup "+VERSION+", initiate bring-up..."

# *** Start of main reset sequence ***

regWrite(device + "30.0", 0x0A01) # Cycle the MDIO at power up to confirm state machine is sync'd
regWrite(device + "30.0", 0x0A01) 
regWrite(device + "30.0", 0x0A01) 

# setup registers that are not covered by MDIO_INIT

regWrite(device + "8.0", 0x8000)    # apply hard reset

regWrite(device + "8.7", 0x0002*pma_type)    # PMA Type

regWrite(device + '30.62',0x0000)   # 10G lanes power up (10G lanes in 25G are ignored)

regWrite(device + '30.180',reg_div)  # refclk div
regWrite(device + '30.181',reg_trim1)  # PLL trim
regWrite(device + '30.182',reg_trim2)  # PLL trim
regWrite(device + "30.0", 0x0A81)   # Set MDIO_INIT
regWrite(device + "30.0", 0x0A01)   # Clear MDIO_INIT
regWrite(device + "8.0", 0x8000)      # Assert Hard reset again (MDIO_INIT will have cleared it - so reapply)
regWrite(device + "8.0", 0x0000)      # De-assert hard reset
time.sleep(0.050)

# set the eq offset override

for reg in range(0x1080+0x39,0x1980+0x39+1,0x100):
  regWrite(device + "30."+str(reg)+".0",1)
for reg in range(0x2080+0x39,0x2980+0x39+1,0x100):
  regWrite(device + "30."+str(reg)+".0",1)
 
regWrite(device + "30.0", 0x0001)       # deassert soft resets, keep MMD8 over-ride

if (regRead(device + '30.3968.0')) == 0:   # INTERNAL: Check if the part is fused
  print "\n****  WARNING:  THIS PART IS NOT FUSED  ***\n"
  
  # EQ COR Method overwrite - until cal is up
  for reg in range(8384,10689,256):
    regWrite(device + '30.'+str(reg),0x1000)
    
  for reg in range(4288,6593,256):
    regWrite(device + '30.'+str(reg),0x1000)
    
  for reg in range(0x1080+0x39,0x1980+0x39+1,0x100):
    regWrite(device + "30."+str(reg)+".15:0",0x1f7d)
  for reg in range(0x2080+0x39,0x2980+0x39+1,0x100):
    regWrite(device + "30."+str(reg)+".15:0",0x1f7d)
    
else:  
  part_id = regRead(device + '30.3968.15:1')
  print "PARTID = ",part_id
  
 
print "EFUSE DONE Status:",regRead(device + "30.3840.13")

print "PLL Status:",regRead(device + "30.194.9"),regRead(device + "30.197.9"),regRead(device + "30.200.9"),regRead(device + "30.203.9")


# force AZ, first apply 
for reg in range(4224,6528+1,0x100):
  regWrite(device + "30."+str(reg)+".1",1)

for reg in range(8320,10624+1,0x100):
  regWrite(device + "30."+str(reg)+".1",1)

time.sleep(1)

# ...then remove to start AZ sequence
print "AZ Status Lanes (Optical):",
for reg in range(4224,6528+1,0x100):
  regWrite(device + "30."+str(reg)+".1",0)
  print regRead(device + "30."+str(reg)+".2"),

print "\nAZ Status Lanes (Host)   :", 
for reg in range(8320,10624+1,0x100):
  regWrite(device + "30."+str(reg)+".1",0)
  print regRead(device + "30."+str(reg)+".2"),
print

time.sleep(0.1)

print "AZ Status   :",regRead(device + "30.0.6"),regRead(device + "30.0.5")
print "Reset Status:",regRead(device + "30.0.10"),regRead(device + "30.0.8")

#regWrite(device + "30.0.9",0)           # De-asserts Tx Datapath Reset

regWrite(device + "30.88",0xc000)    # Asserts Egress FIFO reset
regWrite(device + "30.88",0x4000)    # Deasserts Egress FIFO reset + auto-reset
regRead(device + "30.88")            # Read to clear
regRead(device + "30.88")            # Read to clear history

regWrite(device + "30.80",0xc000)    # Asserts Egress FIFO reset
regWrite(device + "30.80",0x4000)    # Deasserts Egress FIFO reset + auto-reset
regRead(device + "30.80")            # Read to clear
regRead(device + "30.80")            # Read to clear history

# This section checks the status of the various components


if status == 1:
  print "The part is now ready for testing."
else:
  print "The part is not ready, consider your options..."
