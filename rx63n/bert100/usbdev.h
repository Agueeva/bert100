/**
 **********************************************************************************
 * Taken from the freeware embedded System simulator softgun
 **********************************************************************************
 */
#include "types.h"
/*
 * Descriptor types bits from USB 2.0 spec table 9.5
 */
#define USB_DT_DEVICE                   0x01
#define USB_DT_CONFIG                   0x02
#define USB_DT_STRING                   0x03
#define USB_DT_INTERFACE                0x04
#define USB_DT_ENDPOINT                 0x05
#define USB_DT_DEVICE_QUALIFIER         0x06
#define USB_DT_OTHER_SPEED_CONFIG       0x07
#define USB_DT_INTERFACE_POWER          0x08

/* these are from a minor usb 2.0 revision (ECN) */
#define USB_DT_OTG                      0x09
#define USB_DT_DEBUG                    0x0a
#define USB_DT_INTERFACE_ASSOCIATION    0x0b

/* CDC Table 24 */
#define USB_DT_CS_INTERFACE			(0x24)
#define	USB_DT_CS_ENDPOINT			(0x25)

/**
 * Some values can only be used at device level, others only at inteface level 
 */
#define USB_CLASS_INTERFACE	(0x00)	/* Defined at interface level */
#define USB_CLASS_AUDIO		(0x01)	/* I */
#define USB_CLASS_CDC		(0x02)	/* ID */
#define USB_CLASS_HID		(0x03)	/* I */
#define USB_CLASS_PHYSICAL	(0x05)	/* I */
#define USB_CLASS_IMAGE		(0x06)	/* I */
#define USB_CLASS_PRINTER	(0x07)	/* I */
#define USB_CLASS_STORAGE	(0x08)	/* I */
#define USB_CLASS_HUB		(0x09)	/* D */
#define USB_CLASS_CDCDATA	(0x0a)	/* I */
#define USB_CLASS_SMARTCARD	(0x0b)	/* I */
#define USB_CLASS_CONTSEC	(0x0d)	/* I */
#define USB_CLASS_VIDEO	    	(0x0e)	/* I */
#define USB_CLASS_PHEALTHCARE	(0x0f)	/* I */
#define USB_CLASS_AUDIOVIDEO	(0x10)	/* I */
#define USB_CLASS_DIAGNOSTIC	(0xDC)	/* ID */
#define USB_CLASS_WIRELESS	(0xE0)	/* I */
#define USB_CLASS_MISC		(0xEF)	/* ID */
#define USB_CLASS_VENDOR_SPEC	(0xFF)	/* ID */

/*
 * Wireless USB spec
 */
#define USB_DT_SECURITY                 0x0c
#define USB_DT_KEY                      0x0d
#define USB_DT_ENCRYPTION_TYPE          0x0e
#define USB_DT_BOS                      0x0f
#define USB_DT_DEVICE_CAPABILITY        0x10
#define USB_DT_WIRELESS_ENDPOINT_COMP   0x11

/**
 *******************************************************************
 * Result codes for USB transactions.
 *******************************************************************
 */
typedef enum UsbTaRes {
	USBTA_OK = 0,
	USBTA_ERROR = -1,
	USBTA_IGNORED = -2,
	USBTA_NOTHANDLED = -3
} UsbTaRes;

typedef struct UsbSetupBuffer {
	/*
	 *******************************************************************************
	 * The following fileds are filled  with the 8 Byte Setup packet if the request
	 * is a control request
	 *******************************************************************************
	 */
	uint8_t bmRequestType;
	uint8_t bRequest;
	uint16_t wValue;
	uint16_t wIndex;
	uint16_t wLength;
	uint8_t setupData[80];	/* ACM uses 75 byte setup Buffer */
	uint8_t dataLen;
} UsbSetupBuffer;

typedef struct UsbDev UsbDev;

/**
 *************************************************************
 * USB device descriptor 
 *************************************************************
 */
typedef struct usb_device_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_le bcdUSB;
	uint8_t bDeviceClass;
	uint8_t bDeviceSubClass;
	uint8_t bDeviceProtocol;
	uint8_t bMaxPacketSize0;
	uint16_le idVendor;
	uint16_le idProduct;
	uint16_le bcdDevice;
	uint8_t iManufacturer;
	uint8_t iProduct;
	uint8_t iSerialNumber;
	uint8_t bNumConfigurations;
} __attribute__ ((packed)) usb_device_descriptor;

typedef struct usb_config_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;

	uint16_le wTotalLength;
	uint8_t bNumInterfaces;
	uint8_t bConfigurationValue;
	uint8_t iConfiguration;
	uint8_t bmAttributes;
	uint8_t bMaxPower;
} __attribute__ ((packed)) usb_config_descriptor;

typedef struct usb_string_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_le wData[1];	/* UTF-16LE encoded */
} __attribute__ ((packed)) usb_string_descriptor;

/*
 ***************************************************************************** 
 * Interface descriptor is returned as part of config descriptor
 * and can not directly accesses with GetDescr, SetDescr
 ***************************************************************************** 
 */
typedef struct usb_interface_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bInterfaceNumber;
	uint8_t bAlternateSetting;
	uint8_t bNumEndpoints;
	uint8_t bInterfaceClass;
	uint8_t bInterfaceSubClass;
	uint8_t bInterfaceProtocol;
	uint8_t iInterface;
} __attribute__ ((packed)) usb_interface_descriptor;

/* USB_DT_ENDPOINT: Endpoint descriptor */
typedef struct usb_endpoint_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;

	uint8_t bEndpointAddress;
	uint8_t bmAttributes;
	uint16_le wMaxPacketSize;
	uint8_t bInterval;

	/* NOTE:  these two are _only_ in audio endpoints. */
	/* use USB_DT_ENDPOINT*_SIZE in bLength, not sizeof. */
	uint8_t bRefresh;
	uint8_t bSynchAddress;
} __attribute__ ((packed)) usb_endpoint_descriptor;

#define USB_DT_ENDPOINT_SIZE            7
#define USB_DT_ENDPOINT_AUDIO_SIZE      9	/* Audio extension */

/* USB_DT_DEVICE_QUALIFIER: Device Qualifier descriptor */
typedef struct usb_qualifier_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_le bcdUSB;
	uint8_t bDeviceClass;
	uint8_t bDeviceSubClass;
	uint8_t bDeviceProtocol;
	uint8_t bMaxPacketSize0;
	uint8_t bNumConfigurations;
	uint8_t bRESERVED;
} __attribute__ ((packed)) usb_qualifier_descriptor;

/**
 * 
 */
typedef struct usb_interface_assoc_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;	/* 0x0b Interface Assoc. */
	uint8_t bFirstInterface;	/* First IF associated with function */
	uint8_t bInterfaceCount;	/* Number of contiguos interface associated with function */
	uint8_t bFunctionClass;
	uint8_t bFunctionSubClass;
	uint8_t bFunctionProtocol;
	uint8_t iFunction;	/* Index of string descriptor describing this function */
} __attribute__ ((packed)) usb_interface_assoc_descriptor;

/**
 *****************************************************
 * CDC 5.2.3.1
 *****************************************************
 */
typedef struct usb_header_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;	/* CS_INTERFACE */
	uint8_t bDescriptorSubType;
	uint16_le bcdCDC;
} __attribute__ ((packed)) usb_header_descriptor;

/**
 ******************************************************************
 * CDC 5.2.3.2 Call Management Functional Descriptor
 ******************************************************************
 */

typedef struct usb_call_management_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;	/* CS_INTERFACE */
	uint8_t bDescriptorSubtype;
	uint8_t bmCapabilities;
	uint8_t bDataInterface;
} __attribute__ ((packed)) usb_call_management_descriptor;
/*
 ******************************************************
 * CDC 5.2.3.3
 ******************************************************
 */
typedef struct usb_acm_management_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;	/* CS_INTERFACE */
	uint8_t bDescriptorSubtype;
	uint8_t bmCapabilities;
} __attribute__ ((packed)) usb_acm_management_descriptor;

/**
 ************************************************************
 * CDC 5.2.3.8 Table 33
 * Variable sized, but currently only used with one
 * slave, so I use fixed size
 ************************************************************
 */
typedef struct usb_union_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bMasterInterface;
	uint8_t bSlaveInterface0;
} __attribute__ ((packed)) usb_union_descriptor;
/* 
 ***********************************************************
 * USB_DT_OTG (from OTG 1.0a supplement) 
 ***********************************************************
 */
typedef struct usb_otg_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bmAttributes;	/* support for HNP, SRP, etc */
} __attribute__ ((packed)) usb_otg_descriptor;

/* USB_DT_DEBUG:  for special highspeed devices, replacing serial console */
typedef struct usb_debug_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	/* bulk endpoints with 8 byte maxpacket */
	uint8_t bDebugInEndpoint;
	uint8_t bDebugOutEndpoint;
} usb_debug_descriptor;

typedef UsbTaRes UsbDescriptorProc(void *eventData, uint8_t epNr, UsbSetupBuffer *);

typedef struct UsbDescriptorHandler {
	uint16_t keyUsbValue;
	UsbDescriptorProc *getDescriptor;
	UsbDescriptorProc *setDescriptor;
	void *eventData;
	/* Its a linked list with queueHead in USBDevice */
	struct UsbDescriptorHandler *next;
} UsbDescriptorHandler;

void UsbDev_RegisterDescriptorHandler(UsbDev *, uint8_t epNr, UsbDescriptorHandler *, void *evData);

typedef UsbTaRes UsbRequestProc(void *eventData, uint8_t epNr, UsbSetupBuffer *);

typedef struct UsbRequestHandler {
	uint16_t keyRqRqt;	/* High byte request, Low byte request type */
	UsbRequestProc *usbDoRequest;
	void *eventData;
	struct UsbRequestHandler *next;
} UsbRequestHandler;

typedef void UsbEpHandlerProc(void *eventData, const uint8_t * data, uint16_t len);

void UsbDev_RegisterRequestHandler(UsbDev *, uint8_t epNr, UsbRequestHandler *, void *evData);
void UsbDev_RegisterEpHandler(UsbDev * ud, uint8_t epNr, UsbEpHandlerProc * proc, void *evData);
void UsbDev_EpInTransmit(UsbDev * ud, uint8_t epNr, uint8_t * data, uint16_t len);

typedef enum {
	DevState_Attached,
	DevState_Powered,
	DevState_Default,
	DevState_Address,
	DevState_Configured,
	DevState_SuspPwr,
	DevState_SuspDef,
	DevState_SuspAddr,
	DevState_SuspConf,
} UsbDevState;

typedef void UsbStateChangeCB(void *eventData, UsbDevState state);
void UsbDev_RegisterDevStateCB(UsbDev *, UsbStateChangeCB, void *eventData);
int  UsbDev_StrToDescr(uint8_t * dst, const char *src, int maxlen);


bool UsbDev_CheckConnected(void);
UsbDev *UsbDev_Init(void);
