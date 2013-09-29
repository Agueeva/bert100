/*
 *************************************************************************************************************
 * Data flash access routines for the Rensas RX63N
 *************************************************************************************************************
 */
#include <string.h>
#include "iodefine.h"
#include "atomic.h"
#include "tpos.h"
#include "events.h"
#include "timer.h"
#include "interpreter.h"
#include "hex.h"
#include "dataflash.h"
#include "console.h"
#include "fat.h"

#define DFLASH_BLOCK(n)         (0x100000 + ((n) << 3))

#define FCU_PRG_TOP             0xFEFFE000
#define FCU_RAM_TOP             0x007F8000
#define FCU_RAM_SIZE    	0x2000

#define READ_MODE                       0
#define ROM_PE_MODE                     1
#define FLD_PE_MODE                     2

#define DF_ADDRESS      0x00100000

#define TDE32_MAX_MS		20	/*< Max Erase time for 32 Byte block in Data flash */
#define TDP2_MAX_MS		2	/*< Max Programming time for 2 Bytes in Data flash */

#define WAIT_MAX_ERASE          1000000 // 70ms, max 250ms
#define WAIT_MAX_ROM_WRITE      1000000
#define WAIT_MAX_BLANK_CHECK2   1000    // 30us
#define WAIT_MAX_BLANK_CHECK2K  100000  // 0.7ms
#define WAIT_t10USEC 		2000
#define PCLK_FREQUENCY 48

static bool notify_peripheral_clock_done = false;
static Mutex flashSema;

/**
 ***********************************************************
 * Reset the FCU
 ***********************************************************
 */
static void
Flash_Reset(void)
{
        volatile uint32_t wait_cnt;
        /* Reset the FCU */
        FLASH.FRESETR.WORD = 0xCC01;
        /* Give FCU time to reset */
        wait_cnt = 10000;       /* 35 uS */
        while (wait_cnt != 0) {
                wait_cnt--;
        }
        /* FCU is not reset anymore */
        FLASH.FRESETR.WORD = 0xCC00;
}

/**
 *********************************************************************
 * \fn static void DataFlash_StatusClr(void)
 * Clear the status of the previous data flash operation.
 * Used before a new operation.
 *********************************************************************
 */
static void
DataFlash_StatusClr(void)
{
        volatile uint8_t *ptrb;
        ptrb = (uint8_t *) DF_ADDRESS;
        if (FLASH.FSTATR0.BIT.ILGLERR == 1) {
                if (FLASH.FASTAT.BYTE != 0x10) {
                        FLASH.FASTAT.BYTE = 0x10;
                }
        }
        *ptrb = 0x50;
}

/**
 ********************************************************************************
 * \fn static bool Notify_Peripheral_Clock(volatile uint8_t * flash_addr)
 * Notify the FCU about the Peripheral clock
 ********************************************************************************
 */
static bool
Notify_Peripheral_Clock(volatile uint8_t * flash_addr)
{
        /* Declare wait counter variable */
        volatile uint32_t wait_cnt;
        /* Set frequency of PCK */
        FLASH.PCKAR.WORD = PCLK_FREQUENCY;
        /* Execute Peripheral Clock Notification Commands */
        *flash_addr = 0xE9;
        *flash_addr = 0x03;
        *(volatile uint16_t *)flash_addr = 0x0F0F;
        barrier(); // For buggy gcc 4.6.0 only
        *(volatile uint16_t *)flash_addr = 0x0F0F;
        barrier();
        *(volatile uint16_t *)flash_addr = 0x0F0F;
        *flash_addr = 0xD0;

        /* Set timeout wait duration */
        wait_cnt = WAIT_t10USEC;

        /* Check FRDY */
        while (FLASH.FSTATR0.BIT.FRDY == 0) {
                wait_cnt--;
                if (wait_cnt == 0) {
                        /* Timeout duration elapsed, assuming operation failure - Reset FCU */
                        Con_Printf("Notify timeout\n");
                        Con_Printf("FSTATR0 %02x FASTAT %02x\n", FLASH.FSTATR0.BYTE,
                                   FLASH.FASTAT.BYTE);
                        Flash_Reset();
                        /* Return 1, operation failure  */
                        return false;
                }
        }
        /* Check ILGLERR */
        if (FLASH.FSTATR0.BIT.ILGLERR == 1) {
                return false;
        }
        return true;
}

/**
 **************************************************************
 * \fn static void Exit_PEMode(uint8_t current_mode)
 **************************************************************
 */
static void
Exit_PEMode(uint8_t current_mode)
{
        uint32_t wait_cnt;
        volatile uint8_t *pAddr;
        wait_cnt = WAIT_MAX_ERASE;
        while (FLASH.FSTATR0.BIT.FRDY == 0) {
                wait_cnt--;
                if (wait_cnt == 0) {
                        Flash_Reset();
                }
                break;
        }
        if ((FLASH.FSTATR0.BIT.ILGLERR == 1)
            || (FLASH.FSTATR0.BIT.ERSERR == 1)
            || (FLASH.FSTATR0.BIT.PRGERR == 1)) {
                if (FLASH.FSTATR0.BIT.ILGLERR == 1) {
                        if (FLASH.FASTAT.BYTE != 0x10) {
                                FLASH.FASTAT.BYTE = 0x10;
                        }
                }
                if (current_mode == ROM_PE_MODE) {
                        //pAddr = (uint8_t *) ROM_PE_ADDR;
                        //*pAddr = 0x50;  /* clear status command */
                } else {
                        pAddr = (uint8_t *) DF_ADDRESS;
                        *pAddr = 0x50;
                }
        }
        FLASH.FENTRYR.WORD = 0xAA00;
		FLASH.FWEPROR.BYTE = 0x02;
}

/*
 ************************************************************************************
 * \fn static bool Enter_PEMode(uint8_t mode,uint32_t flash_addr,uint16_t bytes) 
 ************************************************************************************
 */
static bool
Enter_PEMode(uint8_t mode, volatile uint8_t * pAddr, uint16_t bytes)
{
        FLASH.FENTRYR.WORD = 0xAA00;
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
        if (mode == FLD_PE_MODE) {
                FLASH.FENTRYR.WORD = 0xaa80;
                DataFlash_StatusClr();
        } else {
                Con_Printf("Illegal mode %d\n", mode);
                return false;
        }
        FLASH.FWEPROR.BYTE = 0x01;
        if ((FLASH.FSTATR0.BIT.ILGLERR == 1)
            || (FLASH.FSTATR0.BIT.ERSERR == 1)
            || (FLASH.FSTATR0.BIT.PRGERR == 1)
            || (FLASH.FSTATR1.BIT.FCUERR == 1)) {
                return false;
        }
        if (notify_peripheral_clock_done == false) {
                if (Notify_Peripheral_Clock(pAddr) == false) {
                        Con_Printf("Notify pclk failed\n");
                        return false;
                } else {
                        notify_peripheral_clock_done = true;
                }
        }
        return true;
}

/**
 *********************************************************************
 * Erase a single 32 Byte flash block from Data flash.
 *********************************************************************
 */
static bool
DFlash_EraseBlock(uint32_t block_addr)
{
        uint8_t mode;
        TimeMs_t startEraseMs;
        bool retval = true;
        volatile uint8_t *pAddr = (volatile uint8_t *)(block_addr & 0x00FFFFFF);

	//Con_Printf("EraseBlock %08lx, %08lx\n",block_addr,pAddr);
        Mutex_Lock(&flashSema);
        mode = FLD_PE_MODE;
        if (Enter_PEMode(mode, pAddr, 0) == false) {
                Con_Printf("enter PE mode %d failed\n", mode);
                retval = false;
                goto done;
        }
        FLASH.FPROTR.WORD = 0x5501;
        *pAddr = 0x20;
        *pAddr = 0xd0;
        startEraseMs = TimeMs_Get();
        while (FLASH.FSTATR0.BIT.FRDY == 0) {
                if ((TimeMs_Get() - startEraseMs) > (TDE32_MAX_MS + 1)) {
                        Flash_Reset();
                        retval = false;
                        goto done;
                }
                EV_Yield();
        }
        if ((FLASH.FSTATR0.BIT.ILGLERR == 1) || (FLASH.FSTATR0.BIT.ERSERR == 1)) {
                retval = false;
        }
        Exit_PEMode(mode);
 done:
        Mutex_Unlock(&flashSema);
        return retval;
}

/**
 ******************************************************************************
 ******************************************************************************
 */
bool
DFlash_Erase(uint32_t relAddr,uint32_t len)
{
	bool retval = true;
	uint32_t i;
	if((relAddr & 31) || (len & 31)) {
		Con_Printf("Wrong alignment in DFlash erase\n");
		return false;
	}
	for(i = 0;i < len; i+= 32) {
		retval = DFlash_EraseBlock(relAddr+ i);
		if(retval == false) {
			return retval;
		}
	}
	return retval;
}

/**
 *****************************************************************************
 * \fn bool DFlash_BlankCheck8(uint32_t addr) 
 * returns true if 2 Bytes are blank 
 *****************************************************************************
 */
bool
DFlash_BlankCheck2(uint32_t addr)
{
        uint32_t wait_cnt;
        uint8_t mode = FLD_PE_MODE;
        volatile uint8_t *pAddr = (uint8_t *) addr;
        bool retval, result;
        Mutex_Lock(&flashSema);
        if (Enter_PEMode(mode, (uint8_t *) addr, 0) == false) {
                Con_Printf("enter PE mode %d failed\n", mode);
                retval = false;
                goto done;
        }
        FLASH.FMODR.BIT.FRDMD = 1;
        FLASH.DFLBCCNT.BIT.BCSIZE = 0;
        FLASH.DFLBCCNT.BIT.BCADR = addr  & 0x7ff;
        *pAddr = 0x71;
        *pAddr = 0xd0;
        wait_cnt = WAIT_MAX_BLANK_CHECK2;
        while (FLASH.FSTATR0.BIT.FRDY == 0) {
                wait_cnt--;
                if (wait_cnt == 0) {
                        Flash_Reset();
                        retval = false;
                        goto done;
                }
        }
        result = FLASH.DFLBCSTAT.BIT.BCST;
        FLASH.FMODR.BIT.FRDMD = 0;
        Exit_PEMode(mode);
        retval = !result;
 done:
        Mutex_Unlock(&flashSema);
        return retval;;
}


/**
 *********************************************************************************
 * \fn static bool DataFlash_Write(uint32_t address,uint8_t *data) 
 *********************************************************************************
 */
static bool
_DataFlash_Write(uint32_t address, uint8_t * data)
{
        TimeMs_t startTimeMs;
        *(volatile uint8_t *)DF_ADDRESS = 0xE8;
        *(volatile uint8_t *)DF_ADDRESS = 1;
	*(volatile uint16_t *)address = *(uint16_t *) data;
        *(volatile uint8_t *)(DF_ADDRESS) = 0xD0;
        startTimeMs = TimeMs_Get();
        while (FLASH.FSTATR0.BIT.FRDY == 0) {
                if ((TimeMs_Get() - startTimeMs) > TDP2_MAX_MS) {
                        Flash_Reset();
                        Con_Printf("Data flash write timeout\n");
                        return false;
                }
                if ((FLASH.FSTATR0.BIT.ILGLERR == 1) || (FLASH.FSTATR0.BIT.PRGERR == 1)) {
                        Con_Printf("Bad status %02x, FASTAT %02x\n",
                                   FLASH.FSTATR0.BYTE, FLASH.FASTAT.BYTE);
                        return false;
                }
                EV_Yield();
        }
        return true;
}

/**
 ************************************************************************
 * \fn bool DFlash_Write(uint32_t flash_addr,uint8_t *buf,uint32_t len)
 ************************************************************************
 */
bool
DFlash_Write(uint32_t flash_addr, void *buf, uint32_t len)
{
        uint8_t mode;
        bool retval = true;
        Mutex_Lock(&flashSema);
        mode = FLD_PE_MODE;
        flash_addr &= 0x00FFFFFF;
        if (Enter_PEMode(mode, (uint8_t *) flash_addr, len) == false) {
                Con_Printf("Enter PE mode %d failed\n", mode);
                retval = false;
                goto done;
        }
        FLASH.FPROTR.WORD = 0x5501;
        while (len >= 2) {
                retval = _DataFlash_Write(flash_addr, buf);
                if (retval == false) {
                        Con_Printf("DataFlash_Write failed\n");
                        break;
                }
                flash_addr += 2;
                buf += 2;
                len -= 2;
        }
        Exit_PEMode(mode);
 done:
        Mutex_Unlock(&flashSema);
        return retval;
}


/*
 ***************************************************************************************
 * \fn static int8_t cmd_flash(Interp * interp, uint8_t argc, char *argv[])
 ***************************************************************************************
 */
static int8_t
cmd_flash(Interp * interp, uint8_t argc, char *argv[])
{
        uint32_t i;
        uint8_t data[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x8 };
        Con_Printf("DB0:  Result: %d\n", DFlash_EraseBlock(DFLASH_BLOCK(0)));
        for (i = 0; i < 8; i++) {
                Con_Printf("%02x ", *(uint8_t *) (DFLASH_BLOCK(0) + i));
        }
        Con_Printf("\n");
	SleepMs(1000);
        Con_Printf("BlankCheck %u\n", DFlash_BlankCheck2(DFLASH_BLOCK(0)));
        Con_Printf("DF-Write: %d\n", DFlash_Write(DFLASH_BLOCK(0), data, 8));
        for (i = 0; i < 8; i++) {
                Con_Printf("%02x ", *(uint8_t *) (DFLASH_BLOCK(0) + i));
        }
        Con_Printf("\n");
        Con_Printf("BlankCheck %u\n", DFlash_BlankCheck2(DFLASH_BLOCK(0)));
        return 0;
}

INTERP_CMD(flashCmd, "flash", cmd_flash, "flash  # flash");

/**
 ********************************************************************************
 * The FCU is a microengine executing code in the FCU RAM. This proc copies
 * the code from the builtin ROM to the FCU-RAM area.
 ********************************************************************************
 */
static void
copy_to_fcu_ram(void)
{
        uint32_t *src, *dst;
        uint16_t i;
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

        /* Iterate for loop to copy the FCU firmware */
        for (i = 0; i < (FCU_RAM_SIZE / 4); i++) {
                *dst = *src;
                src++;
                dst++;
        }
        return;
}

/**
 ********************************************************
 * \fn void Flash_Init(void) 
 ********************************************************
 */
void
DataFlash_Init(void)
{
        uint16_t read_en_mask = 0xffff;
        uint16_t write_en_mask = 0xffff;
        Mutex_Init(&flashSema);
        copy_to_fcu_ram();
        /* Set Read access for the Data Flash blocks DB0-DB7 */
        FLASH.DFLRE0.WORD = 0x2D00 | (read_en_mask & 0x00FF);
        /* Set Read access for the Data Flash blocks DB8-DB15 */
        FLASH.DFLRE1.WORD = 0xD200 | ((read_en_mask >> 8) & 0x00FF);
        /* Set Erase/Program access for the Data Flash blocks DB0-DB7 */
        FLASH.DFLWE0.WORD = 0x1E00 | (write_en_mask & 0x00FF);
        /* Set Erase/Program access for the Data Flash blocks DB8-DB15 */
        FLASH.DFLWE1.WORD = 0xE100 | ((write_en_mask >> 8) & 0x00FF);
	Interp_RegisterCmd(&flashCmd);
}
