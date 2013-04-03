#ifndef _EVENTS_H
#define _EVENTS_H
#include <stdio.h>
#include "atomic.h"
#include "types.h"
typedef void EventProc(void *EventData);
/**
 **************************************************************
 * \def EVENTS_IPL
 * Interrupt privilege level during Event queue manipulation.
 * A Event queue manipulation procedure may not be called
 * from an interrupt with a higher IPL.
 **************************************************************
 */
#define EVENTS_IPL	4

enum {
	EV_StateIdle,

	/************************************************************************* 
	 * An Event is marked as executing during its event handler is running. 
	 *************************************************************************
	 */
	EV_StateExecuting,
	/*
	 **************************************************************************
	 * An Event is in state pending when it is enqueued in the Eventqueue
	 * waiting for its execution.
	 **************************************************************************
	 */
	EV_StatePending,
	/**********************************************************************
	 * When an event is Triggered while the Event handler is 
	 * executed (Event in state Executing) it goes to state
	 * Exepending because it is not allowed to be in the Event-Queue
	 * to avoid recursive execution.
	 ***********************************************************************
	 */
	EV_StateExepending,
};

typedef struct Event {
	EventProc *evProc;
	void *eventData;
	uint16_t evState;
	struct Event *next;
	struct Event *prev;
} Event;

void EV_Loop(void);
void EV_Trigger(Event * event);
void EV_Yield(void);

static inline void
EV_Init(Event * ev, EventProc * proc, void *eventData)
{
	ev->evProc = proc;
	ev->eventData = eventData;
	ev->evState = EV_StateIdle;
	ev->next = ev->prev = NULL;
}

static inline int
Event_Pending(Event * ev)
{
	return (ev->evState == EV_StatePending) || (ev->evState == EV_StateExepending);
}

/* Never call _EventCancel() directly, use Event_Cancel() */
void _EventCancel(Event * ev);

static inline void
EV_Cancel(Event * ev)
{
	Flags_t flags;
	SAVE_FLAGS_SET_IPL(flags, EVENTS_IPL);
	if (ev->evState == EV_StatePending) {
		_EventCancel(ev);
	} else if (ev->evState == EV_StateExepending) {
		ev->evState = EV_StateExecuting;
	}
	RESTORE_FLAGS(flags)
}

#define EVENT_DECLARE(name,proc,data) Event name = {proc,data,EV_StateIdle,NULL,NULL};
#endif
