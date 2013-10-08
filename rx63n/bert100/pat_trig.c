/**
 * Control the pattern trigger
 */
#include <string.h>
#include "pat_trig.h"
#include "types.h"
#include "atomic.h"
#include "iodefine.h"
#include "interpreter.h"
#include "hex.h"
#include "console.h"

#define PATTRIG1_DIROUT BSET(2,PORT5.PDR.BYTE)
#define PATTRIG2_DIROUT BSET(4,PORT5.PDR.BYTE)
#define PATTRIG3_DIROUT BSET(5,PORT5.PDR.BYTE)
#define PATTRIGSHIFTL_DIROUT BSET(2,PORT3.PDR.BYTE)
#define PATTRIGSHIFTR_DIROUT BSET(3,PORT3.PDR.BYTE)
#define PATTRIGPWR_DIROUT BSET(3,PORTJ.PDR.BYTE)

#define PATTRIG1_SET(val) BMOD(2,PORT5.PODR.BYTE,(val))
#define PATTRIG2_SET(val) BMOD(4,PORT5.PODR.BYTE,(val))
#define PATTRIG3_SET(val) BMOD(5,PORT5.PODR.BYTE,(val))
#define PATTRIGPWR_SET(val) BMOD(3,PORTJ.PODR.BYTE,(val))
#define PATTRIGSHIFTL(val)  BMOD(2,PORT3.PODR.BYTE,val)
#define PATTRIGSHIFTR(val)  BMOD(3,PORT3.PODR.BYTE,val)

static const char *patNames[] = {
	"Off",
	"PRBS7",
	"PRBS9",
	"PRBS15",
	"PRBS23",
	"PRBS31",
};

void
PatTrig_SelPat(uint8_t modId,TrigPattern pattern)
{
	PATTRIG1_SET(!!(pattern & 4));
	PATTRIG2_SET(!!(pattern & 2));
	PATTRIG2_SET(!!(pattern & 1));
	if(pattern == 0) {
		PATTRIGPWR_SET(0);
	} else {
		PATTRIGPWR_SET(1);
	}
}

void
PatTrig_Shift(uint8_t modId,bool right)
{
	if(right) {
		PATTRIGSHIFTR(0);
		DelayUs(10);	
		PATTRIGSHIFTR(1);
	} else {
		PATTRIGSHIFTL(0);
		DelayUs(10);	
		PATTRIGSHIFTL(1);
	}
}

static int8_t
cmd_ptrig(Interp * interp, uint8_t argc, char *argv[])
{
	TrigPattern pat;
	if(argc > 1) {
		if(strcmp(argv[1],"left") == 0) {
			PatTrig_Shift(0,false);
			return 0;
		} else if(strcmp(argv[1],"right") == 0) {
			PatTrig_Shift(0,true);
			return 0;
		} else if(!ishexnum(argv[1])){
			return -EC_BADARG;
		}
		pat = astrtoi16(argv[1]);
		if(pat <= 5) {
			PatTrig_SelPat(0,pat);
			Con_Printf("Pattern %s\n",patNames[pat]);
		} else {
			return -EC_ARGRANGE;
		}
		return 0;
	}	
	return -EC_BADNUMARGS;
}

INTERP_CMD(ptrigCmd, "ptrig", cmd_ptrig, "ptrig ?<patternNr>?   # select a pattern");

void
PatTrig_Init(const char *name)
{
	PATTRIG1_DIROUT;
	PATTRIG2_DIROUT;
	PATTRIG3_DIROUT;
	PATTRIGSHIFTL_DIROUT;
	PATTRIGSHIFTR_DIROUT;
	PATTRIGPWR_DIROUT;
	PatTrig_SelPat(0,patTrigOff);
	Interp_RegisterCmd(&ptrigCmd);
}
