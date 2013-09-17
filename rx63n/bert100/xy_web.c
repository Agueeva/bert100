/*
 ************************************************************
 *
 * (C) Jochen Karrer 2002
 *
 ************************************************************
 */
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <string.h>

#include "types.h"
#include "base64.h"
#include "xy_web.h"
#include "xy_string.h"
#include "iram.h"
#include "tcp.h"
#include "hex.h"
#include "console.h"
#include "events.h"
#include "sha1.h"
#include "fat.h"
#include "timer.h"
#include "iram.h"
#include "md5.h"

#define REALM	"Munich Instruments C-BERT"

char *content_string[] = {
	"text/html",
	"image/gif",
	"image/jpeg",
	"image/png",
	"text/css",
	"text/plain",
	"application/octet-stream",
	"image/x-portable-anymap",
	"image/x-portable-bitmap",
	"image/x-portable-graymap",
	"image/x-portable-pixmap",
	"text/xml",
	"video/mp4",
	"video/ogg",
	"video/webm",
};

#define XY_WEB_HEADER "HTTP/1.1 200 OK\r\n" \
        "Server: " XY_WEBSERVERNAME "\r\n" \
        "Connection: close\r\n" \
        "Content-Type: "

#define NOT_FOUND_HEADER "HTTP/1.1 404 Not Found\r\n" \
	"Server: " XY_WEBSERVERNAME "\r\n" \
	"Connection: close\r\n" \
	"Content-Type: text/html\r\n\r\n"

typedef struct XYWebAuthInfo {
	char *realm;
	char *username;
	uint8_t passwd_md5[16];
	struct XYWebAuthInfo *next;
} XYWebAuthInfo;

#define PAGE_TYPE_WEBPAGE	(1)
#define PAGE_TYPE_WEBSOCKET	(2)

typedef struct WebPageRegistration {
	XY_WebReqHandler *getProc;
	void *getData;
	XY_PostProc *postProc;
	void *postData;
	XYWebAuthInfo *authInfo;
} WebPageRegistration;


#define WS_TCP_MAX_WIN_SZ 500

#define WSMSGS_OPC	(0)
#define WSMSGS_PLLB	(1)
#define WSMSGS_PLL16HI	(2)
#define WSMSGS_PLL16LO	(3)
#define WSMSGS_PLL64_0	(4)
#define WSMSGS_PLL64_1	(5)
#define WSMSGS_PLL64_2	(6)
#define WSMSGS_PLL64_3	(7)
#define WSMSGS_PLL64_4	(8)
#define WSMSGS_PLL64_5	(9)
#define WSMSGS_PLL64_6	(10)
#define WSMSGS_PLL64_7	(11)
#define WSMSGS_MSK_0	(12)
#define WSMSGS_MSK_1	(13)
#define WSMSGS_MSK_2	(14)
#define WSMSGS_MSK_3	(15)
#define WSMSGS_PAYLOAD	(16)
#define WSMSGS_IGNORE	(17)

#define WS_OUTBUF_SIZE	(1536)
#define WS_INBUF_SIZE	(512)

/**
 ****************************************************************
 * The following ws_states are defined in w3org html5 websockets
 ****************************************************************
 */
#define WSST_CONNECTING	(0)
#define WSST_OPEN	(1)
#define WSST_CLOSING	(2)
#define WSST_CLOSED	(3)

typedef struct WebCon WebCon;

struct WebSocket {
	Tcb *tcb;
	WebSockOps *wops;
	void *wopsConData;

	WebCon *handoverWc;	/* Freed as soon as possible after handover */
	uint8_t msg_state;
	uint8_t ws_state;	/* As defined by w3org */
	bool msg_masked;
	uint8_t msg_opc;	/* Opcode of the last message which was not of type Continuation */
	uint16_t msg_pllen;
	uint16_t msg_plrcvd;
	uint8_t msg_msk[4];

	uint16_t inbuf_rp;
	uint16_t inbuf_wp;

	/**
 	 ***************************************************************
 	 * Circular buffer for variable sized objects.
  	 * The wrap pointer is changed when an object does not fit into
 	 * the buffer any more.
 	 ***************************************************************
 	 */
	uint16_t outbuf_una;	/* Last unacknowledged */
	uint16_t outbuf_snd;	/* Send pointer, same as in TCP */
	uint16_t outbuf_wp;
	uint16_t outbuf_wrap_pt;

	uint8_t outBuf[WS_OUTBUF_SIZE];
	uint8_t inBuf[WS_INBUF_SIZE];
};

static uint16_t WebSocket_DataSink(void *eventData, uint32_t fpos, const uint8_t * buf, uint16_t len);
static struct Tcb_Operations websockTcbOps;
static struct Tcb_Operations fileTcbOps;

#define MAX_VARS     (30)
#define MAX_PAGEARGS (10)
#define REQBUFLEN XY_WEBPAGEBUF
/**
 * HTTP states
 */
#define HTS_IDLE	(0)
#define HTS_GET		(1)
#define HTS_POST	(2)
#define HTS_WEBSOCK	(3)
#define HTS_FILE	(4)

struct WebCon {
	Tcb *tcb;
	char *page;
	int pagelen;
	XY_WebServer *wserv;
	int authLevel;

	/* Start of private area */
	WebSocket *ws;
	WebPageRegistration *pageOps;
	uint8_t http_state;
	uint32_t postLen;
	uint32_t contentLength;
	int reqbuf_wp;
	int out_rp;
	int argc;
	char *argv[MAX_PAGEARGS];
	int linec;
	char *linev[MAX_VARS];

	/* File type web request */
	FIL file;
	bool file_isopen;
	char reqbuf[0];
	char data[XY_WEBPAGEBUF];
};

/**
 *************************************************************
 * Memory management for Web Connections and WebSockets
 *************************************************************
 */
#define NR_WCONS	5
static WebCon *wcpool[NR_WCONS];
static uint8_t wcpool_initialized = 0;

static WebCon *
WebCon_Alloc(void)
{
	WebCon *wc;
	uint16_t i;
	for (i = 0; i < NR_WCONS; i++) {
		if (wcpool[i] != NULL) {
			wc = wcpool[i];
			wcpool[i] = NULL;
			return wc;
		}
	}
	return NULL;
}

static void
WebCon_Free(WebCon * wc)
{
	uint16_t i;
	if (wc == NULL) {
		Con_Printf("BUG: free zero wc\n");
		return;
	}
	//EV_Yield();
	//memset(wc,0,sizeof(WebCon) - XY_WEBPAGEBUF);
	memset(wc, 0, offsetof(WebCon, data));

	for (i = 0; i < NR_WCONS; i++) {
		if (wcpool[i] == wc) {
			Con_Printf("BUG: double free of WebCon\n");
			return;
		}
	}
	for (i = 0; i < NR_WCONS; i++) {
		if (wcpool[i] == NULL) {
			wcpool[i] = wc;
			return;
		}
	}
	Con_Printf("Bug: WCON pool overflow\n");
}

#define NR_WEBSOCKS 5
static WebSocket *wspool[NR_WEBSOCKS];
static uint8_t wspool_initialized = 0;

static WebSocket *
WebSocket_Alloc(void)
{
	WebSocket *ws;
	uint16_t i;
	for (i = 0; i < NR_WEBSOCKS; i++) {
		if (wspool[i] != NULL) {
			ws = wspool[i];
			wspool[i] = NULL;
			ws->tcb = NULL;
			ws->handoverWc = NULL;
			ws->msg_state = WSMSGS_OPC;
			ws->ws_state = WSST_CONNECTING;
			ws->outbuf_wp = 0;
			ws->outbuf_snd = ws->outbuf_una = 0;
			ws->outbuf_wrap_pt = WS_OUTBUF_SIZE;
			ws->inbuf_wp = ws->inbuf_rp = 0;
			return ws;
		}
	}
	return NULL;
}

static void
WebSocket_Free(WebSocket * ws)
{
	uint16_t i;
	if (ws == NULL) {
		Con_Printf("BUG: free zero ws\n");
		return;
	}
	for (i = 0; i < NR_WEBSOCKS; i++) {
		if (wspool[i] == ws) {
			Con_Printf("BUG: double free of WebSocket\n");
			return;
		}
	}
	for (i = 0; i < NR_WEBSOCKS; i++) {
		if (wspool[i] == NULL) {
			wspool[i] = ws;
			return;
		}
	}
	Con_Printf("Bug: WSPOOL pool overflow\n");
}

/**
 *******************************************************************
 * \fn static void WebCon_PoolInit(void);
 *******************************************************************
 */
static void
Web_PoolsInit(void)
{
	uint16_t i;
	if (wcpool_initialized | wspool_initialized) {
		return;
	}
	for (i = 0; i < NR_WCONS; i++) {
		wcpool[i] = IRam_Calloc(sizeof(WebCon));
	}
	for (i = 0; i < NR_WEBSOCKS; i++) {
		wspool[i] = IRam_Calloc(sizeof(WebSocket));
	}
	wcpool_initialized = 1;
	wspool_initialized = 1;
}

/**
 *****************************************************************
 * Add an http header depending on content type
 *****************************************************************
 */
int
XY_AddHttpHeader(char *page, const char *content_type)
{
	int i = 4;
	const char *str = XY_WEB_HEADER;
	while ((*page = *str)) {
		page++;
		str++;
		i++;
	}
	str = content_type;
	while ((*page = *str)) {
		page++;
		str++;
		i++;
	}
	*page++ = '\r';
	*page++ = '\n';
	*page++ = '\r';
	*page++ = '\n';
	return i;
}

/*
 **************************************************
 * start response 
 **************************************************
 */
static void
web_submit_page(WebCon * wc)
{
	TcpCon_ControlTx(wc->tcb, true);
}

static void
create_notfoundpage(WebCon * wc)
{
	char *page = wc->page;
	page += xy_strcpylen(page, NOT_FOUND_HEADER);
	page += xy_strcpylen(page,
			     "<html><head><title>Page not found</title></head>\n"
			     "<body>\nThis webpage does not exist.\n</body></html>\n\r\n");
	wc->pagelen = page - wc->page;
	web_submit_page(wc);
}

/**
 **************************************************************************
 * Redirects a page to index.html
 **************************************************************************
 */
static int
redirect_page(int argc, char *argv[], XY_WebRequest * wr, void *eventData)
{
	char *page = wr->page;
	page += XY_AddHttpHeader(page, content_string[XY_WEBCONTENT_HTML]);
	page += xy_strcpylen(page,
			     "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" "
			     "\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\r\n\r\n");
	page += xy_strcpylen(page,
			     "<html>\n<head>\n"
			     "<title>A redirect page</title>\n"
			     "<meta http-equiv=\"refresh\" content=\"0; URL=/sd/gui/index.html\">\n"
			     "</head>\n<body><center><H1>Redirect Page</H1><center>\n"
			     "<a href=\"/sd/gui/index.html\">click here</a>\n"
			     "</body>\n</html>\n\r\n");
	wr->pagelen = page - wr->page;
	web_submit_page((WebCon *) wr);
	return 0;
}

/*
 ************************************************************************
 * causes  the browser to open a popup window for
 * username/password 
 ************************************************************************
 */
static void
create_authpage(WebCon * wc, char *realm)
{
	char *page = wc->page;
	page += xy_strcpylen(page,
			     "HTTP/1.1 401 Authorization Required\r\n"
			     "Server: " XY_WEBSERVERNAME "\r\n"
			     "Connection: close\r\n"
			     "Content-Type: text/html\r\n" "WWW-Authenticate: Basic realm=\"");
	page += xy_strcpylen(page, realm);
	page += xy_strcpylen(page, "\"\r\n\r\n");
	page += xy_strcpylen(page,
			     "<html><head><title>Authorization Required</title></head>\n"
			     "<body>\nThis page requires Authorization\n</body></html>\n\r\n");
	wc->pagelen = page - wc->page;
	web_submit_page(wc);
}

/*
 *************************************************************************
 * get the args from a cgi invokation URL line 
 * Input example: "bla.cgi?x=5&z=7&hello"
 *************************************************************************
 */
static int
split_args(char *line, int maxargc, char *argv[])
{
	int i, argc = 1;
	argv[0] = line;
	for (i = 0; line[i]; i++) {
		if ((line[i] == ' ')) {
			line[i] = 0;
			return argc;
		}
		if ((argc == 1) && (line[i] == '?')) {
			line[i] = 0;
			argv[argc] = line + i + 1;
			argc++;
		} else if ((argc > 1) && (line[i] == '&')) {
			line[i] = 0;
			if (argc < maxargc) {
				argv[argc] = line + i + 1;
				argc++;
			}
		}
	}
	return argc;
}

/**
 *******************************************************************************
 * Find a request handler 
 *******************************************************************************
 */
static WebPageRegistration *
FindRequestHandler(XY_WebServer * wserv, const char *path)
{
	StrHashEntry *entryPtr;
	int i;
	WebPageRegistration *reg;
	entryPtr = StrHash_FindEntry(wserv->rqHandlerHash, path);
	if (entryPtr) {
		reg = (WebPageRegistration *) StrHash_GetValue(entryPtr);
		return reg;
	}
	for (i = strlen(path); (i > 1); i--) {
		char c;
		if (path[i - 1] == '/') {
			c = path[i];
			entryPtr = StrNHash_FindEntry(wserv->rqHandlerHash, path, i);
			if (entryPtr) {
				//Con_Printf("Found directory of path\n");
				reg = (WebPageRegistration *) StrHash_GetValue(entryPtr);
				return reg;
			}
		}
	}
	return NULL;
}

/**
 **************************************************************+
 * Extract a variable from the linev (http header)
 **************************************************************+
 */
static void
Extract_Var(char *linev[], char *keyword, char **valp)
{
	char *v, *end;
	uint16_t i;
	//Con_Printf("Extracting keyword \"%s\"\n",keyword);
	for (i = 0; i < MAX_VARS; i++) {
		if (!linev[i])
			break;
		if (xy_strniseq(linev[i], keyword, strlen(keyword))) {
			v = linev[i] + strlen(keyword);
			if (*v == ':') {
				v++;
			}
			while ((*v == ' ') || (*v == '\t')) {
				v++;
			}
			end = v;
			while (*end && (*end != '\r') && (*end != '\n') /* && (*end != ' ') */ ) {
				end++;
			}
			*end = 0;
			*valp = v;
			return;
		}
	}
	*valp = 0;

}

/**
 *****************************************************************
 * check authorization
 *****************************************************************
 */
static int
check_auth(WebCon * wc, WebPageRegistration * reg)
{
	int i;
	XYWebAuthInfo *auth;
	int level = 0;
	int result = -1;
	for (i = 0; i < MAX_VARS; i++) {
		if (!wc->linev[i])
			break;
		if (xy_strniseq(wc->linev[i], "Authorization:", 14)) {
			char *authstr, *username, *colon, *passwd;
			unsigned char passwd_md5[16];
			char *auth_b64 = xy_strstr(wc->linev[i] + 14, "Basic");
			unsigned int len;
			if (!auth_b64)
				return -1;
			auth_b64 += 5;
			while (*auth_b64 == ' ')
				auth_b64++;
			len = xy_strlen(auth_b64);
			if (len > 128) {
				return -1;
			}
			authstr = __builtin_alloca(len + 1);
			len = Base64_Decode(authstr, auth_b64, len);
			authstr[len] = 0;
			colon = xy_strstr(authstr, ":");
			if (!colon)
				return -1;
			*colon = 0;
			passwd = colon + 1;
			username = authstr;
			MD5_EncodeString(passwd_md5, passwd);
			//Con_Printf("encoded passwd \"%s\"\n",passwd);
			for (auth = reg->authInfo; auth; auth = auth->next) {
				level++;
				//Con_Printf("username \"%s\", expected \"%s\"\n",username,auth->username);
				if (xy_striseq(username, (char *)auth->username)) {
					if (!memcmp(passwd_md5, auth->passwd_md5, 16)) {
						result = level;
					}
				}
			}
			if (result >= 0) {
				wc->authLevel = result;
			}
			return result;
		}
	}
	/* Lowest level is allowed to have no protection without popup for passwd */
	if (reg->authInfo && (reg->authInfo->username[0] == 0)) {
		wc->authLevel = 1;
	} else {
		return -1;
	}
	return 0;
}

/*
 ********************************************************************** 
 * Find and execute the requesthandler for a page
 ********************************************************************** 
 */
static void
execute_web_request(WebCon * wc, char *line)
{
	XY_WebServer *wserv = wc->wserv;
	uint16_t i;
	//Con_Printf("execute web request lin %s\n",line);
	if (xy_strniseq("GET", line, 3)) {
		int result;
		WebPageRegistration *reg;
		line = xy_strstr(line + 3, "/");
		wc->http_state = HTS_GET;
		if (!line)
			return;
		wc->argc = split_args(line, MAX_PAGEARGS, wc->argv);
		if (!wc->argc)
			return;
		wc->pageOps = reg = FindRequestHandler(wserv, wc->argv[0]);
		if (!reg || !reg->getProc) {
			create_notfoundpage(wc);	// not foundpage
			return;
		}
		if (reg->authInfo) {
			if (check_auth(wc, reg) < 0) {
				create_authpage(wc, reg->authInfo->realm);
				return;
			}
		}
		EV_Yield();
		result = reg->getProc(wc->argc, wc->argv, (XY_WebRequest *) wc, reg->getData);
		EV_Yield();
		if (result < 0) {
			create_notfoundpage(wc);
			return;
		} else if (result == XY_WEB_SUBMIT_DELAYED) {
			return;
		} else if (result == XY_WEB_AUTH_REQUIRED) {
			int level = 0;
			XYWebAuthInfo *auth;
			for (auth = reg->authInfo; auth; auth = auth->next) {
				level++;
				if (level > wc->authLevel) {
					create_authpage(wc, auth->realm);
					return;
				}
			}
			create_notfoundpage(wc);
			return;
		} else {
			web_submit_page(wc);
		}
	} else if (xy_strniseq("POST", line, 4)) {
		WebPageRegistration *reg;
		line = xy_strstr(line + 4, "/");
		//Con_Printf("Detected a post\n");
		wc->http_state = HTS_POST;
		if (!line)
			return;
		wc->argc = split_args(line, MAX_PAGEARGS, wc->argv);
		if (!wc->argc) {
			return;
		}
		wc->pageOps = reg = FindRequestHandler(wserv, wc->argv[0]);
		if (!reg || !reg->postProc) {
			Con_Printf("Not found a postproc for the page %s\n", wc->argv[0]);
			create_notfoundpage(wc);	// not foundpage
			return;
		}
		//Con_Printf("Found a Post handler\n");
		for (i = 0; i < wc->linec; i++) {
			char *str = wc->linev[i];
			if (xy_strstr(str, "Content-Length:")) {
				str = str + 15;
				while (*str == ' ') {
					str++;
				}
				wc->contentLength = astrtoi32(str);
			}
		}
		//Con_Printf("Vars end, content length %llu\n",wc->contentLength);
	} else {
		// free wc wenn TCB callback with closed message
		Tcp_Close(wc->tcb);
	}
}

/**
 **************************************************************
 * Feed a post handler
 **************************************************************
 */
static void
Feed_PostData(WebCon * wc, const uint8_t * data, uint16_t len, uint16_t flags)
{
	if (wc->http_state != HTS_POST) {
		Con_Printf("BUG: called post without POST state\n");
		return;
	}
	wc->postLen += len;
	if (wc->pageOps->postProc) {
		EV_Yield();
		wc->pageOps->postProc(wc->pageOps->postData, data, len, flags);
		EV_Yield();
	}
	if (wc->postLen >= wc->contentLength) {
		WebPageRegistration *ops = wc->pageOps;
		if (ops->getProc) {
			int result;
			//Con_Printf("Content-Length reached, GET page now\n");
			wc->http_state = HTS_GET;
			EV_Yield();
			result = ops->getProc(wc->argc, wc->argv, (XY_WebRequest *) wc, ops->getData);
			EV_Yield();
			if (result < 0) {
				create_notfoundpage(wc);
				return;
			} else {
				web_submit_page(wc);
			}
		} else {
			wc->http_state = HTS_IDLE;
			//Con_Printf("Content-Length reached, closing\n");
			Tcp_Close(wc->tcb);
		}
	}
}

/*
 ***********************************************************
 * receive a request
 ***********************************************************
 */
static uint16_t 
WebServ_DataSink(void *eventData, uint32_t fpos, const uint8_t * buf, uint16_t len)
{
	WebCon *wc = (WebCon *) eventData;
	int j;
	uint16_t old_wp;
	old_wp = wc->reqbuf_wp;
	if (wc->http_state == HTS_POST) {
		Feed_PostData(wc, buf, len, 0);
		return len;
	} else if (wc->http_state == HTS_GET) {
		return len;
	}
	for (j = 0; j < len; j++) {
		if (wc->reqbuf_wp == (REQBUFLEN - 1)) {
			Tcp_Close(wc->tcb);
			return len;
		}
		wc->reqbuf[wc->reqbuf_wp] = buf[j];
		wc->reqbuf_wp++;
		if ((j & 0x3f) == 0x3f) {
			EV_Yield();
		}
	}
	wc->reqbuf[wc->reqbuf_wp] = 0;	// we kept one byte for termination 
	EV_Yield();
	for (j = old_wp; j < wc->reqbuf_wp; j++) {
		char c = wc->reqbuf[j];
		if ((j & 0x3f) == 0x3f) {
			EV_Yield();
		}
		if (c == '\n') {
			wc->reqbuf[j] = 0;
			if (j > 2) {
				if ((wc->reqbuf[j - 2] == 0) || (wc->reqbuf[j - 1] == 0)) {
					execute_web_request(wc, wc->reqbuf);
					if (wc->http_state == HTS_POST) {
						Feed_PostData(wc, (uint8_t *) wc->reqbuf + j,
							      wc->reqbuf_wp - j - 1, POST_FLG_START);
					}
					return len;
				}
			}
			if (wc->linec < (MAX_VARS - 1)) {
				wc->linec++;
			}
		} else if (wc->linev[wc->linec] == NULL) {
			wc->linev[wc->linec] = wc->reqbuf + j;
		}
	}
	return len;
}

/**
 ********************************************************************************************
 * \fn uint16_t WebServ_DataSrc(void *eventData,uint32_t fpos,uint8_t *buf,uint16_t maxlen)
 ********************************************************************************************
 */
static uint16_t
WebServ_DataSrc(void *eventData, uint32_t fpos, void **_buf, uint16_t maxlen)
{
	WebCon *wc = (WebCon *) eventData;
	uint16_t count;
	//int i;
	bool is_ws = false;
	char **buf = (char **)_buf;
	*buf = wc->page + wc->out_rp;
	count = wc->pagelen - wc->out_rp;
	if (count > maxlen) {
		count = maxlen;
	}
	wc->out_rp += count;
	if (wc->http_state == HTS_WEBSOCK) {
		is_ws = true;
	}
	/**
	 ************************************************************************************
 	 * If this is a websocket then handover the control to the socket user, else close
	 ************************************************************************************
 	 */
	if (wc->out_rp == wc->pagelen) {
		TcpCon_ControlTx(wc->tcb, false);
		if (wc->http_state == HTS_GET) {
			Tcp_Close(wc->tcb);
		} else if (wc->http_state == HTS_POST) {
			Tcp_Close(wc->tcb);
		} else if (wc->http_state == HTS_IDLE) {
			Tcp_Close(wc->tcb);
		} else if (wc->http_state == HTS_WEBSOCK) {
			WebSocket *ws = wc->ws;
			if (!ws) {
				Con_Printf("\nBug, missing WebSocket\n");
				Tcp_Close(wc->tcb);
				return 0;
			}
			//Con_Printf("\nHandover to WebSocket Server\n");
			ws->tcb = wc->tcb;
			Tcb_RegisterOps(ws->tcb, &websockTcbOps, ws);
			Tcb_SetMaxWinSize(ws->tcb,WS_TCP_MAX_WIN_SZ);
			ws->handoverWc = wc;
			//WebCon_Free(wc);
			//WebSocket_SendMsg(ws,WSOP_TEXT,"Kasper hat Geburtstag",21) ;
		} else if (wc->http_state == HTS_FILE) {
			//Con_Printf("\nHandover to File Server\n");
			Tcb_RegisterOps(wc->tcb, &fileTcbOps, wc);
			TcpCon_ControlTx(wc->tcb, true);
		}
	}
	return count;
}

/**
 ****************************************************************************
 * Allocate outbuf space
 * If there is no room at the end of the outbuf restart at the beginning
 * and set the wrap pointer to the   
 ****************************************************************************
 */
uint8_t *
WebSocket_AllocObufSpace(WebSocket * ws, uint16_t size)
{
	uint8_t *buf;
	if (ws->outbuf_wp < ws->outbuf_una) {
		if ((ws->outbuf_wp + size) > ws->outbuf_una) {
			buf = NULL;
		} else {
			buf = ws->outBuf + ws->outbuf_wp;
		}
	} else if ((ws->outbuf_wp + size) >= WS_OUTBUF_SIZE) {
		if (size > ws->outbuf_una) {
			/* temporarily failed;  */
			buf = NULL;
		} else {
			ws->outbuf_wrap_pt = ws->outbuf_wp;
			//ws->outbuf_wp = size;
			ws->outbuf_wp = 0;
			buf = ws->outBuf;
		}
	} else {
		buf = ws->outBuf + ws->outbuf_wp;
	}
#if 0
	Con_Printf("sz %u, obwp %u, wpt %u, una %u, ofs %lu, buf %lu\n",
		size,ws->outbuf_wp,ws->outbuf_wrap_pt,ws->outbuf_una,buf - ws->outBuf,buf);
#endif
	return buf;
}

void
WebSocket_SubmitSpace(WebSocket * ws, uint16_t size)
{
	//Con_Printf("Submited %d bytes\n",size);
	ws->outbuf_wp += size;
	TcpCon_ControlTx(ws->tcb, true);
}

/**
 *****************************************************************************************************
 * \fn static uint16_t WebSocket_DataSrc(void *eventData,uint32_t fpos,uint8_t **buf,uint16_t maxlen)
 *****************************************************************************************************
 */
static uint16_t
WebSocket_DataSrc(void *eventData, uint32_t fpos, void **_buf, uint16_t maxlen)
{

	uint16_t len;
	uint8_t **buf = (uint8_t **) _buf;
	WebSocket *ws = (WebSocket *) eventData;
	//Con_Printf("Called the WebSocket data src\n");
	if (ws->handoverWc) {
		WebCon_Free(ws->handoverWc);
		ws->handoverWc = NULL;
	}
	if (ws->outbuf_snd != ws->outbuf_una) {
		if (ws->outbuf_snd < ws->outbuf_una) {
			/* Wrap detected */
			ws->outbuf_wrap_pt = WS_OUTBUF_SIZE;
		}
		ws->outbuf_una = ws->outbuf_snd;
	}
	if (ws->outbuf_snd == ws->outbuf_wp) {
		Con_Printf("Should not happen, because it should be already disa\n");
		TcpCon_ControlTx(ws->tcb, false);
		return 0;
	}
	if (ws->outbuf_wp >= ws->outbuf_snd) {
		len = ws->outbuf_wp - ws->outbuf_snd;
	} else {
		len = ws->outbuf_wp;
		ws->outbuf_snd = 0;
	}
	*buf = ws->outBuf + ws->outbuf_snd;
	if (len > maxlen) {
		len = maxlen;
	}
	//Con_Printf("outbuf_snd %u, len %u\n",ws->outbuf_snd,len);
	ws->outbuf_snd += len;

#if 0
	Con_Printf("SND %u, wrap %u, wp %u, una %u len %u\n", ws->outbuf_snd, ws->outbuf_wrap_pt,
		   ws->outbuf_wp, ws->outbuf_una, len);
#endif

	if (ws->outbuf_snd > ws->outbuf_wrap_pt) {
		Con_Printf("%s Bug: Send pointer past Wrap pointer %u\n",__FILE__,ws->outbuf_wrap_pt);
		Con_Printf("snd %u, una %u, wp %u\n",ws->outbuf_snd,ws->outbuf_una,ws->outbuf_wp);
	} else if (ws->outbuf_snd == ws->outbuf_wrap_pt) {
		ws->outbuf_snd = 0;
	}
	if (ws->outbuf_snd == ws->outbuf_wp) {
		TcpCon_ControlTx(ws->tcb, false);
	}
	return len;
}

/*
 *******************************************************************************************
 * \fn static void File_DataSrc(void *eventData,uint32_t fpos,uint8_t **buf,uint16_t maxlen)
 *******************************************************************************************
 */

static uint16_t
File_DataSrc(void *eventData, uint32_t fpos, void **_buf, uint16_t maxlen)
{
	UINT size;
	FRESULT res;
	char **buf = (char **)_buf;
	WebCon *wc = (WebCon *) eventData;
	uint16_t count = maxlen;
	if (maxlen > XY_WEBPAGEBUF) {
		count = XY_WEBPAGEBUF;
	} else {
		count = maxlen;
	}
	res = f_read(&wc->file, wc->page, count, &size);
	*buf = wc->page;
	if ((res != FR_OK) || (size == 0)) {
		//Con_Printf("Read failed with %d, size %u\n",res,size);
		res = f_close(&wc->file);
		if (res == FR_OK) {
			wc->file_isopen = false;
		}
		TcpCon_ControlTx(wc->tcb, false);
		Tcp_Close(wc->tcb);
		return 0;
	}
	return size;
}

/*
 **************************************************************************************
 * \fn static const char * find_suffix(const char *path); 
 * find the suffix of the filename (required for determining the content type)
 **************************************************************************************
 */

static const char *
find_suffix(const char *path)
{
	int i;
	for (i = xy_strlen(path); i >= 0; i--) {
		if (path[i] == '.')
			return path + i + 1;
	}
	return NULL;
}

static int
content_type_from_suffix(const char *path)
{
	const char *suffix = find_suffix(path);
	int content_type = XY_WEBCONTENT_TEXT;
	if (suffix) {
		if (xy_striseq("htm", suffix)) {
			content_type = XY_WEBCONTENT_HTML;
		} else if (xy_striseq("html", suffix)) {
			content_type = XY_WEBCONTENT_HTML;
		} else if (xy_striseq("gif", suffix)) {
			content_type = XY_WEBCONTENT_GIF;
		} else if (xy_striseq("jpg", suffix)) {
			content_type = XY_WEBCONTENT_JPG;
		} else if (xy_striseq("jpeg", suffix)) {
			content_type = XY_WEBCONTENT_JPG;
		} else if (xy_striseq("png", suffix)) {
			content_type = XY_WEBCONTENT_PNG;
		} else if (xy_striseq("css", suffix)) {
			content_type = XY_WEBCONTENT_CSS;
		} else if (xy_striseq("class", suffix)) {
			content_type = XY_WEBCONTENT_OCTETS;
		} else if (xy_striseq("xml", suffix)) {
			content_type = XY_WEBCONTENT_XML;
		} else if (xy_striseq("dtd", suffix)) {
			content_type = XY_WEBCONTENT_XML;
		} else if (xy_striseq("mp4", suffix)) {
			content_type = XY_WEBCONTENT_MP4;
		} else if (xy_striseq("ogv", suffix)) {
			content_type = XY_WEBCONTENT_OGV;
		} else if (xy_striseq("webm", suffix)) {
			content_type = XY_WEBCONTENT_WEBM;
		}
	}
	if (content_type >= array_size(content_string)) {
		Con_Printf("Bug, content type not in list\n");
		return 0;
	}
	return content_type;
}

/*
 ******************************************************************************
 * Fat file service
 * Checks for file presence and prepares handover to the FatReader data source.
 * The handover will occur when all http header data are sent.
 ******************************************************************************
 */
static int
Page_FatFile(int argc, char *argv[], XY_WebRequest * wr, void *eventData)
{
	WebCon *wc = (WebCon *) wr;
	FRESULT res;
	char *page = wc->page;
	int content_type;
	int i;
	char *path = argv[0];
	if (strlen(path) == 0) {
		return -1;
	}
	for (i = 1; i < strlen(path); i++) {
		if (path[i] == 0) {
			return -1;
		}
		if (path[i] == '/') {
			path += i;
			break;
		}
	}
	content_type = content_type_from_suffix(path);
	res = f_open(&wc->file, path, FA_OPEN_EXISTING | FA_READ);
	if (res != FR_OK) {
		return -1;
	}
	wc->file_isopen = true;
	page += XY_AddHttpHeader(page, content_string[content_type]);
	wc->pagelen = page - wc->page;
	wc->http_state = HTS_FILE;
	return 0;
}

/**
 **************************************************************
 * \fn static void WebServ_CloseProc(void *eventData)
 * The close proc is called by the TCP implementation when
 * the close is complete. 
 **************************************************************
 */
static void
WebServ_CloseProc(void *eventData)
{
	WebCon *wc = (WebCon *) eventData;
	if (wc == NULL) {
		Con_Printf("Bug: Called closeproc without wc\n");
		return;
	}
	if (wc->http_state == HTS_FILE) {
		if (wc->file_isopen) {
			f_close(&wc->file);
		}
	}
	if (wc->wserv == NULL) {
		Con_Printf("BUG: close already memsetted wc %08lx\n", wc);
		return;
	}
	WebCon_Free(wc);
}

/**
 **********************************************************************************
 * This is the callback of the TCP when the TCB is closed, so just free WebSocket
 * without sending any messages because it is anyway to late.
 **********************************************************************************
 */
static void
WebSocket_CloseProc(void *eventData)
{
	WebSocket *ws = (WebSocket *) eventData;
	if (ws == NULL) {
		Con_Printf("Bug: Called closeproc without WebSocket\n");
		return;
	}
	if (ws->wops->closeProc) {
		ws->wops->closeProc(ws, NULL);
	}
	if (ws->handoverWc) {
		WebCon_Free(ws->handoverWc);
		ws->handoverWc = NULL;
	}
	WebSocket_Free(ws);
	Con_Printf("WebSocket Close is called\n");
}

/**
 ***********************************************************
 * Shity compiler doesn't allow named initializers.
 ***********************************************************
 */
static struct Tcb_Operations wservTcbOps = {
	.sinkProc = WebServ_DataSink,
	.srcProc = WebServ_DataSrc,
	.closeProc = WebServ_CloseProc
};

static struct Tcb_Operations websockTcbOps = {
	.sinkProc = WebSocket_DataSink,
	.srcProc = WebSocket_DataSrc,
	.closeProc = WebSocket_CloseProc
};

static struct Tcb_Operations fileTcbOps = {
	.sinkProc = WebServ_DataSink,
	.srcProc = File_DataSrc,
	.closeProc = WebServ_CloseProc
};

/*
 **************************************************
 * Setup handlers for incoming connection
 **************************************************
 */

static bool
WebServ_Accept(void *eventData, Tcb * tcb, uint8_t ip[4], uint16_t port)
{
	XY_WebServer *wserv = eventData;
	WebCon *wc = WebCon_Alloc();
	if (wc == 0) {
		Con_Printf("All Connections are in use\n");
		return false;
	}
	wc->http_state = HTS_IDLE;
	wc->postLen = 0;
	wc->page = wc->data;
	wc->wserv = wserv;
	wc->out_rp = 0;
	wc->reqbuf_wp = 0;
	wc->argc = 0;
	wc->contentLength = 0;
	wc->pageOps = NULL;
	wc->tcb = tcb;
	/* Hopefully it is already closed, should raise an alarm here if not */
	if (wc->file_isopen) {
		Con_Printf("WebServ_Accept: File is still open\n");
		wc->file_isopen = false;
	}
	//Con_Printf("Wserv accept, wc %08lx\n",wc);
	Tcb_RegisterOps(tcb, &wservTcbOps, wc);
	return true;
}

/*
 *************************************************************************************
 * Direct accept of a Websocket connection without the switch protocols overhead
 * usefull for labview without websocket support.
 *************************************************************************************
 */
static bool 
WebSocket_Accept(void *eventData,Tcb *tcb,uint8_t ip[4], uint16_t port)
{
	WebSocket *ws;
	ws = WebSocket_Alloc();
	if (!ws) {
		Con_Printf("Out of WebSockets\n");
		return false;
	}
	ws->ws_state = WSST_OPEN;
	ws->tcb = tcb;
	ws->wops = eventData;
	ws->wops->connectProc(ws, ws->wops->eventData);
	return true;
}

/*
 * -----------------------------------------------------------
 * Convert an MD5 sum given as Ascii String with 32 hexdigits
 * into a 16 Byte binary representation.
 * -----------------------------------------------------------
 */
static int
parse_md5hexstring(uint8_t * md5passwd, const char *md5hexstring)
{
	char c;
	int i;
	for (i = 0; i < 32; i++) {
		c = md5hexstring[i];
		if (!c)
			return -1;
		if (c >= '0' && c <= '9') {
			c = c - '0';
		} else if (c >= 'A' && c <= 'F') {
			c = c - 'A' + 10;
		} else if (c >= 'a' && c <= 'f') {
			c = c - 'a' + 10;
		} else {
			return -1;
		}
		if (i & 1) {
			md5passwd[i >> 1] |= c;
		} else {
			md5passwd[i >> 1] = c << 4;
		}
	}
	return 0;
}

/*
 * ------------------------------------------------------------------
 * Create and store an MD5 Authentication for a webpage 
 * ------------------------------------------------------------------
 */
int
XY_WebAddMD5Auth(XY_WebServer * wserv, const char *uri, const char *realm, const char *username,
		 const char *md5string)
{
	WebPageRegistration *reg;
	XYWebAuthInfo *auth;
	XYWebAuthInfo *cursor;
	reg = FindRequestHandler(wserv, uri);
	if (!reg)
		return -1;

	auth = IRam_Calloc(sizeof(XYWebAuthInfo));
	if (!auth)
		return -1;
	if (parse_md5hexstring(auth->passwd_md5, md5string) < 0) {
		Con_Printf("Error in parse_md5hexstring\n");
		return -1;
	}
	auth->username = IRam_Strdup(username);
	auth->realm = IRam_Strdup(realm);
	auth->next = NULL;

	if (!reg->authInfo) {
		reg->authInfo = auth;
		return 0;
	}
	for (cursor = reg->authInfo; cursor->next; cursor = cursor->next) {

	}
	cursor->next = auth;
	return 0;
}

/**
 *****************************************************
 * Create a new webserver.
 *****************************************************
 */
XY_WebServer *
XY_NewWebServer(void)
{
	XY_WebServer *wserv = IRam_Calloc(sizeof(XY_WebServer));
	if (!wserv)
		return NULL;
	Tcp_ServerSocket(80, WebServ_Accept, wserv);
	wserv->connections = 0;
	wserv->rqHandlerHash = StrHash_New();
	Web_PoolsInit();
	XY_WebRegisterPage(wserv, "/sd/", Page_FatFile, NULL);
	XY_WebAddMD5Auth(wserv, "/sd/", REALM, "ernie", "3de0746a7d2762a87add40dac2bc95a0");	/* Passwd is "bert" */
	XY_WebRegisterPage(wserv, "/", redirect_page, NULL);
	XY_WebAddMD5Auth(wserv, "/", REALM, "ernie", "3de0746a7d2762a87add40dac2bc95a0");	/* Passwd is "bert" */
	return wserv;
}

/*
 *************************************************************
 * Register a new Web  together with a handler
 *************************************************************
 */
static WebPageRegistration *
new_registration(XY_WebServer * wserv, const char *path)
{
	WebPageRegistration *reg = IRam_Calloc(sizeof(WebPageRegistration));
	StrHashEntry *entryPtr;
	if (!reg)
		return NULL;
	memset(reg, 0, sizeof(WebPageRegistration));
	entryPtr = StrHash_CreateEntry(wserv->rqHandlerHash, path);
	StrHash_SetValue(entryPtr, reg);
	return reg;
}

int
XY_WebRegisterPage(XY_WebServer * wserv, const char *path, XY_WebReqHandler * handler, void *cd)
{
	WebPageRegistration *reg;
	reg = FindRequestHandler(wserv, path);
	if (!reg) {
		reg = new_registration(wserv, path);
	}
	if (!reg)
		return -1;
	reg->getProc = handler;
	reg->getData = cd;
	return 0;
}

/**
 **************************************************************************************************
 * \fn static uint16_t WebSocket_ComposeMsg(uint8_t opc,uint8_t *dst,uint8_t *src,uint16_t pllen) 
 **************************************************************************************************
 */
static uint16_t
WebSocket_ComposeMsg(uint8_t opcode, uint8_t * dst, uint8_t * src, uint16_t pllen)
{
	uint16_t idx = 0;
	uint16_t i;
	dst[idx++] = opcode | 0x80;
	if (pllen < 126) {
		dst[idx++] = pllen;
	} else {
		dst[idx++] = 126;
		dst[idx++] = pllen >> 8;
		dst[idx++] = pllen & 0xff;
	}
	for (i = 0; i < pllen; i++) {
		dst[idx++] = src[i];
	}
#if 0
	Con_Printf("Msgdump %u: ", idx);
	for (i = 0; i < idx; i++) {
		Con_Printf("%02x ", dst[i]);
	}
	Con_Printf("\n");
#endif
	return idx;
}

/**
 *****************************************************************************
 * Calculate the required outbuffer space.
 *****************************************************************************
 */
INLINE uint16_t
CalcObufSpace(uint16_t pllen)
{
	if (pllen < 126) {
		return pllen + 2;
	} else {
		return pllen + 4;
	}
}

/**
 **********************************************************************************************
 * \fn static void WebSocket_SendMsg(WebSocket *ws,uint8_t opcode,uint8_t *data,uint16_t len) 
 * Maybe we need a mutex here to lock the outbuffer  during message composition.
 **********************************************************************************************
 */
void
WebSocket_SendMsg(WebSocket * ws, uint8_t opcode, void *_data, uint16_t pllen)
{
	uint8_t *buf;
	uint16_t msgsize;
	uint8_t *data = _data;
	buf = WebSocket_AllocObufSpace(ws, CalcObufSpace(pllen));
	if (!buf) {
		Con_Printf("No buffer available\n");
		return;
	}
	msgsize = WebSocket_ComposeMsg(opcode, buf, data, pllen);
	WebSocket_SubmitSpace(ws, msgsize);
	// submit the message now
}

#if 0
/**
 ********************************************************************************
 * \fn static uint16_t WebSocket_UnmaskMessage(uint8_t *data,uint16_t maxlen) 
 ********************************************************************************
 */
static uint16_t
WebSocket_UnmaskMsg(uint8_t * data, uint16_t maxlen)
{
	uint8_t opcode;
	uint16_t pllen;
	uint16_t idx = 0;
	uint8_t mkey[4];
	uint16_t i;
	opcode = data[idx++] & 4;
	pllen = data[idx++] & 0x7f;
	if (pllen == 126) {
		pllen = ((uint16_t) data[idx++]) << 8;
		pllen |= ((uint16_t) data[idx++]);
		if (pllen > 65500) {
			Con_Printf("Message to big\n");
			return 0;
		}
	} else if (pllen == 127) {
		Con_Printf("Monster messages not supported\n");
		return 0;
	}
	for (i = 0; i < 4; i++) {
		mkey[i] = data[idx++];
	}
	for (i = 0; i < pllen; i++) {
		data[idx] = data[idx] ^ mkey[i & 3];
		idx++;
	}
	return pllen;
}
#endif

/**
 ******************************************************************************
 * static void WebSocket_MsgEval(WebSocket *ws,uint8_t opc,uint8_t *data,uint16_t len)
 * Called by the statemachine if an incoming Msg is complete.
 ******************************************************************************
 */
static void
WebSocket_MsgEval(WebSocket * ws, uint8_t opc, uint8_t * data, uint16_t len)
{
	//Con_Printf("Msg Eval OPC %1x, len %u\n",opc,len);
	switch (opc) {
	    case WSOP_PING:
		    // reply with pong;
		    break;

	    case WSOP_PONG:
		    /* do nothing */
		    break;

	    case WSOP_CONTINUATION:
	    case WSOP_TEXT:
	    case WSOP_BINARY:
		    // Forward to Application
		    if (ws->wops && ws->wops->sinkProc) {
			    ws->wops->sinkProc(ws, ws->wopsConData, opc, data, len);
		    }
		    break;

	    case WSOP_CLOSE:
		    /* If we have already sent a message close immediately */
		    if (ws->ws_state == WSST_CLOSING) {
			    Tcp_Close(ws->tcb);
			    ws->ws_state = WSST_CLOSED;
		    } else {
			    ws->ws_state = WSST_CLOSING;
			    //SendMsg_Closed(ws,1000); is missing here
			    ws->ws_state = WSST_CLOSED;
			    /* TCP close is delayed until dataavail is false */
			    Tcp_Close(ws->tcb);
		    }
		    break;

	    default:
		    /* Spec says unknown op closes sock */
		    break;
	}
}

/**
 *******************************************************************************
 * \fn static void WebSocket_DataSink(WebSocket *ws,uint8_t *data,uint16_t len)
 * State machine for incoming data. Decodes the opcode/length/mask fields
 * and does the unmasking. 
 *******************************************************************************
 */

static uint16_t
WebSocket_DataSink(void *eventData, uint32_t fpos, const uint8_t * data, uint16_t len)
{
	uint16_t i;
	WebSocket *ws = eventData;
	uint8_t *dst = ws->inBuf;
	for (i = 0; i < len; i++) {
		//Con_Printf("WS sink %02x, msgstate %d\n",data[i],ws->msg_state);
		switch (ws->msg_state) {
		    case WSMSGS_OPC:
			    ws->inbuf_wp = 0;
			    ws->msg_plrcvd = 0;
			    ws->msg_opc = data[i] & 0xf;
			    ws->msg_state = WSMSGS_PLLB;
			    break;

		    case WSMSGS_PLLB:
			    ws->msg_pllen = data[i] & 0x7f;
			    if (!(data[i] & 0x80)) {
				    Con_Printf("Message is not masked, Should not happen\n");
				    ws->msg_msk[0] = ws->msg_msk[1] = ws->msg_msk[2] = ws->msg_msk[3] = 0;
				    ws->msg_masked = false;
			    } else {
				    ws->msg_masked = true;
			    }
			    if (ws->msg_pllen == 126) {
				    ws->msg_state = WSMSGS_PLL16HI;
			    } else if (ws->msg_pllen == 127) {
				    /* Allow noncanonic if < 65536 */
				    ws->msg_state = WSMSGS_PLL64_0;
			    } else {
				    if (ws->msg_masked) {
					    ws->msg_state = WSMSGS_MSK_0;
				    } else {
					    if (ws->msg_pllen) {
						    ws->msg_state = WSMSGS_PAYLOAD;
					    } else {
						    ws->msg_state = WSMSGS_OPC;
						    WebSocket_MsgEval(ws, ws->msg_opc, NULL, 0);
					    }
				    }
			    }
			    break;

		    case WSMSGS_PLL16HI:
			    ws->msg_pllen = (uint16_t) data[i] << 8;
			    ws->msg_state = WSMSGS_PLL16LO;
			    break;

		    case WSMSGS_PLL16LO:
		    case WSMSGS_PLL64_7:
			    ws->msg_pllen |= data[i];
			    if (ws->msg_masked) {
				    ws->msg_state = WSMSGS_MSK_0;
			    } else {
				    if (ws->msg_pllen) {
					    ws->msg_state = WSMSGS_PAYLOAD;
				    } else {
					    ws->msg_state = WSMSGS_OPC;
					    WebSocket_MsgEval(ws, ws->msg_opc, NULL, 0);
				    }
			    }
			    break;

		    case WSMSGS_PLL64_0:
		    case WSMSGS_PLL64_1:
		    case WSMSGS_PLL64_2:
		    case WSMSGS_PLL64_3:
		    case WSMSGS_PLL64_4:
		    case WSMSGS_PLL64_5:
			    ws->msg_state++;
			    if (data[i]) {
				    Con_Printf("Ignoring monster messages because of RAM size\n");
				    // trigger a close is missing here;     
				    return len;
			    }
			    break;

		    case WSMSGS_PLL64_6:
			    ws->msg_state = WSMSGS_PLL64_7;
			    break;

		    case WSMSGS_MSK_0:
			    ws->msg_msk[0] = data[i];
			    ws->msg_state = WSMSGS_MSK_1;
			    break;

		    case WSMSGS_MSK_1:
			    ws->msg_msk[1] = data[i];
			    ws->msg_state = WSMSGS_MSK_2;
			    break;

		    case WSMSGS_MSK_2:
			    ws->msg_msk[2] = data[i];
			    ws->msg_state = WSMSGS_MSK_3;
			    break;

		    case WSMSGS_MSK_3:
			    ws->msg_msk[3] = data[i];
			    if (ws->msg_pllen) {
				    ws->msg_state = WSMSGS_PAYLOAD;
			    } else {
				    ws->msg_state = WSMSGS_OPC;
				    WebSocket_MsgEval(ws, ws->msg_opc, NULL, 0);
			    }
			    break;

		    case WSMSGS_PAYLOAD:
			    /* Do the unmasking */
			    if (ws->inbuf_wp < WS_INBUF_SIZE) {
				    dst[ws->inbuf_wp] = data[i] ^ ws->msg_msk[ws->msg_plrcvd & 3];
				    ws->inbuf_wp++;
			    } else {
				    Con_Printf("Input buffer overflow\n");
				    return len;
			    }
			    ws->msg_plrcvd++;
			    if (ws->msg_plrcvd == ws->msg_pllen) {
				    ws->msg_state = WSMSGS_OPC;
				    WebSocket_MsgEval(ws, ws->msg_opc, ws->inBuf, ws->msg_pllen);
			    }
			    break;
		    default:
			    break;
		}
	}
	return len;
}

/**
 ***************************************************************************************
 * \fn static int WebSocket_Handshake(int argc,char *argv[],XY_WebRequest *wr,void *cd)
 * Make the websocket handshake.  This is implemented like  a normal webpage 
 * which takes some variables from the http header needed for the handshake.
 ***************************************************************************************
 */
static int
WebSocket_Handshake(int argc, char *argv[], XY_WebRequest * wr, void *eventData)
{
	WebCon *wc = (WebCon *) wr;
	WebSocket *ws;
	char *page = wc->page;
	uint8_t len;
	char *wskey = NULL;
	char *wsversion;
	//XY_WebServer *wserv = wc->wserv;
	static uint8_t base64res[29];
	static struct sha1_ctxt ctxt;
	static uint8_t digest[20];
	Extract_Var(wc->linev, "Sec-WebSocket-Version", &wsversion);
	Extract_Var(wc->linev, "Sec-WebSocket-Key", &wskey);
	if (!wskey || !wsversion) {
		/* Maybe we should check what to do really in this case */
		create_notfoundpage(wc);
		return 0;
	}
	if (strcmp(wsversion, "13") != 0) {
		Con_Printf("Wrong websocket version \"%s\"\n", wsversion);
		page += xy_strcpylen(page,
				     "HTTP/1.1 400 Bad Request\r\n" "Sec-WebSocket-Version: 13\r\n\r\n");
		wc->pagelen = page - wc->page;
		return 0;
	}
	ws = WebSocket_Alloc();
	if (!ws) {
		Con_Printf("Out of WebSockets\n");
		page += xy_strcpylen(page,
				     "HTTP/1.1 503 Service Temporarily Unavailable\r\n"
				     "Server: " XY_WEBSERVERNAME "\r\n" "Connection: close\r\n\r\n");
		wc->pagelen = page - wc->page;
		return 0;
	}
	wc->ws = ws;
	ws->ws_state = WSST_OPEN;
	wc->http_state = HTS_WEBSOCK;
	ws->wops = eventData;
	/* 
	 ********************************************************
	 * The Test string from the standard 
	 *      key = "dGhlIHNhbXBsZSBub25jZQ=="; 
	 *      result must be "s3pPLMBiTxaQ9kYGzzhZRbK+xOo="
	 ********************************************************
	 */
	EV_Yield();
	sha1_init(&ctxt);
	sha1_loop(&ctxt, wskey, strlen(wskey));
	sha1_loop(&ctxt, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11", 36);
	sha1_result(&ctxt, digest);
	len = Base64_Encode(base64res, digest, 20);
	base64res[len] = 0;
	page += xy_strcpylen(page,
			     "HTTP/1.1 101 Switching Protocols\r\n"
			     "Upgrade: websocket\r\n" "Connection: Upgrade\r\n" "Sec-WebSocket-Accept: ");
	page += xy_strcpylen(page, (char *)base64res);
	page += xy_strcpylen(page, "\r\n\r\n");
	wc->pagelen = page - wc->page;
	ws->wops->connectProc(ws, ws->wops->eventData);
	return 0;
}

void
XY_NewLabviewServer(WebSockOps *wops,uint16_t portNr)
{
	Tcp_ServerSocket(portNr, WebSocket_Accept, wops);
}
/**
 ***********************************************************************************************
 * \fn int XY_WebSocketRegister(XY_WebServer *,const char *path,WebSockOps *,void *wopsServData)
 * Register a WebSocket server here 
 ***********************************************************************************************
 */
void
XY_WebSocketRegister(XY_WebServer * wserv, const char *path, WebSockOps * wops, void *wopsServData,int32_t labviewPortNr)
{
	wops->eventData = wopsServData;
	XY_WebRegisterPage(wserv, path, WebSocket_Handshake, wops);
	//XY_WebAddMD5Auth(wserv, path, REALM, "ernie", "3de0746a7d2762a87add40dac2bc95a0");	/* Passwd is "bert" */
	if(labviewPortNr >= 0) {
		XY_NewLabviewServer(wops,labviewPortNr);
	}
}

/**
 **********************************************************************************************
 * \fn int XY_RegisterPostProc(XY_WebServer *,const char *path,XY_PostProc *,void *postData)
 * Register a proc which will be called when a HTTP post is detected. 
 **********************************************************************************************
 */
int
XY_RegisterPostProc(XY_WebServer * wserv, const char *path, XY_PostProc * proc, void *postData)
{
	WebPageRegistration *reg;
	reg = FindRequestHandler(wserv, path);
	if (!reg) {
		reg = new_registration(wserv, path);
	}
	if (!reg)
		return -1;
	reg->postProc = proc;
	reg->postData = postData;
	return 0;
}
