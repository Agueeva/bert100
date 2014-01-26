#include "database.h"
void AD537x_ModInit(const char *name);
bool DAC_Set(uint8_t channelNr,float value);
bool DAC_Get(uint8_t chNr,float *valRet);

#define DAC_MZAMP1_VG1(ch)	(4 + (ch))
#define DAC_MZAMP1_VG2(ch)	(8 + (ch))
#define DAC_MZAMP1_VG3(ch)	(12 + (ch))
#define DAC_MZAMP1_VD1(ch)	(16 + (ch))
#define DAC_MZAMP1_VD2(ch)	(20 + (ch))
