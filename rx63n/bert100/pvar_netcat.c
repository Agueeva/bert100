/**
 * Raw TCP interface to the Process variables.
 */

#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <string.h>
#include "config.h"
#include "types.h"
#include "base64.h"
#include "xy_string.h"
#include "iram.h"
#include "tcp.h"
#include "hex.h"
#include "console.h"
#include "events.h"
#include "sha1.h"
#include "timer.h"
#include "iram.h"
#include "md5.h"
#include "database.h"
#include "interpreter.h"
#include "pvar.h"

#define MAX_CONNECTIONS	(3)

typedef struct PVarNetCat_Connection {
	char cmdbuf[60];
	uint16_t cmdbufWp;
	bool busy;
} PVarNetCat_Connection; 

typedef struct PVarNetCat {
	PVarNetCat_Connection con[MAX_CONNECTIONS];
} PVarNetCat;

static PVarNetCat gPvnc;

static uint16_t
PVarNetCat_DataSink(void *eventData, uint32_t fpos, const uint8_t * buf, uint16_t len)
{
	//PVarNetCat_Connection *con = eventData;
	return len;
}

static uint16_t
PVarNetCat_DataSrc(void *eventData, uint32_t fpos, void **_buf, uint16_t maxlen)
{
	//PVarNetCat_Connection *con = eventData;
	return 0;
}


static void
PVarNetCat_CloseProc(void *eventData)
{
	PVarNetCat_Connection *con = eventData;
	con->busy = false;
}


static struct Tcb_Operations pvncTcbOps = {
        .sinkProc = PVarNetCat_DataSink,
        .srcProc = PVarNetCat_DataSrc,
        .closeProc = PVarNetCat_CloseProc
};

static PVarNetCat_Connection *
ConnectionAlloc(PVarNetCat *pvnc) {
	unsigned int i;
	for(i = 0; i < array_size(pvnc->con); i++) {
		PVarNetCat_Connection *con = &pvnc->con[i];
		if(con->busy == false) {
			return con;
		}
	}
	return NULL;
}

static bool
PVarNetCat_Accept(void *eventData, Tcb * tcb, uint8_t ip[4], uint16_t port)
{
        PVarNetCat *pvnc = eventData;
	PVarNetCat_Connection *con;
	con = ConnectionAlloc(pvnc);
	if(!con) {
		Con_Printf("All PVar NetCat connections in use\n");
		return false;
	}
	Tcb_RegisterOps(tcb, &pvncTcbOps, con);
	return true;
}


void
PVarNetCat_Init() {
	PVarNetCat *pvnc = &gPvnc;
	Tcp_ServerSocket(8200, PVarNetCat_Accept, pvnc);
}
