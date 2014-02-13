/**
 ***************************************************************************
 * Buzzer sequencer
 * A command interpreter modifies the Buzzer states 
 ***************************************************************************
 */
#include <stdlib.h>
#include <string.h>
#include "iodefine.h"
#include "types.h"
#include "interpreter.h"
#include "interrupt_handlers.h"
#include "console.h"
#include "hex.h"
#include "timer.h"
#include "atomic.h"
#include "config.h"
#include "version.h"
#include "atomic.h"


#define FREQ_C(n)		(523 << (n))
#define FREQ_CIS(n)		(554 << (n))
#define FREQ_D(n)		(587 << (n))
#define FREQ_DIS(n)		(622 << (n))
#define FREQ_E(n)		(659 << (n))
#define FREQ_F(n)		(698 << (n))
#define FREQ_FIS(n)		(739 << (n))
#define FREQ_G(n)		(783 << (n))
#define FREQ_GIS(n)		(831 << (n))
#define FREQ_A(n)		(880 << (n))
#define FREQ_AIS(n)		(932 << (n))
#define FREQ_H(n)		(987 << (n))

#define CMD_DONE                (0x00000000)
#define CMD_BUZZER_ON           (0x01000000)
#define CMD_BUZZER_OFF          (0x02000000)
#define CMD_DELAY               (0x05000000)
#define CMD_RESTART             (0x06000000)
#define CMD_LOOP                (0x07000000)
#define CMD_ENDLOOP             (0x08000000)


typedef struct BuzzerSeq {
        Timer seqTimer;
        uint16_t shiftReg;
        const uint32_t *code;
        const uint32_t *alarmcode;
        uint16_t instrP;
        uint16_t loopStack[4];
        uint16_t loopCount[4];
        uint16_t stackP;
        bool initialized;
} BuzzerSeq;

static BuzzerSeq gBuzzerSeq;

static void 
Buzzer_Start(uint32_t hz) 
{
	if(hz == 0) {
        	PORT1.PMR.BIT.B7 = 0;
        	MPC.P17PFS.BYTE = 0x0; 
		PORT1.PDR.BIT.B7 = 1;
		PORT1.PODR.BIT.B7 = 0;
		return;
	}
	MSTP(MTU3) = 0;
        MPC.P17PFS.BYTE = 0x1; /* Switch Pin to MTIO3A mode */
        MTU.TSTR.BIT.CST3 = 0;
        //MTU.TOER.BIT.OE3C = 1;  /* Enable the output,must be done before TIOR write */
        PORT1.PMR.BIT.B7 = 1;

        MTU3.TCNT = 0;  /* Reset counter */
        MTU3.TGRA = (F_PCLK / (2 * hz) - 1);
        /* PCLK / 1 */
        MTU3.TCR.BIT.TPSC = 0;
        /* Counter Cleared by TGRA match */
        MTU3.TCR.BIT.CCLR = 1;
        /* Initial output 1 Toggle at compare match */
        MTU3.TIORH.BIT.IOA = 7;
        MTU3.TMDR.BIT.MD = 0;   /* Normal mode */

        MTU.TSTR.BIT.CST3 = 1;
}

/**
 **************************************************************************
 * \fn static void Seq_TimerProc(void *eventData)
 *  The interpreter for the sequences.
 **************************************************************************
 */
static void
Seq_TimerProc(void *eventData)
{
        BuzzerSeq *seq = eventData;
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
			seq->code = seq->alarmcode;
			if(!seq->code) {
				return;
			}
			break;

                    case CMD_DELAY:
                            Timer_Start(&seq->seqTimer, arg);
			    return;

                    case CMD_BUZZER_ON:
			Buzzer_Start(arg & 0x3fff);
                        break;

                    case CMD_BUZZER_OFF:
			Buzzer_Start(0);
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
                                   return; 
                            }
                            if (seq->loopCount[seq->stackP - 1]) {
                                    seq->loopCount[seq->stackP - 1]--;
                                    seq->instrP = seq->loopStack[seq->stackP - 1];
                            } else {
                                    seq->stackP--;
                            }
                            break;

                    default:
			return;
                }
        }
}

#if 0

static const Ton_t soundfailed[] = {
        {PWM_A1, 50},
        {PWM_G1, 50},
        {PWM_F1, 50},
        {PWM_E1, 50},
        {PWM_D1, 50},
        {PWM_C1, 50},
        {PWM_A0, 50},
        {PWM_C0 * 1.5, 150},
        {PWM_PAUSE, 200},
        {PWM_C0 * 1.5, 150},
        {0, 0}
};
#endif


const uint32_t sound_ok[] = {
	CMD_BUZZER_ON | FREQ_C(2),
        CMD_DELAY  | 50,
	CMD_BUZZER_ON | FREQ_D(2),
        CMD_DELAY  | 50,
	CMD_BUZZER_ON | FREQ_E(2),
        CMD_DELAY  | 50,
	CMD_BUZZER_ON | FREQ_F(2),
        CMD_DELAY  | 50,
	CMD_BUZZER_ON | FREQ_G(2),
        CMD_DELAY  | 50,
	CMD_BUZZER_ON | FREQ_A(2),
        CMD_DELAY  | 150,
	CMD_BUZZER_OFF,
        CMD_DONE
};

const uint32_t sound_alarm[] = {
	CMD_BUZZER_ON |  FREQ_C(2),
        CMD_DELAY  | 150,
	CMD_BUZZER_OFF,
        CMD_DELAY  | 50,
	CMD_RESTART
};

void 
Buzzer_SetAlarm(bool on) 
{
	BuzzerSeq *bs = &gBuzzerSeq;
	if(!bs->initialized) {
		return;
	}
	if(on) {
		bs->alarmcode = sound_alarm;
		if(!bs->code) {
			bs->code = bs->alarmcode;
        		Timer_Start(&bs->seqTimer, 100);
		}	
	} else {
		bs->alarmcode = NULL;
		Timer_Cancel(&bs->seqTimer);
		bs->code = 0;
                bs->instrP = 0;
                bs->stackP = 0;
		Buzzer_Start(0);
	}
}

bool
Buzzer_SelectSound(uint16_t nr) 
{
	BuzzerSeq *bs = &gBuzzerSeq;
	const uint32_t *newcode;
	switch(nr) {
		case 0:
			newcode = sound_ok;
			break;
		default:
			return false;
	}
	Timer_Cancel(&bs->seqTimer);
	bs->code = 0;
        bs->instrP = 0;
        bs->stackP = 0;
	bs->code = newcode;
	Timer_Start(&bs->seqTimer,1);
	return true;
}

static int8_t
cmd_buzzer(Interp * interp, uint8_t argc, char *argv[])
{
	if((argc == 3) && (strcmp(argv[1],"alarm") == 0)) {
		bool on = !!astrtoi16(argv[2]);
		Buzzer_SetAlarm(on);
	} else if((argc == 3) && (strcmp(argv[1],"sound") == 0)) {
		uint16_t nr = astrtoi16(argv[2]);
		Buzzer_SelectSound(nr);
	} else {
		Con_Printf("P17: %u\n",PORT1.PIDR.BIT.B7);
	}
	return 0;
}
INTERP_CMD(buzzerCmd, "buzzer", cmd_buzzer, "buzzer <frequency> ");

void Buzzer_Init(void)
{
	Interp_RegisterCmd(&buzzerCmd);
	BuzzerSeq *bs = &gBuzzerSeq;
        bs->code = sound_ok;
        Timer_Init(&bs->seqTimer, Seq_TimerProc, bs);
        Timer_Start(&bs->seqTimer, 500);
	bs->initialized = true;
}
