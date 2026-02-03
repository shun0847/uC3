/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    HTTP Client Application header file
    Copyright (c)  2013-2025, eForce Co., Ltd. All rights reserved.

    Version Information
      2013.06.28: Created
      2014.04.04: Moved user setting to configure file.
      2015.05.26: Added a reliability check processing of SSL connection.
      2015.12.14: The socket ID replaced SID types
      2016.02.10: Add include files for warning avoidance
      2016.07.06: Execute static analysis tool to this source.
      2016.08.22: Added http custom header functions.
      2017.08.31: Add function http_con( ) for verify the certificate.
      2017.08.31: Add below functions for post large data.
                   - http_snd()
                   - http_rcv_status()
                  Change http_cmd_post() and http_cmd_get() for post large data.
                    function is called with http->flag HTTPC_FLG_SND_BODY_LATER bit,
                    return fucntion before send contents.
      2018.02.22: Supported the following methods. (PUT, DELETE, HEAD)
                  Supported HTTPS connection via proxy.
      2018.09.06: Fixed an issue related to the format of the URL string.
      2019.06.24: Add function http_abt( ) for forced termination.
      2022.10.26: The timeout behavior at SSL connection is changed by
	              HTTPC_FLG_SSL_NOT_USE_TMO of T_HTTP_CLIENT.flag.
      2022.10.26: Added option to wait for disconnection in http_abt().
      2025.01.27: Supported Proxy Authentication.
      2025.07.30: Supported IPv6 communication using uNet3-IPv6.
 ***************************************************************************/

#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H
#ifdef __cplusplus
extern "C" {
#endif

#include "kernel.h"
#include "net_hdr.h"
#include "http_client_cfg.h"

#ifdef HTTPC_SSL_SUP
#include "ssl_hdr.h"
#endif

#define HTTP_DEF_PORT     80U
#define HTTPS_DEF_PORT    443U
#define HTTP_STR_PORT     "80"
#define HTTPS_STR_PORT    "443"

#define HOST_NAME_LEN     255U


#define END_OF_LINE       "\r\n"
#define HTTP_CONTENTTYPE_MAX  64


#define HEAD_CONTENTLEN     "Content-Length: "
#define HEAD_CONTENTTYPE    "Content-Type: "
#define HEAD_USERAGENT      "User-Agent: "
#define HEAD_HOST           "Host: "
#define HEAD_ENCONDING      "Transfer-Encoding: "
#define HEAD_WWWAUTH        "WWW-Authenticate: "    /* 401*/
#define HEAD_PROXYAUTH      "Proxy-Authenticate: "  /* 407*/
#define HEAD_AUTHORIZ       "Authorization: "
#define HEAD_AUTHINFO       "Authentication-Info: "
#define HEAD_PROXY_AUTHORIZ "Proxy-Authorization: "

#define AUTH_NONE           0x00U
#define WWW_AUTH_BASIC      0x01U
#define WWW_AUTH_DIGEST     0x02U
#define PROXY_AUTH_BASIC    0x03U
#define PROXY_AUTH_DIGEST   0x04U


#define AUHT_BASIC          "Basic "
#define AUHT_DIGEST         "Digest "

typedef enum http_method{
    HTTP_METHOD_GET = 0,
    HTTP_METHOD_POST,
    HTTP_METHOD_PUT,
    HTTP_METHOD_DELETE,
    HTTP_METHOD_HEAD,
    HTTP_METHOD_CONNECT,
    HTTP_METHOD_NOSUP
}T_HTTP_METHOD;

typedef enum http_scheme{
    HTTP_SCHEME_HTTP = 0,
    HTTP_SCHEME_HTTPS,
    HTTP_SCHEME_NOSUP
}T_HTTP_SCHEME;

/* Internal Control Data */
/* url */
typedef struct http_url {
    const char *host;
    const char *port;
    const char *path;
    UB host_len;
    UB host_type;
}T_HTTP_URL;

/* T_HTTP_URL host_type values */
#define HTTPC_HT_DOMAIN     0
#define HTTPC_HT_IPV4       1
#define HTTPC_HT_IPV6       2

#define HTTP_MSG_BUF_SIZ    (1024U - 4U)
typedef struct http_msg_buf {
    UW  *next;
    UB  buf[HTTP_MSG_BUF_SIZ];
}T_HTTP_MSG_BUF;

typedef struct http_transaction {
    /* keep alive  */
    UB     keeptmo;
    UH     keepmax;
}T_HTTP_TRANSACTION;


/* Structures exposed to the application */
typedef struct http_response {
    UW code;
    UW contentlen;
    UB contenttype[HTTP_CONTENTTYPE_MAX];
    UB chunked;
}T_HTTP_RESPONSE;

typedef struct http_postdata {
    char       *buf;
    UW         buflen;
    const char *contenttype;
}T_HTTP_POSTDATA, T_HTTP_PUTDATA;


#ifdef ENA_AUTH_DIGEST
    #define DG_REALM            "realm"
    #define DG_NONCE            "nonce"
    #define DG_ALGO             "algorithm"
    #define DG_QOP              "qop"
    #define DG_OPAQUE           "opaque"
    #define DG_S_DOMAIN         "domain"
    #define DG_S_STALE          "stale"
    #define DG_S_NEXTNONCE      "nextnonce"
    #define DG_C_USER           "username"
    #define DG_C_URI            "uri"
    #define DG_C_RESPONSE       "response"
    #define DG_C_NC             "nc"
    #define DG_C_CNONCE         "cnonce"
    #define DG_EQU              "="
    #define SPLIT_OF_DG         ","

    #define HEAD_DG_REALM       (DG_REALM       DG_EQU)
    #define HEAD_DG_NONCE       (DG_NONCE       DG_EQU)
    #define HEAD_DG_ALGO        (DG_ALGO        DG_EQU)
    #define HEAD_DG_QOP         (DG_QOP         DG_EQU)
    #define HEAD_DG_OPAQUE      (DG_OPAQUE      DG_EQU)
    #define HEAD_DG_DOMAIN      (DG_S_DOMAIN    DG_EQU)
    #define HEAD_DG_STALE       (DG_S_STALE     DG_EQU)
    #define HEAD_DG_USER        (DG_C_USER      DG_EQU)
    #define HEAD_DG_URI         (DG_C_URI       DG_EQU)
    #define HEAD_DG_RESPONSE    (DG_C_RESPONSE  DG_EQU)
    #define HEAD_DG_NC          (DG_C_NC        DG_EQU)
    #define HEAD_DG_CNONCE      (DG_C_CNONCE    DG_EQU)

    #define DGV_ALGO_MD5        "MD5"
    #define DGV_ALGO_MD5_SESS   "MD5-sess"
    #define DGV_QOP_AUTH        "auth"
    #define DGV_QOP_AUTH_INT    "auth-int"

    #define LEN_CNONCE          8U
    #define LEN_NC              8U
    #define DGINFO_ITEM_NUM     20U

    typedef struct http_digest_info {
        char        buf[HTTP_MSG_BUF_SIZ - (sizeof(char*) * DGINFO_ITEM_NUM)];
        char        *bufpos;

        /* Server Info */
        char        *realm;
        char        *domain;
        char        *nonce;
        char        *opaque;
        char        *stale;
        char        *algorithm;
        char        *qop;

        /* Client Info */
        const char  *usr;
        const char  *pw;
        const char  *urlpath;
        char        *cnonce;
        char        *nc;
        char        *response;
        T_HTTP_METHOD   method;
        const char *entity_body;

        UW          nc_val;
    } T_HTTP_DIGEST_INFO;
#endif


/* T_HTTP_CLIENT prx bits */
#define HTTPC_PRX_USE               0x01U    /* Use Proxy */
#ifdef HTTPC_SSL_SUP
#define HTTPC_PRXSTS_CON            0x10U    /* Status Proxy Connection */
#define HTTPC_PRXSTS_SSL            0x20U    /* Status Proxy Connection SSL */
#define HTTPC_PRXSTS                (HTTPC_PRXSTS_CON | HTTPC_PRXSTS_SSL)
#endif

/* T_HTTP_CLIENT flag bits */
#define HTTPC_FLG_SND_BODY_LATER    0x10U    /* Send message body later */
#define HTTPC_FLG_SSL_NOT_USE_TMO   0x20U    /* Don't use TMO for SSL connection */
#define HTTPC_FLG_SSL_VFY           0x40U    /* SSL vefity */
#define HTTPC_FLG_CLS_NOT_RELBUF    0x01U    /* Don't release MPF buffer when http_cls(). */
#define HTTPC_FLG_ABT_CLSWAIT       0x02U    /* Wait for disconnection when http_abt() */ 
    
/* T_HTTP_CLIENT stat bits */
#define HTTPC_STAT_REQAPI           0x01U
#define HTTPC_STAT_SUBAPI           0x02U
#define HTTPC_STAT_ABORT            0x08U
#define HTTPC_STAT_REQALL           (HTTPC_STAT_REQAPI | HTTPC_STAT_SUBAPI)

    
#define SSL_TEMP_BUFSIZE    255             /* SSL use Temp buffer size */

#ifndef UNDEF_HTTPC_CHDR_DIVRCV
#define HTTPC_CHDR_DIVRCV_SUP       /* Custom header divided receive support */
#endif

#ifdef ENA_CUSTOM_HEADER
#define CUSTM_HDR_INVALID    0x00U
#define CUSTM_HDR_DIR_REQ    0x01U
#define CUSTM_HDR_DIR_RES    0x02U
#define CUSTM_HDR_APPEAR     0x10U
#define CUSTM_HDR_NOMEM      0x20U  /* customheader bufferover */

typedef struct http_custom_hdr {
    struct http_custom_hdr *next;
    char *name;                             /* Header name */
    char *request;                          /* Request value */
    char *response;                         /* Response value */
    UB   sts;                               /* 0x00=invalid
                                               0x01=request
                                               0x02=response
                                               0x10=appeared */
} T_HTTP_CUSTOM_HDR;
#endif

typedef struct http_client {
    /* HTTP Client Info */
    const char   *agt;
    const char   *usr;
    const char   *pw;
#ifdef ENA_CUSTOM_HEADER
    T_HTTP_CUSTOM_HDR *custom;
    ID           cst_mpfid;
#ifdef HTTPC_CHDR_DIVRCV_SUP
    T_HTTP_MSG_BUF  *chbuf;     /* custom header buffer */
    VB              *chpos;     /* custom header buffer top pointer */
#endif
#endif
    ID           mpf_id;
#ifdef HTTPC_SSL_SUP
    SID          ssl_id;
    char         ssl_buf[SSL_TEMP_BUFSIZE];
#endif
    SID          http_sid;    /* TCP */
    UH           dev_num;
    UH           authtype;
    T_HTTP_RESPONSE response;

    /* Control data */
    UW           rx_len;
    UW           rd_len;
    T_HTTP_MSG_BUF  *msg;

    T_NODE       svr;
    UB           prx;
    B            scm;
    UB           flag;
    UB           stat;

#ifdef ENA_AUTH_DIGEST
    T_HTTP_MSG_BUF      *digest;
    T_HTTP_DIGEST_INFO  *dif;
#endif

}T_HTTP_CLIENT;

/* API */
ER http_con(T_HTTP_CLIENT *http, const char *url);
ER http_cmd_get(T_HTTP_CLIENT *http, const char *url);
ER http_cmd_post(T_HTTP_CLIENT *http, const char *url, T_HTTP_POSTDATA *data);
ER http_cmd_put(T_HTTP_CLIENT *http, const char *url, T_HTTP_PUTDATA *data);
ER http_cmd_delete(T_HTTP_CLIENT *http, const char *url);
ER http_cmd_head(T_HTTP_CLIENT *http, const char *url);
ER http_snd(T_HTTP_CLIENT *http, UB *buf, UW len);
ER http_rcv_status(T_HTTP_CLIENT *http);
UW http_rcv_res_s(T_HTTP_CLIENT *http, UB *buf, UW len, ER *ercd);
ER http_rcv_res(T_HTTP_CLIENT *http, UB *buf, UW len);
ER http_cls(T_HTTP_CLIENT *http);

ER http_abt(T_HTTP_CLIENT *http);

#ifdef ENA_CUSTOM_HEADER
ER http_ehdr_ini(T_HTTP_CLIENT *http, ID mpfid);
ER http_ehdr_end(T_HTTP_CLIENT *http);
ER http_ehdr_ins(T_HTTP_CLIENT *http, VB *name, VB *req, UB inists);
ER http_ehdr_rmv(T_HTTP_CLIENT *http, VB *name);
ER http_ehdr_set(T_HTTP_CLIENT *http, VB *name, UB sts);
T_HTTP_CUSTOM_HDR *http_ehdr_get(T_HTTP_CLIENT *http, VB *name, UB filter);
#endif

ER http_get_ipaddr(const char *urlstr, T_NODE *svr, SID dns_id, UW dns_ip);     /* IPv4 */
ER http_get_ip6addr(const char *urlstr, T_NODE *svr, SID dns_id, UW dns_ip);    /* IPv6 */
ER http_get_ip46addr(const char *urlstr, T_NODE *svr, SID dns_id, UW dns_ip);   /* BothIP */

#ifdef __cplusplus
}
#endif
#endif /* HTTP_CLIENT_H */

