/* 
 ************************************************************************
 * SPI   
 * Currently using IO port mode
 ************************************************************************
 */
#include <stdlib.h>
#include <string.h>
#include "iodefine.h"
#include "types.h"
#include "interpreter.h"
#include "spi.h"
#include "events.h"
#include "atomic.h"
#include "console.h"
#include "hex.h"
#include "tpos.h"
#include "timer.h"
#include "byteorder.h"

#define SPI_MISO	PORTA.PORT.BIT.B7
#define SPI_MOSI(val)	BMOD(6,PORTA.DR.BYTE,(val))

#define SPI_CLK_LOW  BCLR(5,PORTA.DR.BYTE)
#define SPI_CLK_HIGH BSET(5,PORTA.DR.BYTE)
#define SPI_MOSI_LOW BCLR(6,PORTA.DR.BYTE)
#define SPI_MOSI_HIGH BSET(6,PORTA.DR.BYTE)

typedef struct Spi {
	Mutex rSema;
	uint8_t spi_mode;
} Spi;

static Spi gSpi;

/**
 **************************************************************
 * \fn void Uart3_TxIrq(void)
 **************************************************************
 */

INLINE uint8_t
_Spix(uint8_t bus, uint8_t data)
{
	uint8_t i;
	for (i = 0; i < 8; i++) {
		SPI_CLK_LOW;
		if (data & 0x80) {
			SPI_MOSI_HIGH;
		} else {
			SPI_MOSI_LOW;
		}
		SPI_CLK_HIGH;
		data <<= 1;
		if (SPI_MISO) {
			data |= 1;
		}
	}
	return data;
}

uint8_t
Spix(uint8_t bus, uint8_t data)
{
	return _Spix(bus, data);
}

INLINE uint8_t
_Spir(uint8_t bus)
{
	register uint8_t data = 0;
	SPI_MOSI_HIGH;
#if 1
	asm volatile ("BCLR %[bit_spiclk], [%[addr_spiclk]].B\n"
		      "BSET %[bit_spiclk], [%[addr_spiclk]].B\n"
		      "BTST %[bit_miso], [%[addr_miso]].B\n"
		      "BMC #7,%[data]\n"
		      "BCLR %[bit_spiclk], [%[addr_spiclk]].B\n"
		      "BSET %[bit_spiclk], [%[addr_spiclk]].B\n"
		      "BTST %[bit_miso], [%[addr_miso]].B\n"
		      "BMC #6,%[data]\n"
		      "BCLR %[bit_spiclk], [%[addr_spiclk]].B\n"
		      "BSET %[bit_spiclk], [%[addr_spiclk]].B\n"
		      "BTST %[bit_miso], [%[addr_miso]].B\n"
		      "BMC #5,%[data]\n"
		      "BCLR %[bit_spiclk], [%[addr_spiclk]].B\n"
		      "BSET %[bit_spiclk], [%[addr_spiclk]].B\n"
		      "BTST %[bit_miso], [%[addr_miso]].B\n"
		      "BMC #4,%[data]\n"
		      "BCLR %[bit_spiclk], [%[addr_spiclk]].B\n"
		      "BSET %[bit_spiclk], [%[addr_spiclk]].B\n"
		      "BTST %[bit_miso], [%[addr_miso]].B\n"
		      "BMC #3,%[data]\n"
		      "BCLR %[bit_spiclk], [%[addr_spiclk]].B\n"
		      "BSET %[bit_spiclk], [%[addr_spiclk]].B\n"
		      "BTST %[bit_miso], [%[addr_miso]].B\n"
		      "BMC #2,%[data]\n"
		      "BCLR %[bit_spiclk], [%[addr_spiclk]].B\n"
		      "BSET %[bit_spiclk], [%[addr_spiclk]].B\n"
		      "BTST %[bit_miso], [%[addr_miso]].B\n"
		      "BMC #1,%[data]\n"
		      "BCLR %[bit_spiclk], [%[addr_spiclk]].B\n"
		      "BSET %[bit_spiclk], [%[addr_spiclk]].B\n"
		      "BTST %[bit_miso], [%[addr_miso]].B\n" "BMC #0,%[data]\n":[data] "+r"(data)
		      :		/*"0" (data), */
		      [bit_spiclk] "i"(5),[addr_spiclk] "r"(&(PORTA.DR.BYTE)),
		      [bit_miso] "i"(7),[addr_miso] "r"(&(PORTA.PORT.BYTE))
		      :"memory", "cc");

#else
	uint32_t i;
	for (i = 0; i < 8; i++) {
		SPI_CLK_LOW;
		data <<= 1;
		SPI_CLK_HIGH;
		if (SPI_MISO) {
			data |= 1;
		}
	}
#endif
	return data;
}

uint8_t
Spir(uint8_t bus)
{
	return _Spir(bus);
}

/**
 *******************************************************************
 * \fn void Spiw(uint8_t spi,uint8_t data);
 * Write a byte to the SPI
 *******************************************************************
 */
INLINE void
_Spiw(uint8_t bus, uint8_t data)
{
#if 0
	uint8_t i;
	for (i = 0; i < 8; i++) {
		SPI_CLK_LOW;
		SPI_MOSI(data >> 7);
		SPI_CLK_HIGH;
		data <<= 1;
	}
#else
	asm volatile ("BCLR %[bit_spiclk], [%[addr_spiclk]].B\n" 
				  "BTST #7, %[data]\n" 
				  "BMC %[bit_mosi],[%[addr_mosi]].B\n" 
				  "BSET %[bit_spiclk], [%[addr_spiclk]].B\n" 
				  "BCLR %[bit_spiclk], [%[addr_spiclk]].B\n" 
				  "BTST #6, %[data]\n" 
				  "BMC %[bit_mosi],[%[addr_mosi]].B\n" 
				  "BSET %[bit_spiclk], [%[addr_spiclk]].B\n" 
				  "BCLR %[bit_spiclk], [%[addr_spiclk]].B\n" 
				  "BTST #5, %[data]\n"
				  "BMC %[bit_mosi],[%[addr_mosi]].B\n" 
				  "BSET %[bit_spiclk], [%[addr_spiclk]].B\n" 
				  "BCLR %[bit_spiclk], [%[addr_spiclk]].B\n" 
				  "BTST #4, %[data]\n"
				  "BMC %[bit_mosi],[%[addr_mosi]].B\n"
				  "BSET %[bit_spiclk], [%[addr_spiclk]].B\n"
				  "BCLR %[bit_spiclk], [%[addr_spiclk]].B\n"
				  "BTST #3, %[data]\n"
				  "BMC %[bit_mosi],[%[addr_mosi]].B\n"
				  "BSET %[bit_spiclk], [%[addr_spiclk]].B\n"
				  "BCLR %[bit_spiclk], [%[addr_spiclk]].B\n"
			 	  "BTST #2, %[data]\n"
				  "BMC %[bit_mosi],[%[addr_mosi]].B\n"
				  "BSET %[bit_spiclk], [%[addr_spiclk]].B\n"
				  "BCLR %[bit_spiclk], [%[addr_spiclk]].B\n"
				  "BTST #1, %[data]\n"
				  "BMC %[bit_mosi],[%[addr_mosi]].B\n"
				  "BSET %[bit_spiclk], [%[addr_spiclk]].B\n"
				  "BCLR %[bit_spiclk], [%[addr_spiclk]].B\n"
				  "BTST #0, %[data]\n"
				  "BMC %[bit_mosi],[%[addr_mosi]].B\n"
				  "BSET %[bit_spiclk], [%[addr_spiclk]].B\n":	/* No output operands */
		      :[data] "r"(data),
		      [bit_spiclk] "i"(5),[addr_spiclk] "r"(&(PORTA.DR.BYTE)),
		      [bit_mosi] "i"(6),[addr_mosi] "r"(&(PORTA.DR.BYTE))
		      :"memory", "cc");
#endif
	return;
}

void
Spiw(uint8_t bus, uint8_t data)
{
	_Spiw(bus, data);
}

/**
 ************************************************************************
 * \fn void Spir_Block(uint8_t bus,uint8_t *data,uint16_t count); 
 * Needs to be called with locked SPI
 ************************************************************************
 */
void
Spir_Block(uint8_t bus, uint8_t * data, uint16_t count)
{
	unsigned int c2;
	while (count > 0) {
		c2 = count & 0x7f;
		while (c2) {
			*data = _Spir(bus);
			c2--;
			data++;
		}
		count = count & ~0x7f;
		if (count) {
			*data = _Spir(bus);
			count--;
			data++;
		}
	}
	EV_Yield();
}

/**
 *******************************************************************************
 * \fn void Spiw_Block(uint8_t bus,const uint8_t *data,uint16_t count); 
 *******************************************************************************
 */
void
Spiw_Block(uint8_t bus, const uint8_t * data, uint16_t count)
{
	for (; count > 0; count--, data++) {
		_Spiw(bus, *data);
		if ((count & 0x7f) == 0) {
			EV_Yield();
		}
	}
	EV_Yield();
}

bool
Spi_Xmit(uint8_t bus, uint8_t * data, int count, uint8_t flags)
{
	bool verify_status = true;
#if 0
	if (!Mutex_Locked(&gSpi.rSema)) {
		Con_Printf("Shit, using nonlocked spi\n");
	}
#endif
	for (; count > 0; count--, data++) {
		uint8_t by;
		if (flags & SPI_FLAG_WRITE) {
			by = _Spix(bus, *data);
		} else {
			by = _Spir(bus);
		}
		if (flags & SPI_FLAG_READ) {
			*data = by;
		} else if (flags & SPI_FLAG_VERIFY) {
			if (*data != by) {
				verify_status = false;
			}
		}
		if ((count & 0x3f) == 1) {
			EV_Yield();
		}
	}
	return verify_status;
}

/**
 **************************************************************************
 * \fn void Spi_SelectMode(uint8_t spi,uint8_t mode) 
 * Select the SPI mode
 * The mode numbers are from wikipedia which defines
 * the modes in the same way as ATMEL.
 * Renesas uses a different CPHA/CPOL definition
 * Renesas -> Atmel + Rest of World Translation:
 * R CPHA/CPOL -> A CPHA/CPOL
 *
 * R00 -> A11
 * R01 -> A00
 * R10 -> A01
 * R11 -> A20
 **************************************************************************
 */
INLINE void
Spi_SelectMode(uint8_t bus, uint8_t mode)
{
	Spi *spi = &gSpi;
	if (spi->spi_mode == mode) {
		return;
	}
	switch (mode) {
	    case 0:
		    Con_Printf("Spi Mode %d not implemented\n", mode);
		    break;

	    case 1:
		    Con_Printf("Spi Mode %d not implemented\n", mode);
		    break;
	    case 2:
		    Con_Printf("Spi Mode %d not implemented\n", mode);
		    break;

	    case 3:
		    break;
	    default:
		    return;
	}
#if 0
	/*
	 *************************************************
	 * I Need to send a dummy byte without nCS
	 * controler updates clock state on polarity/phase
	 * change only when a transmission is done.
	 * Mode 0,1 has IDLE state "low", Mode 2,3 has
	 * Idle state "high".
	 *************************************************
	 */
	if ((uart->spi_mode ^ mode) & 2) {
		_Spir(0);
	}
#endif
	spi->spi_mode = mode;
}

/**
 *******************************************************
 * returns true if we got the lock
 *******************************************************
 */
void
Spi_Lock(uint8_t bus, uint8_t mode)
{
	Spi *spi = &gSpi;
	if (bus) {
		return;
	}
	Mutex_Lock(&spi->rSema);
	Spi_SelectMode(bus, mode);
	return;
}

/**
 *******************************************************************
 * Unlock the SPI 
 *******************************************************************
 */
void
Spi_Unlock(uint8_t bus)
{
	Spi *spi = &gSpi;
	if (bus) {
		return;
	}
	Mutex_Unlock(&spi->rSema);
}

/**
 **************************************************************************
 * static int8_t cmd_spi(Interp *interp,uint8_t argc,char *argv[]);
 **************************************************************************
 */
static int8_t
cmd_spi(Interp * interp, uint8_t argc, char *argv[])
{
	uint32_t value;
	if (argc > 1) {
		value = astrtoi32(argv[1]);
		Con_Printf("%08lx\n", swap32(value));
		value = astrtoi16(argv[1]);
		Con_Printf("%04x\n", swap16(value));
	}
	return 0;
}

INTERP_CMD(spiCmd, "spi", cmd_spi, "spi  # SPI Bus access");

/**
 ******************************************************************************
 * \fn void Spi_Enable();
 * Switch the CPU pins to SPI mode. This is called on init and when
 * switching back from GPIO mode which is used for PLD-JTAG.
 ******************************************************************************
 */

void
Spi_Enable(uint8_t bus)
{
	Spi *spi = &gSpi;
	if (bus != 0) {
		return;
	}
	spi->spi_mode = 3;
	/* SPI_MOSI */
	SPI_MOSI_HIGH;
	BSET(6, PORTA.DDR.BYTE);
	/* SPI_MISO */
	BCLR(7, PORTA.DDR.BYTE);
	BSET(7, PORTA.ICR.BYTE);
	/* SPI_CLK */
	SPI_CLK_HIGH;
	BSET(5, PORTA.DDR.BYTE);
}

/**
 ****************************************************************
 * \fn void Spi_Init(void);
 ****************************************************************
 */
void
Spi_Init(void)
{
	Spi *spi = &gSpi;
	Mutex_Init(&spi->rSema);
	Spi_Enable(0);
	Interp_RegisterCmd(&spiCmd);
}
