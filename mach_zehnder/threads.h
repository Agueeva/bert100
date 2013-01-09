#ifndef _THREADS_H
#define _THREADS_H
#include <stdint.h> 

typedef struct Thread Thread;
extern Thread *g_CurrTh;
static inline Thread *Thread_Current(void) { return g_CurrTh; }

struct Thread {
	uint16_t regSP;
	struct Thread *next;
	struct Thread *prev;
	//struct Thread *waitNext;
	uint8_t *stack;
	uint16_t stacksize;
};
void Threads_Init(void);
#endif
