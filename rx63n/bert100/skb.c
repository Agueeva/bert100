/**
 *********************************************************
 * Management of Tx socket buffers
 *********************************************************
 */

#include "skb.h"
#include "tpos.h"
#include "console.h"

#define SKB_POOLSIZE	(2)
#define SKB_BUFSIZE	(1000)
static Skb skbPool[SKB_POOLSIZE];
static bool skbFree[SKB_POOLSIZE];
static CSema skbCSema;
static uint8_t txbuf[SKB_BUFSIZE * SKB_POOLSIZE];

static void
Skb_Reset(Skb *skb,unsigned int index)
{
	int hdrlen = 86;
        skb->hdrBuf = &txbuf[SKB_BUFSIZE * index];
        skb->hdrStart = skb->hdrEnd = skb->hdrBuf + hdrlen; 
	skb->hdrAvailLen = hdrlen;
	skb->hdrBufSize = hdrlen;

	skb->dataStart = skb->dataBuf = skb->hdrBuf + hdrlen; 
	skb->dataBufSize = skb->dataAvailLen = SKB_BUFSIZE - hdrlen; 
	skb->dataLen = 0;
}

Skb *
Skb_Alloc(void)
{
	static uint32_t cursor = 0;
	uint32_t i;
	CSema_Down(&skbCSema);
	for(i = 0; i < SKB_POOLSIZE; i++) {
		if(skbFree[cursor]) {
			skbFree[cursor] = false;
			return &skbPool[cursor];
		}
		cursor = (cursor + 1) % SKB_POOLSIZE;
	}
	Con_Printf("Fatal bug. SKB alloc failed, CSEMA count was wrong\n");
	while(1);
}

void
Skb_Free(Skb *skb)
{
	static uint32_t cursor = 0;
	uint32_t i;
	for(i = 0; i < SKB_POOLSIZE; i++) {
		cursor = (cursor + 1) % SKB_POOLSIZE;
		if(&skbPool[cursor] == skb) {
			skbFree[cursor] = true;	
			Skb_Reset(skb,cursor);
			CSema_Up(&skbCSema);
			return;
		} 
	}
	Con_Printf("Freeing skb which is not from Pool\n");
}

void
Skb_Init(void)
{
	uint32_t i;
	CSema_Init(&skbCSema);
	for(i = 0; i < SKB_POOLSIZE; i++) {
		Skb_Free(&skbPool[i]);
	}
}
