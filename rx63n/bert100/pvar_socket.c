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

static JSON_Parser gJSON_Parser;
#if 0
/**
 *******************************************************************************
 * {"get":"adc12.raw00"}                   
 * {"set":"test.var1","value":2}  	
 *******************************************************************************
 */
static bool 
JSON_HandleMsg(const char *cmd) {
	bool get;	
	const char *name;
	PVar *pvar;
	cmd = strstr(cmd,"\"");
	if(!cmd) {
		return false;
	}
	cmd++;
	if(strncmp(cmd,"get",3) == 0) {
		get = true;
	} else if(strncmp(cmd,"set",3) == 0) {
		get = false;
	} else {
		Con_Printf("Unknown cmd %s\n",cmd);
		return false;
	}
	cmd = strstr(cmd,"\"");
	if(!cmd) {
		return false;
	}
	cmd++;
	cmd = strstr(cmd,"\"");
	if(!cmd) {
		return false;
	}
	cmd++;
	name = cmd;
	cmd = strstr(cmd,"\"");
	if(!cmd)
	{
		return false;
	}
	pvar = PVar_NFind(name,cmd - name);	
	if(!pvar) {
		return false;
	}	
	if(get) {
		Con_Printf("The Name of the Var %08x is %s\n",pvar,name);
			
	} 
	return true;
}
#endif

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

static void 
JSON_Statemachine(JSON_Parser *jp,char c) 
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
				jp->pvar = PVar_Find(jp->name);
				if(!jp->pvar) {
					jp->state = STATE_ERROR;
				} else if(jp->isGet) {
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
				/* Eventuell hier noch fehlerabfrage PVar_Set */
				PVar_Set(jp->pvar,jp->value);
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

static bool
PVarSock_Connect(WebSocket *ws,void *eventData)
{
	Con_Printf("PVarSock: Got connect\n");
	return true;	
}

void
PVarSock_MsgSink(WebSocket *ws,void *eventData,uint8_t op,uint8_t *data,uint16_t len)
{
	uint32_t i;
        char copy[41] = {0,};
	//Con_Printf("PVar");
        if(len <= 40) {
                memcpy(copy,data,40);
                copy[len] = 0;
                Con_Printf("%-40s",copy);
        }
	Con_Printf("\n");
	gJSON_Parser.state = STATE_IDLE;
	for(i = 0; i < len; i++) {
		JSON_Statemachine(&gJSON_Parser,(char)data[i]);
	}	
	Con_Printf("Json state %u\n",gJSON_Parser.state);
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
