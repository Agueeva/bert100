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

#define FLASH_ADDR_SWUPDATE	(32768-2048)
#define FLASH_SIZE_SWUPDATE	(2048)

static bool
write_chain_entry(uint16_t entry_nr, uint32_t firstsect, uint32_t nsectors)
{
        bool result;
        uint32_t data[2];
        data[0] = firstsect;
        data[1] = nsectors;
        result = DFlash_Write(FLASH_ADDR_SWUPDATE + ((entry_nr + 1) << 3), data, 8);
        return result;
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
        if ((filename[0] != '0') || (filename[1] != ':')) {
                Con_Printf("Wrong drive\n");
                return false;
        }
        res = f_open(&file, filename, FA_OPEN_EXISTING | FA_READ);
        if (res != FR_OK) {
                Con_Printf("Can not open file \"%s\"\n", filename);
                return false;
        }
        if (DFlash_Erase(FLASH_ADDR_SWUPDATE,FLASH_SIZE_SWUPDATE) == false) {
                return false;
        }
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

