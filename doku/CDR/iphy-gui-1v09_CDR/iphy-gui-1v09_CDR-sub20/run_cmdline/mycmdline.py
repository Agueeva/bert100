
# These 3 lines load up the system from 'create_system.txt' in the rundir
# and allow regRead and regWrite to be used.

import iputils
from ipallreg import regRead, regWrite
system = iputils.create_system()
  
# Below this add the test code...
  
print "read:",hex(regRead('GB0::30.2'))
raw_input('Hey, that should have been 0x210!, press a key to keep going...')
  
  
print "done."

