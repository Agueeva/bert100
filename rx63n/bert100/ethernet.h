#ifndef _ETHERNET_H
#define _ETHERNET_H
#include "types.h"
#include "ethdrv.h"
#include "database.h"


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

void * skb_remove_header(Skb *skb,uint16_t len);
void * skb_get_header(Skb *skb);
void * skb_reserve_header(Skb *skb,uint16_t len); 
Skb *skb_alloc(uint16_t hdrlen,uint16_t datalen);
void skb_reset(Skb *skb);
#define DBKEY_ETH_MAC	DBKEY_ETHERNET(0)
#endif
