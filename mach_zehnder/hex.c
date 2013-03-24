/**
 *****************************************************************
 * \file hex.c
 * 	helper routines parsing and printing of Hexadecimal
 *	and decimal numbers
 *****************************************************************
 */
#include <stdint.h>
#include <stdlib.h>
#include "hex.h"

/**
 *******************************************************
 * \fn uint16_t astrtoi16(const char *str);
 * Convert an ascii hex or decimal number to a 16 Bit 
 * integer number.
 *******************************************************
 */

uint16_t
astrtoi16(const char *str)
{
        uint16_t val = 0;
        uint8_t base = 10;
        int8_t mul = 1;
        if (*str == '-') {
                mul = -1;
                str++;
        }
        while (1) {
                if ((*str >= '0') && (*str <= '9')) {
                        val = val * base + (*str - '0');
                } else if ((*str >= 'a') && (*str <= 'f')) {
                        base = 16;
                        val = val * 16 + (*str - 'a' + 10);
                } else if ((*str >= 'A') && (*str <= 'F')) {
                        base = 16;
                        val = val * 16 + (*str - 'A' + 10);
                } else if ((*str == 'x')) {
                        base = 16;
                        val = 0;
                } else {
                        return val * mul;
                }
                str++;
        }
        /* The drunken compiler thinks this can be reached */
        return 0;
}

/**
 **********************************************************
 * \fn uint8_t itoahex(uint16_t value, char *buf);
 * Convert a 16 Bit hex number to an ascii hex string.
 * The Buffer size must be at least 5 Bytes.
 **********************************************************
 */

uint8_t itoahex32(uint32_t value, char *buf)
{
        int8_t shift = 28;
        uint8_t count = 0;
        uint8_t digit;
        while(shift > 0) {
                if(value < (UINT32_C(1) << shift)) {
                        shift -= 4;
                } else {
                        break;
                }
        }
        while(shift >= 0) {
                digit = value >> shift;
                value = value & ~((uint32_t)digit << shift);
                *buf++ = (digit > 9 ? digit + 'a' - 10 : digit + '0');
                shift-=4;
                count++;
        }
        return count;
}

uint8_t itoahex(uint16_t value, char *buf)
{
	return itoahex32(value,buf);
}

/**
 ************************************************************
 * \fn uint8_t uitoa16(uint16_t value, char *buf)
 * Convert an unsigned integer to an ascii string.
 * The buffer size must be at least 6 Bytes 
 ************************************************************
 */
uint8_t uitoa32(uint32_t value, char *buf)
{
        uint32_t div = 1000000000;
        uint8_t digit;
        uint8_t count = 0;
        while(div > 1) {
                if(value < div) {
                        div = div / 10;
                } else {
                        break;
                }
        }
        while(div > 0) {
                digit = value / div;
                value -= (uint32_t)digit * div;
                *buf++ = digit + '0';
                div = div / 10;
                count++;
        }
        return count;
}

uint8_t uitoa16(uint16_t value, char *buf)
{
	return uitoa32(value,buf);
}

/**
 **********************************************************************^M
 * \fn uint8_t itoa16(int16_t value,char *buf);
 **********************************************************************^M
 */
uint8_t itoa16(int16_t value, char *buf)
{
        if(value < 0) {
                buf[0] = '-';
                return uitoa16(-value,buf + 1) + 1;
        } else {
                return uitoa16(value,buf);
        }
}

uint8_t itoa32(int32_t value, char *buf)
{
        if(value < 0) {
                buf[0] = '-';
                return uitoa32(-value,buf + 1) + 1;
        } else {
                return uitoa32(value,buf);
        }
}

