/**
 ***************************************************************
 * Printf state machine.
 ***************************************************************
 */
#include <stdarg.h>
#include <string.h>
#include "types.h"
#include "hex.h"
#include "console.h"

#define PRINTF_STATE_IDLE	0
#define PRINTF_STATE_CONTROL	1
#define PRINTF_STATE_EXACT_LEN  2

#define FLG_ZEROPAD		(1)
#define FLG_LEFTADJ		(2)
#define FLG_UNSIGNED		(4)
#define FLG_LONG		(8)
#define FLG_LONG64		(0x10)

typedef struct PrintfState {
	void *cbData;
	uint8_t state;
	uint8_t field_width;
	uint8_t flags;
} PrintfState;

void
Con_OutStr(const char *str)
{
	Con_Printf(str);
}

/**
 ******************************************************************************
 * \fn static void leftpad(uint8_t len) 
 *	Helper function for the printf statemachine. Pads the left side
 *	of a number with zeros or spaces if len is smaller than 
 *	the minimum field width and the number is not left adjusted.
 ******************************************************************************
 */
static void
leftpad(PrintfState * pfs, PrintCharProc * printChar, uint8_t len)
{
	uint8_t i;
	if ((pfs->field_width > len) && !(pfs->flags & FLG_LEFTADJ)) {
		for (i = 0; i < pfs->field_width - len; i++) {
			if (pfs->flags & FLG_ZEROPAD) {
				printChar(pfs->cbData, '0');
			} else {
				printChar(pfs->cbData, ' ');
			}
		}
	}
}

/**
 ******************************************************************************
 * \fn static void rightpad(uint8_t len) {
 *	Helper function for the printf statemachine. Pads the left side
 *	of a number with spaces if len is smaller than 
 *	the minimum field width and the number is not right adjusted.
 ******************************************************************************
 */
static void
rightpad(PrintfState * pfs, PrintCharProc * printChar, uint8_t len)
{
	uint8_t i;
	if ((pfs->field_width > len) && (pfs->flags & FLG_LEFTADJ)) {
		for (i = 0; i < pfs->field_width - len; i++) {
			printChar(pfs->cbData, ' ');
		}
	}
}

/**
 *************************************************************************************
 * \fn void Con_PrintToVA(PrintCharProc *printChar,const char *format,va_list ap) 
 *************************************************************************************
 */
void
Con_PrintToVA(PrintCharProc * printChar, void *cbData, const char *format, va_list ap)
{
	uint8_t c;
	uint8_t i;
	char str[21];
	PrintfState pfs;
	pfs.state = PRINTF_STATE_IDLE;
	pfs.flags = 0;
	pfs.field_width = 0;
	pfs.cbData = cbData;
	for (; *format; format++) {
		c = *format;
		switch (pfs.state) {
		    case PRINTF_STATE_IDLE:
			    if (c == '%') {
				    pfs.flags = 0;
				    pfs.state = PRINTF_STATE_CONTROL;
				    pfs.field_width = 0;
				    break;
			    }
			    if (c == '\n') {
				    printChar(cbData, '\r');
			    }
			    printChar(cbData, c);
			    break;
		    case PRINTF_STATE_CONTROL:
			    if ((c == '0') && (pfs.field_width == 0)) {
				    pfs.flags |= FLG_ZEROPAD;
			    } else if (c == '-') {
				    pfs.flags |= FLG_LEFTADJ;
			    } else if ((c >= '0') && (c <= '9')) {
				    pfs.field_width = pfs.field_width * 10 + c - '0';
			    } else if (c == '%') {
				    printChar(cbData, '%');
				    pfs.state = PRINTF_STATE_IDLE;
			    } else if (c == 'C') {
				    uint8_t value;
				    value = va_arg(ap, int);
				    printChar(cbData, value);
				    if (value == '\n') {
					    printChar(cbData, '\r');
				    }
				    pfs.state = PRINTF_STATE_IDLE;
			    } else if (c == 'c') {
				    uint8_t value;
				    value = va_arg(ap, int);
				    printChar(cbData, value);
				    pfs.state = PRINTF_STATE_IDLE;
			    } else if (c == 'l') {
				    if (pfs.flags & FLG_LONG) {
					    pfs.flags |= FLG_LONG64;
				    } else {
					    pfs.flags |= FLG_LONG;
				    }
			    } else if (c == 's') {
				    uint16_t len;
				    char *s = va_arg(ap, char *);
				    if (!s) {
					    pfs.state = PRINTF_STATE_IDLE;
					    continue;
				    }
				    len = strlen(s);
				    if (pfs.field_width && (len > pfs.field_width)) {
					    len = pfs.field_width;
				    }
				    pfs.flags &= ~FLG_ZEROPAD;
				    leftpad(&pfs, printChar, len);
				    for (i = 0; i < len; i++) {
					    printChar(cbData, s[i]);
				    }
				    rightpad(&pfs, printChar, len);
				    pfs.state = PRINTF_STATE_IDLE;
			    } else if ((c == 'e')) {
				    uint8_t len;
				    union {
			 	    	float fval;
				    	uint32_t val;
				    } val;
				    val.val = va_arg(ap, uint32_t);
				    len = f32toExp(val.fval, str,sizeof(str));
				    for (i = 0; i < len; i++) {
					    printChar(cbData, str[i]);
				    }
				    pfs.state = PRINTF_STATE_IDLE;
			    } else if ((c == 'f')) {
				    uint8_t len;
				    union {
			 	    	float fval;
				    	uint32_t val;
				    } val;
				    val.val = va_arg(ap, uint32_t);
				    len = f32toa(val.fval, str,sizeof(str));

				    for (i = 0; i < len; i++) {
					    printChar(cbData, str[i]);
				    }
				    pfs.state = PRINTF_STATE_IDLE;
			    } else if ((c == 'd') || (c == 'u')) {
				    uint8_t len = 0;
				    uint64_t value;
				    if (c == 'u') {
					    pfs.flags |= FLG_UNSIGNED;
				    }
				    if (pfs.flags & FLG_LONG64) {
					    value = va_arg(ap, uint64_t);
					    if (pfs.flags & FLG_UNSIGNED) {
						    len = uitoa64(value, str);
					    } else {
						    len = itoa64(value, str);
					    }
				    } else if (pfs.flags & FLG_LONG) {
					    if (pfs.flags & FLG_UNSIGNED) {
					    	value = va_arg(ap, uint32_t);
					    } else {
						    value = (int64_t) (int32_t) va_arg(ap, int);
					    }
					    if (pfs.flags & FLG_UNSIGNED) {
						    len = uitoa32(value, str);
					    } else {
						    len = itoa32(value, str);
					    }
				    } else {
					    if (pfs.flags & FLG_UNSIGNED) {
						    value = va_arg(ap, int);
					    } else {
						    value = (int64_t) (int16_t) va_arg(ap, int);
					    }
					    if (pfs.flags & FLG_UNSIGNED) {
						    len = uitoa16(value, str);
					    } else {
						    len = itoa16(value, str);
					    }
				    }
				    if(str[0] == '-') {
					printChar(cbData, str[0]);
				    	leftpad(&pfs, printChar, len - 1);
				    } else {
				    	leftpad(&pfs, printChar, len);
					printChar(cbData, str[0]);
				    }
				    for (i = 1; i < len; i++) {
					    printChar(cbData, str[i]);
				    }
				    rightpad(&pfs, printChar, len);
				    pfs.state = PRINTF_STATE_IDLE;
			    } else if (c == 'x') {
				    uint8_t len = 0;
				    uint64_t value;
				    if (pfs.flags & FLG_LONG64) {
					    value = va_arg(ap, uint64_t);
					    len = itoahex64(value, str);
				    } else if (pfs.flags & FLG_LONG) {
					    value = va_arg(ap, uint32_t);
					    len = itoahex32(value, str);
				    } else {
					    value = va_arg(ap, int);
					    len = itoahex(value, str);
				    }
				    leftpad(&pfs, printChar, len);
				    for (i = 0; i < len; i++) {
					    printChar(cbData, str[i]);
				    }
				    rightpad(&pfs, printChar, len);
				    pfs.state = PRINTF_STATE_IDLE;
			    } else {
				    pfs.state = PRINTF_STATE_IDLE;
			    }
			    break;
		}
	}
}

void
Con_PrintVA(const char *format, va_list ap)
{
	Con_PrintToVA(Con_TransmitChar, NULL, format, ap);
}

void
Con_Printf(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	Con_PrintVA(format, ap);
	va_end(ap);
}

/**
 ***************************************************
 * The sprintf buffer is on the stack because
 * we are reenetrant. Not really needed currently
 * because SNPrintf can not block and OS is not
 * preemptive.
 ***************************************************
 */

typedef struct SPrintfCtxt {
	char *dstBuf;
	uint16_t cnt;
	uint16_t maxlen;
} SPrintfCtxt;

/**
 *************************************************************************
 * Print a single char into the buffer stored in the sprintf context.
 * Used as callback proc for the printf statemachine.
 *************************************************************************
 */
static void
sprintf_char(void *cbData, uint8_t data)
{
	SPrintfCtxt *ctxt = cbData;
	if (ctxt->cnt < ctxt->maxlen - 1) {
		ctxt->dstBuf[ctxt->cnt++] = data;
	}
}

/**
 **********************************************************************************
 * \fn uint16_t VSNPrintf(char *str,uint16_t maxlen,const char *format,va_list ap)
 * Print a string into a buffer.
 **********************************************************************************
 */
uint16_t
VSNPrintf(char *str, uint16_t maxlen, const char *format, va_list ap)
{
	SPrintfCtxt ctxt;
	ctxt.dstBuf = str;
	ctxt.cnt = 0;
	ctxt.maxlen = maxlen;
	Con_PrintToVA(sprintf_char, &ctxt, format, ap);
	if (maxlen) {
		ctxt.dstBuf[ctxt.cnt] = 0;
	}
	return ctxt.cnt;
}

/**
 ***********************************************************************************
 * \fn uint16_t SNPrintf(char *str,uint16_t maxlen,const char *format,...)
 * Print a string into a buffer. vararg variant with format string.
 * The length is limited to maxlen.
 ***********************************************************************************
 */
uint16_t
SNPrintf(char *str, uint16_t maxlen, const char *format, ...)
{
	va_list ap;
	uint16_t cnt;
	va_start(ap, format);
	cnt = VSNPrintf(str, maxlen, format, ap);
	va_end(ap);
	return cnt;
}
