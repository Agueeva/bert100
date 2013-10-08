#
# startup_dictionary = {"target":"CDR0"}
# execfile("../scripts/cdr_startup.py",globals(),startup_dictionary)
#
# "target" can be CDR0, CDR1, or GB0

import time
from ipmdio import regWriteAck, regValRead


startup_dictionary = locals()

device = startup_dictionary["target"] + "::"
print "Correcting sampler offsets using device ",device

samplerCorrect = [-1,4,3,2,1,8,7,6,5]  # sampler cross connect table

for rxn in range(0,4): 
  saOveride     = device + '30.' + str(440+(rxn*256)) + '.0'
  regWriteAck(saOveride,0) # enable SA offset override

for rxn in range(0,4):
  amuxOffset  = device + '30.' + str(448+(rxn*256)) + '.13:12'
  forceAZ     = device + '30.' + str(384+(rxn*256)) + '.1'
  completeAZ  = device + '30.' + str(384+(rxn*256)) + '.2'
  regWriteAck(amuxOffset,3) # equalizer offset correction method
  regWriteAck(forceAZ,0) # 
  regWriteAck(forceAZ,1) # begin SA offset autozero fn.
  regWriteAck(forceAZ,0)
  # Check to make sure AZ completed, and wait for it if it didn't.  reg Read takes longer than sleep...
  for i in range(10):
    val = regValRead(completeAZ)
    if val == 0:
      time.sleep(0.1)
    else:
      break

  if val == 0:
    if startup_dictionary["quiet"] == "no":
      raw_input("Auto-zero of lane %d did not complete, hit return to continue" % rxn)
    else:
      print "Auto-zero of lane %d did not complete." % rxn
 
for rxn in range(0,4):
  for sampler in range(1,9): # 8 samplers per Rx
    saoffsetRead  = device + '30.' + str(432+(rxn*256)+sampler-1) + '.15:8' # Adapted offset
    saoffsetWrite = device + '30.' + str(432+(rxn*256)+samplerCorrect[sampler]-1) + '.7:0'

    val = regValRead(saoffsetRead)
    regWriteAck(saoffsetWrite,val)
  
    # print 'Rx%d sampler %d gets the offset from sampler %d' %(rxn,samplerCorrect[sampler],sampler)
    
  saOveride     = device + '30.' + str(440+(rxn*256)) + '.0'
  regWriteAck(saOveride,1) # disable SA offset override
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
