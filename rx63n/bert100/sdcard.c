/**
 *********************************************************************
 * SD-Card Interface
 * With multisector read/write statemachine.
 *********************************************************************
 */
#include <ctype.h>
#include <string.h>
#include "types.h"
#include "spi.h"
#include "console.h"
#include "interpreter.h"
#include "timer.h"
#include "tpos.h"
#include "sdcard.h"
#include "iodefine.h"
#include "atomic.h"
#include "config.h"

#define SPI_BUS	(0)
#define SPIR1_IDLE              (1 << 0)
#define SPIR1_ERASE_RESET       (1 << 1)
#define SPIR1_ILLEGAL_CMD       (1 << 2)
#define SPIR1_COM_CRC_ERR       (1 << 3)
#define SPIR1_ERASE_SEQ_ERR     (1 << 4)
#define SPIR1_ADDR_ERR          (1 << 5)
#define SPIR1_PARAM_ERR         (1 << 6)
/* All bits set is typically no reply */
#define SPIR1_NOREPLY		(0xff)
/* No bit set means not idle and no error == ready */
#define SPIR1_READY			(0x00)
#define SPIR1_ERRMSK		(0xfe)

#define SPIR2_CARD_LCKD         (1<<0)
#define SPIR2_WPERSKIPLCK_FAIL  (1<<1)
#define SPIR2_ERROR             (1<<2)
#define SPIR2_CC_ERR            (1<<3)
#define SPIR2_CARD_ECC_FAIL     (1<<4)
#define SPIR2_WP_VIOLATION      (1<<5)
#define SPIR2_ERASE_PARAM       (1<<6)
#define SPIR2_OOR_CSD_OVER      (1<<7)

#define OCR_NOTBUSY    		(UINT32_C(1) << 31)
#define OCR_CCS         	(UINT32_C(1) << 30)	/* Card capacity status */
#define OCR_VOLT_27_28      (UINT32_C(1) << 15)
#define OCR_VOLT_28_29      (UINT32_C(1) << 16)
#define OCR_VOLT_29_30      (UINT32_C(1) << 17)
#define OCR_VOLT_30_31      (UINT32_C(1) << 18)
#define OCR_VOLT_31_32      (UINT32_C(1) << 19)
#define OCR_VOLT_32_33      (UINT32_C(1) << 20)
#define OCR_VOLT_33_34      (UINT32_C(1) << 21)
#define OCR_VOLT_34_35      (UINT32_C(1) << 22)
#define OCR_VOLT_35_36      (UINT32_C(1) << 23)
#define OCR_VOLT_MSK         UINT32_C(0x00ff8000)

#define STATE_MULTSECT_IDLE	(0)
#define STATE_MULTSECT_READ	(1)
#define STATE_MULTSECT_WRITE	(2)

typedef struct SDCard {
	Timer pluggedTimer;
	Timer multSectFlushTimer;
	uint8_t cid[16];
	uint8_t csd[16];
	uint32_t regOCR;
	uint8_t status;
	uint8_t card_is_sdhc;
	uint8_t card_is_2_0;
	/* For multisector read/write a sm is required */
	uint8_t state;
	uint32_t next_multsect_block;
	TimeMs_t lastMultSectWriteMs;

	uint16_t readCurrentLV100uA;
	uint16_t readCurrentHV100uA;
	uint16_t writeCurrentLV100uA;
	uint16_t writeCurrentHV100uA;

	/* Runtime configurable parameters */
	TimeMs_t parReadTimeoutMs;
	TimeMs_t parWriteTimeoutMs;
	TimeMs_t statMaxWriteTime;
	TimeMs_t statMaxReadTime;

	/* Statistics fields */
	uint32_t statMultSectR;
	uint32_t statMultSectW;
	uint32_t statReadErrors;
	uint32_t statReadTimeouts;
	uint32_t statWriteErrors;
	uint32_t statWriteTimeouts;
	uint32_t statMaxmVolt;
	uint32_t statMinmVolt;
} SDCard;

static SDCard g_sdCard;

#ifdef  BOARD_SAKURA
#define SD_CS(val) BMOD(0,PORTC.PODR.BYTE,(val))
#else
#define SD_CS(val) BMOD(2,PORT2.PODR.BYTE,(val))
#endif

/**
 ********************************************************************************
 * \fn void SDCard_EnableBuffer(SDCard *sdc) 
 * RedBox SDCard lines has no tristate buffers. So this function is 
 * empty.
 ********************************************************************************
 */
INLINE void
SDCard_EnableBuffer(SDCard * sdc)
{
}

/**
 ********************************************************************************
 * \fn void SDCard_DisableBuffer(SDCard *sdc) 
 * RedBox SDCard lines has no tristate buffers. So this function is 
 * empty.
 ********************************************************************************
 */
INLINE void
SDCard_DisableBuffer(SDCard * sdc)
{
}

INLINE void
SDCard_Lock(SDCard * sdc)
{
	Spi_Lock(0, 3);
	SDCard_EnableBuffer(sdc);
}

INLINE void
SDCard_Unlock(SDCard * sdc)
{
	SDCard_DisableBuffer(sdc);
	Spi_Unlock(0);
}

INLINE void
SDCard_Select(SDCard * sdc)
{
	SD_CS(0);
}

INLINE void
SDCard_Deselect(SDCard * sdc)
{
	SD_CS(1);
}

/**
 **********************************************************************
 * \fn static inline void SDCard_Power(SDCard *sdc,bool on); 
 * Enable/disable electrical power of the SD-Card
 **********************************************************************
 */
INLINE void
SDCard_Power(SDCard * sdc, bool on)
{
}

INLINE bool
SDCard_GetPower(SDCard * sdc)
{
	return true;
}

/**
 ******************************************************
 * Calculate the 7 bit CRC for polynomial 0x89
 ******************************************************
 */
static void
Crc7_Byte(uint8_t val, uint8_t * crc)
{
	int8_t i;
	for (i = 7; i >= 0; i--) {
		uint8_t carry = (*crc >> 6) & 1;
		uint8_t inbit = ! !(val & (1 << i));
		*crc = (*crc << 1);
		if (carry != inbit) {
			*crc = *crc ^ 0x89;
		}
		*crc = *crc & 0x7f;
	}
}

/**
 ***********************************************************************
 * 7 Bit CRC calculation for SDCard
 ***********************************************************************
 */
static uint8_t
MMC_Crc(uint8_t * data, uint16_t count)
{
	uint16_t i;
	uint8_t crc7 = 0;
	for (i = 0; i < count; i++) {
		Crc7_Byte(data[i], &crc7);
	}
	crc7 = (crc7 << 1) | 1;
	return crc7;
}

/*
 ************************************************************
 * Bit access functions for card registers cid/csd/scr
 ************************************************************
 */
static uint32_t
getbits(uint8_t * arr, int arrlen, int from, int to)
{
	int i;
	uint32_t val = 0;
	for (i = to; i >= from; i--) {
		int idx = arrlen - 1 - (i >> 3);
		int bit = i & 7;
		if (arr[idx] & (1 << bit)) {
			val = (val << 1) | 1;
		} else {
			val = (val << 1);
		}
	}
	return val;
}

#define GETBITS(a,from,to) getbits((a),sizeof((a)),(from),(to))

/**
 ************************************************************
 * \fn static uint16_t csd_get_blocklen(uint8_t *csd)
 * Get the blocklen from the CSD field.
 *************************************************************
 */
static uint16_t
csd_get_blocklen(uint8_t * csd)
{
	int csd_structure;
	uint16_t blocklen;
	int csdsize = 16;
	csd_structure = getbits(csd, csdsize, 126, 127);
	switch (csd_structure) {
	    case 0:
		    blocklen = 1 << getbits(csd, csdsize, 80, 83);
		    break;
	    case 1:
		    blocklen = 512;
		    break;
	    default:
		    blocklen = 512;
		    break;
	}
	return blocklen;
}

/*
 ***********************************************************************
 * \fn static uint64_t csd_get_capacity
 * Decode and return the capacity from the CSD field.
 ***********************************************************************
 */
static uint64_t
csd_get_capacity(uint8_t * csd)
{
	int csd_structure;
	uint32_t c_size_mult;
	uint64_t c_size;
	uint64_t capacity;
	uint32_t blocklen;
	int csdsize = 16;
	csd_structure = getbits(csd, csdsize, 126, 127);
	blocklen = csd_get_blocklen(csd);
	switch (csd_structure) {
	    case 0:
		    c_size = getbits(csd, csdsize, 62, 73);
		    c_size_mult = getbits(csd, csdsize, 47, 49);
		    //Con_Printf("csize %ld, c_size_mult %ld, blocklen %ld\n",(uint32_t)c_size,(uint32_t)c_size_mult,(uint32_t)blocklen);
		    capacity = blocklen * ((c_size + 1) << (c_size_mult + 2));
		    break;
	    case 1:
		    c_size = getbits(csd, csdsize, 48, 69);
		    capacity = (c_size + 1) * 512 * 1024;
		    break;
	    default:
		    capacity = 0;
		    Con_Printf("Unknown CSD structure version\n");
	}
	return capacity;
}

/*
 * Currents in units of 100 uA;
 */
static const uint16_t readCurrTab[8] = { 5, 10, 50, 100, 250, 350, 600, 1000 };
static const uint16_t writeCurrTab[8] = { 10, 50, 100, 250, 350, 450, 800, 2000 };

static void
csd_decode_current(SDCard * sdc)
{
	uint16_t csdsize = 16;
	uint8_t *csd = sdc->csd;
	sdc->readCurrentLV100uA = readCurrTab[getbits(csd, csdsize, 59, 61)];
	sdc->readCurrentHV100uA = readCurrTab[getbits(csd, csdsize, 56, 58)];
	sdc->writeCurrentLV100uA = writeCurrTab[getbits(csd, csdsize, 53, 55)];
	sdc->writeCurrentHV100uA = writeCurrTab[getbits(csd, csdsize, 50, 52)];
}

/**
 ***************************************************************
 * Dump the information about the card on the console
 ***************************************************************
 */
static void
dump_cardinfo(SDCard * sdc)
{
	char vendname[3] = { ' ', ' ', 0 };
	char *interface;
	uint64_t size;
	int manfact;
	uint16_t csdvers = getbits(sdc->csd, 16, 126, 127);
	int blocklen = csd_get_blocklen(sdc->csd);
	int erase_blk_en;
	int ccc;
	uint32_t psn;
	psn = GETBITS(sdc->cid, 24, 55);
	erase_blk_en = GETBITS(sdc->csd, 46, 46);
	ccc = GETBITS(sdc->csd, 84, 95);
	/* Old linux kernel has a bad CCC field check */
	manfact = sdc->cid[0];
	if (isprint(sdc->cid[1])) {
		vendname[0] = sdc->cid[1];
	} else {
		vendname[0] = ' ';
	}
	if (isprint(sdc->cid[2])) {
		vendname[1] = sdc->cid[2];
	} else {
		vendname[1] = ' ';
	}
	if (csdvers == 0) {
		interface = "SD-Card  ";
	} else if (csdvers == 1) {
		interface = "SDHC-Card";
	} else {
		interface = "Unknown";
	}
	size = csd_get_capacity(sdc->csd);

	//speed_class = GETBITS(spec->ssr,440,447) * 2;
	if (csdvers == 0) {
		Con_Printf("%s: Man: 0x%02x \"%s\" bl %d sz %ldk\n", interface, manfact, vendname,
			   blocklen, size / 1024);
	} else {
		Con_Printf("%s: Man: 0x%02x \"%s\" bl %d sz %ldk\n", interface, manfact, vendname,
			   blocklen, size / 1024);
	}
}

INLINE void
SDCard_IdleClk(SDCard * sdc, int nr_bytes)
{
	Spi_Xmit(SPI_BUS, NULL, nr_bytes, SPI_FLAG_FF);
}

/**
 **********************************************************************
 * \fn static void SDCard_StartCmd(cmd, cmdlen);
 **********************************************************************
 */
static uint8_t
SDCard_StartCmd(SDCard * sdc, uint8_t * cmd, int cmdlen)
{
	uint8_t data = 0xff;
	uint16_t timeout = 10;
//      int i;
	SDCard_Deselect(sdc);
	/* The spec says 8 clock cycles are enough */
	SDCard_IdleClk(sdc, 3);
	SDCard_Select(sdc);
//	Con_Printf("Cmd 0x%02x\n",cmd[0]);
#if 0
	for (i = 0; i < cmdlen; i++) {
		Con_Printf("%02x ", cmd[i]);
	}
	Con_Printf("\n");
#endif
	Spi_Xmit(SPI_BUS, cmd, cmdlen, SPI_FLAG_WRITE);

	data = 0xff;
	while ((timeout-- > 0) && (data == 0xff)) {
		Spi_Xmit(SPI_BUS, &data, 1, SPI_FLAG_BIDIR);
	}
	return data;
}

static void SDCard_StopMult(SDCard * sdc);

static uint8_t
SDCard_Cmd(SDCard * sdc, uint8_t * cmd, uint8_t cmdlen, uint8_t * data, uint8_t datalen)
{
	uint8_t result = 0xff;
	uint16_t timeout = 10;
	//int i;
	SDCard_Deselect(sdc);
	/* The spec says 8 clock cycles are enough */
	SDCard_IdleClk(sdc, 3); 
//	Con_Printf("cmd 0x%02x\n",cmd[0]);
	SDCard_Select(sdc);
#if 0
	for (i = 0; i < cmdlen; i++) {
		Con_Printf("%02x ", cmd[i]);
	}
	Con_Printf("\n");
#endif
	Spi_Xmit(SPI_BUS, cmd, cmdlen, SPI_FLAG_WRITE);

	result = 0xff;
	while ((timeout-- > 0) && (result == 0xff)) {
		Spi_Xmit(SPI_BUS, &result, 1, SPI_FLAG_BIDIR);
	}
	if ((result & SPIR1_ERRMSK) == 0) {
		Spi_Xmit(SPI_BUS, data, datalen, SPI_FLAG_READ);
	}
	SDCard_Deselect(sdc);
	return result;
}

/**
 *****************************************************************************
 * Wait until a write is complete
 *****************************************************************************
 */
static bool
SDCard_WriteWait(SDCard * sdc)
{
	TimeMs_t startTimeMs, actionTimeMs;
	uint8_t data;
	bool retval = true;
	startTimeMs = TimeMs_Get();
	do {
		EV_Yield();
		Spi_Xmit(SPI_BUS, &data, 1, SPI_FLAG_READ);
		actionTimeMs = TimeMs_Get() - startTimeMs;
	} while ((data != 0xff) && (actionTimeMs < sdc->parWriteTimeoutMs));
	if (actionTimeMs > sdc->statMaxWriteTime) {
		sdc->statMaxWriteTime = actionTimeMs;
	}
	if (data != 0xff) {
		Con_Printf("SD-Card Write: Wait Ready timeout\n");
		sdc->statWriteTimeouts++;
		retval = false;
	}
	//LCD_Backlight(1);
	return retval;
}

#if 0
static void
SDCard_SendStatus(SDCard * sdc)
{
	uint8_t cmd[6] = { 0x40 + 13, 0, 0, 0, 0, 0xff };
	if (SDCard_StartCmd(sdc, cmd, 6) == SPIR1_NOREPLY) {
		Con_Printf("Send status failed\n");
		sdc->status |= SDSTA_NOINIT;
	}
	SDCard_Deselect(sdc);
}
#endif

/**
 ************************************************************************
 * \fn static bool SDCard_FinishMultWrite(SDCard *sdc) 
 * Leave the multi block write state by inserting a "Stop transmission"
 * token
 ************************************************************************
 */
static bool
SDCard_FinishMultWrite(SDCard * sdc)
{
	bool retval;
	uint8_t token = 0xFD;
	SDCard_Select(sdc);
	retval = SDCard_WriteWait(sdc);
	Spi_Xmit(SPI_BUS, NULL, 2, SPI_FLAG_FF);	/* dummy */
	Spiw(SPI_BUS, token);
	/* To dummys, I think they are not needed */
	Spir(SPI_BUS);
	Spir(SPI_BUS);
	SDCard_WriteWait(sdc);
	SDCard_Deselect(sdc);
	return retval;
}

/**
 ***************************************************************
 * \fn static void SDCard_StopMult(SDCard *sdc) 
 * Stop multisector Read/Write sequences.  On multisector
 * Read a "Stop" command is sent. On multisector write a
 * "Stop Transmission" token is inserted into the data stream. 
 * When the multisector state machine is idle nothing is done.
 *
 * Does not lock the SDCard itself because it is called from
 * other commands with already locked sdcard.
 ***************************************************************
 */
static void
SDCard_StopMult(SDCard * sdc)
{
	uint8_t cmd[6] = { 0x40 + 12, 0, 0, 0, 0, 0xff };
	if (sdc->state == STATE_MULTSECT_IDLE) {
		return;
	} else if (sdc->state == STATE_MULTSECT_READ) {
		if (SDCard_Cmd(sdc, cmd, 6, NULL, 0) == SPIR1_NOREPLY) {
			Con_Printf("Stop command failed\n");
			sdc->status |= SDSTA_NOINIT;
		}
		sdc->state = STATE_MULTSECT_IDLE;
		//SDCard_IdleClk(sdc, 3);  /* Read empty ?????????????? */
		//SDCard_Deselect(sdc);
	} else if (sdc->state == STATE_MULTSECT_WRITE) {
		if (SDCard_FinishMultWrite(sdc) != true) {
			Con_Printf("Finish multsect failed\n");
			sdc->status |= SDSTA_NOINIT;
		}
		sdc->state = STATE_MULTSECT_IDLE;
	} else {
		Con_Printf("Illegal state of sdc: %u\n", sdc->state);
	}
}

/**
 ********************************************************
 * \fn static void SDCard_ReadData();
 * Must be entered with locked SPI and with Chip select
 * low. Standard says that timeout must be always 
 * 100 ms or less.
 * \retval number of bytes written.
 ********************************************************
 */
static uint16_t
SDCard_ReadData(SDCard * sdc, uint8_t * buf, uint16_t count)
{
	TimeMs_t startTimeMs;
	TimeMs_t actionTimeMs;
	uint8_t crc[2];
	uint8_t data;
	uint32_t loopcnt = 0;
	startTimeMs = TimeMs_Get();
	for(loopcnt = 0; loopcnt < 2000; loopcnt++) {
		Spi_Xmit(SPI_BUS, &data, 1, SPI_FLAG_READ);
		actionTimeMs = TimeMs_Get() - startTimeMs;
		if(data != 0xff) {
			if(data == 0xfe) {
				break;
			} else if(loopcnt != 0) {
				break;
			}
		}	
		if(actionTimeMs > sdc->parReadTimeoutMs) {
			break;
		}
		EV_Yield();
	}
	if (actionTimeMs > sdc->statMaxReadTime) {
		sdc->statMaxReadTime = actionTimeMs;
	}
	if (data != 0xfe) {
		if (data == 0xff) {
			sdc->statReadTimeouts++;
			Con_Printf("Read timeout: token %02x, sector %lu, loopcnt %lu\n", data,sdc->next_multsect_block,loopcnt);
			return 0;
		} else {
			sdc->statReadErrors++;
#if 0
			for (i = 0; i < 514; i++) {
				Spi_Xmit(SPI_BUS, &data, 1, SPI_FLAG_READ);
			}
#endif
			Con_Printf("Read: Bad Token %02x, sector %lu, loopcnt %lu\n", data,sdc->next_multsect_block,loopcnt);
		}
		return 0;
	}
	
	Spir_Block(SPI_BUS, buf, count);
	Spir_Block(SPI_BUS, crc, 2);
	return count;
}

/**
 ********************************************************************
 * \fn uint16_t SDCard_WriteData(SDCard *sdc,uint8_t token,uint8_t *buf,uint16_t count);
 * Write a sector to the card.
 * Standard says that maximum timeout is 300ms or less.
 * \retval Number of bytes written.
 ********************************************************************
 */
static uint16_t
SDCard_WriteData(SDCard * sdc, uint8_t token, const uint8_t * buf, uint16_t count)
{
	uint16_t timeout = 1;
	uint8_t data;
	uint8_t ack;
	do {
		Spi_Xmit(SPI_BUS, &data, 1, SPI_FLAG_READ);
	} while ((data != 0xff) && (timeout-- > 0));
	if (data != 0xff) {
		Con_Printf("Write wait ready timeout\n");
		return 0;
	}
	Spiw(SPI_BUS, token);
	Spiw_Block(SPI_BUS, (uint8_t *) buf, count);
	Spi_Xmit(SPI_BUS, NULL, 2, SPI_FLAG_FF);	/* CRC dummy */
	Spi_Xmit(SPI_BUS, &ack, 1, SPI_FLAG_READ);
	if ((ack & 0x1F) != 0x05) {	/* If not accepted, return with error */
		Con_Printf("Not accepted: %02x\n", ack);
		sdc->statWriteErrors++;
		Spi_Xmit(SPI_BUS, &ack, 1, SPI_FLAG_READ);
		Con_Printf("Next: %02x\n", ack);
		return 0;
	}
	if (SDCard_WriteWait(sdc) == false) {
		return 0;
	} else {
		return count;
	}
	return count;
}

/**
 *****************************************************************************************
 * \fn static uint16_t SDCard_WriteMultData(SDCard *sdc,const uint8_t *buf,uint16_t count) 
 * Multi sector write data phase
 *****************************************************************************************
 */
static uint16_t
SDCard_WriteMultData(SDCard * sdc, const uint8_t * buf, uint16_t count)
{
	bool retval;
	uint8_t ack;
	uint8_t token = 0xFC;	/* Multi sector write block start token */

	retval = SDCard_WriteWait(sdc);
	if (retval == false) {
		Con_Printf("Timeout waiting for Write\n");
		return 0;
	}

	Spiw(SPI_BUS, token);
	Spiw_Block(SPI_BUS, (uint8_t *) buf, count);
	Spi_Xmit(SPI_BUS, NULL, 2, SPI_FLAG_FF);	/* CRC dummy */
	Spi_Xmit(SPI_BUS, &ack, 1, SPI_FLAG_READ);
	if ((ack & 0x1F) != 0x05) {	/* If not accepted, return with error */
		Con_Printf("Multblock not accepted: %02x\n", ack);
		Spi_Xmit(SPI_BUS, &ack, 1, SPI_FLAG_READ);
		Con_Printf("Next: %02x\n", ack);
		return 0;
	}
	return count;
}

/**
 **************************************************************
 * SDCard CMD 0
 **************************************************************
 */
static uint8_t
SDCard_Cmd0(SDCard * sdc)
{
	static uint8_t cmd[] = { 0x40, 0x00, 0x00, 0x00, 0x00, 0x95 };
	uint8_t result;
	SDCard_Lock(sdc);
	SDCard_StopMult(sdc);
	result = SDCard_Cmd(sdc, cmd, 6, NULL, 0);
	SDCard_Unlock(sdc);
	return result;
}

/**
 *****************************************************
 * \fn static uint8_t SDCard_Cmd(SDCard *sdc);
 * Send Interface condition
 *****************************************************
 */

static uint8_t
SDCard_Cmd8(SDCard * sdc)
{
	uint8_t cmd[] = { 0x48, 0x00, 0x00, 0x01, 0xaa, 0x87 };
	uint8_t data[4];
	uint8_t result;
	SDCard_Lock(sdc);
	SDCard_StopMult(sdc);
	result = SDCard_Cmd(sdc, cmd, 6, data, 4);
	if ((result & SPIR1_ERRMSK) == 0) {
		sdc->card_is_2_0 = 1;
	} else {
		sdc->card_is_2_0 = 0;
	}
	SDCard_Unlock(sdc);
	return result;
}

/**
 *******************************************************************
 * SDCard_Cmd58()
 * Read OCR. 
 *******************************************************************
 */
static uint8_t
SDCard_Cmd58(SDCard * sdc)
{
	uint8_t cmd0[] = { 0x40 + 58, 0x00, 0x00, 0x00, 0x00, 0xff };
	uint8_t data[4];
	uint32_t ocr;
	uint8_t result;
	SDCard_Lock(sdc);
	SDCard_StopMult(sdc);
	result = SDCard_Cmd(sdc, cmd0, 6, data, 4);
	if ((result & SPIR1_ERRMSK) == 0) {
		ocr = ((uint32_t) data[0] << 24) | ((uint32_t) data[1] << 16)
		    | ((uint32_t) data[2] << 8) | data[3];
		if (ocr & OCR_CCS) {
			sdc->card_is_sdhc = 1;
		} else {
			sdc->card_is_sdhc = 0;
		}
		sdc->regOCR = ocr;
	}
	SDCard_Unlock(sdc);
	return result;
}

/**
 **********************************************************************
 * \fn static uint8_t SDCard_Cmd16(SDCard *sdc) 
 * Set blocklen command. The Blocklen is set to 512 Bytes.
 **********************************************************************
 */
static uint8_t
SDCard_Cmd16(SDCard * sdc)
{
	uint8_t cmd[] = { 0x50, 0x00, 0x00, 0x02, 0x00, 0xff };
	uint8_t result;
	SDCard_Lock(sdc);
	SDCard_StopMult(sdc);
	result = SDCard_Cmd(sdc, cmd, 6, NULL, 0);
	SDCard_Unlock(sdc);
	return result;
}

/**
 ********************************************************
 * \fn static uint8_t SDCard_ACmd41(SDCard *sdc)
 ********************************************************
 */
static uint8_t
SDCard_ACmd41(SDCard * sdc)
{
	uint8_t cmd55[6] = { 0x77, 0x00, 0x00, 0x00, 0x00, 0xff };
	uint8_t acmd41[6] = { 0x69, 0x00, 0x30, 0x00, 0x00, 0xff };	/* 3.2 to 3.4 volt */
	uint8_t result;
	if (sdc->card_is_2_0) {
		acmd41[1] |= OCR_CCS >> 24;
	}
	/* Lock should be no problem, no taskswitch between unlock and lock */
	SDCard_Lock(sdc);
	SDCard_StopMult(sdc);
	SDCard_Cmd(sdc, cmd55, 6, NULL, 0);
	result = SDCard_Cmd(sdc, acmd41, 6, 0, 0);
	SDCard_Unlock(sdc);
	return result;
}

/**
 ***************************************************************
 * \fn static void SDCard_Init();
 ***************************************************************
 */
static bool
SDCard_Init(SDCard * sdc)
{
	int i;
	SDCard_Power(sdc, true);
	SleepMs(10);
	SDCard_Lock(sdc);
	sdc->state = STATE_MULTSECT_IDLE;
	Spi_Xmit(SPI_BUS, NULL, 24, SPI_FLAG_FF);
	SleepMs(10);
	SDCard_Unlock(sdc);
	sdc->card_is_sdhc = 0;
	sdc->card_is_2_0 = 0;
	SDCard_Cmd0(sdc);	/* Goto Idle */
	SDCard_Cmd8(sdc);
	SDCard_Cmd58(sdc);
	for (i = 0; i < 100; i++) {
		if (SDCard_ACmd41(sdc) != 1) {
			break;
		}
		SDCard_Lock(sdc);
		SleepMs(10);
		SDCard_Unlock(sdc);
	}
	/* CMD58 (Read OCR) is needed a second time to update OCR_CCS */
	SDCard_Cmd58(sdc);
	if (SDCard_Cmd16(sdc) != SPIR1_NOREPLY) {
		Timer_Start(&sdc->pluggedTimer, 500);
		sdc->status = 0;
		SDCard_Lock(sdc);
		SleepMs(200);
		SDCard_Unlock(sdc);
		return true;
	} else {
		Con_Printf("SD-Card Init failed\n");
		return false;
	}
}

/**
 ***************************************************
 * SDCard_ReadCid();
 * CMD10
 ***************************************************
 */
static uint8_t
SDCard_ReadCid(SDCard * sdc)
{
	int i, cnt;
	uint8_t cmd[6] = { 0x40 + 0xA, 0, 0, 0, 0, 0xff };
	SDCard_Lock(sdc);
	SDCard_StopMult(sdc);
	SDCard_StartCmd(sdc, cmd, 6);
	cnt = SDCard_ReadData(sdc, sdc->cid, 16);

	SDCard_Deselect(sdc);
	SDCard_IdleClk(sdc, 1);
	SDCard_Unlock(sdc);
	for (i = 0; i < cnt; i++) {
		Con_Printf("0x%02x,", sdc->cid[i]);
	}
	Con_Printf("\n");
	if (cnt == 16) {
		return SDRES_OK;
	} else {
		return SDRES_ERROR;
	}
}

/**
 ***********************************************************************
 * \fn static void SDCard_ReadCsd(SDCard *sdc);
 * CMD9
 ***********************************************************************
 */
static uint8_t
SDCard_ReadCsd(SDCard * sdc)
{
	int cnt;
	uint8_t cmd[6] = { 0x40 + 0x9, 0, 0, 0, 0, 0xff };
	SDCard_Lock(sdc);
	SDCard_StopMult(sdc);
	SDCard_StartCmd(sdc, cmd, 6);
	cnt = SDCard_ReadData(sdc, sdc->csd, 16);
	SDCard_Deselect(sdc);
	/* ATP card is not happy with 8 clock cycles it requires 128 more */
	SDCard_IdleClk(sdc, 1);
	SDCard_Unlock(sdc);
#if 0
	for (i = 0; i < cnt; i++) {
		Con_Printf("0x%02x,", sdc->csd[i]);
	}
	Con_Printf("\n");
#endif
	if (cnt == 16) {
		return SDRES_OK;
	} else {
		return SDRES_ERROR;
	}
}

/**
 *********************************************************************************
 * Read a sector from the SD-Card
 * CMD17
 *********************************************************************************
 */
static uint16_t
SDCard_ReadBlock(SDCard * sdc, uint8_t * buf, uint32_t sector)
{
	int cnt = 0;
	uint8_t cmd[6] = { 0x40 + 17, 0, 0, 0, 0, 0xff };
	uint32_t address;
	if (sdc->card_is_sdhc) {
		address = sector;
	} else {
		address = sector << 9;
	}
	cmd[1] = address >> 24;
	cmd[2] = address >> 16;
	cmd[3] = address >> 8;
	cmd[4] = address;
//      cmd[5] = MMC_Crc(cmd,5);
	SDCard_Lock(sdc);
	SDCard_StopMult(sdc);
	if (SDCard_StartCmd(sdc, cmd, 6) == SPIR1_NOREPLY) {
		Con_Printf("Read sector %lu failed\n", sector);
		sdc->status |= SDSTA_NOINIT;
	} else {
		cnt = SDCard_ReadData(sdc, buf, 512);
	}

	SDCard_Deselect(sdc);
	SDCard_IdleClk(sdc, 1);
	SDCard_Unlock(sdc);
	return cnt;
}

/*
 ******************************************************************************
 * \fn static uint16_t SDCard_ReadMultBlock(SDCard *sdc,uint8_t *buf,uint32_t sector) 
 ******************************************************************************
 */
static uint16_t
SDCard_ReadMultBlock(SDCard * sdc, uint8_t * buf, uint32_t sector)
{
	uint32_t address;
	uint8_t cmd[6] = { 0x40 + 18, 0, 0, 0, 0, 0xff };
	uint16_t cnt = 0;
	SDCard_Lock(sdc);
	if (sdc->state == STATE_MULTSECT_READ) {
		if (sector != sdc->next_multsect_block) {
			//Con_Printf("Prev not matching %lu->%lu\n",sdc->next_multsect_block,sector);
			SDCard_StopMult(sdc);
		}
	} else if (sdc->state != STATE_MULTSECT_IDLE) {
		SDCard_StopMult(sdc);
	}
	switch (sdc->state) {
	    case STATE_MULTSECT_IDLE:
		    if (sdc->card_is_sdhc) {
			    address = sector;
		    } else {
			    address = sector << 9;
		    }
		    cmd[1] = address >> 24;
		    cmd[2] = address >> 16;
		    cmd[3] = address >> 8;
		    cmd[4] = address;
		    if (SDCard_StartCmd(sdc, cmd, 6) == SPIR1_NOREPLY) {
			    Con_Printf("Read sector %lu failed\n", sector);
			    sdc->status |= SDSTA_NOINIT;
			    break;
		    } else {
			    sdc->state = STATE_MULTSECT_READ;
			    sdc->next_multsect_block = sector;
		    }
		    SDCard_Deselect(sdc);
		    SDCard_IdleClk(sdc, 1);

	    case STATE_MULTSECT_READ:
		    sdc->statMultSectR++;
		    SDCard_Select(sdc);
		    cnt = SDCard_ReadData(sdc, buf, 512);
		    break;
	}
	SDCard_Deselect(sdc);
	SDCard_IdleClk(sdc, 1);
	if (cnt == 512) {
		sdc->next_multsect_block++;
	}
	SDCard_Unlock(sdc);
	return cnt;
}

/**
 ****************************************************
 * Single block write CMD24
 ****************************************************
 */
static uint16_t
SDCard_WriteBlock(SDCard * sdc, const uint8_t * buf, uint32_t sector)
{
	int cnt;
	uint8_t cmd[6] = { 0x40 + 24, 0, 0, 0, 0, 0xff };
	uint32_t address;
	uint8_t result;
	if (sdc->card_is_sdhc) {
		address = sector;
	} else {
		address = sector << 9;
	}
	cmd[1] = address >> 24;
	cmd[2] = address >> 16;
	cmd[3] = address >> 8;
	cmd[4] = address;
	SDCard_Lock(sdc);
	SDCard_StopMult(sdc);
	result = SDCard_StartCmd(sdc, cmd, 6);
	cnt = SDCard_WriteData(sdc, 0xfe, buf, 512);

	SDCard_Deselect(sdc);
	SDCard_IdleClk(sdc, 1);
	SDCard_Unlock(sdc);
	return cnt;
}

/**
 **************************************************************************************
 * \fn uint16_t SDCard_WriteMultBlock(SDCard *sdc,const uint8_t *buf,uint32_t sector) 
 * Write a block using the multisector state machine. 
 **************************************************************************************
 */
static uint16_t
SDCard_WriteMultBlock(SDCard * sdc, const uint8_t * buf, uint32_t sector)
{
	uint32_t address;
	uint8_t cmd[6] = { 0x40 + 25, 0, 0, 0, 0, 0xff };
	uint16_t cnt = 0;
	SDCard_Lock(sdc);
	if (sdc->state == STATE_MULTSECT_WRITE) {
		if (sector != sdc->next_multsect_block) {
			SDCard_StopMult(sdc);
		}
	} else if (sdc->state != STATE_MULTSECT_IDLE) {
		SDCard_StopMult(sdc);
	}
	switch (sdc->state) {
	    case STATE_MULTSECT_IDLE:
		    if (sdc->card_is_sdhc) {
			    address = sector;
		    } else {
			    address = sector << 9;
		    }
		    cmd[1] = address >> 24;
		    cmd[2] = address >> 16;
		    cmd[3] = address >> 8;
		    cmd[4] = address;
		    if (SDCard_StartCmd(sdc, cmd, 6) == SPIR1_NOREPLY) {
			    Con_Printf("Write Mult %lu failed\n", sector);
			    sdc->status |= SDSTA_NOINIT;
			    break;
		    } else {
			    sdc->state = STATE_MULTSECT_WRITE;
			    sdc->next_multsect_block = sector;
			    sdc->lastMultSectWriteMs = TimeMs_Get();
			    Timer_Start(&sdc->multSectFlushTimer, 400);
			    //Con_Printf("Now in multsect write, next %lu\n",sector);
		    }
		    cnt = SDCard_WriteMultData(sdc, buf, 512);
		    break;

	    case STATE_MULTSECT_WRITE:
		    //Con_Printf("Write follow up block %lu\n",sector);
		    sdc->statMultSectW++;
		    sdc->lastMultSectWriteMs = TimeMs_Get();
		    SDCard_Select(sdc);
		    cnt = SDCard_WriteMultData(sdc, buf, 512);
		    break;
	}
	SDCard_Deselect(sdc);
	SDCard_IdleClk(sdc, 1);
	if (cnt == 512) {
		sdc->next_multsect_block++;
		//Con_Printf("Succ multsect write, next %lu\n",sdc->next_multsect_block);
	}
	SDCard_Unlock(sdc);
	return cnt;

}

/**
 ************************************************
 * Check if sdcard is still plugged.
 ************************************************
 */
static void
SDCard_PluggedTimerProc(void *eventData)
{
	uint8_t plugged = MMC_CardPlugged();
	SDCard *sdc = eventData;
	if (!plugged) {
		sdc->status |= SDSTA_NOINIT;
	}
	Timer_Start(&sdc->pluggedTimer, 517);
}

/**
 ***************************************************************************
 * \fn static void SDCard_MultSectFlush(void *eventData); 
 * Timer handler for checking if the multisector write is open for
 * too long without traffic. If a timeout 
 * occurs the multisector sequence is stopped so that the
 * SDCard will do the write transaction. 
 ***************************************************************************
 */
static void
SDCard_MultSectFlush(void *eventData)
{
	SDCard *sdc = eventData;
	TimeMs_t now;
	if (sdc->state != STATE_MULTSECT_WRITE) {
		return;
	}
	now = TimeMs_Get();
	/* 
	 ****************************************************************
	 * Check if the multisector close timer needs to finish
	 * the multisector write transaction 
	 ****************************************************************
	 */
	if ((now - sdc->lastMultSectWriteMs) >= 400) {
		SDCard_Lock(sdc);
		SDCard_StopMult(sdc);
		SDCard_Unlock(sdc);
	} else {
		Timer_Start(&sdc->multSectFlushTimer, 400);
	}
}

/**
 *****************************************************************
 * Interface functions for FAT fs
 *****************************************************************
 */

uint8_t
MMC_disk_read(uint8_t * buff, uint32_t sector, uint16_t nr_sects)
{
	SDCard *sdc = &g_sdCard;
	uint16_t i;
	uint16_t result;
	for (i = 0; i < nr_sects; i++) {
		result = SDCard_ReadMultBlock(sdc, buff + ((uint32_t) i << 9), sector + i);
		if (result != 512) {
			/* This retry is just for the silly Kingston/Toshiba card */
			result = SDCard_ReadBlock(sdc, buff + ((uint32_t) i << 9), sector + i);
			if (result != 512) {
				Con_Printf("Retry not successful, sect %lu\n",sector);
				return SDRES_ERROR;
			} else  {
				Con_Printf("Retry successful, sect %lu\n",sector);
			}
		}
	}
	return SDRES_OK;
}

/**
 ********************************************************
 * MMC_disk_write
 ********************************************************
 */
uint8_t
MMC_disk_write(const uint8_t * buf, uint32_t sector, uint16_t nr_sects)
{
	SDCard *sdc = &g_sdCard;
	uint16_t i;
	uint16_t result;
	for (i = 0; i < nr_sects; i++) {
		//result = SDCard_WriteBlock(sdc,buf + ((uint32_t)i << 9),sector + i);
		//LCD_Backlight(0);
		result = SDCard_WriteMultBlock(sdc, buf + ((uint32_t) i << 9), sector + i);
		if (result != 512) {
			Con_Printf("Mult Write failed, try single sector %lu\n",sector);
			result = SDCard_WriteBlock(sdc,buf + ((uint32_t)i << 9),sector + i);
			if (result != 512) {
				return SDRES_ERROR;
			}
		}
	}
	return SDRES_OK;
}

/**
 ************************************************************************
 * \fn uint8_t MMC_CardPlugged()
 * Check the card presence switch.
 ************************************************************************
 */
bool
MMC_CardPlugged(void)
{
	return true;
}

/**
 *************************************************
 * \fn uint8_t MMC_disk_initialize
 *************************************************
 */
uint8_t
MMC_disk_initialize(void)
{
	SDCard *sdc = &g_sdCard;
	if (!MMC_CardPlugged()) {
		return SDSTA_NODISK | SDSTA_NOINIT;
	}
	SDCard_Deselect(sdc);
	SDCard_EnableBuffer(sdc);
	if (SDCard_Init(sdc) != true) {
		return SDSTA_NODISK | SDSTA_NOINIT;
	} else {
		return SDSTA_OK;
	}
}

/**
 *****************************************************************
 * \n uint8_t MMC_disk_status(void)
 * Interface proc for getting the disk status.
 *****************************************************************
 */
uint8_t
MMC_disk_status(void)
{
	SDCard *sdc = &g_sdCard;
	MMC_CardPlugged();
	return sdc->status;
}

/**
 ********************************************************
 * MMC ioctl
 ********************************************************
 */
uint8_t
MMC_disk_ioctl(uint8_t ctrl, uint8_t * buf)
{
	SDCard *sdc = &g_sdCard;
	uint8_t result = SDRES_ERROR;
	uint32_t sector_count;
	switch (ctrl) {
	    case SD_CTRL_SYNC:
		    /* Finish open multi sector access */
		    SDCard_Lock(sdc);
		    SDCard_StopMult(sdc);
		    SDCard_Unlock(sdc);
		    result = SDRES_OK;
		    break;

	    case SD_GET_SECTOR_COUNT:
		    if (SDCard_ReadCsd(sdc) != SDRES_OK) {
			    result = SDRES_ERROR;
			    break;
		    }
		    sector_count = csd_get_capacity(sdc->csd) >> 9;
		    *(uint32_t *) buf = sector_count;
		    result = SDRES_OK;
		    break;

	    case SD_GET_SECTOR_SIZE:
		    *(uint16_t *) buf = 512;
		    result = SDRES_OK;
		    break;

	    case SD_GET_BLOCK_SIZE:
		    *(uint32_t *) buf = 1;
		    result = SDRES_OK;
		    break;

	    case SD_CTRL_POWER:
		    if (*buf == 1) {
			    /* Power on */
			    SDCard_Power(sdc, true);
			    result = SDRES_OK;
		    } else if (*buf == 0) {
			    /* Power off */
			    SDCard_Power(sdc, false);
			    result = SDRES_OK;
		    } else if (*buf == 2) {
			    *(buf + 1) = SDCard_GetPower(sdc);
			    result = SDRES_OK;
		    } else {
			    result = SDRES_PARERR;
		    }
		    break;

	    case SD_CTRL_LOCK:
		    break;
	    case SD_CTRL_EJECT:
		    break;
		    /* MMC/SDC command */
	    case SD_GET_TYPE:
		    break;

	    case SD_GET_CSD:
		    if (SDCard_ReadCsd(sdc) != SDRES_OK) {
			    result = SDRES_ERROR;
			    break;
		    }
		    result = SDRES_OK;
		    memcpy(buf, sdc->csd, 16);
		    break;

	    case SD_GET_CID:
		    if (SDCard_ReadCid(sdc) != SDRES_OK) {
			    result = SDRES_ERROR;
			    break;
		    }
		    result = SDRES_OK;
		    memcpy(buf, sdc->cid, 16);
		    break;

	    case SD_GET_OCR:
		    break;

	    case SD_GET_SDSTAT:
		    break;
	}
	return result;
}

/**
 ************************************************************
 * return a text description for the multisector state
 ************************************************************
 */
static const char *
multsect_state_str(uint8_t state)
{
	const char *str;
	switch (state) {
	    case STATE_MULTSECT_IDLE:
		    str = "Idle";
		    break;
	    case STATE_MULTSECT_READ:
		    str = "Read";
		    break;
	    case STATE_MULTSECT_WRITE:
		    str = "Write";
		    break;
	    default:
		    str = "Unknown";
		    break;

	}
	return str;
}

/**
 ******************************************************************
 * Command line interface for reading Vendor information from the
 * SD-Card.
 ******************************************************************
 */
static int8_t
cmd_sdcard(Interp * interp, uint8_t argc, char *argv[])
{
	SDCard *sdc = &g_sdCard;
	int i;
	if ((argc > 1) && (strcmp(argv[1], "info") == 0)) {
		Interp_Printf_P(interp, "CardDetect: %s\n",
				MMC_CardPlugged()? "present" : "not present");
		
		SDCard_Lock(sdc);
		SDCard_StopMult(sdc);
		SDCard_Deselect(sdc);
		SDCard_EnableBuffer(sdc);

		SDCard_Unlock(sdc);
		SDCard_Init(sdc);
		SDCard_ReadCsd(sdc);
		SDCard_ReadCid(sdc);
		csd_decode_current(sdc);
		dump_cardinfo(sdc);
		Interp_Printf_P(interp, "isVersion2.0: %d\n", sdc->card_is_2_0);
		Interp_Printf_P(interp, "isSDHC:       %d\n", sdc->card_is_sdhc);
		Interp_Printf_P(interp, "ReadCurrLV: %d.%1d mA\n",
				sdc->readCurrentLV100uA / 10, sdc->readCurrentLV100uA % 10);
		Interp_Printf_P(interp, "ReadCurrHV: %d.%1d mA\n",
				sdc->readCurrentHV100uA / 10, sdc->readCurrentHV100uA % 10);
		Interp_Printf_P(interp, "WriteCurrLV: %d.%1d mA\n",
				sdc->writeCurrentLV100uA / 10, sdc->writeCurrentLV100uA % 10);
		Interp_Printf_P(interp, "WriteCurrHV: %d.%1d mA\n",
				sdc->writeCurrentHV100uA / 10, sdc->writeCurrentHV100uA % 10);
		Interp_Printf_P(interp,"OCR: 0x%08lx\n",sdc->regOCR);
		for(i = 15; i < 24; i ++) {
			if(sdc->regOCR & (1 << i)) {
				Con_Printf("From %u.%u Volt to ",(27 - 15 + i) / 10,(27 - 15 + i) % 10);
				break;
			}
		}
		for( ; i < 24; i ++) {
			if(!(sdc->regOCR & (1 << i)) || (i == 23)) {
				Con_Printf("to %u.%u Volt\n",(28 - 15 + i) / 10,(28 - 15 + i) % 10);
				break;
			}
		}
	} else {
		Con_Printf("MaxWriteTime:  %lu\n", sdc->statMaxWriteTime);
		Con_Printf("MaxReadTime :  %lu\n", sdc->statMaxReadTime);
		Con_Printf("MultSectRead:  %lu\n", sdc->statMultSectR);
		Con_Printf("MultSectWrit:  %lu\n", sdc->statMultSectW);
		Con_Printf("MultSectState: %s\n", multsect_state_str(sdc->state));
		Con_Printf("ReadErrors:    %lu\n", sdc->statReadErrors);
		Con_Printf("ReadTimeouts:  %lu\n", sdc->statReadTimeouts);
		Con_Printf("WriteErrors:   %lu\n", sdc->statWriteErrors);
		Con_Printf("WriteTimeouts: %lu\n", sdc->statWriteTimeouts);
#if 0
		Con_Printf("Max mVolt:     %lu\n", sdc->statMaxmVolt);
		Con_Printf("Min mVolt:     %lu\n", sdc->statMinmVolt);
#endif
	}
	return 0;
}

INTERP_CMD(sdcardCmd, "sdcard", cmd_sdcard,
	   "sdcard ?info? # statistics / Read information from the SDCard");

/**
 **********************************************************
 * Initialize the SD-Card module. It is called
 * Module Init for discrimination from Initialization
 * of the Card.
 **********************************************************
 */
bool
SDCard_ModuleInit(void)
{
	SDCard *sdc = &g_sdCard;
	sdc->parReadTimeoutMs = 300;
	sdc->parWriteTimeoutMs = 600;
	sdc->status = SDSTA_NOINIT;
	sdc->state = STATE_MULTSECT_IDLE;
	sdc->statMaxmVolt = 0;
	sdc->statMinmVolt = 100000;
#ifdef BOARD_SAKURA 
	BSET(0, PORTC.PDR.BYTE); /* Direction of Chip select to OUT */
#else
	BSET(2, PORT2.PDR.BYTE); /* Direction of Chip select to OUT */
#endif
	SDCard_Power(sdc, true);
	Timer_Init(&sdc->pluggedTimer, SDCard_PluggedTimerProc, sdc);
	Timer_Init(&sdc->multSectFlushTimer, SDCard_MultSectFlush, sdc);
	Interp_RegisterCmd(&sdcardCmd);
	return (MMC_disk_initialize() == SDSTA_OK);
}
