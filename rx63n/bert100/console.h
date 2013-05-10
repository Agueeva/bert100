#ifndef _CONSOLE_H
#define _CONSOLE_H
#include "types.h"
#include "sci0.h"
#include <stdarg.h>

typedef void PrintCharProc(void *evData, uint8_t c);

#define CON 0 
#if CON == 0
static inline void
Con_TransmitChar(void *cbData, uint8_t c)
{
	Sci0_TransmitChar(c);
}

#define Con_OutChar Con_TransmitChar
#define Con_RegisterSink Sci0_RegisterSink
#endif
#if CON == 6
static inline void
Con_TransmitChar(void *cbData, uint8_t c)
{
	Sci6_TransmitChar(c);
	//UsbAcm_TransmitChar(c);
}

#define Con_OutChar Con_TransmitChar
#define Con_RegisterSink Sci6_RegisterSink
#endif

void Con_Printf(const char *format, ...);
void Con_PrintVA(const char *format, va_list ap);
void Con_PrintToVA(PrintCharProc *, void *cbData, const char *format, va_list ap);
void Con_OutStr(const char *str);
uint16_t VSNPrintf(char *str, uint16_t maxlen, const char *format, va_list ap);
uint16_t SNPrintf(char *str, uint16_t maxlen, const char *format, ...);
#endif
