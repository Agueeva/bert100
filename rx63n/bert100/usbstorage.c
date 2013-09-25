/**
 ******************************************************************************************
 * USB storage device.
 * Implementation of the Bulk only transport protocoll and the SCSI commandset.
 ******************************************************************************************
 */
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "usbdev.h"
#include "byteorder.h"
#include "usbstorage.h"
#include "console.h"
#include "scsi.h"
#include "diskio.h"

#if 1
#define dbgprintf(x...) Con_Printf(x)
#else
#define dbgprintf(x...)
#endif

#define EP0 0
#define EP1 1
#define EP2 2

#define US_STATE_IDLE 		0
#define US_STATE_DISK_WRITE	1
#define DRIVE_NR(us)	((us)->driveNr)
/**
 ****************************************************************
 * The Usb storage device inherits from the USB device.
 ****************************************************************
 */

typedef struct UsbStor {
	UsbDev *usbDev;
	bool unitReady;
	uint8_t driveNr;
	uint8_t state;
	uint32_t expected_bytes;
	uint16_t ibuf_wp;
	uint32_t write_lba;

	uint32_t dataResidue;	/* Tracking of transmitted bytes */

	uint8_t cswBuf[16];	/* 13 Bytes, pad to 16 */
	uint8_t iobuf[512];

	/* Buffer for SCSI sense */
	bool sense_valid;
	uint8_t sense_key;
	uint32_t sense_info;
	uint8_t addsense_key;
	uint8_t addsense_qual;
} UsbStor;

static UsbStor gUsbStor;

static const usb_device_descriptor dev_descr_tmpl = {
	0x12,			/* uint8_t  bLength;                            */
	USB_DT_DEVICE,
	host16_to_le(0x0200),	/* le16 bcdUSB Version 1.1                      */
	0x00,			/* Class code defined at interface level        */
	0x00,			/* SubClass;                                    */
	0x00,			/* DeviceProtocol                               */
	0x40,			/* MaxPacketSize0                               */
	host16_to_le(0x045b),	/* idVendor Hitachi                             */
	host16_to_le(0x0816),	/* le16 idProduct                               */
	host16_to_le(0x0100),	/* le16 bcdDevice                               */
	0x01,			/* uint8_t  iManufacturer;                      */
	0x02,			/* uint8_t  iProduct;                           */
	0x03,			/* uint8_t  iSerialNumber;                      */
	0x01,			/* bNumConfigurations, eventually patched       */
};

static const usb_config_descriptor conf_descr_tmpl = {
	0x09,			/* uint8_t  bLength; */
	USB_DT_CONFIG,		/* uint8_t  bDescriptorType;                            */
	host16_to_le(0x0009),	/* le16 TotalLength will be patched later               */
	0x01,			/* uint8_t  bNumInterfaces;                             */
	0x01,			/* uint8_t  bConfigurationValue;                        */
	0x00,			/* uint8_t  iConfiguration; (starts with 1)             */
	0x80,			/* uint8_t  bmAttributes; Laserjet 2200 does not set D7 */
	0xfa,			/* uint8_t  bMaxPower in units of 2mA */
};

static const usb_interface_descriptor if_descr_tmpl = {
	0x09,			/* uint8_t  bLength;                                    */
	USB_DT_INTERFACE,
	0x00,			/* uint8_t  bInterfaceNumber is zerobased               */
	0x00,			/* uint8_t  bAlternateSetting  (zerobased)              */
	0x02,			/* uint8_t  bNumEndpoints; excluding ep0                */
	USB_CLASS_STORAGE,	/* uint8_t  bInterfaceClass Mass Storage   */
	0x06,			/* uint8_t  bInterfaceSubClass SCSI transparent */
	0x50,			/* uint8_t  bInterfaceProtocol                          */
	0x00			/* uint8_t  iInterface; Don't describe it with a string */
};

/**
 ***********************************************************************************************
 * My memorystick has 512 Bytes maxpacketsize, but Specification says 8 - 64 Bytes is allowed
 ***********************************************************************************************
 */

static const usb_endpoint_descriptor bulkin_ep_descr_tmpl = {
	0x07,			/* uint8_t  bLength; printer class doku is shit ?       */
	USB_DT_ENDPOINT,
	0x81,			/* uint8_t  bEndpointAddress;                           */
	0x02,			/* uint8_t  bmAttributes;                               */
	0x40, 0x00,		/* le16 wMaxPacketSize 64 Bytes ?                       */
	0x00,			/* uint8_t  bInterval is for isochronous only           */
};

static const usb_endpoint_descriptor bulkout_ep_descr_tmpl = {
	0x07,			/* uint8_t  bLength;                                    */
	USB_DT_ENDPOINT,
	0x02,			/* uint8_t  bEndpointAddress;                           */
	0x02,			/* uint8_t  bmAttributes: Bulk endpoint                 */
	0x40, 0x00,		/* le16 wMaxPacketSize 64 Bytes ?                       */
	0x00,			/* uint8_t  bInterval is for isochronous only           */
};

/**
 ********************************************************************+
 * The command block wrapper for USB Storage devices
 ********************************************************************+
 */
typedef struct UsbStor_CBW {
	uint32_le dCBWSignature;
	uint32_le dCBWTag;
	uint32_le dCBWDataTransferLength;	/* Number of bytes expected by host. */
	uint8_t bmCBWFlags;
	uint8_t bCBWLUN;
	uint8_t bCBWCBLength;
	uint8_t CBWCB[16];
} __attribute__ ((packed)) UsbStor_CBW;

/**
 ********************************************************************+
 * The command status wrapper for USB Storage devices
 ********************************************************************+
 */
typedef struct UsbStor_CSW {
	uint32_le dCSWSignature;
	uint32_le dCSWTag;
	uint32_le dCSWDataResidue;
	uint8_t bCSWStatus;
} __attribute__ ((packed)) UsbStor_CSW;

typedef enum {
	CSWS_Good = 0,
	CSWS_Failed = 1,
	CSWS_PhaseErr = 2,
} CSWStatus;

#define MIN(x,y) ((x) < (y) ? (x) : (y))

/*
 **********************************************************************
 * memcpy with using the smaller of the two length fields and
 * returning the number of bytes copied.
 **********************************************************************
 */
INLINE size_t
mincpy(void *dst, const void *src, size_t len1, size_t len2)
{
	size_t len = len1 < len2 ? len1 : len2;
	memcpy(dst, src, len);
	return len;
}

static UsbTaRes
Usb_GetDeviceDescriptor(void *eventData, uint8_t epNr, UsbSetupBuffer * sb)
{
	sb->dataLen = mincpy(sb->setupData, &dev_descr_tmpl, sizeof(dev_descr_tmpl), sb->wLength);
	return USBTA_OK;
}

static UsbDescriptorHandler devDescrHandler = {
	.keyUsbValue = 0x100,
	.getDescriptor = Usb_GetDeviceDescriptor,
	.setDescriptor = NULL,
};

/**
 ********************************************************************************************
 * \fn UsbTaRes UsbStor_GetConfigDescriptor(void *eventData,uint8_t epNr,UsbSetupBuffer *sb)
 * Read the configuration descriptor.
 ********************************************************************************************
 */

static UsbTaRes
UsbStor_GetConfigDescriptor(void *eventData, uint8_t epNr, UsbSetupBuffer * sb)
{
	int len_config = conf_descr_tmpl.bLength;
	int len_if = if_descr_tmpl.bLength;
	int len_out = bulkout_ep_descr_tmpl.bLength;
	int len_in = bulkin_ep_descr_tmpl.bLength;
	int totallen = len_config + len_if + len_out + len_in;
	uint8_t *buf = sb->setupData;
	int count;
	memcpy(buf, &conf_descr_tmpl, len_config);
	count = len_config;
	memcpy(buf + count, &if_descr_tmpl, len_if);
	count += len_if;
	memcpy(buf + count, &bulkin_ep_descr_tmpl, len_in);
	count += len_in;
	memcpy(buf + count, &bulkout_ep_descr_tmpl, len_out);
	count += len_out;
	/* Patch the tottallen field of config descriptor */
	buf[2] = totallen & 0xff;
	buf[3] = totallen >> 8;
	sb->dataLen = count;
	return USBTA_OK;
}

static UsbDescriptorHandler confDescrHandler = {
	.keyUsbValue = ((uint16_t) USB_DT_CONFIG << 8) | 0,
	.getDescriptor = UsbStor_GetConfigDescriptor,
	.setDescriptor = NULL,
};

/**
 *******************************************************
 * Omit the CSW. Only difference to "omit data" is
 * the missing residue tracking. 
 *******************************************************
 */
INLINE void
UsbStor_CswIn(UsbStor * us, uint8_t * data, uint16_t cnt)
{
	UsbDev_EpInTransmit(us->usbDev, EP1, data, cnt);
}

/**
 *****************************************************************************
 * \fn static void UsbStor_DataIn(UsbStor *us,uint8_t *data,uint16_t cnt)
 * Omit data to the IN endpoint with residue tracking for the CSW 
 * A Zero size paket must be possible because data transmission might
 * be terminated with a short paket
 *****************************************************************************
 */
static void
UsbStor_DataIn(UsbStor * us, uint8_t * data, uint16_t cnt)
{
	cnt = MIN(cnt, us->dataResidue);
	UsbDev_EpInTransmit(us->usbDev, EP1, data, cnt);
	us->dataResidue -= cnt;
}

static void
SC_SetSense(UsbStor * us, uint8_t sense_key, uint32_t sense_info, uint8_t asc, uint8_t ascq)
{
	us->sense_key = sense_key;
	us->sense_info = sense_info;
	us->addsense_key = asc;
	us->addsense_qual = ascq;
	us->sense_valid = true;
}

/**
 *************************************************************************
 * \fn static bool check_ready(UsbStor *us);
 * Check if unit is ready. called before all commands accessing the
 * Disk. It sets a sense key only if the unit is not ready.
 * It uses the ASC/ASCQ for "Unit is in progress becoming ready"
 *************************************************************************
 */
static bool
check_ready(UsbStor * us)
{
	if (!us->unitReady) {
		/* Logical unit is in process of becoming ready */
		SC_SetSense(us, SK_NOT_READY, 0, 4, 1);
	}
	return us->unitReady;
}

/**
 **********************************************************************
 * Test Unit ready command
 **********************************************************************
 */
static CSWStatus
UsbStor_SCTestUnitReady(UsbStor * us, uint8_t * cmdblock)
{
	if (check_ready(us)) {
		SC_SetSense(us, SK_NO_SENSE, 0, 0, 0);
		return CSWS_Good;
	} else {
		return CSWS_Failed;
	}
}

/**
 **************************************************************************
 * SCSI command Inquire.
 * The Medium removable flag is unclear in the SCSI spec because
 * it doesn't say if only the medium or the complete device is removable.
 * But Windows requires the removeable bit to be set. If not it will
 * sync the disc less often and will not send PREV/ALLOW removal commands.
 * And it will create folders for "Trash" on the drive.
 **************************************************************************
 */
static CSWStatus
UsbStor_SCInquire(UsbStor * us)
{
	uint8_t *data = us->iobuf;
	data[0] = 0;
	data[1] = 0x80;		/* medium removable */
	//data[1] = 0; /* medium not removable  */
	data[2] = 2;		/* SPC 1997  */
	data[3] = 2;		/* SCSI level */
	data[4] = 31;
	data[5] = data[6] = data[7] = 0;
#ifdef BOARD_SAKURA
	strncpy((char *)data + 8, "Renesas ", 8);
	strncpy((char *)data + 16, "Sakura Board    ", 16);
#else
	strncpy((char *)data + 8, "Munich-I", 8);
	strncpy((char *)data + 16, "C-BERT          ", 16);
	strncpy((char *)data + 32, "0001", 4);
#endif
	UsbStor_DataIn(us, data, 36);
	SC_SetSense(us, SK_NO_SENSE, 0, 0, 0);
	return CSWS_Good;
}

/**
 ****************************************************************************
 * \fn static void UsbStor_SCReadCapacities(UsbStor *us) 
 *
 ****************************************************************************
 */
static CSWStatus
UsbStor_SCReadFormatCapacities(UsbStor * us)
{
	uint8_t *data = us->iobuf;
	uint32_t num_sectors = 0;
	if (!check_ready(us)) {
		return CSWS_Failed;
	}
	if (disk_ioctl(DRIVE_NR(us), GET_SECTOR_COUNT, &num_sectors) != RES_OK) {
		Con_Printf("Can not get num_sectors\n");
		//      SC_SetSense(us,SK_NO_SENSE,0,0,0);
	}
	data[0] = data[1] = data[2] = 0;
	data[3] = 8;		/* Size in bytes of the following capacity descriptors */
	data[4] = num_sectors >> 24;
	data[5] = (num_sectors >> 16) & 0xff;
	data[6] = (num_sectors >> 8) & 0xff;
	data[7] = (num_sectors >> 0) & 0xff;
	data[8] = 2;
	data[9] = 0;
	data[10] = 512 >> 8;
	data[11] = 512 & 0xff;
	UsbStor_DataIn(us, data, 12);
	SC_SetSense(us, SK_NO_SENSE, 0, 0, 0);
	return CSWS_Good;
}

static CSWStatus
UsbStor_SCReadCapacity10(UsbStor * us)
{
	uint8_t *data = us->iobuf;
	uint32_t num_sectors = 0;
	if (!check_ready(us)) {
		return CSWS_Failed;
	}
	if (disk_ioctl(DRIVE_NR(us), GET_SECTOR_COUNT, &num_sectors) != RES_OK) {
		// shit
		//SC_SetSense(us, SK_NOT_READY, 0, 0, 0);
		SC_SetSense(us, SK_NO_SENSE, 0, 0, 0);
	} else {
		SC_SetSense(us, SK_NO_SENSE, 0, 0, 0);
	}
	data[0] = (num_sectors - 1) >> 24;
	data[1] = ((num_sectors - 1) >> 16) & 0xff;
	data[2] = ((num_sectors - 1) >> 8) & 0xff;
	data[3] = ((num_sectors - 1) >> 0) & 0xff;
	data[4] = 0;
	data[5] = 0;
	data[6] = 512 >> 8;
	data[7] = 512 & 0xff;
	UsbStor_DataIn(us, data, 8);
	return CSWS_Good;
}

INLINE uint32_t
read32(void *addr)
{
	return *(uint32_t *) addr;
}

INLINE uint16_t
read16(void *addr)
{
	return *(uint16_t *) addr;
}

INLINE void
write16(uint16_t value, void *addr)
{
	*(uint16_t *) addr = value;
}

INLINE void
write32(uint16_t value, void *addr)
{
	*(uint32_t *) addr = value;
}

/**
 *********************************************************************
 * \fn static void UsbStor_SCRead6(UsbStor *us,uint8_t *cmdblock) 
 *********************************************************************
 */
static CSWStatus
UsbStor_SCRead6(UsbStor * us, uint8_t * cmdblock)
{
	uint8_t lun = cmdblock[1] >> 5;
	uint32_t lba;
	uint8_t transfer_length;
	uint16_t i;
	uint8_t *data = us->iobuf;
	lba = cmdblock[1] & 0x1F;
	lba <<= 8;
	lba |= cmdblock[2];
	lba <<= 8;
	lba |= cmdblock[3];
	transfer_length = cmdblock[4];
	if (!check_ready(us)) {
		return CSWS_Failed;
	}
	if (lun != 0) {
		Con_Printf("Illegal Logical unit number %d, lba %08lx\n", lun, lba);
		SC_SetSense(us, SK_ILLEGAL_REQUEST, 0, ASQ_ILLEGAL_LUN, 0);
		return CSWS_Failed;
	}
	for (i = 0; i < transfer_length; i++) {
		if (disk_read(DRIVE_NR(us), data, lba + i, 1) != RES_OK) {
			Con_Printf("************** Can not read sector\n");
			UsbStor_DataIn(us, data, 0);
			SC_SetSense(us, SK_MEDIUM_ERROR, 0, ASQ_UNREC_READ_ERR, 0);
			return CSWS_Failed;
		}
		UsbStor_DataIn(us, data, 512);
		if (us->dataResidue == 0) {
			break;
		}
	}
	SC_SetSense(us, SK_NO_SENSE, 0, 0, 0);
	return CSWS_Good;
}

/**
 *****************************************************************************
 * static void UsbStor_SCRead10(UsbStor *us,uint8_t *cmdblock) 
 *****************************************************************************
 */
static CSWStatus
UsbStor_SCRead10(UsbStor * us, uint8_t * cmdblock)
{
	uint8_t lun = cmdblock[1] >> 5;
	uint32_t lba;
	uint16_t i;
	uint16_t transfer_length;
	uint8_t *data = us->iobuf;
	lba = be32_to_host(read32(cmdblock + 2));
	transfer_length = be16_to_host(read16(cmdblock + 7));
	if (!check_ready(us)) {
		return CSWS_Failed;
	}
	//Con_Printf("Read10 0x%08lx %02x\n",lba,transfer_length);
	if (lun != 0) {
		Con_Printf("Illegal Logical unit number %d, lba %08lx\n", lun, lba);
		SC_SetSense(us, SK_ILLEGAL_REQUEST, 0, 0x25, 0);
		return CSWS_Failed;
	}
	for (i = 0; i < transfer_length; i++) {
		if (disk_read(DRIVE_NR(us), data, lba + i, 1) != RES_OK) {
			Con_Printf("************** Can not read sector\n");
			UsbStor_DataIn(us, data, 0);
			SC_SetSense(us, SK_MEDIUM_ERROR, 0, ASQ_UNREC_READ_ERR, 0);
			return CSWS_Failed;
		}
		UsbStor_DataIn(us, data, 512);
		if (us->dataResidue == 0) {
			break;
		}
	}
	SC_SetSense(us, SK_NO_SENSE, 0, 0, 0);
	return CSWS_Good;
}

/**
 *************************************************************
 * SCSI READ12
 *************************************************************
 */
static CSWStatus
UsbStor_SCRead12(UsbStor * us, uint8_t * cmdblock)
{
	uint8_t lun = cmdblock[1] >> 5;
	uint32_t lba;
	uint32_t i;
	uint32_t transfer_length;
	uint8_t *data = us->iobuf;
	if (!check_ready(us)) {
		return CSWS_Failed;
	}
	lba = be32_to_host(read32(cmdblock + 2));
	transfer_length = be32_to_host(read32(cmdblock + 6));
	dbgprintf("Read12 0x%08lx %08lx\n", lba, transfer_length);
	if (lun != 0) {
		Con_Printf("Illegal Logical unit number %d, lba %08lx\n", lun, lba);
		SC_SetSense(us, SK_ILLEGAL_REQUEST, 0, ASQ_ILLEGAL_LUN, 0);
		return CSWS_Failed;
	}
	for (i = 0; i < transfer_length; i++) {
		if (disk_read(DRIVE_NR(us), data, lba + i, 1) != RES_OK) {
			Con_Printf("************** Can not read sector\n");
			UsbStor_DataIn(us, data, 0);
			SC_SetSense(us, SK_MEDIUM_ERROR, 0, ASQ_UNREC_READ_ERR, 0);
			return CSWS_Failed;
		}
		UsbStor_DataIn(us, data, 512);
		if (us->dataResidue == 0) {
			break;
		}
	}
	SC_SetSense(us, SK_NO_SENSE, 0, 0, 0);
	return CSWS_Good;
}

/**
 **************************************************************
 * SCSI VERIFY(10) command
 **************************************************************
 */
static CSWStatus
UsbStor_SCVerify10(UsbStor * us, uint8_t * cmdblock)
{
	uint8_t lun = cmdblock[1] >> 5;
	uint32_t lba;
	uint16_t i;
	uint16_t transfer_length;
	uint8_t *data = us->iobuf;
	if (!check_ready(us)) {
		return CSWS_Failed;
	}
	lba = be32_to_host(read32(cmdblock + 2));
	transfer_length = be16_to_host(read16(cmdblock + 7));
	dbgprintf("Verify10 0x%08lx %02x\n", lba, transfer_length);
	if (lun != 0) {
		Con_Printf("Illegal Logical unit number %d, lba %08lx\n", lun, lba);
		SC_SetSense(us, SK_ILLEGAL_REQUEST, 0, ASQ_ILLEGAL_LUN, 0);
		return CSWS_Failed;
	}
	for (i = 0; i < transfer_length; i++) {
		if (disk_read(DRIVE_NR(us), data, lba + i, 1) != RES_OK) {
			Con_Printf("************** Can not verify sector\n");
			// set sense somehow ???
		}
	}
	UsbStor_DataIn(us, data, 0);
	SC_SetSense(us, SK_NO_SENSE, 0, 0, 0);
	return CSWS_Good;
}

/**
 *******************************************************
 * ModeSense6
 * windows asks for 0x1c:002
 *******************************************************
 */
static CSWStatus
UsbStor_SCModeSense6(UsbStor * us, uint8_t * cmdblock)
{
	uint8_t *data = us->iobuf;
	uint16_t parlistlen;
	uint8_t len;
	uint8_t page_code;
	uint8_t subpage_code;
	uint8_t pc;
	uint8_t *buf = data + 4;
	bool valid = false;
	memset(data, 0, 128);
	parlistlen = cmdblock[4];
	pc = (cmdblock[2] >> 5) & 3;
	page_code = cmdblock[2] & 0x3f;
	subpage_code = cmdblock[3];
	dbgprintf("Page Code %02x, maxlen %u\n", page_code, us->dataResidue);
	if ((page_code == 0x08) || (page_code == 0x3f)) {
		buf[0] = 8;
		buf[1] = 10;
		valid = true;
		if (pc != 1) {
			buf[2] = 4;
			/* Disable prefetch transfer length */
			write16(host16_to_be(0xffff), buf + 4);
			/* Minimum prefetch */
			write16(host16_to_be(0x0000), buf + 6);
			/* Maximum prefetch */
			write16(host16_to_be(0xffff), buf + 8);
			/* Maximum prefetch ceiling */
			write16(host16_to_be(0xffff), buf + 10);
		}
	}
	len = 16;
	data[0] = len - 1;	/* Len not including itself */
	data[1] = 0;
	//data[2] = 0x80; /* Read only device for now */
	data[2] = 0x0;		/* Writable */

	if (valid) {
		SC_SetSense(us, SK_NO_SENSE, 0, 0, 0);
		UsbStor_DataIn(us, data, len);
		return CSWS_Good;
	} else {
		Con_Printf("Unhandled MODE SENSE page code 0x%02x:%0x02x\n", page_code,
			   subpage_code);
		SC_SetSense(us, SK_ILLEGAL_REQUEST, 0, ASQ_INVALID_FIELD, 0);
		UsbStor_DataIn(us, data, 0);
		return CSWS_Failed;
	}
}

/**
 *********************************************************************************
 * \fn static void UsbStor_SCModeSense10(UsbStor *us,uint8_t *cmdblock) 
 *********************************************************************************
 */
static CSWStatus
UsbStor_SCModeSense10(UsbStor * us, uint8_t * cmdblock)
{

	uint8_t *data = us->iobuf;
	uint16_t parlistlen;
	uint8_t len;
	uint8_t page_code;
	uint8_t pc;
	uint8_t *buf = data + 8;
	bool valid = false;
	memset(data, 0, 128);
	parlistlen = be16_to_host(read16(cmdblock + 7));
	pc = (cmdblock[2] >> 5) & 3;
	page_code = cmdblock[2] & 0x3f;
	dbgprintf("Page Code %02x, maxlen %u\n", page_code, us->dataResidue);
	if ((page_code == 0x08) || (page_code == 0x3f)) {
		buf[0] = 8;
		buf[1] = 10;
		valid = true;
		if (pc != 1) {
			buf[2] = 4;
			/* Disable prefetch transfer length */
			write16(host16_to_be(0xffff), buf + 4);
			/* Minimum prefetch */
			write16(host16_to_be(0x0000), buf + 6);
			/* Maximum prefetch */
			write16(host16_to_be(0xffff), buf + 8);
			/* Maximum prefetch ceiling */
			write16(host16_to_be(0xffff), buf + 10);
		}
	}
	data[2] = 0;
	data[3] = 0x0;		/* Writable */

	if (!valid) {
		Con_Printf("Shit, MODE SENSE error handling ins missing\n");
		len = 0;
	} else {
		len = 20;
		write16(host16_to_be(len - 2), data);
	}
	/* Len not including length field */

	UsbStor_DataIn(us, data, len);
	if (valid) {
		SC_SetSense(us, SK_NO_SENSE, 0, 0, 0);
		return CSWS_Good;
	} else {
		SC_SetSense(us, SK_ILLEGAL_REQUEST, 0, ASQ_INVALID_FIELD, 0);
		return CSWS_Failed;
	}
}

/**
 *****************************************************************************
 * \fn static void UsbStor_SCRequestSense(UsbStor *us,uint8_t *cmdblock) 
 *****************************************************************************
 */
static CSWStatus
UsbStor_SCRequestSense(UsbStor * us, uint8_t * cmdblock)
{

	uint8_t *data = us->iobuf;
	memset(data, 0, 18);
	data[0] = us->sense_valid ? 0xF0 : 0x70;
	data[1] = 0;
	data[2] = us->sense_key;
	/* Byteorder ??? where is it converted ? */
	write32(host32_to_be(us->sense_info), data + 3);
	data[7] = 10;
	data[12] = us->addsense_key;
	data[13] = us->addsense_qual;
#if 0
	/* Information is NOT cleared by REQUEST SENSE ! */
	if (us->sense_valid) {
		us->sense_key = 0;
		us->sense_info = 0;
		us->addsense_key = 0;
		us->addsense_qual = 0;
	}
#endif
	UsbStor_DataIn(us, data, 18);
	SC_SetSense(us, SK_NO_SENSE, 0, 0, 0);
	return CSWS_Good;
}

/**
 *****************************************************
 * ModeSelect is not implemented
 * This is a minimal implementation 
 *****************************************************
 */
static CSWStatus
UsbStor_SCModeSelect6(UsbStor * us, uint8_t * cmdblock)
{
	uint8_t *data = us->iobuf;
	UsbStor_DataIn(us, data, 0);
	SC_SetSense(us, SK_NO_SENSE, 0, 0, 0);
	return CSWS_Good;
}

/**
 ***************************************************************************
 * \fn static void UsbStor_SCModeSelect10(UsbStor *us,uint8_t *cmdblock) 
 ***************************************************************************
 */
static CSWStatus
UsbStor_SCModeSelect10(UsbStor * us, uint8_t * cmdblock)
{
	uint8_t *data = us->iobuf;
	UsbStor_DataIn(us, data, 0);
	SC_SetSense(us, SK_NO_SENSE, 0, 0, 0);
	return CSWS_Good;
}

/*
 ******************************************************************
 * SCSI WRITE(6)
 ******************************************************************
 */
static CSWStatus
UsbStor_SCWrite6(UsbStor * us, uint8_t * cmdblock)
{
	uint8_t lun = cmdblock[1] >> 5;
	uint32_t lba;
	uint8_t transfer_length;
	lba = cmdblock[1] & 0x1F;
	lba <<= 8;
	lba |= cmdblock[2];
	lba <<= 8;
	lba |= cmdblock[3];
	transfer_length = cmdblock[4];
	if (!check_ready(us)) {
		return CSWS_Failed;
	}
	if (lun != 0) {
		Con_Printf("BAD LUN %d\n", lun);
		SC_SetSense(us, SK_ILLEGAL_REQUEST, 0, ASQ_ILLEGAL_LUN, 0);
		return CSWS_Failed;
	}
	lba = be32_to_host(read32(cmdblock + 2));
	transfer_length = be16_to_host(read16(cmdblock + 7));
	us->expected_bytes = transfer_length * 512;
	us->ibuf_wp = 0;
	us->write_lba = lba;
	us->state = US_STATE_DISK_WRITE;
	dbgprintf("SCSI Write 6 LBA 0x%08lx, length 0x%04x\n", lba, transfer_length);
	SC_SetSense(us, SK_NO_SENSE, 0, 0, 0);
	return CSWS_Good;
}

/**
 ***************************************************************
 * WRITE(10) 
 ***************************************************************
 */

static CSWStatus
UsbStor_SCWrite10(UsbStor * us, uint8_t * cmdblock)
{
	uint8_t lun = (cmdblock[1] >> 5) & 7;
	uint16_t transfer_length;
	uint32_t lba;
	if (!check_ready(us)) {
		return CSWS_Failed;
	}
	if (lun != 0) {
		Con_Printf("BAD LUN %d\n", lun);
		SC_SetSense(us, SK_ILLEGAL_REQUEST, 0, ASQ_ILLEGAL_LUN, 0);
		return CSWS_Failed;
	}
	lba = be32_to_host(read32(cmdblock + 2));
	transfer_length = be16_to_host(read16(cmdblock + 7));
	us->expected_bytes = transfer_length * 512;
	us->ibuf_wp = 0;
	us->write_lba = lba;
	us->state = US_STATE_DISK_WRITE;
	dbgprintf("Write 10 LBA 0x%08lx, length 0x%04x\n",lba,transfer_length);
	SC_SetSense(us, SK_NO_SENSE, 0, 0, 0);
	return CSWS_Good;
}

/**
 **********************************************************************
 * WRITE(12)
 **********************************************************************
 */
static CSWStatus
UsbStor_SCWrite12(UsbStor * us, uint8_t * cmdblock)
{
	uint8_t lun = cmdblock[1] >> 5;
	uint32_t lba;
	uint32_t transfer_length;
	if (lun != 0) {
		Con_Printf("BAD LUN %d\n", lun);
		SC_SetSense(us, SK_ILLEGAL_REQUEST, 0, ASQ_ILLEGAL_LUN, 0);
		return CSWS_Failed;
	}
	if (!check_ready(us)) {
		return CSWS_Failed;
	}
	lba = be32_to_host(read32(cmdblock + 2));
	transfer_length = be32_to_host(read32(cmdblock + 6));
	us->expected_bytes = transfer_length * 512;
	us->ibuf_wp = 0;
	us->write_lba = lba;
	us->state = US_STATE_DISK_WRITE;
	dbgprintf("Write12 0x%08lx %08lx\n", lba, transfer_length);
	SC_SetSense(us, SK_NO_SENSE, 0, 0, 0);
	return CSWS_Good;
}

/**
 ********************************************************************************
 * \fn static void UsbStor_SCUnimplemented(UsbStor *us,uint8_t *cmdblock) 
 ********************************************************************************
 */
static CSWStatus
UsbStor_SCUnimplemented(UsbStor * us, uint8_t * cmdblock)
{
	/* If somethin is requested reply with a short paket */
	if (us->dataResidue) {
		UsbStor_DataIn(us, us->iobuf, 0);
	}
	SC_SetSense(us, SK_ILLEGAL_REQUEST, 0, ASQ_UNSUPPORTED_CMD, 0);
	return CSWS_Failed;
}

/**
 ******************************************************************************
 * Prevent/Allow medium removal.
 * Just accept it and do nothing because we can not stop anybody
 * from unplugging the USB cable
 *****************************************************************************
 */
static CSWStatus
UsbStor_SCPrevAllowRemoval(UsbStor * us, uint8_t * cmdblock)
{
	if (!check_ready(us)) {
		return CSWS_Failed;
	}
	if (disk_ioctl(DRIVE_NR(us), CTRL_SYNC, (void *)0) != RES_OK) {
		Con_Printf("Prev/Allow Removal: Sync failed\n");
	}
	SC_SetSense(us, SK_NO_SENSE, 0, 0, 0);
	return CSWS_Good;
}

/**
 **********************************************************************
 * Start Stop Unit 
 * Currently only syncs the disk. Should also disable the Unit on Stop
 **********************************************************************
 */
static CSWStatus
UsbStor_SCStartStopUnit(UsbStor * us, uint8_t * cmdblock)
{
	if (!check_ready(us)) {
		return CSWS_Failed;
	}
	if (disk_ioctl(DRIVE_NR(us), CTRL_SYNC, (void *)0) != RES_OK) {
		Con_Printf("Start Stop Unit: Sync failed\n");
	}
	SC_SetSense(us, SK_NO_SENSE, 0, 0, 0);
	return CSWS_Good;
}

/**
 **********************************************************************************
 * \fn static CSWStatus UsbStor_SynchronizeCache(UsbStor *us,uint8_t *cmdblock) 
 * Always sync the device ignore all arguments.
 **********************************************************************************
 */
static CSWStatus
UsbStor_SCSyncCache(UsbStor * us, uint8_t * cmdblock)
{
	if (!check_ready(us)) {
		return CSWS_Failed;
	}
	if (disk_ioctl(DRIVE_NR(us), CTRL_SYNC, (void *)0) != RES_OK) {
		Con_Printf("Sync failed\n");
	}
	SC_SetSense(us, SK_NO_SENSE, 0, 0, 0);
	return CSWS_Good;
}

/**
 ****************************************************************************************
 * \fn static void UsbStor_HandleScsiCmd(UsbStor *us,uint8_t *cmdblock,uint16_t word) 
 ****************************************************************************************
 */
static CSWStatus
UsbStor_HandleScsiCmd(UsbStor * us, uint8_t * cmdblock, uint16_t word)
{
	uint8_t cmd = cmdblock[0];
	uint8_t exp_len;
	CSWStatus csws;
	switch (cmd) {
	    case SC_FORMAT_UNIT:
		    Con_Printf("unsupported ** SC_FORMAT_UNIT\n");
		    csws = UsbStor_SCUnimplemented(us, cmdblock);
		    break;

	    case SC_TEST_UNIT_READY:
		    csws = UsbStor_SCTestUnitReady(us, cmdblock);
		    break;

	    case SC_INQUIRY:
		    dbgprintf("SC_INQUIRY\n");
		    exp_len = 6;
		    csws = UsbStor_SCInquire(us);
		    break;

	    case SC_MODE_SELECT6:
		    csws = UsbStor_SCModeSelect6(us, cmdblock);
		    dbgprintf("SC_MODE_SELECT6\n");
		    break;

	    case SC_MODE_SELECT10:
		    dbgprintf("SC_MODE_SELECT10\n");
		    csws = UsbStor_SCModeSelect10(us, cmdblock);
		    break;

	    case SC_MODE_SENSE6:
		    dbgprintf("SC_MODE_SENSE6\n");
		    csws = UsbStor_SCModeSense6(us, cmdblock);
		    break;

	    case SC_MODE_SENSE10:
		    Con_Printf("Untested ** SC_MODE_SENSE10\n");
		    csws = UsbStor_SCModeSense10(us, cmdblock);
		    break;

	    case SC_PREV_ALLOW_REMOVAL:
		    //dbgprintf("SC_PREV_ALLOW_REMOVAL\n");
		    csws = UsbStor_SCPrevAllowRemoval(us, cmdblock);
		    break;

	    case SC_READ6:
		    Con_Printf("untested ** SC_READ6\n");
		    csws = UsbStor_SCRead6(us, cmdblock);
		    break;

	    case SC_READ10:
		    csws = UsbStor_SCRead10(us, cmdblock);
		    break;

	    case SC_READ12:
		    dbgprintf("untested ** SC_READ12\n");
		    csws = UsbStor_SCRead12(us, cmdblock);
		    break;

	    case SC_READ_CAPACITY10:
		    dbgprintf("SC_READ_CAPACITY10\n");
		    csws = UsbStor_SCReadCapacity10(us);
		    break;

	    case SC_READ_FORMAT_CAPACITIES:
		    Con_Printf("SC_READ_FORMAT_CAPACITIES\n");
		    csws = UsbStor_SCReadFormatCapacities(us);
		    break;

	    case SC_RELEASE:
		    Con_Printf("unimplemented ** SC_RELEASE\n");
		    csws = UsbStor_SCUnimplemented(us, cmdblock);
		    break;

	    case SC_REQUEST_SENSE:
		    dbgprintf("SC_REQUEST_SENSE\n");
		    csws = UsbStor_SCRequestSense(us, cmdblock);
		    break;

	    case SC_RESERVE:
		    Con_Printf("unimplementd ** SC_RESERVE\n");
		    csws = UsbStor_SCUnimplemented(us, cmdblock);
		    break;

	    case SC_SEND_DIAGNOSTICS:
		    Con_Printf("unimplemented ** SC_SEND_DIAG\n");
		    csws = UsbStor_SCUnimplemented(us, cmdblock);
		    break;

	    case SC_START_STOP_UNIT:
		    Con_Printf("Untested: SC_START_STOP_UNIT\n");
		    csws = UsbStor_SCStartStopUnit(us, cmdblock);
		    break;

	    case SC_SYNC_CACHE:
		    Con_Printf("SC_SYNC_CACHE\n");
		    csws = UsbStor_SCSyncCache(us, cmdblock);
		    break;

	    case SC_VERIFY10:
		    Con_Printf("untested ** SC_VERIFY10\n");
		    csws = UsbStor_SCVerify10(us, cmdblock);
		    break;

	    case SC_WRITE6:
		    Con_Printf("untested ** SC_WRITE6\n");
		    csws = UsbStor_SCWrite6(us, cmdblock);
		    break;

	    case SC_WRITE10:
		    csws = UsbStor_SCWrite10(us, cmdblock);
		    break;

	    case SC_WRITE12:
		    Con_Printf("untested ** SC_WRITE12\n");
		    csws = UsbStor_SCWrite12(us, cmdblock);
		    break;

	    default:
		    Con_Printf("Unexpected SCSI command %02x\n", cmd);
		    csws = UsbStor_SCUnimplemented(us, cmdblock);
		    break;

	}
	return csws;
}

/**
 ***********************************************************************************
 * The output endpoint. Because of host side view this is the input of the 
 * usb device. It feeds a statemachine.
 ***********************************************************************************
 */
static void
UsbStor_EPOut(void *eventData,const uint8_t * data, uint16_t cnt)
{
	UsbStor *us = eventData;
	uint16_t i;
	UsbStor_CBW *cbw;
	UsbStor_CSW *csw = (UsbStor_CSW *) us->cswBuf;
	switch (us->state) {
	    case US_STATE_IDLE:
		    if ((cnt != 31) /* || csw->cswSignature != 0x43425355) */ ) {
			    Con_Printf("Not a Command Block Wrapper, len %d\n", cnt);
			    break;
		    }
		    cbw = (UsbStor_CBW *) data;
		    us->dataResidue = le32_to_host(cbw->dCBWDataTransferLength);
		    csw->dCSWSignature = host32_to_le(0x53425355);
		    csw->dCSWTag = cbw->dCBWTag;
		    csw->bCSWStatus = 0;

		    //Con_Printf("Max Data length is %d\n",cbw->dCBWDataTransferLength);
		    if (cbw->bCBWCBLength <= 16) {
			    csw->bCSWStatus =
				UsbStor_HandleScsiCmd(us, cbw->CBWCB, cbw->bCBWCBLength);
		    } else {
			    Con_Printf("Illegal length %d for command area\n", cbw->bCBWCBLength);
		    }
		    if (us->state == US_STATE_IDLE) {
			    csw->dCSWDataResidue = host32_to_le(us->dataResidue);
			    UsbStor_CswIn(us, (uint8_t *) csw, sizeof(*csw));
		    }
		    break;

	    case US_STATE_DISK_WRITE:
		    // Feed the current sink        
		    for (i = 0; i < cnt; i++) {
			    us->iobuf[us->ibuf_wp] = data[i];
			    us->ibuf_wp++;
			    if (us->ibuf_wp == 512) {
				    if (disk_write(DRIVE_NR(us), us->iobuf, us->write_lba, 1) !=
					RES_OK) {
					    SC_SetSense(us, SK_MEDIUM_ERROR, 0, ASQ_WRITE_FAULT, 0);
					    Con_Printf("*********** Write error\n");
					    us->state = US_STATE_IDLE;
					    csw->bCSWStatus = CSWS_Failed;
					    csw->dCSWDataResidue = us->expected_bytes;
					    UsbStor_CswIn(us, (uint8_t *) csw, sizeof(*csw));
					    break;
				    }
				    us->ibuf_wp = 0;
				    us->write_lba++;
			    }
		    }
		    us->expected_bytes -= cnt;
		    if (us->expected_bytes == 0) {
			    csw->dCSWDataResidue = host32_to_le(0);
			    UsbStor_CswIn(us, (uint8_t *) csw, sizeof(*csw));
			    us->state = US_STATE_IDLE;
		    }
		    break;
	    default:
		    Con_Printf("Illegal state %u of USB storage device\n", us->state);
		    break;
	}
}

/**
 *********************************************************************************
 * String descriptors have no templates. They are composed at runtime here.
 *********************************************************************************
 */
static UsbTaRes
Usb_GetStringDescr(void *evData, uint8_t epNr, UsbSetupBuffer * sb)
{
	int index = sb->wValue & 0xff;
	uint8_t *reply = sb->setupData;
	int maxlen = sb->wLength;
	dbgprintf("Get String descriptor %u\n", index);
	switch (index) {
	    case 0:		/* Special case: return Languages */
		    sb->dataLen = *reply++ = 4;
		    *reply++ = USB_DT_STRING;
		    *reply++ = 0x09;
		    *reply++ = 0x04;	/* English USA */
		    break;

	    case 1:		/* iManufacturer        */
		    sb->dataLen = UsbDev_StrToDescr(reply, "Munich-Instruments", maxlen);
		    break;

	    case 2:		/* iProductId           */
		    sb->dataLen = UsbDev_StrToDescr(reply, "C-Bert", maxlen);
		    break;

	    case 3:		/* iSerialNumber        */
		    sb->dataLen = UsbDev_StrToDescr(reply, "00000000001", maxlen);
		    break;

	    case 4:		/* iConfiguration       */
		    sb->dataLen = UsbDev_StrToDescr(reply, "ConfigABC", maxlen);
		    break;
	    default:
		    return USBTA_ERROR;
	}
	return USBTA_OK;
}

static UsbDescriptorHandler strDescrHandlers[5] = {
	{
	 .keyUsbValue = ((uint16_t) USB_DT_STRING << 8) | 0,
	 .getDescriptor = Usb_GetStringDescr,
	 .setDescriptor = NULL,
	 },
	{
	 .keyUsbValue = ((uint16_t) USB_DT_STRING << 8) | 1,
	 .getDescriptor = Usb_GetStringDescr,
	 .setDescriptor = NULL,
	 },
	{
	 .keyUsbValue = ((uint16_t) USB_DT_STRING << 8) | 2,
	 .getDescriptor = Usb_GetStringDescr,
	 .setDescriptor = NULL,
	 },
	{
	 .keyUsbValue = ((uint16_t) USB_DT_STRING << 8) | 3,
	 .getDescriptor = Usb_GetStringDescr,
	 .setDescriptor = NULL,
	 },
	{
	 .keyUsbValue = ((uint16_t) USB_DT_STRING << 8) | 4,
	 .getDescriptor = Usb_GetStringDescr,
	 .setDescriptor = NULL,
	 },
};

/**
 **************************************************************************************
 * static UsbTaRes UsbStor_GetMaxLun(void *evData,uint8_t epNr,UsbSetupBuffer *sb)
 * Device class specific request for reading the max lun.
 **************************************************************************************
 */
static UsbTaRes
UsbStor_GetMaxLun(void *evData, uint8_t epNr, UsbSetupBuffer * sb)
{
	sb->setupData[0] = 0;
	sb->dataLen = 1;
	return USBTA_OK;
}

static UsbRequestHandler getMaxLunHandler = {
	.keyRqRqt = 0xfea1,
	.usbDoRequest = UsbStor_GetMaxLun,
};

static UsbTaRes
UsbStor_clearFeature(void *evData, uint8_t epNr, UsbSetupBuffer * sb)
{
	uint16_t featureSelector = sb->wValue;
	Con_Printf("CLEAR Feature Selector %04x not implemented \n", featureSelector);
	switch (featureSelector) {
	    case 0:		/* Endpoint halt */
		    // EndpointUnhalt();
		    break;
	    default:
		    break;
	}
	return USBTA_ERROR;
}

static UsbRequestHandler clearFeature0 = {
	.keyRqRqt = 0x0100,
	.usbDoRequest = UsbStor_clearFeature,
};

static UsbRequestHandler clearFeature1 = {
	.keyRqRqt = 0x0101,
	.usbDoRequest = UsbStor_clearFeature,
};

static UsbRequestHandler clearFeature2 = {
	.keyRqRqt = 0x0102,
	.usbDoRequest = UsbStor_clearFeature,
};

static UsbTaRes
UsbStor_setFeature(void *evData, uint8_t epNr, UsbSetupBuffer * sb)
{
	uint16_t featureSelector = sb->wValue;
	Con_Printf("SET Feature not implemented\n");
	switch (featureSelector) {
	    case 0:		/* Endpoint Halt */
		    //EndpointHalt();
		    break;
	    default:
		    break;
	}
	return USBTA_ERROR;
}

/**
 **********************************************************
 * Device class specific request reset:
 **********************************************************
 */
static UsbTaRes
UsbStor_Reset(void *evData, uint8_t epNr, UsbSetupBuffer * sb)
{
	Con_Printf("Got Mass storage class specific USB request Reset\n");
	return USBTA_OK;
}

static UsbRequestHandler setFeature0 = {
	.keyRqRqt = 0x0300,
	.usbDoRequest = UsbStor_setFeature,
};

static UsbRequestHandler setFeature1 = {
	.keyRqRqt = 0x0301,
	.usbDoRequest = UsbStor_setFeature,
};

static UsbRequestHandler setFeature2 = {
	.keyRqRqt = 0x0302,
	.usbDoRequest = UsbStor_setFeature,
};

static UsbRequestHandler rqReset = {
	.keyRqRqt = 0xff21,
	.usbDoRequest = UsbStor_Reset,
};

/**
 ********************************************************************
 * State changes must be forwarded to filesystem shutdowner
 ********************************************************************
 */
static void
UsbDevStateChange(void *eventData, UsbDevState devState)
{
	switch (devState) {
	    case DevState_Default:
	    case DevState_Address:
	    case DevState_Configured:
		    //Shutdown_SendEvent(SHUTDOWN_FILESYSTEM, true);
		    break;

	    case DevState_SuspPwr:
	    case DevState_SuspDef:
	    case DevState_SuspAddr:
	    case DevState_SuspConf:
		    //Shutdown_SendEvent(SHUTDOWN_FILESYSTEM, false);
		    break;
		    /* do nothing in current HW */
	    case DevState_Powered:
		    break;

	    case DevState_Attached:
		    break;
	}
}

/**
 ********************************************************************
 * Make the unit ready after disk spin up ( = SD Card init )
 ********************************************************************
 */
void
UsbStor_UnitReady(bool state)
{
	UsbStor *us = &gUsbStor;
	if (state == true) {
		us->driveNr = 0;
	}
	us->unitReady = state;
}

/**
 ***********************************************************
 * Initialize a storage device
 ***********************************************************
 */
void
UsbStor_Init()
{
	uint16_t i;
	UsbStor *us = &gUsbStor;
	UsbDev *ud = UsbDev_Init();
	if (!ud) {
		Con_Printf("Error, can not get USB device\n");
		return;
	}
	us->usbDev = ud;
	us->unitReady = false;
	UsbDev_RegisterDescriptorHandler(ud, EP0, &devDescrHandler, us);
	UsbDev_RegisterDescriptorHandler(ud, EP0, &confDescrHandler, us);
	for (i = 0; i < array_size(strDescrHandlers); i++) {
		UsbDev_RegisterDescriptorHandler(ud, EP0, &strDescrHandlers[i], us);
	}
	UsbDev_RegisterEpHandler(ud, EP2, UsbStor_EPOut, us);
	UsbDev_RegisterDevStateCB(ud, UsbDevStateChange, us);

	UsbDev_RegisterRequestHandler(ud, EP0, &getMaxLunHandler, us);
	UsbDev_RegisterRequestHandler(ud, EP0, &setFeature0, us);
	UsbDev_RegisterRequestHandler(ud, EP0, &setFeature1, us);
	UsbDev_RegisterRequestHandler(ud, EP0, &setFeature2, us);
	UsbDev_RegisterRequestHandler(ud, EP0, &clearFeature0, us);
	UsbDev_RegisterRequestHandler(ud, EP0, &clearFeature1, us);
	UsbDev_RegisterRequestHandler(ud, EP0, &clearFeature2, us);
	UsbDev_RegisterRequestHandler(ud, EP0, &rqReset, us);
	//UsbDev_ConfigureEndpoint(ud,EP1,inendpoint);
}
