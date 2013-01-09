#include <stdlib.h>
#include <stdint.h>
#include "threads.h"
#define THREAD_POOL_SIZE 2

static Thread *unusedqHead = NULL;
Thread *g_RunqHead = NULL;
Thread *g_RunqTail = NULL;
static Thread *yieldqHead = NULL;
