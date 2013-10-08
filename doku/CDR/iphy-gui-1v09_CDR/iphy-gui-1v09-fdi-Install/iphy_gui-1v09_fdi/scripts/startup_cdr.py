# Startup for CDR A0 and B0
# V2.0 July-08,2012
# V3.0 18 July,2012, M. Case, added EQ offset over-ride as final step, applies to A0 and B0.
VERSION = '3.1'
# 3.1: rjrbw changed to match gb startup with startup_dictionary (again)

import time
from ipmdio import regValRead as regRead

A0_device = 0

startup_dictionary = locals()

device = startup_dictionary["target"] + "::"
print "Using device ",device

if regRead(device + '30.3.15:4') == 0x740:
  print "CDR device detected"
else:
  raise Exception("No CDR device detected at the specified PHYADDR")
  
if regRead(device + '30.3.3:0') == 0:             # A0 device found, use the read after write fix for clock park
  print "A0 device detected. Clock park correction enabled."
  A0_device = 1
  from ipmdio import regWriteAck as regWrite
else:     
  from ipmdio import regWrite

status = 1

print "Resetting the CDR device using startup "+VERSION+", initiate bring-up..."

# Reset the device, assume normal bring-up

regWrite(device + "30.0",0x1020) # Hard reset (bit 5) and MDIO init (bit 12)

regWrite(device + "30.0",0x0200) # Datapath soft reset (bit 9)

regWrite(device + "30.385.10:8",4)      # Set 128 steps for PI resolution - inside reset"
regWrite(device + "30.641.10:8",4)
regWrite(device + "30.897.10:8",4)
regWrite(device + "30.1153.10:8",4) 
regWrite(device + "30.0",0x0000)
  
# CDR recal of TxPLL while PI3 is locked
regWrite(device + "30.1184.1",1)        # Locking Rx3 PI
# Re-Calibrating Tx PLL
regWrite(device + "30.39.15",0)         # normal mode
regWrite(device + "30.39.15",1)         # re-calibrate
regWrite(device + "30.39.15",0)         # normal mode
time.sleep(0.1)                         # wait for it to lock
regWrite(device + "30.1184.1",0)        # and unlock PI3
time.sleep(0.1)

for lane in range(4):                   # EQ offset override is set
  regWrite(device + "30." + str(441 + 256 * lane) + ".0",1)
  
if A0_device == 1:
  print "Implemening swizzle sampler offset correction (only for A0/A1 stepping)"
  execfile("../scripts/correct_offsets.py",globals(),startup_dictionary)
  for lane in range(4):                 # Set 128 steps here for A0 (not inside reset)
    regWrite(device + "30." + str(385 + 256 * lane) + ".10:8",4)
  
regWrite(device + "30.37.15",1) # Asserts FIFO reset
regWrite(device + "30.37.14",1) # Enable auto-reset
regWrite(device + "30.37.15",0) # De-Asserts FIFO reset

# This section just checks things did complete are startup OK

reply = regRead(device + "30.42.8") 
print "Rx PLL Lock Status should be 1:" + str(reply)
if reply==0:
  print "***  Rx PLL not locked, is REFCLK present?"
  status = 0

reply = regRead(device + "30.40.8") 
print "Tx PLL Lock Status should be 1:" + str(reply)
if reply==0:
  print "***  Tx PLL not locked, is REFCLK present and Rx3 PI locked?"
  status = 0
    
reply = regRead(device + "30.0.8")
print "GB Tx or CDR Reset sequence done:" + str(reply)
if reply == 0:
  print "***  Part did not complete GB Tx or CDR reset sequence"
  status = 0

print "Enabling calibrated EQ offsets"
for lane in range(4):
  regWrite(device + "30." + str(441 + 256 * lane) + ".0",1)

if status == 1:
  print "The part is now ready for testing."
else:
  print "The part is not ready, expect limited or no functionality"

