/**
 *****************************************************************************************
 * Timer module 
 *****************************************************************************************
 */
#include "interrupt_handlers.h"
#include "timer.h"
#include "iodefine.h"
#include "types.h"
#include <stdlib.h>
#include "events.h"
#include "atomic.h"
#include "config.h"
#include "console.h"
#include "interpreter.h"
#include "hex.h"

static Timer *timerHead = NULL;
volatile TimeMs_t g_TimeClockTick = 0;

void
SleepUs(uint32_t sleepUs)
{
	uint64_t starttime = TimeNs_Get();
	uint64_t sleepNs = 1000 * (uint64_t) sleepUs;
	while ((TimeNs_Get() - starttime) < sleepNs) {
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
	ENABLE_IRQ();
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
	return (uint64_t) now *1000 + us;
}

#define DELAY 120
#define DELAY_US 5

static int8_t
cmd_delay(Interp * interp, uint8_t argc, char *argv[])
{
	TimeMs_t beforeMs, afterMs;
	uint32_t i;
	beforeMs = TimeMs_Get();
	for (i = 0; i < (10000000 / DELAY); i++) {
		DelayNs(DELAY);
		DelayNs(DELAY);
		DelayNs(DELAY);
		DelayNs(DELAY);
		DelayNs(DELAY);
		DelayNs(DELAY);
		DelayNs(DELAY);
		DelayNs(DELAY);
		DelayNs(DELAY);
		DelayNs(DELAY);
	}
	afterMs = TimeMs_Get();
	EV_Yield();
	Con_Printf("Needed %lu ms \n", afterMs - beforeMs);
	beforeMs = TimeMs_Get();
	for (i = 0; i < (10000 / DELAY_US); i++) {
		DelayUs(DELAY_US);
		DelayUs(DELAY_US);
		DelayUs(DELAY_US);
		DelayUs(DELAY_US);
		DelayUs(DELAY_US);
		DelayUs(DELAY_US);
		DelayUs(DELAY_US);
		DelayUs(DELAY_US);
		DelayUs(DELAY_US);
		DelayUs(DELAY_US);
	}
	afterMs = TimeMs_Get();
	EV_Yield();
	Con_Printf("Needed %lu ms \n", afterMs - beforeMs);
	return 0;
}

INTERP_CMD(delayCmd, "delay", cmd_delay, "delay   # test the delay loop");

/**
 *************************************************************
 * \fn void DelayMs(uint32_t ms)
 * Delay for some milliseconds. Use with care, better
 * use SleepMs when possible because this eats up the
 * CPU time. 
 *************************************************************
 */
void
DelayMs(uint32_t ms)
{
	uint32_t i;
	for (i = 0; i < ms; i++) {
		DelayUs(1000);
	}
}

/**
 ******************************************************************
 * \fn static int8_t cmd_uptime(Interp *interp);
 * Printf the system uptime.
 ******************************************************************
 */
static int8_t
cmd_uptime(Interp * interp, uint8_t argc, char *argv[])
{
	TimeMs_t uptime = TimeMs_Get();
	Interp_Printf_P(interp, "%luh %lumin %lus\n", uptime / UINT32_C(3600000),
			(uptime % UINT32_C(3600000)) / UINT32_C(60000),
			(uptime % UINT32_C(60000)) / UINT32_C(1000));
	return 0;
}

INTERP_CMD(uptimeCmd, "uptime", cmd_uptime, "uptime     # Print the system uptime");

static int8_t
cmd_msleep(Interp * interp, uint8_t argc, char *argv[])
{
	uint32_t sleepTimeMs; 
	if(argc > 1) {
		sleepTimeMs = astrtoi32(argv[1]);
		SleepMs(sleepTimeMs);
	} else {
		return -EC_BADNUMARGS;
	}
	return 0;
}

INTERP_CMD(msleepCmd, "msleep", cmd_msleep, "msleep   <time/ms> #  Sleep for some milliseconds");

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
	Interp_RegisterCmd(&delayCmd);
	Interp_RegisterCmd(&uptimeCmd);
	Interp_RegisterCmd(&msleepCmd);
}
