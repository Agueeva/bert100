/**
 ***********************************************************************************
 * Thread pool Operating system Header
 ***********************************************************************************
 */
#ifndef _TPOS_H
#define _TPOS_H
#include "threads.h"
typedef struct TPOSRSema {
	Thread *waitHead;
	Thread *owner;
} Mutex;

typedef struct TPOSCSema {
	Thread *waitHead;
	uint16_t cnt;
} CSema;

void TPOS_Init(void);
extern Thread *g_RunqHead;

void TPOS_Yield(void);

void CSema_Init(CSema * csema);
void CSema_Up(CSema * csema);
void CSema_Down(CSema * csema);

void Mutex_Init(Mutex * rsema);
void Mutex_Lock(Mutex * rsema);
INLINE bool
Mutex_Locked(Mutex * rsema)
{
	return ! !rsema->owner;
};

void Mutex_Unlock(Mutex * rsema);

/*
 *******************************************************
 * A mutex has to be locked when it is deleted so
 * that nobody else can have it.
 *******************************************************
 */
INLINE void
Mutex_Delete(Mutex * rsema)
{
	Mutex_Lock(rsema);
}

#endif
