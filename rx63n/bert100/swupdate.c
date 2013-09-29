#include <string.h>
#include <stdlib.h>
#include "dataflash.h"
#include "fat.h"
#include "console.h"
#include "timer.h"
#include "interpreter.h"
#include "hex.h"
#include "version.h"
#include "wdta.h"
#include "swupdate.h"
#include "types.h"
#include "wdta.h"

#define SWUP_CRC32_START        (0x73911293)
#define FLASH_ADDR_SWUPDATE	DFLASH_MAP_ADDR(32768-2048)
#define FLASH_SIZE_SWUPDATE	(2048)

#define POLY 0xEDB88320

/**
 ***************************************************************************************
 * Ethernet CRC calculation. The initial and final XOR by 0xFFFFFFFF is not required
 * because a negated algorithm is used. 
 ***************************************************************************************
 */
static uint32_t
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

static uint32_t
CRC32_Eth(uint32_t crc, uint8_t * buf, uint32_t len)
{
        uint32_t i;
        for (i = 0; i < len; i++) {
                crc = CRC32Byte_Eth(crc, buf[i]);
        }
        return crc;
}

static bool
write_chain_entry(uint16_t entry_nr, uint32_t firstsect, uint32_t nsectors)
{
        bool result;
        uint32_t data[2];
        data[0] = firstsect;
        data[1] = nsectors;
        result = DFlash_Write(FLASH_ADDR_SWUPDATE + ((entry_nr + 1) << 3), data, 8);
	//Con_Printf("from %lu nsectors %lu\n",firstsect,nsectors); 
        return result;
}

/**
 ************************************************************************************
 * Clear the signature of sector chain in Data flash
 ************************************************************************************
 */
static void
ClearUpdateSignature()
{
        bool result;
        uint8_t signature[8];
        if (*(uint32_t *)FLASH_ADDR_SWUPDATE == 0x08154711) {
                Con_Printf("Erasing Software update Signature from Data Flash\n");
                memset(signature, 0xff, 8);
                DFlash_Erase(FLASH_ADDR_SWUPDATE,32);
		Con_Printf("Erased\n");
                result = DFlash_Write(FLASH_ADDR_SWUPDATE + 0, signature, 8);
                if (result == false) {
                        Con_Printf("failed\n");
                }
		Con_Printf("written\n");
        } else {
		Con_Printf("nothing to clear\n");
	}
}

static bool
store_sector_chain(const char *filename)
{
        FIL file;
        FRESULT res;
        DWORD clust;
        uint32_t sect, firstsect;
        uint32_t nextsect;
        uint32_t nsectors;
        uint32_t cluster_size;
        bool result = true;
        uint16_t entry_nr = 0;
        uint32_t i;
        uint32_t signature[2];
        res = f_open(&file, filename, FA_OPEN_EXISTING | FA_READ);
        if (res != FR_OK) {
                Con_Printf("Can not open file \"%s\"\n", filename);
                return false;
        }
        if (DFlash_Erase(FLASH_ADDR_SWUPDATE,FLASH_SIZE_SWUPDATE) == false) {
                return false;
        }
	Con_Printf("Flash erase was successful\n");
        clust = file.sclust;
        cluster_size = fat_clustersize(file.fs);
        if (cluster_size == 0) {
                Con_Printf("Cluster size is 0\n");
                return false;
        }
        firstsect = clust2sect(file.fs, clust);
        nextsect = firstsect;
        nsectors = 0;
	for (i = 1; i < 2049; i++) {
                sect = clust2sect(file.fs, clust);
                if (sect == 0) {
                        break;
                }
                if (sect == nextsect) {
                        nsectors += cluster_size;
                        nextsect += cluster_size;
                } else {
                        result = write_chain_entry(entry_nr, firstsect, nsectors);
                        if (result == false) {
                                Con_Printf("Write failed\n");
                                return false;
                        }
                        firstsect = sect;
                        nextsect = sect + cluster_size;
                        nsectors = cluster_size;
                        entry_nr++;
                        if (entry_nr > 250) {
                                Con_Printf("File is too fragmented\n");
                                return false;
                        }
                }
                clust = get_fat(file.fs, clust);
        }
        if (nsectors) {
                result = write_chain_entry(entry_nr, firstsect, nsectors);
                if (result == false) {
                        Con_Printf("Write failed\n");
                        return false;
                }
                entry_nr++;
        }
        result = write_chain_entry(entry_nr, 0, 0);
        signature[0] = signature[1] = 0x08154711;
        result = DFlash_Write(FLASH_ADDR_SWUPDATE + 0, signature, 8);
        f_close(&file);
        return result;
}

/*
 *********************************************************************************************
 * \fn static bool CheckWriteSignature(const char *filename, bool checkOnly)
 * Check or write the signature of a Software update file
 *********************************************************************************************
 */
static bool
CheckWriteSignature(const char *filename, bool checkOnly)
{
        FRESULT res;
	FIL infile;
        unsigned int i;
        uint8_t buf[32];
        uint32_t crc32 =  SWUP_CRC32_START;
        UINT sz;
        res = f_open(&infile, filename, FA_OPEN_EXISTING | FA_READ | FA_WRITE);
        if (res != FR_OK) {
                Con_Printf("Can't open file\n");
                return 0;
        }
        for (i = 0; i < 0xFF000; i += sizeof(buf)) {
                res = f_read(&infile, buf, sizeof(buf), &sz);
                if (sz != sizeof(buf)) {
                        Con_Printf("Read failed\n");
                        return 0;
                }
                crc32 = CRC32_Eth(crc32, buf, sz);
        }
        if (checkOnly) {
                if (f_read(&infile, buf, 4, &sz) != FR_OK) {
                        return false;
                }
                f_close(&infile);
                crc32 = CRC32_Eth(crc32, buf, 4);
                if (crc32 == 0xffffffff) {
                        return true;
                } else {
                        return false;
                }
        } else {
                buf[0] = ~crc32 & 0xff;
                buf[1] = (~crc32 >> 8) & 0xff;
                buf[2] = (~crc32 >> 16) & 0xff;
                buf[3] = (~crc32 >> 24) & 0xff;
                if (f_write(&infile, buf, 4, &sz) != FR_OK) {
                        Con_Printf("Write failed\n");
                }
                f_close(&infile);
        }
        return true;
}


uint8_t 
SWUpdate_Execute(const char *filepath)
{
        if (CheckWriteSignature(filepath, true) == false) {
                Con_Printf("No vailid checksum found in %s\n", filepath);
                return SWUP_EC_CRC;
        }
        if (store_sector_chain(filepath) != true) {
                Con_Printf("Can not create sector chain in dataflash\n");
        } else {
                Con_Printf("Software Update is valid, Updating now !\n");
		SleepMs(10);
	}
        DISABLE_IRQ();
        WDTA_Reboot();
	return 0;
}

/**
 ***********************************************************************************
 * \fn static int8_t cmd_chain(Interp *interp,uint8_t argc,char *argv[]) 
 * Shell command for creating the sector chain in data flash.
 ***********************************************************************************
 */
static int8_t
cmd_chain(Interp * interp, uint8_t argc, char *argv[])
{
        char *filename;
        bool result;
        unsigned int i;
        if (argc == 2) {
                filename = argv[1];
                result = store_sector_chain(filename);
                if (result == false) {
                        Con_Printf("failed\n");
                        return 0;
                }
        }
        for (i = 0; i < 32; i++) {
                uint32_t dword = *(uint32_t *) (FLASH_ADDR_SWUPDATE + (i << 2));
                Con_Printf("%08lx ", dword);
                if ((i % 8) == 7) {
                        Con_Printf("\n", dword);
                }
        }
        return 0;
}

INTERP_CMD(chainCmd, "chain", cmd_chain, "chain <filename>  # store a cluster chain in data flash");

static int8_t
cmd_swupdate(Interp * interp, uint8_t argc, char *argv[])
{
	if(argc < 2) {
		return -EC_BADNUMARGS;
	}
	SWUpdate_Execute(argv[1]);
        return 0;
}

INTERP_CMD(swupdateCmd, "swupdate", cmd_swupdate, "swupdate <filename>  # Update the software");

/**
 *****************************************************************************
 * Initialize the Software updater
 *****************************************************************************
 */
void
SWUpdate_Init(void)
{
        ClearUpdateSignature();
        Interp_RegisterCmd(&swupdateCmd);
        Interp_RegisterCmd(&chainCmd);
}

