/**
 **********************************************************
 * Timer  
 **********************************************************
 */
 

#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include "timer.h"
#include "irqflags.h"
#include "events.h"

Timer *timerHead;
static TimeMs_t clock_tick;

/**
 *****************************************************************
 * Timer Interrupt. Increment the clock_tick counter and
 * Check if at least one timer is timeouted.
 *****************************************************************
 */
ISR(TCC0_OVF_vect)
{
	clock_tick++;
	while(timerHead && (timerHead->timeout == clock_tick)) {
		timerHead->busy = 0;
		EV_Trigger(&timerHead->evTimeout);
		timerHead = timerHead->next;
	}
}
/*
 *****************************************************************
 * Timer_Start
 *****************************************************************
 */
int8_t 
Timer_Start(Timer * timer, TimeMs_t delay_ms)
{
	Flags_t ipl;
	Timer *cursor, *prev;
	ipl = save_ipl_set(IPL_TIMERS);
	if (timer->busy || Event_Pending(&timer->evTimeout)) {
		restore_ipl(ipl);
		return -1;
	}
	if(delay_ms == 0) {
                restore_ipl(ipl);
                EV_Trigger(&timer->evTimeout);
                return 0;
        }
	timer->busy = 1;
	timer->timeout = clock_tick + delay_ms;
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
	restore_ipl(ipl);
	return 0;
}

static void TestTimerProc(void *timerData);
static TIMER_DECLARE(tmr_test,TestTimerProc,NULL);

/*
 ***************************************************************
 * TestTimerProc
 * Procedure for Testing the timer.
 ***************************************************************
 */
static void 
TestTimerProc(void *timerData) 
{
	PORTC.OUTTGL = 0x40;
	Timer_Start(&tmr_test,500);
}

/**
 *****************************************************************
 * \fn void Timer_Remove(Timer * timer);
 * Remove a timer from the linked list of active timers.
 * Current problem: If the event is already triggered
 * it will be invoked. 
 * \param timer The timer to remove.
 *****************************************************************
 */
void Timer_Cancel(Timer * timer)
{
	Timer *cursor, *prev;
	uint8_t ipl;
	if(!timer->busy) {
		Event_Cancel(&timer->evTimeout);
		return;
	}
	ipl = save_ipl_set(IPL_TIMERS);
	for (prev = NULL, cursor = timerHead; cursor; prev = cursor, cursor = cursor->next) {
		if (timer == cursor) {
			if (prev) {
				prev->next = cursor->next;
			} else {
				timerHead = cursor->next;
			}
			cursor->busy = 0;
			/* cursor->next = NULL */
		}
	}
	restore_ipl(ipl);
}

/**
 *************************************************************
 * Get the time in milliseconds
 *************************************************************
 */
TimeMs_t 
TimeMs_Get(void) {
	TimeMs_t tm;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		tm = clock_tick;
	}
	return tm;
}

/*
 ******************************************************************************
 * \fn void SleepMs(TimeMs_t delay);
 * Call the main event loop until some time is elapsed.
 * Use Delay only for very timing uncritical things. Its not recommended to
 * call delay for longer than 10 ms because of events lower in stack.
 ******************************************************************************
 */
void
SleepMs(TimeMs_t delay) {
        TimeMs_t starttime = TimeMs_Get();
        while((TimeMs_Get() - starttime) < delay) {
                EV_Yield();
        }
}

/*
 **************************************************************
 * \fn void Timers_Init(void)
 **************************************************************
 */
void
Timers_Init(void) 
{
	TCC0.CTRLA = 1; /* Divide by 1 */
        TCC0.INTCTRLA = TC_OVFINTLVL_LO_gc;
        TCC0.PER = 32000;
	PORTC.DIRSET = (1 << 6);
//	PORTE.DIRSET = 0xff;
//	PORTC.DIRSET = 0xff;
	Timer_Start(&tmr_test,500);
}
