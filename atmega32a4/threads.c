#include "threads.h"
#include <string.h>
#include "events.h"
#include "interpreter.h"
#include "console.h"
#include "hex.h"

#define NR_THREADS		(4)
#define THREAD_STACKSIZE	(256)

static Thread g_Thread[NR_THREADS];
static uint8_t thread_stacks[(NR_THREADS - 1) * THREAD_STACKSIZE];
extern void _switch_context(uint16_t *newStack,uint16_t *oldStack);

Thread *g_CurrTh = NULL;

void
Thread_Switch(Thread *newTh) {
        Thread *oldTh = g_CurrTh;
        g_CurrTh = newTh;
        _switch_context(&newTh->regSP,&oldTh->regSP);
        /*
         ****************************************************************
         * Everything below switch_registers will be done in the new
         * context (or not at all if the new context was not left here).
         ****************************************************************
         */
}

void
Thread_Init(Thread *newTh,void (*threadproc)(void))
{
        uint8_t *stack_top;
        memset(newTh->stack,0,newTh->stacksize);
        stack_top = (uint8_t *) (newTh->stack + newTh->stacksize - 1);
        *(uint8_t*) stack_top = (uint16_t)threadproc & 0xff; /* PC */
        stack_top -= 1;
        *(uint8_t*) stack_top = ((uint16_t)threadproc >> 8) & 0xff; /* PC */
        stack_top -= 1;
        newTh->regSP = (uint16_t)stack_top;
        return;
}

Thread *
Thread_Alloc()
{
        static uint8_t id = 0;
        Thread *th = NULL;
        id++;
        if(id < NR_THREADS) {
                th  = &g_Thread[id];
                th->stack = (uint8_t *)thread_stacks + THREAD_STACKSIZE * (id - 1);
                th->stacksize = THREAD_STACKSIZE;
        }
        return th;
}

/**
 *****************************************************************************
 * \fn static int8_t cmd_thread(Interp * interp, uint8_t argc, char *argv[])
 *****************************************************************************
 */
static int8_t
cmd_thread(Interp *interp, uint8_t argc, char *argv[])
{
        uint16_t i,j;
        if((argc > 1) && strcmp(argv[1],"test") == 0) {
                return 0;
        }
        for(i = 0; i < NR_THREADS;i++) {
                Thread *th = &g_Thread[i];
                uint8_t *st = (uint8_t *)th->stack;
                if(th->stack == NULL) {
                        continue;
                }
                for(j = 0; j < th->stacksize;j++) {
                        if(st[j]) {
                                break;
                        }
                }
                if(j == 0) {
                        j = 1;
                }
                Con_Printf_P("Th%d: Room %u\n",i,j - 1);
        }
//        Interp_Printf_P(interp,"ContextSw: %lu\n",stat_ContextSwitch);
        return 0;
}

INTERP_CMD(thread, cmd_thread,
           "thread     # Trhead status ");

void
Threads_Init(void) 
{
	memset(g_Thread,0,sizeof(g_Thread));
	g_CurrTh = &g_Thread[0];
	g_CurrTh->stack = (uint8_t *)(0x3000 - THREAD_STACKSIZE);
	g_CurrTh->stacksize = THREAD_STACKSIZE;	
#if 0
	g_Thread[1].stacksize = THREAD_STACKSIZE;
	g_Thread[1].stack = thread_stacks + 0 * THREAD_STACKSIZE;
	Thread_Init(&g_Thread[1],EV_Loop);
	Thread_Switch(&g_Thread[1]);
#endif
//	Thread_Init(&g_Thread[0],EV_Loop);
//	Thread_Switch(&g_Thread[0]);
}
