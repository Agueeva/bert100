#include "types.h" 
void MDIO_Init(void);
void MDIO_Address(uint16_t phy_addr,uint16_t devType,uint16_t addr);
void MDIO_Write(uint8_t phy_addr,uint8_t devType,uint16_t value);
uint16_t MDIO_Read(uint8_t phy_addr,uint8_t regAddr);
