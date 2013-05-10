#ifndef _IP_H
#define _IP_H
#include "types.h"
#include "ethernet.h" /* For skb, should be moved */

typedef struct IpHdr {
	uint8_t versIhl;
	uint8_t tos;
	uint16_be totalLength;
	uint16_be ident;
	uint16_be flagsFrag;
	uint8_t ttl;		
	uint8_t proto;
	uint16_t chksum;
	uint8_t srcaddr[4];
	uint8_t dstaddr[4];
	// optspad[4];	
} IpHdr;

#define IPPROT_ICMP	0x01
#define IPPROT_TCP	0x06
#define IPPROT_UDP	0x11

typedef struct IcmpHdr {
	uint8_t type;
	uint8_t code;
	uint16_be chksum;
	uint16_be id;
	uint16_be seqNr;
} IcmpHdr;

void IP_MakePacket(Skb *skb,uint8_t *dstip,uint8_t *srcip,uint8_t ipproto,uint16_t payloadlen);
void IP_SendPacket(Skb *skb);
#endif
