from ipmdio import regValRead, regWrite
import re

def memRawWrite(addr,macro,wdat,device='GB0',debug=False):
  mem_addr = device + '::30.1344'
  mem_wr   = device + '::30.1345.8'
  mem_wall = device + '::30.1345'
  WRITE    = 1
  wall = (1 << (9+macro)) + (WRITE << 8) + wdat
  regWrite(mem_addr,addr)
  regWrite(mem_wall,wall)
  regWrite(mem_wall,0)
  if(debug):
    print '-'* 70
    print 'memRawWrite'
    print 'mem_addr: %s - 0x%04x' % (mem_addr, addr)
    print 'mem_wall: %s - 0x%04x' % (mem_wall, wall)
  
  return()
 
def memWrite(offset_addr,lane,wdat,device='GB0',debug=False):
  global memRawWrite
  laneDecode = { 0 : (0,1),
                '0': (0,1),
                 1 : (0,2),
                '1': (0,2),
                 2 : (0,3),
                '2': (0,3),
                 3 : (1,0),
                '3': (1,0), 
                 4 : (1,1),
                '4': (1,1), 
                 5 : (1,2),
                '5': (1,2),
                 6 : (1,3),
                '6': (1,3),
                 7 : (2,0),
                '7': (2,0),
                 8 : (2,1),
                '8': (2,1),
                 9 : (2,2),
                '9': (2,2),
                'common0': (0,4),
                'common1': (1,4),
                'common2': (2,4)}  
  (macro,ip_lane) = laneDecode[lane]  #macro, IP lane
  nbits=8
  s=0
  if(debug): print 'memWrite: %s = 0x%02x'%(offset_addr,wdat)
  if isinstance(offset_addr,str):
    m = re.search('(\w+)\[(\d+):*(\d*)\]',offset_addr)
    if m:
      offset_addr = m.group(1)
      hi   = m.group(2)
      lo   = m.group(3)
      if lo:
        s = int(lo)
        nbits = int(hi)-int(lo)+1
      else:
        s = int(hi)
        nbits = 1
        
    offset_addr = int(offset_addr,16)
  if(debug):
    print '0x%x: s=%d, n=%d'%(offset_addr,s,nbits)

  # 8/24 Add device to parameter list
  # GJL - 10/28
  #Construct IP Address
  ip_address = ((ip_lane<<13) + offset_addr)

  #GJL 10/28 - Read modify write with n < 8 bits
  if (nbits < 8):
  #readVal = memRawRead((iplane<<13)+offset_addr,macro,device) 
    readVal = memRawRead(ip_address,macro,device) 
    myReadBin = bin(readVal)[2:].zfill(8) + 'x'
    wdatBin   = bin(wdat<<s)[2:].zfill(8) + 'x'
    value2writeBin = myReadBin[0:8-s-nbits] + wdatBin[8 - s - nbits:8 - s] + myReadBin[8-s:8]
    value2write = int(value2writeBin,2)
    if(debug):
      print 'readVal:   0x%x'%readVal
      print 'myReadBin: ',myReadBin
      print 'wdatBin:   ',wdatBin
      print 'value2writeBin: ',value2writeBin
      print 'value2write:   0x%02x'%value2write
  else:
    value2write = wdat
  if(debug): print 'memRawWrite(0x%x, %d,0x%x, %s)'%(ip_address,macro,value2write,device)
  memRawWrite(ip_address,macro,value2write,device,debug)
  return()
  
def memRawRead(addr,macro,device='GB0',debug=False):
  mem_addr = device + '::30.1344'
  mem_req  = device + '::30.1345.' + str(9+macro)
  mem_wr   = device + '::30.1345.8'
  READ     = 0
  if   macro == 0:
    mem_rd  = device + '::30.1346.7:0'
  elif macro == 1:
    mem_rd  = device + '::30.1346.15:8'
  elif macro == 2:
    mem_rd  = device + '::30.1347.7:0'

  regWrite(mem_addr,addr)
  regWrite(mem_wr,READ)
  regWrite(mem_req,0)
  regWrite(mem_req,1)
  regWrite(mem_req,0)
  val = regValRead(mem_rd)
  if(debug):
    print '-'* 70
    print 'memRawRead'
    print 'mem_addr: %s - 0x%x' % (mem_addr, addr)
    print 'mem_wr: %s' % (mem_wr)
    print 'mem_req:%s' % (mem_req)
    print 'mem_rd: %s, val: 0x%x' % (mem_rd,val)
  
  return(val)
  
def memRead(offset_addr,lane,device='GB0'):
  """memBus Read. lane = 0-9 for caui or 'common0-2' for macro0,1,2 common page"""
  global memRawRead
  laneDecode ={  0 : (0,1),
                '0': (0,1),
                 1 : (0,2),
                '1': (0,2),
                 2 : (0,3),
                '2': (0,3),
                 3 : (1,0),
                '3': (1,0), 
                 4 : (1,1),
                '4': (1,1), 
                 5 : (1,2),
                '5': (1,2),
                 6 : (1,3),
                '6': (1,3),
                 7 : (2,0),
                '7': (2,0),
                 8 : (2,1),
                '8': (2,1),
                 9 : (2,2),
                '9': (2,2),
                'common0': (0,4),
                'common1': (1,4),
                'common2': (2,4)}
  (macro,iplane) = laneDecode[lane]
  n=8
  s=0
  if isinstance(offset_addr,str):
    m = re.search('(\w+)\[(\d+):*(\d*)\]',offset_addr)
    if m:
      offset_addr = m.group(1)
      hi   = m.group(2)
      lo   = m.group(3)
      if lo:
        s = int(lo)
        n = int(hi)-int(lo)+1
      else:
        s = int(hi)
        n = 1
        
    offset_addr = int(offset_addr,16)

# readVal = memRawRead((iplane<<13)+offset_addr,macro) 
# 8/24 - Add device to parameter list
  readVal = memRawRead((iplane<<13)+offset_addr,macro,device)   
  myBin = bin(readVal)[2:].zfill(8) + 'x'
  valueback = int(myBin[8 - s - n:8 - s],2)

  return(valueback)
  
