/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2007        */
/*-----------------------------------------------------------------------*/
/* This is a stub disk I/O module that acts as front end of the existing */
/* disk I/O modules and attach it to FatFs module with common interface. */
/*-----------------------------------------------------------------------*/

#include "diskio.h"
#include "sdcard.h"
//include "rtc.h"
#include "console.h"

/*-----------------------------------------------------------------------*/
/* Correspondence between physical drive number and physical drive.      */
/*-----------------------------------------------------------------------*/

#define MMC			0
#define SPIDRV		1
#define USB			2

static DRESULT
translate_result(uint8_t sdresult)
{
	switch (sdresult) {
	    case SDRES_OK:
		    return RES_OK;
	    case SDRES_ERROR:
		    return RES_ERROR;
	    case SDRES_WRPRT:
		    return RES_WRPRT;
	    case SDRES_NOTRDY:
		    return RES_NOTRDY;
	    case SDRES_PARERR:
		    return RES_PARERR;
	    default:
		    return RES_ERROR;
	}
}

static DRESULT
translate_ioctl(uint8_t ioctl)
{
	switch (ioctl) {
	    case CTRL_SYNC:
		    return SD_CTRL_SYNC;
	    case GET_SECTOR_COUNT:
		    return SD_GET_SECTOR_COUNT;
	    case GET_SECTOR_SIZE:
		    return SD_GET_SECTOR_SIZE;
	    case GET_BLOCK_SIZE:
		    return SD_GET_BLOCK_SIZE;
	    case CTRL_POWER:
		    return SD_CTRL_POWER;
	    case CTRL_LOCK:
		    return SD_CTRL_LOCK;
	    case CTRL_EJECT:
		    return SD_CTRL_EJECT;

	    case MMC_GET_TYPE:
		    return SD_GET_TYPE;
	    case MMC_GET_CSD:
		    return SD_GET_CSD;
	    case MMC_GET_CID:
		    return SD_GET_CID;
	    case MMC_GET_OCR:
		    return SD_GET_OCR;
	    case MMC_GET_SDSTAT:
		    return SD_GET_SDSTAT;
	    default:
		    return ioctl;
	}
}

DSTATUS
translate_status(uint8_t sdsta)
{
	DSTATUS stat = 0;
	if (sdsta & SDSTA_NOINIT) {
		stat |= STA_NOINIT;
	}
	if (sdsta & SDSTA_NODISK) {
		stat |= STA_NODISK;
	}
	if (sdsta & SDSTA_PROTECT) {
		stat |= STA_PROTECT;
	}
	return stat;
}

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS
disk_initialize(BYTE drv	/* Physical drive nmuber (0..) */
    )
{
	DSTATUS stat;
	int result;

	switch (drv) {
	    case MMC:
		    result = MMC_disk_initialize();
		    stat = translate_status(result);
		    return stat;
	    case SPIDRV:
		    stat = 0;
		    return stat;

	}
	return STA_NOINIT;
}

/*-----------------------------------------------------------------------*/
/* Return Disk Status                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS
disk_status(BYTE drv		/* Physical drive nmuber (0..) */
    )
{
	DSTATUS stat;
	int result;

	switch (drv) {
	    case MMC:
		    result = MMC_disk_status();
		    stat = translate_status(result);
		    return stat;
	    case SPIDRV:
		    stat = 0;
		    return stat;

	    default:
		    Con_Printf("Unknown drive %d\n", drv);
		    return STA_NOINIT;
	}
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT
disk_read(BYTE drv,		/* Physical drive nmuber (0..) */
	  BYTE * buff,		/* Data buffer to store read data */
	  DWORD sector,		/* Sector address (LBA) */
	  BYTE count		/* Number of sectors to read (1..255) */
    )
{
	DRESULT res;
	uint8_t result;
	switch (drv) {
	    case MMC:
		    /* translate the result code here */
		    res = MMC_disk_read(buff, sector, count);
		    result = translate_result(res);
		    return res;
	    case SPIDRV:
		    break;
	}
	return RES_PARERR;
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/
/* The FatFs module will issue multiple sector transfer request
/  (count > 1) to the disk I/O layer. The disk function should process
/  the multiple sector transfer properly Do. not translate it into
/  multiple single sector transfers to the media, or the data read/write
/  performance may be drasticaly decreased. */

#if _READONLY == 0
DRESULT
disk_write(BYTE drv,		/* Physical drive nmuber (0..) */
	   const BYTE * buff,	/* Data to be written */
	   DWORD sector,	/* Sector address (LBA) */
	   BYTE count		/* Number of sectors to write (1..255) */
    )
{
	DRESULT res;
	uint8_t result;
	switch (drv) {
	    case MMC:
		    result = MMC_disk_write(buff, sector, count);
		    res = translate_result(result);
		    return res;

	    case SPIDRV:
		    break;

	}
	return RES_PARERR;
}
#endif				/* _READONLY */

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT
disk_ioctl(BYTE drv,		/* Physical drive nmuber (0..) */
	   BYTE ctrl,		/* Control code */
	   void *buff		/* Buffer to send/receive control data */
    )
{
	DRESULT res;
	int result;
	uint8_t tctrl;
	switch (drv) {
	    case MMC:
		    tctrl = translate_ioctl(ctrl);	/* Translate the control */
		    result = MMC_disk_ioctl(tctrl, buff);
		    res = translate_result(result);
		    return res;

	    case SPIDRV:
		break;
	}
	return RES_PARERR;
}

/**
 *********************************************************
 * DWORD get_fattime(void)
 *********************************************************
 */

DWORD
get_fattime(void)
{
	//RTime rtc;
	DWORD fattime;
//	if (RTime_Get(&rtc) != RTIME_OK) {
		/* Some day in the year 2012 */
		return 0x40e35564;
//	}
	/* Pack date and time into a DWORD variable */
#if 0
	fattime = ((DWORD) (rtc.year - 1980) << 25)
	    | ((DWORD) rtc.month << 21)
	    | ((DWORD) rtc.mday << 16)
	    | ((DWORD) rtc.hour << 11)
	    | ((DWORD) rtc.min << 5)
	    | ((DWORD) rtc.sec >> 1);
	return fattime;
#endif
}
