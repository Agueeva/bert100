#ifndef _TIMER_H
#define _TIMER_H
#include <stdint.h>
#include <util/atomic.h>
#include "events.h"
#include "irqflags.h"

#define IPL_TIMERS IPL_LOW

typedef uint16_t TimeMs_t;
/* signed time only used internally */
typedef int16_t sTimeMs_t;

/* Timer Proc is invoked when timer is timeouted */
typedef void TimerProc(void *TimerData);

/*
 **************************************************************************
 * Timer structure, All fields are private. Do not access them directly.
 **************************************************************************
 */

typedef struct Timer {
	Event evTimeout;
	struct Timer *next;
	uint8_t busy;
	TimeMs_t timeout;
} Timer;

/* 
 ******************************************************************************
 * Use TIMER_DECLARE or Timer_Init();
 * Timer_Init can not be used for active timers ! Stop them first if
 * proc or data needs to be changed at runtime.
 ******************************************************************************
 */
#define TIMER_DECLARE(name,tproc,cData) \
        Timer name = { \
        .evTimeout.evProc = (tproc), \
        .evTimeout.eventData = (cData), \
        .evTimeout.evState = EV_StateIdle, \
        .busy = 0 \
	};


static inline void
Timer_Init(Timer *timer,TimerProc *proc,void *timerData) 
{
	EV_Init(&timer->evTimeout,proc,timerData);
	timer->busy = 0;
}

static inline void
Timer_SetData(Timer *timer,void *timerData)
{
	timer->evTimeout.eventData = timerData;
}

TimeMs_t TimeMs_Get(void);

static inline TimeMs_t 
Time_Add(TimeMs_t time1,TimeMs_t time2) 
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		return (time2 + time1);
	}
}
static inline uint8_t Timer_Busy(Timer *timer) 
{
	return timer->busy; 
}
int8_t Timer_Start(Timer *timer,TimeMs_t timeout_ms);
void Timer_Cancel(Timer * timer);
void SleepMs(TimeMs_t delay);
void Timers_Init(void);

#endif
