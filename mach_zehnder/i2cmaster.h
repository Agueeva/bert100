/*
 *************************************************************
 * (C) 2009 Jochen Karrer
 *//**
 * \file i2cmaster.h
 * Soft I2C master using normal IO-port pins.
 *************************************************************
 */
#include <stdint.h>

#define I2C_RESULT_OK           0
#define I2C_RESULT_SCL_BLKD     5
#define I2C_RESULT_ARB_LOSS     6
#define I2C_RESULT_NO_ACK       7

uint8_t I2C_Read8(uint8_t i2caddr, uint8_t daddr, uint8_t * data,
		  uint8_t count);
uint8_t I2C_Write8(uint8_t i2caddr, uint8_t daddr, const uint8_t * data,
		   uint8_t count);

void I2CM_Init(void);
