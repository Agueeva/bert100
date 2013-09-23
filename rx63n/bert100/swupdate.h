#include "types.h"
#define SWUP_EC_UPTODATE        (1)
#define SWUP_EC_CRC                     (2)
#define SWUP_EC_WRONG_FILE      (3)
#define SWUP_EC_HWERR           (4)
#define SWUP_EC_NOFILE      (5)
uint8_t SWUpdate_Execute(const char *filename);
void SWUpdate_Init(void);
