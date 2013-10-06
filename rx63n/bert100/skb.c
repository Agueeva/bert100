/**
 *********************************************************
 * Management of Tx socket buffers
 *********************************************************
 */

#include "skb.h"
#include "tpos.h"
#include "console.h"
#include "interpreter.h"
#include "hex.h"

#define SKB_POOLSIZE	(2)
#define SKB_BUFSIZE	(1600)
static Skb skbPool[SKB_POOLSIZE];
static bool skbFree[SKB_POOLSIZE];
static CSema skbCSema;
static uint8_t txbuf[SKB_BUFSIZE * SKB_POOLSIZE];
static uint32_t skb_waiters = 0;

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
	skb_waiters++;
	CSema_Down(&skbCSema);
	skb_waiters--;
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
			if(skbFree[cursor] == false) {
				skbFree[cursor] = true;	
				Skb_Reset(skb,cursor);
				CSema_Up(&skbCSema);
			} else {
				Con_Printf("Bug: Freeing free SKB\n");
			}
			return;
		} 
	}
	Con_Printf("Freeing skb which is not from Pool\n");
}

static int8_t
cmd_skb(Interp * interp, uint8_t argc, char *argv[])
{
	uint32_t i;
	for(i = 0; i < SKB_POOLSIZE; i++) {
		Con_Printf("Skb %u: ",i);
		if(skbFree[i] == true) {
			Con_Printf("Free\n");
		} else {
			Con_Printf("Busy\n");
		}	
	}
	Con_Printf("Waiters: %u\n",skb_waiters);
	return 0;
}

INTERP_CMD(skbCmd, "skb", cmd_skb, "skb  # Show socket buffer allocator status");

/**
 */
void
Skb_Init(void)
{
	uint32_t i;
	CSema_Init(&skbCSema);
	for(i = 0; i < SKB_POOLSIZE; i++) {
		Skb_Free(&skbPool[i]);
	}
	Interp_RegisterCmd(&skbCmd);
}
