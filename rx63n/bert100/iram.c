/*
 *************************************************************************
 * Management of internal RAM
 *************************************************************************
 */

#include "types.h"
#include "iram.h"

#define ALIGNMENT (4)
#define IRAM_MANAGED_SIZE	(16384)
typedef struct IRamManager {
	uint8_t iram[IRAM_MANAGED_SIZE];
	uint32_t firstFreeIdx;
} IRamMan;

static IRamMan gIramMan; 

void *
IRam_Alloc(uint32_t size)
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
