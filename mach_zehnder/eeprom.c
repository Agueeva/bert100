/*
 *************************************************************
 * (C) 2010 Jochen Karrer
 *//**
 * \file eeprom.c
 * EEProm access routines for ATxmega32A4.
 *************************************************************
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include "eeprom.h"
#include "irqflags.h"
#include "interpreter.h"
#include "hex.h"

#define NVM_EXEC()      asm("push r30"      "\n\t"      \
                            "push r31"      "\n\t"      \
                            "push r16"      "\n\t"      \
                            "push r18"      "\n\t"      \
                            "ldi r30, 0xCB" "\n\t"      \
                            "ldi r31, 0x01" "\n\t"      \
                            "ldi r16, 0xD8" "\n\t"      \
                            "ldi r18, 0x01" "\n\t"      \
                            "out 0x34, r16" "\n\t"      \
                            "st Z, r18"     "\n\t"      \
                            "pop r18"       "\n\t"      \
                            "pop r16"       "\n\t"      \
                            "pop r31"       "\n\t"      \
                            "pop r30"       "\n\t"      \
                            )


static inline void 
EEProm_WaitForNVM( void )
{
        while ((NVM.STATUS & NVM_NVMBUSY_bm) == NVM_NVMBUSY_bm) {
		/* Do nothing */
	}
}

 
uint8_t
EEProm_Read8(uint16_t addr)
{
	EEProm_WaitForNVM();
	NVM.ADDR0 = addr & 0xff;
	NVM.ADDR1 = (addr >> 8) & 0x1f;
	NVM.ADDR2 = 0;
	NVM.CMD = NVM_CMD_READ_EEPROM_gc; 
	NVM_EXEC();
	return NVM.DATA0;
}

/**
 *********************************************************************
 * \fn void EEProm_Read(uint16_t addr, void *_buf, uint8_t len)
 * Read a data block from the EEPROM.
 *********************************************************************
 */
void EEProm_Read(uint16_t addr, void *_buf, uint8_t len)
{
	uint8_t *buf = (uint8_t *)_buf;
	uint8_t i;
	for(i = 0; i < len; i++) {
		buf[i] = EEProm_Read8(addr + i);
	}
	return;
}


void 
EEProm_Write8(uint8_t val,uint16_t addr)
{
        /*  Flush buffer to make sure no unintetional data is written and load
         *  the "Page Load" command into the command register.
         */
        //EEPROM_FlushBuffer();
	EEProm_WaitForNVM();
        NVM.CMD = NVM_CMD_LOAD_EEPROM_BUFFER_gc;
        NVM.ADDR0 = addr & 0xFF;
        NVM.ADDR1 = (addr >> 8) & 0x1F;
        NVM.ADDR2 = 0x00;
        NVM.DATA0 = val;
        NVM.CMD = NVM_CMD_ERASE_WRITE_EEPROM_PAGE_gc;
        NVM_EXEC();
}

#if 0
/**
 *  
 */
void
EEProm_Load8(uint8_t val,uint16_t addr)
{
	EEProm_WaitForNVM();
        NVM.CMD = NVM_CMD_LOAD_EEPROM_BUFFER_gc;
        /* Set address. */
        NVM.ADDR0 = addr & (EEPROM_PAGE_SIZE - 1);
        NVM.ADDR1 = 0x00;
        NVM.ADDR2 = 0x00;
        /* Set data, which triggers loading of EEPROM page buffer. */
        NVM.DATA0 = val;
}
#endif

/**
 *********************************************************************
 * \fn void EEProm_Write(uint16_t addr, const void *_buf, uint8_t len);
 * Write a data block to the EEPROM.
 *********************************************************************
 */
void EEProm_Write(uint16_t addr, const void *_buf, uint8_t len)
{
	uint8_t i;
	uint8_t *buf = (uint8_t *) _buf;
	for (i = 0; i < len; i++) {
		EEProm_Write8(buf[i],addr + i);
	}
}

/*
 ******************************************************************
 * cmd_eer
 * 	Interpreter Interface for reading from CPU EEPROM.
 ******************************************************************
 */
static int8_t 
cmd_eer(Interp * interp, uint8_t argc, char *argv[])
{
        uint16_t ee_addr;
        uint16_t count = 1;
        uint16_t i;
        if (argc < 2) {
                return -EC_BADNUMARGS;
        }
        ee_addr = astrtoi16(argv[1]);
        if (argc > 2) {
                count = astrtoi16(argv[2]);
        }
	if((count+ee_addr) > EEPROM_SIZE) {
		count = EEPROM_SIZE - ee_addr;
	}
	for(i=0;i<count;i++) {
        	Interp_Printf_P(interp,"%02x ",EEProm_Read8(ee_addr + i));
                if(((i & 0xf) == 0xf) || (i == (count - 1))) {
			Interp_Printf_P(interp, "\n");
		}
        }
        return 0;
}

INTERP_CMD(eer, cmd_eer,
           "eer         <address> ?<count>? # Read from CPU internal EEPROM");

/*
 **************************************************************************
 * cmd_eew
 *	Interpreter interface for writing to CPU EEPROM.
 **************************************************************************
 */
static int8_t 
cmd_eew(Interp * interp, uint8_t argc, char *argv[])
{
        uint16_t ee_addr;
	uint16_t i;
	uint8_t val;
        if (argc < 3) {
                return -EC_BADNUMARGS;
        }
        ee_addr = astrtoi16(argv[1]);
        for (i = 0; i < (argc - 2); i++) {
                val = astrtoi16(argv[i + 2]);
		EEProm_Write8(val, ee_addr + i);
        }
	return 0;
}

INTERP_CMD(eew, cmd_eew,
           "eew         <address> ?<byte1>? ... # Write to CPU internal EEPROM");
