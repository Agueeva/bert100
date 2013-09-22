#include "types.h"
void DataFlash_Init(void);
bool DFlash_Erase(uint32_t relAddr,uint32_t len);
bool DFlash_Write(uint32_t flash_addr, void *buf, uint32_t len);
