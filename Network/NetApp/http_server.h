/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    HTTP Server header file
    Copyright (c)  2009-2021, eForce Co., Ltd. All rights reserved.

    Version Information
      2009.01.09: Created
      2010.03.08: Support for query in URI
      2012.08.09: Add HttpSendFile(), HttpSendErrorResponse().
      2012.08.14: Expanded TX/RX buffer length 16 -> 32bit
      2013.02.13: Changed variable type to pointer path, ctype
      2014.11.05: Add an API of http_server_stop
      2015.02.25: Supported to HTTP Keep-Alive feature.
      2015.03.26: Delete HttpSendImage, Add HttpSendResponse, Modify HttpSendText
      2015.06.05: Published the following function as API.
        HttpSetContent, HttpSetContentLen, HttpSetContentKpa HttpSendBuffer
      2015.06.22: Supported to Cookie Header Received.
      2015.12.14: The socket ID replaced SID types
                  Add Safety version of the function CgiGetParam.
      2016.02.10: Add include files for warning avoidance
      2016.02.17: Corrected to not disable HttpSetContentKpa().
      2016.03.30: Support receive multipart data.
      2016.07.06: Improved and fixed the following:
                  - Execute static analysis tool to this source.
                  - Improved the operation of http_server_stop ().
                  - Fixed the operation at the time of receiving the HEAD request.
                  - Improved the operation when the request receiving with garbage.
      2017.06.13: Moved Definition HTTP_BUF_SIZ to http_server.c/_cfg.h.
      2019.06.08: Check for Upgrade header (for WebSocket)
      2020.08.28: Change gHTTP_SERVER_UPGCBK to T_HTTP_SERVER::upgcbk;
      2021.08.20: HTTPS support.
 ***************************************************************************/

#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "kernel.h"
#include "net_hdr.h"

#define HTTP_PORT           80
#define HTTP_SERVER_RETRY_WAIT  100U    /* wait retrying(ms)                 */
#define HTTP_SERVER_BOOT        1U      /* http server boot flag             */
#define HTTP_SERVER_STOP        0U      /* http server stop flag             */

#include "http_server_cfg.h"

typedef struct t_http_header {
    VB *method;   /* GET or HEAD or POST  */
    VB *url;      /* URL / File Name      */
    VB *url_q;    /* URL query            */
    VB *ver;      /* 1.0 or 1.1           */
    VB *host;     /* Client Host          */
    VB *ctype;    /* Content-Type         */
    VB *Content;  /* Content              */
    UW ContentLen;/* Content length       */
    UB kpa;
#ifdef ENA_USER_EXT
    VB *auth;     /* Authorization        */
    VB *cookie;   /* Cookie */
#endif
}T_HTTP_HEADER;

typedef struct t_http_server {
    UW sbufsz;
    UW rbufsz;
    UW txlen;
    UW rxlen;
    UW rdlen;
    UW len;     /* temp */
    UB *rbuf;
    UB *sbuf;
    UB *req;
    UH Port;
    SID SocketID;
    ER (*upgcbk)(struct t_http_server*);       /* callback Upgrade receive */
    BOOL upgrade;
    T_HTTP_HEADER hdr;
    UB NetChannel;
    UB ver;     /* Added for IPV6 support */
    UB server_tsk_stat; /* http server status flag                           */
    ID server_tsk_id;   /* http server task id                               */
    struct t_http_server *next;
#ifdef ENA_KEEP_ALIVE
    UH kpa_max;
#endif
    UB flag;
}T_HTTP_SERVER;

#define HTTPD_EXT_AUTH          0x01U
#define HTTPD_EXT_UHDR          0x02U
#define HTTPD_EXT_SMCGI         0x40U   /* Streaming CGI */
#define HTTPD_EXT_MPCGI         0x80U   /* Multipart CGI */

/* T_HTTP_SERVER::flag */
#define HTTPD_FLG_SSL           0x01U

typedef struct t_http_file {
    const char *path;
    const char *ctype;
    const char *file;
    UW len;
    void (*cbk)(T_HTTP_SERVER *http);
#ifdef ENA_USER_EXT
    UB ext;
#endif
}T_HTTP_FILE;

/* API CGI */
/* CgiGetParam is not api recommended */
void CgiGetParam(char *msg, INT clen, char *cgi_var[], char *cgi_val[], INT *cgi_cnt);
void CgiGetParamN(char *msg, INT clen, char *cgi_var[], char *cgi_val[], INT *cgi_cnt);

/* API HTTP */
ER http_server(T_HTTP_SERVER *http);
ER http_server_stop( UW retry );

/* API for callback function */
ER HttpSendText(T_HTTP_SERVER *http, const char *str, UW len);
ER HttpSendResponse(T_HTTP_SERVER *http, const char *str, UW len, const char *type);
ER HttpSendFile(T_HTTP_SERVER *http, const char *str, UW len, const char *name, const char *type);
ER HttpSendErrorResponse(T_HTTP_SERVER *http, const char *str);

ER HttpSetContent(T_HTTP_SERVER *http, const char *str);
ER HttpSetContentLen(T_HTTP_SERVER *http, const char *str, UW len);
ER HttpSetContentKpa(T_HTTP_SERVER *http);
ER HttpSendBuffer(T_HTTP_SERVER *http, const char *str, UW len);

extern T_HTTP_FILE  *gHTTP_FILE;
extern T_HTTP_SERVER *gHTTP_SERVER_STACK;
extern ER (*gHTTP_SERVER_UPGCBK)(T_HTTP_SERVER*);

#ifdef ENA_USER_EXT
extern ER (*gHTTP_EXT_CBK)(T_HTTP_SERVER *http, const T_HTTP_FILE *fp, UB evt);
ER HttpSetContentCookie(T_HTTP_SERVER *http, const char *name, const char *val, const char *opt);
UB CookieGetItem(char **cookie, char **name, char **value);
#endif


#ifdef ENA_DIVRECV_CGI
#include "httpd_mp_cgi.h"
#endif
 

#ifdef __cplusplus
}
#endif
#endif /* HTTP_SERVER_H */
