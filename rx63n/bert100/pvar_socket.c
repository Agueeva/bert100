/**
 ******************************************************************************
 * WebSocket interface to the Process variable server.
 ******************************************************************************
 */
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "xy_web.h"
#include "console.h"
#include "pvar_socket.h"

static bool
PVarSock_Connect(WebSocket *ws,void *eventData)
{
	Con_Printf("PVarSock: Got connect\n");
	return true;	
}

void
PVarSock_MsgSink(WebSocket *ws,void *eventData,uint8_t op,uint8_t *data,uint16_t len)
{
        char copy[17] = {0,};
	//Con_Printf("PVar");
        if(len <= 16) {
                memcpy(copy,data,16);
                copy[len] = 0;
                Con_Printf("%-16s",copy);
        }
	Con_Printf("\n");
}

void
PVarSock_Close(WebSocket *ws,void *eventData)
{
	Con_Printf("PVarSock: Close\n");
}


static WebSockOps pvarWsOps = {
        PVarSock_Connect,
        PVarSock_MsgSink,
        PVarSock_Close
};

void
PVarSocket_New(XY_WebServer *wserv) 
{
	XY_WebSocketRegister(wserv,"/pvars",&pvarWsOps,NULL);
}
