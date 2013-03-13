/*
 **************************************************************
 * Thread pool operating system.
 * Resource semaphores and thread allocation for events
 * Only one thread has the right to live. So whenever a
 * thread is created one of the thread will be disabled
 * (at end of eventqueue for yield, on Unlock for semaphores). 
 **************************************************************
 */
#include <string.h>
#include <stdint.h>
#include "threads.h"
#include "events.h"
#include "console.h"
#include "tpos.h"
#include "interpreter.h"
#include "timer.h"
#include "hex.h"

#define THREAD_POOL_SIZE  4

/**
 *************************************************************************
 * This is a linked list of threads currently available for usage
 *************************************************************************
 */
static Thread *unusedqHead = NULL;

static uint16_t stat_MaxThreads = 1;
static uint16_t stat_NumThreads = 1;

/**
 *********************************************************************************************
 * \fn static void MoveRunningToUnused(Thread *th); 
 * Must be already in state Unused;
 *********************************************************************************************
 */
INLINE void
MoveRunningToUnused(Thread *th) {
	th->next = unusedqHead;
	unusedqHead = th;
	stat_NumThreads--;
}

static void
TPos_ThreadWake(void *eventData) {
	Thread *nextTh = eventData;
	MoveRunningToUnused(Thread_Current()); 
	Thread_Switch(nextTh);
}

static Thread * 
GetNewThread(void) 
{
	Thread *th;
	th = unusedqHead;	
	if(th) {
		unusedqHead = unusedqHead->next;
		th->next = NULL;
		stat_NumThreads++;
		if(stat_NumThreads > stat_MaxThreads) {
			stat_MaxThreads = stat_NumThreads;
		}
	} else {
		Con_Printf_P("Out of threads\n");
	}
	return th;	
}
/**
 *****************************************************************
 * \fn void TPOS_Yield(void) 
 * Give the CPU time to an other thread. If the current thread
 * is the main thread sentence it to death and create a new one
 *****************************************************************
 */
void
TPOS_Yield(void) 
{
	Thread *newTh;
	EV_Trigger(&Thread_Current()->wakeEvent);
	newTh = GetNewThread();
	if(newTh) {
		Thread_Switch(newTh);
	} else {
		/* Out of threads == Big shit */
	}
}
/*
 **********************************************************************
 * \fn void Mutex_Lock(Mutex *rs) 
 * Wenn ein Prozess eingeschläfert wird muss ein neuer auf die 
 * Haupt eventschleife angesetzt werden. There is no need
 * for creating a new thread here because the thread which owns the
 * RSEMA must exist 
 * Falsch denn Eine RSEMA is owned by event. So there is a need for
 * running a new thread:
 **********************************************************************
 */
void
Mutex_Lock(Mutex *rs) 
{
	Thread *newTh;
	while(1) {	
		if(!rs->owner) {
			/* Shit, because of pool it is not a thread-owned semaphore */
			rs->owner = Thread_Current();
			return;
		} else {
			//Con_Printf_P("Rsema is NOT FREE\n");
			Thread_Current()->waitNext = rs->waitHead;
			rs->waitHead = Thread_Current();
			newTh = GetNewThread();
			if(newTh) {
				Thread_Switch(newTh);
			} else {
				/* Shit out of threads, which thread should I call now ? */
			}
		}
	}
}


/**
 **************************************************************************
 * \fn void Mutex_Unlock(Mutex *rs) 
 * Eine rsema kann immer nur vom Event dem es gehört freigegeben werden.
 * Eventuelle schläfer müssen zur vernichtung beim entern von main loop
 * freigegeben werden. 
 **************************************************************************
 */
void
Mutex_Unlock(Mutex *rs) 
{
	Thread *th;
	if(rs->owner/* == Thread_Current()*/) {
		rs->owner = NULL;
		/* Wake up one of the threads waiting for this resource */
		if(rs->waitHead) {
			th = rs->waitHead;
			rs->waitHead = th->waitNext;
			th->waitNext = NULL;		
			EV_Trigger(&th->wakeEvent);
		}
	} else {
		Con_Printf_P("Error, TPOSRsema has no owner \n");
	} 
}

/**
 *************************************************************
 * \fn static void CSema_Up(EvCSema *cs); 
 * Darf auch in Interrupthandlern aufgerufen werden.
 * Ein Prozess der Aufgeweckt wird ist zwangsweise 
 * schon auf der Todesliste
 ************************************************************
 */
void
CSema_Up(CSema *cs) {
	Flags_t flags;
	Thread *th;
	uint16_t cnt;
	flags = save_ipl_set(IPL_EVENTS);
	cnt = ++cs->cnt;	
	restore_ipl(flags);
	if(cnt != 1) {
		return;
	}
	flags = save_ipl_set(IPL_EVENTS);
	if(cs->waitHead) {
		th = cs->waitHead;
		cs->waitHead = th->waitNext;
		th->waitNext = NULL;
		EV_Trigger(&th->wakeEvent);
		//Con_Printf("CSema Waking up thread %d\n",th->threadId);
	}
	restore_ipl(flags);
}

/**
 ****************************************************************
 * \fn static void CSema_Down(EvCSema *cs) 
 * Darf nicht aus Interrupt aufgerufen werden,
 * Wenn es einen Prozess einschläfert muss es einen neuen
 * auf main ansetzen.
 ****************************************************************
 */
void
CSema_Down(CSema *cs) 
{
	Flags_t flags;
	Thread *newTh;
	while(1) {
		flags = save_ipl_set(IPL_EVENTS);
		if(cs->cnt > 0) {
			cs->cnt--;	
			restore_ipl(flags);
			return;
		} 
		Thread_Current()->waitNext = cs->waitHead;
		cs->waitHead = Thread_Current();
		newTh = GetNewThread();
		if(newTh) {
			restore_ipl(flags);
			Thread_Switch(newTh);
		} else {
			while(1);
		}
	}
}

/**
 **********************************************
 * \fn void CSema_Init(EvCSema *cs) 
 **********************************************
 */
void
CSema_Init(CSema *cs) {
	memset(cs,0,sizeof(*cs));
}

void
Mutex_Init(Mutex *rs) {
	memset(rs,0,sizeof(*rs));
}

/** END of code, rest is testcode */

static CSema testCSema;

static void
CSemaUpTest(void *dummy) {
	Con_Printf_P("I'm the wakeup timer\n");
	SleepMs(100);
	CSema_Up(&testCSema);
}

TIMER_DECLARE(tmrEvOSTest,CSemaUpTest,0);

static void
CSema_Test(void) {
	CSema_Init(&testCSema);
	Timer_Start(&tmrEvOSTest,4000);
	CSema_Down(&testCSema);		
}

Mutex testRSema;

static void
RSemaTestEvProc(void *evData) 
{
	static uint8_t toggle = false;
	toggle = !toggle;
	Con_Printf_P("RSemaTestTimer %08lx %d\n",Thread_Current(),toggle);
	if(toggle == true) {
		Mutex_Lock(&testRSema);
	} else {
		Mutex_Unlock(&testRSema);
	}
}

TIMER_DECLARE(tmrRSemaToggle,RSemaTestEvProc,0);
/**
 *********************************************************************************
 * \fn static void RSema_Test(void);
 *
 *********************************************************************************
 */
static void
RSema_Test(void) 
{
	Timer_Start(&tmrRSemaToggle,0);
	SleepMs(100);
	Timer_Start(&tmrRSemaToggle,4000);
	Con_Printf_P("Trying to lock rsema\n");
	Mutex_Lock(&testRSema);
	Con_Printf_P("Success, Unlocking rsema\n");
	Mutex_Unlock(&testRSema);
}

/**
 *****************************************************************************
 * \fn static int8_t cmd_csema(Interp * interp, uint8_t argc, char *argv[])
 *****************************************************************************
 */
static int8_t
cmd_csema(Interp * interp, uint8_t argc, char *argv[])
{
	CSema_Test();
	return 0;
}

INTERP_CMD(csema, cmd_csema,
           "csema       # Test if TPOS Counting semaphores are working");

/**
 *****************************************************************************
 * \fn static int8_t cmd_rsema(Interp * interp, uint8_t argc, char *argv[])
 *****************************************************************************
 */
static int8_t
cmd_rsema(Interp * interp, uint8_t argc, char *argv[])
{
	RSema_Test();
        return 0;
}

INTERP_CMD(rsema, cmd_rsema,
           "rsema       # Test if TPOS Mutexes are working correctly");


/**
 ***********************************************************************************
 * \fn static int8_t cmd_tpos(Interp *interp,uint8_t argc,char *argv[]);
 * Command thread pool operating system.
 * Shows the statistics
 ***********************************************************************************
 */
static int8_t 
cmd_tpos(Interp *interp,uint8_t argc,char *argv[]) {
	Interp_Printf_P(interp,"NumUsedThreads %u\n",stat_NumThreads);
	Interp_Printf_P(interp,"MaxUsedThreads %u\n",stat_MaxThreads);
	//Interp_Printf_P(interp,"CurrentThread  %u\n",Thread_Current()->threadId);
	return 0;
}

INTERP_CMD(tpos, cmd_tpos,
           "tpos        # Show Thread pool OS statistics");

/**
 ****************************************************
 * \fn void TPOS_Init(void) 
 ****************************************************
 */
void 
TPOS_Init(void) 
{
	Thread *th;
	uint16_t i;

	Thread_Current()->next = NULL;
	EV_Init(&Thread_Current()->wakeEvent,TPos_ThreadWake,Thread_Current());
	for(i = 1; i < THREAD_POOL_SIZE; i++) {
		th = Thread_Alloc();		
		Thread_Init(th,EV_Loop);
		th->next = unusedqHead;		
		unusedqHead = th;
		EV_Init(&th->wakeEvent,TPos_ThreadWake,th);
	}
	Mutex_Init(&testRSema);
}
