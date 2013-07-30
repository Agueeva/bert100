#ifndef _ETHERNET_H
#define _ETHERNET_H
#include "types.h"
#include "ethdrv.h"

#if 0
/**
 *************************************************************************
 * Pakets are split into two halfs. A header and a Data part. 
 * The header is written from the end. It end-first may not get
 * bigger than maxlen; The header grows downward up to maxlen
 * on composition of packet. Header grows upward on decomposition
 * For decomposition all data needs to be in one databuf and
 * hdr_end moves. hdr_start has a reserve of 32 Bytes for 
 * the case that the sent header is bigger than the received header.
 *************************************************************************
 */
typedef struct Skb {
	uint8_t *hdrBuf;
	uint8_t *hdrStart; 
	uint8_t *hdrEnd;
	uint16_t hdrAvailLen;
	uint16_t hdrBufSize;

	uint8_t *dataBuf;
	uint8_t *dataStart;
	uint16_t dataLen;
	uint16_t dataAvailLen;
	uint16_t dataBufSize;
} Skb;

typedef void Eth_TxProc(void *ctrl,Skb *skb);
typedef void Eth_RegPktSinkProc(void *driverData,void (*p)(void *evData,Skb *skb),void *evData);
typedef void Eth_SetMacProc(void *evData,uint8_t *mac);

/**
 ***********************************************************
 * The ethernet driver structure
 ***********************************************************
 */
typedef struct EthernetDriver {
	Eth_TxProc *txProc;
	Eth_RegPktSinkProc *regRxProc;
	Eth_SetMacProc *setMacProc;
	void *driverData;
} EthernetDriver;
#endif

/**
 ***********************************************************
 * Ethernet paket types.
 * Stolen from Wikipedia
 ***********************************************************
 */
typedef enum EtherType {
	ET_IP =  0x0800,     /* IP Internet Protocol, Version 4 (IPv4)  */
	ET_ARP = 0x0806,     /* Address Resolution Protocol (ARP) */
	ET_WOL = 0x0842,     /* Wake on LAN (WoL) */
	ET_RARP = 0x8035,    /* Reverse Address Resolution Protocol (RARP) */
	ET_ETALK = 0x809B,   /* AppleTalk (EtherTalk)  */
	ET_AARP = 0x80F3,    /* Appletalk Address Resolution Protocol (AARP) */
	ET_VLAN = 0x8100,    /* VLAN Tag (VLAN) */
	ET_IPX = 0x8137,     /* Novell IPX (alt) */
	ET_NOVELL = 0x8138,  /* Novell  */
	ET_IPV6 = 0x86DD,    /* IP Internet Protocol, Version 6 (IPv6) */
	ET_PPPOED = 0x8863,  /* PPPoE Discovery */
	ET_PPPOES = 0x8864,  /* PPPoE Session */
	ET_JUMBO = 0x8870,   /* Jumbo Frames */
	ET_PROFINET = 0x8892,/* Echtzeit-Ethernet PROFINET */
	ET_ATA = 0x88A2,     /* ATA over Ethernet Coraid AoE [2] */
	ET_ETCAT = 0x88A4,   /* Echtzeit-Ethernet EtherCAT */
	ET_PBRIDG = 0x88A8,  /* Provider Bridging */
	ET_PWRLNK = 0x88AB,  /* Echtzeit-Ethernet Ethernet POWERLINK */
	ET_SERCOS = 0x88CD,  /* Echtzeit-Ethernet SERCOS III */
	ET_FCOE	= 0x8906,    /* Fibre Channel over Ethernet */
	ET_FCOEIP = 0x8914,  /* FCoE Initialization Protocol (FIP) */
} EtherType;


/* ARP request is 28 bytes long */
typedef struct ArpRq {
	uint16_be htype; /* 1 == ethernet */		
	uint16_be ptype;
	uint8_t hwlen;
	uint8_t plen;
	uint16_be oper;
	uint8_t sha[6]; /* Sender Hardware address */
	uint8_t spa[4]; /* Sender Protocol address (IP Address) */
	uint8_t tha[6]; /* Target Hardware address, ignored in rq */
	uint8_t tpa[4];	/* Target protocol address (IP Address) */
} ArpRq;


void Ethernet_Init(EthDriver *);

uint8_t * skb_remove_header(Skb *skb,uint16_t len);
uint8_t * skb_get_header(Skb *skb);
uint8_t * skb_reserve_header(Skb *skb,uint16_t len); 
Skb *skb_alloc(uint16_t hdrlen,uint16_t datalen);
void skb_reset(Skb *skb);
#endif
