/*
 *************************************************************
 * (C) 2009 Jochen Karrer
 *//**
 * \file console.c
 * Serial console for AVR8.
 *************************************************************
 */
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <util/atomic.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "console.h"
#include "events.h"
#include "hex.h"

/**
 *************************************************************************
 * \fn static inline void hexdigit_out(uint8_t n)
 * Helper function for the formated printing of hexadecimal digits. 
 *************************************************************************
 */
static inline void hexdigit_out(uint8_t n)
{
	Con_OutChar(n > 9 ? n + 'A' - 10 : n + '0');
}

/**
 *************************************************************************
 * \fn void Con_OutHex(uint16_t num)
 * Helper function for the formated printing of hexadecimal numbers. 
 *************************************************************************
 */
void Con_OutHex(uint16_t num)
{
	int8_t i;
	for (i = 12; i >= 0; i -= 4) {
		hexdigit_out((num >> i) & 0xf);
	}
}

/**
 *************************************************************************
 * \fn void Con_OutStr(const char *str)
 * Function for writing strings to the serial console. 
 *************************************************************************
 */
void Con_OutStr(const char *str)
{
	for (; *str; str++) {
		if (*str == '\n') {
			Con_OutChar('\r');
		}
		Con_OutChar(*str);
	}
}

/**
 *************************************************************************
 * \fn void _Con_OutPStr(char *str)
 * Function for writing strings from the programm memory to the serial console. 
 *************************************************************************
 */
void _Con_OutPStr(char *str)
{
	uint8_t c;
	for (; (c = pgm_read_byte(str)); str++) {
		if (c == '\n') {
			Con_OutChar('\r');
		}
		Con_OutChar(c);
	}
}


#define PRINTF_STATE_IDLE	0
#define PRINTF_STATE_CONTROL	1
#define PRINTF_STATE_EXACT_LEN  2

#define FLG_ZEROPAD	1
#define FLG_LEFTADJ	2
#define FLG_UNSIGNED	4
#define FLG_LONG	8

typedef struct PrintfState {
	uint8_t printf_state;
	uint8_t printf_flags;
	uint8_t printf_field_width;
} PrintfState;

/**
 ******************************************************************************
 * \fn static void leftpad(PrintfState *pfs,uint8_t len) 
 *	Helper function for the printf statemachine. Pads the left side
 *	of a number with zeros or spaces if len is smaller than 
 *	the minimum field width and the number is not left adjusted.
 ******************************************************************************
 */
static void
leftpad(PrintfState *pfs,uint8_t len) {
	uint8_t i;
	if((pfs->printf_field_width > len) && !(pfs->printf_flags & FLG_LEFTADJ)) {
		for(i=0;i<pfs->printf_field_width - len;i++) {
			if(pfs->printf_flags & FLG_ZEROPAD) {
				Con_OutChar('0');
			} else {
				Con_OutChar(' ');
			}
		}
	}	
}

/**
 ******************************************************************************
 * \fn static void rightpad(PrintfState *pfs,uint8_t len) {
 *	Helper function for the printf statemachine. Pads the left side
 *	of a number with spaces if len is smaller than 
 *	the minimum field width and the number is not right adjusted.
 ******************************************************************************
 */
static void rightpad(PrintfState *pfs,uint8_t len) {
	uint8_t i;
	if((pfs->printf_field_width > len) && (pfs->printf_flags & FLG_LEFTADJ)) {
		for(i=0;i<pfs->printf_field_width - len;i++) {
			Con_OutChar(' ');
		}
	}	
}

/**
 *******************************************************************
 * \fn void _Con_Printf_P(const char *format,...) 
 * The formated printing state machine. It reads the format string
 * always from programm memory.
 *******************************************************************
 */
void _Con_Printf_P(const char *format,...) 
{
	PrintfState pfs;
	uint8_t c;
	uint8_t i;
	va_list ap;
	char str[12];
	pfs.printf_state = PRINTF_STATE_IDLE;
	pfs.printf_flags = 0;
	pfs.printf_field_width = 0;

	va_start(ap,format);
	for( ; (c = pgm_read_byte(format)); format++) {
		switch(pfs.printf_state) {
			case PRINTF_STATE_IDLE:
				if(c == '%') {
					pfs.printf_flags = 0;
					pfs.printf_state = PRINTF_STATE_CONTROL;
					pfs.printf_field_width = 0;
					break;
				}
				if(c == '\n') {
					Con_OutChar('\r');
				}
				Con_OutChar(c);
				break;
			case PRINTF_STATE_CONTROL:
				if((c == '0') && (pfs.printf_field_width == 0)) {
					pfs.printf_flags |= FLG_ZEROPAD;
				} else if ( c == '-' ) {
					pfs.printf_flags |= FLG_LEFTADJ; 
				} else if((c >= '0') && (c <= '9')) {
					pfs.printf_field_width = pfs.printf_field_width * 10 + c - '0';
				} else if( c == '%') {
					Con_OutChar('%');
					pfs.printf_state = PRINTF_STATE_IDLE;
				} else if(c == 's') {
                                        uint16_t len;
                                        char *s = va_arg(ap,char *);
                                        len = strlen(s);
                                        pfs.printf_flags &= ~FLG_ZEROPAD;
                                        leftpad(&pfs,len);
                                        for(i=0;i < len;i++) {
						Con_OutChar(s[i]);
                                        }
                                        rightpad(&pfs,len);
                                        pfs.printf_state = PRINTF_STATE_IDLE;
				} else if(c == 'S') {
                                        uint16_t len;
                                        char * PROGMEM s = va_arg(ap,char *PROGMEM);
                                        len = strlen_P(s);
                                        pfs.printf_flags &= ~FLG_ZEROPAD;
                                        leftpad(&pfs,len);
					_Con_OutPStr(s);
                                        rightpad(&pfs,len);
                                        pfs.printf_state = PRINTF_STATE_IDLE;
				} else if(c == 'l') {
					pfs.printf_flags |= FLG_LONG;
				} else if(c == 'c') {
                                        uint8_t value;
                                        value = va_arg(ap,uint16_t);
                                        Con_OutChar(value);
                                        pfs.printf_state = PRINTF_STATE_IDLE;
				} else if( (c == 'd') || (c == 'u')) {
					uint8_t len = 0;
					uint32_t value;
					if(c == 'u') {
                                                pfs.printf_flags |= FLG_UNSIGNED;
                                        }
					if(pfs.printf_flags & FLG_LONG) {
						value = va_arg( ap, int32_t);
					} else {
						value = va_arg( ap, int);
					}
					if(pfs.printf_flags & FLG_UNSIGNED) {
						if(pfs.printf_flags & FLG_LONG) {
                                                	len = uitoa32(value,str);
						} else {
                                                	len = uitoa16(value,str);
						}
                                        } else {
						if(pfs.printf_flags & FLG_LONG) {
                                               		len = itoa32(value,str);
						} else {
                                               		len = itoa16(value,str);
						}
                                        }
					leftpad(&pfs,len);
					for(i=0;i<len;i++) {
						Con_OutChar(str[i]);
					}
					rightpad(&pfs,len);
					pfs.printf_state = PRINTF_STATE_IDLE;
				} else if(c == 'x') {
					uint8_t len = 0;
					uint32_t value;
					if(pfs.printf_flags & FLG_LONG) {
						value = va_arg( ap, int32_t);
						Con_OutChar('L');
						len = itoahex32(value,str);
					} else {
						value = va_arg( ap, int);
						len = itoahex(value,str);
					}
					leftpad(&pfs,len);
					for(i=0;i<len;i++) {
						Con_OutChar(str[i]);
					}
					rightpad(&pfs,len);
					pfs.printf_state = PRINTF_STATE_IDLE;
				} else {
					pfs.printf_state = PRINTF_STATE_IDLE;
				}
				break;
				
		}
	}
	va_end(ap);
}

