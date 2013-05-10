/*
 *************************************************************
 * (C) 2009 Jochen Karrer
 *//**
 * \file eeprom.h
 * eeprom access routines for AVR8.
 *************************************************************
 */

#include "irqflags.h"
#include <util/atomic.h> 

void EEProm_Read(uint16_t addr, void *_buf, uint8_t len);
void EEProm_Write(uint16_t addr, const void *_buf, uint8_t len);
void EEProm_Init(void);
uint8_t EEProm_Read8(uint16_t addr);
void EEProm_Write8(uint8_t val,uint16_t addr);
