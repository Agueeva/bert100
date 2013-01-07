/**
 *******************************************************************************************************
 * TX Control
 *******************************************************************************************************
 */

#include <stdint.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "eeprom_map.h"
#include "eeprom.h"
#include "interpreter.h"
#include "console.h"
#include "hex.h"
#include "adc.h"
#include "tx.h"

#define PORT_LDDIS      PORTC
#define PIN_LDDIS       (5)
#define CTRL_LDDIS      PORTC_PIN5CTRL 

uint16_t 
TX_GetImon(uint8_t channel) 
{
	uint16_t adval;
	uint16_t millivolt;
	adval = ADC_Read(ADC_MON2G5_CH1 - channel);
	if(adval & 0x8000) {
		millivolt = 0;
	} else {
		millivolt = (((adval * 25) >> 5) * 25) >> 4;
	}
	return millivolt;
}

void TX_Init() 
{
	uint8_t lddis_init;
	EEProm_Read(EEADDR(lddis_init),&lddis_init,1);
        if(lddis_init) {
                PORT_LDDIS.OUTSET = (1 << PIN_LDDIS);
        } else {
                PORT_LDDIS.OUTCLR = (1 << PIN_LDDIS);
        }
        CTRL_LDDIS = PORT_OPC_WIREDANDPULL_gc;
	PORT_LDDIS.DIRSET = (1 << PIN_LDDIS);

}

static int8_t cmd_lddis(Interp * interp, uint8_t argc, char *argv[])
{

        uint8_t value;
        if(argc < 2) {
                Interp_Printf_P(interp,"%d\n",(PORT_LDDIS.IN >> PIN_LDDIS) & 1);
                return 0;
        }
        value = astrtoi16(argv[1]);
        if(strcmp(argv[1],"save") == 0) {
                value = (PORT_LDDIS.IN >> PIN_LDDIS) & 1;
                EEProm_Write(EEADDR(lddis_init),&value,1);
        } else if(value) {
                PORT_LDDIS.OUTSET = (1 <<  PIN_LDDIS);
        } else {
                PORT_LDDIS.OUTCLR = (1 <<  PIN_LDDIS);
        }
        return 0;
}

INTERP_CMD(lddis, cmd_lddis,
           "lddis      # Switch the TX2G-LDDIS pin");


static int8_t cmd_mon(Interp * interp, uint8_t argc, char *argv[])
{
        uint8_t i;
        uint16_t millivolt;
        Interp_Printf_P(interp,"mon: ");
        for(i = 0;i < 4; i++) {
                millivolt = TX_GetImon(i);
                Interp_Printf_P(interp,"%d ",millivolt);
        }
        Interp_Printf_P(interp,"\n");
        return 0;
}

INTERP_CMD(mon, cmd_mon,
           "mon         # Meassure Monitor current");
