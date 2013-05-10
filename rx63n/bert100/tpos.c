/*
 ****************************************************************
 * Thread pool OS. Use a separate thread on demand. Possible
 * demands are "yield" or request of a locked semaphore.
 * In most exactly one thread is enabled. Whenever 
 * "yield" or a semaphore requests terminate they will disable
 * one thread if they did enable one. 
 ****************************************************************
 */
#include <string.h>
#include "types.h"
#include "atomic.h"
#include "threads.h"
#include "events.h"
#include "console.h"
#include "tpos.h"
#include "interpreter.h"
#include "timer.h"
#include "hex.h"

#define THREAD_POOL_SIZE	6

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
MoveRunningToUnused(Thread * th)
{
	th->next = unusedqHead;
	unusedqHead = th;
	stat_NumThreads--;
	return;
}

/**
 ********************************************************************************************
 * This is an Event handler which avakes a sleeping thread. It always disables the invoking 
 * thread. S
 ********************************************************************************************
 */
static void
TPos_ThreadWake(void *eventData)
{
	Thread *nextTh = eventData;
	MoveRunningToUnused(Thread_Current());
	Thread_Switch(nextTh);
}

/**
 **********************************************************************
 * Get a thread from the list of unused threads
 **********************************************************************
 */
static Thread *
GetNewThread(void)
{
	Thread *th;
	th = unusedqHead;
	if (th) {
		unusedqHead = unusedqHead->next;
		th->next = NULL;
		stat_NumThreads++;
		if (stat_NumThreads > stat_MaxThreads) {
			stat_MaxThreads = stat_NumThreads;
		}
	} else {
		Con_Printf("Out of threads\n");
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
	if (newTh) {
		Thread_Switch(newTh);
	} else {
		/* 
		 *********************************************************************
		 * Out of threads == Big shit, hopefully the yield condition 
		 * goes away by itself 
		 *********************************************************************
		 */
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
Mutex_Lock(Mutex * rs)
{
	Thread *newTh;
	while (1) {
		if (!rs->owner) {
			/* Shit, because of pool it is not a thread-owned semaphore */
			rs->owner = Thread_Current();
			return;
		} else {
			//Con_Printf("Rsema is NOT FREE\n");
			Thread_Current()->waitNext = rs->waitHead;
			rs->waitHead = Thread_Current();
			/* 
			 ************************************************************
			 * If a semaphore is locked there must be at least 
			 * one other thread owning the semaphore 
			 ************************************************************
			 */
			newTh = GetNewThread();
			if (newTh) {
				Thread_Switch(newTh);
			} else {
				/* Shit out of threads, Let the watchdog reboot the system */
				while (1) ;
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
Mutex_Unlock(Mutex * rs)
{
	Thread *th;
	if (rs->owner /* == Thread_Current() */ ) {
		rs->owner = NULL;
		/* Wake up the threads which are waiting for this resource */
		if (rs->waitHead) {
			th = rs->waitHead;
			rs->waitHead = th->waitNext;
			th->waitNext = NULL;
			EV_Trigger(&th->wakeEvent);
		}
	} else {
		Con_Printf("Error, TPOSRsema has no owner \n");
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
CSema_Up(CSema * cs)
{
	Flags_t flags;
	Thread *th;
	uint16_t cnt;
	SAVE_FLAGS_SET_IPL(flags, EVENTS_IPL);
	cnt = ++cs->cnt;
	RESTORE_FLAGS(flags);
	if (cnt != 1) {
		return;
	}
	SAVE_FLAGS_SET_IPL(flags, EVENTS_IPL);
	if (cs->waitHead) {
		th = cs->waitHead;
		cs->waitHead = th->waitNext;
		th->waitNext = NULL;
		EV_Trigger(&th->wakeEvent);
	}
	RESTORE_FLAGS(flags);
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
CSema_Down(CSema * cs)
{
	Flags_t flags;
	Thread *newTh;
	while (1) {
		SAVE_FLAGS_SET_IPL(flags, EVENTS_IPL);
		if (cs->cnt > 0) {
			cs->cnt--;
			RESTORE_FLAGS(flags);
			return;
		}
		//Con_Printf("Need to sleep\n");
		Thread_Current()->waitNext = cs->waitHead;
		cs->waitHead = Thread_Current();
		newTh = GetNewThread();
		if (newTh) {
			RESTORE_FLAGS(flags);
			Thread_Switch(newTh);
		} else {
			/* Shit, no threads available */
			while (1) ;
		}
	}
}

/**
 **********************************************
 * \fn void CSema_Init(EvCSema *cs) 
 **********************************************
 */
void
CSema_Init(CSema * cs)
{
	memset(cs, 0, sizeof(*cs));
}

void
Mutex_Init(Mutex * rs)
{
	memset(rs, 0, sizeof(*rs));
}

/** END of code, rest is testcode */

static CSema testCSema;

static void
CSemaUpTest(void *dummy)
{
	Con_Printf("I'm the wakeup timer\n");
	SleepMs(100);
	CSema_Up(&testCSema);
}

TIMER_DECLARE(tmrEvOSTest, CSemaUpTest, 0);

static void
CSema_Test(void)
{
	CSema_Init(&testCSema);
	Timer_Start(&tmrEvOSTest, 4000);
	CSema_Down(&testCSema);
}

Mutex testRSema;

static void
RSemaTestEvProc(void *evData)
{
	static uint8_t toggle;
	toggle = !toggle;
	Con_Printf("Test %08lx %d\n", Thread_Current(), toggle);
	if (toggle == true) {
		Mutex_Lock(&testRSema);
	} else {
		Mutex_Unlock(&testRSema);
	}
}

TIMER_DECLARE(tmrRSemaToggle, RSemaTestEvProc, 0);
/**
 *********************************************************************************
 * \fn static void RSema_Test(void);
 *
 *********************************************************************************
 */
static void
RSema_Test(void)
{
	Timer_Start(&tmrRSemaToggle, 0);
	SleepMs(100);
	Timer_Start(&tmrRSemaToggle, 4000);
	Con_Printf("Trying to lock rsema\n");
	Mutex_Lock(&testRSema);
	Con_Printf("Success, Unlocking rsema\n");
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

INTERP_CMD(csemaCmd, "csema", cmd_csema,
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

INTERP_CMD(rsemaCmd, "rsema", cmd_rsema,
	   "rsema       # Test if TPOS Mutexes are working correctly");

/**
 ***********************************************************************************
 * \fn static int8_t cmd_tpos(Interp *interp,uint8_t argc,char *argv[]);
 * Command thread pool operating system.
 * Shows the statistics
 ***********************************************************************************
 */
static int8_t
cmd_tpos(Interp * interp, uint8_t argc, char *argv[])
{
	Interp_Printf_P(interp, "NumUsedThreads %u\n", stat_NumThreads);
	Interp_Printf_P(interp, "MaxUsedThreads %u\n", stat_MaxThreads);
	Interp_Printf_P(interp, "CurrentThread  %u\n", Thread_Current()->threadId);
	return 0;
}

INTERP_CMD(tposCmd, "tpos", cmd_tpos, "tpos        # Show Thread pool OS statistics");
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
	EV_Init(&Thread_Current()->wakeEvent, TPos_ThreadWake, Thread_Current());
	for (i = 1; i < THREAD_POOL_SIZE; i++) {
		th = Thread_Alloc();
		Thread_Init(th, EV_Loop);
		th->next = unusedqHead;
		unusedqHead = th;
		EV_Init(&th->wakeEvent, TPos_ThreadWake, th);
	}
	Mutex_Init(&testRSema);
	Interp_RegisterCmd(&csemaCmd);
	Interp_RegisterCmd(&rsemaCmd);
	Interp_RegisterCmd(&tposCmd);
}
