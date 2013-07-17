#include "types.h"
typedef enum {
	SDRES_OK = 0,		/* 0: Successful */
	SDRES_ERROR,		/* 1: R/W Error */
	SDRES_WRPRT,		/* 2: Write Protected */
	SDRES_NOTRDY,		/* 3: Not Ready */
	SDRES_PARERR		/* 4: Invalid Parameter */
} SDResult;

#define SDSTA_OK		0
#define SDSTA_NOINIT		0x01	/* Drive not initialized */
#define SDSTA_NODISK		0x02	/* No medium in the drive */
#define SDSTA_PROTECT		0x04	/* Write protected */

/*
 * IOCTRL
 */
#define SD_CTRL_SYNC			0	/* Mandatory for write functions */
#define SD_GET_SECTOR_COUNT		1	/* Mandatory for only f_mkfs() */
#define SD_GET_SECTOR_SIZE		2	/* Mandatory for multiple sector size cfg */
#define SD_GET_BLOCK_SIZE		3	/* Mandatory for only f_mkfs() */
#define SD_CTRL_POWER			4
#define SD_CTRL_LOCK			5
#define SD_CTRL_EJECT			6
/* MMC/SDC command */
#define SD_GET_TYPE			10
#define SD_GET_CSD			11
#define SD_GET_CID			12
#define SD_GET_OCR			13
#define SD_GET_SDSTAT		14

bool SDCard_ModuleInit(void);
bool MMC_CardPlugged(void);
uint8_t MMC_disk_read(uint8_t * buff, uint32_t sector, uint16_t nr_sects);
uint8_t MMC_disk_write(const uint8_t * buf, uint32_t sector, uint16_t nr_sects);
uint8_t MMC_disk_initialize(void);
uint8_t MMC_disk_status(void);
uint8_t MMC_disk_ioctl(uint8_t ctrl, uint8_t * buf);
