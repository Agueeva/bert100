# Startup for Gearbox2 A0
VERSION = '0.1'
# Nov-20,2012
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
print "Using device ",device

if debug==1 or regRead(device + '30.3.15:4') == 0x742:
  print "GB2 device detected"
else:
  raise Exception("No GB device detected at the specified PHYADDR")

status = 1

regWrite(device + '30.62',0x0000) # power downs
#regWrite(device + '30.180',0x0000) # refclk div
#regWrite(device + '30.181',0x6262) # refclk div
#regWrite(device + '30.182',0x6262) # refclk div

if startup_dictionary["mode"]=='10_10':
  print "Starting GB2 in 10:10 mode"
  pma_type = 0  # 0=10:10, 1=10:4
else:
  pma_type = 1  # 0=10:10, 1=10:4
  print "Starting GB2 in 10:4 mode"

print "Resetting the GB device using startup "+VERSION+", initiate bring-up..."

# *** Start of main reset sequence ***

print "PMA 1",regRead('GB2::8.7.1')

regWrite(device + "8.0.15", 1)          # apply hard reset
print "8.0=",regRead(device + "8.0.15")          # apply hard reset
regWrite(device + "8.0.15", 1)          # apply hard reset
print "8.0=",regRead(device + "8.0.15")          # apply hard reset
regWrite(device + "8.0.15", 1)          # apply hard reset
print "8.0=",regRead(device + "8.0.15")          # apply hard reset

print "PMA 2",regRead('GB2::8.7.1')
regWrite(device + "8.7", pma_type*0x0002)    # PMA Type

print "PMA 3",regRead('GB2::8.7.1')
regWrite(device + "8.0.15", 0)          # De-assert hard reset
print "PMA 4",regRead('GB2::8.7.1')

regWrite(device + "30.0", 0x0a81)       # assert soft resets, and MMD8 over-ride + mdio init
#regWrite(device + "30.0.7", 1)          # apply mdio init
regWrite(device + "30.0.7", 0)

regWrite(device + "30.0", 0x0001)       # deassert soft resets, keep MMD8 over-ride

# EQ COR Method overwrite - until cal is up
for reg in range(8384,10689,256):
  regWrite(device + '30.'+str(reg),0x1000)
  
for reg in range(4288,6593,256):
  regWrite(device + '30.'+str(reg),0x1000)
  
# check for all AZ completes

time.sleep(0.1)
print "AZ Status   :",bin(regRead(device + "30.0.6:5"))[2:].zfill(2)


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
