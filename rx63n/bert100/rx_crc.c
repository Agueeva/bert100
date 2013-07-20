/**
 ********************************************************************************************
 * Module for using the internal CRC generator of the RX.
 * Default mode is compatible to M32C/M16C CRC generator.
 ********************************************************************************************
 */

#include <string.h>
#include <stdlib.h>
#include "iodefine.h"
#include "events.h"
#include "types.h"
#include "atomic.h"
#include "config.h"
#include "interpreter.h"
#include "interrupt_handlers.h"
#include "console.h"
#include "hex.h"
#include "rx_crc.h"

#define RX_POLY_7_LSB		(1)
#define RX_POLY_8005_LSB	(2)
#define RX_POLY_1021_LSB	(3)
#define RX_POLY_7_MSB		(5)
#define RX_POLY_8005_MSB	(6)
#define RX_POLY_1021_MSB	(7)
#define CR_CRCCLR		0x80

static uint16_t
CRC_Poly(uint8_t poly, uint16_t crc, uint8_t * data, uint32_t count)
{
	uint32_t i;
	CRC.CRCCR.BYTE = poly;
	CRC.CRCDOR = crc;
	for (i = 0; i < count; i++) {
		CRC.CRCDIR = data[i];
	}
	return CRC.CRCDOR;
}

/*
 *****************************************************
 * Same results as for M16C/M32C crc generator with
 * LSB first and 0x1021 polynomial.
 *****************************************************
 */
uint16_t
CRC16(uint16_t crc, const uint8_t * data, uint32_t count)
{
	uint32_t i;
	CRC.CRCCR.BYTE = RX_POLY_1021_LSB;
	CRC.CRCDOR = crc;
	for (i = 0; i < count; i++) {
		CRC.CRCDIR = data[i];
	}
	return CRC.CRCDOR;
}

/**
 *************************************************************
 * CRC A001 is the reverse (LSB first) 0x8005 polynom.
 *************************************************************
 */
uint16_t
CRC16_A001(uint16_t crc, const uint8_t * data, uint32_t count)
{
	uint32_t i;
	CRC.CRCCR.BYTE = RX_POLY_8005_LSB;
	CRC.CRCDOR = crc;
	for (i = 0; i < count; i++) {
		CRC.CRCDIR = data[i];
	}
	return CRC.CRCDOR;
}

/*
 ****************************************************************
 * Calculate the CRC16 value of a text string.
 * Useful for generating string hash values for example.
 ****************************************************************
 */
uint16_t
CRC16_String(const char *str)
{
	CRC.CRCCR.BYTE = RX_POLY_1021_LSB | CR_CRCCLR;
	while (*str) {
		CRC.CRCDIR = *str;
		str++;
	}
	return CRC.CRCDOR;
}

/**
 ****************************************************************************
 * Self test of the CRC16 generator with a known message taken from
 * an example in the EVA-DTS Standard.
 ****************************************************************************
 */
static void
CRC16_A001Test(void)
{
	uint16_t crc = 0;
	const uint8_t data[] = { 0x81, 0x02, 0x40, 0x05, 0x03, 0x01, 0xa3,
		0x20, 0x77, 0xff
	};
	crc = CRC16_A001(0, data, array_size(data));
	if (crc != 0xb067) {
		while (1) {
			Con_Printf("CRC selftest failed: %04x\n", crc);
		}
	}
}

/**
 *******************************************************************************
 * \fn static int8_t cmd_crc8(Interp * interp, uint8_t argc, char *argv[])
 * Calculate the 32 Bit CRC of the bytes given in the shell.
 *******************************************************************************
 */
static int8_t
cmd_crc(Interp * interp, uint8_t argc, char *argv[])
{
	uint16_t crc16 = 0;
	uint8_t data;
	uint8_t poly = 3;
	int i;
	if (argc > 1) {
		poly = astrtoi16(argv[1]);
	}
	for (i = 2; i < argc; i++) {
		data = astrtoi16(argv[i]);
		crc16 = CRC_Poly(poly, crc16, &data, 1);
	}
	Interp_Printf_P(interp, "The CRC16 is 0x%04x\n", crc16);
	return 0;
}

INTERP_CMD(crcCmd, "crc", cmd_crc, "crc <mode> <b1> <b2> ... # RX_CRC8/16");

void
RxCRC_Init(void)
{
	MSTP(CRC) = 0;
	CRC16_A001Test();
	Interp_RegisterCmd(&crcCmd);
}
