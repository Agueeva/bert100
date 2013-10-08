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
#include <math.h>

#define NR_CHANNELS	(4)

/* Fifo for moving window BERatio and Rate */
#define BEFIFO_SIZE	(4)
#define BEFIFO_WP(fifo) ((fifo)->fifoWp % BEFIFO_SIZE)

typedef struct BeFifo {
	uint64_t errCnt[BEFIFO_SIZE];
	uint32_t tStamp[BEFIFO_SIZE];
	uint8_t channel;
	uint8_t fifoWp;
	Timer getErrCntTimer;
} BeFifo;

typedef struct Bert {
	BeFifo beFifo[NR_CHANNELS];
	uint32_t dataRate; 
} Bert;

static Bert gBert;

static void 
GetErrCntTimerProc(void *eventData)
{
	BeFifo *fifo = eventData;
	unsigned int wp;
	Timer_Start(&fifo->getErrCntTimer,250);
	wp = BEFIFO_WP(fifo);
	fifo->errCnt[wp] = Cdr_GetErrCnt(0, fifo->channel);
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
	BeFifo *fifo = &bert->beFifo[adId];	
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

void
Bert_Init(void) 
{
	Bert *bert = &gBert;
	BeFifo *fifo;
	const char *name = "bert0";
	unsigned int ch;
	for(ch = 0 ; ch < NR_CHANNELS; ch++)
	{
		fifo = &bert->beFifo[ch];	
		fifo->channel = ch;
		fifo->fifoWp = 0;
		Timer_Init(&fifo->getErrCntTimer,GetErrCntTimerProc,fifo);
		Timer_Start(&fifo->getErrCntTimer,250);
		PVar_New(PVBeratio_Get,NULL,bert,ch ,"%s.l%lu.%s",name,ch,"beratio");
		PVar_New(PVBerate_Get,NULL,bert,ch ,"%s.l%lu.%s",name,ch,"berate");
	}
	Interp_RegisterCmd(&berCmd);
}
