#include <stdlib.h>
#include <stdint.h>
#include "threads.h"
#include "tpos.h"
#define THREAD_POOL_SIZE 2

#if 0
static Thread *unusedqHead = NULL;
Thread *g_RunqHead = NULL;
Thread *g_RunqTail = NULL;
static Thread *yieldqHead = NULL;
#endif
void
Mutex_Lock(Mutex *rs)
{
	uint8_t ipl;
	while(1) {
		if(!rs->owner) {
			rs->owner = Thread_Current();
			return;
		} else {
			Thread_Current()->waitNext = rs->waitHead;
			rs->waitHead = Thread_Current();
		}
	}
}
void
TPos_Init(void)
{

}
