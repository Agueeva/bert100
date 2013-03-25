#ifndef _THREADS_H
#define _THREADS_H
#include "events.h"
#include <stdint.h> 

typedef struct Thread Thread;
extern Thread *g_CurrTh;
static inline Thread *Thread_Current(void) { return g_CurrTh; }
void Thread_Init(Thread *newTh,void (*threadproc)(void));

struct Thread {
	uint16_t regSP;
	struct Thread *next;
	struct Thread *waitNext;
	Event	wakeEvent;
	uint8_t *stack;
	uint16_t stacksize;
};
Thread * Thread_Alloc(void);
void Thread_Switch(Thread *newTh);
void Threads_Init(void);
#endif
