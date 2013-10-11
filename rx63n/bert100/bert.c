/**
  *******************************************************
  * Bert Layer, Layer above Synthesizer and CDR
  *******************************************************
  */

#include "xo.h"
#include "timer.h"
#include "cdr.h"
#include "interpreter.h"
#include "console.h"
#include "hex.h"
#include "pvar.h"
#include "shiftreg.h"
#include <math.h>

#define NR_CHANNELS	(4)

/* Fifo for moving window BERatio and Rate */
#define BEFIFO_SIZE	(4)
#define BEFIFO_WP(fifo) ((fifo)->fifoWp % BEFIFO_SIZE)

/**
 * Some registers are rawly forwarded to one or two CDR's
 */
typedef struct CdrForward {
	char *name;
	uint32_t cdrRegId;
	uint8_t bfCdrSelectW;	/* Bitfield forwarding to one or both CDR's */ 
	uint8_t bfCdrSelectR;	/* Bitfield for selecting the CDR to read from */ 
} CdrForward;

typedef struct BeFifo {
	uint64_t errCnt[BEFIFO_SIZE];
	uint32_t tStamp[BEFIFO_SIZE];
	uint8_t channel;
	uint8_t fifoWp;
	Timer getErrCntTimer;
	TimeMs_t berMeasTime;
} BeFifo;

typedef struct Bert {
	BeFifo beFifo[NR_CHANNELS];
	Timer updateLedsTimer;
	uint32_t dataRate; 
	uint8_t pvLatchedLol[NR_CHANNELS];
} Bert;

static Bert gBert;

static const uint8_t EqStateToLed[16] = {
	0x0,
	0x0,
	0x8,
	0x8,
	0x8,
	0xc,
	0xc,
	0xc,
	0xc,
	0xe,
	0xe,
	0xe,
	0xe,
	0xe,
	0xf,
	0xf,
};




/*
 *********************************************************************
 * Define the variables which are forwarded to the  
 *********************************************************************
 */
static const CdrForward gForwardRegs[] = 
{
	{
		.name = "L0.prbsLock",
		.cdrRegId = CDR_L0_PRBS_LOCK,
		.bfCdrSelectW = 0, 
		.bfCdrSelectR = (1 << CDR_ID_RX),
		
	},
	{
		.name = "L1.prbsLock",
		.cdrRegId = CDR_L1_PRBS_LOCK,
		.bfCdrSelectW = 0,
		.bfCdrSelectR = (1 << CDR_ID_RX),
		
	},
	{
		.name = "L2.prbsLock",
		.cdrRegId = CDR_L2_PRBS_LOCK,
		.bfCdrSelectW = 0, 
		.bfCdrSelectR = (1 << CDR_ID_RX),
		
	},
	{
		.name = "L3.prbsLock",
		.cdrRegId = CDR_L3_PRBS_LOCK,
		.bfCdrSelectW = 0, 
		.bfCdrSelectR = (1 << CDR_ID_RX),
		
	},
#if 0
#define CDR_L0_PRBS_AUTOVR                  (0x001000ee)
#define CDR_L0_LOOPBACK_EN                  (0x001000dd) // makes no sense here
#endif
	{
		.name = "L0.PatVerEn",
		.cdrRegId = CDR_L0_PAT_VER_EN, 
		.bfCdrSelectW = (1 << CDR_ID_RX),
		.bfCdrSelectR = (1 << CDR_ID_RX),
	},
	{
		.name = "L1.PatVerEn",
		.cdrRegId = CDR_L1_PAT_VER_EN, 
		.bfCdrSelectW = (1 << CDR_ID_RX),
		.bfCdrSelectR = (1 << CDR_ID_RX),
	},
	{
		.name = "L2.PatVerEn",
		.cdrRegId = CDR_L2_PAT_VER_EN, 
		.bfCdrSelectW = (1 << CDR_ID_RX),
		.bfCdrSelectR = (1 << CDR_ID_RX),
	},
	{
		.name = "L3.PatVerEn",
		.cdrRegId = CDR_L3_PAT_VER_EN, 
		.bfCdrSelectW = (1 << CDR_ID_RX),
		.bfCdrSelectR = (1 << CDR_ID_RX),
	},
	{
		.name = "L0.prbsVerInv",
		.cdrRegId = CDR_L0_PRBS_VER_INV,
		.bfCdrSelectW = (1 << CDR_ID_RX),
		.bfCdrSelectR = (1 << CDR_ID_RX),
	},
	{
		.name = "L1.prbsVerInv",
		.cdrRegId = CDR_L1_PRBS_VER_INV,
		.bfCdrSelectW = (1 << CDR_ID_RX),
		.bfCdrSelectR = (1 << CDR_ID_RX),
	},
	{
		.name = "L2.prbsVerInv",
		.cdrRegId = CDR_L2_PRBS_VER_INV,
		.bfCdrSelectW = (1 << CDR_ID_RX),
		.bfCdrSelectR = (1 << CDR_ID_RX),
	},
	{
		.name = "L3.prbsVerInv",
		.cdrRegId = CDR_L3_PRBS_VER_INV,
		.bfCdrSelectW = (1 << CDR_ID_RX),
		.bfCdrSelectR = (1 << CDR_ID_RX),
	},
	{
		.name = "L0.patVerSel",	
		.cdrRegId = CDR_L0_PAT_VER_SEL,
		.bfCdrSelectW = (1 << CDR_ID_RX),
		.bfCdrSelectR = (1 << CDR_ID_RX),
	}, 
	{
		.name = "L1.patVerSel",	
		.cdrRegId = CDR_L1_PAT_VER_SEL,
		.bfCdrSelectW = (1 << CDR_ID_RX),
		.bfCdrSelectR = (1 << CDR_ID_RX),
	}, 
	{
		.name = "L2.patVerSel",	
		.cdrRegId = CDR_L2_PAT_VER_SEL,
		.bfCdrSelectW = (1 << CDR_ID_RX),
		.bfCdrSelectR = (1 << CDR_ID_RX),
	}, 
	{
		.name = "L3.patVerSel",	
		.cdrRegId = CDR_L3_PAT_VER_SEL,
		.bfCdrSelectW = (1 << CDR_ID_RX),
		.bfCdrSelectR = (1 << CDR_ID_RX),
	}, 
	{		
		.name = "L0.prbsPatGenSel",	
		.cdrRegId = CDR_L0_PAT_GEN_SEL,
		.bfCdrSelectW = (1 << CDR_ID_TX),
		.bfCdrSelectR = (1 << CDR_ID_TX),
	},
	{		
		.name = "L1.prbsPatGenSel",	
		.cdrRegId = CDR_L1_PAT_GEN_SEL,
		.bfCdrSelectW = (1 << CDR_ID_TX),
		.bfCdrSelectR = (1 << CDR_ID_TX),
	},
	{		
		.name = "L2.prbsPatGenSel",	
		.cdrRegId = CDR_L2_PAT_GEN_SEL,
		.bfCdrSelectW = (1 << CDR_ID_TX),
		.bfCdrSelectR = (1 << CDR_ID_TX),
	},
	{		
		.name = "L3.prbsPatGenSel",	
		.cdrRegId = CDR_L3_PAT_GEN_SEL,
		.bfCdrSelectW = (1 << CDR_ID_TX),
		.bfCdrSelectR = (1 << CDR_ID_TX),
	},
#if 0
#define CDR_L0_ODB_EN                       (0x00100077)
#define CDR_L0_TX_DISABLE                   (0x00100066)
#define CDR_L0_ERROR_INSERT                 (0x00100055)
#define CDR_L0_PAT_GEN_EN                   (0x00100044)
#endif
	{		
		.name = "L0.prbsGenInv",	
		.cdrRegId = CDR_L0_PRBS_GEN_INV, 
		.bfCdrSelectW = (1 << CDR_ID_TX),
		.bfCdrSelectR = (1 << CDR_ID_TX),
	},
	{		
		.name = "L1.prbsGenInv",	
		.cdrRegId = CDR_L1_PRBS_GEN_INV, 
		.bfCdrSelectW = (1 << CDR_ID_TX),
		.bfCdrSelectR = (1 << CDR_ID_TX),
	},
	{		
		.name = "L2.prbsGenInv",	
		.cdrRegId = CDR_L2_PRBS_GEN_INV, 
		.bfCdrSelectW = (1 << CDR_ID_TX),
		.bfCdrSelectR = (1 << CDR_ID_TX),
	},
	{		
		.name = "L3.prbsGenInv",	
		.cdrRegId = CDR_L3_PRBS_GEN_INV, 
		.bfCdrSelectW = (1 << CDR_ID_TX),
		.bfCdrSelectR = (1 << CDR_ID_TX),
	},
	{
		.name = "L0.patGenSel",
		.cdrRegId = CDR_L0_PAT_GEN_SEL,
		.bfCdrSelectW = (1 << CDR_ID_TX),
		.bfCdrSelectR = (1 << CDR_ID_TX),
	},
	{
		.name = "L1.patGenSel",
		.cdrRegId = CDR_L1_PAT_GEN_SEL,
		.bfCdrSelectW = (1 << CDR_ID_TX),
		.bfCdrSelectR = (1 << CDR_ID_TX),
	},
	{
		.name = "L2.patGenSel",
		.cdrRegId = CDR_L2_PAT_GEN_SEL,
		.bfCdrSelectW = (1 << CDR_ID_TX),
		.bfCdrSelectR = (1 << CDR_ID_TX),
	},
	{
		.name = "L3.patGenSel",
		.cdrRegId = CDR_L3_PAT_GEN_SEL,
		.bfCdrSelectW = (1 << CDR_ID_TX),
		.bfCdrSelectR = (1 << CDR_ID_TX),
	},
#if 0
#define CDR_L0_NO_PRBS_LCK                  (0x00210000)
#define CDR_L1_NO_PRBS_LCK                  (0x00210011)
#define CDR_L2_NO_PRBS_LCK                  (0x00210022)
#define CDR_L3_NO_PRBS_LCK                  (0x00210033)
#define CDR_L0_NO_PROT_LCK                  (0x00210044)
#define CDR_L1_NO_PROT_LCK                  (0x00210055)
#define CDR_L2_NO_PROT_LCK                  (0x00210066)
#define CDR_L3_NO_PROT_LCK                  (0x00210066)
#endif
	{
		.name = "L0.LolStat",
		.cdrRegId = CDR_L0_LOL_STAT,
		.bfCdrSelectW = (1 << CDR_ID_RX),
		.bfCdrSelectR = (1 << CDR_ID_RX),
	},
	{
		.name = "L1.LolStat",
		.cdrRegId = CDR_L1_LOL_STAT,
		.bfCdrSelectW = (1 << CDR_ID_RX),
		.bfCdrSelectR = (1 << CDR_ID_RX),
	},
	{
		.name = "L2.LolStat",
		.cdrRegId = CDR_L2_LOL_STAT,
		.bfCdrSelectW = (1 << CDR_ID_RX),
		.bfCdrSelectR = (1 << CDR_ID_RX),
	},
	{
		.name = "L3.LolStat",
		.cdrRegId = CDR_L3_LOL_STAT,
		.bfCdrSelectW = (1 << CDR_ID_RX),
		.bfCdrSelectR = (1 << CDR_ID_RX),
	},
#if 0
#define CDR_L0_FIFO_URUN                    (0x00250000)
#define CDR_L1_FIFO_URUN                    (0x00250011)
#define CDR_L2_FIFO_URUN                    (0x00250022)
#define CDR_L3_FIFO_URUN                    (0x00250033)
#define CDR_L0_FIFO_ORUN                    (0x00250044)
#define CDR_L1_FIFO_ORUN                    (0x00250055)
#define CDR_L2_FIFO_ORUN                    (0x00250066)
#define CDR_L3_FIFO_ORUN                    (0x00250077)
#define CDR_L0_FIFO_ERROR                   (0x00250088)
#define CDR_L1_FIFO_ERROR                   (0x00250099)
#define CDR_L2_FIFO_ERROR                   (0x002500aa)
#define CDR_L3_FIFO_ERROR                   (0x002500bb)
#define CDR_RESET_ON_LOL                    (0x002500dd)
#define CDR_RESET_ON_ERR                    (0x002500ee)
#define CDR_FIFO_RESET                      (0x002500ff)
#define CDR_TXPLL_CAL_BUS_FORCE_FINE        (0x00270002)
#define CDR_TXPLL_CAL_BUS_FORCE_COARSE      (0x00270037)
#define CDR_TXPLL_FORCE_CAL_BUS             (0x00270088)
#define CDR_TXPLL_CHARGE_PUMP_CURR          (0x0027009b)
#define CDR_TXPLL_CAL_STEP                  (0x002700de)
#define CDR_TXPLL_RECALIB                   (0x002700ff)
#define CDR_TXPLL_CAL_READ_FINE             (0x00280002)
#define CDR_TXPLL_CAL_READ_COARSE           (0x00280037)
#define CDR_TXPLL_LOCK                      (0x00280088)
#endif
	{
		.name = "txPllLock",
		.cdrRegId = CDR_TXPLL_LOCK, 
		.bfCdrSelectW = (1 << CDR_ID_TX),
		.bfCdrSelectR = (1 << CDR_ID_TX),
	},
#if 0
#define CDR_TXPLL_LCO_AMPL                  (0x002800bd)
#define CDR_TXPLL_VCTLREF                   (0x002800ef)
#define CDR_RXPLL_CAL_BUS_FORCE_FINE        (0x00290002)
#define CDR_RXPLL_CAL_BUS_FORCE_COARSE      (0x00290037)
#define CDR_RXPLL_FORCE_CAL_BUS             (0x00290088)
#define CDR_RXPLL_CHARGE_PUMP_CURR          (0x0029009b)
#define CDR_RXPLL_CAL_STEP                  (0x002900de)
#define CDR_RXPLL_RECALIB                   (0x002900ff)
#define CDR_RXPLL_CAL_READ_FINE             (0x002a0002)
#define CDR_RXPLL_CAL_READ_COARSE           (0x002a0037)
#define CDR_RXPLL_LOCK                      (0x002a0088)
#endif
	{
		.name = "rxPllLock",
		.cdrRegId = CDR_RXPLL_LOCK, 
		.bfCdrSelectW = (1 << CDR_ID_RX),
		.bfCdrSelectR = (1 << CDR_ID_RX),
	},
#if 0
#define CDR_RXPLL_LCO_AMPL                  (0x002a00bd)
#define CDR_RXPLL_VCTLREF                   (0x002a00ef)
#define CDR_L0_FIFO_ERROR                   (0x00250088)
#define CDR_L1_FIFO_ERROR                   (0x00250099)
#define CDR_L2_FIFO_ERROR                   (0x002500aa)
#define CDR_L3_FIFO_ERROR                   (0x002500bb)
#endif
	{
		.name = "L0.swapTxPN",
		.cdrRegId = CDR_SWAP_TXP_N(0), 
		.bfCdrSelectW = (1 << CDR_ID_TX),
		.bfCdrSelectR = (1 << CDR_ID_TX),
	},
	{
		.name = "L1.swapTxPN",
		.cdrRegId = CDR_SWAP_TXP_N(1), 
		.bfCdrSelectW = (1 << CDR_ID_TX),
		.bfCdrSelectR = (1 << CDR_ID_TX),
	},
	{
		.name = "L2.swapTxPN",
		.cdrRegId = CDR_SWAP_TXP_N(2), 
		.bfCdrSelectW = (1 << CDR_ID_TX),
		.bfCdrSelectR = (1 << CDR_ID_TX),
	},
	{
		.name = "L3.swapTxPN",
		.cdrRegId = CDR_SWAP_TXP_N(3), 
		.bfCdrSelectW = (1 << CDR_ID_TX),
		.bfCdrSelectR = (1 << CDR_ID_TX),
	},
#if 0
#define CDR_LOOPBACKOE(lane)                (0x01000011 + ((lane) << 24))
#define CDR_TXA_EQPST(lane)                 (0x0101008a + ((lane) << 24))
#define CDR_TXA_EQPRE(lane)                 (0x01010001 + ((lane) << 24))
#define CDR_TXA_SWING(lane)                 (0x01020002 + ((lane) << 24))
#endif
	{
		.name = "L0.EqState",
		.cdrRegId = CDR_EQ_STATE(0), 
		.bfCdrSelectW = (1 << CDR_ID_RX),
		.bfCdrSelectR = (1 << CDR_ID_RX),
	},
	{

		.name = "L1.EqState",
		.cdrRegId = CDR_EQ_STATE(1), 
		.bfCdrSelectW = (1 << CDR_ID_RX),
		.bfCdrSelectR = (1 << CDR_ID_RX),
	},
	{
		.name = "L2.EqState",
		.cdrRegId = CDR_EQ_STATE(2), 
		.bfCdrSelectW = (1 << CDR_ID_RX),
		.bfCdrSelectR = (1 << CDR_ID_RX),
	},
	{
		.name = "L3.EqState",
		.cdrRegId = CDR_EQ_STATE(3), 
		.bfCdrSelectW = (1 << CDR_ID_RX),
		.bfCdrSelectR = (1 << CDR_ID_RX),
	},
#if 0
#define CDR_CDR2_TRIP(lane)                 (0x01a7000d + ((lane) << 24))
#define CDR_PI_POS1_PISEL(lane)             (0x01a800cd + ((lane) << 24))
#define CDR_PI_POS1_PIQUADR(lane)           (0x01a80089 + ((lane) << 24))
#define CDR_PI_POS1_PICODE(lane)            (0x01a80007 + ((lane) << 24))
#define CDR_SEC_ORDER_STATE(lane)           (0x01aa0004 + ((lane) << 24))
#endif

};

/*
 ****************************************************
 * Update the leds
 ****************************************************
 */
static void
UpdateLedsTimerProc(void *eventData) 
{
	unsigned int ch;
	uint16_t ledOut = 0;
	Bert *bert = eventData;
	Timer_Start(&bert->updateLedsTimer,250);
	for(ch = 0; ch < NR_CHANNELS; ch++) {
		uint16_t idx = CDR_ReadEqObserve(0,ch) & 15;
		ledOut = (ledOut >> 4) | (uint32_t)EqStateToLed[idx] << 12;
	}
	ShiftReg_Out(ledOut);
}

static void 
GetErrCntTimerProc(void *eventData)
{
	BeFifo *fifo = eventData;
	unsigned int wp;
	Timer_Start(&fifo->getErrCntTimer,fifo->berMeasTime >> 2);
	wp = BEFIFO_WP(fifo);
	fifo->errCnt[wp] = CDR_GetErrCnt(0, fifo->channel);
	fifo->tStamp[wp] = TimeMs_Get();
	fifo->fifoWp++;
}

/**
 **********************************************************************
 * \fn static float Bert_GetBerate(Bert *bert,unsigned int lane)
 * Get the Current Bit error rate.
 **********************************************************************
 */
static float 
Bert_GetBerate(Bert *bert,unsigned int lane)
{
	BeFifo *fifo;
	unsigned int rp;
	float rate;
	uint64_t errCntOld,errCntNew;
	uint32_t tStampNew,tStampOld;
	uint32_t tDiff;
	if(lane >= NR_CHANNELS) {
		return 0;
	}
	fifo = &bert->beFifo[lane];	
	rp = BEFIFO_WP(fifo);
	errCntOld = fifo->errCnt[rp];
	tStampOld = fifo->tStamp[rp];
	rp = (rp + BEFIFO_SIZE - 1) % BEFIFO_SIZE;
	errCntNew = fifo->errCnt[rp];
	tStampNew = fifo->tStamp[rp];
	tDiff = tStampNew - tStampOld;
	if(tDiff) {
		rate = 1000. * (errCntNew - errCntOld) / tDiff;
	} else {
		rate = 1e11;
	}
	return rate;
}

/**
 ********************************************************************************
 * \fn static int8_t cmd_ber(Interp * interp, uint8_t argc, char *argv[])
 * Shell interface for getting BERatio und BERate.
 ********************************************************************************
 */
static int8_t
cmd_ber(Interp * interp, uint8_t argc, char *argv[])
{
	Bert *bert = &gBert;
	unsigned int ch;
	uint64_t freq = 40 * (uint64_t)Synth_GetFreq(0);
	if(argc > 1) {
		for(ch = 0; ch < NR_CHANNELS; ch++) {
			BeFifo *fifo = &bert->beFifo[ch];
			fifo->berMeasTime = astrtoi32(argv[1]);
			if(fifo->berMeasTime < 200) {
				fifo->berMeasTime = 200;
			}
		}
		return 0;
	}
	for(ch = 0; ch < NR_CHANNELS; ch++) 
	{
		float rate,ratio;
		rate = Bert_GetBerate(bert,ch);
		ratio = rate / freq; 
		Con_Printf("Lane %u: rate %f/s, ratio %e\n",ch,rate,ratio);
	}
	return 0;
}

INTERP_CMD(berCmd, "ber", cmd_ber, "ber # Get all Bit error ratios");

static bool
PVBeratio_Get (void *cbData, uint32_t adId, char *bufP,uint16_t maxlen)
{
	Bert *bert = cbData;
	//BeFifo *fifo = &bert->beFifo[adId];	
	float rate,ratio;
	uint64_t freq = 40 * (uint64_t)Synth_GetFreq(0);
	rate = Bert_GetBerate(bert,adId);
	ratio = rate / freq; 
	bufP[f32toExp(ratio, bufP,  maxlen)] = 0;
	return true;
}

static bool
PVBerate_Get (void *cbData, uint32_t adId, char *bufP,uint16_t maxlen)
{
	Bert *bert = cbData;
	unsigned int lane = adId;
	float rate;
	rate = Bert_GetBerate(bert,lane);
	bufP[f32toa(rate, bufP,  maxlen)] = 0;
	return true;
}

static bool
PVBerMs_Get(void *cbData, uint32_t adId, char *bufP,uint16_t maxlen)
{
	Bert *bert = cbData;
	BeFifo *fifo = &bert->beFifo[0]; /* Currently all 4 fifos have the same meassurement time */
	bufP[uitoa32(fifo->berMeasTime,bufP)] = 0;
	return true;
}

static bool
PVBerMs_Set(void *cbData, uint32_t adId, const char *strP)
{
	TimeMs_t ms = astrtoi32(strP);
	unsigned int ch;
	Bert *bert = cbData;
	for(ch = 0; ch < NR_CHANNELS; ch++) {
		BeFifo *fifo = &bert->beFifo[ch];
		/* Don't let the user wait to long for the change */
		if(fifo->berMeasTime == ms) {
			continue;
		}
		if(fifo->berMeasTime > 2000) {
			fifo->berMeasTime = ms;
			Timer_Mod(&fifo->getErrCntTimer,fifo->berMeasTime >> 2);
		} else {
			fifo->berMeasTime = ms;
		}
	}
	return true;
}

/*
 ****************************************************************************
 * Forwarded variables.
 ****************************************************************************
 */
static bool
PVLatchedLol_Get (void *cbData, uint32_t adId, char *bufP,uint16_t maxlen)
{
        Bert *bert = cbData;
	uint8_t lane = adId;
	uint32_t regId = 0;
	switch(lane) {
		case 0:
			regId =	CDR_L0_LATCHED_LOL;
			break;
		case 1:
			regId = CDR_L1_LATCHED_LOL; 
			break;
		case 2:
			regId = CDR_L2_LATCHED_LOL;
			break;
		case 3:
			regId = CDR_L3_LATCHED_LOL;
			break;
	}
	bert->pvLatchedLol[lane] |= CDR_Read(CDR_ID_RX,regId);
        bufP[uitoa16(bert->pvLatchedLol[lane],bufP)] = 0;
        return true;
}

static bool
PVLatchedLol_Set(void *cbData, uint32_t adId, const char *strP)
{
        Bert *bert = cbData;
	uint8_t lane = adId;
	uint32_t regId = 0;
	uint32_t value = astrtoi16(strP);
	switch(lane) {
		case 0:
			regId =	CDR_L0_LATCHED_LOL;
			break;
		case 1:
			regId = CDR_L1_LATCHED_LOL; 
			break;
		case 2:
			regId = CDR_L2_LATCHED_LOL;
			break;
		case 3:
			regId = CDR_L3_LATCHED_LOL;
			break;
	}
	CDR_Read(CDR_ID_RX,regId); // throw away the result, Clear on read
	bert->pvLatchedLol[lane] = !!value; 
	return true;
}

static bool
PVForward_Get (void *cbData, uint32_t adId, char *bufP,uint16_t maxlen)
{
        const CdrForward *fwd;
        //Bert *bert = cbData;
        uint16_t value;
        if(adId >= array_size(gForwardRegs)) {
                Con_Printf("Bert %s Unexpected ID %lu\n",__func__,adId);
                return false;
        }
        fwd = &gForwardRegs[adId];
	if(fwd->bfCdrSelectR & (1 << CDR_ID_RX)) {	
        	value = CDR_Read(CDR_ID_RX,fwd->cdrRegId);
	} else if(fwd->bfCdrSelectR & (1 << CDR_ID_TX)) {	
        	value = CDR_Read(CDR_ID_TX,fwd->cdrRegId);
	} else {
		bufP[0] = 0;
		return false;
	}
        bufP[uitoa16(value,bufP)] = 0;
        return true;
}

static bool
PVForward_Set(void *cbData, uint32_t adId, const char *strP)
{
        const CdrForward *fwd;
        uint16_t value;
        if(adId >= array_size(gForwardRegs)) {
                Con_Printf("Bert %s Unexpected ID %lu\n",__func__,adId);
                return false;
        }
        value = astrtoi16(strP);
        fwd = &gForwardRegs[adId];
	if(fwd->bfCdrSelectW & (1 << CDR_ID_RX)) {	
        	CDR_Write(CDR_ID_RX,fwd->cdrRegId,value);
	} 
	if(fwd->bfCdrSelectR & (1 << CDR_ID_TX)) {	
        	CDR_Write(CDR_ID_TX,fwd->cdrRegId,value);
	} 
        return true;
}


void
Bert_Init(void) 
{
	Bert *bert = &gBert;
	BeFifo *fifo;
	const char *name = "bert0";
	unsigned int ch;
	int i;
	for(ch = 0 ; ch < NR_CHANNELS; ch++)
	{
		fifo = &bert->beFifo[ch];	
		fifo->channel = ch;
		fifo->fifoWp = 0;
		fifo->berMeasTime = 10000;
		Timer_Init(&fifo->getErrCntTimer,GetErrCntTimerProc,fifo);
		Timer_Start(&fifo->getErrCntTimer,250);
		PVar_New(PVBeratio_Get,NULL,bert,ch ,"%s.L%lu.%s",name,ch,"beRatio");
		PVar_New(PVBerate_Get,NULL,bert,ch ,"%s.L%lu.%s",name,ch,"beRate");
		PVar_New(PVLatchedLol_Get,PVLatchedLol_Set,bert,ch ,"%s.L%lu.%s",name,ch,"latchedLol");
	}
       for(i = 0; i < array_size(gForwardRegs); i++) {
                const CdrForward *fwd = &gForwardRegs[i];
                PVar_New(PVForward_Get,PVForward_Set,bert,i,"%s.%s",name,fwd->name);
        }

	PVar_New(PVBerMs_Get,PVBerMs_Set,bert,0 ,"%s.%s",name,"berMeasWin_ms");
	Timer_Init(&bert->updateLedsTimer,UpdateLedsTimerProc,bert);
	Timer_Start(&bert->updateLedsTimer,250);
	Interp_RegisterCmd(&berCmd);
}
