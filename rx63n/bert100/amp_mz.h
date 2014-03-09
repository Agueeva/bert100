/**
 ************************************************************************
 * Define the DAC channel numbers for the MZ Variant
 ************************************************************************
 */
#define DAC_MZAMP1_VG1(ch)      (4 + (ch))
#define DAC_MZAMP1_VG2(ch)      (8 + (ch))
#define DAC_MZAMP1_VG3(ch)      (12 + (ch))
#define DAC_MZAMP1_VD1(ch)      (16 + (ch))
#define DAC_MZAMP1_VD2(ch)      (20 + (ch))

void AmpMZ_Init(const char *name);
