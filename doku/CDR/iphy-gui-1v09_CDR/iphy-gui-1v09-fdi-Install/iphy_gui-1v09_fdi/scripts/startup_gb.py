# Startup for Gearbox A0 and B0
VERSION = '2.0'
# Aug-02,2012
#
# Pass in the local dictionary to define what you want to startup:
#
# startup_dictionary = {"target":"GB", "28Gdata":"no", "10Gdata":"no", "quiet":"no"}
# execfile("../scripts/cdr_startup.py",globals(),startup_dictionary)
#
# "target" must be GB0, GB1, ---> GB7
# "28Gdata" can be yes or no
# "10Gdata" can be yes or no
# "quiet" can be yes or no

from ipmdio import regValRead as regRead

A0_device = 0

startup_dictionary = locals()

device = startup_dictionary["target"] + "::"
print "Using device ",device

if regRead(device + '30.3.15:4') == 0x741:
  print "GB device detected"
else:
  raise Exception("No GB device detected at the specified PHYADDR")

if regRead(device + '30.3.3:0') == 0:   # A0 device found, use the read after write fix for clock park
  print "A0 device detected. Clock park correction enabled. Offset AZ and Swizzle enabled."
  A0_device = 1
  from ipmdio import regWriteAck as regWrite
else:     
  from ipmdio import regWrite

# First, check that communications is established (all 1, no part, all 0, no interface):
reply = regRead(device + "30.3.15")
if reply == 1:
  print "Cannot communicate with ",device,", terminating"
  raise Exception("check address...")
reply = regRead(device + "30.3.14")
if reply == 0:
  print "Cannot communicate with ",device,", terminating"
  raise Exception("check interface box...")

status = 1

print "Resetting the GB device using startup "+VERSION+", initiate bring-up..."

# *** Start of main reset sequence ***

regWrite(device + "30.0", 0x0a01)       # assert soft resets, and MMD8 over-ride
regWrite(device + "30.0.7", 1)          # apply mdio init
regWrite(device + "30.0.7", 0)
regWrite(device + "8.0.15", 1)          # apply hard reset

if A0_device == 1: 
  regWrite(device + "30.1536.15", 0)    # A0 requires independant reset of CAUI interface
  
regWrite(device + "8.0.15", 0)          # De-assert hard reset

if A0_device == 1:
  regWrite(device + "30.1536.15", 1)
  
for lane in range(4):                   # Set 128 steps while in soft reset mode
  regWrite(device + "30." + str(385 + 256 * lane) + ".10:8",4)
  
regWrite(device + "30.0.9",0)           # De-asserts Tx Datapath Reset

regWrite(device + "30.38.15",1)         # Asserts Tx FIFO reset
regWrite(device + "30.38.14",1)         # Enable auto-reset
regWrite(device + "30.38.15",0)         # De-Asserts Tx FIFO reset
regRead(device + "30.38")               # Clears FIFO errors

regWrite(device + "30.0.11",0)          # De-Asserts Rx Datapath Reset

for lane in range(4):                   # EQ offset override is set
  regWrite(device + "30." + str(441 + 256 * lane) + ".0",1)
  
if A0_device == 1:                      # Swizzle sampler offset correction
  execfile("../scripts/correct_offsets.py",globals(),startup_dictionary)
  for lane in range(4):                 # Set 128 steps here for A0 (not inside reset)
    regWrite(device + "30." + str(385 + 256 * lane) + ".10:8",4)
  
regWrite(device + "30.37.15",1)         # Asserts Rx FIFO reset
regWrite(device + "30.37.14",1)         # Enable auto-reset
regWrite(device + "30.37.15",0)         # De-Asserts Rx FIFO reset
regRead(device + "30.37")               # Clears Rx FIFO errors

# This section checks the status of the various components

for lane in range(4):                   # Check for AZ completion
  reply = regRead(device + "30." + str(384 + 256 * lane) + ".2")
  if reply == 0:
    if startup_dictionary["quiet"] == "no":
      raw_input("Auto-zero of lane %d did not complete, hit return to continue" % lane)
    else:
      print "Auto-zero of lane %d did not complete." % lane
    status = 0

reply = regRead(device + "30.0.10")
print "Rx Reset Status should be 1:" + str(reply)
if reply==0:
  if startup_dictionary["quiet"] == "no":
    raw_input("  Rx did not RESET, is REFCLK present? enter to continue")
  else:
    print "  Rx did not RESET, is REFCLK present?"
  status = 0
  
if startup_dictionary["10Gdata"] != "no":
  reply = regRead(device + "30.1544")
  print "10G Data path Status should be 0xE3FF: 0x%04x" % reply
  if reply != 0xe3ff:
    if startup_dictionary["quiet"] == "no":
      raw_input("    10G data path not ready.  enter to continue")
    else:
      print "    10G data path not ready."
    status = 0
    
reply = regRead(device + "30.0.8")
print "Tx Reset Status should be 1:" + str(reply)
if reply==0:
  if startup_dictionary["quiet"] == "no":
    raw_input("  Tx did not RESET, is REFCLK present? enter to continue")
  else:
    print "  Tx did not RESET, is REFCLK present?"
  status = 0
    
reply = regRead(device + "30.42.8") 
print "Rx PLL Lock Status should be 1:" + str(reply)
if reply==0:
  if startup_dictionary["quiet"] == "no":
    raw_input("  Rx PLL not locked, is REFCLK present? enter to continue...")
  else:
    print "  Rx PLL not locked, is REFCLK present?"
  status = 0
  
reply = regRead(device + "30.40.8") 
print "Tx PLL Lock Status should be 1:" + str(reply)
if reply==0:
  if startup_dictionary["quiet"] == "no":
    raw_input("  Tx PLL not locked, is REFCLK present and Rx3 PI locked? enter to continue...")
  else:
    print "  Tx PLL not locked, is REFCLK present and Rx3 PI locked?"
  status = 0

reply = regRead(device + "30.0.10")
print "Rx Reset sequence done:" + str(reply)
if reply == 0:
  if startup_dictionary["quiet"] == "no":
    raw_input("  Part did not complete Rx reset sequence? enter to continue")
  else:
    print "  Part did not complete Rx reset sequence."
  status = 0
    
reply = regRead(device + "30.0.8")
print "Tx Reset sequence done:" + str(reply)
if reply == 0:
  if startup_dictionary["quiet"] == "no":
    raw_input("  Part did not complete GB Tx reset sequence? enter to continue...")
  else:
    print "  Part did not complete GB Tx reset sequence."
  status = 0

if status == 1:
  print "The part is now ready for testing."
else:
  print "The part is not ready, consider your options..."
