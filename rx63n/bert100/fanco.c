/**
  *****************************************************************
  * Fan Controller
  *****************************************************************
  */
#include "types.h"
#include "interpreter.h"
#include "hex.h"
#include "i2cmaster.h"
#include "fanco.h"
#include "timer.h"
#include "pvar.h"

#define MAX6651_SPEED       (0)
#define MAX6651_CONFIG      (2)
#define MAX6651_GPIO_DEF    (4)
#define MAX6651_DAC         (6)
#define MAX6651_ALARM_ENA   (8)
#define         ALARM_ENA_GPIO2         (1<<4)
#define         ALARM_ENA_GPIO1         (1<<3)
#define         ALARM_ENA_TACH          (1<<2)
#define         ALARM_ENA_MIN           (1<<1)
#define         ALARM_ENA_MAX           (1<<0)
#define MAX6651_ALARM_STAT  (10)
#define         ALARM_STAT_GPIO2        (1<<4)
#define         ALARM_STAT_GPIO1        (1<<3)
#define         ALARM_STAT_TACH         (1<<2)
#define         ALARM_STAT_MIN          (1<<1)
#define         ALARM_STAT_MAX          (1<<0)
#define MAX6651_TACH0       (12)
#define MAX6651_TACH1       (14)
#define MAX6651_TACH2       (16)
#define MAX6651_TACH3       (18)
#define MAX6651_GPIO_STAT   (20)
#define MAX6651_COUNT       (22)

#define NR_FANS	(4)

typedef struct FanCo {
	uint16_t i2cAddr;
	uint16_t fanRpm[NR_FANS];
	TimeMs_t timeStamp[NR_FANS];
} FanCo;

static FanCo gFanCo[1];

static uint32_t 
FanCo_GetRpm(FanCo *fc,int fanNr)
{
        uint8_t tacho;
	TimeMs_t now = TimeMs_Get();
        uint32_t rpm;
	if(fanNr >= NR_FANS) {
		return 0;
	}
	if((now - fc->timeStamp[fanNr]) > 550) {
		I2C_Read8(fc->i2cAddr,0xc + (fanNr << 1),&tacho, 1);
		fc->fanRpm[fanNr] = rpm = (((uint32_t)tacho << 1) >> 1) * 60;
		fc->timeStamp[fanNr] = now;
	} else {
		rpm = fc->fanRpm[fanNr];
	}
	return rpm;
}

static bool
PVRpm_Get (void *cbData, uint32_t adId, char *bufP,uint16_t maxlen)
{
	FanCo *fc = cbData;
	uint16_t rpm,fanNr;
	fanNr = adId;
	rpm = FanCo_GetRpm(fc,fanNr);
        bufP[uitoa16(rpm, bufP)] = 0;
	return true;
}

static int8_t
cmd_fan(Interp * interp, uint8_t argc, char *argv[])
{
	uint32_t i;
	FanCo *fc = &gFanCo[0];
	for(i = 0; i < NR_FANS; i ++) {
		Con_Printf("Fan %lu RPM %lu\n",i,FanCo_GetRpm(fc,i));
	}
	return 0;
}

INTERP_CMD(fanCmd, "fan", cmd_fan, "fan # Get all FAN speeds");

void
FanCo_Init(void)
{
	FanCo *fc = &gFanCo[0];
	uint8_t value = 1;
	uint16_t fanNr;
	fc->i2cAddr = 0x3e;
	I2C_Write8(fc->i2cAddr,MAX6651_COUNT,&value,1);
	Interp_RegisterCmd(&fanCmd);
	for(fanNr = 0 ; fanNr < NR_FANS; fanNr++)
	{
		PVar_New(PVRpm_Get,NULL,fc,fanNr ,"fanco.fan%u.rpm",fanNr);
	}
}
