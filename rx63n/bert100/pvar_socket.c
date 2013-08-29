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
#include "pvar.h"
#include "xy_string.h"

#define MAX_NAMELEN	(40)
#define MAX_VALUELEN	(40)
typedef struct JSON_Parser {
	uint16_t state;
	bool isGet;
	uint8_t cmdlen;
	uint16_t litvallen;
	uint16_t namelen;
	uint16_t vallen;
	char cmd[3];
	char litval[3];
	char name[MAX_NAMELEN + 1];
	char value[MAX_VALUELEN + 1];
	PVar *pvar;
} JSON_Parser;

JSON_Parser gJSON_Parser;

#define STATE_IDLE		(0)
#define STATE_CMD		(1)
#define STATE_FIND_NAME		(2)
#define STATE_ERROR		(3)
#define STATE_NAME		(4)
#define STATE_DONE		(5)
#define STATE_FIND_LITVAL	(6)
#define STATE_LITVAL		(7)
#define STATE_FIND_VAL		(8)
#define STATE_VAL		(9)

/**
 *******************************************************************************
 * This statemachine understands get and set messages.
 * {"get":"adc12.raw00"}                   
 * {"set":"test.var1","val":2}  	
 *******************************************************************************
 */
INLINE void 
JSON_SMFeed(JSON_Parser *jp,char c) 
{
//	Con_Printf("\"%c\" before %u, ",c,jp->state);
	switch(jp->state) {
		case STATE_IDLE: 	
			if(c == '\"') {		
				jp->state = STATE_CMD;		
				jp->cmdlen = 0;
			} 
			break;
		case STATE_CMD:		
			if(jp->cmdlen >= 3) {
				if(c != '\"') {
					jp->state = STATE_ERROR;
				} else if(strncmp(jp->cmd,"get",3) == 0) {
					jp->isGet = true;
					jp->state = STATE_FIND_NAME;	
				} else if(strncmp(jp->cmd,"set",3) == 0) {
					jp->isGet = false;
					jp->state = STATE_FIND_NAME;
				} else if(strncmp(jp->cmd,"msg",3) == 0) { 
					jp->isGet = false;
					jp->state = STATE_FIND_NAME;
				} else {
					jp->state = STATE_ERROR;
				}
			} else {
				jp->cmd[jp->cmdlen] = c;	
				jp->cmdlen++;
			}
			break;

		case STATE_FIND_NAME:
			if(c == '\"') {		
				jp->namelen = 0;
				jp->state = STATE_NAME;		
			} 
			break;
		
		case STATE_NAME:
			if(c == '\"') {
				jp->name[jp->namelen] = 0; 
				if(jp->isGet) {
					jp->state = STATE_DONE;
				} else {
					jp->state = STATE_FIND_LITVAL;
				}
			} else {	
				jp->name[jp->namelen] = c;
				jp->namelen++;
				if(jp->namelen > MAX_NAMELEN) {
					jp->state = STATE_ERROR;
				}
			}
			break;

		case STATE_FIND_LITVAL:
			if(c == '\"') {		
				jp->state = STATE_LITVAL;		
				jp->litvallen = 0;
			} 
			break;

		case STATE_LITVAL:
			if(jp->litvallen >= 3) {
				if(c != '\"') {
					jp->state = STATE_ERROR;
				} else if(strncmp(jp->litval,"val",3) == 0) {
					jp->state = STATE_FIND_VAL;	
				} else {
					jp->state = STATE_ERROR;
				}
			} else {
				jp->litval[jp->litvallen] = c;	
				jp->litvallen++;
			}
			break;

		case STATE_FIND_VAL:
			if(c == ':') {		
				jp->state = STATE_VAL;		
				jp->vallen = 0;
			} 
			break;

		case STATE_VAL:
			if(c == '}') {
				jp->value[jp->vallen] = 0; 
				jp->state = STATE_DONE;
			} else if(c == '\"') {
				/* currently ignored */
			} else {	
				jp->value[jp->vallen] = c;
				jp->vallen++;
				if(jp->vallen > MAX_VALUELEN) {
					jp->state = STATE_ERROR;
				}
			}
			break;
		
	}
//	Con_Printf(" ,after %u\n",jp->state);
}

INLINE void 
JSON_SMReset(JSON_Parser *jp) 
{
	jp->state = STATE_IDLE;
}

static bool
PVarSock_Connect(WebSocket *ws,void *eventData)
{
	Con_Printf("PVarSock: Got connect\n");
	return true;	
}

static void
SendMsgType(JSON_Parser *jp,WebSocket *ws,const char *msgType) 
{
	char replyStr[MAX_VALUELEN + MAX_NAMELEN + 20];
	char *strP;
	strP = replyStr;
	strP += xy_strcpylen(strP,"{\""); 		
	strP += xy_strcpylen(strP,msgType);
	strP += xy_strcpylen(strP,"\":\""); 		
	strP += xy_strcpylen(strP,jp->name);
	strP += xy_strcpylen(strP,"\",\"val\":");
	strP += xy_strcpylen(strP,jp->value);
	strP += xy_strcpylen(strP,"}");
	*strP = 0;
	WebSocket_SendMsg(ws,WSOP_TEXT,replyStr,strP - replyStr) ;
}

INLINE void
SendVar(JSON_Parser *jp,WebSocket *ws) 
{
	SendMsgType(jp,ws,"var");
}

INLINE void
SendNak(JSON_Parser *jp,WebSocket *ws) 
{
	SendMsgType(jp,ws,"nak");
}

/**
 ********************************************************************
 * After getting a set msg it is acked
 ********************************************************************
 */
INLINE void
SendAck(JSON_Parser *jp,WebSocket *ws) 
{
	SendMsgType(jp,ws,"ack");
}

void
PVarSock_MsgSink(WebSocket *ws,void *eventData,uint8_t op,uint8_t *data,uint16_t len)
{
	JSON_Parser *jp = &gJSON_Parser;
	uint32_t i;
	JSON_SMReset(jp);
	for(i = 0; i < len; i++) {
		JSON_SMFeed(jp,(char)data[i]);
	}	
	if(jp->state != STATE_DONE) {
		Con_Printf("Error in message\n");
		for(i = 0; i < len; i++) {
			Con_Printf("%c",data[i]);
		}
		return;
	}
	jp->pvar = PVar_Find(jp->name);
	if(jp->pvar == NULL) {
		Con_Printf("MSG target \"%s\" not found\n",jp->name);
	} else if(jp->isGet) {
		jp->value[sizeof(jp->value) - 1] = 0;	
		if(PVar_Get(jp->pvar,jp->value,sizeof(jp->value)) == true) {
			SendVar(jp,ws);
		} else {
			SendNak(jp,ws);
		}
	} else {
		if(PVar_Set(jp->pvar,jp->value) == true) {
			SendAck(jp,ws);
		} else {
			SendNak(jp,ws);
		}
	}
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
	XY_WebSocketRegister(wserv,"/messages",&pvarWsOps,NULL);
}
