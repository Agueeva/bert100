void CDR_Init(void);

uint16_t Cdr_Read(uint8_t phyAddr,uint16_t regAddr);
void Cdr_Write(uint8_t phyAddr,uint16_t regAddr,uint16_t value);
void iShuffleRead(uint16_t* uOutp, uint16_t uInp,uint8_t uStartBit,uint8_t uNumBit);
void iShuffle(uint16_t* uOutp, uint16_t uInp,uint8_t uStartBit,uint8_t uNumBit,uint8_t uNumber);
void Cdr_Write_Part(uint16_t phy_addr,uint16_t uRegister,uint8_t uNumBit,uint8_t uStartBit,uint8_t uNumber);
void Cdr_Read_Part(uint16_t* uOutp,uint16_t phy_addr,uint16_t uRegister,uint8_t uNumBit,uint8_t uStartBit);


