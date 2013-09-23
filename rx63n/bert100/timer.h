#ifndef _TIMER_H
#define _TIMER_H
#include "events.h"
#include "types.h"
#include "config.h"

void INT_Excep_CMTU0_CMI0(void) __attribute__ ((interrupt));

/**
 **************************************************************
 * \def TIMERS_IPL
 * Interrupt privilege level during Timer queue manipulation.
 * A Timer queue manipulation procedure may not be called
 * from an interrupt with a higher IPL.
 **************************************************************
 */
#define TIMERS_IPL 4

typedef uint32_t TimeMs_t;

/* signed time only used internally */
typedef int32_t sTimeMs_t;

/* Timer Proc is invoked when timer is timeouted */
typedef void TimerProc(void *TimerData);

/*
 **************************************************************************
 * Timer structure, All fields are private. Do not access them directly.
 **************************************************************************
 */

typedef struct Timer {
	Event evTimeout;
	uint8_t busy;
	TimeMs_t timeout;
	TimeMs_t delay_ms;
	struct Timer *next;
} Timer;

//define pT2pE(x) (&(x)->evTimeout)

extern volatile TimeMs_t g_TimeClockTick;
/* 
 ******************************************************************************
 * Use TIMER_DECLARE or Timer_Init();
 * Timer_Init can not be used for active timers ! Stop them first if
 * proc or data needs to be changed at runtime.
 ******************************************************************************
 */
#define TIMER_DECLARE(name,tproc,cData) Timer name = {{tproc,cData,EV_StateIdle,NULL,NULL},0};

static inline void
Timer_Init(Timer * timer, TimerProc * proc, void *timerData)
{
	EV_Init(&timer->evTimeout, proc, timerData);
	timer->busy = 0;
}

static inline inline TimeMs_t
TimeMs_Get(void)
{
	return g_TimeClockTick;
}

/**
 *********************************************************************
 * \fn TimeMs_t Time_Sum(TimeMs_t time1,TimeMs_t time2);
 * Calculate the sum of time2 and time1.
 *********************************************************************
 */

static inline TimeMs_t
Time_Sum(TimeMs_t time1, TimeMs_t time2)
{
	return time2 + time1;
}

/**
 *********************************************************
 * Calculate the remaining time.
 *********************************************************
 */
static inline TimeMs_t
Timer_Remaining(Timer * timer)
{
	if (!timer->busy) {
		return 0;
	} else {
		return timer->timeout - TimeMs_Get();
	}
};

static inline uint8_t
Timer_Busy(Timer * timer)
{
	return timer->busy || Event_Pending(&timer->evTimeout);
}

int8_t Timer_Start(Timer * timer, TimeMs_t timeout_ms);
void Timer_Cancel(Timer * timer);
static inline void
Timer_Mod(Timer * timer, TimeMs_t timeout_ms)
{
	Timer_Cancel(timer);
	Timer_Start(timer, timeout_ms);
}

static inline bool
is_later(TimeMs_t time1, TimeMs_t time2)
{
	if ((sTimeMs_t) (time1 - time2) > 0) {
		return true;
	} else {
		return false;
	}
}

uint64_t TimeNs_Get(void);
uint64_t TimeUs_Get(void);
void SleepUs_Get(uint32_t us);

/**
 ***********************************************************************
 * delay loop needs 4 clock cycles per loopcnt
 * According to ABI specification rej10j2062_ccrx_v100um.pdf page 203
 * the first argument of a function is in the register r1.
 ***********************************************************************
 */

static inline void
_delay_loop(uint32_t loopcnt)
{
        __asm__ volatile(
	    "label9279%=:      \n\t"
            "   sub #1,%0    \n\t"
            "   bpz.b label9279%=":"=r" (loopcnt):"0"(loopcnt):);
}

/**
 ***************************************************************************
 * \def DelayNs(ns)
 * Macro calculating the Loop count for a Delay in Nanoseconds and
 * calling the delay loop.
 ***************************************************************************
 */
#define DelayNs(ns)  { _delay_loop(((ns) * (F_CPU / 100000) / 40000)); }
#define DelayUs(us)  { _delay_loop(((us) * (F_CPU / 1000) / 4000)); }
void DelayMs(uint32_t ms);

void Timers_Init(void);
#endif
