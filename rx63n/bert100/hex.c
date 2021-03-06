/**
 *****************************************************************
 * \file hex.c
 * 	helper routines parsing and printing of Hexadecimal
 *	and decimal numbers
 *****************************************************************
 */
#include "types.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "hex.h"

/**
 *****************************************************************
 * Convert an Ascii string to a floating point value.
 * Format is xxx.yyy, (No exponent is allowed)
 *****************************************************************
 */
#include "console.h"
float
astrtof32(const char *str)
{
	int64_t ival = 0;
	int64_t div = 1;
	uint8_t base = 10;
	uint8_t digits = 0;
	bool dot = false;
	if (*str == '-') {
		div = -1;
		str++;
	}
	while (1) {
		if (digits > 18) {
			if (dot) {
				return (float)ival / div;
			} else {
				return 0;
			}
		} else if ((*str >= '0') && (*str <= '9')) {
			ival = ival * base + (*str - '0');
			if (dot) {
				div *= 10;
			}
			digits++;
		} else if (*str == '.') {
			dot = true;
		} else {
			return (float)ival / div;
		}
		str++;
	}
}

/**
 *******************************************************
 * \fn uint32_t astrtoi32(const char *str);
 * Convert an ascii hex or decimal number to a 32 Bit 
 * integer number.
 *******************************************************
 */
uint32_t
astrtoi32(const char *str)
{
	uint32_t val = 0;
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

uint64_t
astrtoi64(const char *str)
{
	uint64_t val = 0;
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
 * Convert a 16 Bit number to an ascii hex string.
 * The Buffer size must be at least 5 Bytes.
 **********************************************************
 */
uint8_t
itoahex(uint16_t value, char *buf)
{
	int8_t shift = 12;
	uint8_t count = 0;
	uint8_t digit;
	while (shift > 0) {
		if (value < ((uint16_t) 1 << shift)) {
			shift -= 4;
		} else {
			break;
		}
	}
	while (shift >= 0) {
		digit = value >> shift;
		value = value & ~((uint16_t) digit << shift);
		*buf++ = (digit > 9 ? digit + 'a' - 10 : digit + '0');
		shift -= 4;
		count++;
	}
	return count;
}

uint8_t
itoahex32(uint32_t value, char *buf)
{
	int8_t shift = 28;
	uint8_t count = 0;
	uint8_t digit;
	while (shift > 0) {
		if (value < ((uint32_t) 1 << shift)) {
			shift -= 4;
		} else {
			break;
		}
	}
	while (shift >= 0) {
		digit = value >> shift;
		value = value & ~((uint32_t) digit << shift);
		*buf++ = (digit > 9 ? digit + 'a' - 10 : digit + '0');
		shift -= 4;
		count++;
	}
	return count;
}

uint8_t
itoahex64(uint64_t value, char *buf)
{
	int8_t shift = 60;
	uint8_t count = 0;
	uint8_t digit;
	while (shift > 0) {
		if (value < ((uint64_t) 1 << shift)) {
			shift -= 4;
		} else {
			break;
		}
	}
	while (shift >= 0) {
		digit = value >> shift;
		value = value & ~((uint64_t) digit << shift);
		*buf++ = (digit > 9 ? digit + 'a' - 10 : digit + '0');
		shift -= 4;
		count++;
	}
	return count;
}

/**
 ************************************************************
 * \fn uint8_t uitoa16(uint16_t value, char *buf)
 * Convert an unsigned integer to an ascii string.
 * The buffer size must be at least 6 Bytes 
 ************************************************************
 */
uint8_t
uitoa16(uint16_t value, char *buf)
{
	uint16_t divisor = 10000;
	uint8_t digit;
	uint8_t count = 0;
	while (divisor > 1) {
		if (value < divisor) {
			divisor = divisor / 10;
		} else {
			break;
		}
	}
	while (divisor > 0) {
		digit = value / divisor;
		value -= (uint16_t) digit *divisor;
		*buf++ = digit + '0';
		divisor = divisor / 10;
		count++;
	}
	return count;
}

uint8_t
uitoa32(uint32_t value, char *buf)
{
	uint32_t divisor = 1000000000;
	uint8_t digit;
	uint8_t count = 0;
	while (divisor > 1) {
		if (value < divisor) {
			divisor = divisor / 10;
		} else {
			break;
		}
	}
	while (divisor > 0) {
		digit = value / divisor;
		value -= (uint32_t) digit *divisor;
		*buf++ = digit + '0';
		divisor = divisor / 10;
		count++;
	}
	return count;
}

uint8_t
uitoa64(uint64_t value, char *buf)
{
	uint64_t divisor = UINT64_C(10000000000000000000);
	uint8_t digit;
	uint8_t count = 0;
	while (divisor > 1) {
		if (value < divisor) {
			divisor = divisor / 10;
		} else {
			break;
		}
	}
	while (divisor > 0) {
		digit = value / divisor;
		value -= digit * divisor;
		*buf++ = digit + '0';
		divisor = divisor / 10;
		count++;
	}
	return count;
}

/**
 **********************************************************************
 * \fn uint8_t itoa16(int16_t value,char *buf);
 **********************************************************************
 */
uint8_t
itoa16(int16_t value, char *buf)
{
	if (value < 0) {
		buf[0] = '-';
		return uitoa16(-value, buf + 1) + 1;
	} else {
		return uitoa16(value, buf);
	}
}

/**
 *******************************************************************
 * Convert a signed integer into a string.
 *******************************************************************
 */
uint8_t
itoa32(int32_t value, char *buf)
{
	if (value < 0) {
		buf[0] = '-';
		return uitoa32(-value, buf + 1) + 1;
	} else {
		return uitoa32(value, buf);
	}
}

uint8_t
itoa64(int64_t value, char *buf)
{
	if (value < 0) {
		buf[0] = '-';
		return uitoa64(-value, buf + 1) + 1;
	} else {
		return uitoa64(value, buf);
	}
}

uint8_t
ishexnum(char *str)
{
	uint8_t i;
	if (str[0] == 0) {
		return 0;
	}
	for (i = 0; (i < 255) && str[i]; i++) {
		if ((str[i] >= '0') && (str[i] <= '9')) {
			continue;
		}
		if ((str[i] >= 'a') && (str[i] <= 'f')) {
			continue;
		}
		if ((str[i] >= 'A') && (str[i] <= 'F')) {
			continue;
		}
		if ((str[i] == 'x') || (str[i] == '-')) {
			continue;
		}
		return 0;
	}
	return 1;
}

uint8_t
f32toa(float fval, char *_str, int maxlen)
{
	uint32_t ival;
	uint8_t count;
	uint8_t digit;
	uint64_t div = 1;
	char *str = _str;
	if(maxlen < 11) {
		return 0;		
	}
	if(fval < 0) {
		*str++ = '-';
		maxlen--;
		fval = -fval;
	}
	maxlen = 10;
	ival = fval;
	count = uitoa32(ival, str);
	if (count >= maxlen) {
		return count;
	}
	str[count++] = '.';
	fval = fval - (float)ival;
	str += count;
	while (count < maxlen) {
		fval *= 10;
		++count;
		div = div * 10;
	}
	ival = fval;
	div = div / 10;
//	Con_Printf("xival %lld, div %llu\n",ival,div);
	while(div) {
		digit = ival / div;
	//	Con_Printf("xival %ld, div %llu: %u\n",ival,div,digit);
		ival -= digit * div;
		*str++ = digit + '0';
		div = div / 10;
		if(ival == 0) {
			break;
		}
	}
	return str - _str;
}

uint8_t 
f32toExp(float fval, char *_str, int maxlen)
{
	int16_t exp;
	char *str = _str;
	if(maxlen < 20) {
		return 0;
	}
	if(fval == 0.0) {
		exp = 0;
	} else {
		exp = logf(fabs(fval))/logf(10) - 0.99999999;
	}
	fval *= pow(10,-exp);
	str += f32toa(fval, str, maxlen);
	*str++ = 'e';	
	if(exp < 0) {
		*str++ = '-';	
		exp = - exp;
	}
	if(exp < 10) {
		*str++ = '0';	
	}
	str += uitoa16(exp, str);
	return str - _str;
}

