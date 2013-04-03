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
	if (ev->prev) {
		ev->prev->next = ev->next;
	} else {
		queueHead = ev->next;
	}
	if (ev->next) {
		ev->next->prev = ev->prev;
	} else {
		queueTail = ev->prev;
	}
	ev->evState = EV_StateIdle;
}

void
EV_Yield(void)
{
	if (queueHead) {
		TPOS_Yield();
	} else {
		//TPOS_CondYield();
	}
}

/**
 **********************************************
 * \fn void EV_DoOneEvent(void);
 * Execute one event if available.
 **********************************************
 */
void
_EV_DoOneEvent(void)
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
#if 0
	if (!ev) {
		TPOS_Schedule();
	}
#endif
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
		_EV_DoOneEvent();
	}
}
