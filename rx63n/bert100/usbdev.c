/**
 ******************************************************************************
 * Usb device for Renesas RX 
 ******************************************************************************
 */
#include <string.h>
#include "iodefine.h"
#include "types.h"
#include "interrupt_handlers.h"
#include "usbdev.h"
#include "events.h"
#include "console.h"
#include "byteorder.h"
#include "timer.h"
#include "tpos.h"
#include "interpreter.h"
#include "hex.h"

#if 1 
#define dbgprintf(x...) Con_Printf(x)
#else
#define dbgprintf(x...) 
#endif

#define EP0 0
#define USB_IPL	1
/*
 ****************************************************************************
 * Standard requests, for the bRequest field of a SETUP packet.
 * These requests are valid only for Type 0 Requests
 * (bmRequestType bits 5+6).
 * See table 9-4. in USB 2.0 spec
 ****************************************************************************
 */
#define USBRQ_GET_STATUS              0x00
#define USBRQ_CLEAR_FEATURE           0x01
#define USBRQ_SET_FEATURE             0x03
#define USBRQ_SET_ADDRESS             0x05
#define USBRQ_GET_DESCRIPTOR          0x06
#define USBRQ_SET_DESCRIPTOR          0x07
#define USBRQ_GET_CONFIGURATION       0x08
#define USBRQ_SET_CONFIGURATION       0x09
#define USBRQ_GET_INTERFACE           0x0A
#define USBRQ_SET_INTERFACE           0x0B
#define USBRQ_SYNCH_FRAME             0x0C

#define         CTSQ_IDST       (0)	/* Idle or setup stage */
#define         CTSQ_RDDS       (1)	/* Ctrl read data stage */
#define         CTSQ_RDSS       (2)	/* Ctrl read status stage */
#define         CTSQ_WRDS       (3)	/* Ctrl write data stage */
#define         CTSQ_WRSS       (4)	/* Ctrl write status stage */
#define		CTSQ_WRNDSS	(5)	/* Ctrl write (no data) status stage */
#define		CTSQ_TSQERR	(6)	/* Ctrl transfer sequence error */

#define		DVSQ_POWERED	(0)
#define 	DVSQ_DEFAULT	(1)
#define		DVSQ_ADDRESS	(2)
#define 	DVSQ_CONFIGURED	(3)
#define		DVSQ_SUSP_PWR	(4)
#define		DVSQ_SUSP_DEF	(5)
#define		DVSQ_SUSP_ADDR	(6)
#define		DVSQ_SUSP_CONF	(7)

#define         INTSTS0_VBINT           (uint16_t)(1 << 15)
#define         INTSTS0_RESM            (UINT16_C(1) << 14)
#define         INTSTS0_SOFR            (UINT16_C(1) << 13)
#define         INTSTS0_DVST            (UINT16_C(1) << 12)
#define         INTSTS0_CTRT            (UINT16_C(1) << 11)
#define         INTSTS0_BEMP            (UINT16_C(1) << 10)
#define         INTSTS0_NRDY            (UINT16_C(1) << 9)
#define         INTSTS0_BRDY            (UINT16_C(1) << 8)
#define         INTSTS0_VBSTS           (UINT16_C(1) << 7)
#define         INTSTS0_DVSQ_MSK        (UINT16_C(7) << 4)
#define         INTSTS0_VALID           (UINT16_C(1) << 3)
#define         INTSTS0_CTSQ_MSK        (UINT16_C(7))

#define		INTSTS1_OVRCR		(1 << 15)
#define		INTSTS1_BCHG		(1 << 14)
#define		INTSTS1_DTCH		(1 << 12)
#define		INTSTS1_ATTCH		(1 << 11)
#define		INTSTS1_EOFERR		(1 << 6)
#define		INTSTS1_SIGN		(1 << 5)
#define		INTSTS1_SACK		(1 << 4)

#define         FIFOSEL_RCNT            (1 << 15)
#define         FIFOSEL_REW             (1 << 14)
#define         FIFOSEL_MBW             (1 << 10)
#define         FIFOSEL_BIGEND          (1 << 8)
#define         FIFOSEL_ISEL            (1 << 5)
#define         FIFOSEL_CURPIPE_MSK     (0xf)

#define         FIFOCTR_BVAL    (1 << 15)
#define         FIFOCTR_BCLR    (1 << 14)
#define         FIFOCTR_FRDY    (1 << 13)

#define		PID_NAK		(0)
#define		PID_BUF 	(1)
#define		PID_STALL	(2)
#define		PID_STALL2	(3)

#define INPKT_FIFO_SIZE		(8)
#define INPKT_RP(ud) ((ud)->inPktRp % INPKT_FIFO_SIZE)	  /**< The fifo readpointer.  */
#define INPKT_WP(ud) ((ud)->inPktWp % INPKT_FIFO_SIZE)	  /**< The fifo writepointer. */
#define INPKT_CNT(ud) (((ud)->inPktWp - (ud)->inPktRp)  & 0xff)	  /**< The fifo writepointer. */
#define BEMPSTS_INTERRUPT	1

typedef struct UsbInPkt {
	uint8_t data[64];
	uint16_t len;
} UsbInPkt;

struct UsbDev {
	Mutex usbMutex;
	Event evCtrl;
	Event evDevState;
	Event evBrdy;
	uint16_t brdysts;
	UsbDescriptorHandler *ep0DescrHead;
	UsbRequestHandler *ep0RqHandlerHead;
	UsbSetupBuffer ep0Buf;
	UsbEpHandlerProc *epHandlerProc[4];

	UsbStateChangeCB *devStateCB;
	void *devStateCBData;

	void *epHandlerData[4];
	uint8_t epOutBuf[256];	/* Size of biggest fifo in Renesas RX */

#if BEMPSTS_INTERRUPT
	UsbInPkt inPktBuf[INPKT_FIFO_SIZE];
	uint8_t inPktWp;	/* Written by enqueue */
	uint8_t inPktRp;	/* Written by the interrupt handler */
	bool inFifoIdle;
#endif
	/* Statistics */
	uint32_t statBEMP;
	uint32_t statBCHG;
	uint32_t statATTCH;
	uint32_t statDTCH;
	uint32_t statDVST;
	uint32_t statVBINT;
	uint32_t statCTRT;
	uint32_t statBRDY;
};

static UsbDev gUsb0;

/**
 ************************************************************************************
 * \fn int Usb_StrToDescr(uint8_t * dst, const char *src, int maxlen)
 * Helper function for conversion of ascii strings to USB strings.
 ************************************************************************************
 */
int
UsbDev_StrToDescr(uint8_t * dst, const char *src, int maxlen)
{
        int i;
        int len = strlen(src);
        uint8_t *d = dst;
        if (maxlen < 2) {
                return 0;
        }
        *d++ = 2 + (len << 1);
        *d++ = USB_DT_STRING;
        for (i = 0; (i < len) && i < (maxlen - 2); i++) {
                *d++ = src[i];
                *d++ = 0;
        }
        return d - dst;
}

/**
 ******************************************************************************
 * \fn static UsbDescriptorHandler *find_descriptor_handler(UsbDev *,uint8_t epNr,uint8_t dt,uint8_t index); 
 ******************************************************************************
 */
static UsbDescriptorHandler *
find_descriptor_handler(UsbDev * dev, uint8_t epNr, uint16_t key)
{
	UsbDescriptorHandler *cursor;
	if (epNr != 0) {
		Con_Printf("Non 0 control endpoints not implemented\n");
		return NULL;
	}
	for (cursor = dev->ep0DescrHead; cursor; cursor = cursor->next) {
		if (key == cursor->keyUsbValue) {
			return cursor;
		}
	}
	Con_Printf("descriptor 0x%04x not found\n", key);
	return NULL;
}

/**
 ***********************************************************************************************
 * \fn static UsbRequestHandler *find_request_handler(UsbDev *dev,uint8_t epNr,uint16_t key) 
 ***********************************************************************************************
 */
static UsbRequestHandler *
find_request_handler(UsbDev * dev, uint8_t epNr, uint16_t key)
{
	UsbRequestHandler *cursor;
	if (epNr != 0) {
		Con_Printf("Non 0 control endpoints not implemented\n");
		return NULL;
	}
	for (cursor = dev->ep0RqHandlerHead; cursor; cursor = cursor->next) {
		if (key == cursor->keyRqRqt) {
			return cursor;
		}
	}
	Con_Printf("No request found for RqRqt 0x%04x\n", key);
	return NULL;
}

static inline void
WritePipe(UsbDev * ud, uint8_t pipeNr, uint8_t * buf, uint16_t cnt)
{
	uint16_t i;
	/* Some registers should not be set when PID_BUF */
	//USB0.PIPE1CTR.BIT.PID = PID_NAK;
	USB0.D1FIFOSEL.WORD = pipeNr | FIFOSEL_MBW;
	USB0.BEMPSTS.WORD = ~(UINT16_C(1) << pipeNr);
	for (i = 0; i < (cnt & ~1); i += 2) {
		USB0.D1FIFO.WORD = *(uint16_t *) (buf + i);
	}
	if (cnt & 1) {
		USB0.D1FIFO.BYTE.L = buf[cnt - 1];
	}
	USB0.D1FIFOCTR.WORD = FIFOCTR_BVAL;
	USB0.PIPE1CTR.BIT.PID = PID_BUF;
}

/**
 ***********************************************************************************
 * Write one buffer to the control fifo. The control fifo has 64 bytes.  
 * So the packet needs to be split if it is larger.
 ***********************************************************************************
 */
static void
WriteCtrlBuf(UsbDev * ud, uint8_t * buf, uint16_t cnt)
{
	uint16_t i;
	if (cnt > 64) {
		Con_Printf("Usb Bug: Writing more than 64 bytes to the control fifo\n");
		return;
	}
	USB0.CFIFOSEL.WORD = FIFOSEL_MBW;
	USB0.CFIFOSEL.WORD = FIFOSEL_ISEL | FIFOSEL_MBW;
	for (i = 0; i < 10; i++) {
		if (USB0.CFIFOCTR.BIT.FRDY) {
			break;
		}
	}
	for (i = 0; i < (cnt & ~1); i += 2) {
		USB0.CFIFO.WORD = *(uint16_t *) (buf + i);
	}
	if (cnt & 1) {
		USB0.CFIFO.BYTE.L = buf[cnt - 1];
	}
	USB0.CFIFOCTR.WORD = FIFOCTR_BVAL;
	USB0.DCPCTR.BIT.PID = PID_BUF;
}

/**
 ******************************************************************* 
 * Write to the control fifo.
 * Splits the data into packets of max. 64 bytes.
 * The Control fifo has no double buffering.
 ******************************************************************* 
 */
static void
WriteControlFifo(UsbDev * dev, uint8_t * buf, uint16_t cnt)
{
	uint16_t i;
	uint16_t wsize;
	USB0.CFIFOCTR.WORD = FIFOCTR_BCLR;
	USB0.BEMPSTS.WORD = ~(UINT16_C(1) << 0);
	USB0.BEMPENB.WORD = (1 << 0);
	while (cnt > 0) {
		wsize = (cnt > 64) ? 64 : cnt;
		WriteCtrlBuf(dev, buf, wsize);
		for (i = 5; i > 0; i--) {
			if (USB0.BEMPSTS.WORD & 1) {
				break;
			}
			SleepMs(1);
		}
		if (i == 0) {
			Con_Printf("Timeout BEMPSTS 0x%04x\n", USB0.BEMPSTS.WORD);
			Con_Printf("WP %u\n", gUsb0.inPktWp);
			break;
		}
		cnt -= wsize;
		buf += wsize;
	}
	//Con_Printf("USBADDR 0x%04x\n",USB0.USBADDR.WORD);
}

static void
StallControlFifo(UsbDev * dev)
{
	Con_Printf("STALL\n");
	USB0.DCPCTR.BIT.PID = PID_STALL;
}

/**
 ******************************************************************
 * Register a descriptor handler. A descriptor handler belongs to
 * a control endpoint.
 ******************************************************************
 */
void
UsbDev_RegisterDescriptorHandler(UsbDev * dev, uint8_t epNr, UsbDescriptorHandler * dh,
				 void *evData)
{
	if (epNr != 0) {
		Con_Printf("Non 0 control endpoints not supported\n");
		return;
	}
	dh->eventData = evData;
	dh->next = dev->ep0DescrHead;
	dev->ep0DescrHead = dh;
	//Con_Printf("Registered a descriptor handler for %04x\n",dh->keyUsbValue);
}

/**
 *******************************************************************************************************
 * \fn void UsbDev_RegisterRequestHandler(UsbDev *dev,uint8_t epNr,UsbRequestHandler *rh,void *evData) 
 *******************************************************************************************************
 */
void
UsbDev_RegisterRequestHandler(UsbDev * dev, uint8_t epNr, UsbRequestHandler * rh, void *evData)
{
	if (epNr != 0) {
		Con_Printf("Non 0 control endpoints not supported\n");
		return;
	}
	rh->eventData = evData;
	rh->next = dev->ep0RqHandlerHead;
	dev->ep0RqHandlerHead = rh;
}

/**
 ****************************************************************************************
 * \fn static void PrintControlStage(uint8_t ctsq) 
 ****************************************************************************************
 */

static void
PrintControlStage(uint8_t ctsq)
{
	char *str;
	switch (ctsq) {
	    case CTSQ_IDST:
		    str = "Idle/Setup";
		    break;
	    case CTSQ_RDDS:
		    str = "Read Data";
		    break;
	    case CTSQ_RDSS:
		    str = "Read Status";
		    break;
	    case CTSQ_WRDS:
		    str = "Write Data";
		    break;
	    case CTSQ_WRSS:
		    str = "Write Status";
		    break;
	    case 5:
		    str = "Write No Data Status";
		    break;
	    case 6:
		    str = "Transfer Seq error";
		    break;
	    default:
		    str = "Unknown stage";
		    break;
	}
	dbgprintf("CTSQ %s\n", str);
}

/**
 ********************************************************************
 *
 ********************************************************************
 */
static bool
ReadSetupPacket(UsbDev * ud, UsbSetupBuffer * sb)
{
	uint16_t usbreq;
	uint16_t intsts0;
	intsts0 = USB0.INTSTS0.WORD;
	if (intsts0 & INTSTS0_VALID) {
		USB0.INTSTS0.WORD = ~INTSTS0_VALID;
		usbreq = le16_to_host(USB0.USBREQ.WORD);
		sb->wValue = le16_to_host(USB0.USBVAL);
		sb->wIndex = le16_to_host(USB0.USBINDX);
		sb->wLength = le16_to_host(USB0.USBLENG);
		sb->bmRequestType = usbreq & 0xff;
		sb->bRequest = (usbreq >> 8) & 0xff;
		dbgprintf("Value %04x wIndex %04x wLength %04x, REQ %04x\n", sb->wValue,
			   sb->wIndex, sb->wLength, usbreq);
		return true;
	} else {
		return false;
	}
}

static void
ReadFromCfifo(UsbDev * ud, UsbSetupBuffer * sb)
{
	Con_Printf("Read from CFIFO\n");

}

/**
 **********************************************************************
 * \fn static void HandleUsbRequest(UsbDev *ud,UsbSetupBuffer *sb) 
 **********************************************************************
 */
static void
HandleUsbRequest(UsbDev * ud, UsbSetupBuffer * sb)
{
	UsbDescriptorHandler *udh;
	UsbRequestHandler *rqh;
	UsbTaRes taRes;
	uint16_t rqrqt = ((uint16_t) sb->bRequest << 8) | sb->bmRequestType;
	switch (sb->bRequest) {
	    case USBRQ_GET_DESCRIPTOR:
		    /*  USBLENG is the maximum length for the data phase in this case */
		    udh = find_descriptor_handler(ud, EP0, sb->wValue);
		    if (udh && udh->getDescriptor) {
			    taRes = udh->getDescriptor(udh->eventData, EP0, sb);
			    if (taRes == USBTA_OK) {
				    dbgprintf("datalen %u, wlen %u\n", sb->dataLen, sb->wLength);
				    if (sb->dataLen > sb->wLength) {
					    sb->dataLen = sb->wLength;
				    }
				    WriteControlFifo(ud, sb->setupData, sb->dataLen);
			    }
		    }
		    break;

	    case USBRQ_SET_DESCRIPTOR:
		    dbgprintf("SetDescriptor\n");
		    udh = find_descriptor_handler(ud, EP0, sb->wValue);
		    if (udh && udh->setDescriptor) {
			    taRes = udh->setDescriptor(udh->eventData, EP0, sb);
		    } else {
			    Con_Printf("No Set handler for %04x\n", sb->wValue);
		    }
		    break;

	    case USBRQ_SET_CONFIGURATION:
		    /* Does this initialize the toggle ??? */
		    USB0.PIPE2CTR.BIT.SQCLR = 1;
		    USB0.PIPE1CTR.BIT.SQCLR = 1;
		    USB0.PIPE2CTR.BIT.PID = PID_BUF;
		    dbgprintf("Set configuration\n");
		    break;

	    default:
		    rqh = find_request_handler(ud, EP0, rqrqt);
		    if (rqh && rqh->usbDoRequest) {
			    taRes = rqh->usbDoRequest(rqh->eventData, EP0, sb);
			    if (taRes == USBTA_OK) {
				    if (sb->dataLen > sb->wLength) {
					    sb->dataLen = sb->wLength;
				    }
				    WriteControlFifo(ud, sb->setupData, sb->dataLen);
			    }
		    } else {
			    Con_Printf("********* Missing request handler for 0x%04x\n", rqrqt);
			    StallControlFifo(ud);
		    }
		    break;
	}
}

/**
 ***********************************************************************************
 * Handle a control transfer stage transition
 * The USB module always sends an Ack response for a correct setup packet 
 * targeted to the USB module. See 28.3.6.2. It sets VALID to 1, PID in DCPCTR to
 * NAK and CCPL in DCPCTR to 0. Response set
 ***********************************************************************************
 */
static void
UsbCtrtEvent(void *eventData)
{
	UsbDev *ud = &gUsb0;
	UsbSetupBuffer *sb;
	static int ctsq_old = -1;
	uint16_t intsts0;
	uint16_t ctsq;
	Mutex_Lock(&ud->usbMutex);
	sb = &ud->ep0Buf;

	intsts0 = USB0.INTSTS0.WORD;
	ctsq = intsts0 & INTSTS0_CTSQ_MSK;
	//Con_Printf("Got an usb event, instst0 0x%04x\n",intsts0);
	if (ctsq == ctsq_old) {
		Con_Printf("CTSQ %d is equal to CTSQold ******\n", ctsq);
		//return;
	} else {
		dbgprintf("CTSQ old %d ", ctsq_old);
	}
	ctsq_old = ctsq;
	PrintControlStage(ctsq);
	switch (ctsq) {
	    case CTSQ_IDST:
		    break;

	    case CTSQ_RDDS:
		    ReadSetupPacket(ud, sb);
		    HandleUsbRequest(ud, sb);
		    break;

	    case CTSQ_RDSS:
		    USB0.DCPCTR.BIT.PID = PID_BUF;
		    USB0.DCPCTR.BIT.CCPL = 1;
		    break;

	    case CTSQ_WRDS:
		    ReadSetupPacket(ud, sb);
		    break;

	    case CTSQ_WRSS:
		    ReadFromCfifo(ud, sb);
		    break;

	    case CTSQ_WRNDSS:
		    ReadSetupPacket(ud, sb);
		    HandleUsbRequest(ud, sb);
		    /*
		     ***********************************************************
		     * Probably we should check the result of the request before
		     * successing it.
		     ***********************************************************
		     */
		    USB0.DCPCTR.BIT.PID = PID_BUF;
		    USB0.DCPCTR.BIT.CCPL = 1;
		    break;

	    case CTSQ_TSQERR:
		    break;

	    default:
		    break;
	}
	Mutex_Unlock(&ud->usbMutex);
}

static void
UsbBrdyEvent(void *eventData)
{
	UsbDev *ud = &gUsb0;
	uint16_t cnt;
	uint16_t i;
	Mutex_Lock(&ud->usbMutex);
	USB0.D0FIFOSEL.WORD = 2 | FIFOSEL_MBW;
	//Con_Printf("BRDY Event %04x\n",ud->brdysts);
	ud->brdysts = 0;
	//Con_Printf("D0FIFOCTR %04x\n",USB0.D0FIFOCTR);
	//while((cnt = USB0.D0FIFOCTR.BIT.DTLN)) {
	cnt = USB0.D0FIFOCTR.BIT.DTLN;
//      Con_Printf("PIPE2CTR %04x\n",USB0.PIPE2CTR.WORD);
	for (i = 0; (i < (cnt & ~1)) && (i < array_size(ud->epOutBuf)); i += 2) {
		*(uint16_t *) (ud->epOutBuf + i) = USB0.D0FIFO.WORD;
	}
	if ((cnt & 1) && (i < array_size(ud->epOutBuf))) {
		ud->epOutBuf[i] = USB0.D0FIFO.BYTE.L;
	}
	//Con_Printf("PID is %d\n",USB0.PIPE2CTR.BIT.PID);
	//Con_Printf("DTLN %d\n",USB0.D0FIFOCTR.BIT.DTLN);
	//Con_Printf("Out Pfeife: Pipe2CTR.SQMON before: %d\n",USB0.PIPE2CTR.BIT.SQMON);
	//USB0.D0FIFOCTR.BIT.BCLR = 1; /** ?? */
	//USB0.PIPE2CTR.BIT.PID = PID_BUF;

	/* Should be at the end because it might send packets */
	if (cnt && ud->epHandlerProc[2]) {
		ud->epHandlerProc[2] (ud->epHandlerData[2], ud->epOutBuf, cnt);
	}
	Mutex_Unlock(&ud->usbMutex);
}

static const uint8_t usb_dvsq_to_devstate[8] = {
/* DVSQ_POWERED	*/ DevState_Powered,
/* DVSQ_DEFAULT */ DevState_Default,
/* DVSQ_ADDRESS */ DevState_Address,
/* DVSQ_CONFIGURED */ DevState_Configured,
/* DVSQ_SUSP_PWR   */ DevState_SuspPwr,
/* DVSQ_SUSP_DEF   */ DevState_SuspDef,
/* DVSQ_SUSP_ADDR  */ DevState_SuspAddr,
/* DVSQ_SUSP_CONF  */ DevState_SuspConf,
};

/**
 ******************************************************************
 * \fn static void UsbDevStateEvent(void *eventData) 
 ******************************************************************
 */
static void
UsbDevStateEvent(void *eventData)
{
	UsbDev *ud = eventData;
	uint8_t dvsq = USB0.INTSTS0.BIT.DVSQ;
	UsbDevState dvst = usb_dvsq_to_devstate[dvsq];
	if (ud->devStateCB) {
		dvsq = USB0.INTSTS0.BIT.DVSQ;
		dvst = usb_dvsq_to_devstate[dvsq & 0x7];
		ud->devStateCB(ud->devStateCBData, dvst);
	}
}

void __attribute__ ((interrupt))
    INT_Excep_USB0_D0FIFO0(void)
{

}

void __attribute__ ((interrupt))
    INT_Excep_USB0_D1FIFO0(void)
{
}

#if BEMPSTS_INTERRUPT
static inline void
Bemp_InterruptHandler(UsbDev * ud)
{
	UsbInPkt *pkt;
	uint8_t pipeNr = 1;	/* Should not be fixed */
#if 0
	/* Doesn't work any more with this ???? */
	if (!(USB0.BEMPSTS.WORD & (UINT16_C(1) << pipeNr))) {
		/* Not for me */
		return;
	}
#endif
	if (ud->inFifoIdle) {
		return;
	}
	USB0.BEMPSTS.WORD = ~(UINT16_C(1) << pipeNr);
	if (ud->inPktWp != ud->inPktRp) {
		pkt = &ud->inPktBuf[INPKT_RP(ud)];
		WritePipe(ud, pipeNr, pkt->data, pkt->len);
		ud->inPktRp++;
	} else {
		/* Disable the bemp interrupt */
		USB0.BEMPENB.WORD &= ~(UINT16_C(1) << pipeNr);
		ud->inFifoIdle = true;
	}
}

/**
 *******************************************************************************
 * \fn void UsbDev_EpInTransmit(UsbDev *ud,uint8_t epNr,uint8_t *data,uint16_t len) 
 * Zero sized pakets must be possible to allow termination with a short paket.
 *******************************************************************************
 */
void
UsbDev_EpInTransmit(UsbDev * ud, uint8_t epNr, uint8_t * data, uint16_t len)
{
	Flags_t flags;
	uint32_t timeout;
	uint8_t pipeNr = epNr;	/* There should be a translation table */
	do {
		UsbInPkt *pkt;
		timeout = 200000;
		while (timeout && (INPKT_CNT(ud) >= INPKT_FIFO_SIZE)) {
			EV_Yield();
			timeout--;
			/* timeout ? */
		}
		if (timeout == 0) {
			Con_Printf("EP IN transmit timeout\n");
			Con_Printf("WP %u\n", ud->inPktWp);
			USB0.D1FIFOCTR.BIT.BCLR = 1;
			ud->inPktWp = ud->inPktRp;
			ud->inFifoIdle = true;
		}
		pkt = &ud->inPktBuf[INPKT_WP(ud)];
		pkt->len = (len > 64) ? 64 : len;
		memcpy(pkt->data, data, pkt->len);
		len -= pkt->len;
		data += pkt->len;
		//Con_Printf("len %d, pkt->len %d\n",len,pkt->len);
		SAVE_FLAGS_SET_IPL(flags, USB_IPL);
		ud->inPktWp++;
		if (ud->inFifoIdle) {
			USB0.BEMPSTS.WORD = ~(UINT16_C(1) << pipeNr);
			USB0.BEMPENB.WORD = (UINT16_C(1) << pipeNr);
			pkt = &ud->inPktBuf[INPKT_RP(ud)];
			ud->inFifoIdle = false;
			//Con_Printf("Start pkt %d, rp %d\n",pkt->len,ud->inPktRp);
			WritePipe(ud, pipeNr, pkt->data, pkt->len);
			ud->inPktRp++;
		}
		RESTORE_FLAGS(flags);
	} while (len);
}

#else
/**
 *******************************************************************
 * This is entered with already locked mutex because this is
 * always a reply. The IN pipe uses the D1FIFO
 * Zero sized pakets must be allowed to allow termination 
 * by a short paket.
 *******************************************************************
 */
void
UsbDev_EpInTransmit(UsbDev * ud, uint8_t epNr, uint8_t * data, uint16_t len)
{
	uint32_t i;
	uint16_t cnt;
	bool done = false;
	uint8_t pipeNr = epNr;	/* There should be a translation table */
	do {
		cnt = (len > 64) ? 64 : len;
		WritePipe(ud, epNr, data, cnt);
		len -= cnt;
		data += cnt;
		for (i = 100000; i > 0; i--) {
			if (USB0.BEMPSTS.WORD & (1 << pipeNr)) {
				break;
			}
			EV_Yield();
		}
		if (i == 0) {
			Con_Printf("IN pipe write failed\n");
			USB0.D1FIFOCTR.BIT.BCLR = 1;
		}
	} while (len);
}
#endif

/**
 *************************************************************
 * This is the USB main interrupt handler. 
 *************************************************************
 */
void __attribute__ ((interrupt))
    INT_Excep_USB0_USBI0(void)
{
	uint16_t intsts0, intsts1;
	UsbDev *ud = &gUsb0;
	intsts0 = USB0.INTSTS0.WORD;
	intsts1 = USB0.INTSTS1.WORD;
#if 0
	if (intsts1 & INTSTS1_BCHG) {
		USB0.INTSTS1.WORD = ~INTSTS1_BCHG;
		ud->statBCHG++;
	}
#endif
	if (intsts1 & INTSTS1_DTCH) {
		USB0.INTSTS1.WORD = ~INTSTS1_DTCH;
		ud->statDTCH++;
	}
	if (intsts1 & INTSTS1_ATTCH) {
		USB0.INTSTS1.WORD = ~INTSTS1_ATTCH;
		ud->statATTCH++;
	}
	if (intsts0 & INTSTS0_DVST) {
		USB0.INTSTS0.WORD = ~INTSTS0_DVST;
		ud->statDVST++;
		EV_Trigger(&gUsb0.evDevState);
	}
	if (intsts0 & INTSTS0_VBINT) {
		USB0.INTSTS0.WORD = (uint16_t) ~ INTSTS0_VBINT;
		ud->statVBINT++;
	}
	if (intsts0 & INTSTS0_CTRT) {
		USB0.INTSTS0.WORD = ~INTSTS0_CTRT;
		EV_Trigger(&gUsb0.evCtrl);
		ud->statCTRT++;
	}
	if (intsts0 & INTSTS0_BRDY) {
		ud->brdysts |= USB0.BRDYSTS.WORD;
		USB0.BRDYSTS.WORD = ~ud->brdysts;
		EV_Trigger(&gUsb0.evBrdy);
		ud->statBRDY++;
	}
	if (intsts0 & INTSTS0_BEMP) {
		ud->statBEMP++;
#if BEMPSTS_INTERRUPT
		Bemp_InterruptHandler(ud);
#endif
	}
}

/**
 ****************************************************************
 * \fn static void UsbDev_ConfBulkInPipe(UsbDev *ud) 
 * A bulk in endpoint is the the Pipe for sending data to the
 * host. (Host point of view).  
 ****************************************************************
 */
static void
UsbDev_ConfBulkInPipe(UsbDev * ud, uint8_t pipeNr, uint8_t epNr)
{
	USB0.PIPESEL.BIT.PIPESEL = pipeNr;
	USB0.PIPEMAXP.WORD = 64;
	USB0.PIPECFG.BIT.EPNUM = epNr;
	/* Configure as transmitting */
	USB0.PIPECFG.BIT.DIR = 1;
	/* Configure as Bulk pipe */
	USB0.PIPECFG.BIT.TYPE = 1;
	//USB0.PIPECFG.BIT.DBLB = 1;
	//USB0.D1FIFOSEL.WORD = 1;
	//USB0.D1FIFOCTR.BIT.BCLR = 1;
	//USB0.PIPE1CTR.BIT.PID = PID_BUF; /* ?? Should this only be done when a packet is available */
	USB0.BRDYSTS.WORD = ~(1 << 1);	/* Changing dir from 0 to 1 triggered an BRDY interrupt */

	USB0.BRDYENB.BIT.PIPE1BRDYE = 1;
}

/**
 ********************************************************************
 * \fn static void UsbDev_ConfBulkOutPipe(UsbDev *ud) 
 * The Out pipe is the data sink for the Device (Host point of view). 
 ********************************************************************
 */
static void
UsbDev_ConfBulkOutPipe(UsbDev * ud, uint8_t pipeNr, uint8_t epNr)
{
	USB0.PIPESEL.BIT.PIPESEL = pipeNr;
	USB0.PIPEMAXP.WORD = 64;
	/* Endpoint is the Bulk out endpoint */
	USB0.PIPECFG.BIT.EPNUM = epNr;
	USB0.PIPECFG.BIT.DIR = 0;	/* 0 == Receiving */
	/* Configure it as Bulk pipe */
	USB0.PIPECFG.BIT.TYPE = 1;

	USB0.PIPECFG.BIT.BFRE = 0;
	USB0.PIPECFG.BIT.DBLB = 1;

	USB0.D0FIFOSEL.WORD = 2;
	USB0.D0FIFOCTR.BIT.BCLR = 1;

	USB0.PIPE2CTR.BIT.SQCLR = 1;
	USB0.PIPE2CTR.BIT.PID = PID_BUF;

	USB0.BRDYENB.BIT.PIPE2BRDYE = 1;
}

/**
 *************************************************************************************************
 * \fn void UsbDev_RegisterEpHandler(UsbDev *ud,uint8_t epNr,UsbEpHandlerProc *proc,void *evData)
 *************************************************************************************************
 */
void
UsbDev_RegisterEpHandler(UsbDev * ud, uint8_t epNr, UsbEpHandlerProc * proc, void *evData)
{
	if (epNr > array_size(ud->epHandlerProc)) {
		Con_Printf("epHandlerProc array to small for ep %d\n", epNr);
		return;
	}
	ud->epHandlerProc[epNr] = proc;
	ud->epHandlerData[epNr] = evData;
}

/**
 **********************************************************************************
 * Register a device state change callback.
 **********************************************************************************
 */

void
UsbDev_RegisterDevStateCB(UsbDev * ud, UsbStateChangeCB * proc, void *eventData)
{
	ud->devStateCB = proc;
	ud->devStateCBData = eventData;
}

/**
 ****************************************************************
 * \fn static char * DVSQStr(uint8_t dvsq); 
 ****************************************************************
 */
static char *
DVSQStr(uint8_t dvsq)
{
	char *str;
	switch (dvsq) {
	    case DVSQ_POWERED:
		    str = "Powered";
		    break;
	    case DVSQ_DEFAULT:
		    str = "Default";
		    break;
	    case DVSQ_ADDRESS:
		    str = "Address";
		    break;
	    case DVSQ_CONFIGURED:
		    str = "Configured";
		    break;
	    case DVSQ_SUSP_PWR:
		    str = "SuspPwr";
		    break;
	    case DVSQ_SUSP_DEF:
		    str = "SuspDef";
		    break;
	    case DVSQ_SUSP_ADDR:
		    str = "SuspAddr";
		    break;
	    case DVSQ_SUSP_CONF:
		    str = "SuspConf";
		    break;
	    default:
		    str = "";
		    break;
	}
	return str;
}

/**
 **********************************************************************
 * Check if the USB cable is connected 
 * to the device. The Powered state is
 * currently excluded because hardware is buggy. 
 **********************************************************************
 */
bool
UsbDev_CheckConnected(void)
{
	switch (USB0.INTSTS0.BIT.DVSQ) {
	    case DVSQ_CONFIGURED:
	    case DVSQ_ADDRESS:
	    case DVSQ_DEFAULT:
		    return true;
	    default:
		    return false;
	}
}

/**
 ********************************************************************************
 * \fn static int8_t cmd_usb(Interp *interp,uint8_t argc,char *argv[]) 
 ********************************************************************************
 */
static int8_t
cmd_usb(Interp * interp, uint8_t argc, char *argv[])
{
	UsbDev *ud = &gUsb0;
	Interp_Printf_P(interp, "BEMP:  %lu\n", ud->statBEMP);
	Interp_Printf_P(interp, "BCHG:  %lu\n", ud->statBCHG);
	Interp_Printf_P(interp, "ATTCH: %lu\n", ud->statATTCH);
	Interp_Printf_P(interp, "DTCH:  %lu\n", ud->statDTCH);
	Interp_Printf_P(interp, "DVST:  %lu DVSQ: %s\n", ud->statDVST,
			DVSQStr(USB0.INTSTS0.BIT.DVSQ));
	Interp_Printf_P(interp, "VBINT: %lu\n", ud->statVBINT);
	Interp_Printf_P(interp, "CTRT:  %lu\n", ud->statCTRT);
	Interp_Printf_P(interp, "BRDY:  %lu\n", ud->statBRDY);
	Interp_Printf_P(interp, "FRM:   %lu\n", USB0.FRMNUM.BIT.FRNM);
	return 0;
}

INTERP_CMD(usbCmd, "usb", cmd_usb, "usb        # Show USB statistics");

/**
 **********************************************************
 * \fn UsbDev * UsbDev_Init(void) 
 **********************************************************
 */

UsbDev *
UsbDev_Init(void)
{
	UsbDev *ud = &gUsb0;
	Mutex_Init(&ud->usbMutex);
	ud->brdysts = 0;
#if BEMPSTS_INTERRUPT
	ud->inFifoIdle = true;
#endif
	EV_Init(&ud->evCtrl, UsbCtrtEvent, ud);
	EV_Init(&ud->evBrdy, UsbBrdyEvent, ud);
	EV_Init(&ud->evDevState, UsbDevStateEvent, ud);
	/* Enable the clock to the device */
	MSTP(USB0) = 0;
	/* Enable the clock in the device */
	USB0.SYSCFG.BIT.SCKE = 1;
	/* Select the function mode */
	USB0.SYSCFG.BIT.DCFM = 0;
	USB0.SYSCFG.BIT.DPRPU = 1;
	/* Enable it */
	USB0.SYSCFG.BIT.USBE = 1;
	IPR(USB0, USBI0) = USB_IPL;
	IPR(USB0, D0FIFO0) = 0;
	IPR(USB0, D1FIFO0) = 0;
	/* Now configure the IO Ports */
	PORT1.PDR.BIT.B4        = 0;
	PORT1.PMR.BIT.B4        = 0;	
	PORT1.PDR.BIT.B6        = 0; // VBUS
	PORT1.PMR.BIT.B6        = 0;
	MPC.P14PFS.BYTE = 0x11;	/* USB0_DPUPE */
	MPC.P16PFS.BYTE = 0x11;	/* VBUS input */
	MPC.PFUSB0.BIT.PUPHZS   = 1;	/* DPUPE pin to HIGH */
	PORT1.PMR.BIT.B4        = 1;	
	PORT1.PMR.BIT.B6        = 1;	
	
	//IEN(USB0,D0FIFO0) = 1;
	//IEN(USB0,D1FIFO0) = 1;
	IEN(USB0, USBI0) = 1;

	USB0.INTENB0.BIT.VBSE = 1;
	USB0.INTENB0.BIT.DVSE = 1;
	USB0.INTENB0.BIT.CTRE = 1;
	USB0.INTENB0.BIT.BRDYE = 1;
	USB0.INTENB1.BIT.BCHGE = 0;
	USB0.INTENB1.BIT.DTCHE = 1;
	USB0.INTENB1.BIT.ATTCHE = 1;
	USB0.BRDYSTS.WORD = 0;
	UsbDev_ConfBulkInPipe(ud, 1, 1);
	UsbDev_ConfBulkOutPipe(ud, 2, 2);
	Interp_RegisterCmd(&usbCmd);
	return ud;
}
