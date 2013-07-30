#ifndef _SKB_H
#define _SKB_H
#include "types.h"
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

void Skb_Init(void);
Skb * Skb_Alloc(void);
void Skb_Free(Skb *skb);
#endif
