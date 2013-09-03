/**
 *******************************************************************************
 * Threads
 * Allocate, Initialize and  switch threads
 *******************************************************************************
 */
#include <string.h>
#include "types.h"
#include "threads.h"
#include "interpreter.h"
#include "hex.h"
#include "console.h"
#include "timer.h"
#include "stack.h"

#define NR_THREADS	9	

static Thread g_Thread[NR_THREADS];
Thread *g_CurrTh;

/* uint32_t for alignment */
uint32_t thread0_stack[THREAD_STACKSIZE >> 2];
static uint32_t thread_stacks[(NR_THREADS - 1) * THREAD_STACKSIZE >> 2];
static uint32_t stat_ContextSwitch = 0;
uint32_t istack0[ISTACKSIZE >> 2];

/*
 ********************************************************************
 * Ablauf:
 * Register Saven, 
 * Stack switchen, 
 * Register (vom neuen Task) wiederherstellen.
 *
 * Beim stackswitch wird der gesavete Programmcounter maninpuliert
 * damit nach dem switch der switch nicht erneut ausgeführt wird. 
 ********************************************************************
 */
extern void switch_context(uint32_t * newStack, uint32_t * oldStack);

void
Thread_Switch(Thread * newTh)
{
	Thread *oldTh = g_CurrTh;
	g_CurrTh = newTh;
	stat_ContextSwitch++;
	newTh->nrUsed++;
	switch_context(&newTh->regSP, &oldTh->regSP);
	/*
	 ****************************************************************
	 * Everything below switch_registers will be done in the new
	 * context (or not at all if the new context was not left here).
	 ****************************************************************
	 */
}

/**
 ***********************************************************
 * Initialize a new thread by filling its stack 
 * with Initial register values and with a proc as return 
 * address on its top
 ***********************************************************
 */
void
Thread_Init(Thread * newTh, void (*threadproc) (void))
{
	uint8_t *stack_top;
	memset(newTh->stack, 0, newTh->stacksize);
	stack_top = (uint8_t *) (newTh->stack + newTh->stacksize);
	stack_top -= 4;
	*(uint32_t *) stack_top = (uint32_t) threadproc;	/* PC */
	newTh->regSP = (uint32_t) stack_top;
	return;
}

/**
 *******************************************************************
 * \fn Thread * Thread_Alloc(Thread *newTh,void (*threadproc)(void)) 
 * Allocate a thread from the pool or in SRAM if none in pool is
 * available.
 *******************************************************************
 */
Thread *
Thread_Alloc()
{
	static uint16_t id = 0;
	Thread *th;
	id++;
	if (id < NR_THREADS) {
		th = &g_Thread[id];
		th->stack = (uint8_t *) thread_stacks + THREAD_STACKSIZE * (id - 1);
		th->stacksize = THREAD_STACKSIZE;
	} else {
		while (1) {
			Con_Printf("Out of threads\n");
		}
	}
	th->threadId = id;
	return th;
}

/**
 *****************************************************************************
 * \fn static int8_t cmd_thread(Interp * interp, uint8_t argc, char *argv[])
 *****************************************************************************
 */
static int8_t
cmd_thread(Interp * interp, uint8_t argc, char *argv[])
{
	uint16_t i, j;
	if ((argc > 1) && strcmp(argv[1], "test") == 0) {
		return 0;
	}
	for (i = 0; i < NR_THREADS; i++) {
		Thread *th = &g_Thread[i];
		uint8_t *st = (uint8_t *) th->stack;
		if (th->stack == NULL) {
			continue;
		}
		for (j = 0; j < th->stacksize; j++) {
			if (st[j]) {
				break;
			}
		}
		if (j == 0) {
			j = 1;
		}
		Con_Printf("Th%d: Room %u, used %lu\n", i, j - 1, th->nrUsed);
	}
	Interp_Printf_P(interp, "ContextSw: %lu\n", stat_ContextSwitch);
	return 0;
}

INTERP_CMD(threadCmd, "thread", cmd_thread, "thread      # Control thread");

/**
 *********************************************************
 * \fn void Threads_Init(void) 
 *********************************************************
 */
void
Threads_Init(void)
{
	memset(g_Thread, 0, sizeof(g_Thread));
	g_CurrTh = &g_Thread[0];
	g_CurrTh->stack = (uint8_t *) thread0_stack;
	g_CurrTh->stacksize = THREAD_STACKSIZE;
	Interp_RegisterCmd(&threadCmd);
}
