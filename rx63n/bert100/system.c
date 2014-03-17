/**
 * Process variable interface to the system
 */
#include "interpreter.h"
#include "types.h"
#include "pvar.h"
#include "timer.h"
#include "adc12.h"
#include "fanco.h"
#include "hex.h"

typedef struct SystemIf {
	Timer tempPollTimer;	
	float temp0;
	float currTemp;
	uint32_t rpmPerCelsius;
} SystemIf;

static SystemIf gSystemIf;
/**
 ************************************************************
 * Poll the CPU temperature and control the FAN
 ************************************************************
 */
static void
CPUTemp_PollTimerProc(void *eventData)
{
        SystemIf *sys = eventData;
        float temp = ADC12_GetTemperature();
        int32_t fanRpm;
	sys->currTemp = sys->currTemp * 0.8  + temp * 0.2;
        fanRpm = 2500 + (sys->currTemp - sys->temp0) * sys->rpmPerCelsius;
        if(fanRpm > 10000) {
                fanRpm = 10000;
        } else if(fanRpm < 2500) {
                fanRpm = 2500;
        }
        FanCo_SetTargetRpm(fanRpm);
        Timer_Start(&sys->tempPollTimer,2500);
}

static int8_t
cmd_fancontrol(Interp * interp, uint8_t argc, char *argv[])
{
	SystemIf *sys = &gSystemIf;
	if(argc == 3) {
		sys->temp0 = astrtof32(argv[1]);	
		sys->rpmPerCelsius = astrtoi32(argv[2]);	
	} else {
		Con_Printf("T0: %f rpmPerDegree %lu\n" ,sys->temp0, sys->rpmPerCelsius);
	}	
	return 0;
}
INTERP_CMD(fanControlCmd, "fancontrol", cmd_fancontrol, "fancontrol <temp0> <rpmPerDegree>");

static bool
PVExecScript_Set(void *cbData, uint32_t adId, const char *strP)
{
	Interp_StartScript(strP);
	return true;
}

void
SystemIf_Init(void) 
{
	SystemIf *sys = &gSystemIf;
	PVar_New(NULL,PVExecScript_Set,NULL,0,"system.execScript");
	sys->currTemp = sys->temp0 = 30;
	sys->rpmPerCelsius = 200;
	Timer_Init(&sys->tempPollTimer, CPUTemp_PollTimerProc, sys);
        Timer_Start(&sys->tempPollTimer, 2000);
	Interp_RegisterCmd(&fanControlCmd);
}
