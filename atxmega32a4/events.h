#ifndef _EVENTS_H
#define _EVENTS_H
#include <stdio.h>
#include "irqflags.h"
typedef void EventProc(void *EventData);
#define IPL_EVENTS      (IPL_LOW)
/**
 **************************************************************
 * \def EVENTS_IPL
 * Interrupt privilege level during Event queue manipulation.
 * A Event queue manipulation procedure may not be called
 * from an interrupt with a higher IPL.
 **************************************************************
 */

enum  {
	EV_StateIdle,
	EV_StateExecuting,
	EV_StatePending,
	EV_StateExepending,
};

typedef struct Event {
	EventProc *evProc;
	void *eventData;
	uint8_t evState;
	struct Event *next;
	struct Event *prev;
} Event;

void EV_Loop(void);
void EV_Trigger(Event *event);
void EV_Yield(void);

static inline void
EV_Init(Event *ev,EventProc *proc,void *eventData)
{
	ev->evProc = proc;
	ev->eventData = eventData;
	ev->evState = EV_StateIdle;
	ev->next = NULL;
}

/* Never call _EventCancel directly */
void _EventCancel(Event *ev);

static inline uint8_t 
Event_Pending(Event *ev) 
{
	return (ev->evState == EV_StatePending) || (ev->evState == EV_StateExepending);

}

static inline void 
Event_Cancel(Event *ev) 
{
	uint8_t ipl = save_ipl_set(IPL_EVENTS);
	if(ev->evState == EV_StatePending) {
		_EventCancel(ev);
	} else if(ev->evState == EV_StateExepending) {
		ev->evState = EV_StateExecuting;
	}
	restore_ipl(ipl);
}

/**
 ****************************************************
 * \def EVENT_DECLARE(name,procedure,clientData)
 ****************************************************
 */
#define EVENT_DECLARE(name,ev_proc,cData) \
        Event name = { \
                .eventData = (cData), \
                .evProc = (ev_proc), \
                .evState = EV_StateIdle \
        }

#endif 
