void RxCRC_Init(void);
/* 
 ***********************************************************
 * This is the default CRC with Polynom 0x1021 LSB first 
 * (same as on M32C/M16C)
 ***********************************************************
 */
uint16_t CRC16(uint16_t crc, const uint8_t * data, uint32_t count);
uint16_t CRC16_String(const char *str);
/**
 *************************************************************
 * CRC polynomial 0xa001 as used for EVA-DTS DDCMP.
 *************************************************************
 */
uint16_t CRC16_A001(uint16_t crc, const uint8_t * data, uint32_t count);
