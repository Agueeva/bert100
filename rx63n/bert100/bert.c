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

#define BEFIFO_SIZE	(4)
#define BEFIFO_WP(fifo) ((fifo)->fifoWp % BEFIFO_SIZE)
#define NR_CHANNELS	(4)

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

static int8_t
cmd_beratio(Interp * interp, uint8_t argc, char *argv[])
{
	Bert *bert = &gBert;
	unsigned int ch;
	uint64_t errCntOld,errCntNew;
	uint32_t tStampNew,tStampOld;
	uint32_t tDiff;
	uint64_t freq = 40 * (uint64_t)Synth_GetFreq(0);
	for(ch = 0; ch < NR_CHANNELS; ch++) 
	{
		BeFifo *fifo = &bert->beFifo[ch];	
		unsigned int rp;
		float rate,ratio;
		rp = BEFIFO_WP(fifo);
		errCntOld = fifo->errCnt[rp];
		tStampOld = fifo->tStamp[rp];
		rp = (rp + BEFIFO_SIZE - 1) % BEFIFO_SIZE;
		errCntNew = fifo->errCnt[rp];
		tStampNew = fifo->tStamp[rp];
		tDiff = tStampNew - tStampOld;
		if(tDiff) {
			rate = 1000. * (errCntNew - errCntOld) / tDiff;
			ratio = rate / freq; 
			Con_Printf("Lane %u: rate %f, ratio %f\n",ch,rate,ratio);
		}
	}
}

INTERP_CMD(beratioCmd, "beratio", cmd_beratio, "beratio # Get all Bit error ratios");

static int8_t
cmd_berate(Interp * interp, uint8_t argc, char *argv[])
{

}

INTERP_CMD(berateCmd, "berate", cmd_berate, "berate # Get all Bit error rates");

void
Bert_Init(void) 
{
	Bert *bert = &gBert;
	BeFifo *fifo;
	unsigned int ch;
	for(ch = 0 ; ch < NR_CHANNELS; ch++)
	{
		fifo = &bert->beFifo[ch];	
		fifo->channel = ch;
		fifo->fifoWp = 0;
		Timer_Init(&fifo->getErrCntTimer,GetErrCntTimerProc,fifo);
		Timer_Start(&fifo->getErrCntTimer,250);
	}
	Interp_RegisterCmd(&beratioCmd);
	Interp_RegisterCmd(&berateCmd);
}
