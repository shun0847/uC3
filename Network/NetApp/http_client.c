/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    HTTP Client Application
    Copyright (c)  2013-2025, eForce Co., Ltd. All rights reserved.

    Version Information
      2013.06.28: Created
      2014.04.21: Changed to use base64calc.c at BASE64 processing
      2014.07.10: Suppressed warning of the GCC compiler.
      2015.05.26: Added a reliability check processing of SSL connection.
      2015.07.14: Corrected 65536 bytes or more of data is to be able to send.
      2015.12.14: The socket ID replaced SID types
      2016.02.22: Fixed a NULL pointer access when processing of DIGEST auth.
      2016.07.06: Execute static analysis tool to this source.
      2016.08.15: Stop a reference to the variable of uninitialized in http_snd_req().
      2016.08.19: Change the memory release process of http_cls processing.
      2016.08.22: Added http custom header functions.
      2017.01.11: Accept the sockets with SSL sessions established.
                  Correction of missing call of cls_ssoc().
      2017.01.18: Improved the validity check on HTTPS connection.
      2017.01.26: Fixed an error problem when header was received by dividing.
                  Added upper limit check processing of host name to http_get_ipaddr().
      2017.03.03: Fixed NULL check order of http_cls().
      2017.03.07: Changed to ignore errors when receiving data at http_rcv_res().
      2017.03.08: Fixed a bug in search processing of custom header name.
      2017.03.30: Changed the return value to E_TMOUT when disconnecting due to timeout.
                    (Target function: http_cmd_get(), http_cmd_post() )
      2017.08.31: Add function http_con( ) for verify the certificate.
      2017.08.31: Add below functions for post large data.
                   - http_snd()
                   - http_rcv_status()
                  Change http_cmd_post() and http_cmd_get() for post large data.
                    function is called with http->flag HTTPC_FLG_SND_BODY_LATER bit,
                    return fucntion before send contents.
      2017.09.01: Clear low layer(TCP or TLS) receive data before send request.
      2018.02.22: Supported the following methods. (PUT, DELETE, HEAD)
                  Supported HTTPS connection via proxy.
      2018.02.27: Fix an exception problem in HTTPS connection via proxy.
      2018.03.15: Supports SNI extension of HTTPS communication.
      2018.03.29: Fix an exception problem in HTTPS. (Degrade on 2018.03.15)
      2018.09.06: Fixed an issue related to the format of the URL string.
                  Fixed problem with header item with no data.
      2018.10.24: Suppressed warning of the static analysis tool.
      2018.10.29: Fixed a problem of overwriting the receive buffer by 1 byte
                  when receiving HTTP response.
      2018.10.30: Add buffer size check for digest authentication.
      2019.06.24: Add function http_abt( ) for forced termination.
      2019.07.12: Improved the argument check of some APIs.
      2019.10.18: Fixed processing related to parsing.
                   - Fixed parsing of URL containing colons.
                   - Fixed parse_url() error in uppercase schema.
                   - Fixed invalid HTTP header of HTTPS connection via proxy.
      2020.03.23: Suppressed warning of the 64bit GCC compiler.
      2020.12.06: Fixed problem that an error occurs in send size when using HTTPS.
      2022.03.02: Fixed for changed HTTP requenst "Host" header.
      2022.04.22: Supported HTTPS connection using uNet3-TLS.
      2022.10.26: The timeout behavior at SSL connection is changed by
	              HTTPC_FLG_SSL_NOT_USE_TMO of T_HTTP_CLIENT.flag.
      2022.10.26: Added option to wait for disconnection in http_abt().
      2024.10.30: Modified http_get_ipaddr() does not return an error when
                  DNS is absent in the local network.
      2025.01.27: Supported Proxy Authentication.
      2025.07.30: Supported IPv6 communication using uNet3-IPv6.
***************************************************************************/

#include "kernel.h"
#include "net_hdr.h"

#include "http_client.h"
#include "dns_client.h"
#include "net_strlib.h"

#ifdef ENA_AUTH_BASIC
#include "base64calc.h"
#endif

#ifdef ENA_AUTH_DIGEST
#include "md5calc.h"
#define strcat_quo(dest,src)     \
    { \
        net_strcat((dest), "\"");   net_strcat((dest), (src));   net_strcat((dest), "\"");  \
    }
#endif

#ifdef ENA_CUSTOM_HEADER
extern const T_HTTP_CUSTOM_HDR http_chdr_tbl[];
#endif

#define HDR_LEN_HTTP        9U      // "HTTP/1.x "

#define SCHEME_LEN_HTTP     7U
#define SCHEME_LEN_HTTPS    8U

#define UH_MAX_VAL      0xFFFFU
#define W_MAX_VAL       0x7FFFFFFF

#define HTTP_RCV_MAX    (UH_MAX_VAL)

#ifndef HTTPSC_SNI_BUFSZ
#define HTTPSC_SNI_BUFSZ    0U
#endif

#ifndef HTTPC_ABTAPI_PRI
#define HTTPC_ABTAPI_PRI    1U
#endif

#ifndef HTTPC_SSLPDU_LEN
#define HTTPC_SSLPDU_LEN    1024U
#endif

#ifndef HTTPS_SSOC_TMO
#define HTTPS_SSOC_TMO      (10*1000)
#endif


static ER http_get_buf(T_HTTP_MSG_BUF **buf, ID mpf_id)
{
    ER ercd;

    *buf = NULL;
    ercd = tget_mpf(mpf_id, (VP*)buf, TMO_POL);
    if (ercd != E_OK) {
        ercd = E_NOMEM;
    }
    else {
        net_memset(*buf, 0, sizeof(T_HTTP_MSG_BUF));
    }
    return ercd;
}

static void http_rel_buf(T_HTTP_MSG_BUF *buf, ID mpf_id)
{
    if (buf != NULL) {
        rel_mpf(mpf_id, buf);
    }
}

#if defined(HTTPC_SSL_SUP) && defined(SSL_SERVER_NODE)  /* using uNet3-TLS */
static ER httpc_set_sni(SID soc_id, ID ssn_id , VP cfg_val)
{
    return ssl_set_server_name(ssn_id, (const char*)cfg_val);
}
#endif

static ER http_con_soc(T_HTTP_CLIENT *http, T_HTTP_URL *url)
{
    ER ercd;
#ifndef HTTPSC_SNI_NOSUP
#ifdef HTTPC_SSL_SUP
    T_SSL_NODE ssl_svr = {0};
#if (0 == HTTPSC_SNI_BUFSZ)
    VB *mbuf;
#else
    VB mbuf[HTTPSC_SNI_BUFSZ];
#endif
    TMO ssl_tmo;
#endif
#endif

    /* alloc method buffer */
    if (NULL == http->msg) {
        ercd = http_get_buf(&http->msg, http->mpf_id);
        if (E_OK != ercd) {
            return ercd;
        }
    }
    
    ercd = E_PAR;
    http->svr.num = (UB)http->dev_num;
    if (http->scm == HTTP_SCHEME_HTTP) {
        ercd = con_soc(http->http_sid, &http->svr, SOC_CLI);
    }
#ifdef HTTPC_SSL_SUP
    else if (http->scm == HTTP_SCHEME_HTTPS) {

        if ((http->flag & HTTPC_FLG_SSL_NOT_USE_TMO) != 0U) {
            ssl_tmo = 0;
        }
        else {
            ssl_tmo = HTTPS_SSOC_TMO;
        }

#ifndef HTTPSC_SNI_NOSUP
#if (0 == HTTPSC_SNI_BUFSZ)
        /* Reuse the message buffer */
        ercd = net_strlen((VB*)http->msg->buf);
        mbuf = (VB*)&http->msg->buf[ercd];
        ercd = sizeof(http->msg->buf) - ercd;
#else
        ercd = sizeof(mbuf);
#endif
        /* Check the free buffer size */
        if (ercd <= (ER)(url->host_len)) {
            ercd = E_NOMEM;
        }
        else {
            ercd = (ER)(url->host_len);
        }

        /* To process in the case of a valid buffer */
        if (0 < ercd) {
            /* Literal IPv4 and IPv6 addresses are not permitted in SNI. */
            if (url->host_type == HTTPC_HT_DOMAIN) {
                net_strncpy(mbuf, url->host, ercd);
                mbuf[ercd] = '\0';
                ssl_svr.name = mbuf;
            }
            net_memcpy(&ssl_svr.host, &http->svr, sizeof(http->svr));
#ifdef SSL_SERVER_NODE  /* using uNet3-TLS */
            ssl_svr.node_type = SSL_CLIENT_NODE;
            ssl_svr.cfg_hnd = httpc_set_sni;
            ssl_svr.cfg_val = (VP)ssl_svr.name;
            ercd = con_ssoc(http->http_sid, &ssl_svr, 0, ssl_tmo);
#else
            ercd = con_ssocv(http->http_sid, &ssl_svr, 0, ssl_tmo);
#endif
            mbuf[0] = '\0';
        }
#else
        ercd = con_ssoc(http->http_sid, &http->svr, 0, ssl_tmo);
#endif
    }
#endif
    return ercd;
}

static ER http_snd_soc(T_HTTP_CLIENT *http, VP data, UW len)
{
    ER ercd;

    ercd = E_PAR;
    if (http->scm == HTTP_SCHEME_HTTP) {
        len = (len > UH_MAX_VAL) ? UH_MAX_VAL : len;
        ercd = snd_soc(http->http_sid, data, (UH)len);
    }
#ifdef HTTPC_SSL_SUP
    else if (http->scm == HTTP_SCHEME_HTTPS) {
        len = (len > HTTPC_SSLPDU_LEN) ? HTTPC_SSLPDU_LEN : len;
        ercd = snd_ssoc(http->http_sid, data, (UH)len);
    }
#endif

    if (0 == ercd) {    /* abnormal return value */
        ercd = E_OBJ;   
    }
    return ercd;
}

static ER http_rcv_soc(T_HTTP_CLIENT *http, VP data, UH len)
{
    ER ercd;

    ercd = E_PAR;
    if (http->scm == HTTP_SCHEME_HTTP) {
        ercd = rcv_soc(http->http_sid, data, len);
    }
#ifdef HTTPC_SSL_SUP
    else if (http->scm == HTTP_SCHEME_HTTPS) {
        ercd = rcv_ssoc(http->http_sid, data, len);
    }
#endif
    return ercd;
}

static ER http_cls_soc(T_HTTP_CLIENT *http)
{
    ER ercd;

    ercd = E_PAR;
    if (http->scm == HTTP_SCHEME_HTTP) {
        ercd = cls_soc(http->http_sid, SOC_TCP_CLS);
    }
#ifdef HTTPC_SSL_SUP
    else if (http->scm == HTTP_SCHEME_HTTPS) {
        ercd = cls_ssoc(http->http_sid);
    }
    /* Clear status */
    http->prx &= ~HTTPC_PRXSTS;     /* proxy status */
#endif
    return ercd;
}

static ER parse_url(const char *str, T_HTTP_URL *url, B *scheme)
{
    ER ercd;
    INT val;
    char *p;

    /* scheme */
    ercd = E_OK;
    if (net_strncasecmp(str, "http://", SCHEME_LEN_HTTP) == 0) {
        *scheme = HTTP_SCHEME_HTTP;
    } else if (net_strncasecmp(str, "https://", SCHEME_LEN_HTTPS) == 0) {
#ifdef HTTPC_SSL_SUP
        *scheme = HTTP_SCHEME_HTTPS;
#else
        ercd = E_NOSPT;
#endif
    } else {
        *scheme = HTTP_SCHEME_NOSUP;
        ercd = E_PAR;
    }

    if ((ercd == E_OK) && (url != NULL)) {
        /* host */
        url->host = str + SCHEME_LEN_HTTP;
        if (*scheme == HTTP_SCHEME_HTTPS) {
            url->host++;
        }

        /* path */
        url->path = net_strchr((char const*)url->host, '/');
        if (url->path == NULL) {
            url->path = net_strchr((char const*)url->host, '\0');
        }

        /* port */
        url->host_type = HTTPC_HT_DOMAIN;
#ifdef IPV6_SUP     /* IPv6 support */        
        p = net_strchr((char const*)url->host, ']');
        if (p != NULL) {
            url->host_type = HTTPC_HT_IPV6;
            if (*url->host == '[') {
                url->port = net_strchr((char const*)p, ':');
            }
            else {
                ercd = E_PAR;   /* no corresponding bracket */
            }
        } else {
            if (*url->host == '[') {
                ercd = E_PAR;   /* no corresponding bracket */
            }
            else {
                url->port = net_strchr((char const*)url->host, ':');
            }
        }
#else /* only IPv4 support */
        p = net_strchr((char const*)url->host, ']');
        if ((p != NULL) || (*url->host == '[')) {
            ercd = E_PAR;
        }
        else {
            url->port = net_strchr((char const*)url->host, ':');
            if (url->path < url->port) {    /* not port number */
                url->port = NULL;
            }
        }
#endif
        
        if (ercd == E_OK) {
            if (url->port == NULL) {
                url->port = (*scheme == HTTP_SCHEME_HTTP) ? HTTP_STR_PORT : HTTPS_STR_PORT;
                url->host_len = (UB)(url->path - url->host);
            }
            else {
                url->host_len = (UB)(url->port - url->host);
                if (*url->port == ':') {
                    ++url->port;                
                    if (('1' > *url->port) || (*url->port > '9')) {
                        ercd = E_PAR;       /* Port number not specified */
                    }
                }
            }
            
            if (url->host_type == HTTPC_HT_IPV6) {
                url->host++;
                url->host_len -= 2U;        /* Skip brackets length */
            }
            else {
                val = ip_aton(url->host);
                url->host_type = (val != 0) ? HTTPC_HT_IPV4 : HTTPC_HT_DOMAIN ;
            }
        }
    }

    return ercd;
}

static ER http_rd_res_cd(T_HTTP_CLIENT *http, char *buf)
{
    if (net_strncasecmp((char const*)buf, "HTTP/1.1 ", HDR_LEN_HTTP) == 0) {
        buf += HDR_LEN_HTTP;
    } else if (net_strncasecmp((char const*)buf, "HTTP/1.0 ", HDR_LEN_HTTP) == 0) {
        buf += HDR_LEN_HTTP;
    } else {
        return E_PAR;
    }
    http->response.code = (UW)net_atoi(buf);
    return (0U < http->response.code) ? E_OK : E_PAR;
}

/*
   Note : this function receive HTTP response using http->msg
          and pointing to the beginning of HTTP header line at "*top"
*/
static ER http_rd_hdr_line(T_HTTP_CLIENT *http, char **top)
{
    ER ercd;
    char *buf;
    char *tmp;

    ercd = E_OK;

    buf = (char*)http->msg->buf;
    *top = NULL;

    while (1) {
        if (http->rd_len < http->rx_len) {
            tmp = net_strchr((buf+http->rd_len), '\n');
            if (tmp != NULL) {
                *top = buf + http->rd_len;
                http->rd_len += (UW)(tmp - *top) + 1U;
                ercd = tmp - *top;

                if ((http->rx_len == http->rd_len) &&
                    (http->rx_len == (HTTP_MSG_BUF_SIZ - 1U))) {
                        http->rx_len = 0;
                        http->rd_len = 0;
                }

                break;
            } else {
                net_memcpy((VP)buf, buf + http->rd_len, http->rx_len - http->rd_len);
                http->rx_len -= http->rd_len;
                http->rd_len = 0U;
            }
        }


        if (http->rx_len >= (HTTP_MSG_BUF_SIZ - 1U)) {
            ercd = E_NOMEM;
            break;
        }

        ercd = http_rcv_soc(http, buf + http->rx_len, (HTTP_MSG_BUF_SIZ - http->rx_len) - 1U);
        if (ercd <= 0) {
            if (ercd != E_TMOUT) {
            ercd = E_CLS;
            }
            break;
        }

        http->rx_len += (UW)ercd;
        buf[http->rx_len] = '\0';
    }

    return ercd;
}

static ER http_rcv_flush(T_HTTP_CLIENT *http)
{
    TMO tmo;
    ER  ercd;
    char *buf;

    buf = (char*)http->msg->buf;

    /* save receive timeout to local variable */
    (void)ref_soc(http->http_sid, SOC_TMO_RCV, (VP)&tmo);

    /* set receive timeout 0 */
    (void)cfg_soc(http->http_sid, SOC_TMO_RCV, (VP)0);

    /* receive */
    do {
        ercd = http_rcv_soc(http, buf, HTTP_MSG_BUF_SIZ);
    } while (ercd > 0);

    /* restore receive timeout */
    (void)cfg_soc(http->http_sid, SOC_TMO_RCV, (VP)(ADDR)tmo);
    
    return (ercd == E_RLWAI) ? ercd : E_OK ;
}

static const VB* get_method_str(T_HTTP_METHOD method)
{
    const VB *ret;

    switch (method) {
    case HTTP_METHOD_POST:
        ret = "POST";
        break;

    case HTTP_METHOD_PUT:
        ret = "PUT";
        break;

    case HTTP_METHOD_DELETE:
        ret = "DELETE";
        break;

    case HTTP_METHOD_HEAD:
        ret = "HEAD";
        break;

    case HTTP_METHOD_CONNECT:
        ret = "CONNECT";
        break;

    default:
        ret = "GET";
        break;
    }

    return ret;
}


#ifdef ENA_AUTH_DIGEST

#define DIFBUF_STR_MEMERR       "ERMEM"         /* not enough internal memory */
#define DIFBUF_STR_PARERR       "ERPAR"         /* error parameter */
#define DIFBUF_USE_SIZE(_dif_)  ((_dif_)->bufpos - (_dif_)->buf + 1)
#define DIFBUF_MAX_SIZE(_dif_)  (sizeof((_dif_)->buf))
#define DIFBUF_IS_VALIDCAP(_dif_,_len_) \
    ((DIFBUF_USE_SIZE(_dif_) + (_len_)) < DIFBUF_MAX_SIZE(_dif_))

/* Extract the parameter info from WWW-Authenticate header. */
static const VB* get_dgs_prm(const VB *head, const VB *param, UW *len)
{
    const VB *ret;
    VB *tmp;

    *len = 0U;
    ret = head;

    while (1) {
        ret = net_strcasestr(ret, param);
        if (0 == ret) {
            break;
        }

        ret += net_strlen(param);
        if (*(ret++) == '=') {
            if (*ret == '"') {
                ++ret;
                tmp = net_strchr(ret, '"');
            }
            else {
                tmp = net_strchr(ret, ',');
            }
            *len = tmp - ret;
            break;
        }
        else {
            ++ret;
        }
    }

    return ret;
}


/* Extract the parameter info from WWW-Authenticate header. And set to T_HTTP_DIGEST_INFO */
static void get_dgs_svr_inf(T_HTTP_DIGEST_INFO *dif, const VB *head)
{
    /* server parameter and set buffer */
    const char *prm_list[] = {DG_NONCE, DG_REALM, DG_ALGO, DG_QOP, DG_OPAQUE, DG_S_STALE, DG_S_DOMAIN};
    char ** buf_list[7];

    const VB *pmval;
    UW len;
    UB ni;

    buf_list[0] = &dif->nonce;
    buf_list[1] = &dif->realm;
    buf_list[2] = &dif->algorithm;
    buf_list[3] = &dif->qop;
    buf_list[4] = &dif->opaque;
    buf_list[5] = &dif->stale;
    buf_list[6] = &dif->domain;

    /* server info */
    pmval = get_dgs_prm(head, DG_NONCE, &len);
    if (!dif->nonce) {
        dif->nc_val = 0U;
    }
    else if (0 != net_memcmp(dif->nonce, pmval, len)) {
        dif->nc_val = 0U;
    }
    else {
        /* do nothing */
    }
    len = dif->nc_val;
    net_memset(dif, 0, sizeof(*dif));
    dif->nc_val = len;

    /* get parameter value (string or NULL) */
    dif->bufpos = dif->buf;
    
    for (ni = 0U; ni < (UB)(sizeof(buf_list)/sizeof(char *)); ++ni) {
        *buf_list[ni] = dif->bufpos;
        pmval = get_dgs_prm(head, prm_list[ni], &len);

        if (0U < len) {
            if (DIFBUF_IS_VALIDCAP(dif, len)) {
                net_memcpy((VP)*buf_list[ni], pmval, len);
                dif->bufpos = (char *)&(*buf_list[ni])[len + 1];
            }
            else {
                *buf_list[ni] = DIFBUF_STR_MEMERR;
            }
        } else {
            *buf_list[ni] = NULL;
        }
    }
}

/* Extract the parameter info from Authentication-Info header. And set to T_HTTP_DIGEST_INFO */
static void get_dgs_svr_inf_ai(T_HTTP_DIGEST_INFO *dif, const VB *head)
{
    const VB *pmval;
    UW len;

    /* server info (ignore other than nextnonce) */
    pmval = get_dgs_prm(head, DG_S_NEXTNONCE, &len);
    if (pmval != NULL) {
        if (0 != net_memcmp(dif->nonce, pmval, len)) {
            dif->nc_val = 0U;
        }

        if (DIFBUF_IS_VALIDCAP(dif, len)) {
            /* replace nonce */
            dif->nonce = dif->bufpos;
            net_memcpy(dif->nonce, pmval, len);
            dif->nonce[len] = '\0';
            dif->bufpos = &dif->nonce[len + 1];
        }
        else {
            dif->nonce = DIFBUF_STR_MEMERR;
        }
    }
}

/* Create the parameters of Authorization header. And set to T_HTTP_DIGEST_INFO */
static void set_dgs_cli_inf(T_HTTP_DIGEST_INFO *dif)
{
    MD5_INFO *md5i;
    HASH *HA;
    HASHHEX *HA1h;
    HASHHEX *HA2h;
    HASHHEX *HEnt;
    HASHHEX *RetH;
    UW tmp;
    const VB *mtd;
    
    /* check valid buffer size, parameter */
    tmp = 0;
    tmp += LEN_CNONCE + LEN_NC + HASHHEXLEN;
    tmp += sizeof(*md5i) + sizeof(*HA) + sizeof(*HA1h) * 4;
    if (!DIFBUF_IS_VALIDCAP(dif, tmp)) {
        dif->cnonce = DIFBUF_STR_MEMERR;
        dif->nc = DIFBUF_STR_MEMERR;
        return;
    }
    if ((!dif->usr) || (!dif->realm) || (!dif->pw) || (!dif->urlpath) || (!dif->nonce)) {
        dif->cnonce = DIFBUF_STR_PARERR;
        dif->nc = DIFBUF_STR_PARERR;
        return;
    }

    /* make "cnonce", "nc" */
    dif->cnonce = dif->bufpos;
    net_memcpy(dif->cnonce, &dif->nonce[++dif->nc_val & 0x07U], LEN_CNONCE);
    dif->cnonce[LEN_CNONCE] = '\0';

    dif->nc = &dif->cnonce[LEN_CNONCE + 1];
    tmp = htonl(dif->nc_val);
    cnv_n_hex((char *)&tmp, dif->nc, sizeof(tmp));
    dif->bufpos    = &dif->nc[LEN_NC + 1];

    /* MD5 info (prepare buffer) */
    dif->response = dif->bufpos;
    dif->bufpos = &dif->response[HASHHEXLEN + 1];
    RetH = (HASHHEX *)  &dif->response[0];
    HA1h = RetH + 1;
    HA2h = HA1h + 1;
    HEnt = HA2h + 1;
    HA   = (HASH *)(HEnt + 1);
    md5i = (MD5_INFO *)(HA + 1);

    /* MD5 calc */
    /* A1 = user ":" realm-value ":" passwd */
    /* A1 = H(user ":" realm-value ":" passwd) ":" unq(nonce-value) ":" unq(cnonce-value) */
    /* Note: unsupport MD5-sess */
    md5i_init(md5i);
    md5i_append(md5i, (const UB *)dif->usr, net_strlen(dif->usr));
    md5i_append(md5i, (const UB *)":", 1U);
    md5i_append(md5i, (const UB *)dif->realm, net_strlen(dif->realm));
    md5i_append(md5i, (const UB *)":", 1U);
    md5i_append(md5i, (const UB *)dif->pw, net_strlen(dif->pw));
    md5i_finish(md5i, *HA);
    if (dif->algorithm != NULL) {
        if (0 == net_strncasecmp(dif->algorithm, DGV_ALGO_MD5_SESS, net_strlen(DGV_ALGO_MD5_SESS))) {
            md5i_init(md5i);
            md5i_append(md5i, (const UB *)*HA, HASHLEN);
            md5i_append(md5i, (const UB *)":", 1U);
            md5i_append(md5i, (const UB *)dif->nonce, net_strlen(dif->nonce));
            md5i_append(md5i, (const UB *)":", 1U);
            md5i_append(md5i, (const UB *)dif->cnonce, net_strlen(dif->cnonce));
            md5i_finish(md5i, *HA);
        }
    }
    cnv_hex(*HA, *HA1h);

    /* A2 = method ":" uri */
    /* A2 = method ":" uri ":" H(entity-body) */   /* Note: unsupport auth-int */
    if (dif->qop != NULL) {
        tmp = (UW)net_strncasecmp(dif->qop, DGV_QOP_AUTH_INT, net_strlen(DGV_QOP_AUTH_INT));
        if (0U == tmp) {
            md5i_init(md5i);
            md5i_append(md5i, (const UB *)dif->entity_body, net_strlen(dif->entity_body));
            md5i_finish(md5i, *HEnt);
        }
    }
    else {
        tmp = (UW)-1;       /* "qop" value is not equal "auth-int". */
    }
    md5i_init(md5i);

    mtd = get_method_str(dif->method);
    md5i_append(md5i, (const UB *)mtd, net_strlen(mtd));
    md5i_append(md5i, (const UB *)":", 1U);
    md5i_append(md5i, (const UB *)dif->urlpath, net_strlen(dif->urlpath));
    if (0U == tmp) {
        md5i_append(md5i, (const UB *)":", 1U);
        md5i_append(md5i, (const UB *)*HEnt, HASHHEXLEN);
    }
    md5i_finish(md5i, *HA);
    cnv_hex(*HA, *HA2h);

    /* Response = A1 ":" unq(nonce-value) ":" nc-value ":" unq(cnonce-value) ":" unq(qop-value) ":" A2 */
    /* Response = A1 ":" unq(nonce-value) ":" A2 */
    md5i_init(md5i);
    md5i_append(md5i, (const UB *)*HA1h, HASHHEXLEN);
    md5i_append(md5i, (const UB *)":", 1U);
    md5i_append(md5i, (const UB *)dif->nonce, net_strlen(dif->nonce));
    md5i_append(md5i, (const UB *)":", 1U);
    if (dif->qop != NULL) {
        md5i_append(md5i, (const UB *)dif->nc, net_strlen(dif->nc));
        md5i_append(md5i, (const UB *)":", 1U);
        md5i_append(md5i, (const UB *)dif->cnonce, net_strlen(dif->cnonce));
        md5i_append(md5i, (const UB *)":", 1U);
        md5i_append(md5i, (const UB *)dif->qop, net_strlen(dif->qop));
        md5i_append(md5i, (const UB *)":", 1U);
    }
    md5i_append(md5i, (const UB *)*HA2h, HASHHEXLEN);
    md5i_finish(md5i, *HA);
    cnv_hex(*HA, *RetH);

    *RetH[HASHHEXLEN] = '\0';
}
#endif

#ifdef HTTPC_SSL_SUP
/* check cetificate hostname */
static UB https_host_check(const char *hostname, const char *cer_dns)
{
    if ((NULL == hostname)  || (NULL == cer_dns)) {
        return 0;
    }
    if (('\0' == *hostname) || ('\0' == *cer_dns)) {
        return 0;
    }

    for (; *hostname; ++hostname, ++cer_dns) {
        if ('*' == *cer_dns) {        /* wildcard */
            for (; *hostname; ++hostname) {
                /* increment until next separator */
                if (('.' == *hostname) || ('/' == *hostname)) {
                    break;
                }
            }
            ++cer_dns;
        }
        else if (0 != net_strncasecmp(hostname, cer_dns, 1)) {
            break;
        }
    }

    return ((('\0' == *hostname) || ('/' == *hostname)) && ('\0' == *cer_dns)) ? 1 : 0 ;
}

/* is validate HTTPS url */
static ER https_url_validate(T_HTTP_CLIENT *http, const char *urlstr)
{
    ER ercd;
    UH cnt;
    VB *tmp;

    tmp =  (VB *)urlstr + sizeof("https://") - 1;

    /* check signature */
    ercd = get_sig_verify(http->ssl_id);
    if (E_OK == ercd) {
        /* check validate dns name */
        cnt = get_dns_cnt(http->ssl_id);
        ercd = EV_NOT_VLD;          /* not found dns name */
        for (cnt = get_dns_cnt(http->ssl_id); cnt; --cnt) {
            ercd = get_dns_name(http->ssl_id, (VP)http->ssl_buf, SSL_TEMP_BUFSIZE - 1, cnt - 1);
            if (ercd >= SSL_TEMP_BUFSIZE) {
                ercd = E_NOMEM;     /* dns name length is too long */
                break;
            }
            else if (0 > ercd) {
                break;              /* unknown error */
            }

            http->ssl_buf[ercd] = '\0';
            if (https_host_check(tmp, http->ssl_buf)) {
                ercd = E_OK;        /* found dns name */
                break;
            }
            ercd = EV_NOT_VLD;      /* not found dns name */
        }
    }

    return ercd;
}
#endif

#ifdef HTTPC_SSL_SUP
static ER https_con_prx(T_HTTP_CLIENT *http, T_HTTP_URL *url)
{
    ER ercd;
    VB *buf = NULL;
    VB pbuf[8];

    /* check none CONNECT method target */
    if (((http->prx & HTTPC_PRX_USE) == 0) || (http->scm != HTTP_SCHEME_HTTPS)) {
        return E_OK;
    }

    if ((http->prx & HTTPC_PRXSTS_CON) != 0) {  /* already connection */
        return E_OK;
    }

    /* Because it connects by TCP, temporarily change the scheme */
    http->scm = HTTP_SCHEME_HTTP;

    do {
        ercd = http_con_soc(http, url);
        if (E_OK != ercd) {
            break;
        }
        /* Get URL port number */
        if (NULL != url->port) {
            ercd = (UH)net_atoi(url->port);      /* Remove URL garbage */
            net_itoa(ercd, pbuf, 10);
        }
        else {
            net_strcpy(pbuf, HTTPS_STR_PORT);
        }

        /* CONNECT domain:port HTTP/1.1 */
        buf = (VB*)http->msg->buf;
        *buf = '\0';
        net_strcat(buf, "CONNECT ");
        if (url->host_type == HTTPC_HT_IPV6) {
            net_strcat(buf, "[");
            net_strncpy(&buf[net_strlen(buf)], url->host, url->host_len);
            net_strcat(buf, "]");
        }
        else {
            net_strncpy(&buf[net_strlen(buf)], url->host, url->host_len);
        }
        net_strcat(buf, ":");
        net_strcat(buf, pbuf);
        net_strcat(buf, " HTTP/1.1\r\n");
        net_strcat(buf, "Host: ");
        if (url->host_type == HTTPC_HT_IPV6) {
            net_strcat(buf, "[");
            net_strncpy(&buf[net_strlen(buf)], url->host, url->host_len);
            net_strcat(buf, "]");
        }
        else {
            net_strncpy(&buf[net_strlen(buf)], url->host, url->host_len);
        }
        net_strcat(buf, ":");
        net_strcat(buf, pbuf);
        net_strcat(buf, "\r\n");
        net_strcat(buf, "Proxy-Connection: keep-alive\r\n");
        net_strcat(buf, "\r\n");

        ercd = http_snd(http, (UB *)buf, net_strlen(buf));
        if (E_OK != ercd) {
            break;
        }

        /* HTTP/1.1 200 Connection established. */
        ercd = http_rcv_status(http);
        if (E_OK != ercd) {
            break;
        }
        if (http->response.code == 200) {
            http->prx |= HTTPC_PRXSTS_CON;  /* Success proxy connection */
        }
        else {
            (void)http_cls_soc(http);
            ercd = E_OBJ;
        }
    } while (0);
    if (NULL != buf) {
        *buf = '\0';
    }

    /* Restore scheme */
    http->scm = HTTP_SCHEME_HTTPS;

    return ercd;
}
#endif

/* HTTP send request */
static ER http_snd_req(T_HTTP_CLIENT *http, const char *urlstr, T_HTTP_POSTDATA *data, T_HTTP_METHOD method)
{
    ER ercd;
    UW len;
    char *buf;
    T_HTTP_URL url;
#ifdef ENA_AUTH_BASIC
    T_HTTP_MSG_BUF *tmp;
    T_BASE64_INFO b64i;
#endif
#ifdef ENA_CUSTOM_HEADER
    T_HTTP_CUSTOM_HDR *custom;
    BOOL custom_host = FALSE;
#endif

    net_memset(http->msg->buf, 0, HTTP_MSG_BUF_SIZ);

    buf = (char*)http->msg->buf;

    ercd = parse_url(urlstr, &url, &http->scm);
    if (ercd != E_OK) {
        return ercd;
    }
    
    ercd = http_rcv_flush(http);
    if (ercd != E_OK) {
        return ercd;
    }

#ifdef HTTPC_SSL_SUP
    /* Use proxy server and TLS connection */
    ercd = https_con_prx(http, &url);
    if (ercd != E_OK) {
        return ercd;
    }
#endif

    /* Request-Line */
    net_strcpy(buf, get_method_str(method));
    net_strcat(buf, " ");

    if (http->prx != 0U) {
        net_strcat(buf, urlstr);
    } else {
        if (*url.path != '\0') {
            net_strcat(buf, url.path);
        } else {
            net_strcat(buf, "/");
        }
    }

    net_strcat(buf, " HTTP/1.1\r\n");

    /* User-Agent */
    if (http->agt != NULL) {
        net_strcat(buf, "User-Agent: ");
        net_strcat(buf, http->agt);
        net_strcat(buf, END_OF_LINE);
    }

    /* Host */
    net_strcat(buf, HEAD_HOST);
    len = net_strlen(buf);
#ifdef ENA_CUSTOM_HEADER
    for ( custom = http->custom; custom != NULL; custom = custom->next ) {
        if ( net_strcasecmp("Host", custom->name) == 0 ) {
            custom_host = TRUE;
            break;
        }
    }

    if ( (custom_host == TRUE) && (custom->request != NULL) ) {
        net_strncat(buf, custom->request, net_strlen(custom->request));
        net_strcat(buf, END_OF_LINE);
    } else {
#endif
    if (url.host_type == HTTPC_HT_IPV6) {
        net_strcat(buf, "[");        
        net_strncat(buf, url.host, url.host_len);
        net_strcat(buf, "]");        
    }
    else {
        net_strncat(buf, url.host, url.host_len);
    }    
    net_strcat(buf, END_OF_LINE);
#ifdef ENA_CUSTOM_HEADER
    }
#endif

    if (data != NULL) {
        if (data->buflen > 0U) {
            /* Content-Length */
            net_strcat(buf, HEAD_CONTENTLEN);
            net_utoa((UINT)data->buflen, buf+net_strlen(buf), 10);
            net_strcat(buf, END_OF_LINE);
            /* Content-Type */
            net_strcat(buf, HEAD_CONTENTTYPE);
            net_strcat(buf, data->contenttype);
            net_strcat(buf, END_OF_LINE);
        }
    }

    /* Authorization */
#ifdef ENA_AUTH_BASIC
    if ((http->authtype == WWW_AUTH_BASIC) || (http->authtype == PROXY_AUTH_BASIC)) {
        if (http->usr && http->pw) {
#ifdef ENA_AUTH_DIGEST
            if (http->digest != NULL) {
                http_rel_buf(http->digest, http->mpf_id);
                http->digest = NULL;
                http->dif = NULL;
            }
#endif
            if (http->authtype == WWW_AUTH_BASIC) {
                net_strcat(buf, HEAD_AUTHORIZ);
            } else {
                net_strcat(buf, HEAD_PROXY_AUTHORIZ);
            }
            net_strcat(buf, AUHT_BASIC);
            /* encode base64 */
            ercd = http_get_buf(&tmp, http->mpf_id);
            if (ercd != E_OK) {
                return ercd;
            }
            net_memset(tmp->buf, 0, HTTP_MSG_BUF_SIZ);
            net_strcat((char*)tmp->buf, http->usr);
            net_strcat((char*)tmp->buf, ":");
            net_strcat((char*)tmp->buf, http->pw);

            INI_B64I(&b64i);
            SET_B64I(&b64i, (VB*)(buf+net_strlen(buf)), (VB*)tmp->buf, (HTTP_MSG_BUF_SIZ-net_strlen(buf)), net_strlen((VB*)tmp->buf));
            ercd = base64enc(&b64i);

            http_rel_buf(tmp, http->mpf_id);
            if (ercd < E_OK) {
                return ercd;
            }
            net_strcat(buf, END_OF_LINE);
        }
    }
#endif

#ifdef ENA_AUTH_DIGEST
    if ((http->authtype == WWW_AUTH_DIGEST) || (http->authtype == PROXY_AUTH_DIGEST)) {
        if (http->usr && http->pw && http->digest) {
            if (http->authtype == WWW_AUTH_DIGEST) {
                net_strcat(buf, HEAD_AUTHORIZ);
            } else {
                net_strcat(buf, HEAD_PROXY_AUTHORIZ);
            }
            net_strcat(buf, AUHT_DIGEST);

            http->dif->usr      = http->usr;
            http->dif->pw       = http->pw;
            http->dif->urlpath  = url.path;
            http->dif->method   = method;
            http->dif->entity_body = (data != NULL) ? data->buf : NULL ; /* Note: unsupport */
            set_dgs_cli_inf(http->dif);

            net_strcat(buf, HEAD_DG_USER);      strcat_quo(buf, http->usr);
            net_strcat(buf, SPLIT_OF_DG);
            net_strcat(buf, HEAD_DG_REALM);     strcat_quo(buf, http->dif->realm);
            net_strcat(buf, SPLIT_OF_DG);
            net_strcat(buf, HEAD_DG_NONCE);     strcat_quo(buf, http->dif->nonce);
            net_strcat(buf, SPLIT_OF_DG);
            net_strcat(buf, HEAD_DG_URI);       strcat_quo(buf, http->dif->urlpath);
            net_strcat(buf, SPLIT_OF_DG);
            if (http->dif->qop != NULL) {   /* quality of protection */
                net_strcat(buf, HEAD_DG_CNONCE);    strcat_quo(buf, http->dif->cnonce);
                net_strcat(buf, SPLIT_OF_DG);
                net_strcat(buf, HEAD_DG_QOP);       strcat_quo(buf, http->dif->qop);
                net_strcat(buf, SPLIT_OF_DG);
                net_strcat(buf, HEAD_DG_NC);        net_strcat(buf, http->dif->nc);
                net_strcat(buf, SPLIT_OF_DG);
                net_strcat(buf, HEAD_DG_ALGO);      net_strcat(buf, http->dif->algorithm);
                net_strcat(buf, SPLIT_OF_DG);
            }
            if (http->dif->opaque != NULL) {
                net_strcat(buf, HEAD_DG_OPAQUE);    strcat_quo(buf, http->dif->opaque);
                net_strcat(buf, SPLIT_OF_DG);
            }
            net_strcat(buf, HEAD_DG_RESPONSE);  strcat_quo(buf, http->dif->response);
            net_strcat(buf, END_OF_LINE);
        }
    }
#endif

#ifdef ENA_CUSTOM_HEADER
#ifdef HTTPC_CHDR_DIVRCV_SUP
    /* Release custom header buffer allocate */
    if (http->chbuf != NULL) {
        http_rel_buf(http->chbuf, http->mpf_id);
        http->chbuf = NULL;
        http->chpos = NULL;
    }
#endif

    /* Add custom header and Off the "APPEAR" bit */
    for ( custom = http->custom; custom != NULL; custom = custom->next ) {
        if ( net_strcasecmp("Host", custom->name) == 0 ) {
            continue;
        }
        if ( (custom->sts & CUSTM_HDR_APPEAR) != 0U ) {
            custom->sts &= ~CUSTM_HDR_APPEAR;
        }
        if ( ((custom->sts & CUSTM_HDR_DIR_REQ) != 0U) && (custom->request != NULL) ) {
            net_strcat(buf, custom->name);
            net_strcat(buf, ": ");
            net_strcat(buf, custom->request);
            net_strcat(buf, END_OF_LINE);
        }
    }
#endif

    /* End of header */
    net_strcat(buf, END_OF_LINE);

    /* connect to server */
    ercd = http_con(http, urlstr);
    if (ercd != E_OK) {
        return ercd;
    }


    /* Transmit HTTP header */
    len = net_strlen(buf);

TX_CONTENTS:
    ercd = http_snd(http, (UB *)buf, len);
    if (ercd != E_OK) {
        return ercd;
    }

    if ((http->flag & HTTPC_FLG_SND_BODY_LATER) != 0U) {
        /*
         * Application program call http_snd_body() for send messagebody
         * and http_rcv_status() for receive response(status and headers).
         */
        return E_OK;
    }

    /* Transmit HTTP Contents */
    if (data != NULL) {
        if (data->buflen > 0U) {
            len = data->buflen;
            buf = data->buf;
            data = NULL;
            goto TX_CONTENTS;
        }
    }

    /* receive status line and response headers */
    ercd = http_rcv_status(http);

    return ercd;
}

/*******************************
          Public API
*******************************/
#define GIPM_IP_VER4    0x01
#define GIPM_IP_VER6    0x02
#define GIPM_IP_BOTH    0x03

ER http_get_ipaddr_ex(const char *urlstr, T_NODE *svr, SID dns_id, UW dns_ip, UW mode)
{
    ER ercd;
    T_HTTP_URL url;
    char hostname[HOST_NAME_LEN];
    B  scm;

    do {
        ercd = E_PAR;
        if (!(urlstr && svr)) {
            break;
        }
        svr->ipa = 0U;
        svr->port = 0U;
        ercd = parse_url(urlstr, &url, &scm);
        if (ercd != E_OK) {
            break;
        }
        if (sizeof(hostname) <= url.host_len) {
            ercd = E_NOMEM;
            break;
        }
        net_memcpy(hostname, url.host, url.host_len);
        hostname[url.host_len] = '\0';
        ercd = E_NOSPT;     /* mode is unknown value */
        
        /* Try IPv4 resolve */
        if ((ercd != E_OK) && (mode & GIPM_IP_VER4)) {
            svr->ver = IP_VER4;
            svr->ipa = ip_aton(hostname);
            if (svr->ipa != 0U) {
                ercd = E_OK;
            }
            else {
                ercd = dns_get_ipaddr(dns_id, dns_ip, hostname, &svr->ipa);
            }
        }        
#ifdef IPV6_SUP
        /* Fail IPv4, Try IPv6 resolve */
        if ((ercd != E_OK) && (mode & GIPM_IP_VER6)) {
            /* IPv6 */
            svr->ver = IP_VER6;
            ip6_aton(hostname, svr->ip6a);
            /* check valid ip6a */
            if (ip6_addr_type(svr->ip6a) != IP6_ADDR_UNSPEC) {
                ercd = E_OK;
            }
            else {
                ercd = dns_get_ip6addr(dns_id, dns_ip, hostname, svr->ip6a);
            }
        }
#endif
        if (ercd != E_OK) {
            break;      /* Fail resolve IPv4 and IPv6.  */
        }        
    
        /* Port */
        if (NULL != url.port) {
            svr->port = (UH)net_atoi(url.port);
        } else if (scm == HTTP_SCHEME_HTTP) {
            svr->port = HTTP_DEF_PORT;
        } else {
            svr->port = HTTPS_DEF_PORT;
        }

        ercd = E_OK;
    } while (0);

    return ercd;
}


ER http_get_ipaddr(const char *urlstr, T_NODE *svr, SID dns_id, UW dns_ip)
{
    return http_get_ipaddr_ex(urlstr, svr, dns_id, dns_ip, GIPM_IP_VER4);
}

ER http_get_ip6addr(const char *urlstr, T_NODE *svr, SID dns_id, UW dns_ip)
{
    return http_get_ipaddr_ex(urlstr, svr, dns_id, dns_ip, GIPM_IP_VER6);
}

ER http_get_ip46addr(const char *urlstr, T_NODE *svr, SID dns_id, UW dns_ip)
{
    return http_get_ipaddr_ex(urlstr, svr, dns_id, dns_ip, GIPM_IP_BOTH);
}


ER http_con(T_HTTP_CLIENT *http, const char *urlstr)
{
    T_HTTP_URL url;
    ER ercd;
    UB sts;
    UB con_flg;
    
    if (!(http && urlstr)) {
        return E_PAR;
    }
    
    http->stat |= HTTPC_STAT_SUBAPI;
    
    do {
        if (http->http_sid == 0U) {
            ercd = E_PAR;
            break;
        }
        
        ercd = parse_url(urlstr, &url, &http->scm);
        if (ercd != E_OK) {
            break;
        }

#ifdef HTTPC_SSL_SUP
        /* Use proxy server and TLS connection */
        ercd = https_con_prx(http, &url);
        if (ercd != E_OK) {
            break;
        }
#endif

        /* already TCP established ? */
        sts = 0U;
        ercd = ref_soc(http->http_sid, SOC_TCP_STATE, (VP)&sts);
        if (ercd != E_OK) {
            break;
        }

        con_flg = 0;
        if (sts == TCP_CLOSED) {
            con_flg |= 0x01U;
        }
#ifdef HTTPC_SSL_SUP
        else if ((http->scm == HTTP_SCHEME_HTTPS) && ((http->prx & HTTPC_PRX_USE) != 0)) {
            con_flg |= ((http->prx & HTTPC_PRXSTS_SSL) == 0) ? 0x02U : 0 ;
        }
#endif

        if (0 != con_flg) {    /* connecting */
            http->svr.num = (UB)http->dev_num;
            
            ercd = http_con_soc(http, &url);
#ifdef HTTPC_SSL_SUP
            if (http->scm == HTTP_SCHEME_HTTPS) {
                if (ercd > 0) {
                    http->ssl_id = (SID)ercd;
                    if (http->flag & HTTPC_FLG_SSL_VFY) {
                        ercd = https_url_validate(http, urlstr);
                    }
                    else {
                        ercd = E_OK;
                    }
                }
                if (ercd != E_OK) {
                    (void)http_cls_soc(http);
                }
                else if ((http->prx & HTTPC_PRX_USE) != 0) {
                    http->prx |= HTTPC_PRXSTS_SSL;
                }
            }
#endif
        }
#if 1   /* Enable with uNet3/SSL version 1.16 or later. */
#ifdef HTTPC_SSL_SUP
        else {      /* already connecting */
            if (http->scm == HTTP_SCHEME_HTTPS) {
                ercd = get_ssid_soc(http->http_sid);
                if (ercd > 0) {
                    http->ssl_id = (SID)ercd;
                    ercd = E_OK;
                }
                else {
                    (void)http_cls_soc(http);
                }
            }
        }
#endif
#endif
    } while (0);
    
    if (http->stat & HTTPC_STAT_ABORT) {
        if (ercd != E_OK) {
            ercd = E_RLWAI;
        }
    }
    
    http->stat &= ~HTTPC_STAT_SUBAPI;
    

    return ercd;
}

static ER http_cmd_core(T_HTTP_CLIENT *http, const char *url, T_HTTP_METHOD method, T_HTTP_POSTDATA *data)
{
    ER ercd;

    if (!(http && url)) {
        return E_PAR;
    }
    
    http->stat = HTTPC_STAT_REQAPI;
    do {
        if (http->msg == NULL) {
            ercd = http_get_buf(&http->msg, http->mpf_id);
            if (ercd != E_OK) {
                break;
            }
            net_memset((char*)http->msg->buf, 0, HTTP_MSG_BUF_SIZ);
        }
        if (http->http_sid == 0U) {
            http_rel_buf(http->msg, http->mpf_id);
            http->msg = NULL;
            ercd = E_PAR;
            break;
        }
    
        ercd = http_snd_req(http, url, data, method);
        if (ercd < 0) {
            http_rel_buf(http->msg, http->mpf_id);
            http->msg = NULL;
            if (http->stat & HTTPC_STAT_ABORT) {
                ercd = E_RLWAI;
            }
        }
    } while (0);
    http->stat &= ~HTTPC_STAT_REQAPI;
    
    return ercd;
}


ER http_cmd_get(T_HTTP_CLIENT *http, const char *url)
{
    return http_cmd_core(http, url, HTTP_METHOD_GET, NULL);
}

ER http_cmd_post(T_HTTP_CLIENT *http, const char *url, T_HTTP_POSTDATA *data)
{
    return http_cmd_core(http, url, HTTP_METHOD_POST, data);
}

ER http_cmd_put(T_HTTP_CLIENT *http, const char *url, T_HTTP_PUTDATA *data)
{
    return http_cmd_core(http, url, HTTP_METHOD_PUT, data);
}

ER http_cmd_delete(T_HTTP_CLIENT *http, const char *url)
{
    return http_cmd_core(http, url, HTTP_METHOD_DELETE, NULL);
}

ER http_cmd_head(T_HTTP_CLIENT *http, const char *url)
{
    return http_cmd_core(http, url, HTTP_METHOD_HEAD, NULL);
}


ER http_snd(T_HTTP_CLIENT *http, UB *buf, UW len)
{
    ER ercd;

    if (!(http && buf)) {
        return E_PAR;
    }
    
    http->stat |= HTTPC_STAT_SUBAPI;
    do {
        if (http->http_sid == 0U) {
            ercd = E_PAR;
            break;
        }
        if (len == 0U) {
            ercd = E_PAR;
            break;
        }
        
        while (len > 0U) {
            ercd = http_snd_soc(http, buf, len);
            if (ercd <= 0) {
                (void)http_cls_soc(http);
                break;
            }
            buf += ercd;
            len -= (UW)ercd;
        }
        if (ercd < 0) {
            if (http->stat & HTTPC_STAT_ABORT) {
                ercd = E_RLWAI;
            }
        }
        else {
            ercd = E_OK;
        }
    } while (0);
    http->stat &= ~HTTPC_STAT_SUBAPI;

    return ercd;
}

ER http_rcv_status(T_HTTP_CLIENT *http)
{
    ER ercd;
    char *buf;
#ifdef ENA_CUSTOM_HEADER
    T_HTTP_CUSTOM_HDR *custom;
    UW len;
#endif
    
    if (http == NULL) {
        return E_PAR;
    }
    
    http->stat |= HTTPC_STAT_SUBAPI;
    do {
        if (http->http_sid == 0U) {
            ercd = E_PAR;
            break;
        }
        
        /* Receive response until delimiter */
        net_memset(&http->response, 0, sizeof(http->response));
        http->rd_len = 0U;
        http->rx_len = 0U;

        while (1) {
            ercd = http_rd_hdr_line(http, &buf);
            if (ercd < E_OK) {
                break;
            }
            /* Delimiter */
            if (ercd < 2) {
                ercd = E_OK;
                break;
            }
            /* Response Line */
            if (http->response.code == 0U) {
                ercd = http_rd_res_cd(http, buf);
                if (ercd < E_OK) {
                    break;
                }
                continue;
            }

            /*
                 each HTTP headers in the received HTTP resonse
            */

            /* Content-Type: */
            else if (net_strncasecmp((char const*)buf, HEAD_CONTENTTYPE, net_strlen(HEAD_CONTENTTYPE)) == 0) {
                ercd -= (ER)net_strlen(HEAD_CONTENTTYPE);
                if (ercd > HTTP_CONTENTTYPE_MAX) {
                    ercd = HTTP_CONTENTTYPE_MAX;
                }
                net_memcpy(http->response.contenttype,  buf+net_strlen(HEAD_CONTENTTYPE), (UW)ercd);
            /* Content-Length: */
            } else if (net_strncasecmp((char const*)buf, HEAD_CONTENTLEN, net_strlen(HEAD_CONTENTLEN)) == 0) {
                http->response.contentlen = (UW)net_atoi(buf+net_strlen(HEAD_CONTENTLEN));
            /* Transfer-Encoding: */
            } else if (net_strncasecmp((char const*)buf, HEAD_ENCONDING, net_strlen(HEAD_ENCONDING)) == 0) {
                http->response.chunked = 1U;
                http->response.contentlen = 0U;
            /* WWW-Authenticate: */
            /* Porxy-Authenticate: */
            } else if ((net_strncasecmp((char const*)buf, HEAD_WWWAUTH, net_strlen(HEAD_WWWAUTH)) == 0) ||
                       (net_strncasecmp((char const*)buf, HEAD_PROXYAUTH, net_strlen(HEAD_PROXYAUTH)) == 0)) {
                buf = net_strchr(buf, ' ');
                buf++;
                if (net_strncasecmp((char const*)buf, AUHT_BASIC, net_strlen(AUHT_BASIC)) == 0) {
                    if (http->response.code == 401) {
                        http->authtype = WWW_AUTH_BASIC;
                    } else if (http->response.code == 407) {
                        http->authtype = PROXY_AUTH_BASIC;
                    } else {
                        http->authtype = AUTH_NONE;
                    }
                } else if (net_strncasecmp((char const*)buf, AUHT_DIGEST, net_strlen(AUHT_DIGEST)) == 0) {
                    if (http->response.code == 401) {
                        http->authtype = WWW_AUTH_DIGEST;
                    } else if (http->response.code == 407) {
                        http->authtype = PROXY_AUTH_DIGEST;
                    } else {
                        http->authtype = AUTH_NONE;
                    }

    #ifdef ENA_AUTH_DIGEST
                    if (NULL == http->digest) {
                        ercd = http_get_buf(&http->digest, http->mpf_id);
                        if (ercd != E_OK) {
                            break;
                        }
                        http->dif = (T_HTTP_DIGEST_INFO *)http->digest->buf;
                    }
                    get_dgs_svr_inf(http->dif, buf);
    #endif
                } else {
                    /* other auth */
                    http->authtype = AUTH_NONE;
                }
    #ifdef ENA_AUTH_DIGEST
            /* Authentication-Info: */
            } else if (net_strncasecmp((char const*)buf, HEAD_AUTHINFO, net_strlen(HEAD_AUTHINFO)) == 0) {
                if (http->digest != NULL) {
                    get_dgs_svr_inf_ai(http->dif, buf);
                }
    #endif
            }
            else {
    #ifdef ENA_CUSTOM_HEADER
    #ifdef HTTPC_CHDR_DIVRCV_SUP
                /* Custom header buffer allocate */
                if (NULL == http->chbuf) {
                    ercd = 0;
                    /* use custom header ? */
                    for ( custom = http->custom; custom != NULL; custom = custom->next ) {
                        if ( (custom->sts & CUSTM_HDR_DIR_RES) != 0U ) {
                            ercd++;
                        }
                    }
                    if (0 < ercd) {
                        ercd = http_get_buf(&http->chbuf, http->mpf_id);
                        http->chpos = (VB*)http->chbuf->buf;
                    }
                    if (ercd != E_OK) {
                        break;
                    }
                }
    #endif
                /* Get custom header */
                for ( custom = http->custom; custom != NULL; custom = custom->next ) {
                    if ( (custom->sts & CUSTM_HDR_DIR_RES) == 0U ) {
                        continue;
                    }
                    len = net_strlen(custom->name);
                    if (net_strncasecmp((char const*)buf, custom->name, len) == 0) {
                        custom->response = net_strstr(buf, ": ");
                        if ( (custom->response != NULL) && (((UW)custom->response - (UW)buf) == len) ) {
                            custom->sts |= CUSTM_HDR_APPEAR;
                            custom->response += 2;

    #ifdef HTTPC_CHDR_DIVRCV_SUP
                            /* Custom header buffer set user pointer */
                            ercd = sizeof(http->chbuf->buf) - (http->chpos - (VB*)http->chbuf->buf);
                            if (0 < ercd) {
                                //len = net_strstr(custom->response, "\r") - custom->response;
                                len = net_strchr(custom->response, '\r') - custom->response;
                                if (len > (UW)ercd) {
                                    custom->sts |= CUSTM_HDR_NOMEM;
                                    len = (UW)ercd;
                                }
                                if (0U < len) {
                                    net_memcpy(http->chpos, custom->response,  len);
                                }
                                http->chpos[len] = '\0';
                                custom->response = http->chpos;
                                http->chpos += len + 1U;     /* step next buffer area */
                            }
                            else {
                                /* Usually it will not enter here (Logic error) */
                                custom->sts |= CUSTM_HDR_NOMEM;
                                custom->response = NULL;
    #endif
                            }
                            break;
                        }
                    }
                }
                ercd = E_OK;
    #endif
            }
        }
        
        if (ercd < E_OK) {
            (void)http_cls_soc(http);
            
            if (http->stat & HTTPC_STAT_ABORT) {
                ercd = E_RLWAI;
            }
        }
    } while (0);
    http->stat &= ~HTTPC_STAT_SUBAPI;

    return ercd;
}

/* HTTP receive response */
UW http_rcv_res_s(T_HTTP_CLIENT *http, UB *buf, UW len, ER *ercd)
{
    UW l;
    UH n;
    
    l = 0U;
    if (!(http && buf)) {
        *ercd = E_PAR;
        return l;
    }    

    http->stat = HTTPC_STAT_REQAPI;
    do {
        if (http->http_sid == 0U) {
            *ercd = E_PAR;
            break;
        }
        
        *ercd = E_OK;
        if (http->rd_len < http->rx_len) {
            l = http->rx_len - http->rd_len;
            if (l > len) {
                l = len;
            }
        }
        if (0U < l) {
            net_memcpy(buf, (UB*)http->msg->buf + http->rd_len, l);
            len -= l;
            http->rd_len += l;
        }
        while (0U < len) {
            n = (UH)((len > HTTP_RCV_MAX) ? HTTP_RCV_MAX : len);
            *ercd = http_rcv_soc(http, buf+l, n);
            if (*ercd <= 0) {
                if (http->stat & HTTPC_STAT_ABORT) {
                    *ercd = E_RLWAI;
                }
                break;
            }
            len -= (UW)*ercd;
            l += (UW)*ercd;
        }
    } while (0);
    http->stat &= ~HTTPC_STAT_REQAPI;
    
    return l;
}

ER http_rcv_res(T_HTTP_CLIENT *http, UB *buf, UW len)
{
    ER ercd;
    UW ret;

    ret = http_rcv_res_s(http, buf, len, &ercd);
    if (ercd == E_RLWAI) {
        ;       /* execute http_abt() */
    }
    else if (0 < ret) {      /* Do not set an error if data is received. */
        if (ret > (UW)W_MAX_VAL) {  /* return value over */
            ercd = E_NOSPT;
        }
        else {
            ercd = (ER)ret;
        }
    }
    else if ((ercd != E_TMOUT) && (ercd != E_PAR)) {
        ercd = E_CLS;
    }

    return ercd;
}


ER http_cls(T_HTTP_CLIENT *http)
{
    ER ercd;
    
    if (http == NULL) {
        return E_PAR;
    }
    
    if ((http->flag & HTTPC_FLG_CLS_NOT_RELBUF) == 0U) {
#ifdef ENA_AUTH_DIGEST
    if (http->digest != NULL) {
        http_rel_buf(http->digest, http->mpf_id);
        http->digest = NULL;
        http->dif = NULL;
    }
#endif

#ifdef ENA_CUSTOM_HEADER
#ifdef HTTPC_CHDR_DIVRCV_SUP
    if (http->chbuf != NULL) {
        http_rel_buf(http->chbuf, http->mpf_id);
        http->chbuf = NULL;
        http->chpos = NULL;
    }
#endif
#endif
    }
    if (http->msg != NULL) {
        http_rel_buf(http->msg, http->mpf_id);
        http->msg = NULL;
    }

    ercd = http_cls_soc(http);
    if (http->stat & HTTPC_STAT_ABORT) {
        if (ercd != E_OK) {
            ercd = E_RLWAI;
        }
    }
    else {
        ercd = E_OK;
    }
    return ercd;
}

ER http_abt(T_HTTP_CLIENT *http)
{
    ER ercd;
    PRI tskpri;
    UB sts;
    
    if (http == NULL) {
        return E_PAR;
    }
    if (http->http_sid == 0U) {
        return E_PAR;
    }
    
    get_pri(TSK_SELF, &tskpri);
    chg_pri(TSK_SELF, HTTPC_ABTAPI_PRI);
    
    ercd = ref_soc(http->http_sid, SOC_TCP_STATE, (VP)&sts);
    if (E_OK == ercd) {
        if (TCP_CLOSED == sts) {
            ercd = (0 == (http->stat & HTTPC_STAT_REQALL)) ? E_OK : E_CLS;
        }
        http->stat |= HTTPC_STAT_ABORT;
        if ((http->flag & HTTPC_FLG_ABT_CLSWAIT) == 0U) {
            abt_soc(http->http_sid, SOC_ABT_ALL);
        }
        http_cls_soc(http);

    }
    
    chg_pri(TSK_SELF, tskpri);
    
    return ercd;
}


#ifdef ENA_CUSTOM_HEADER
ER http_ehdr_ini(T_HTTP_CLIENT *http, ID mpfid)
{
    UW i;
    ER ercd;

    if ( (http == NULL) || (mpfid <= 0) ) {
        return E_PAR;
    }

    /* Set custom header mpfid and initialize node  */
    http->custom = NULL;
    http->cst_mpfid = mpfid;

    /* Insert custom header */
    for ( i = 0UL; http_chdr_tbl[i].name != NULL; i++ ) {
        ercd = http_ehdr_ins( http,
                              http_chdr_tbl[i].name,
                              http_chdr_tbl[i].request,
                              http_chdr_tbl[i].sts );
        if ( ercd != E_OK ) {
            return ercd;
        }
    }

    return E_OK;
}

ER http_ehdr_end(T_HTTP_CLIENT *http)
{
    T_HTTP_CUSTOM_HDR *pNode = NULL;

    if ( http == NULL ) {
        return E_PAR;
    }

    /* All resource release */
    for ( pNode = http->custom; pNode != NULL; pNode = http->custom ) {
        http->custom = pNode->next;
        (void)rel_mpf(http->cst_mpfid, pNode);
    }

    http->cst_mpfid = 0;

    return E_OK;
}

ER http_ehdr_ins(T_HTTP_CLIENT *http, VB *name, VB *req, UB inists)
{
    T_HTTP_CUSTOM_HDR *pElem = NULL;
    ER ercd;

    if ( (http == NULL) || (name == NULL) ) {
        return E_PAR;
    }

    if ( http->cst_mpfid <= 0 ) {
        return E_PAR;
    }

    /* Already checked whether the registered */
    for ( pElem = http->custom; pElem != NULL; pElem = pElem->next ) {
        if ( net_strcasecmp(name, pElem->name) == 0 ) {
            /* Overwrite */
            pElem->request = req;
            pElem->response = NULL;
            pElem->sts = inists;
            return E_OK;
        }
    }

    /* Get element */
    ercd = tget_mpf( http->cst_mpfid, (VP *)&pElem, TMO_POL );
    if ( ercd != E_OK ) {
        return ercd;
    }

    /* Set data */
    pElem->name = name;
    pElem->request = req;
    pElem->response = NULL;
    pElem->sts = inists;

    /* Push element(FILO) */
    if ( http->custom == NULL ) {
        pElem->next = NULL;
        http->custom = pElem;
    }
    else {
        pElem->next = http->custom;
        http->custom = pElem;
    }

    return E_OK;
}

ER http_ehdr_rmv(T_HTTP_CLIENT *http, VB *name)
{
    T_HTTP_CUSTOM_HDR *pPrev = NULL;
    T_HTTP_CUSTOM_HDR *pNode = NULL;

    if ( (http == NULL) || (name == NULL) ) {
        return E_PAR;
    }

    if ( http->cst_mpfid <= 0 ) {
        return E_PAR;
    }

    if ( http->custom == NULL ) {
        return E_NOEXS;
    }

    for ( pNode = http->custom; pNode != NULL; pNode = pNode->next ) {
        if ( net_strcasecmp(name, pNode->name) == 0 ) {
            /* re-chain and release */
            if ( pPrev == NULL ) {
                http->custom = pNode->next;
            } else {
                pPrev->next = pNode->next;
            }
            pNode->next = NULL;
            (void)rel_mpf(http->cst_mpfid, pNode);
            break;
        }
        pPrev = pNode;
    }

    return E_OK;
}

ER http_ehdr_set(T_HTTP_CLIENT *http, VB *name, UB sts)
{
    T_HTTP_CUSTOM_HDR *pNode = NULL;
    ER ercd;

    if ( (http == NULL) || (name == NULL) ) {
        return E_PAR;
    }

    ercd = E_NOEXS;
    for ( pNode = http->custom; pNode != NULL; pNode = pNode->next ) {
        if ( net_strcasecmp(name, pNode->name) == 0 ) {
            pNode->sts = sts;
            ercd = E_OK;
            break;
        }
    }

    return ercd;
}

T_HTTP_CUSTOM_HDR *http_ehdr_get(T_HTTP_CLIENT *http, VB *name, UB filter)
{
    T_HTTP_CUSTOM_HDR *pNode = NULL;

    if ( (http == NULL) || (name == NULL) ) {
        return NULL;
    }

    if ( filter == 0U ) {
        for ( pNode = http->custom; pNode != NULL; pNode = pNode->next ) {
            if ( net_strcasecmp(name, pNode->name) == 0 ) {
                break;
            }
        }
    } else {
        for ( pNode = http->custom; pNode != NULL; pNode = pNode->next ) {
            /* The filter mode is fast speed processing */
            if ( (pNode->sts & filter) == 0U ) {
                continue;
            }
            if ( net_strcasecmp(name, pNode->name) == 0 ) {
                break;
            }
        }
    }

    return pNode;
}
#endif
