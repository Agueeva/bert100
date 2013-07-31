/*
 *************************************************************************
 * Management of internal RAM
 *************************************************************************
 */

#include <string.h>
#include "types.h"
#include "iram.h"

#define ALIGNMENT (4)
#define IRAM_MANAGED_SIZE	(16384)
typedef struct IRamManager {
	uint8_t iram[IRAM_MANAGED_SIZE];
	uint32_t firstFreeIdx;
} IRamMan;

static IRamMan gIramMan; /* Must be static or memsetted to zero */

/**
 ******************************************************************************************
 * \fn void * IRam_Calloc(uint32_t size)
 * Allocate zeroed memory. As long there is no "free" I need no memset 0, because
 * this is already done by the static declaration.
 ******************************************************************************************
 */
void *
IRam_Calloc(uint32_t size)
{
	IRamMan *irm = &gIramMan;
	void *dataP;
	if((irm->firstFreeIdx + size) <= IRAM_MANAGED_SIZE) {
		dataP = &irm->iram[irm->firstFreeIdx];
		irm->firstFreeIdx = (irm->firstFreeIdx + size + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);
		return dataP;
	}
	return NULL; 
}

char *
IRam_Strdup(const char *str) {
        char *dup;
        uint16_t len = strlen(str);
        dup = IRam_Calloc(len + 1);
        strcpy(dup,str);
        return dup;
}

