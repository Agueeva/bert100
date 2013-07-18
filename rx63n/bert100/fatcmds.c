/**
 *****************************************************************************************
 * Command shell interface to the FAT.
 *****************************************************************************************
 */
#include "fat.h"
#include "fatcmds.h"
#include "interpreter.h"
#include "hex.h"
#include "diskio.h"
#include <string.h>

static void
File_WriteChar(void *cbData, uint8_t c)
{
	FIL *file = cbData;
	f_putc(c, file);
}

void
F_Printf(FIL * file, const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	Con_PrintToVA(File_WriteChar, file, format, ap);
	va_end(ap);
}


char *
f_errstr(FRESULT res)
{
	char *str;
	switch (res) {
	    case FR_DISK_ERR:	/* (1) A hard error occured in the low level disk I/O layer */
		    str = "DiskErr";
		    break;

	    case FR_INT_ERR:	/* (2) Assertion failed */
		    str = "IntErr";
		    break;

	    case FR_NOT_READY:	/* (3) The physical drive cannot work */
		    str = "NotRdy";
		    break;

	    case FR_NO_FILE:	/* (4) Could not find the file */
		    str = "NoFile";
		    break;

	    case FR_NO_PATH:	/* (5) Could not find the path */
		    str = "NoPath";
		    break;

	    case FR_INVALID_NAME:	/* (6) The path name format is invalid */
		    str = "InvName";
		    break;

	    case FR_DENIED:	/* (7) Acces denied due to prohibited access or directory full */
		    str = "Denied";
		    break;

	    case FR_EXIST:	/* (8) Acces denied due to prohibited access */
		    str = "Exist";
		    break;

	    case FR_INVALID_OBJECT:	/* (9) The file/directory object is invalid */
		    str = "InvObj";
		    break;

	    case FR_WRITE_PROTECTED:	/* (10) The physical drive is write protected */
		    str = "WProt";
		    break;

	    case FR_INVALID_DRIVE:	/* (11) The logical drive number is invalid */
		    str = "InvDrv";
		    break;

	    case FR_NOT_ENABLED:	/* (12) The volume has no work area */
		    str = "Disab";
		    break;

	    case FR_NO_FILESYSTEM:	/* (13) There is no valid FAT volume on the physical drive */
		    str = "NoFS";
		    break;

	    case FR_MKFS_ABORTED:	/* (14) The f_mkfs() aborted due to any parameter error */
		    str = "MkfsAb";
		    break;

	    case FR_TIMEOUT:	/* (15) Could not get a grant to access the volume within defined period */
		    str = "Timeout";
		    break;

	    case FR_LOCKED:	/* (16) The operation is rejected according to the file shareing policy */
		    str = "Locked";
		    break;

	    case FR_NOT_ENOUGH_CORE:	/* (17) LFN working buffer could not be allocated */
		    str = "NEnCor";
		    break;

	    case FR_TOO_MANY_OPEN_FILES:	/* (18) Number of open files > _FS_SHARE */
		    str = "ToMany";
		    break;

	    default:
		    Con_Printf("Unexpected FAT result %ld\n", res);
		    str = "Unknown";
		    break;
	}
	return str;
}

FATFS f_fatfs[2];
static char lfn[_MAX_LFN * (_DF1S ? 2 : 1) + 1];

/**
 **********************************************************************************
 * \fn static int8_t cmd_fatchain(Interp *interp,uint8_t argc,char *argv[]); 
 * Show the cluster chain of a file;
 **********************************************************************************
 */
static int8_t
cmd_fatchain(Interp * interp, uint8_t argc, char *argv[])
{
	FIL file;
	FRESULT res;
	DWORD clust;
	DWORD sect;
	uint32_t i;
	if (argc < 2) {
		return -EC_BADNUMARGS;
	}
	res = f_open(&file, argv[1], FA_OPEN_EXISTING | FA_READ);
	if (res != FR_OK) {
		Con_Printf("Can not open file \"%s\"\n", argv[1]);
		return 0;
	}
	clust = file.sclust;
	for (i = 0; i < 1000; i++) {
		sect = clust2sect(file.fs, clust);
		if (sect) {
			Con_Printf("clust %lu, sect %lu\n", clust, sect);
		} else {
			break;
		}
		clust = get_fat(file.fs, clust);
	}
	f_close(&file);
	return 0;
}

INTERP_CMD(fatchainCmd, "fatchain", cmd_fatchain,
	   "fatchain <fname> # Show the FAT cluster chain of a file");


#if 0
/**
 ***********************************************************************
 * \fn static int8_t cmd_gc(Interp * interp, uint8_t argc, char *argv[])
 * Garbage collector walks through FAT and TRIMS unused clusters. 
 ***********************************************************************
 */
static int8_t
cmd_gc(Interp * interp, uint8_t argc, char *argv[])
{
	uint32_t i;
	FATFS *fs = &f_fatfs[0];
	DWORD fatent, sect;
	uint32_t trim_args[2];
	if (fs->csize != 8) {
		Con_Printf("Unexpected cluster size %u\n", fs->csize);
		return 0;
	}
	Con_Printf("%04u: ", 2);
	for (i = 2; i < fs->n_fatent; i++) {
		lock_fs(fs);
		fatent = get_fat(fs, i);
		sect = clust2sect(fs, i);
		if ((sect > 0x70) && (fatent == 0)) {
			trim_args[0] = sect;
			trim_args[1] = fs->csize;
			disk_ioctl(fs->drv, CTRL_TRIM, trim_args);
			//Con_Printf("Erased %03lx %04lx\n",fatent,sect);
		}
		unlock_fs(fs, FR_OK);
		EV_Yield();
	}
	Con_Printf("\n");
	return 0;
}

INTERP_CMD(gcCmd, "gc", cmd_gc, "gc # Run fat Garbage collector for drive 0");
#endif

/**
 ******************************************************************************
 * \fn static int8_t cmd_fatls(interp, argc, argv);
 ******************************************************************************
 */
static int8_t
cmd_fatls(Interp * interp, uint8_t argc, char *argv[])
{
	int i;
	DRESULT res;
	FILINFO Finfo;
	FATFS *fs;
	DIR dir;
	DWORD free_clust;
	uint32_t free_sect;
	uint64_t free_bytes;
	char sizestr[22];
	int sizelen;

	Finfo.lfname = lfn;
	Finfo.lfsize = sizeof(lfn);
	if (argc < 2) {
		res = f_opendir(&dir, "0:");
	} else {
		res = f_opendir(&dir, argv[1]);
	}
	if (res) {
		Con_Printf("Error %d opening dir\n", res);
		return 0;
	}
	for (i = 0; i < 1000; i++) {
		res = f_readdir(&dir, &Finfo);
		if (res || !Finfo.fname[0]) {
			break;
		}
		Con_Printf("%c%c%c%c%c %u/%02u/%02u %02u:%02u %9lu  ",
			   (Finfo.fattrib & AM_DIR) ? 'D' : '-',
			   (Finfo.fattrib & AM_RDO) ? 'R' : '-',
			   (Finfo.fattrib & AM_HID) ? 'H' : '-',
			   (Finfo.fattrib & AM_SYS) ? 'S' : '-',
			   (Finfo.fattrib & AM_ARC) ? 'A' : '-',
			   (Finfo.fdate >> 9) + 1980, (Finfo.fdate >> 5) & 15, Finfo.fdate & 31,
			   (Finfo.ftime >> 11), (Finfo.ftime >> 5) & 63, Finfo.fsize);
		if (Finfo.lfname[0]) {
			Con_Printf("%s\n", Finfo.lfname);
		} else if (Finfo.fname[0]) {
			Con_Printf("%s\n", Finfo.fname);
		}
	}
	if (argc < 2) {
		res = f_getfree("0:", &free_clust, &fs);
	} else {
		res = f_getfree(argv[1], &free_clust, &fs);
	}
	free_sect = free_clust * fs->csize;
	free_bytes = (((uint64_t) free_sect) << 9);
	if (res) {
		Con_Printf("Can not get free cluster count\n");
	} else {
		Con_Printf("\tFree Clusters %lu, ", free_clust);
		Con_Printf("Free Bytes: ");
	}
	sizelen = uitoa64(free_bytes, sizestr);
	for (i = 0; i < sizelen; i++) {
		Con_Printf("%c", sizestr[i]);
	}
	Con_Printf("\n\n");
	return 0;
}

INTERP_CMD(fatlsCmd, "fatls", cmd_fatls, "fatls # List files on SD-Card");

/**
 **************************************************************************
 * \fn static int8_t cmd_fatcat(Interp *interp,uint8_t argc,char *argv[]);
 * dump a file from SD card on console
 **************************************************************************
 */
static int8_t
cmd_fatcat(Interp * interp, uint8_t argc, char *argv[])
{
	FRESULT res;
	UINT s2;
	uint8_t buf[64];
	uint32_t i, j;
	FIL file1;
	if (argc < 2) {
		return -EC_BADNUMARGS;
	}
	res = f_open(&file1, argv[1], FA_OPEN_EXISTING | FA_READ);
	if (res) {
		Con_Printf("open failed with %d\n", res);
		return 0;
	}
	for (i = 0; i < UINT32_C(5000000) / sizeof(buf); i++) {
		res = f_read(&file1, buf, sizeof(buf), &s2);
		if (res) {
			break;
		}
		if (s2 == 0) {
			break;
		}
		for (j = 0; j < s2; j++) {
			Con_Printf("%C", buf[j]);
		}
	}
	res = f_close(&file1);
	return 0;
}

INTERP_CMD(fatcatCmd, "fatcat", cmd_fatcat, "fatcat <filename> # Dump file on SD-Card");

/**
 **************************************************************************************
 * static int8_t cmd_fatcp(Interp * interp, uint8_t argc, char *argv[])
 **************************************************************************************
 */
static int8_t
cmd_fatcp(Interp * interp, uint8_t argc, char *argv[])
{
	FRESULT res;
	UINT s2, s3;
	uint32_t i;
	uint8_t buf[64];
	FIL file1, file2;
	if (argc != 3) {
		return -EC_BADNUMARGS;
	}
#if 0
	res = f_mount(0, &f_fatfs[0]);
	if (res) {
		Con_Printf("Mount failed with %d\n", res);
		return 0;
	}
#endif
	res = f_open(&file1, argv[1], FA_OPEN_EXISTING | FA_READ);
	if (res) {
		Con_Printf("open source failed with %d\n", res);
		return 0;
	}
	res = f_open(&file2, argv[2], FA_CREATE_ALWAYS | FA_WRITE);
	if (res) {
		Con_Printf("open destination failed with %d\n", res);
		return 0;
	}
	for (i = 0; i < 500000; i++) {
		res = f_read(&file1, buf, sizeof(buf), &s2);
		if (res) {
			break;
		}
		if (s2 == 0) {
			break;
		}
		res = f_write(&file2, buf, s2, &s3);
		if (res) {
			Con_Printf("Write error %d\n", res);
			break;
		}
	}
	res = f_close(&file2);
	if (res) {
		Con_Printf("Close error %d\n", res);
	}
	return 0;
}

INTERP_CMD(fatcpCmd, "fatcp", cmd_fatcp, "fatcp <oldfile> <newfile># Copy a file on SD-Card");

/**
 ***************************************************************************
 * \fn static int8_t cmd_fatrm(interp, argc, argv);
 * Remove a file on the FAT filesystem from command shell
 ***************************************************************************
 */
static int8_t
cmd_fatrm(Interp * interp, uint8_t argc, char *argv[])
{
	FRESULT res;
	if (argc != 2) {
		return -EC_BADNUMARGS;
	}
#if 0
	res = f_mount(0, &f_fatfs[0]);
	if (res) {
		Con_Printf("Mount failed with %d\n", res);
		return 0;
	}
#endif
	res = f_unlink(argv[1]);
	if (res) {
		Con_Printf("Unlink failed with %d\n", res);
		return 0;
	}
	return 0;
}

INTERP_CMD(fatrmCmd, "fatrm", cmd_fatrm, "fatrm # Unlink a file on SD-Card");

static int8_t
cmd_fatmv(Interp * interp, uint8_t argc, char *argv[])
{
	FRESULT res;
	if (argc != 3) {
		return -EC_BADNUMARGS;
	}
	res = f_rename(argv[1], argv[2]);
	if (res != FR_OK) {
		Interp_Printf_P(interp, "Failed with EC %d\n", res);
	}
	return 0;
}

INTERP_CMD(fatmvCmd, "fatmv", cmd_fatmv, "fatmv # rename a file");

static int8_t
cmd_fatmkdir(Interp * interp, uint8_t argc, char *argv[])
{
	FRESULT res;
	if (argc != 2) {
		return -EC_BADNUMARGS;
	}
	res = f_mkdir(argv[1]);
	if (res != FR_OK) {
		Interp_Printf_P(interp, "Failed with EC %d\n", res);
	}
	return 0;
}

INTERP_CMD(fatmkdirCmd, "fatmkdir", cmd_fatmkdir, "fatmkdir # Make a directory");

static int8_t
cmd_fatmkfs(Interp * interp, uint8_t argc, char *argv[])
{
	BYTE drv;		/* Logical drive number */
	BYTE sfd;		/* Partitioning rule 0:FDISK, 1:SFD */
	UINT au;		/* Allocation unit size [bytes] */
	FRESULT res;
	if ((argc < 2)) {
		Interp_Printf_P(interp, "Missing drive number\n");
		return -EC_BADNUMARGS;
	}
	if (strcmp(argv[1], "0:") == 0) {
		drv = 0;
	} else if (strcmp(argv[1], "1:") == 0) {
		drv = 1;
	} else {
		return -EC_BADARG;
	}
	if (drv == 0) {
		sfd = 0;
		au = 4096;
	} else {
		au = 0;		/* automatically selected cluster size */
		sfd = 0;
	}
	res = f_mkfs(drv, sfd, au);
	if (res != FR_OK) {
		Interp_Printf_P(interp, "Failed with EC %d\n", res);
	}
	if (drv == 0) {
		f_setlabel("0:RedBox");
	} else {
		f_setlabel("1:RedBox");
	}
	return 0;
}

INTERP_CMD(fatmkfsCmd, "fatmkfs", cmd_fatmkfs, "fatmkfs # Make filesystem");

/**
 **********************************************************************
 * Remount the filesystem (at first start or when data might have
 * changed because of USB connection)
 **********************************************************************
 */

void
Fat_Remount()
{
	f_mount(0, &f_fatfs[0]);
//	f_mount(1, &f_fatfs[1]);
}

/**
 *****************************************************************************
 * \fn void Fat_Init();
 *****************************************************************************
 */
void
Fat_Init(void)
{
	FRESULT res;
	DIR dir;
	Fat_Remount();
	Interp_RegisterCmd(&fatlsCmd);
	Interp_RegisterCmd(&fatcatCmd);
	Interp_RegisterCmd(&fatcpCmd);
	Interp_RegisterCmd(&fatrmCmd);
	Interp_RegisterCmd(&fatmvCmd);
	Interp_RegisterCmd(&fatmkfsCmd);
	Interp_RegisterCmd(&fatmkdirCmd);
	//Interp_RegisterCmd(&gcCmd);
	Interp_RegisterCmd(&fatchainCmd);
}
