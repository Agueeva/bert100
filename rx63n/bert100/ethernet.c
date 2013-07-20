/**
 *******************************************************************************
 * Ethernet layer of the Network Protocollstack
 * Does ARP handling. 
 *******************************************************************************
 */
#include <stdlib.h>
#include <string.h>
#include "console.h"
#include "types.h"
#include "byteorder.h"
#include "ethernet.h"
#include "ip.h"
#include "timer.h"
#include "interpreter.h"
#include "hex.h"
#include "tcp.h"
#include "iram.h"

typedef struct ArpCE {
	uint8_t arp_mac[6];
	union {
		uint8_t arp_ip[4];
		uint32_t arp_ip32;
	};
	TimeMs_t timestamp;
} ArpCE;

typedef struct EthIf {
	EthernetDriver *drv;
        void *hwif;
        uint8_t if_mac[6];
	union {
		uint8_t if_ip[4]; /* for arp */
		uint32_t if_ip32; /* for arp */
	};
	union {
		uint8_t if_netmask[4];
		uint32_t if_netmask32;
	};
	union {
		uint8_t defaultGW[4];
		uint32_t defaultGW32;
	};
	ArpCE arpCache[4];
} EthIf;

typedef struct EthHdr {
	uint8_t dstmac[6];
	uint8_t srcmac[6];
	uint16_be proto;
} EthHdr;

EthIf g_EthIf;

/* 
 ******************************************************************
 * May only be used for single buffer rx messages for
 * decomposition.
 ******************************************************************
 */
uint8_t * 
skb_remove_header(Skb *skb,uint16_t len) 
{
	uint8_t *hdr = skb->hdrEnd;	
	skb->hdrEnd += len;
	skb->hdrStart += len;
	skb->dataStart += len;
	if(len <= skb->dataLen) {
		skb->dataLen -= len;
		skb->hdrAvailLen += len;
		skb->dataAvailLen -= len;
	} else {
		skb->dataLen = 0;
	}
//	Con_Printf("remove hdr %u, datalen %u\n",
//		(uint16_t)(skb->hdr_end - skb->hdr_start),skb->dataLen);
	return hdr;
}

uint8_t * 
skb_get_header(Skb *skb) 
{
	return skb->hdrStart;
}

uint8_t *
skb_reserve_header(Skb *skb,uint16_t len) {
	if(len <= skb->hdrAvailLen) {
		skb->hdrStart -= len;
		skb->hdrAvailLen -= len;
	} else {
		Con_Printf("Bug: skb_reserve_header buffer to small\n");
	}
//	Con_Printf("reserve: hdr %u, datalen %u\n",
//		(uint16_t)(skb->hdr_end - skb->hdr_start),skb->dataLen);
	return skb->hdrStart;
}

/**
 ********************************************************************
 * \fn Skb * skb_alloc(uint16_t hdrlen,uint16_t datalen) 
 ********************************************************************
 */
Skb *
skb_alloc(uint16_t hdrlen,uint16_t datalen) 
{
#warning alloc missing
	Skb *skb = IRam_Calloc(sizeof(Skb));
	if(hdrlen) {
		skb->hdrBuf = skb->hdrStart = skb->hdrEnd;// = sr_calloc(hdrlen);
		skb->hdrAvailLen = hdrlen;		
		skb->hdrBufSize = hdrlen;
	}	
	if(datalen) {
		skb->dataStart = skb->dataBuf; //  = sr_calloc(datalen); 
		skb->dataAvailLen = datalen;
		skb->dataBufSize = datalen;
	}
	return skb;
}

void
skb_reset(Skb *skb) {
	if(skb->dataBuf) {
		skb->dataStart = skb->dataBuf;
		skb->dataAvailLen = skb->dataBufSize;
	}
	if(skb->hdrBuf) {
		skb->hdrStart = skb->hdrEnd = skb->hdrBuf + skb->hdrBufSize;
		skb->hdrAvailLen = skb->hdrBufSize;
	}
}
/**
 **********************************************************
 * Transmit an ethernet packet.
 **********************************************************
 */
inline void
Eth_Transmit(EthIf *eth,Skb *skb) 
{
	eth->drv->txProc(eth->drv->driverData,skb);
}

/**
 ***********************************************************
 * Enter a new mac/ip to the arp cache
 ***********************************************************
 */

static void
ArpCE_Enter(EthIf *eth,uint8_t *ip,uint8_t *mac) 
{
	ArpCE *ace;
	uint16_t entry_nr = 0;
	uint16_t i;
	TimeMs_t now = TimeMs_Get();
	TimeMs_t age = 0;
	for(i = 0; i < array_size(eth->arpCache); i++) {
		ace = &eth->arpCache[i];
		if(*(uint32_t *)ip == ace->arp_ip32) {
			entry_nr = i;
			break;
		}
		if((now - ace->timestamp) > age) {
			entry_nr = i;	
		}
	}
	ace = &eth->arpCache[entry_nr];
	ace->timestamp = now;
	for(i = 0; i < 6; i++) {
		ace->arp_mac[i] = mac[i];
	}
	for(i = 0; i < 4; i++) {
		ace->arp_ip[i] = ip[i];
	}
}

inline bool  
netmask_match(EthIf *eth,const uint8_t *ip) {
	if((*(uint32_t *)ip & eth->if_netmask32) ==
	  (eth->if_ip32 & eth->if_netmask32)) 
	{
		return true;
	} else {
		return false;
	}
}

/**
 ****************************************************
 * Find an arp entry by its ip address
 ****************************************************
 */
ArpCE *
ArpCE_Find(EthIf *eth,const uint8_t *ip) 
{
	uint16_t i;
	ArpCE *ace = NULL;
	//TimeMs_t now = TimeMs_Get();
	for(i = 0; i < array_size(eth->arpCache); i++) {
		ace = &eth->arpCache[i];
		if(*(uint32_t*)ip == ace->arp_ip32) {
			return ace;
		}
	}
	/* check for default gw */
	if(!netmask_match(eth,ip)) {
		for(i = 0; i < array_size(eth->arpCache); i++) {
			ace = &eth->arpCache[i];
			if(eth->defaultGW32 == ace->arp_ip32) {
				return ace;
			}
		}
	}
	return NULL;
	/* Check if ARP entry is too old */
	/* Primitive approach: living forever */
#if 0
	if((now - ace->timestamp) > 60000) {
		return NULL;
	}
#endif
}

/**
 *************************************************************************
 * \fn static void Eth_HandleArp(EthIf *eth,uint8_t *pkt,uint16_t pktlen) 
 * Does crossing over in reply, so it does not need the arp cache
 *************************************************************************
 */
static void
Eth_HandleArp(EthIf *eth,EthHdr *ethHdr,Skb *skb) 
{
	uint16_t i;
	ArpRq *arp = (ArpRq *)skb_remove_header(skb,sizeof(ArpRq));
	//Con_Printf("Arp tpa %u.%u.%u.%u\n",arp->tpa[0],arp->tpa[1],arp->tpa[2],arp->tpa[3]);
	//Con_Printf("my ip %u.%u.%u.%u\n",eth->if_ip[0],eth->if_ip[1],eth->if_ip[2],eth->if_ip[3]);
	if((arp->ptype == htons(0x800)) &&
	   (arp->oper == htons(1)) && 
	   (memcmp(eth->if_ip,arp->tpa,4) == 0)) {

		ArpCE_Enter(eth,arp->spa,arp->sha);

		arp->oper = htons(2); /* Arp reply */
		for (i = 0; i < 6; i++) {
			//Con_Printf("sha %d: %02x\n",i,arp->sha[i]);
			arp->tha[i] = arp->sha[i];
			arp->sha[i] = eth->if_mac[i];
		}
		for(i = 0; i < 4; i++) {
			arp->tpa[i] = arp->spa[i];
			arp->spa[i] = eth->if_ip[i];
		}
		// cross the ethernet header
		for (i = 0; i < 6; i++) {
			ethHdr->dstmac[i] = ethHdr->srcmac[i];
			ethHdr->srcmac[i] = eth->if_mac[i];
		}
		#if 0
		Con_Printf("Arp Reply:\n");
		for(i = 0; i < 28 + 14; i++){
			Con_Printf("%02x ",pkt[i]);
			if((i & 15) == 15) {
				Con_Printf("\n");
			}
		}
		Con_Printf("\n");
		#endif
		skb_reserve_header(skb,sizeof(ArpRq) + sizeof(EthHdr));
		Eth_Transmit(eth,skb);
	} else if((arp->ptype == htons(0x800)) &&
	   (arp->oper == htons(2))) {
		/* Believe everything, no arp spoofing test here */
		// Enter to arp cache
		ArpCE_Enter(eth,arp->spa,arp->sha);
	} 
}

/**
 ****************************************************************************
 * \fn static void make_eth_header(EthIf *eth,uint8_t *pkt,uint8_t *ipaddr) 
 ****************************************************************************
 */

static void 
add_eth_header(EthIf *eth,Skb *skb,const uint8_t *dstip,uint16_t proto) 
{
	ArpCE * ace = ArpCE_Find(eth,dstip);
	EthHdr *ethHdr = (EthHdr *)skb_reserve_header(skb,sizeof(EthHdr));
	uint8_t i;
	if(!ace) {
		Con_Printf("ACE not found\n");
		Con_Printf("for IP %u.%u.%u.%u\n",
			dstip[0],dstip[1],dstip[2],dstip[3]);	
		// send an arp request;
		return; /* Not yet implemented */
	}
	for(i = 0; i < 6; i++) {
		ethHdr->dstmac[i] = ace->arp_mac[i];	
		ethHdr->srcmac[i] = eth->if_mac[i];
	}
	ethHdr->proto = htons(proto);
	return;
}


/**
 ****************************************************
 * Returns the ckecksum in host byteorder
 * RFC791 is very imprecise in description of the
 * algorithm. But the intention is that the 
 * total sum of the packet  and the checksum is zero.
 * Look at RFC1071.
 ****************************************************
 */
static uint16_t
chksum_be(uint8_t *data,uint16_t len) 
{
	uint16_be *chkdata = (uint16_t *)data; 
	uint16_t i;
	uint32_t sum = 0;
	for(i = 0; i < (len >> 1); i++) {
		sum += chkdata[i];	
	}
#if 0
        if(len & 1) {
                uint8_t tmp[2] = {data[len - 1], 0 };
                sum += *(uint16_t *)tmp;
        }
#else
	/* Warning, this works only correctly on Little endian machines */
        if(len & 1) {
                sum += data[len - 1];
        }
#endif
  	while (sum >> 16) {
    		sum = (sum & 0xffff) + (sum >> 16);
	}
	sum = ~sum;
	return sum;	
}

/**
 ********************************************************************************************
 * Create an IP header.
 ********************************************************************************************
 */
static void 
add_ip_header(Skb *skb,const uint8_t *dstip,const uint8_t *srcip,uint8_t proto,uint16_t payloadlen) 
{
	static uint16_t id_counter = 0;
	IpHdr *ipHdr = (IpHdr *)skb_reserve_header(skb,sizeof(IpHdr));	

	ipHdr->versIhl = 0x45; 
  	ipHdr->tos = 0;
  	ipHdr->totalLength = htons(payloadlen + sizeof(IpHdr));
	//Con_Printf("IP Length is %d\n",ntohs(ipHdr->totalLength));
	ipHdr->ident = htons(id_counter);
	id_counter++;
	ipHdr->flagsFrag = 0x40;
	ipHdr->ttl = 64;
	ipHdr->proto = proto;
	ipHdr->dstaddr32 = *(uint32_t *)dstip;	
	ipHdr->srcaddr32 = *(uint32_t *)srcip;
	
	/* For purposes of calcluating the checksum the checksum field is zero */ 
	ipHdr->chksum = 0;
	ipHdr->chksum = chksum_be((uint8_t *)ipHdr,sizeof(IpHdr));
	return;
}

/**
 ******************************************************************************************************
 * \fn uint8_t make_icmpHdr(IcmpHdr *,uint8_t type,uint8_t code,uint16_t id,uint16_t seqNr,uint16_t icmpsz) 
 ******************************************************************************************************
 */
static uint8_t 
make_icmpHdr(IcmpHdr *icmpHdr,uint8_t type,uint8_t code,uint16_t id,uint16_t seqNr,uint16_t icmpsz) 
{
	icmpHdr->type = type;
	icmpHdr->code = code;
	icmpHdr->id = htons(id);
	icmpHdr->seqNr = htons(seqNr);	
	icmpHdr->chksum = 0;
	icmpHdr->chksum = chksum_be((uint8_t *)icmpHdr,icmpsz);
	return sizeof(IcmpHdr);
}

/**
 *********************************************************************************************
 * \fn void IP_MakePacket(uint8_t *pktbuf,uint8_t *dstip,uint8_t ipproto,uint16_t payloadlen) 
 *********************************************************************************************
 */
void
IP_MakePacket(Skb *skb,const uint8_t *dstip,const uint8_t *srcip,uint8_t ipproto,uint16_t payloadlen) 
{
	EthIf *eth = &g_EthIf;	
//	uint8_t ethhdrlen;
//	uint8_t iphdrlen;
	
	add_ip_header(skb,dstip,srcip,ipproto,payloadlen);
	add_eth_header(eth,skb,dstip,0x800);
}

void
IP_SendPacket(Skb *skb) 
{
	Eth_Transmit(&g_EthIf,skb);
}

/**
 **********************************************************************************************************
 * \fn static void ping_reply(uint8_t *pkt,uint8_t *dstip,uint16_t id,uint16_t seqNr,uint16_t totalLength) 
 * Ping replies only on request assembled into the rx buffer.
 **********************************************************************************************************
 */
static void
ping_reply(Skb *skb,const uint8_t *dstip,const uint8_t *srcip,uint16_t id,uint16_t seqNr,uint16_t totalLength) 
{
	IcmpHdr *icmpHdr;
	uint16_t ip_payloadlen = totalLength - sizeof(IpHdr);
	//uint16_t hdrlen = sizeof(IcmpHdr);
	//uint8_t dstip_copy[4]; /* Need a copy because original is overwritten at arp resolution */

	icmpHdr = (IcmpHdr *)skb_reserve_header(skb,sizeof(IcmpHdr));
	make_icmpHdr(icmpHdr,0,0,id,seqNr,ip_payloadlen);
	IP_MakePacket(skb,dstip,srcip,IPPROT_ICMP,ip_payloadlen); 
	IP_SendPacket(skb); 
}

void
Eth_HandleICMP(EthIf *eth,IpHdr *ipHdr,Skb *skb) 
{
	/* ping request */
	uint8_t dstip[4]; /* Need a copy because original is overwritten at arp resolution */
	uint8_t srcip[4]; /* Need a copy because original is overwritten at arp resolution */
	IcmpHdr *icmpHdr = (IcmpHdr *)skb_remove_header(skb,sizeof(IcmpHdr));

	*(uint32_t *)dstip = ipHdr->srcaddr32;
	*(uint32_t *)srcip = ipHdr->dstaddr32;

	if(icmpHdr->type == 8) {
		ping_reply(skb,dstip,srcip,ntohs(icmpHdr->id),ntohs(icmpHdr->seqNr),ntohs(ipHdr->totalLength));
	}
}

/**
 *********************************************************
 * Handler for incoming IP packets
 *********************************************************
 */
static void
Eth_HandleIp(EthIf *eth,EthHdr *ethHdr,Skb *skb) 
{
	IpHdr *ipHdr = (IpHdr *)skb_get_header(skb);
	uint8_t hdrlen;
	hdrlen = (ipHdr->versIhl & 0xf) * 4;
	if(hdrlen < 20) {
		Con_Printf("IPHeader to small\n");
		return;
	}
	skb_remove_header(skb,hdrlen);
	
	if(ipHdr->dstaddr32 != eth->if_ip32) 
	{
		return;	
	}

	/* Hack, to lazy for arp requests currently */
	ArpCE_Enter(eth,ipHdr->srcaddr,ethHdr->srcmac); 
	switch(ipHdr->proto) {
		case IPPROT_ICMP:
			//Con_Printf("Detected ICMP\n");
			Eth_HandleICMP(eth,ipHdr,skb); 
			break;

		case IPPROT_TCP:
			Con_Printf("Detected TCP\n");
			//Tcp_ProcessPacket(ipHdr,skb);
			break;

		case IPPROT_UDP:
			Con_Printf("Detected UDP\n");
			break;

		default:
			break;
	}
}

/**
 **********************************************************************
 * Receive an ethernet packet
 **********************************************************************
 */
static void
Eth_PktRx(void *eventData,Skb *skb) 
{
	EthIf *eth = eventData;
	EthHdr *ethHdr = (EthHdr *)skb_remove_header(skb,sizeof(EthHdr));
	switch(ntohs(ethHdr->proto)) {
		case ET_IP:
			Eth_HandleIp(eth,ethHdr,skb);
			break;
		case ET_ARP:
			Eth_HandleArp(eth,ethHdr,skb); 
			break;
		default:
			break;
	}
}

/**
 *************************************************************************
 * \fn static int8_t cmd_arp(Interp * interp, uint8_t argc, char *argv[])
 *************************************************************************
 */
static int8_t
cmd_arp(Interp * interp, uint8_t argc, char *argv[])
{
	EthIf *eth = &g_EthIf;
	ArpCE *ace;
	uint16_t i;
	TimeMs_t now,age;
	now = TimeMs_Get();
	for(i = 0; i < array_size(eth->arpCache); i++) {
		ace = &eth->arpCache[i];
		if(ace->arp_ip32 == 0) {
			continue;	
		}
		age = now - ace->timestamp;
		Interp_Printf_P(interp,"%u.%u.%u.%u  ",
			ace->arp_ip[0],ace->arp_ip[1],ace->arp_ip[2],ace->arp_ip[3]);
		Interp_Printf_P(interp,"%02x:%02x:%02x:%02x:%02x:%02x  Age %lu ms\n",
			ace->arp_mac[0],ace->arp_mac[1],ace->arp_mac[2],ace->arp_mac[3],
			ace->arp_mac[4],ace->arp_mac[5],age);
	}
	return 0;
}


INTERP_CMD(arpCmd,"arp", cmd_arp, "arp  # Show arp cache");

/**
 *****************************************************
 * Configure the IP address using the command shell
 *****************************************************
 */
static int8_t
cmd_ip(Interp * interp, uint8_t argc, char *argv[])
{
	uint8_t i;
	EthIf *eth = &g_EthIf;
	if(argc > 1) {
		uint8_t netmask_bits;
		char *s = argv[1];
		for(i = 0; i < 4; i++) {
			eth->if_ip[i] = astrtoi16(s);
			while(*s && (*s != '.') && (*s != '/')) {
				s++;
			}
			if(*s) {
				s++;
			}
		}
		//Param_Write(ipAddr,eth->if_ip);
		if(ishexnum(s)) {
			netmask_bits = astrtoi16(s);	
			//Param_Write(netmask,&netmask_bits);
			if(netmask_bits == 0) {
				eth->if_netmask32 = UINT32_C(~0); 
			} else {
				eth->if_netmask32 = ntohl(UINT32_C(~0) << (32 - netmask_bits));
			}
		}
	}
	if(argc > 2) {
		//uint8_t netmask_bits;
		char *s = argv[2];
		for(i = 0; i < 4; i++) {
			eth->defaultGW[i] = astrtoi16(s);
			while(*s && (*s != '.') && (*s != '/')) {
				s++;
			}
			if(*s) {
				s++;
			}
		}
		//Param_Write(defaultGW,eth->defaultGW);
	}
	for(i = 0; i < 4; i++) {
		Con_Printf("%u",eth->if_ip[i]);
		if(i != 3) {
			Con_Printf(".",eth->if_ip[i]);
		}
	}
	Con_Printf("/");
	for(i = 0; i < 4; i++) {
		Con_Printf("%u",eth->if_netmask[i]);
		if(i != 3) {
			Con_Printf(".",eth->if_ip[i]);
		}
	}
	Con_Printf(" gw ");
	for(i = 0; i < 4; i++) {
		Con_Printf("%u",eth->defaultGW[i]);
		if(i != 3) {
			Con_Printf(".",eth->if_ip[i]);
		}
	}
	Con_Printf(" \n");
	return 0;
}

INTERP_CMD(ipCmd,"ip",cmd_ip,"ip # show/change IP Address/Mask Gateway");

/**
 *****************************************************************
 * \fn void Ethernet_Init(void) 
 *****************************************************************
 */
void
Ethernet_Init(EthernetDriver *drv) 
{
	EthIf *eth = &g_EthIf;
	uint8_t netmask_bits;
	
	if(!drv) {
		Con_Printf("No ethernet driver available\n");
		return;
	}
	//Param_Read(ipAddr,eth->if_ip);
	if(eth->if_ip32 == ~UINT32_C(0)) {
#if 0
		eth->if_ip[0] = 192;
		eth->if_ip[1] = 168;
		eth->if_ip[2] = 101;
		eth->if_ip[3] = 23;
#else
		eth->if_ip[0] = 192;
		eth->if_ip[1] = 168;
		eth->if_ip[2] = 80;
		eth->if_ip[3] = 10;
#endif
	}
	//Param_Read(netmask,&netmask_bits);
	if(netmask_bits > 32) {
		netmask_bits = 24;
	}
	if(netmask_bits == 0) {
		eth->if_netmask32 = UINT32_C(~0); 
	} else {
		eth->if_netmask32 = ntohl(UINT32_C(~0) << (32 - netmask_bits));
	}
	//Param_Read(defaultGW,eth->defaultGW);
	if(eth->defaultGW32 == ~UINT32_C(0)) {
		eth->defaultGW[0] = 192;
		eth->defaultGW[1] = 168;
		eth->defaultGW[2] = 101;
		eth->defaultGW[3] = 1;
	}
		
	memset(eth->if_mac,0xff,6);
	//I2C_Read8(I2CA_PARAM_EEPROM,0xFA,eth->if_mac,6);
	/* Check the broadcast bit */
	//if(eth->if_mac[0] & 1) {
		Con_Printf("Error: No valid ethernet MAC address in EEPROM\n");
		eth->if_mac[0] = 0x00;
		eth->if_mac[1] = 0x05;
		eth->if_mac[2] = 0x51;
		eth->if_mac[3] = 0x03;
		eth->if_mac[4] = 0x71;
		eth->if_mac[5] = 0xe3;
	//}
	eth->drv = drv;
	drv->regRxProc(drv->driverData,Eth_PktRx,eth);
	drv->setMacProc(drv->driverData,eth->if_mac);
	Interp_RegisterCmd(&arpCmd);
	Interp_RegisterCmd(&ipCmd);
}
