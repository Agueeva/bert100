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
#include "database.h"
#include "bert.h"
#include "ad537x.h"
#include "modreg.h"
#include <math.h>
#include <string.h>

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
	Timer getErrCntTimer;
	TimeMs_t berMeasTime;
	uint8_t channel;
	uint8_t fifoWp;
	uint16_t alignDummy;
	/** Accumulated BER section */
	uint64_t accBerStartCntr;
	uint64_t accBerStopCntr;
	uint64_t accFlownBits;		/* should be summed up during complete meassurement */
	uint32_t accBerStartTStampMs;
	uint32_t accBerStopTStampMs;
	bool	 accRunning;
	bool	 accMeasValid;
} BeFifo;

typedef struct Bert {
	BeFifo beFifo[NR_CHANNELS];
	Timer updateLedsTimer;
	Timer cdrRecalTimer;
	uint32_t dataRate; 
	int32_t currentDataSet;
	char currDataSetDescr[32];
	uint8_t pvLatchedLol[NR_CHANNELS];
	bool dbSwapTxPNInv[NR_CHANNELS];
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
#if 1
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
#endif
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
		.name = "L0.txaEqpst",
		.cdrRegId = CDR_TXA_EQPST(0),
		.bfCdrSelectW = (1 << CDR_ID_TX),
		.bfCdrSelectR = (1 << CDR_ID_TX),
	},
	{
		.name = "L1.txaEqpst",
		.cdrRegId = CDR_TXA_EQPST(1),
		.bfCdrSelectW = (1 << CDR_ID_TX),
		.bfCdrSelectR = (1 << CDR_ID_TX),
	},
	{
		.name = "L2.txaEqpst",
		.cdrRegId = CDR_TXA_EQPST(2),
		.bfCdrSelectW = (1 << CDR_ID_TX),
		.bfCdrSelectR = (1 << CDR_ID_TX),
	},
	{
		.name = "L3.txaEqpst",
		.cdrRegId = CDR_TXA_EQPST(3),
		.bfCdrSelectW = (1 << CDR_ID_TX),
		.bfCdrSelectR = (1 << CDR_ID_TX),
	},
	{
		.name = "L0.txaEqpre",
		.cdrRegId = CDR_TXA_EQPRE(0),
		.bfCdrSelectW = (1 << CDR_ID_TX),
		.bfCdrSelectR = (1 << CDR_ID_TX),
	},
	{
		.name = "L1.txaEqpre",
		.cdrRegId = CDR_TXA_EQPRE(1),
		.bfCdrSelectW = (1 << CDR_ID_TX),
		.bfCdrSelectR = (1 << CDR_ID_TX),
	},
	{
		.name = "L2.txaEqpre",
		.cdrRegId = CDR_TXA_EQPRE(2),
		.bfCdrSelectW = (1 << CDR_ID_TX),
		.bfCdrSelectR = (1 << CDR_ID_TX),
	},
	{
		.name = "L3.txaEqpre",
		.cdrRegId = CDR_TXA_EQPRE(3),
		.bfCdrSelectW = (1 << CDR_ID_TX),
		.bfCdrSelectR = (1 << CDR_ID_TX),
	},
#if 0
#define CDR_LOOPBACKOE(lane)                (0x01000011 + ((lane) << 24))
#endif
	{ 
		.name = "L0.txaSwing",
		.cdrRegId = CDR_TXA_SWING(0),
		.bfCdrSelectW = (1 << CDR_ID_TX),
		.bfCdrSelectR = (1 << CDR_ID_TX),
	},
	{ 
		.name = "L1.txaSwing",
		.cdrRegId = CDR_TXA_SWING(1),
		.bfCdrSelectW = (1 << CDR_ID_TX),
		.bfCdrSelectR = (1 << CDR_ID_TX),
	},
	{ 
		.name = "L2.txaSwing",
		.cdrRegId = CDR_TXA_SWING(2),
		.bfCdrSelectW = (1 << CDR_ID_TX),
		.bfCdrSelectR = (1 << CDR_ID_TX),
	},
	{ 
		.name = "L3.txaSwing",
		.cdrRegId = CDR_TXA_SWING(3),
		.bfCdrSelectW = (1 << CDR_ID_TX),
		.bfCdrSelectR = (1 << CDR_ID_TX),
	},
	{
		.name = "L0.txaOpz",
		.cdrRegId = CDR_TXA_OPZ(0),
		.bfCdrSelectW = (1 << CDR_ID_TX),
		.bfCdrSelectR = (1 << CDR_ID_TX),

	},
	{
		.name = "L1.txaOpz",
		.cdrRegId = CDR_TXA_OPZ(1),
		.bfCdrSelectW = (1 << CDR_ID_TX),
		.bfCdrSelectR = (1 << CDR_ID_TX),

	},
	{
		.name = "L2.txaOpz",
		.cdrRegId = CDR_TXA_OPZ(2),
		.bfCdrSelectW = (1 << CDR_ID_TX),
		.bfCdrSelectR = (1 << CDR_ID_TX),

	},
	{
		.name = "L3.txaOpz",
		.cdrRegId = CDR_TXA_OPZ(3),
		.bfCdrSelectW = (1 << CDR_ID_TX),
		.bfCdrSelectR = (1 << CDR_ID_TX),

	},
	{ 
		.name = "L0.txaSwingFine",
		.cdrRegId = CDR_TXA_SWING_FINE(0),
		.bfCdrSelectW = (1 << CDR_ID_TX),
		.bfCdrSelectR = (1 << CDR_ID_TX),
	},
	{ 
		.name = "L1.txaSwingFine",
		.cdrRegId = CDR_TXA_SWING_FINE(1),
		.bfCdrSelectW = (1 << CDR_ID_TX),
		.bfCdrSelectR = (1 << CDR_ID_TX),
	},
	{ 
		.name = "L2.txaSwingFine",
		.cdrRegId = CDR_TXA_SWING_FINE(2),
		.bfCdrSelectW = (1 << CDR_ID_TX),
		.bfCdrSelectR = (1 << CDR_ID_TX),
	},
	{ 
		.name = "L3.txaSwingFine",
		.cdrRegId = CDR_TXA_SWING_FINE(3),
		.bfCdrSelectW = (1 << CDR_ID_TX),
		.bfCdrSelectR = (1 << CDR_ID_TX),
	},
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
	{
		.name = "L0.PiPos1Piquadr",
		.cdrRegId = CDR_PI_POS1_PIQUADR(0), 
		.bfCdrSelectW = (1 << CDR_ID_RX),
		.bfCdrSelectR = (1 << CDR_ID_RX),
	},
	{
		.name = "L1.PiPos1Piquadr",
		.cdrRegId = CDR_PI_POS1_PIQUADR(1), 
		.bfCdrSelectW = (1 << CDR_ID_RX),
		.bfCdrSelectR = (1 << CDR_ID_RX),
	},
	{
		.name = "L2.PiPos1Piquadr",
		.cdrRegId = CDR_PI_POS1_PIQUADR(2), 
		.bfCdrSelectW = (1 << CDR_ID_RX),
		.bfCdrSelectR = (1 << CDR_ID_RX),
	},
	{
		.name = "L3.PiPos1Piquadr",
		.cdrRegId = CDR_PI_POS1_PIQUADR(3), 
		.bfCdrSelectW = (1 << CDR_ID_RX),
		.bfCdrSelectR = (1 << CDR_ID_RX),
	},
	{
		.name = "L0.PiPos1Picode",
		.cdrRegId = CDR_PI_POS1_PICODE(0), 
		.bfCdrSelectW = (1 << CDR_ID_RX),
		.bfCdrSelectR = (1 << CDR_ID_RX),
	},
	{
		.name = "L1.PiPos1Picode",
		.cdrRegId = CDR_PI_POS1_PICODE(1), 
		.bfCdrSelectW = (1 << CDR_ID_RX),
		.bfCdrSelectR = (1 << CDR_ID_RX),
	},
	{
		.name = "L2.PiPos1Picode",
		.cdrRegId = CDR_PI_POS1_PICODE(2), 
		.bfCdrSelectW = (1 << CDR_ID_RX),
		.bfCdrSelectR = (1 << CDR_ID_RX),
	},
	{
		.name = "L3.PiPos1Picode",
		.cdrRegId = CDR_PI_POS1_PICODE(3), 
		.bfCdrSelectW = (1 << CDR_ID_RX),
		.bfCdrSelectR = (1 << CDR_ID_RX),
	},
	{
		.name = "L0.SecOrderState",
		.cdrRegId = CDR_SEC_ORDER_STATE(0), 
		.bfCdrSelectW = (1 << CDR_ID_RX),
		.bfCdrSelectR = (1 << CDR_ID_RX),
	},
	{
		.name = "L1.SecOrderState",
		.cdrRegId = CDR_SEC_ORDER_STATE(1), 
		.bfCdrSelectW = (1 << CDR_ID_RX),
		.bfCdrSelectR = (1 << CDR_ID_RX),
	},
	{
		.name = "L2.SecOrderState",
		.cdrRegId = CDR_SEC_ORDER_STATE(2), 
		.bfCdrSelectW = (1 << CDR_ID_RX),
		.bfCdrSelectR = (1 << CDR_ID_RX),
	},
	{
		.name = "L3.SecOrderState",
		.cdrRegId = CDR_SEC_ORDER_STATE(3), 
		.bfCdrSelectW = (1 << CDR_ID_RX),
		.bfCdrSelectR = (1 << CDR_ID_RX),
	},
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
	fifo->errCnt[wp] = CDR_GetErrCnt(CDR_ID_RX, fifo->channel);
	fifo->tStamp[wp] = TimeMs_Get();
	fifo->fifoWp++;
}

static void 
LatchedLol_Update(Bert *bert,unsigned int lane)
{
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
		default:
			return;
	}
	bert->pvLatchedLol[lane] |= CDR_Read(CDR_ID_RX,regId);
	return;
}
/**
 **********************************************************************
 * \fn static float Bert_GetCurrBerate(Bert *bert,unsigned int lane)
 * Get the Current Bit error rate.
 **********************************************************************
 */
static float 
Bert_GetCurrBerate(Bert *bert,unsigned int lane)
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
		rate = Bert_GetCurrBerate(bert,ch);
		ratio = rate / freq; 
		Con_Printf("Lane %u: rate %f/s, ratio %e\n",ch,rate,ratio);
	}
	return 0;
}

INTERP_CMD(berCmd, "ber", cmd_ber, "ber # Get all Bit error ratios");

/**
 *************************************************************************************************
 * \fn static bool PVRelErrCntr_Get(void *cbData, uint32_t adId, char *bufP,uint16_t maxlen)
 * Get the relative error counter (difference to start of meassurement)
 *************************************************************************************************
 */
static bool
PVRelErrCntr_Get(void *cbData, uint32_t adId, char *bufP,uint16_t maxlen)
{
	uint64_t startCntr;
	uint64_t endCntr;
	uint64_t errCntr;
	unsigned int lane = adId;
	Bert *bert = cbData;
	BeFifo *fifo = &bert->beFifo[lane];	
	if(fifo->accRunning) {
		endCntr = CDR_GetErrCnt(CDR_ID_RX, fifo->channel); 
	} else {
		endCntr = fifo->accBerStopCntr;
	}	
	startCntr = fifo->accBerStartCntr;
	errCntr = endCntr - startCntr;
	bufP[uitoa64(errCntr,bufP)] = 0;
	return true;
}

/**
 *********************************************************************************************
 * \fn static bool PVBeratio_Get (void *cbData, uint32_t adId, char *bufP,uint16_t maxlen)
 * Get the Bit error ratio of the last accumulated meassurement. If the accumulation is
 * still running calculate the BERate of the partial interval.
 *********************************************************************************************
 */
static bool
PVBeratio_Get (void *cbData, uint32_t adId, char *bufP,uint16_t maxlen)
{
	Bert *bert = cbData;
	BeFifo *fifo;
	unsigned int lane = adId;
	float ratio;
	uint64_t freq;
	uint64_t endCntr,startCntr,errCntr,bitCntr;
	uint32_t tStampEnd,tStampStart;
	uint32_t tDiff;
	fifo = &bert->beFifo[lane];	
	startCntr = fifo->accBerStartCntr;
	tStampStart = fifo->accBerStartTStampMs;
	if(fifo->accRunning) {
		endCntr = CDR_GetErrCnt(CDR_ID_RX, fifo->channel); 
		tStampEnd = TimeMs_Get();
		tDiff = tStampEnd - tStampStart;
		/* accFlownBits is currently not kept up to date */
		freq = 40 * (uint64_t)Synth_GetFreq(0);
		bitCntr = freq * tDiff / 1000;
	} else {
		endCntr = fifo->accBerStopCntr;
		tStampEnd = fifo->accBerStopTStampMs;
		tDiff = tStampEnd - tStampStart;
		bitCntr = fifo->accFlownBits; 
	}	
	errCntr = endCntr - startCntr;
	if(tDiff) {
		ratio  = (float)(errCntr) / bitCntr;
	} else {
		ratio = 1;
	}
	bufP[f32toExp(ratio, bufP,  maxlen)] = 0;
	return true;
}

/**
 **************************************************************************************************
 * \fn static bool PVAccTime_Get (void *cbData, uint32_t adId, char *bufP,uint16_t maxlen)
 * Get the time of the meassurement. If the accumulation is no longer running this is the
 * difference between the stop and the start time. If the accumulation is running this is
 * the timer between the current time and the start time. 
 **************************************************************************************************
 */

static bool
PVAccTime_Get (void *cbData, uint32_t adId, char *bufP,uint16_t maxlen)
{
	Bert *bert = cbData;
	BeFifo *fifo;
	unsigned int lane = adId;
	uint32_t tStampEnd,tStampStart;
	uint32_t tDiff;
	fifo = &bert->beFifo[lane];	
	tStampStart = fifo->accBerStartTStampMs;
	if(fifo->accRunning) {
		tStampEnd = TimeMs_Get();
	} else {
		tStampEnd = fifo->accBerStopTStampMs;
	}	
	tDiff = tStampEnd - tStampStart;
	bufP[f32toa(tDiff / 1000., bufP,  maxlen)] = 0;
	return true;
}

/**
 ************************************************************************************************
 * \fn static bool PVBerAccStart_Get (void *cbData, uint32_t adId, char *bufP,uint16_t maxlen)
 ************************************************************************************************
 */
static bool
PVAccBerMeasStart_Get(void *cbData, uint32_t adId, char *bufP,uint16_t maxlen)
{
	Bert *bert = cbData;
	BeFifo *fifo = &bert->beFifo[adId];	
	bufP[uitoa16(fifo->accRunning, bufP)] = 0;
	return true;
}

static bool
PVAccBerMeasStart_Set(void *cbData, uint32_t adId, const char *strP)
{
	bool newval = !!astrtoi16(strP);
	bool running;
	unsigned int lane = adId;
	Bert *bert = cbData;
	BeFifo *fifo = &bert->beFifo[lane];	
	running = fifo->accRunning;
	if(running == newval) {
		return true;
	}
	if(newval == false) {
		uint64_t freq = 40 * (uint64_t)Synth_GetFreq(0);
		uint64_t runTimeMs;
		fifo->accBerStopCntr = CDR_GetErrCnt(CDR_ID_RX, fifo->channel);
		fifo->accBerStopTStampMs = TimeMs_Get();
		runTimeMs = fifo->accBerStopTStampMs - fifo->accBerStartTStampMs; 
		fifo->accFlownBits += freq / 100 * runTimeMs / 10; 
		LatchedLol_Update (bert,lane);
		if(bert->pvLatchedLol[lane]) {
			fifo->accMeasValid = false;
		}
	} else {
		fifo->accFlownBits = 0; 
		fifo->accBerStartCntr = CDR_GetErrCnt(CDR_ID_RX, fifo->channel);
		fifo->accBerStartTStampMs = TimeMs_Get();
		/* currently accessing the global LOL, not good but a quick hack */
		LatchedLol_Update (bert,lane);
		bert->pvLatchedLol[lane] = 0; 
		ModReg_ResetCtrlFault(lane);
		fifo->accMeasValid = true;
	}
	fifo->accRunning = newval;	
	return true;
}

static bool
PVAccMeasValid_Get(void *cbData, uint32_t adId, char *bufP,uint16_t maxlen)
{
	Bert *bert = cbData;
	unsigned int lane = adId;
	BeFifo *fifo = &bert->beFifo[lane];	
	if(fifo->accRunning) {
		LatchedLol_Update (bert,lane);
		if(bert->pvLatchedLol[lane]) {
			fifo->accMeasValid = false;
		}
	}
	bufP[uitoa16(fifo->accMeasValid,bufP)] = 0;
	return true;
}

/**
 ****************************************************************************
 * Current BER meassurement
 ****************************************************************************
 */
static bool
PVCurrBeratio_Get (void *cbData, uint32_t adId, char *bufP,uint16_t maxlen)
{
	Bert *bert = cbData;
	//BeFifo *fifo = &bert->beFifo[adId];	
	float rate,ratio;
	uint64_t freq = 40 * (uint64_t)Synth_GetFreq(0);
	rate = Bert_GetCurrBerate(bert,adId);
	ratio = rate / freq; 
	bufP[f32toExp(ratio, bufP,  maxlen)] = 0;
	return true;
}

static bool
PVCurrBerate_Get (void *cbData, uint32_t adId, char *bufP,uint16_t maxlen)
{
	Bert *bert = cbData;
	unsigned int lane = adId;
	float rate;
	rate = Bert_GetCurrBerate(bert,lane);
	bufP[f32toa(rate, bufP,  maxlen)] = 0;
	return true;
}

/**
 ************************************************************************************************
 * Meassurement window for current Bit error rate
 ************************************************************************************************
 */
static bool
PVBerMeasWin_Get(void *cbData, uint32_t adId, char *bufP,uint16_t maxlen)
{
	Bert *bert = cbData;
	BeFifo *fifo = &bert->beFifo[0]; /* Currently all 4 fifos have the same meassurement time */
	bufP[uitoa32(fifo->berMeasTime,bufP)] = 0;
	return true;
}

static bool
PVBerMeasWin_Set(void *cbData, uint32_t adId, const char *strP)
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
 * Los of lock is latched here because it is Clear on read  in the CDR.
 * But we require it with "Clear on write zero" semantics for gui or multiple
 * GUIs.
 ****************************************************************************
 */
static bool
PVLatchedLol_Get (void *cbData, uint32_t adId, char *bufP,uint16_t maxlen)
{
        Bert *bert = cbData;
	uint8_t lane = adId;
	LatchedLol_Update (bert,lane);
        bufP[uitoa16(bert->pvLatchedLol[lane],bufP)] = 0;
        return true;
}

static bool
PVLatchedLol_Set(void *cbData, uint32_t adId, const char *strP)
{
	return false;
}

static bool
PVAbsErrCntr_Get (void *cbData, uint32_t adId, char *bufP,uint16_t maxlen)
{
        uint32_t lane = adId;
        uint64_t value = CDR_GetErrCnt(CDR_ID_RX, lane);
        bufP[uitoa64(value,bufP)] = 0;
        return true;
}

static void 
Bert_RecalCdrProc(void *eventData)
{
	CDR_Recalibrate(CDR_ID_TX);
	CDR_Recalibrate(CDR_ID_RX);
}

static bool 
PVCdrTrip_Get (void *cbData, uint32_t adId, char *bufP,uint16_t maxlen)
{
	uint32_t cdrRegId = adId;
	int32_t value = 0;	
	float ppm;
	unsigned int i;
	for(i = 0; i < 4; i++) {
       		value += CDR_Read(CDR_ID_RX,cdrRegId);
	}
	ppm = (value >> 2) - 8192;
	bufP[f32toa(ppm, bufP,  maxlen)] = 0;
	return true;
}

static bool
PVBitrate_Set(void *cbData, uint32_t adId, const char *strP)
{
	Bert *bert = cbData;
	uint64_t bitrate;
	uint32_t synthFreq;
	bitrate = astrtoi64(strP);
	synthFreq = bitrate / 40;
	Synth_SetFreq(SYNTH_0,synthFreq);
	Timer_Start(&bert->cdrRecalTimer,200);
	return true;
}

static bool
PVBitrate_Get (void *cbData, uint32_t adId, char *bufP,uint16_t maxlen)
{
	uint64_t bitrate;
	uint32_t synthFreq;
	synthFreq = Synth_GetFreq(SYNTH_0);	
	bitrate = 40 * (uint64_t)synthFreq;	
	bufP[uitoa64(bitrate,bufP)] = 0;
	return true;
}

/**
 **************************************************************************************************
 * \fn static bool PVSwapTxPN_Get (void *cbData, uint32_t chNr, char *bufP,uint16_t maxlen)
 * Forwarded after translation by database.
 **************************************************************************************************
 */
static bool
PVSwapTxPN_Get (void *cbData, uint32_t chNr, char *bufP,uint16_t maxlen)
{
	Bert *bert = cbData;
	uint32_t cdrRegId;
	uint16_t value;
        if(chNr >= 4) {
                Con_Printf("Bert %s Unexpected Channel %lu\n",__func__,chNr);
                return false;
        }
	cdrRegId = CDR_SWAP_TXP_N(chNr); 
       	value = !!CDR_Read(CDR_ID_TX,cdrRegId) ^ bert->dbSwapTxPNInv[chNr];
        bufP[uitoa16(value,bufP)] = 0;
        return true;
}

static bool
PVSwapTxPN_Set(void *cbData, uint32_t chNr, const char *strP)
{
	Bert *bert = cbData;
	uint32_t cdrRegId;
        uint16_t value;
        if(chNr >= 4) {
                Con_Printf("Bert %s Unexpected Channel %lu\n",__func__,chNr);
                return false;
        }
	cdrRegId = CDR_SWAP_TXP_N(chNr); 
        value = !!astrtoi16(strP);
       	value ^= bert->dbSwapTxPNInv[chNr];
       	CDR_Write(CDR_ID_TX,cdrRegId,value);
	return true;
}

/**
 ***********************************************************************************************
 * \fn static bool PVForward_Get (void *cbData, uint32_t adId, char *bufP,uint16_t maxlen)
 * Reading some variables are directly forwarded to one of the CDR's. Here the reading
 * of these variables is done. A variable can be only read from one of the two CDR's. Reading
 * them from both makes no sense.
 ***********************************************************************************************
 */
static bool
PVForward_Get (void *cbData, uint32_t adId, char *bufP,uint16_t maxlen)
{
        const CdrForward *fwd;
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



/**
 ***********************************************************************************************
 * \fn static bool PVForward_Set(void *cbData, uint32_t adId, const char *strP)
 *
 * Write to some variables is directly forwarded to one or both CDR's. 
 ***********************************************************************************************
 */
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

/*
 *************************************************************************************************
 * \fn static bool PVUserPattern_Get(void *cbData, uint32_t adId, char *bufP,uint16_t maxlen)
 * Set a User pattern. There is only one global user pattern.
 *************************************************************************************************
 */
static bool 
PVUserPattern_Get(void *cbData, uint32_t adId, char *bufP,uint16_t maxlen)
{
	uint64_t userPattern;
       	userPattern = CDR_Read(CDR_ID_TX,CDR_CUSTOM_TST_PATL);
       	userPattern |= (uint32_t)CDR_Read(CDR_ID_TX,CDR_CUSTOM_TST_PATM) << 16;
       	userPattern |= (uint64_t)(CDR_Read(CDR_ID_TX,CDR_CUSTOM_TST_PATH) & 0xff) << 32; 
	*bufP++ = '\"';
	*bufP++ = '0';
	*bufP++ = 'x';
        bufP += itoahex64(userPattern,bufP);
	*bufP++ = '\"';
	*bufP++ = 0;
        return true;
}

static bool 
PVUserPattern_Set(void *cbData, uint32_t adId, const char *strP) 
{
        uint64_t userPattern;
	if(*strP == '\"') {
		strP++;
	} else if(*strP != '0') {
		return false;
	}
        userPattern = astrtoi64(strP);
       	CDR_Write(CDR_ID_TX,CDR_CUSTOM_TST_PATL,userPattern & 0xffff);
       	CDR_Write(CDR_ID_TX,CDR_CUSTOM_TST_PATM,(userPattern >> 16) & 0xffff);
       	CDR_Write(CDR_ID_TX,CDR_CUSTOM_TST_PATH,(userPattern >> 32) & 0xff);
	return true;
}

#define NR_TX_DRIVER_SETTINGS	(20)

typedef struct TxDriverSettings {
	uint32_t signature;
	float vg1[4];
	float vg2[4];
	float vd1[4];
	float vd2[4];

	uint8_t txaSwing[4];
	uint8_t txaEqpst[4];
	uint8_t txaEqpre[4];
	uint8_t txaSwingFine[4];
	bool	swapTxPN[4];
	float   modKi[4];	
	char	strDescription[32];
} TxDriverSettings;

/**
 ********************************************************************************
 * \fn static bool Bert_LoadDataset(uint16_t idx) 
 * Load a dataset with DAC, CDR and modulator settings from the database
 ********************************************************************************
 */
static bool 
Bert_LoadDataset(Bert *bert,uint16_t idx) 
{
        TxDriverSettings txDs;
	bool result;
	unsigned int chNr;
	unsigned int descrLen;
	if(idx >= NR_TX_DRIVER_SETTINGS) {
		Con_Printf("Selected bad driver setting with index %u\n",idx);
		return false;
	}
	memset(&txDs,0,sizeof(txDs));
	result = DB_GetObj(DBKEY_BERT0_TXDRIVER_SETTINGS(idx),&txDs,sizeof(txDs));
	if(result == false) {
		Con_Printf("Failed to load dataset %u\n",idx);
		return false;
	}
	descrLen = array_size(txDs.strDescription);
	if(txDs.strDescription[descrLen - 1] != 0) {
		txDs.strDescription[0] = 0;	/* Completely invalidate it in this case */
	}
	if(txDs.signature != 0x08154711) {
		Con_Printf("Dataset not valid\n");
		return false;
	}
	for(chNr = 0; chNr < 4; chNr++) {
		DAC_Set(DAC_MZAMP1_VG1(chNr),txDs.vg1[chNr]);
	}
	for(chNr = 0; chNr < 4; chNr++) {
		DAC_Set(DAC_MZAMP1_VG2(chNr),txDs.vg2[chNr]);
	}
	for(chNr = 0; chNr < 4; chNr++) {
		DAC_Set(DAC_MZAMP1_VD2(chNr),txDs.vd2[chNr]);
	}
	for(chNr = 0; chNr < 4; chNr++) {
		DAC_Set(DAC_MZAMP1_VD1(chNr),txDs.vd1[chNr]);
	}
	for(chNr = 0; chNr < 4; chNr++) {
        	CDR_Write(CDR_ID_TX,CDR_TXA_SWING(chNr),txDs.txaSwing[chNr]);
        	CDR_Write(CDR_ID_TX,CDR_TXA_EQPST(chNr),txDs.txaEqpst[chNr]);
        	CDR_Write(CDR_ID_TX,CDR_TXA_EQPRE(chNr),txDs.txaEqpre[chNr]);
        	CDR_Write(CDR_ID_TX,CDR_TXA_SWING_FINE(chNr),txDs.txaSwingFine[chNr]);
        	CDR_Write(CDR_ID_TX,CDR_SWAP_TXP_N(chNr),txDs.swapTxPN[chNr]);
	}
	for(chNr = 0; chNr < 4; chNr++) {
		ModReg_SetKi(chNr,txDs.modKi[chNr]);	
	}
	SNPrintf(bert->currDataSetDescr,array_size(bert->currDataSetDescr),"%s",txDs.strDescription);
	return true;		
}

/*
 ************************************************************************************************
 * \fn static bool Bert_ShowDataset(uint16_t idx) 
 ************************************************************************************************
 */
static bool 
Bert_ShowDataset(uint16_t idx) 
{
        TxDriverSettings txDs;
	bool result;
	unsigned int descrLen;
	unsigned int chNr;
	if(idx >= NR_TX_DRIVER_SETTINGS) {
		Con_Printf("Selected bad driver setting with index %u\n",idx);
		return false;
	}
	memset(&txDs,0,sizeof(txDs));
	result = DB_GetObj(DBKEY_BERT0_TXDRIVER_SETTINGS(idx),&txDs,sizeof(txDs));
	if(result == false) {
		Con_Printf("Failed to load dataset %u\n",idx);
		return false;
	}
	descrLen = array_size(txDs.strDescription);
	if(txDs.strDescription[descrLen - 1] != 0) {
		txDs.strDescription[0] = 0;	/* Completely invalidate it in this case */
	}
	if(txDs.signature != 0x08154711) {
		Con_Printf("Dataset not valid\n");
		return false;
	}
	for(chNr = 0; chNr < 4; chNr++) {
		Con_Printf("vg1_%u %f\n",chNr,txDs.vg1[chNr]);
	}
	for(chNr = 0; chNr < 4; chNr++) {
		Con_Printf("vg2_%u %f\n",chNr,txDs.vg2[chNr]);
	}
	for(chNr = 0; chNr < 4; chNr++) {
		Con_Printf("vd2_%u %f\n",chNr,txDs.vd2[chNr]);
	}
	for(chNr = 0; chNr < 4; chNr++) {
		Con_Printf("vd1_%u %f\n",chNr,txDs.vd1[chNr]);
	}
	for(chNr = 0; chNr < 4; chNr++) {
		Con_Printf("txaSwing_%u: %u\n",chNr,txDs.txaSwing[chNr]);
	}
	for(chNr = 0; chNr < 4; chNr++) {
		Con_Printf("txaSwingFine_%u: %u\n",chNr,txDs.txaSwingFine[chNr]);
	}
	for(chNr = 0; chNr < 4; chNr++) {
		Con_Printf("txaEqpst_%u: %u\n",chNr,txDs.txaEqpst[chNr]);
	}
	for(chNr = 0; chNr < 4; chNr++) {
		Con_Printf("txaEqpre_%u: %u\n",chNr,txDs.txaEqpre[chNr]);
	}
	for(chNr = 0; chNr < 4; chNr++) {
		Con_Printf("swapTxPN_%u: %u\n",chNr,txDs.txaEqpre[chNr]);
	}
	for(chNr = 0; chNr < 4; chNr++) {
		Con_Printf("modKI_%u: %f\n",chNr,txDs.modKi[chNr]);
	}
	Con_Printf("Name: \"%s\"\n",txDs.strDescription);
	return true;		
}

/**
 ********************************************************************************
 * \nf static bool Bert_SaveDataset(uint16_t idx) 
 * Write a dataset with DAC, CDR and modulator settings to the database
 ********************************************************************************
 */
static bool 
Bert_SaveDataset(Bert *bert, uint16_t idx) 
{
        TxDriverSettings txDs;
	bool result;
	unsigned int chNr;
	memset(&txDs,0,sizeof(txDs));
	if(idx >= NR_TX_DRIVER_SETTINGS) {
		Con_Printf("Selected bad driver setting with index %u\n",idx);
		return false;
	}
	for(chNr = 0; chNr < 4; chNr++) {
		DAC_Get(DAC_MZAMP1_VG1(chNr),&txDs.vg1[chNr]);
	}
	for(chNr = 0; chNr < 4; chNr++) {
		DAC_Get(DAC_MZAMP1_VG2(chNr),&txDs.vg2[chNr]);
	}
	for(chNr = 0; chNr < 4; chNr++) {
		DAC_Get(DAC_MZAMP1_VD2(chNr),&txDs.vd2[chNr]);
	}
	for(chNr = 0; chNr < 4; chNr++) {
		DAC_Get(DAC_MZAMP1_VD1(chNr),&txDs.vd1[chNr]);
	}
	for(chNr = 0; chNr < 4; chNr++) {
		txDs.txaSwing[chNr] =	CDR_Read(CDR_ID_TX,CDR_TXA_SWING(chNr));
		txDs.txaEqpst[chNr] =	CDR_Read(CDR_ID_TX,CDR_TXA_EQPST(chNr));
		txDs.txaEqpre[chNr] =  	CDR_Read(CDR_ID_TX,CDR_TXA_EQPRE(chNr));
		txDs.txaSwingFine[chNr] = CDR_Read(CDR_ID_TX,CDR_TXA_SWING_FINE(chNr));
		txDs.swapTxPN[chNr] = CDR_Read(CDR_ID_TX,CDR_SWAP_TXP_N(chNr));
	}
	for(chNr = 0; chNr < 4; chNr++) {
		txDs.modKi[chNr] = ModReg_GetKi(chNr);	
	}
	SNPrintf(txDs.strDescription,array_size(txDs.strDescription), "%s",bert->currDataSetDescr);
	txDs.signature = 0x08154711;
	result = DB_SetObj(DBKEY_BERT0_TXDRIVER_SETTINGS(idx),&txDs,sizeof(txDs));
	if(result == false) {
		Con_Printf("Failed to save dataset %u\n",idx);
		return false;
	}
	return true;		
}

/**
 *********************************************************************************
 * \fn static bool PVDataSet_Load(void *cbData, uint32_t adId, const char *strP) 
 *********************************************************************************
 */
static bool 
PVDataSet_Load(void *cbData, uint32_t adId, const char *strP) 
{
        uint16_t idx; 
	Bert *bert = cbData;
        idx = astrtoi16(strP);
	if(Bert_LoadDataset(bert, idx) == false) {
		return false;
	} else {
		return true;
	}
}

/**
 ***********************************************************************************
 * \fn static bool PVDataSet_Save(void *cbData, uint32_t adId, const char *strP) 
 * Save a dataset to the database. 
 ***********************************************************************************
 */
static bool 
PVDataSet_Save(void *cbData, uint32_t adId, const char *strP) 
{
        uint16_t idx; 
	Bert *bert = cbData;
        idx = astrtoi16(strP);
	if(idx < 1) {
		/* Dataset 0 is unchangeable by the user */
		return false;
	}
	if(Bert_SaveDataset(bert, idx) == false) {
		return false;
	} else {
		return true;
	}
}

static bool 
PVDataSet_SetDescr(void *cbData, uint32_t adId, const char *strP) 
{
	Bert *bert = cbData;
	SNPrintf(bert->currDataSetDescr, array_size(bert->currDataSetDescr), "%s", strP);
	return true;
}

static bool 
PVDataSet_GetDescr(void *cbData, uint32_t adId, char *bufP,uint16_t maxlen)
{
	Bert *bert = cbData;
	SNPrintf(bufP,maxlen,"\"%s\"",bert->currDataSetDescr);
	return true;
}

/**
 ***********************************************************************************
 * \fn static int8_t cmd_dataset(Interp * interp, uint8_t argc, char *argv[])
 ***********************************************************************************
 */
static int8_t
cmd_dataset(Interp * interp, uint8_t argc, char *argv[])
{
	uint16_t dataSetNr;
	Bert *bert = &gBert;
	if((argc == 3) && (strcmp(argv[1],"load") == 0)) {
		dataSetNr = astrtoi16(argv[2]); 
		Bert_LoadDataset(bert,dataSetNr);
	} else if((argc == 3) && (strcmp(argv[1],"save") == 0)) {
		dataSetNr = astrtoi16(argv[2]); 
		Bert_SaveDataset(bert,dataSetNr);
	} else if((argc == 3) && (strcmp(argv[1],"dump") == 0)) {
		dataSetNr = astrtoi16(argv[2]); 
		Bert_ShowDataset(dataSetNr);
	} else {
		return -EC_BADARG;		
	}
	return 0;
}

INTERP_CMD(datasetCmd, "dataset", cmd_dataset, "dataset <load | save | dump> <DataSetNr> # ");

/*
 ********************************************
 * \fn void Bert_Init(void) 
 ********************************************
 */
void
Bert_Init(void) 
{
	Bert *bert = &gBert;
	BeFifo *fifo;
	const char *name = "bert0";
	unsigned int ch;
	int i;
	memset(bert, 0, sizeof(*bert));
	bert->currentDataSet = -1; // Invalid
	for(ch = 0 ; ch < NR_CHANNELS; ch++)
	{
		fifo = &bert->beFifo[ch];	
		fifo->channel = ch;
		fifo->fifoWp = 0;
		fifo->berMeasTime = 10000;
		fifo->accMeasValid = false;
		Timer_Init(&fifo->getErrCntTimer,GetErrCntTimerProc,fifo);
		Timer_Start(&fifo->getErrCntTimer,250);
		PVar_New(PVAbsErrCntr_Get,NULL,bert,ch ,"%s.L%lu.%s",name,ch,"absErrCntr");

		PVar_New(PVCurrBeratio_Get,NULL,bert,ch ,"%s.L%lu.%s",name,ch,"currBeRatio");
		PVar_New(PVCurrBerate_Get,NULL,bert,ch ,"%s.L%lu.%s",name,ch,"currBeRate");
		PVar_New(PVLatchedLol_Get,PVLatchedLol_Set,bert,ch ,"%s.L%lu.%s",name,ch,"latchedLol");

		PVar_New(PVAccTime_Get,NULL,bert,ch ,"%s.L%lu.%s",name,ch,"accTime");
		PVar_New(PVAccBerMeasStart_Get,PVAccBerMeasStart_Set,bert,ch ,"%s.L%lu.%s",name,ch,"accBerMeasStart");
		PVar_New(PVBeratio_Get,NULL,bert,ch ,"%s.L%lu.%s",name,ch,"accBeRatio");
		PVar_New(PVRelErrCntr_Get,NULL,bert,ch ,"%s.L%lu.%s",name,ch,"accErrCntr");
		PVar_New(PVAccMeasValid_Get,NULL,bert,ch ,"%s.L%lu.%s",name,ch,"accMeasValid");
	
		PVar_New(PVCdrTrip_Get,NULL,bert,CDR_CDR2_TRIP(ch) ,"%s.L%lu.%s",name,ch,"CdrTrip");

		/* Some registers are forwarded after a translation */
		PVar_New(PVSwapTxPN_Get,PVSwapTxPN_Set,bert,ch ,"%s.L%lu.%s",name,ch,"swapTxPN");
		DB_VarInit(DBKEY_BERT0_SWAP_TXPN(ch),&bert->dbSwapTxPNInv[ch],"%s.L%lu.swapTxPNInv",name,ch);
		CDR_Write(CDR_ID_TX,CDR_SWAP_TXP_N(ch),!!bert->dbSwapTxPNInv[ch]);
	}
	for(i = 0; i < array_size(gForwardRegs); i++) {
                const CdrForward *fwd = &gForwardRegs[i];
                PVar_New(PVForward_Get,PVForward_Set,bert,i,"%s.%s",name,fwd->name);
        }
	PVar_New(NULL,PVDataSet_Load,bert,0 ,"%s.%s",name,"loadDataSet");
	PVar_New(NULL,PVDataSet_Save,bert,0 ,"%s.%s",name,"saveDataSet");
	PVar_New(PVDataSet_GetDescr,PVDataSet_SetDescr,bert,0 ,"%s.%s",name,"dataSetDescription");
	PVar_New(PVBerMeasWin_Get,PVBerMeasWin_Set,bert,0 ,"%s.%s",name,"berMeasWin_ms");
	PVar_New(PVBitrate_Get,PVBitrate_Set,bert,0 ,"%s.%s",name,"bitrate");
	PVar_New(PVUserPattern_Get,PVUserPattern_Set,bert,0 ,"%s.userPattern");
	Timer_Init(&bert->updateLedsTimer,UpdateLedsTimerProc,bert);
	Timer_Init(&bert->cdrRecalTimer,Bert_RecalCdrProc,bert);
	Timer_Start(&bert->updateLedsTimer,4000);
	Interp_RegisterCmd(&berCmd);
	Interp_RegisterCmd(&datasetCmd);
}
