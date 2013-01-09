#ifndef _THREADS_H
#define _THREADS_H
#include <stdint.h> 

typedef struct Thread Thread;

struct Thread {
	uint16_t regSP;
	uint8_t *stack;
	uint16_t stacksize;
};

void Threads_Init(void);
#endif
