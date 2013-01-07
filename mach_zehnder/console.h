/*
 *************************************************************
 * (C) 2009 Jochen Karrer
 *//**
 * \file console.h
 * Serial console for AVR8.
 *************************************************************
 */

#ifndef _CONSOLE_H
#define _CONSOLE_H

#include <stdarg.h>
#include <avr/pgmspace.h>
#include "uartc0.h"

/* Interface */
void Con_Init(void);
void Con_OutStr(const char *str);
void _Con_OutPStr(char *str);
#define Con_OutPStr(x) _Con_OutPStr(PSTR(x))

void _Con_Printf_P(const char *format,...);
#define __Con_Printf_P(x,y...) _Con_Printf_P(PSTR(x),y)
#define Con_Printf_P(x...) __Con_Printf_P(x,0)

void Con_OutHex(uint16_t num);

static inline void Con_OutChar(uint8_t c)
{
	UartC0_TransmitChar(c);
	//TWITerm_TransmitChar(c);
}

#include "uartc0.h"
#define Con_RegisterSink UartC0_RegisterSink
#endif
