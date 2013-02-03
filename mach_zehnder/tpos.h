#include <stdint.h>
#include "threads.h"

typedef struct TPOSRSema {
        Thread *waitHead;
        Thread *owner;
} Mutex;

