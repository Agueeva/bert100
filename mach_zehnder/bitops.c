/**
 */
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "console.h"
#include "interpreter.h"
#include "hex.h"
#include "eeprom.h"
#include "eeprom_map.h"
#include "bitops.h"

/**
 **********************************************************************
 * \fn static inline uint8_t get_bit(const uint8_t * buf, uint8_t bit);
 * get_bit
 *      Read a bit from a byte array.
 **********************************************************************
 */
uint8_t get_bit(const uint8_t * buf, uint8_t bit)
{
        return (buf[bit >> 3] >> (bit & 7)) & 1;
}

uint8_t ee_get_bit(uint16_t eeaddr, uint8_t bit)
{
        uint16_t addr = eeaddr + (bit >> 3);
        uint8_t value = EEProm_Read8(addr);
        return (value >> (bit & 7)) & 1;
}

/*
 *********************************************************************
 * \fn static inline void mod_bit(uint8_t * buf, uint8_t value, uint8_t bit)
 *      Modify a bit in a byte array.
 *********************************************************************
 */
void mod_bit(uint8_t * buf, uint8_t value, uint8_t bit)
{
        if (value) {
                buf[bit >> 3] |= (1 << (bit & 7));
        } else {
                buf[bit >> 3] &= ~(1 << (bit & 7));
        }
}


uint8_t get_field(const uint8_t *buf,uint8_t bits,uint8_t firstbit)
{
        uint8_t value = 0;
        uint8_t i;
        for (i = 0; i < bits; i++) {
                value |= get_bit(buf, firstbit + i) << i;
        }
        return value;
}

/**
 *********************************************************************************
 * \fn static inline void mod_field(uint8_t *buf,uint8_t value,uint8_t bits,uint8_t firstbit); * Modify a set of bits in a byte array.  *********************************************************************************
 */

void
mod_field(uint8_t *buf,uint8_t value,uint8_t bits,uint8_t firstbit)
{
        uint8_t i;
        for (i = 0; i < bits; i++) {
                mod_bit(buf, (value >> i) & 1, firstbit + i);
        }
}

void
ee_mod_field(uint16_t eeaddr,uint8_t value,uint8_t bits,uint8_t firstbit)
{
        uint8_t i;
        uint8_t firstbyte = firstbit >> 3;
        uint8_t eedata[2];
        if(bits > 8)  {
                return;
        }
        EEProm_Read(eeaddr + firstbyte, eedata, 2);
        for (i = 0; i < bits; i++) {
                mod_bit(eedata, (value >> i) & 1, (firstbit & 7) + i);
        }
        EEProm_Write(eeaddr + firstbyte,eedata ,2);
}
