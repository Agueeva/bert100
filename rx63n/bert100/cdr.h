void CDR_Init(const char *name);
#if 0
uint16_t Cdr_Read(uint8_t phyAddr,uint16_t regAddr);
void Cdr_Write(uint8_t phyAddr,uint16_t regAddr,uint16_t value);
void Cdr_WritePart(uint16_t phy_addr,uint16_t uRegister,uint8_t uLastBit,uint8_t uStartBit,uint16_t uValue);
uint16_t Cdr_ReadPart(uint16_t phy_addr,uint16_t uRegister,uint8_t uLastBit,uint8_t uStartBit);
#endif
uint64_t Cdr_GetErrCnt(uint8_t cdrID, uint8_t lane);

