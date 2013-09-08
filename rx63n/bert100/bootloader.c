/**
 ********************************************************
 * Bootloader checking for the existence of a Software
 * update. If it is exists the CRC of the file on SD-Card
 * is checked and written to the Internal flash. 
 * The bootloader is in the last flash sector.
 * This is guaranteed by the definition of a separate
 * section in the linker script file.
 ********************************************************
 */

#include <string.h>
#include "types.h"
#include "iodefine.h"
#include "atomic.h"
#include "interpreter.h"
#include "console.h"
#include "hex.h"
#include "config.h"

#define SWUP_CRC32_START	(0x73911293)

#define SPI_MISO		PORTA.PORT.BIT.B7
#define SPI_MOSI(val)	BMOD(6,PORTA.DR.BYTE,(val))

#define SPI_CLK_LOW		BCLR(5,PORTA.DR.BYTE)
#define SPI_CLK_HIGH	BSET(5,PORTA.DR.BYTE)
#define SPI_MOSI_LOW	BCLR(6,PORTA.DR.BYTE)
#define SPI_MOSI_HIGH	BSET(6,PORTA.DR.BYTE)

#define SD_CS(val)		BMOD(1,PORTA.DR.BYTE,(val))
#define SD_nON(val)		BMOD(6,PORTB.DR.BYTE,(val))
#define SD_nON_DIR(val)	BMOD(6,PORTB.DDR.BYTE,(val))

#define SPIR1_ERRMSK        (0xfe)
#define SPIR1_NOREPLY       (0xff)

#define RAMCODE			__attribute__ ((section (".bootloader")))
#define RAMCODE_DATA	__attribute__ ((section (".bootloader.P_1")))
#define NOINLINE		__attribute__ ((noinline))

/*  Bottom of User Flash Area */
#define ROM_PE_ADDR     0x00F80000

#define PCLK_FREQUENCY 48
#define WAIT_t10USEC 2000
#define WAIT_MAX_ERASE      1000000
#define WAIT_MAX_ROM_WRITE  1000000

#define FCU_PRG_TOP     0xFEFFE000
#define FCU_RAM_TOP     0x007F8000
#define FCU_RAM_SIZE    0x2000
#define FLASH_BLOCK_SECTOR_CHAIN   0x00107800	/*   DB15 2KB: 0x00107800 - 0x00107FFF */

RAMCODE_DATA static bool card_is_sdhc;
RAMCODE_DATA static bool card_is_2_0;

RAMCODE static uint8_t
spix(uint8_t data)
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

RAMCODE NOINLINE static void
delayMs(unsigned int ms)
{
	unsigned int cnt;	
	for(cnt = 0; cnt < ms;cnt++) {
        asm("mov.l  %0, r1": : "g"((F_CPU/4000) - 1):"memory","r1");
        asm("xlabel992: ":::);
        asm("sub    #1, r1":::"r1");
        asm("bne    xlabel992":::);
    }
}

RAMCODE static inline void
MinSD_Select(void)
{
	SD_CS(0);
}

RAMCODE static inline void
MinSD_Deselect(void)
{
	SD_CS(1);
}

/**
 *******************************************************************
 * Send a command to the SD Card and receveive the result
 *******************************************************************
 */
RAMCODE static uint8_t
MinSD_Cmd(const uint8_t * cmd, uint8_t cmdlen, uint8_t * data, uint8_t datalen)
{
	uint8_t result = 0xff;
	uint16_t timeout = 10;
	unsigned int i;
	MinSD_Deselect();
	/* The spec says 8 clock cycles are enough */
	spix(0xff);
	spix(0xff);
	spix(0xff);
	MinSD_Select();
	for (i = 0; i < cmdlen; i++) {
		spix(cmd[i]);
	}
	while ((timeout-- > 0) && (result == 0xff)) {
		result = spix(0xff);
	}
	if ((result & SPIR1_ERRMSK) == 0) {
		for (i = 0; i < datalen; i++) {
			data[i] = spix(0xff);
		}
	}
	MinSD_Deselect();
	return result;
}

RAMCODE static void
MinSD_Cmd0(void)
{
	uint8_t cmd[6];
	cmd[0] = 0x40;
	cmd[1] = 0x00;
	cmd[2] = 0x00;
	cmd[3] = 0x00;
	cmd[4] = 0x00;
	cmd[5] = 0x95;
	MinSD_Cmd(cmd, 6, NULL, 0);
}

RAMCODE static void
MinSD_Cmd8(void)
{
	uint8_t cmd[6];
	uint8_t data[4];
	uint8_t result;
	cmd[0] = 0x48;
	cmd[1] = 0;
	cmd[2] = 0;
	cmd[3] = 1;
	cmd[4] = 0xaa;
	cmd[5] = 0x87;
	result = MinSD_Cmd(cmd, 6, data, 4);
	if ((result & SPIR1_ERRMSK) == 0) {
		card_is_2_0 = 1;
	} else {
		card_is_2_0 = 0;
	}
}

RAMCODE static uint8_t
MinSD_Cmd58(void)
{
	uint8_t cmd[6];
	uint8_t data[4];
	uint8_t result;
	cmd[0] = 0x40 + 58;
	cmd[1] = 0;
	cmd[2] = 0;
	cmd[3] = 0;
	cmd[4] = 0;
	cmd[5] = 0xff;
	result = MinSD_Cmd(cmd, 6, data, 4);
	if ((result & SPIR1_ERRMSK) == 0) {
		/* OCR CCS */
		if (data[0] & (1 << 6)) {
			card_is_sdhc = 1;
		} else {
			card_is_sdhc = 0;
		}
	}
	return result;
}

/**
 ********************************************************
 * \fn static uint8_t MinSD_ACmd41(void)
 ********************************************************
 */
RAMCODE static uint8_t
MinSD_ACmd41(void)
{
	uint8_t cmd55[6];
	uint8_t acmd41[6];
	uint8_t result;
	cmd55[0] = 0x77;
	cmd55[1] = 0x00;
	cmd55[2] = 0x00;
	cmd55[3] = 0x00;
	cmd55[4] = 0x00;
	cmd55[5] = 0xff;
	acmd41[0] = 0x69;
	acmd41[1] = 0x00;
	acmd41[2] = 0x30;
	acmd41[3] = 0x00;
	acmd41[4] = 0x00;
	acmd41[5] = 0xff;	/* 3.2 to 3.4 volt */
	if (card_is_2_0) {
		acmd41[1] |= 0x40;
	}
	result = MinSD_Cmd(cmd55, 6, NULL, 0);
	result = MinSD_Cmd(acmd41, 6, 0, 0);
	return result;
}

RAMCODE static uint8_t
MinSD_Cmd16(void)
{
	uint8_t cmd[6];
	cmd[0] = 0x50;
	cmd[1] = 0x00;
	cmd[2] = 0x00;
	cmd[3] = 0x02;
	cmd[4] = 0x00;
	cmd[5] = 0xff;
	return MinSD_Cmd(cmd, 6, NULL, 0);
}

/**
 **************************************************************
 * For commands with dataphase (Start token 0xfe)
 **************************************************************
 */
RAMCODE uint16_t
MinSD_ReadData(uint8_t * buf, uint16_t count)
{
	uint8_t data = 0xff;
	int i;
	do {
		data = spix(0xff);
	} while (data == 0xff);
	if (data != 0xfe) {
		return 0;
	}
	for (i = 0; i < 512; i++) {
		buf[i] = spix(0xff);
	}
	spix(0xff);		/* CRC */
	spix(0xff);
	return count;
}

/**
 *********************************************************************************
 * Read a sector from the SD-Card
 * CMD17
 *********************************************************************************
 */
RAMCODE uint16_t
MinSD_ReadBlock(uint8_t * buf, uint32_t sector)
{
	uint8_t cmd[6];
	int cnt = 0;
	int i;
	int timeout = 10;
	uint32_t address;
	uint8_t result = 0xff;
	if (card_is_sdhc) {
		address = sector;
	} else {
		address = sector << 9;
	}
	cmd[0] = 0x40 + 17;
	cmd[1] = address >> 24;
	cmd[2] = address >> 16;
	cmd[3] = address >> 8;
	cmd[4] = address;
	cmd[5] = 0xff;
	MinSD_Deselect();
	/* The spec says 8 clock cycles are enough */
	spix(0xff);
	spix(0xff);
	spix(0xff);
	MinSD_Select();
	for (i = 0; i < 6; i++) {
		spix(cmd[i]);
	}
	while ((timeout-- > 0) && (result == 0xff)) {
		result = spix(0xff);
	}
	if ((result & SPIR1_ERRMSK) != 0) {
		return 0;
	}
	cnt = MinSD_ReadData(buf, 512);
	MinSD_Deselect();
	spix(0xff);		/* Idle clock */
	return cnt;
}

/**
 ************************************************************************
 * \fn static bool MinSD_Init(void) 
 * Initialize the IO-Ports and the SD-Card 
 ************************************************************************
 */
RAMCODE static bool
MinSD_Init(void)
{
	int i;
	BSET(1, PORTA.DDR.BYTE);
	/* SD-CD does not work correctly because the pin is DPUPEB  */
	BSET(4, PORT1.ICR.BYTE);
	BCLR(4, PORT1.DDR.BYTE);

	/* SPI_MOSI */
	SPI_MOSI_HIGH;
	BSET(6, PORTA.DDR.BYTE);
	/* SPI_MISO */
	BCLR(7, PORTA.DDR.BYTE);
	BSET(7, PORTA.ICR.BYTE);
	/* SPI_CLK */
	SPI_CLK_HIGH;
	BSET(5, PORTA.DDR.BYTE);
	SD_nON_DIR(1);
	SD_nON(0);

	delayMs(10);
	for (i = 0; i < 24; i++) {
		spix(0xff);
	}
	delayMs(10);

	MinSD_Cmd0();		/* Goto Idle */
	MinSD_Cmd8();
	MinSD_Cmd58();
	for (i = 0; i < 100; i++) {
		if (MinSD_ACmd41() != 1) {
			break;
		}
		delayMs(10);
	}
	/* CMD58 (Read OCR) is needed a second time to update OCR_CCS */
	MinSD_Cmd58();
	if (MinSD_Cmd16() != SPIR1_NOREPLY) {
		return true;
	} else {
		return false;
	}
}

RAMCODE static void
copy_to_fcu_ram(void)
{
	uint32_t *src, *dst;
	/* Before writing data to the FCU RAM, clear FENTRYR to stop the FCU. */
	if (FLASH.FENTRYR.WORD != 0x0000) {
		/* Disable the FCU from accepting commands - Clear both the
		   FENTRY0(ROM) and FENTRYD(Data Flash) bits to 0 */
		FLASH.FENTRYR.WORD = 0xAA00;
	}

	/* Enable the FCU RAM */
	FLASH.FCURAME.WORD = 0xC401;
	src = (uint32_t *) FCU_PRG_TOP;
	dst = (uint32_t *) FCU_RAM_TOP;
#if 1
	__builtin_memcpy((void *)FCU_RAM_TOP, (void *)FCU_PRG_TOP, FCU_RAM_SIZE);
#else
	uint16_t i;
	/* Iterate for loop to copy the FCU firmware */
	for (i = 0; i < (FCU_RAM_SIZE / 4); i++) {
		*dst = *src;
		src++;
		dst++;
	}
#endif
	return;
}

RAMCODE static bool
Flash_Write(volatile uint8_t * flash_addr, uint8_t * data, uint32_t len)
{
	volatile uint32_t wait_cnt;
	uint32_t i;
	bool retval = true;
	FLASH.FENTRYR.WORD = 0xAA00;
	asm("nop");
	asm("nop");
	FLASH.FENTRYR.WORD = 0xaa01;
	FLASH.FWEPROR.BYTE = 0x01;

	FLASH.PCKAR.WORD = PCLK_FREQUENCY;
	/* Execute Peripheral Clock Notification Commands */
	*flash_addr = 0xE9;
	*flash_addr = 0x03;
	*(volatile uint16_t *)flash_addr = 0x0F0F;
	barrier();
	*(volatile uint16_t *)flash_addr = 0x0F0F;
	barrier();
	*(volatile uint16_t *)flash_addr = 0x0F0F;
	*flash_addr = 0xD0;
	wait_cnt = WAIT_t10USEC;
	while ((FLASH.FSTATR0.BIT.FRDY == 0) && wait_cnt) {
		wait_cnt--;
	}
	if (wait_cnt == 0) {
		retval = false;
	}
	while (len >= 256) {
		uint32_t addr;
		FLASH.FPROTR.WORD = 0x5501;
		*flash_addr = 0xE8;
		*flash_addr = 0x80;
		for (i = 0; i < 128; i++) {
			addr = (uint32_t) flash_addr + (i << 1);
			if ((addr >= 0x00FFFFA0) && (addr <= 0x00FFFFAF)) {
				/* Protect against ID code */
				*(volatile uint16_t *)flash_addr = 0xffff;
			} else {
				*(volatile uint16_t *)flash_addr = *(uint16_t *) data;
			}
			data += 2;
		}
		*flash_addr = 0xD0;
		wait_cnt = WAIT_MAX_ROM_WRITE;
		while ((FLASH.FSTATR0.BIT.FRDY == 0) && wait_cnt) {
			wait_cnt--;
		}
		if (wait_cnt == 0) {
			retval = false;
		}
		len -= 256;
		flash_addr += 256;
	}
	if (len || FLASH.FASTAT.BYTE) {
		retval = false;
	}
	*(volatile uint8_t *)ROM_PE_ADDR = 0x50;	/* Clear status */
	FLASH.FENTRYR.WORD = 0xAA00, FLASH.FWEPROR.BYTE = 0x02;
	asm("nop");
	asm("nop");
	asm("nop");
	asm("nop");
	return retval;
}

/**
 ***********************************************************************
 * Erase the ROM
 ***********************************************************************
 */
RAMCODE static bool
Flash_Erase(volatile uint8_t * flash_addr)
{
	uint32_t wait_cnt;
	bool retval = true;
	FLASH.FENTRYR.WORD = 0xAA00;
	asm("nop");
	asm("nop");
	FLASH.FENTRYR.WORD = 0xaa01;
	FLASH.FWEPROR.BYTE = 0x01;
	asm("nop");
	asm("nop");
	FLASH.PCKAR.WORD = PCLK_FREQUENCY;
	/* Execute Peripheral Clock Notification Commands */
	*flash_addr = 0xE9;
	*flash_addr = 0x03;
	*(volatile uint16_t *)flash_addr = 0x0F0F;
	barrier();
	*(volatile uint16_t *)flash_addr = 0x0F0F;
	barrier();
	*(volatile uint16_t *)flash_addr = 0x0F0F;
	*flash_addr = 0xD0;
	wait_cnt = WAIT_t10USEC;
	while ((FLASH.FSTATR0.BIT.FRDY == 0) && wait_cnt) {
		wait_cnt--;
	}
	if (wait_cnt == 0) {
		retval = false;
	}
	FLASH.FPROTR.WORD = 0x5501;
	*flash_addr = 0x20;
	*flash_addr = 0xd0;
	wait_cnt = WAIT_MAX_ERASE;
	while ((FLASH.FSTATR0.BIT.FRDY == 0) && wait_cnt) {
		wait_cnt--;
	}
	if (FLASH.FASTAT.BYTE || (wait_cnt == 0)) {
		retval = false;
	}
	*(volatile uint8_t *)ROM_PE_ADDR = 0x50;	/* Clear status */
	FLASH.FENTRYR.WORD = 0xAA00, FLASH.FWEPROR.BYTE = 0x02;
	asm("nop");
	asm("nop");
	asm("nop");
	asm("nop");
	return retval;
}

#define POLY 0xEDB88320

RAMCODE static uint32_t
CRC32Byte_Eth(uint32_t crc, uint8_t val)
{
	int i;
	for (i = 0; i < 8; i++) {
		int carry = !(crc & 1);
		int inbit = ! !(val & (1 << i));
		crc = (crc >> 1) | (UINT32_C(1) << 31);
		if (inbit ^ carry) {
			crc = crc ^ POLY;
		}
	}
	return crc;
}

RAMCODE static uint32_t
CRC32_Eth(uint32_t crc, uint8_t * buf, uint32_t len)
{
	uint32_t i;
	for (i = 0; i < len; i++) {
		crc = CRC32Byte_Eth(crc, buf[i]);
	}
	return crc;
}

RAMCODE NOINLINE static bool
Do_SWUpdate(uint32_t * sectorChainP)
{
	uint32_t val;
	uint32_t i;
	uint32_t addr;
	uint32_t *dflashP = sectorChainP;
	uint8_t buf[512];
	uint32_t sector;
	uint32_t nsectors;
	uint32_t crc32, expected_crc = 0;
	bool is_same_software = true;
	/* Set Read access for the Data Flash blocks DB8-DB15 */
	if ((dflashP[0] != 0x08154711) || (dflashP[1] != 0x08154711)) {
		return false;
	}
	dflashP = sectorChainP + 2;
	if (MinSD_Init() != true) {
		return false;
	}
	sector = dflashP[0];
	nsectors = dflashP[1];
	crc32 = SWUP_CRC32_START;
	/* 
	 ***************************************************************************************
	 * Check if the CRC is good and if the Software is identical to the already installed
	 * one
	 ***************************************************************************************
	 */
	for (addr = UINT32_C(0xFFF80000); addr <= UINT32_C(0xfffff000); addr += 512) {
		if (MinSD_ReadBlock(buf, sector) != 512) {
			return false;
		}
		nsectors--;
		sector++;
		if (addr != 0xFFFFF000) {
			for (i = 0; i < 512; i++) {
				if (buf[i] != *(uint8_t *) (addr + i)) {
					is_same_software = false;
				}
			}
			crc32 = CRC32_Eth(crc32, buf, 512);
		} else {
			expected_crc = crc32;
			crc32 = CRC32_Eth(crc32, buf, 4);
		}
		if (nsectors == 0) {
			dflashP += 2;
			sector = dflashP[0];
			nsectors = dflashP[1];
			if (sector == 0) {
				break;
			}
		}
	}
	if (crc32 != 0xffffffff) {
		return false;
	}
	if (is_same_software) {
		return false;
	}
	/* 
	 ***************************************************************************************
	 * Now flash it.
	 ***************************************************************************************
	 */
	dflashP = sectorChainP + 2;
	copy_to_fcu_ram();
	sector = dflashP[0];
	nsectors = dflashP[1];
	crc32 = SWUP_CRC32_START;
	for (addr = UINT32_C(0xFFF80000); addr < UINT32_C(0xFFFFF000); addr += 4) {
		val = *(uint32_t *) addr;
		if (val != 0xFFFFFFFF) {
			Flash_Erase((uint8_t *) (addr & 0x00ffffff));
			addr = (addr & ~UINT32_C(0xfff)) + 0x1000;
		}
	}
	for (addr = UINT32_C(0xFFF80000); addr != 0xFFFFF000; addr += 512) {
		if (MinSD_ReadBlock(buf, sector) != 512) {
			while (1) ;
		}
		Flash_Write((uint8_t *) (addr & 0x00FFFFFF), buf, 256);
		Flash_Write((uint8_t *) ((addr + 256) & 0x00FFFFFF), buf + 256, 256);
		nsectors--;
		sector++;
		if (nsectors == 0) {
			dflashP += 2;
			sector = dflashP[0];
			nsectors = dflashP[1];
			if (sector == 0) {
				break;
			}
		}
	}
	crc32 = CRC32_Eth(crc32, (uint8_t *) 0xFFF80000, 0x7f000);
	if (expected_crc != crc32) {
		/* Second time the CRC was different. Give the user a second chance by rebooting */
		/* Battery off ? */
		while (1) ;
	}
	return true;
}

extern uint8_t bootloader_start, bootloader_end;

__attribute__ ((section(".bootloader_flash")))
void
BootloaderStart(void)
{
	uint32_t addr;
	void (*fn) (void);
	SYSTEM.SCKCR.LONG = 0x00820100;
	FLASH.DFLRE1.WORD = 0xD200 | 0x0080;

    /* Enable the power from battery */
    BSET(5, PORTD.DDR.BYTE);
    BSET(5, PORTD.DR.BYTE);

	Do_SWUpdate((uint32_t *) FLASH_BLOCK_SECTOR_CHAIN);
	addr = 0xFFFFeFFC;
	fn = (typeof(fn)) addr;
	fn();
}
