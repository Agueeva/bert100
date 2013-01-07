/**
 *****************************************************************
 * \file hex.h
 *      helper routines parsing and printing of Hexadecimal
 *      and decimal numbers
 *****************************************************************
 */

uint16_t astrtoi16(const char *str);

/* 
 * Convert an 8 bit interger value to a 2 digit hexnumber, 
 * return the number of digits 
 */
uint8_t itoahex(uint16_t value, char *buf);
uint8_t itoahex32(uint32_t value, char *buf);
uint8_t uitoa16(uint16_t value,char *buf);
uint8_t uitoa32(uint32_t value,char *buf);
uint8_t itoa16(int16_t value, char *buf);
uint8_t itoa32(int32_t value, char *buf);
