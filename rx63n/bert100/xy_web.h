#ifndef XY_WEB_H
#define XY_WEB_H

#include "strhash.h"
#include "tcp.h"

#define XY_WEBSERVERNAME "C-Bert (TPOS)"

#define XY_WEBPAGEBUF 2048 

typedef struct XY_WebServer { 
	//XY_TcpServer tcpserv;
	StrHashTable *rqHandlerHash;
	int connections;
} XY_WebServer;

#define XY_WEBCONTENT_HTML  	(0)
#define XY_WEBCONTENT_GIF   	(1)
#define XY_WEBCONTENT_JPG   	(2)
#define XY_WEBCONTENT_PNG   	(3)
#define XY_WEBCONTENT_CSS   	(4)
#define XY_WEBCONTENT_TEXT    	(5)
#define XY_WEBCONTENT_OCTETS  	(6)
#define XY_WEBCONTENT_PNM  	(7)
#define XY_WEBCONTENT_PBM  	(8)
#define XY_WEBCONTENT_PGM  	(9)
#define XY_WEBCONTENT_PPM  	(10)
#define XY_WEBCONTENT_XML  	(11)
#define XY_WEBCONTENT_MP4  	(12)
#define XY_WEBCONTENT_OGV  	(13)
#define XY_WEBCONTENT_WEBM  	(14)

#if 1
typedef struct XY_WebRequest {
	Tcb *tcb;
	char *page;
	int pagelen; 
	XY_WebServer *wserv;
	int authLevel;
} XY_WebRequest;
#endif

typedef struct WebSocket WebSocket;

typedef int XY_WebReqHandler(int argc,char *argv[],XY_WebRequest *wc,void *cd);
typedef int XY_PostProc(void *eventData,const uint8_t *buf,uint32_t len,uint16_t flags);

#define POST_FLG_START	(1)
int XY_RegisterPostProc(XY_WebServer *,const char *path,XY_PostProc *,void *postData);

XY_WebServer *XY_NewWebServer(void);
int XY_WebRegisterPage(XY_WebServer *wserv,const char *path,XY_WebReqHandler *handler,void *cd);
int XY_WebRegisterFile(XY_WebServer *wserv,const char *filepath,const char *wservpath);
int XY_WebPageUnRegister(XY_WebServer *wserv,const char *path);
int XY_WebAddAuth(XY_WebServer *wserv,const char *uri,const char *realm,const char *auth);

#define XY_WEB_SUBMIT_DELAYED 67
#define XY_WEB_AUTH_REQUIRED  68

int XY_AddHttpHeader(char *page,const char *content_type); 

typedef bool WebSock_ConnectProc(WebSocket *ws,void *wsServerData);
typedef void WebSock_MsgSinkProc(WebSocket *ws,void *eventData,uint8_t op,uint8_t *data,uint16_t len);
typedef void WebSock_CloseProc(WebSocket *ws,void *eventData);

typedef struct WebSockOps {
	WebSock_ConnectProc *connectProc; 
	WebSock_MsgSinkProc *sinkProc;
	WebSock_CloseProc *closeProc;
	void *eventData;
} WebSockOps;

void XY_WebSocketRegister(XY_WebServer *wserv,const char *path,WebSockOps *wops,void *evData,int32_t lvPortNr);
void WebSocket_SendMsg(WebSocket *ws,uint8_t opcode,void *_data,uint16_t pllen);

/* Websocket operations */
#define WSOP_CONTINUATION       (0)
#define WSOP_TEXT               (1)
#define WSOP_BINARY             (2)
#define WSOP_CLOSE              (8)
#define WSOP_PING               (9)
#define WSOP_PONG               (0xa)

#endif 
