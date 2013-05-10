#ifndef _THREADS_H
#define _THREADS_H
#include "types.h"
#include "events.h"
typedef struct Thread Thread;
extern uint32_t thread0_stack[];
extern uint32_t thread0_stack_top[];
extern uint32_t istack0_stack[];
extern uint32_t istack0_top[];
extern Thread *g_CurrTh;
static inline Thread *
Thread_Current(void)
{
	return g_CurrTh;
}

struct Thread {
	uint32_t regSP;
	struct Thread *next;
	struct Thread *waitNext;
	Event wakeEvent;
	uint32_t nrUsed;
	uint8_t *stack;
	uint16_t stacksize;
	uint8_t threadId;
};

void Threads_Init(void);
Thread *Thread_Alloc(void);
void Thread_Init(Thread * newTh, void (*threadproc) (void));
void Thread_Switch(Thread * newTh);
#define Thread_SetState(x,y);
#endif
