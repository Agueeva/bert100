
#define DAC_EMLAMP1_VG1(ch)      (4 + (ch))
#define DAC_EMLAMP1_VG2(ch)      (8 + (ch))
#define DAC_EMLAMP1_VG3(ch)      (12 + (ch))
#define DAC_EMLAMP1_VD1(ch)      (16 + (ch))
#define DAC_EMLAMP1_VD2(ch)      (20 + (ch))
#define DAC_EMLAMP1_VS(ch)       (24 + (ch))

void AmpEML_Init(const char *name);
