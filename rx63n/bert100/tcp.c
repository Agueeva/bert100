/*
 ********************************************************************
 * TCP
 ********************************************************************
 */

#include "tpos.h"
#include "ip.h"
#include "byteorder.h"
#include "tcp.h"
#include "timer.h"
#include "types.h"
#include "console.h"
#include "timer.h"
#include <string.h>
#include <stdlib.h>

#define MAX_WIN_SZ 1460 
#define MAX_SEG_SZ 1460

/* The TCP flags (See header) */
#define TCPFLG_FIN      (1 << 0)
#define TCPFLG_SYN      (1 << 1)
#define TCPFLG_RST      (1 << 2)
#define TCPFLG_PSH      (1 << 3)
#define TCPFLG_ACK      (1 << 4)
#define TCPFLG_URG      (1 << 5)
/**
 ****************************************
 * The TCP states
 ****************************************
 */
#define TCPS_CLOSED		(0)
#define TCPS_LISTEN		(1)
#define TCPS_SYN_RCVD		(2)
#define TCPS_SYN_SENT		(3)
#define TCPS_ESTABLISHED	(4)
#define TCPS_FIN_WAIT_1		(5)
#define TCPS_CLOSE_WAIT		(6)
#define TCPS_FIN_WAIT_2		(7)
#define TCPS_CLOSING		(8)
#define TCPS_LAST_ACK		(9)
#define TCPS_TIME_WAIT		(10)

//define DBG(x) (x)
#define DBG(x) 
static Mutex tcpSema;
/**
 *******************************************
 * TCP header Size is min 20 bytes
 *******************************************
 */
typedef struct TcpHdr {
        uint16_be srcPort;
        uint16_be dstPort;
        uint32_be seqNr;
        uint32_be ackNr;
        uint8_t hdrLen;
        uint8_t flags;
        uint16_be window;
        uint16_be chksum;
        uint16_be urgentPtr;
} TcpHdr;

typedef struct TcpServerSocket {
	uint16_t port;
	Tcp_AcceptProc *acceptProc;
	void *acceptData;
} TcpServerSocket;

/**
 ***************************************************************************
 * The Pseudo header is an object included in the checksum of the TCP packet
 * It is a layer violation done by TCP.
 ***************************************************************************
 */
typedef struct TcpPseudoHdr {
	uint8_t my_ip[4];
	uint8_t dst_ip[4];
	uint16_be proto; 
	uint16_be payloadlen;
} TcpPseudoHdr;

#if 0
typedef struct PacketQueueEntry {
	uint8_t *currDataP;
	uint16_t currDataLen;
	uint8_t  flags;
} PQE;
#endif
INLINE void
Write32(uint32_t value, void *addr)
{
        *(uint32_t *) (addr) = (value);
}

INLINE uint32_t
Read32(const void *addr)
{
        return *(const uint32_t *) (addr);
}

/**
 **********************************************************************
 * RFC 793 Section 2.7 says that information about a connection is
 * stored in a data structure called Transmission Control Block. 
 **********************************************************************
 */
struct Tcb {
	Tcp_DataSink *dataSink;
	Tcp_DataSrc *dataSrc;
	Tcp_CloseProc *closeProc;

	TimeMs_t lastActionTimeMs;
	Timer retransTimer;
	uint8_t retransCounter;
	Timer watchdogTimer;

	void *eventData;
	uint8_t ipAddr[4];
	uint8_t myIp[4];
	uint16_t srcPort;
	uint16_t dstPort;

	/* RFC 793 Section 3.2 */
	uint32_t ISS; /* Initial send sequence number */
	uint32_t SND_NXT; /* Sequence number to send next */
	uint32_t SND_UNA; /* */
	uint32_t SND_WND; /* Send window. Never send a Byte with a SEQ NR > SND_WND */
#if 0
	uint32_t SND_UP; 
	uint32_t SND_WL1;
	uint32_t SND_WL2;

	uint32_t SEG_SEQ; 
	uint32_t SEG_ACK;
	uint32_t SEG_LEN;
	uint32_t SEG_WND;
	uint32_t SEG_UP;
	uint32_t SEG_PRC; /* Segment Precedence value */
#endif

	uint32_t IRS;	  /* IRS initial receive sequence NR */
	uint32_t RCV_NXT; /* Receive next sequence variable */
#if 0
	uint32_t RCV_WND; /* Receive window */
	uint32_t RCV_UP;  /* Receive urgent pointer */
#endif
	uint8_t nxtFlgs;
	bool txDataAvail; /* Set by the upper protocol layer if data can be fetched */
	bool inUse;
	bool busy;	/* for avoiding recursive calls */
	bool do_close;
	uint8_t state;
	uint8_t target_state;
	bool retransCanceled;
	/** Data about the current outgoing packet is stored here because of possible retransmissions */
	bool retransPending; 	/* true when a packet is in the below fields */
	uint8_t currFlags;
	uint8_t *currDataP;
	uint16_t currDataLen;
	uint32_t currDataSeqNr;
};

#define MAX_SERVER_SOCKETS	(4)
#define MAX_TCP_CONS		(10)

static TcpServerSocket serverSocket[MAX_SERVER_SOCKETS];
static Tcb tcpConnection[MAX_TCP_CONS];

/*
 ******************************************************************************+
 * \fn static TcpServerSocket * find_server_socket(uint16_t port) 
 * Find a server socket by port number. Maybe this should be augmented by
 * the IP address because ports might be restricted to local.
 ******************************************************************************+
 */
static TcpServerSocket * 
find_server_socket(uint16_t port) 
{
	uint16_t i;
	for(i = 0; i < array_size(serverSocket); i++) {
		TcpServerSocket *s = &serverSocket[i];
		if(s->port == port) {
			return s;
		}
	}
	return NULL;
}

/**
 **************************************************************
 * Tcb * TCB_Alloc(void); 
 * Allocate a new TCB. This is called when a new connection
 * is detected or opened from local side. 
 **************************************************************
 */
static Tcb *
TCB_Alloc(void) {
	uint16_t i;
	Tcb *tc;
	for(i = 0; i < array_size(tcpConnection); i++) {
		tc = &tcpConnection[i];
		if(tc->inUse == false) {
			tc->inUse = true;
			/* 
			 ***************************************************************
			 * Initial sequence number should be incremented every 4 us 
			 * by one according to section 3.3 of RFC793 
			 ***************************************************************
			 */
			tc->SND_UNA = tc->ISS = tc->SND_NXT = 
				(TimeUs_Get() >> 2); 
			tc->dataSink = NULL;
			tc->retransCanceled = false;
			tc->eventData = NULL;
			tc->closeProc = NULL;
			tc->txDataAvail = false;
			tc->nxtFlgs = 0;
			tc->do_close = false;
			tc->busy = false;
			tc->currDataLen = 0;
			Timer_Start(&tc->watchdogTimer,5000);
			tc->lastActionTimeMs = TimeMs_Get();
			//Con_Printf("Alloced tc %08lx",tc);
			return tc;	
		}
	}
	return 0;
}

/**
 *************************************************************+
 * \fn static void TCB_Free(Tcb *tcb) 
 *************************************************************+
 */
static void
TCB_Free(Tcb *tcb) 
{
	tcb->dataSink = NULL;
	tcb->dataSrc = NULL;
	tcb->closeProc = NULL;
	tcb->eventData = NULL;
	Timer_Cancel(&tcb->watchdogTimer);
	Timer_Cancel(&tcb->retransTimer);
	tcb->retransCanceled = true;
	tcb->state = TCPS_CLOSED;
	tcb->inUse = false;
}

/**
 **************************************************************
 * \fn static void TCB_Close(Tcb *tcb); 
 * Close a Transmission control block. This is called when
 * the connection is totaly finished, The TCB can be freed
 * and the application can be informed that there will
 * be no further data.
 **************************************************************
 */
static void
TCB_Close(Tcb *tcb) {
	if(!tcb->inUse) {
		Con_Printf("Bug, closing unused tcb\n");
	}
	if(tcb->closeProc) {
		tcb->closeProc(tcb->eventData);
	}
	TCB_Free(tcb);
}


#if 0
/**
 **************************************************************************************************
 * \fn void TcpCon_RegisterDataProcs(Tcb *tcon,Tcp_DataSink *sink,Tcp_DataSrc *src,void *eventData) 
 **************************************************************************************************
 */
static void
TcpCon_RegisterDataProcs(Tcb *tcon,Tcp_DataSink *sink,Tcp_DataSrc *src,void *eventData) 
{
	tcon->dataSink = sink;
	tcon->dataSrc = src;
	tcon->eventData = eventData;	
}
#endif

/**
 **********************************************************
 * Register the interface procs
 **********************************************************
 */
void
Tcb_RegisterOps(Tcb *tcb,Tcb_Operations *tcops,void *eventData) 
{
	tcb->dataSink = tcops->sinkProc;
	tcb->dataSrc = tcops->srcProc;
	tcb->closeProc = tcops->closeProc;
	tcb->eventData = eventData;	
}

/**
 ********************************************************************
 * \nf static Tcb * TcpFindConnection(uint8_t ip[4],uint16_t port) 
 * RFC793 Section 2.7 says that a connection is identified by
 * source port and IP.
 ********************************************************************
 */
static Tcb *
TcpFindConnection(uint8_t ip[4],uint16_t port) 
{
	uint16_t i;
	Tcb *tc;
	for(i = 0; i < array_size(tcpConnection); i++) {
		tc = &tcpConnection[i];
		if(tc->inUse == false) {
			continue;
		}
		if((tc->srcPort == port) && (memcmp(ip,tc->ipAddr,4) == 0)) {
			return tc;
		}
	}
	return NULL;
}

/**
 *******************************************************************************
 * \fn static uint16_t chksum_be(uint8_t *data,uint16_t len,uint16_t startval)
 * Calculate the TCP checksum and return it in the byte order of the data 
 * which is big endian here (network byteorder).
 *******************************************************************************
 */
static uint16_t
chksum_be(uint8_t *data,uint16_t len,uint16_t startval)
{
	uint16_be *chkdata = (uint16_t *)data;
	uint16_t i;
	uint32_t sum = startval;
	for(i = 0; i < (len >> 1); i++) {
		sum += chkdata[i];
	}
	/* RFC 793 3.1 */
#if 0
	if(len & 1) {
		uint8_t tmp[2] = {data[len - 1], 0 };
		sum += *(uint16_t *)tmp; 
	}
#else
	/* This only works on little endian machine */
	if(len & 1) {
		sum += data[len - 1]; 
	}
#endif
	while (sum >> 16) {
		sum = (sum & 0xffff) + (sum >> 16);
	}
	sum = ~sum;
	EV_Yield();
	return sum;
}
/**
 ********************************************************************
 * \fn static void Tcp_Watchdog(void *eventData) 
 ********************************************************************
 */
static void
Tcp_Watchdog(void *eventData) 
{
	Tcb *tcb = eventData;
	TimeMs_t now;
	Mutex_Lock(&tcpSema);
	if(tcb->state == TCPS_CLOSED) {
		Mutex_Unlock(&tcpSema);
		return;
	}
	now = TimeMs_Get();
	/* TCP timeout is 1 minutes */
	if((now - tcb->lastActionTimeMs) >= 60000) { 
		DBG(Con_Printf("Now the watchdog is closing\n"));
		TCB_Close(tcb);
	} else {
		Timer_Start(&tcb->watchdogTimer,10000);
	}
	Mutex_Unlock(&tcpSema);
}

static void
Enqueue_Retransmit(Tcb *tcb,uint16_t flags,uint8_t *buf,uint16_t len,uint32_t seqNr) 
{
	tcb->currDataLen = len;
	tcb->currDataP = buf;
	tcb->currFlags = flags;
	tcb->currDataSeqNr = seqNr; 
	tcb->retransPending = true;
	Timer_Start(&tcb->retransTimer,1000);
	tcb->retransCounter = 10;
	DBG(Con_Printf("Enqueued retransmit len %u\n",len));
}
/**
 *****************************************************************************
 * fetch new data is always actualized
 * when the old currentData packet is completely acked. 
 *****************************************************************************
 */
static void
fetch_next_tx_data(Tcb *tcb,uint8_t **dataPP,uint16_t *dataLenP) 
{
	uint32_t fpos = tcb->SND_UNA - tcb->ISS - 1;
	uint16_t wnd = tcb->SND_WND - tcb->SND_UNA;
	uint16_t rsize = wnd > MAX_SEG_SZ ? MAX_SEG_SZ : wnd;
	/**
 	 **********************************************************************
	 * Fetch pointers and seq of current outgoing data 
 	 **********************************************************************
 	 */
	if(tcb->txDataAvail && tcb->dataSrc) {
		//int i;
		tcb->busy = true;
		*dataLenP = tcb->dataSrc(tcb->eventData,fpos,(void **)dataPP,rsize << 1);
		tcb->busy = false;
		if(*dataLenP == 0) {
			//Con_Printf("Warning, got no data\n"); 
			tcb->txDataAvail = false;
		}
#if 0
		Con_Printf("buf %08x",*dataPP);
		for(i = 0; i < *dataLenP; i++) {
			Con_Printf("%c",(*dataPP)[i]);
		}
#endif
	} else {
		*dataLenP = 0;
	}
}

/**
 ******************************************************************************************
 * Eth header is 14 bytes, IP Header is 20 bytes, TcpHdr is 20-24 bytes, Eth packet is max
 * 1518 bytes. So MSS is the rest
 ******************************************************************************************
 */

/**
 ****************************************************************************
 * \fn static void Tcp_Send(Tcb *tc,uint8_t *pktbuf,uint8_t flags) 
 * Send a packet using the connections TCB. 
 ****************************************************************************
 */
static void
Tcp_Send(Tcb *tcb,uint8_t flags,uint8_t *dataP,uint16_t dataLen) 
{
	TcpHdr *tcpHdr;
	TcpPseudoHdr psHdr;
	bool expectingAck = false;
	uint16_t tcphdrlen;
	uint8_t *hdrend;
	uint16_be csum;
	Skb *skbSend = Skb_Alloc(); 

	//skb_reset(skb);
	if(tcb->state == TCPS_SYN_RCVD) {
		tcphdrlen = sizeof(TcpHdr) + 4;
	} else {
		tcphdrlen = sizeof(TcpHdr);
	}
	tcpHdr = (TcpHdr *)skb_reserve_header(skbSend,tcphdrlen); 
	hdrend = ((uint8_t *)tcpHdr)  + sizeof(TcpHdr);
	
	tcpHdr->srcPort   = htons(tcb->dstPort);
	tcpHdr->dstPort  = htons(tcb->srcPort);
	tcpHdr->urgentPtr = 0;
	tcpHdr->window    = htons(MAX_WIN_SZ);
	tcpHdr->flags = flags; 
	tcpHdr->chksum = 0;

	switch(tcb->state) {
		/* Include Maximum Segment size Option */
		case TCPS_SYN_RCVD:
			*hdrend++ = 2;
			*hdrend++ = 4;
			*hdrend++ = (MAX_SEG_SZ >> 8) & 0xff;
			*hdrend++ = MAX_SEG_SZ & 0xff;
			break;

		case TCPS_ESTABLISHED:
			/* Page 16 RFC 793, ACK is always set if established */
			if(!(flags & TCPFLG_ACK)) {
				Con_Printf("TCP bug: Send packet without ack in established state\n");
			}
			break;
		default:
			break;
	}
	if(dataLen) {
		skbSend->dataLen = dataLen;
		if(dataLen <= skbSend->dataAvailLen) {
			memcpy(skbSend->dataStart,dataP,dataLen);
			//skbSend->dataStart = dataP;
		} else {
			Con_Printf("Packet to big: %u, avail %u\n",dataLen,skbSend->dataAvailLen);
		}
		tcpHdr->flags |= TCPFLG_PSH;
	} else {
		skbSend->dataLen = 0;
	}
	tcpHdr->hdrLen = (tcphdrlen >> 2) << 4;
	tcpHdr->ackNr = htonl(tcb->RCV_NXT);
	if(dataLen) {
#if 0
		tcpHdr->seqNr = htonl(tcb->currDataSeqNr);
		tcb->SND_NXT = tcb->currDataSeqNr + dataLen; 
#else
		tcpHdr->seqNr = htonl(tcb->SND_NXT);
		tcb->SND_NXT += dataLen;  /* retransmit should reset to SND_UNA */
#endif
		expectingAck = true;
	} else {
		tcpHdr->seqNr = htonl(tcb->SND_NXT);	
	}
	/* I hope we have not SYN and FIN in the same packet */
	if(flags & (TCPFLG_FIN | TCPFLG_SYN)) {
		/* should set a flag requiresAck, and possible retrans */
		tcb->SND_NXT += 1;
		expectingAck = true;
	}
#if 0
	Con_Printf("TCP out Seq Nr %08lx\n",ntohl(tcpHdr->seqNr));
	Con_Printf("Send TCP packet from %d.%d.%d.%d to %d.%d.%d.%d\n",
		tc->myIp[0],tc->myIp[1],tc->myIp[2],tc->myIp[3],
		tc->ipAddr[0],tc->ipAddr[1],tc->ipAddr[2],tc->ipAddr[3]);
#endif
	Write32(Read32(tcb->myIp),psHdr.my_ip);
	Write32(Read32(tcb->ipAddr),psHdr.dst_ip);

	psHdr.proto = htons(IPPROT_TCP);
	psHdr.payloadlen = htons(dataLen + tcphdrlen);

	csum = chksum_be((uint8_t *)&psHdr,12,0);	
	csum = chksum_be((uint8_t *)tcpHdr,tcphdrlen,~csum);
	csum = chksum_be((uint8_t *)skbSend->dataStart,dataLen,~csum);
	tcpHdr->chksum = csum; 

	IP_MakePacket(skbSend,tcb->ipAddr,tcb->myIp,IPPROT_TCP,tcphdrlen + dataLen);
	IP_SendPacket(skbSend);
#if 0
	if(expectingAck) {
		tcb->retransCounter = 4;
		//Con_Printf("Start the ack timer of tc %08lx\n",tc);
		Timer_Start(&tcb->retransTimer,1000);
	}
#endif
}

static void
Tcp_SendData(Tcb *tcb,uint8_t flags,uint8_t *dataP,uint16_t dataLen) 
{
	if(dataLen > 512) {
		uint16_t half = dataLen >> 1;
		Tcp_Send(tcb,flags,dataP,half);
		Tcp_Send(tcb,flags,dataP + half,dataLen - half);
	} else {
		Tcp_Send(tcb,flags,dataP,dataLen);
	}
}

/**
 ******************************************************************************
 * RFC 793 says that the lower bound for retransmission is 1 second. So we
 * misuse this as fixed value here.
 ******************************************************************************
 */
static void
Tcp_Retrans(void *eventData) 
{
	Tcb *tcb = eventData;
	Mutex_Lock(&tcpSema);
	if(tcb->retransCanceled) {
		Con_Printf("BUG: Retrans called with canceled timer\n"); 
	}
	if(tcb->retransCounter) {
		DBG(Con_Printf("Retransmit %d, state %d\n",tcb->retransCounter,tcb->state));
		tcb->retransCounter--;
		Timer_Start(&tcb->retransTimer,1000);
		if(!tcb->retransPending) {
			Con_Printf("Bug: Retransmit but no pkt enqueued\n");
		} 
		/* Better would be to reset to SND_UNA only */
		tcb->SND_NXT = tcb->currDataSeqNr;
		Tcp_SendData(tcb,tcb->currFlags,tcb->currDataP,tcb->currDataLen);
	} else {
		DBG(Con_Printf("Now the retrans timer is closing\n"));
		TCB_Close(tcb); /* Will trigger a RST */
	}
	Mutex_Unlock(&tcpSema);
}

/**
 *****************************************************************
 * \fn void TcpCon_ControlTx(Tcb *tcon,bool enable); 
 *****************************************************************
 */
void
TcpCon_ControlTx(Tcb *tcb,bool enable) {
	uint16_t dataLen = 0;
	uint8_t *dataP;
	if(tcb->state == TCPS_CLOSED) {
		Con_Printf("Bug, controling unused tcp connection\n");
		return;
	}
	DBG(Con_Printf("Set Data Avail to %d, OUTSTANDING %lu, flags %02x\n",
		enable,tcb->SND_NXT - tcb->SND_UNA,tcb->nxtFlgs));
	if(tcb->txDataAvail == enable) {
		DBG(Con_Printf("Do nothing because already in state %d\n",enable));
		return;
	}
	tcb->txDataAvail = enable;
	/* Check here if something is outstanding */
	if(enable && (tcb->SND_UNA == tcb->SND_NXT)) {
		if(!tcb->busy) {
			Mutex_Lock(&tcpSema);
			fetch_next_tx_data(tcb,&dataP,&dataLen);
			if(dataLen) {
				uint8_t flags = TCPFLG_ACK | TCPFLG_PSH;
				Enqueue_Retransmit(tcb,flags,dataP,dataLen,tcb->SND_NXT);
				Tcp_SendData(tcb,flags,dataP,dataLen);
			}
			Mutex_Unlock(&tcpSema);
		} else {
			DBG(Con_Printf("Busy"));
		}
	} else if(!enable && tcb->do_close) {
		uint8_t flags;
		Mutex_Lock(&tcpSema);
		tcb->state = TCPS_FIN_WAIT_1;
		/* May we send data on FIN ? */
		flags = TCPFLG_FIN | TCPFLG_ACK;
		Enqueue_Retransmit(tcb,flags,NULL,0,tcb->SND_NXT);
		Tcp_Send(tcb,flags,NULL,0);
		Mutex_Unlock(&tcpSema);
	}
}
/*
 ****************************************************************************************
 * The data communincation can be enabled for a connection. If the data communication
 * is enabled the transmitter fetches the data whenever the ack and seq are in sync
 ****************************************************************************************
 */

void
Tcp_Close(Tcb *tcb) {
	/* 
	 ************************************************************************
	 * Strictly the close should only be possible in established state 
	 ************************************************************************
	 */
	DBG(Con_Printf("Called TCP close\n"));
	switch(tcb->state) { 
		case TCPS_CLOSED:
			DBG(Con_Printf("Already closed\n"));
			return;
		/* Check if closing is already in progress */
		case TCPS_FIN_WAIT_1:
		case TCPS_CLOSE_WAIT:
		case TCPS_FIN_WAIT_2:
		case TCPS_CLOSING:
		case TCPS_LAST_ACK:
		case TCPS_TIME_WAIT:
			return;
	}
	if(tcb->state != TCPS_ESTABLISHED) {
		DBG(Con_Printf("Warning, close not in established state\n"));
	}
	/* 
	 ****************************************************************
	 * if still waiting for a reply or unsent outgoing data  
	 * then do the close delayed.
	 ****************************************************************
	 */
	if((tcb->SND_UNA != tcb->SND_NXT) /*|| Timer_Busy(&tcb->retransTimer)*/ || tcb->busy) {
		DBG(Con_Printf("Delaying the close\n"));
		tcb->do_close = true;
	} else {
		tcb->state = TCPS_FIN_WAIT_1;
		Tcp_Send(tcb,TCPFLG_FIN | TCPFLG_ACK,NULL,0);
	}
}

/**
 *****************************************************************
 * RST is the only thing which is sent without having a tcb.
 * See Section 3.4 Reset processing in RFC 793
 * This one needs to use the RX skb for the reply because it
 * belongs to no TCB. 
 *****************************************************************
 */
void
Tcp_Rst(IpHdr *ipHdr,Skb *skbReq) {
	Skb *skbReply = Skb_Alloc();
	TcpPseudoHdr psHdr;
	uint8_t dstaddr[4];
	uint8_t srcaddr[4];
	//bool requiresAck = false;
	uint16_t tcphdrlen = sizeof(TcpHdr);
	uint16_t payloadlen = 0;
	uint16_be csum; 
	uint16_t tmpPort;
	uint32_t tmpNr;
	TcpHdr *tcpHdrReq = (TcpHdr *)skb_reserve_header(skbReq,sizeof(TcpHdr));
	TcpHdr *tcpHdrReply = (TcpHdr *)skb_reserve_header(skbReply,sizeof(TcpHdr));
	//uint8_t *hdrend = ((uint8_t *)tcpHdr)  + tcphdrlen;
	//skbReply->dataLen = 0;

	tmpNr = tcpHdrReq->ackNr;
	tcpHdrReply->ackNr = htonl(ntohl(tcpHdrReq->seqNr) + 1);	
	tcpHdrReply->seqNr = tmpNr; 

	tmpPort = tcpHdrReq->srcPort;
	tcpHdrReply->srcPort  = tcpHdrReq->dstPort;
	tcpHdrReply->dstPort  = tmpPort;

	tcpHdrReply->urgentPtr = 0;
	tcpHdrReply->window    = htons(MAX_WIN_SZ);
	tcpHdrReply->flags = TCPFLG_RST | TCPFLG_ACK; 
	tcpHdrReply->chksum = 0;

	tcpHdrReply->hdrLen = (tcphdrlen >> 2) << 4;

	Write32(Read32(ipHdr->dstaddr),psHdr.my_ip);
	Write32(Read32(ipHdr->srcaddr),psHdr.dst_ip);
	psHdr.proto = htons(IPPROT_TCP);
	psHdr.payloadlen = htons(payloadlen + tcphdrlen);

	csum = chksum_be((uint8_t *)&psHdr,12,0);	
	csum = chksum_be((uint8_t *)tcpHdrReply,tcphdrlen + payloadlen,~csum);
	tcpHdrReply->chksum = csum; 
	Write32(Read32(ipHdr->srcaddr),dstaddr);
	Write32(Read32(ipHdr->dstaddr),srcaddr);

	IP_MakePacket(skbReply,dstaddr,srcaddr,IPPROT_TCP,tcphdrlen + payloadlen);
	IP_SendPacket(skbReply);
}

/**
 ***********************************************************
 * SYN and FIN increment the Sequence number
 * See errata to rfc793
 ***********************************************************
 */
void
Tcp_ProcessPacket(IpHdr *ipHdr,Skb *skb) 
{
	Tcb *tc = NULL;
	Tcb *tcb;
	TcpHdr *tcpHdr;
	TimeMs_t now;
	uint32_t seqNr,ackNr;
	bool needToSendAck = false;
	bool needToSendFin = false;
	uint8_t flags;
	uint8_t *dataP;
	uint16_t dataLen = 0;

	now = TimeMs_Get();
	tcpHdr = (TcpHdr *)skb_remove_header(skb,sizeof(TcpHdr)); 
	seqNr = ntohl(tcpHdr->seqNr);
	ackNr = ntohl(tcpHdr->ackNr);
	if(tcpHdr->flags == TCPFLG_SYN) {
		bool result = false;
		TcpServerSocket *ssock;
		DBG(Con_Printf("Got SYN, a new con on port %u, src %u\n",
			ntohs(tcpHdr->dstPort),ntohs(tcpHdr->srcPort))); 
		ssock = find_server_socket(ntohs(tcpHdr->dstPort));	
		if(ssock) {
			tcb = tc = TCB_Alloc();
			if(!tc) {
				Con_Printf("Out of TCB's\n");	
				Tcp_Rst(ipHdr,skb);
				return;
			}
			tc->state = TCPS_SYN_RCVD;
			Mutex_Lock(&tcpSema);
			if(ssock->acceptProc) {
				result = ssock->acceptProc(ssock->acceptData,tc,ipHdr->srcaddr,tcpHdr->srcPort); 
			}
			if(result != true) {
				DBG(Con_Printf("nobody listening on TCP port %d\n",ntohs(tcpHdr->dstPort)));
				TCB_Close(tc);
				Tcp_Rst(ipHdr,skb);
				Mutex_Unlock(&tcpSema);
				return;
			}
		} else {
			DBG(Con_Printf("No ssock, sending RST\n"));
			Tcp_Rst(ipHdr,skb);
			return;
		}
		tc->srcPort = ntohs(tcpHdr->srcPort);
		tc->dstPort = ntohs(tcpHdr->dstPort);
		/* SYN and FIN increment AckNr by one */
		tc->IRS = seqNr;
	 	tc->RCV_NXT = seqNr + 1; 
		tc->SND_WND = tc->SND_UNA + ntohs(tcpHdr->window);
		Write32(Read32(ipHdr->srcaddr),tc->ipAddr);
		Write32(Read32(ipHdr->dstaddr),tc->myIp);
		/* 
 		 *********************************************************************************
		 * Steps 2 and 3 according to section 3.3 of RFC793 , Ack the received seq. Nr
		 * and SYN the OWN initial sequence number
 		 *********************************************************************************
		 */
		Enqueue_Retransmit(tcb,TCPFLG_SYN | TCPFLG_ACK,NULL,0,tcb->SND_NXT);
		Tcp_Send(tcb,TCPFLG_SYN | TCPFLG_ACK,NULL,0);
		Mutex_Unlock(&tcpSema);
		return;
	}
	/* 
 	 ******************************************************************************
	 * If not SYN try to find an existing TCB 
 	 ******************************************************************************
	 */
	Mutex_Lock(&tcpSema);
	tcb = tc = TcpFindConnection(ipHdr->srcaddr,ntohs(tcpHdr->srcPort));
	if(!tc) {
		DBG(
		Con_Printf("No TCB for port %u, ip %u.%u.%u.%u\n",htons(tcpHdr->srcPort),
			ipHdr->srcaddr[0],ipHdr->srcaddr[1],ipHdr->srcaddr[2],
			ipHdr->srcaddr[3]));
		/* Send RST */
		Tcp_Rst(ipHdr,skb);
		Mutex_Unlock(&tcpSema);
		return;
	}
	tc->lastActionTimeMs = now;
	if(tcpHdr->flags & TCPFLG_RST) {
		TCB_Close(tc);
		Mutex_Unlock(&tcpSema);
		return;
	}
	if(tc->state == TCPS_SYN_RCVD) {
		if(tcpHdr->flags & TCPFLG_ACK) {
			if(/*(seqNr == tc->RCV_NXT) && */ /* Should check seq in win */
				(ackNr == tc->SND_NXT)) {

				Timer_Cancel(&tc->retransTimer);
				DBG(Con_Printf("Synack was acked, Connection established\n"));	
				tc->state = TCPS_ESTABLISHED;
				/* 
				 ***************************************************************
				 * After we got the ack for our SYN ACK we can send data 
				 ***************************************************************
				 */
				if(tc->txDataAvail) {
					DBG(Con_Printf("Data avail\n"));	
					fetch_next_tx_data(tcb,&dataP,&dataLen);
				}
			} else {
				DBG(Con_Printf("Connection not Established, wrong ack number, seq Nr %lu(%lu) ackNr %lu(%lu)\n",seqNr,tc->RCV_NXT,ackNr,tc->SND_NXT));
				/* Should send a reset here  */
				TCB_Close(tc);
				Tcp_Rst(ipHdr,skb);
			}
		} else {
			/* Leave a chance for retransmission of SYN_ACK */
			Mutex_Unlock(&tcpSema);
			return;
		}
		/* Fall through, No return ! may contain data ! */
	} 

	/**
	 ************************************************************************
	 * New version	
	 ************************************************************************
 	 */
	/* First check if something new came in */
	if(seqNr == tc->RCV_NXT) {
		/* Eat up the Data */	
		uint16_t iplen = ntohs(ipHdr->totalLength);
		uint16_t nr_bytes = iplen - sizeof(IpHdr) - ((tcpHdr->hdrLen >> 4) << 2);
		if(nr_bytes) {
			uint32_t fpos = tc->RCV_NXT - tc->IRS - 1;
			tcb->RCV_NXT += nr_bytes; 
			needToSendAck = true; 
			DBG(Con_Printf("RCV next set to %llu and needToSendAck\n",tcb->RCV_NXT));
			if(tc->dataSink) {
				/* Shit, might recurse and call TX_Avail which locks semaphore */	
				tcb->busy = true;
				tcb->dataSink(tc->eventData,fpos,skb->dataStart,nr_bytes);	
				tcb->busy = false;
			}
			/* We should see from ack nr that we have to transmit a ack reply */
		} 
	} else if((int32_t)(seqNr - tc->RCV_NXT) < 0) { 
		DBG(Con_Printf("Seems like a retransmited packet\n"));
		needToSendAck = true;
	} else if(seqNr > tc->RCV_NXT) {
		/* Future packet, ignore it, resend ack for old packet */
		needToSendAck = true;
	}
	/* If we got everything acked we can fetch the next data packet */
	if(ackNr == tcb->SND_NXT) {
		tcb->SND_UNA = ackNr;
		tcb->SND_WND = ackNr + ntohs(tcpHdr->window);
		tcb->retransPending = false;
		Timer_Cancel(&tcb->retransTimer);
		fetch_next_tx_data(tcb,&dataP,&dataLen);
		if(tcb->do_close && (!dataLen)) {
			needToSendFin = true;
			needToSendAck = true;
			tcb->do_close = false;
			tcb->state = TCPS_FIN_WAIT_1;
		} else if((tcb->state == TCPS_ESTABLISHED) && 
			(tcpHdr->flags & TCPFLG_FIN)) {
			/* Syn and fin increment the seqNr */
			tcb->RCV_NXT += 1;
			needToSendAck = true;
			needToSendFin = true;	
			tc->state = TCPS_LAST_ACK;
		} else if(tcb->state == TCPS_LAST_ACK && (tcpHdr->flags & TCPFLG_ACK)) {
			/* This one has no reply */
			TCB_Close(tc);
			Mutex_Unlock(&tcpSema);
			return;
		} else if((tc->state == TCPS_FIN_WAIT_1)  && (tcpHdr->flags & TCPFLG_ACK)) {
			/* Do I need to check for seq NR here ? */ 
			if(tcpHdr->flags & TCPFLG_FIN) {
				tcb->RCV_NXT += 1; 
				DBG(Con_Printf("Closed by me and acked with FIN,ACK\n"));
				Tcp_Send(tcb,TCPFLG_ACK,NULL,0);
				tcb->state = TCPS_TIME_WAIT;
				TCB_Close(tcb);
				Mutex_Unlock(&tcpSema);
				return;
			} else {
				DBG(Con_Printf("FINWAIT2, send nix\n"));
				tc->state = TCPS_FIN_WAIT_2;
			}
		} else if((tc->state == TCPS_FIN_WAIT_2) && (tcpHdr->flags & TCPFLG_FIN))  {
			DBG(Con_Printf("Closed by me and FINed in FIN2\n"));
			tc->RCV_NXT += 1; 
			Tcp_Send(tc,TCPFLG_ACK,NULL,0);
			tc->state = TCPS_TIME_WAIT;
			TCB_Close(tc);
			Mutex_Unlock(&tcpSema);
			return;
		}
	}
	flags = TCPFLG_ACK;
	if(needToSendFin) {
		flags |= TCPFLG_FIN;
		DBG(Con_Printf("Case need fin\n"));
		Enqueue_Retransmit(tcb,flags,NULL,0,tcb->SND_NXT);
		Tcp_Send(tcb,flags,NULL,0);
	} else if(dataLen) {
		DBG(Con_Printf("Calling TCP send with flags 0x%02x\n",flags));
		Enqueue_Retransmit(tcb,flags,dataP,dataLen,tcb->SND_NXT);
		Tcp_SendData(tcb,flags,dataP,dataLen);
	} else if(needToSendAck) {
		Tcp_Send(tcb,flags,NULL,0);
	} else {
		DBG(Con_Printf("Send nothing because not necessary\n"));
	}
	Mutex_Unlock(&tcpSema);
}

/**
 *************************************************************************
 * Create a TCP server socket. 
 *************************************************************************
 */
void
Tcp_ServerSocket(uint16_t port,Tcp_AcceptProc *proc,void *eventData) 
{
	uint16_t i;
	if(find_server_socket(port)) {
		Con_Printf("Port %d is already in use\n",port);
		return;
	}
	for(i = 0; i < array_size(serverSocket); i++) {
		TcpServerSocket *ts = &serverSocket[i];
		if(ts->acceptProc == NULL) {
			ts->port = port;
			ts->acceptProc = proc;
			ts->acceptData = eventData;
			break;
		}
	}
}

/**
 ************************************************************
 * Initialize the TCP module
 ************************************************************
 */
void 
Tcp_Init(void) 
{
	uint16_t i;
	for(i = 0; i < array_size(tcpConnection); i++) {
		Tcb *tcb =  &tcpConnection[i];
		Timer_Init(&tcb->retransTimer,Tcp_Retrans,tcb);
		Timer_Init(&tcb->watchdogTimer,Tcp_Watchdog,tcb);
	}
	Mutex_Init(&tcpSema);
}
