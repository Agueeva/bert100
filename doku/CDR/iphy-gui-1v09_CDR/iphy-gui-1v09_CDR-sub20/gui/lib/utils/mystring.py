#
# $Header: /projects/wlv/iphy/sos/repository/iphy_A.rep/TEMP/case_1285613542/#alidation#iphy-gui#gui#lib#utils#mystring.py,v 1.1 2011-09-02 11:44:10-07 case Exp $
# $Log: #alidation#iphy-gui#gui#lib#utils#mystring.py,v $
# Revision 1.1  2011-09-02 11:44:10-07  case
# ...No comments entered during checkin...
#
# Revision 1.2  2011-06-30 21:28:53-07  rbemra
# CSV block reader fixes
#
# Revision 1.1  2011-05-04 19:07:24-07  rbemra
# Initial python translation from C: not working yet
#
# No ctypes: direct C to Python translation of csv block parser
#
import sys
import bv
from itertools import repeat
# from mytoken import *
FLOAT_VCTL = 100.0

DataE = {
  'intE': 0, 'floatE': 1, 'complexE': 2, 'stringE': 3,
  'ptrE': 4, 'charE': 5, 'ptrptrE': 6
}

class CsvType:
  def __init__(self, hdr=[], rcTab=[]):
    self.hdrList = hdr # list of |nCol| str type entries # VecType
    self.rcTab = rcTab # [row][col], |nRow| lists, each = list of |nCol| entries
  # self.nRows = nRows # noneed

MAXCOLNAME = 255
MAXLINELEN = (100*(MAXCOLNAME+1)) #  100 rows, 255 long each
DelimS = " ,\t\n"

NumToken = 0
HexToken = 1
FltToken = 2
StrgToken = 3

COMMENT_STR = "!"
HEADER_STR = "#"
DATA_DELIM = " ,\t\n;\r" #  number separators
HEADER_DELIM_STR = ",\t " #
DATA_DELIM_STR = "," #  allows white space infix in name
GIF_Ext = "GIF"
CSV_EOF = -1
# ----------------------------------------------------------------------------------

def lowtoup(strg):
  return strg.upper()
# ----------------------------------------------------------------------------------

def uptolow(strg):
  return strg.lower()
# ----------------------------------------------------------------------------------

def msplit(strg, sDelim):
  if len(sDelim)==1: # recursion terminating condition
    sub1 = strg.split(sDelim)
    curLen = len(sub1)
    while True:
      try:
        sub1.remove('')
      except:
        break # no more empty string entries
    return sub1
  sub1 = msplit(strg, sDelim[0])
  retList = []
  for eStrg in sub1:
    retList.extend(msplit(eStrg, sDelim[1::]))
  return retList
# ----------------------------------------------------------------------------------

def col_sep(nWrote):
  if (1+nWrote)%nCol:
    sep = ','
  else:
    sep = '\n'
  return sep
# ----------------------------------------------------------------------------------
# 
#  return input string, strip/chop leading+trailing whitespace
#
def fgets_chop(pFile, retStrg): # FILE *pFile):
  # whitespace = (space, tab, return, newline, formfeed, vertical tab)

  if pFile!=None and retStrg!=None:
    retStrg = pFile.readline()
  # retStrg = retStrg.lstrip()
  # retStrg = retStrg.rstrip()
  return retStrg.strip()
# ----------------------------------------------------------------------------------------

def token_value(pToken):
  lToken = pToken.lower()
  if lToken[0:2]=="0x":
    retVal = int(lToken, 16)
  if lToken[0:2]=="0b":
    retVal = int(lToken, 2)
  elif (lToken[0:3]=="flt"):
    retVal = FLOAT_VCTL
  elif (lToken.find('.')>=0 or lToken.find('e')>0):
    try:
      retVal = float(pToken)
    except ValueError: # string
      try:
        [_, retVal] = bv.vs2value(pToken)
      except:
        retVal = str(pToken)
  else: # integer
    try:
      retVal = int(pToken)
    except ValueError:
      try:
        [_, retVal] = bv.vs2value(pToken)
      except:
        retVal = str(pToken)
  return retVal
# ----------------------------------------------------------------------------------------

def token_list(sVal, sHdr=None, sDelim=None, sComment=None):
  if sComment!=None:
    (locBuf, _, _) = sVal.partition(sComment) # chop past comment
  else:
    locBuf = sVal
  locBuf = locBuf.strip() # chop lead/trail whitespace
  if locBuf=='':
    return []
  tokenList = msplit(locBuf, sDelim) # locBuf.split(sDelim) # sDelim = ", \t" csv

  nToken = len(tokenList)

  if (nToken and sHdr!=None and len(tokenList[0])>0 and
      tokenList[0][0]==sHdr): # separate out header token
      tokenList[0] = tokenList[0][1::]
      tokenList[0] = tokenList[0].lstrip() # strip any leading ws
      if len(tokenList[0])>0:
        tokenList.insert(0, sHdr) # prepend header
      else:
        tokenList[0] = sHdr
  return tokenList
# ----------------------------------------------------------------------------------------

def read_csv_header(
  hdrList,
  hdrLine, # header string 
  colBeg, #  0 up
  nCols):

  tokenList = token_list(hdrLine, HEADER_STR, HEADER_DELIM_STR, COMMENT_STR)
  if tokenList[0]==HEADER_STR: #  allow first token # for header
    tokStart = 1
  else:
    tokStart = 0

  nDone = kCol = 0
  nCol = len(hdrList)
  for pToken in tokenList[tokStart::]:
    if (kCol >= colBeg):
      hdrList[kCol] = str(pToken)
      nDone += 1
      if (nDone >= nCols or nDone>nCol):
        break
    kCol += 1
# ----------------------------------------------------------------------------------------

def make_default_header(
  hdrList,
  nCols):
  prefix = "col" # char *pStrg, *prefix = "col"
  kCol = 0

  while (kCol < nCols):
    hdrList[kCol] = "%s%d"%(prefix, kCol)
    kCol += 1
# ----------------------------------------------------------------------------------------

def read_row(
  rowList,
  kRow,
  tokenList,
  colBeg, #  0 up
  nCols):

  nDone = kCol = 0
  for pToken in tokenList:
    if (kCol >= colBeg):
      dVal = token_value(pToken)
      # if type(dVal)==str: # dVal.dType==DataE['stringE']:
      #   sys.stderr.write("Warning: row %d, col %d not numeric\n"%
      #     (kRow+1, kCol+1))
      #   sys.stderr.flush()
      rowList[kCol] = dVal
      nDone += 1
      if (nDone >= nCols):
        break
    kCol += 1
# ----------------------------------------------------------------------------------------

def read_csv(
  fName,
  pTab, # CsvType *pTab, #  dynamically allocated
  pMsg):

  pFile = open(fName, "r")
  if (pFile==None):
    raise Exception("Bad filename")

  #  Get header, if exists
  isHdr = False
  nCols = 0
  while (True):
    posFirstDataRow = pFile.tell()
    rowLine = pFile.readline()
    (rowLine, _, _) = rowLine.partition(COMMENT_STR)
    rowLine = rowLine.strip()
    if (rowLine==''):
      raise Exception("Empty file")

    strcpy(hdrLine, rowLine) #  save header
    tokenList = rowLine.split()
    if len(tokenList)==0:
      continue #  empty line
    isHdr = tokenList[0]==HEADER_STR #  allow first token # for header
    if (isHdr):
      currToken = 1
    else:
      currToken = 0
    for pToken in tokenList[currToken::]: #  count columns from header or data row
      nCols += 1
      if (not isHdr):
        dVal = token_value(pToken)
        isHdr = type(dVal)==str # dVal.dType==DataE['stringE']
    break

  if isHdr>0:
    nRows = 1
  else:
    nRows = 0

  while True:
    rowLine = pFile.readline()
    (rowLine, _, _) = rowLine.partition(COMMENT_STR) # chop past comment
    rowLine = rowLine.strip()
    tokenList = rowLine.split()
    if len(tokenList)==0:
      continue #  empty line
    nRows += 1

  #  Allocate memory for columns & rows, initially all entries = None/zero
  ppStrg = pTab.hdrList = list(repeat('',nCols))
  ppRowVec = pTab.rcTab = list(repeat([],nRows))
  if (ppStrg==None or ppRowVec==None):
    raise Exception("read_csv: Allocation of CSV header failed")

  for kRow in range(0,nRows):
    ppRowVec[kRow] = list(repeat(0,nCols))
    if (ppRowVec[kRow]==None):
      raise Exception("read_csv: Allocation of CSV entries failed")

  if (isHdr):
    read_csv_header(pTab.hdrList, hdrLine, 0, nCols)
  else:
    make_default_header(pTab.hdrList, nCols)

  pFile.seek(posFirstDataRow, 0) #  first data row
  kRow = 0
  while kRow<nRows:
    rowLine = pFile.readline()
    tokenList = token_list(rowLine, None, DATA_DELIM_STR, COMMENT_STR)
    if (len(tokenList)==0):
      continue #  non-data lines are not part of rows
    read_row(ppRowVec[kRow], kRow, tokenList, 0, nCols)
    kRow += 1

  return True

 #  read_csv()
# --------------------------------------------------------------
# 
#  From current file-position in pFile,
#  read a "rectangle" of (atmost) maxRows x maxCols data.
#  Move file-position to header of next block, if any
#  Return: False:error, True:success, -1:EOF
#
def read_csv_rect(
  pFile, # FILE
  pTab, # CsvType *pTab, #  dynamically allocated
  colBeg, #  0 up
  colEnd, #  0 up; <colBeg => upto EOL
  rowBeg, #  0 up
  rowEnd): #  0 up; <rowBeg => upto EOF

  if (pFile==None):
    raise Exception("Bad filename")

  #  Get header, if exists
  isHdr = False
  nCols = 0
  prevPos = -1
  #  Read an optional header line
  while (True):
    posFirstDataRow = pFile.tell()
    if (posFirstDataRow==prevPos):
      return
    prevPos = posFirstDataRow
    rowLine = pFile.readline()
    if (rowLine==''): # end of file
      return
    hdrLine = rowLine #  save header
    tokenList = token_list(rowLine, HEADER_STR, HEADER_DELIM_STR, COMMENT_STR)
    if len(tokenList)==0:
      continue #  empty line

    isHdr = tokenList[0]==HEADER_STR #  allow first token # for header
    if (isHdr):
      currToken = 1
    else:
      currToken = 0
    nCols = 0
    for pToken in tokenList[currToken::]: #  count columns from header or data row
      nCols += 1
      if (not isHdr):
        dVal = token_value(pToken)
        isHdr = type(dVal)==str # dVal.dType==DataE['stringE']
    break

  if isHdr:
    nRows = 0
  else:
    nRows = 1
  nextHdr = False
  posNextHdr = -1
  # 
  #  Find # rows of data for this block & file position
  #  to header of next block, if any
  # 
  prevPos = posFirstDataRow
  while True:
    cPos = pFile.tell()
    if (cPos==prevPos):
      break
    prevPos = cPos
    rowLine = pFile.readline()
    tokenList = token_list(rowLine, HEADER_STR, HEADER_DELIM_STR, COMMENT_STR)
    if len(tokenList)==0:
      continue #  empty line

    nextHdr = tokenList[0]==HEADER_STR #  allow first token # for header
    if (nextHdr):
      currToken = 1
    else:
      currToken = 0
    for pToken in tokenList[currToken::]: #  count columns from header or data row
      if (not nextHdr and len(pToken)>0):
        dVal = token_value(pToken)
        nextHdr = type(dVal)==str # dVal.dType==DataE['stringE']

    if (nextHdr):
      posNextHdr = cPos
      break
    elif (nRows==rowBeg):
      posFirstDataRow = cPos
    nRows += 1

  #  Allocate memory for columns & rows, initially all entries = None/zero
  if (colEnd-colBeg >= 0):
    nCols = min(nCols, colEnd-colBeg+1)
  if (rowEnd-rowBeg >= 0):
    nRows = min(nRows, rowEnd-rowBeg+1)
  ppStrg = pTab.hdrList = list(repeat('',nCols))
  ppRowVec = pTab.rcTab = list(repeat([],nRows))
  if (ppStrg==None or ppRowVec==None):
    raise Exception("read_csv_rect: Allocation of CSV header failed")

  for kRow in range(0, nRows):
    ppRowVec[kRow] = list(repeat(0,nCols))
    if (ppRowVec[kRow]==None):
      raise Exception("read_csv_rect: Allocation of CSV entries failed")

  if (isHdr):
    read_csv_header(pTab.hdrList, hdrLine, colBeg, nCols)
  else:
    make_default_header(pTab.hdrList, nCols)

  pFile.seek(posFirstDataRow, 0) #  first data row
  kRow = 0
  while kRow<nRows:
    rowLine = pFile.readline()
    tokenList = token_list(rowLine, None, DATA_DELIM_STR, COMMENT_STR)
    if (len(tokenList)==0):
      continue #  non-data lines are not part of rows
    read_row(ppRowVec[kRow], kRow, tokenList, colBeg, nCols)
    kRow += 1

  if (nextHdr and posNextHdr>0):
    pFile.seek(posNextHdr, 0)
# --------------------------------------------------------------
# 
#  From current file-position in pFile,
#  read a "rectangle" of (atmost) maxRows x maxCols data.
#  Move file-position to header of next block, if any
#  Return: False:error, True:success, -1:EOF
#
def read_csv_rect_head(
  pFile, # FILE
  pTab, # CsvType *pTab, #  dynamically allocated
  colBeg=0, #  0 up
  colEnd=-1, #  0 up; <colBeg => upto EOL
  rowBeg=0, #  0 up
  rowEnd=-1): #  0 up; <rowBeg => upto EOF

  if (pFile==None):
    raise Exception("Bad filename")

  #  Get header
  isHdr = False
  nCols = 0
  prevPos = -1
  #  Read an optional header line
  while (True):
    posFirstDataRow = pFile.tell()
    if (posFirstDataRow==prevPos):
      return # end of file
    prevPos = posFirstDataRow
    rowLine = pFile.readline()
    hdrLine = rowLine #  save header
    tokenList = token_list(rowLine, HEADER_STR, HEADER_DELIM_STR, COMMENT_STR)
    if len(tokenList)==0:
      continue #  empty line

    isHdr = tokenList[0]==HEADER_STR #  allow first token # for header
    nCols = len(tokenList)
    if (isHdr):
      nCols -= 1
    break

  if isHdr:
    nRows = 0
  else:
    nRows = 1
  nextHdr = False
  posNextHdr = -1
  # 
  #  Find # rows of data for this block & file position
  #  to header of next block, if any
  # 
  prevPos = posFirstDataRow
  while True:
    cPos = pFile.tell()
    if (cPos==prevPos):
      break
    prevPos = cPos
    rowLine = pFile.readline()
    tokenList = token_list(rowLine, HEADER_STR, HEADER_DELIM_STR, COMMENT_STR)
    if len(tokenList)==0:
      continue #  empty line
    nextHdr = tokenList[0]==HEADER_STR #  allow first token # for header
    if (nextHdr):
      currToken = 1
    else:
      currToken = 0

    if (nextHdr):
      posNextHdr = cPos
      break
    elif (nRows==rowBeg):
      posFirstDataRow = cPos
    nRows += 1

  #  Allocate memory for columns & rows, initially all entries = None/zero
  if (colEnd-colBeg >= 0):
    nCols = min(nCols, colEnd-colBeg+1)
  if (rowEnd-rowBeg >= 0):
    nRows = min(nRows, rowEnd-rowBeg+1)
  ppStrg = pTab.hdrList = list(repeat('', nCols))
  ppRowVec = pTab.rcTab = list(repeat([], nRows))
  if (ppStrg==None or ppRowVec==None):
    raise Exception("read_csv_rect_head: Allocation of CSV header failed")

  for kRow in range(0, nRows):
    ppRowVec[kRow] = list(repeat(0,nCols))
    if (ppRowVec[kRow]==None):
      raise Exception("read_csv_rect_head: Allocation of CSV entries failed")

  if (isHdr):
    read_csv_header(pTab.hdrList, hdrLine, colBeg, nCols)
  else:
    make_default_header(pTab.hdrList, nCols)

  pFile.seek(posFirstDataRow, 0) #  first data row
  kRow = 0
  while kRow<nRows:
    rowLine = pFile.readline()
    tokenList = token_list(rowLine, None, DATA_DELIM_STR, COMMENT_STR)
    if (len(tokenList)==0):
      continue #  non-data lines are not part of rows
    read_row(ppRowVec[kRow], kRow, tokenList, colBeg, nCols)
    kRow += 1

  if (nextHdr and posNextHdr>0):
    pFile.seek(posNextHdr, 0)

# --------------------------------------------------------------

def get_col_vec(
  pTab, # CsvType *pTab,
  colName,
  caseSense): # boolean caseSense):

  pHdr = pTab.hdrList
  nCols = len(pHdr)
  cList = None

  if caseSense:
    for kCol in range(0, nCols):
      if (pHdr[kCol]==colName):
        break
  else:
    colName = colName.upper()
    for kCol in range(0, nCols):
      if (pHdr[kCol].upper()==colName):
        break
  if kCol>=nCols:
    cList = None
  else:
    cList = []
    for rowList in pTab.rcTab:
      cList.append(rowList[kCol])
  return cList
# --------------------------------------------------------------

def dump_val(pVal, pOut):
  if (pOut==None):
    pOut = sys.stdout

# --------------------------------------------------------------

#define MIN_INT_TRUNC 1E-6

def dump_csv(
  pTab, # CsvType *pTab,
  pOut=sys.stdout, # FILE *pOut,
  colBeg=0,
  colEnd=None,
  rowBeg=0,
  rowEnd=None,
  doRowNums=True):

  pHdr = pTab.hdrList
  nCols = len(pHdr)
  if colEnd==None:
    colEnd = nCols-1 # all upto end
  if (nCols>0):
    for kCol in range(colBeg, min(colEnd+1, nCols)):
      if kCol==colBeg:
        sep = HEADER_STR
      else:
        sep = ','
      pOut.write("%c%s"%(sep, pHdr[kCol]))
    pOut.write("\n")
    pOut.flush()

  nRows = len(pTab.rcTab)
  if rowEnd==None:
    rowEnd = nRows-1 # all upto end
  if (nRows>0):
    for kRow in range(rowBeg, min(rowEnd+1, nRows)):
      ppVec = pTab.rcTab[kRow]
      if doRowNums:
        pOut.write("%d) "%kRow)
      for kCol in range(colBeg, min(colEnd+1, nCols)):
        # dump_val(ppVec[kCol], pOut)
        val = ppVec[kCol]
        if kCol==colBeg:
          sep = ''
        else:
          sep = ','
        # iVal = int(fVal)
        # isInt = fabs(fVal - int(fVal))<MIN_INT_TRUNC # check this: necessary?
        if type(val)==int:
          pOut.write("%s%d"%(sep, val))
        elif type(val)==long:
          pOut.write("%s%ld"%(sep, val))
        elif type(val)==float:
          pOut.write("%s%.3f"%(sep, val))
        else:
          pOut.write(sep+val)
      pOut.write("\n")
      pOut.flush()
# ----------------------------------------------------------------------------------

def dump_csv_user(
  pFile, # FILE
  pTab): # CsvType *pTab):

  pHdr = pTab.hdrList
  nCols = len(pHdr)
  nRows = len(pTab.rcTab)
  if (pFile==None):
    pFile = sys.stderr

  while (True):
    print("\nEnter column range[0, %d]:"%(nCols-1))
    sBuf = fgets_chop(sys.stdin)
    if (sBuf==None or len(sBuf)<=0):
      break
    tokList = token_list(sBuf)
    colBeg = max(0, token_value(tokList[0]))
    colEnd = min(nCols-1, token_value(tokList[1]))
    print("\nEnter row range[0, %d]:"%(nRows-1))
    sBuf = fgets_chop(sys.stdin)
    if (sBuf==None or len(sBuf)<=0):
      break
    tokList = token_list(sBuf)
    rowBeg = max(0, token_value(tokList[0]))
    rowEnd = min(nRows-1, token_value(tokList[1]))
    dump_csv(pTab, pFile, colBeg, colEnd, rowBeg, rowEnd)
# ----------------------------------------------------------------------------------

def dump_vec(
  pData, # VecType
  pFile, # FILE
  kBeg, # unsigned kBeg,
  kEnd, # unsigned kEnd,
  nCol, # num. columns to print per line
  fScale, # float fScale,
  pFmt):

  oneFmt = " "
  cplxFmt = " "

  if (pFile==None):
    pFile = sys.stderr
  dType = type(pData[0])
  if (pFmt!=None):
    oneFmt = oneFmt + pFmt + "%c"
    cplxFmt = cplxFmt + pFmt + ","
    cplxFmt = cplxFmt + pFmt + "%c"
  else:
    if (dType==float):
      oneFmt = oneFmt + "%.3f%c"
    elif (dType==int):
      oneFmt = oneFmt + "d%c"
    else:
      oneFmt = oneFmt + "%c%c"
    cplxFmt = cplxFmt + "%.3f,%.3f%c"
  if (dType == int):
    for nWrote in range(0, kEnd-kBeg+1):
      pFile.write(oneFmt%(pData[nWrote], col_sep(nWrote)))
  elif (pData.dType == floatE):
    for nWrote in range(0, kEnd-kBeg+1):
      pFile.write(oneFmt%(fScale*pData[nWrote], col_sep(nWrote)))
  elif (pData.dType == complexE):
    for nWrote in range(0, kEnd-kBeg+1):
      zv = pData[nWrote]
      nWrote += 1
      pFile.write(cplxFmt%(fScale*zv.real, fScale*zv.imag, col_sep(nWrote)))
  else:
    for nWrote in range(0, kEnd-kBeg+1):
      pFile.write(" \"%s\"%c"%(pData[nWrote], col_sep(nCol)))
# ----------------------------------------------------------------------------------

def dump_vec_user( # TODO
  pData, # VecType
  pFile, # FILE
  nCol):

  if (pFile==None):
    pFile = sys.stderr
  while (True):
    print("\nEnter range(0, %d) [scale [fmt]]:"%(pData.nVal-1))
    sBuf = fgets_chop(sys.stdin)
    if (sBuf==None or len(sBuf)<=0):
      break
    tokList = token_list(sBuf)
    kBeg = max(0, token_value(tokList[0]))
    kEnd = min(len(pData)-1, token_value(tokList[1]))
    pCmd = strtok(None, DATA_DELIM)
    fScale = 1.0
    pFmt = None
    if (len(tokList)>2):
      fScale = token_value(tokList[2])
      if (len(tokList)>3):
        pFmt = token_value(tokList[3])
    dump_vec(pData, pFile, kBeg, kEnd, nCol, fScale, pFmt)
#---------------------------------------------------------------------------- 

def run(csvName):
  pFile = open(csvName, "rb")
  if (pFile==None):
    raise Exception("Bad filename")

  nBlk = 0
  while True:
    csvTab = CsvType()
    read_csv_rect_head(pFile, csvTab)
    if len(csvTab.hdrList)<=0:
      break
    print "------- Block %d of CSV file read ok ----------\n"%(nBlk)
    nBlk += 1
    # dump_csv(csvTab, sys.stdout, 0, len(csvTab.hdrList)-1, 0, len(csvTab.rcTab)-1)
    # dump_csv(csvTab)
    dump_csv(csvTab, doRowNums=False)

  pFile.close()
  return True
#---------------------------------------------------------------------------- 
#  Local Variables:
#  tab-width: 2
#  indent-tabs-mode: nil
#  scroll-step: 1
#  line-number-mode: t
#  column-number-mode: t
#  End:
# 
