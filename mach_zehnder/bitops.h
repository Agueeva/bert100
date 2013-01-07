/**
 */

/**
 **********************************************************************
 * \fn static inline uint8_t get_bit(const uint8_t * buf, uint8_t bit);
 * get_bit
 *      Read a bit from a byte array.
 **********************************************************************
 */
uint8_t get_bit(const uint8_t * buf, uint8_t bit);
uint8_t ee_get_bit(uint16_t eeaddr, uint8_t bit);
void mod_bit(uint8_t * buf, uint8_t value, uint8_t bit);
uint8_t get_field(const uint8_t *buf,uint8_t bits,uint8_t firstbit);
void mod_field(uint8_t *buf,uint8_t value,uint8_t bits,uint8_t firstbit);
void ee_mod_field(uint16_t eeaddr,uint8_t value,uint8_t bits,uint8_t firstbit);
