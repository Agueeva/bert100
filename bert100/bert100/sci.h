#ifndef _SCI_H
#define _SCI_H
#define PAR_NONE  0
#define PAR_ODD  (1)
#define PAR_EVEN (2)
#include "types.h"

typedef void SciSinkProc(void *clientData, uint16_t data);
typedef void SciTransmitProc(uint16_t c);

typedef struct SciHandle {
	SciSinkProc *sinkProc;
	void *eventData;
	uint32_t fpos;
	SciTransmitProc *txProc;
	struct SciHandle *next;
} SciHandle;

#if 0
INLINE void
Sci_Transmit(SciHandle * sci, uint16_t c)
{
	if (sci->txProc) {
		sci->txProc(c);
	}
}
#endif
#endif
