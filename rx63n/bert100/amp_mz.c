/**
  ************************************************************
  * Initialization of the MZ modulator amplifier. 
  ************************************************************
  */
#include <string.h>
#include "types.h"
#include "console.h"
#include "events.h"
#include "interpreter.h"
#include "hex.h"
#include "ad537x.h"
#include "atomic.h"
#include "iodefine.h"
#include "config.h"
#include "timer.h"
#include "pvar.h"
#include "database.h"
#include "version.h"

#define NR_CHANNELS     (32)

typedef struct DacAlias {
	const char *name;
	uint8_t flags;
} DacAlias;

#define FLG_READABLE 	(1)
#define FLG_WRITABLE 	(2)


static const DacAlias dac0AliasesMZ[NR_CHANNELS] = 
{
	{ NULL,FLG_READABLE | FLG_WRITABLE },
	{ NULL,FLG_READABLE | FLG_WRITABLE },
	{ NULL,FLG_READABLE | FLG_WRITABLE },
	{ NULL,FLG_READABLE | FLG_WRITABLE },
	{"amp0.vg1", FLG_READABLE | FLG_WRITABLE },
	{"amp1.vg1", FLG_READABLE | FLG_WRITABLE },
	{"amp2.vg1", FLG_READABLE | FLG_WRITABLE },
	{"amp3.vg1", FLG_READABLE | FLG_WRITABLE },
	{"amp0.vg2", FLG_READABLE | FLG_WRITABLE },
	{"amp1.vg2", FLG_READABLE | FLG_WRITABLE },
	{"amp2.vg2", FLG_READABLE | FLG_WRITABLE },
	{"amp3.vg2", FLG_READABLE | FLG_WRITABLE },
	{"amp0.vg3", FLG_READABLE | FLG_WRITABLE },
	{"amp1.vg3", FLG_READABLE | FLG_WRITABLE },
	{"amp2.vg3", FLG_READABLE | FLG_WRITABLE },
	{"amp3.vg3", FLG_READABLE | FLG_WRITABLE },
	{"amp0.vd1", FLG_READABLE | FLG_WRITABLE },
	{"amp1.vd1", FLG_READABLE | FLG_WRITABLE },
	{"amp2.vd1", FLG_READABLE | FLG_WRITABLE },
	{"amp3.vd1", FLG_READABLE | FLG_WRITABLE },
	{"amp0.vd2", FLG_READABLE | FLG_WRITABLE },
	{"amp1.vd2", FLG_READABLE | FLG_WRITABLE },
	{"amp2.vd2", FLG_READABLE | FLG_WRITABLE },
	{"amp3.vd2", FLG_READABLE | FLG_WRITABLE },
	{NULL, 0},
	{NULL, 0},
	{NULL, 0},
	{NULL, 0},
	{NULL, 0},
	{NULL, 0},
	{NULL, 0},
	{NULL, 0},
};	

/**
 **************************************************************************************
 * Process variable interface to the DAC
 **************************************************************************************
 */
static bool 
PVDac_Set (void *cbData, uint32_t chNr, const char *strP)
{
	float val;	
	val = astrtof32(strP);
	return DAC_Set(chNr,val);
}

static bool 
PVDac_Get (void *cbData, uint32_t chNr, char *bufP,uint16_t maxlen)
{
	float dacval;
        uint8_t cnt;
	bool retval;
	retval = DAC_Get(chNr,&dacval);
        cnt = f32toa(dacval,bufP,maxlen);
        bufP[cnt] = 0;
        return true;
}

/**
  ***********************************************************
  * \fn void AmpMZ_Init(const char *name);
  * Initialize the DAC device driver module.
  ***********************************************************
  */
void
AmpMZ_Init(const char *name)
{
	int ch;
	//uint8_t variant = Variant_Get();
 	for(ch = 0; ch < NR_CHANNELS; ch++) {
		const DacAlias *alias;
		alias = &dac0AliasesMZ[ch];
		if(alias->name) {
			if(alias->flags == (FLG_READABLE | FLG_WRITABLE)) {
				PVar_New(PVDac_Get,PVDac_Set,NULL,ch,alias->name);
			} else if(alias->flags == FLG_READABLE) {
				PVar_New(PVDac_Get,NULL,NULL,ch,alias->name);
			}
		}
	}
}
