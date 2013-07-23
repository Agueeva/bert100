/*
 *************************************************************
 * (C) 2009 2010 Jochen Karrer
 *//**
 * \file events.c
 * System independent event queue implementation. 
 *************************************************************
 */

#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include "irqflags.h"
#include "events.h"
#include "tpos.h"

static Event *queueHead = NULL;
static Event *queueTail = NULL;


/**
 ****************************************************************
 * \fn static inline void event_enqueue(void)
 * Append an event to the queue of active events.
 ****************************************************************
 */
static inline void
Event_Enqueue(Event *ev) {
	ev->next = NULL;
	ev->evState = EV_StatePending;
	ev->prev = queueTail;
	if(!queueHead) {
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
EV_Trigger(Event *ev) {
	uint8_t flags;
	flags = save_ipl_set(IPL_EVENTS);
	if (ev->evState == EV_StateIdle) {
		Event_Enqueue(ev);
	} else if (ev->evState == EV_StateExecuting) {
		ev->evState = EV_StateExepending;
	} 
	restore_ipl(flags);
}

/* Never call this directly (with an idle event) */
void 
_EventCancel(Event *ev) 
{
	if(ev == queueHead) {
		queueHead = ev->next;
	} else {
		ev->prev->next = ev->next;
	}
	if(ev == queueTail) {
		queueTail = ev->prev;
	} else {
		ev->next->prev = ev->prev;
	}
	ev->evState = EV_StateIdle;
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
	uint8_t ipl;
	ipl = save_ipl_set(IPL_EVENTS);
 	ev = queueHead;
	if(ev) {
		ev->evState = EV_StateExecuting;
		queueHead = queueHead->next;
		if(!queueHead) {
			queueTail = NULL;
		}
		restore_ipl(ipl);
		if(ev->evProc) {
			ev->evProc(ev->eventData);
		}
		ipl = save_ipl_set(IPL_EVENTS);
		if(ev->evState == EV_StateExepending) {
			Event_Enqueue(ev);
		} else {
			ev->evState  = EV_StateIdle;
		}
	}	
	restore_ipl(ipl);
}
 
void
EV_Yield(void)
{
	if(queueHead) {
		TPOS_Yield();
	}
}
/**
 *************************************************************
 * \fn void EV_Loop(void)
 * Event Loop: The main loop of the application
 *************************************************************
 */
void
EV_Loop(void) {
	while(1) {
		_EV_DoOneEvent();
	}		 
}
