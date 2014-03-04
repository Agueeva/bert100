void ModReg_Init(void);
void ModReg_SetKi(uint8_t chNr, float Ki);
float ModReg_GetKi(uint8_t chNr);
void ModReg_ResetCtrlFault(uint8_t chNr);
#define DBKEY_MODREG_RECT_DELAY DBKEY_MODREG(0)
#define DBKEY_MODREG_FAULT_LIMIT DBKEY_MODREG(1)
