# import ipreg
import sys
import ipvar
import ipconfig
#import iputils
# import ipuser
#-----------------------------------------------------------------------------

def main(device, outFile):
  device = device.upper()
  if device.find('GB')>=0:
    devId = 'iPHY-GB'
  elif device.find('CDR')>=0:
    devId = 'iPHY-CDR'
  else:
    devId = device
  if ipvar.System!=None and devId!=None:
    devDict = ipvar.System[devId].regDefTab
  else:
    devDict = None
  if devDict!=None:
    ipconfig.store_regs_to_csv(outFile, devDict, doScan=False)
    print "Wrote: ", outFile
#-----------------------------------------------------------------------------

if __name__ == "__main__" :
  if len(sys.argv)>2:
    main(sys.argv[1], sys.argv[2])
  else:
    sys.stderr.write("usage: python iptest1.py (gb|cdr) outfile.csv\n")
#------------------------------------------------------------
# 
#  Local Variables:
#  tab-width: 2
#  indent-tabs-mode: nil
#  scroll-step: 1
#  line-number-mode: t
#  column-number-mode: t
#  End:
# 

