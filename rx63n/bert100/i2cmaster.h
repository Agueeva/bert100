/*
 *************************************************************
     *//**
 * \file i2cmaster.h
 * Soft I2C master for ATMega644 using normal IO-port pins.
 *************************************************************
 */
#include "types.h"
#include "events.h"
#include "types.h"

#define I2C_RESULT_OK           0
#define I2C_RESULT_SCL_BLKD     5
#define I2C_RESULT_ARB_LOSS     6
#define I2C_RESULT_NO_ACK       7
#define I2C_RESULT_CORRUPTED	8

/**
 ************************************************************************************************************
 * I2C-Address composition:
 *			Bit  0 is zero
 * 			Bits 1-7 is the device address
 * 			Bits 8-11 is the multiplexer Channel number (Currently unused)
 *			Bits 12 + 13 is the Bus Number
 *			Bit  14	indicates that the device uses a 16 Bit memory address (64 kBit EEPROM for example).
 *			Bit  15	indicates that the device is a LOW-Speed device (100 kHz).
 ************************************************************************************************************
 */

#define I2C_LOW_SPEED_DEV		(0x8000)
#define I2C_16BIT_ADDR_DEV		(0x4000)

uint8_t I2C_Read(uint16_t i2caddr, uint16_t daddr, uint8_t * data, uint8_t count);
uint8_t I2C_Read8(uint16_t i2caddr, uint8_t daddr, uint8_t * data, uint8_t count);
uint8_t I2C_Write8(uint16_t i2caddr, uint8_t daddr, const uint8_t * data, uint8_t count);

uint8_t I2C_Read16(uint16_t i2caddr, uint16_t daddr, uint8_t * data, uint16_t count);
uint8_t I2C_Write16(uint16_t i2caddr, uint16_t daddr, const uint8_t * data, uint16_t count);
uint8_t I2C_Write(uint16_t i2caddr, uint16_t daddr, const uint8_t * data, uint8_t count);

void I2CM_Init(void);
