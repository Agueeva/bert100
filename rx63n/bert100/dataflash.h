#include "types.h"
#define DFLASH_MAP_ADDR(ofs) (0x00100000 + (ofs))

void DataFlash_Init(void);
bool DFlash_Erase(uint32_t relAddr,uint32_t len);
bool DFlash_Write(uint32_t flash_addr, void *buf, uint32_t len);
bool DFlash_BlankCheck2(uint32_t addr);
void DFlash_Lock(void);
void DFlash_Unlock(void);
