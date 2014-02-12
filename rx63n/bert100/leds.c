/**
 ***************************************************************************
 * Led sequencer
 * A command interpreter modifies the LED states 
 ***************************************************************************
 */

#include <string.h>
#include "atomic.h"
#include "iodefine.h"
#include "console.h"
#include "types.h"
#include "hex.h"
#include "interpreter.h"
#include "timer.h"
#include "leds.h"
#include "version.h"
#include "shiftreg.h"

#define CMD_DONE                (0x00000000)
#define CMD_LED_ON	    	(0x01000000)
#define CMD_LED_OFF   		(0x02000000)
#define CMD_SET_PATTERN		(0x03000000)
#define CMD_DELAY               (0x05000000)
#define CMD_RESTART             (0x06000000)
#define CMD_LOOP                (0x07000000)
#define CMD_ENDLOOP             (0x08000000)


typedef struct LedSeq {
        Timer seqTimer;
	uint16_t shiftReg;
        const uint32_t *code;
        uint16_t instrP;
        uint16_t loopStack[4];
        uint16_t loopCount[4];
        uint16_t stackP;
        bool leds_enabled;
} LedSeq;

static LedSeq gLedSeq;

INLINE void 
LedsRefresh(LedSeq *seq)
{
       ShiftReg_Out(~seq->shiftReg);
}

/**
 **************************************************************************
 *  The interpreter for the sequences.
 **************************************************************************
 */
static void
Seq_TimerProc(void *eventData)
{
        LedSeq *seq = eventData;
        uint32_t code;
        uint32_t arg;
        while (1) {
                code = seq->code[seq->instrP] & 0xFF000000;
                arg = seq->code[seq->instrP] & 0x00FFFFFF;
                seq->instrP++;
                switch (code) {
                    case CMD_DONE:
                        seq->instrP = 0;
                        seq->stackP = 0;
			seq->code = 0;
			//Con_Printf("Sequence Done\n");
			goto refresh;
                        break;

                    case CMD_DELAY:
                            Timer_Start(&seq->seqTimer, arg);
                            goto refresh;

		    case CMD_LED_ON:	
			seq->shiftReg |= (1 << arg);
			break;

		    case CMD_LED_OFF:
			seq->shiftReg &= ~(1 << arg);
			break;

		    case CMD_SET_PATTERN:
			seq->shiftReg = arg;
			break;

                    case CMD_RESTART:
                            seq->instrP = 0;
                            break;

                    case CMD_LOOP:
                            seq->loopStack[seq->stackP] = seq->instrP;
                            seq->loopCount[seq->stackP] = arg;
                            seq->stackP++;
                            break;

                    case CMD_ENDLOOP:
                            if (seq->stackP == 0) {
                                    goto refresh;
                            }
                            if (seq->loopCount[seq->stackP - 1]) {
                                    seq->loopCount[seq->stackP - 1]--;
                                    seq->instrP = seq->loopStack[seq->stackP - 1];
                            } else {
                                    seq->stackP--;
                            }
                            break;

                    default:
                            goto refresh;

                }
        }
refresh:
	LedsRefresh(seq);
}

const uint32_t seq_startup[] = {
	CMD_SET_PATTERN | 0,
        CMD_LOOP | 0,
                CMD_LED_ON | 0,
                CMD_DELAY  | 100,
                CMD_LED_ON | 1,
                CMD_DELAY  | 100,
                CMD_LED_ON | 2,
                CMD_DELAY  | 100,
                CMD_LED_ON | 3,
                CMD_DELAY  | 100,
                CMD_LED_ON | 4,
                CMD_DELAY  | 100,
                CMD_LED_ON | 5,
                CMD_DELAY  | 100,
                CMD_LED_ON | 6,
                CMD_DELAY  | 100,
                CMD_LED_ON | 7,
                CMD_DELAY  | 100,
                CMD_LED_ON | 8,
                CMD_DELAY  | 100,
                CMD_LED_ON | 9,
                CMD_DELAY  | 100,
                CMD_LED_ON | 10,
                CMD_DELAY  | 100,
                CMD_LED_ON | 11,
                CMD_DELAY  | 100,
                CMD_LED_ON | 12,
                CMD_DELAY  | 100,
                CMD_LED_ON | 13,
                CMD_DELAY  | 100,
                CMD_LED_ON | 14,
                CMD_DELAY  | 100,
                CMD_LED_ON | 15,
                CMD_DELAY  | 500,
        CMD_ENDLOOP,
	CMD_SET_PATTERN | 0x1111,
        CMD_DELAY  | 100,
	CMD_SET_PATTERN | 0x2222,
        CMD_DELAY  | 100,
	CMD_SET_PATTERN | 0x4444,
        CMD_DELAY  | 100,
	CMD_SET_PATTERN | 0x8888,
        CMD_DELAY  | 100,
	CMD_SET_PATTERN | 0x4444,
        CMD_DELAY  | 100,
	CMD_SET_PATTERN | 0x2222,
        CMD_DELAY  | 100,
	CMD_SET_PATTERN | 0x1111,
        CMD_DELAY  | 100,
	CMD_SET_PATTERN | 0,
        CMD_DONE,
};


void 
Leds_Init() 
{
        LedSeq *ls = &gLedSeq;

	ls->leds_enabled = true;
	ls->code = seq_startup;
	Timer_Init(&ls->seqTimer, Seq_TimerProc, ls);
	Timer_Start(&ls->seqTimer, 200);

}
