#include "interrupt_handlers.h"
#include "timer.h"
#include "iodefine.h"
#include "types.h"
#include <stdlib.h>
#include "events.h"
#include "atomic.h"
#include "config.h"
#include "console.h"

static Timer *timerHead = NULL;
volatile TimeMs_t g_TimeClockTick = 0;
/*
 ******************************************************************************
 * \fn void SleepMs(TimeMs_t delay);
 * Call the main event loop until some time is elapsed.
 * Use Delay only for very timing uncritical things. Its not recommended to
 * call delay for longer than 10 ms because of events lower in stack.
 ******************************************************************************
 */
void
SleepMs(TimeMs_t delay)
{
	TimeMs_t starttime = TimeMs_Get();
	while (Time_Sum(TimeMs_Get(), -starttime) < delay) {
		EV_Yield();
	}
}

/**
 *****************************************************************
 * \fn void timer_b0_interrupt(void);
 * Timer Interrupt routine.
 * Increment the clock_tick counter and
 * wake up Event handlers for timers which are timeoutet at
 * exactly this clock tick. This interrupt handler has the lowest
 * possible IPL of 1.
 *****************************************************************
 */

void
Excep_CMT0_CMI0(void)
{
	Timer *timer;
	g_TimeClockTick++;
	while (timerHead && (timerHead->timeout == g_TimeClockTick)) {
		timer = timerHead;
		EV_Trigger(&timer->evTimeout);
		timerHead = timer->next;
		timer->busy = 0;
	}
}

/*
 *****************************************************************
 * \fn int8_t Timer_Start(Timer *timer, TimeMs_t delay_ms);
 * Start a timer if not already active. 
 *****************************************************************
 */
int8_t
Timer_Start(Timer * timer, TimeMs_t delay_ms)
{
	uint32_t flags;
	Timer *cursor, *prev;

	SAVE_FLAGS_SET_IPL(flags, TIMERS_IPL);
	if (timer->busy || Event_Pending(&timer->evTimeout)) {
		RESTORE_FLAGS(flags);
		return -1;
	}
	if (delay_ms == 0) {
		RESTORE_FLAGS(flags);
		EV_Trigger(&timer->evTimeout);
		return 0;
	}
	timer->busy = 1;
	timer->delay_ms = delay_ms;
	timer->timeout = g_TimeClockTick + delay_ms;
	for (prev = NULL, cursor = timerHead; cursor; prev = cursor, cursor = cursor->next) {
		if ((sTimeMs_t) (cursor->timeout - timer->timeout) > 0) {
			break;
		}
	}
	timer->next = cursor;
	if (prev) {
		prev->next = timer;
	} else {
		timerHead = timer;
	}
	RESTORE_FLAGS(flags);
	return 0;
}

/**
 *****************************************************************
 * \fn void Timer_Cancel(Timer * timer);
 * Remove a timer from the linked list of active timers.
 * \param timer The timer to cancel.
 *****************************************************************
 */
void
Timer_Cancel(Timer * timer)
{
	Timer *cursor, *prev;
	uint32_t flags;
	if (!timer->busy) {
		EV_Cancel(&timer->evTimeout);
		return;
	}
	SAVE_FLAGS_SET_IPL(flags, TIMERS_IPL);
	for (prev = NULL, cursor = timerHead; cursor; prev = cursor, cursor = cursor->next) {
		if (timer == cursor) {
			if (prev) {
				prev->next = cursor->next;
			} else {
				timerHead = cursor->next;
			}
			cursor->busy = 0;
		}
	}
	RESTORE_FLAGS(flags);
}

/*
 ******************************************************************** 
 * \fn uint32_t TimeUs_Get(void);
 * Get the time in microseconds.
 * May not be called with interrupts disabled.
 ********************************************************************
 */
uint64_t
TimeNs_Get(void)
{
	TimeMs_t now;
	uint64_t ns;
	do {
		now = TimeMs_Get();
		ns = CMT0.CMCNT * 160;
	} while (now != TimeMs_Get());
	return now * (uint64_t) 1000000 + ns;
}

uint64_t
TimeUs_Get(void)
{
	TimeMs_t now;
	uint64_t us;
	do {
		now = TimeMs_Get();
		us = CMT0.CMCNT * 160 / 1000;
	} while (now != TimeMs_Get());
	return (uint64_t)now * 1000 + us;
}

static uint32_t cnt_per_ms = 0;
static uint16_t ns_per_call;

static void
DelayNs(uint32_t ns)
{
	uint32_t max;
	volatile uint32_t cnt;
	max = ((uint64_t) (ns - ns_per_call) * cnt_per_ms) / 1000000;
	for (cnt = 0; cnt < max; cnt++) ;
}

void
CalibrateDelayLoop()
{
	uint32_t i;
	TimeMs_t now;
	TimeMs_t diff;
	now = TimeMs_Get();
	while (now == TimeMs_Get()) ;
	now = TimeMs_Get();
	for (i = 0; i < 10000; i++) {
		DelayNs(0);
		DelayNs(0);
		DelayNs(0);
		DelayNs(0);
		DelayNs(0);
		DelayNs(0);
		DelayNs(0);
		DelayNs(0);
		DelayNs(0);
		DelayNs(0);
	}
	diff = TimeMs_Get() - now;
	ns_per_call = diff * (1050000 / 100000);
	cnt_per_ms = 10000;

	now = TimeMs_Get();
	while (now == TimeMs_Get()) ;
	now = TimeMs_Get();
	DelayNs(100 * 1000 * 1000);	/* Excpect 10 ms */
	diff = TimeMs_Get() - now;

	Con_Printf("ns_per_call %u ns\n", ns_per_call);
	Con_Printf("Delay loop needed %u ms\n", diff);
	cnt_per_ms = ((100 * cnt_per_ms)) / diff;
	//ps_per_cnt_per_ms = ((diff * 1000000) - 1000 * ns_per_call);
	Con_Printf("cnt_per_ms %u\n", cnt_per_ms);
	/* Now test it */

	now = TimeMs_Get();
	while (now == TimeMs_Get()) ;
	now = TimeMs_Get();
	DelayNs(100 * 1000 * 1000);	/* Excpect 10 ms */
	diff = TimeMs_Get() - now;
	Con_Printf("Test needed %u\n", diff);

	now = TimeMs_Get();
	while (now == TimeMs_Get()) ;
	now = TimeMs_Get();
	for (i = 0; i < 1000000; i++) {
		DelayNs(1000);
	}
	diff = TimeMs_Get() - now;
	Con_Printf("Test needed %u\n", diff);
}

/*
 **************************************************************
 * \fn void Timers_Init(void)
 * Initialize the hardware timer which is the base for the
 * 1ms clock tick of the timer module and install a test timer.
 **************************************************************
 */
void
Timers_Init(void)
{
	MSTP(CMT0) = 0;
	CMT.CMSTR0.BIT.STR0 = 1;
	CMT0.CMCR.BIT.CKS = 0;
	CMT0.CMCOR = (F_PCLK / 1000 / 8);
	CMT0.CMCR.BIT.CMIE = 1;
	IPR(CMT0, CMI0) = 2;
	IEN(CMT0, CMI0) = 1;
}
