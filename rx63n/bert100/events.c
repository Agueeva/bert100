/**
 **********************************************
 * Event Queue
 **********************************************
 */
#include <stdlib.h>
#include "types.h"
#include "atomic.h"
#include "events.h"
#include "timer.h"
#include "tpos.h"

static Event *queueHead = NULL;
static Event *queueTail = NULL;

static inline void
Event_Enqueue(Event * ev)
{
	ev->next = NULL;
	ev->prev = queueTail;
	if (!queueTail) {
		queueHead = ev;
	} else {
		queueTail->next = ev;
	}
	queueTail = ev;
}

/*
 ***********************************************************
 * Trigger an event by inserting it into the queue or by
 * marking it for reinsertion.
 ************************************************************
 */
void
EV_Trigger(Event * ev)
{
	Flags_t flags;
	SAVE_FLAGS_SET_IPL(flags, EVENTS_IPL);
	if (ev->evState == EV_StateIdle) {
		Event_Enqueue(ev);
		ev->evState = EV_StatePending;
	} else if (ev->evState == EV_StateExecuting) {
		ev->evState = EV_StateExepending;
	}
	RESTORE_FLAGS(flags);
}

/*
 ***************************************************************** 
 * Never call this directly (with an idle event) 
 *****************************************************************
 */
void
_EventCancel(Event * ev)
{
	if (ev == queueHead) {
		queueHead = ev->next;
		if (queueHead == NULL) {
			queueTail = NULL;
		}
	} else {
		ev->prev->next = ev->next;
		if (ev == queueTail) {
			queueTail = ev->prev;
		} else {
			ev->next->prev = ev->prev;
		}
	}
	ev->evState = EV_StateIdle;
}

void
EV_Yield(void)
{
	if (queueHead) {
		TPOS_Yield();
	}
}

/**
 **********************************************
 * \fn void EV_DoOneEvent(void);
 * Execute one event if available.
 **********************************************
 */
static void
EV_DoOneEvent(void)
{
	Event *ev;
	Flags_t flags;
	//_asm("brk");
	SAVE_FLAGS_SET_IPL(flags, EVENTS_IPL);
	ev = queueHead;
	if (ev) {
		ev->evState = EV_StateExecuting;
		queueHead = ev->next;
		if (!queueHead) {
			queueTail = NULL;
		}
		RESTORE_FLAGS(flags);
		if (ev->evProc) {
			ev->evProc(ev->eventData);
		}
		SAVE_FLAGS_SET_IPL(flags, EVENTS_IPL);
		if (ev->evState == EV_StateExepending) {
			Event_Enqueue(ev);
			ev->evState = EV_StatePending;
		} else {
			ev->evState = EV_StateIdle;
		}
	}
	RESTORE_FLAGS(flags);
}

/**
 *************************************************************
 * \fn void EV_Loop(void)
 * Event Loop: The main loop of the application
 *************************************************************
 */
void
EV_Loop(void)
{
	while (1) {
		EV_DoOneEvent();
	}
}
