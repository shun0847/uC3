/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    HTTP Server
    Copyright (c)  2009-2025, eForce Co., Ltd. All rights reserved.

    Version Information
      2009.01.09: Created
      2010.03.08: Correction of file not found error code.
      2010.03.08: Support for query in URI
      2010.06.29: Updated for IPv6 support
      2010.10.21: Change timeout in accordance with Compact
      2011.07.01: Remove lock/unlock TCP.
      2012.03.30: Use strncasecmp for upper header case
      2012.06.14: Corresponding reception of truncated header
      2012.06.19: Corrected to break off posted data which is unacceptable
                  and respond 413
      2012.06.28: Replace strncasecmp() to net_strncasecmp()
      2012.06.28: Change response phrase 200 and 404
      2012.07.11: Corrected HttpTcpRecv() parameter the case of receiving
                  partial contents
      2012.08.09: Add HttpSendFile(), HttpSendErrorResponse() Allowed tx/rx
                  buffer setting from parameter
      2012.08.14: Expanded TX/RX buffer length 16 -> 32bit
      2012.10.02  Modify to avoid use of string libraries.
      2013.02.13: Changed variable type to pointer path, ctype
      2013.05.07: Consider NULL terminate area in RX buffer
      2014.04.04: Remove line of "#include <stdlib.h>".
      2014.08.05: Corrected to The value of T_HTTP_SERVER::Port.
      2014.11.05: Add an API of http_server_stop
      2015.02.25: Supported to HTTP Keep-Alive feature.
      2015.03.26: Delete HttpSendImage, Add HttpSendResponse, Modify HttpSendText
      2015.06.05: Add the ability to perform optional processing of the user.
      2015.06.22: Supported to Cookie Header Received.
                  Modify to trim OWS before and after the header-field-value.
      2015.12.14: The socket ID replaced SID types.
                  Add Safety version of the function CgiGetParam.
      2016.02.17: Corrected to not disable HttpSetContentKpa().
      2016.02.29: Modified to use to listen port number of target socket.
      2016.03.18: Update for H/W OS
      2016.03.30: Support receive multipart data. (multipart/form-data)
      2016.07.06: Execute static analysis tool to this source.
      2017.06.13: Moved Definition HTTP_BUF_SIZ to http_server.c/_cfg.h.
      2017.07.27: Support 64bit processor
      2017.10.17: Changed the expression of the value of HDR_LEN_xxx.
      2018.08.14: Corrected a loop problem when request packet is large.
      2018.10.02: Added "Connection: close" header when KeepAlive is disable.
      2019.06.08: Check for Upgrade header (for WebSocket)
      2019.06.08: Change the scope of some functions.
      2019.07.10: Add function to accept PUT request.
      2020.07.09: Changed callback function when receiving Upgrade header.
      2020.08.28: Change gHTTP_SERVER_UPGCBK to T_HTTP_SERVER::upgcbk;
      2021.08.20: HTTPS support.
      2023.07.21: Removed the static attribute of HttpTcpSend().
      2025.04.16: Fixed an unauthorized access issue when waiting for a request.
	  2025.08.04: Added con_ssoc() timeout definition HTTPS_SSOC_TMO.
 ***************************************************************************/

#include "kernel.h"
#include "net_hdr.h"
#include "net_strlib.h"

#include "http_server.h"
#ifdef HTTPD_SSL_SUP
#include "ssl_hdr.h"
#endif

#ifndef HTTPS_SSOC_TMO
#define HTTPS_SSOC_TMO  (TMO_FEVR)
#endif

#define CSTRLEN(_s_)  (sizeof((_s_)) - 1U)
#define LEN_CRLF        CSTRLEN("\r\n")
#define HDR_LEN_HOST    CSTRLEN("Host:")
#define HDR_LEN_CONN    CSTRLEN("Connection:")
#define HDR_LEN_KPA     CSTRLEN("Keep-Alive")
#define HDR_LEN_CLOSE   CSTRLEN("close")
#define HDR_LEN_CTYPE   CSTRLEN("Content-Type:")
#define HDR_LEN_CLEN    CSTRLEN("Content-Length:")
#define HDR_LEN_AUTH    CSTRLEN("Authorization:")
#define HDR_LEN_COOKIE  CSTRLEN("Cookie:")
#define HDR_LEN_GET     CSTRLEN("GET ")
#define HDR_LEN_HEAD    CSTRLEN("HEAD ")
#define HDR_LEN_POST    CSTRLEN("POST ")
#define HDR_LEN_PUT     CSTRLEN("PUT ")
#define HDR_LEN_HTTP    CSTRLEN("HTTP/1.x")
#define TAG_LEN_H       CSTRLEN("<h1></h1>")

#define UH_MAX_VAL      0xFFFFU
#define W_MAX_VAL       0x7FFFFFFF
#define UINT_MAX_DIG    11      // 4294967295
#define INT_MAX_DIG     12      // -2147483648
#define HTTP_SND_MAX    1460U
#define HTTP_RCV_MAX    (UH_MAX_VAL)
#define BASE_DEC        10      // net_itoa() parameter

#ifndef HTTP_BUF_SIZ
#define HTTP_BUF_SIZ    1500U   /* default buffer size (same network buffer size) */
#endif


T_HTTP_FILE *gHTTP_FILE = NULL;
T_HTTP_SERVER *gHTTP_SERVER_STACK = NULL;

#ifdef ENA_USER_EXT
ER (*gHTTP_EXT_CBK)(T_HTTP_SERVER *http, const T_HTTP_FILE *fp, UB evt) = NULL;
#endif

/* Safety version of the function CgiGetParam */
void CgiGetParamN(char *msg, INT clen, char *cgi_var[], char *cgi_val[], INT *cgi_cnt)
{
    char *c;
    char *pr;
    INT  i;
    INT  cnt;

    /* parameter check */
    i = 0;
    if (!cgi_cnt) {
        i = 1;
    }
    else if ((!msg) || (!cgi_var) || (!cgi_val) || (clen == 0) || (*cgi_cnt <= 0)) {
        i = 1;
        *cgi_cnt = 0;
    }
    else {
        /* do nothing */
    }
    if (i > 0) {
        return;
    }

    /* Replace + into space */
    /* Split name and values name1=value1&name2=value2 */
    cnt = 0;
    pr = msg;
    c = msg;
    for (i = 0; i < clen; i++) {
        if (*c == '+') {
            *c = ' ';
        } else if (*c == '&') {
            if ( *cgi_cnt <= (cnt + 1) ) {
                /* Set elem limit */
                break;
            }
            /* variable */
            *c = '\0';
            cgi_var[cnt++] = pr;
            pr = c + 1;
        }
        else {
            /* do nothing */
        }
        c++;
    }

    if (0 < cnt) {
        cgi_var[cnt++] = pr;
        *c = '\0';
    }
    else {
        cnt = 1;
        cgi_var[0] = pr;
        *c = '\0';
    }

    *cgi_cnt = cnt;
    cnt--;

    /* Seperate name1=value1 -> cgi_var[] = name1, cgi_val[] = value1 */
    for ( ; cnt >= 0; cnt--) {
        cgi_val[cnt] = NULL;
        c = cgi_var[cnt];
        while (*c != '\0') {
            if (*c == '=') {
                *c = '\0';
                cgi_val[cnt] = c+1;
                break;
            }
            c++;
        }
    }
    return;
}

void CgiGetParam(char *msg, INT clen, char *cgi_var[], char *cgi_val[], INT *cgi_cnt)
{
    /* Maximum value set by the unconditional */
    *cgi_cnt = W_MAX_VAL;
    CgiGetParamN( msg, clen, cgi_var, cgi_val, cgi_cnt );
}

#ifdef ENA_USER_EXT
UB CookieGetItem(char **cookie, char **name, char **value)
{
    char *tmp;

    if ((!cookie) || (!name) || (!value)) {
        return 0U;
    }

    /* cueing variable */
    tmp = *cookie;
    for (; (*tmp == ' '); ++tmp) {
        ;
    }
    if (*tmp == '\0') {
        return 0U;
    }

    /* Cookie: name1=value1; name2=value2; ... */
    *name = tmp;
    for (; ((*tmp != '\0') && (*tmp != '=')); ++tmp) {
        ;
    }
    if (*tmp == '=') {
        *tmp = '\0';
        ++tmp;      /* element value */
    }

    *value = tmp;
    for (; ((*tmp != '\0') && (*tmp != ';')); ++tmp) {
        ;
    }
    if (*tmp == ';') {
        *tmp = '\0';
        ++tmp;      /* next element */
    }

    /* return next element */
    *cookie = tmp;

    return 1U;
}
#endif

static void HttpCgiHandler(T_HTTP_SERVER *http, const T_HTTP_FILE *fp)
{
    fp->cbk(http);
}

static T_HTTP_FILE *GetHttpFile(char *url)
{
    T_HTTP_FILE *fp = gHTTP_FILE;

    if (fp != NULL) {
        while (1) {
            if ((fp->path == NULL) || (net_strcmp(fp->path, "") == 0)) {
                fp = NULL;
                break;
            }
            if (net_strcmp(fp->path, url) == 0) {
                break;
            }
            fp++;
        }
    }
    return fp;
}


UW HttpTcpSend(T_HTTP_SERVER* http, const UB *data, UW len)
{
    ER (*snd)(SID, VP, UH);
    ER ercd;
    UW i;
    UH slen;

    snd = snd_soc;
#ifdef HTTPD_SSL_SUP
    if (0 != (http->flag & HTTPD_FLG_SSL)) {
        snd = snd_ssoc;
    }
#endif

    i = 0U;
    while (len > 0U) {
        slen = (UH)((len > HTTP_SND_MAX) ? HTTP_SND_MAX : len);
        ercd = snd(http->SocketID, (VP)(data + i), slen);
        if (ercd <= 0) {
            break;
        }
        i += (UW)ercd;
        len -= (UW)ercd;
    }

    return i;
}

static UW HttpTcpRecv(T_HTTP_SERVER* http, UB *data, UW len, ER *ercd)
{
    ER (*rcv)(SID, VP, UH);
    UW i;
    UH l;

    i = 0U;

    rcv = rcv_soc;
#ifdef HTTPD_SSL_SUP
    if (0 != (http->flag & HTTPD_FLG_SSL)) {
        rcv = rcv_ssoc;
    }
#endif

    while (len > 0U)
    {
        l = (UH)((len > HTTP_RCV_MAX) ? HTTP_RCV_MAX : len);
        *ercd = rcv(http->SocketID, data + i, l);
        if (*ercd <= 0) {
            break;
        }
        i += (UW)*ercd;
        len -= (UW)*ercd;
    }

    return i;
}

static void HttpTcpRecvClr(T_HTTP_SERVER* http)
{
    ER (*rcv)(SID, VP, UH);
    VB buf[64];
    TMO set_tmo;

    rcv = rcv_soc;
#ifdef HTTPD_SSL_SUP
    if (0 != (http->flag & HTTPD_FLG_SSL)) {
        rcv = rcv_ssoc;
    }
#endif

    (void)ref_soc(http->SocketID, SOC_TMO_RCV, (VP)&set_tmo);
    (void)cfg_soc(http->SocketID, SOC_TMO_RCV, (VP)0);
    while (0 < rcv(http->SocketID, buf, sizeof(buf))) {
        ;   /* clear receive buffer */
    }
    (void)cfg_soc(http->SocketID, SOC_TMO_RCV, (VP)(ADDR)set_tmo);
}


ER HttpSetContent(T_HTTP_SERVER *http, const char *str)
{
    UW len;

    if (http == NULL) {
        return E_PAR;
    }

    if (str == NULL) {
        http->txlen = 0U;
        return 0;
    }

    len = net_strlen(str);

    len = (len > (http->sbufsz - http->txlen)) ? (http->sbufsz - http->txlen) : len;
    if (0U < len) {
        net_memcpy((http->sbuf + http->txlen), str, len);
        http->txlen += len;
    }

    return (ER)len;
}

ER HttpSetContentKpa(T_HTTP_SERVER *http)
{
#ifdef ENA_KEEP_ALIVE
    ER ercd;
    TMO to_rcv;
    VB tmp[INT_MAX_DIG];

    if (http == NULL) {
        ercd = E_PAR;
    }
    else {
        ercd = ref_soc(http->SocketID, SOC_TMO_RCV, (VP)&to_rcv);
    }
    if (ercd != E_OK) {
        return ercd;
    }

    ercd = 0;
    if (0U < http->hdr.kpa) {
        if (0U < http->kpa_max) {
            to_rcv /= 1000;
            ercd += HttpSetContent(http, "Connection: Keep-Alive\r\n");
            ercd += HttpSetContent(http, "Keep-Alive: timeout=");
            ercd += HttpSetContent(http, net_itoa((INT)to_rcv, tmp, BASE_DEC));
            ercd += HttpSetContent(http, ", max=");
            ercd += HttpSetContent(http, net_itoa((INT)http->kpa_max, tmp, BASE_DEC));
            ercd += HttpSetContent(http, "\r\n");
        }
        else {      /* Last persistent connection */
            ercd += HttpSetContent(http, "Connection: close\r\n");
        }
    }

    return ercd;
#else
    return HttpSetContent(http, "Connection: close\r\n");
#endif
}

ER HttpSetContentCookie(T_HTTP_SERVER *http, const char *name, const char *val, const char *opt)
{
    ER ercd;

    ercd = E_OK;
    if ((http == NULL) || (name == NULL) || (val == NULL)) {
        ercd = E_PAR;
    }
    if (ercd != E_OK) {
        return ercd;
    }

    ercd = 0;
    ercd += HttpSetContent(http, "Set-Cookie: ");
    ercd += HttpSetContent(http, name);
    ercd += HttpSetContent(http, "=");
    ercd += HttpSetContent(http, val);
    if (opt != NULL) {
        ercd += HttpSetContent(http, "; ");
        ercd += HttpSetContent(http, opt);
    }
    ercd += HttpSetContent(http, "\r\n");

    return ercd;
}


ER HttpSetContentLen(T_HTTP_SERVER *http, const char *str, UW len)
{
    ER ercd;
    char s[UINT_MAX_DIG];

    ercd = E_OK;
    if ((http == NULL) || (str == NULL)) {
        ercd = E_PAR;
    }
    if (ercd != E_OK) {
        return ercd;
    }

    ercd = 0;
    ercd += HttpSetContent(http, str);
    ercd += HttpSetContent(http, net_utoa((UINT)len, s, BASE_DEC));
    ercd += HttpSetContent(http, "\r\n");

    return ercd;
}

ER HttpSendBuffer(T_HTTP_SERVER *http, const char *str, UW len)
{
    UW clen;

    if (http == NULL) {
        return E_PAR;
    }
    if ((str != NULL) && (len != 0U) && (http->txlen != 0U)) {
        clen = http->sbufsz - http->txlen;
        clen = (len > clen) ? clen : len;
        net_memcpy(http->sbuf + http->txlen, str, clen);
        str += clen;
        len -= clen;
        http->txlen += clen;
    }

    clen = 0U;

    if (http->txlen != 0U) {
        clen += HttpTcpSend(http, http->sbuf, http->txlen);
    }
    if (len != 0U) {
        clen += HttpTcpSend(http, (const UB*)str, len);
    }

    return (ER)clen;
}

ER HttpSendResponse(T_HTTP_SERVER *http, const char *str, UW len, const char *type)
{
    ER ercd;

    ercd = HttpSetContent(http, 0);
    if (0 == ercd) {
        (void)HttpSetContent(http, "HTTP/1.1 200 OK\r\n");
        (void)HttpSetContent(http, "Content-Type: ");
        if (type == NULL) {
            (void)HttpSetContent(http, "application/octet-stream");
        }
        else {
            (void)HttpSetContent(http, type);
        }
        (void)HttpSetContent(http, "\r\n");
        (void)HttpSetContentLen(http, "Content-Length: ", len);
        (void)HttpSetContentKpa(http);
        (void)HttpSetContent(http, "\r\n");
        if (*http->hdr.method == 'H') {     /* HEAD method */
            (void)HttpSendBuffer(http, NULL, 0U);
        }
        else {
            (void)HttpSendBuffer(http, str, len);
        }
        ercd = E_OK;
    }

    return ercd;
}

ER HttpSendText(T_HTTP_SERVER *http, const char *str, UW len)
{
    return HttpSendResponse(http, str, len, "text/html");
}


ER HttpSendErrorResponse(T_HTTP_SERVER *http, const char *str)
{
    ER ercd;

    if (str == NULL) {
        ercd = E_PAR;
    }
    else {
        ercd = HttpSetContent(http, 0);
    }
    if (0 == ercd) {
        (void)HttpSetContent(http, "HTTP/1.1 ");
        (void)HttpSetContent(http, str);
        (void)HttpSetContentLen(http, "Content-Length: ", net_strlen(str) + TAG_LEN_H);
        (void)HttpSetContentKpa(http);
        (void)HttpSetContent(http, "\r\n");
        if (*http->hdr.method == 'H') {     /* HEAD method */
            (void)HttpSendBuffer(http, NULL, 0U);
        }
        else {
            (void)HttpSetContent(http, "<h1>");
            (void)HttpSetContent(http, str);
            (void)HttpSetContent(http, "</h1>");
            (void)HttpSendBuffer(http, str, 0U);
        }
    }

    return ercd;
}

ER HttpSendFile(T_HTTP_SERVER *http, const char *str, UW len, const char *name, const char *type)
{
    ER ercd;

    if ((name == NULL) || (type == NULL)) {
        ercd = E_PAR;
    }
    else {
        ercd = HttpSetContent(http, 0);
    }
    if (0 == ercd) {
        (void)HttpSetContent(http, "HTTP/1.1 200 OK\r\n");
        (void)HttpSetContent(http, "Content-Disposition: attachment; filename=");
        (void)HttpSetContent(http, name);
        (void)HttpSetContent(http, "\r\n");
        (void)HttpSetContent(http, "Content-Type: ");
        (void)HttpSetContent(http, type);
        (void)HttpSetContent(http, "\r\n");
        (void)HttpSetContentLen(http, "Content-Length: ", len);
        (void)HttpSetContentKpa(http);
        (void)HttpSetContent(http, "\r\n");
        if (*http->hdr.method == 'H') {     /* HEAD method */
            (void)HttpSendBuffer(http, NULL, 0U);
        }
        else {
            (void)HttpSendBuffer(http, str, len);
        }
        ercd = E_OK;
    }

    return ercd;
}


char* trim_ows(char *str)
{
    char *s = str;

    if (0 != s) {
        /* Skip increment OWS */
        for (; (*s == ' ') || (*s == '\t'); s++) {
            ;
        }
        str = s;

        /* Terminate */
        for (; *s != '\0'; s++) {
            ;
        }

        /* Skip decrement OWS */
        for (--s; (*s == ' ') || (*s == '\t'); s--) {
            ;
        }
        ++s;
        *s = '\0';
    }

    return str;
}

/*
    " HTTP:\r\n\r\n"        return value    parameter
    Reached string length    0              NULL
    Reached end of line      1              NULL
    Reached token            1              start offset of token

Note:
    When token is \n then    1              start offset of token
*/

UB parse_tok(T_HTTP_SERVER *sess, char tok, char **par)
{
    char *c;

    *par = NULL;

    if (sess->len == 0U) {
        return 0U;
    }

    /* Skip leading space */
    if (*sess->req == ' ') {
        while (sess->len-- > 0U) {
            if (*++sess->req != ' ') {
                break;
            }
        }
    }

    /* Search for token, break if \r\n */
    c = (char *)sess->req;

    while (sess->len-- > 0U) {
        if (*sess->req == (UB)tok) {
            sess->req++;
            *par = c;
            return 1U;
        }

        if (*sess->req == '\n') {
            sess->req++;
            return 1U;
        }

        sess->req++;
    }

    return 0U;
}


static void HttpReadOptHeader(T_HTTP_SERVER *sess)
{
    UB c;
    char *hdr = NULL;
    T_HTTP_HEADER *hr = &sess->hdr;

    c = 1U;
    while (1) {

        if ((c == 1U)&& (hdr == NULL)) {
            if (net_strncasecmp((char const *)sess->req, "\r\n", LEN_CRLF) == 0) {
                sess->req += LEN_CRLF;
                return; /* Blank line */
            }
        }

        c = parse_tok(sess, ':', &hdr);
        if (hdr == NULL) {
            if (!c) {   /* End of the string */
                return;
            }
            continue;   /* End of Line */
        }

        if (net_strncasecmp(hdr, "Host:", HDR_LEN_HOST) == 0) {
            c = parse_tok(sess, '\n', &hdr);
            if (!c) {
                return;
            }
            if (hdr != NULL) {
                *(sess->req - LEN_CRLF) = '\0';    /* \r\n <- \r = 0 */
                hr->host = trim_ows(hdr);
                hdr = NULL;
            }
        }

        else if (net_strncasecmp(hdr, "Connection:", HDR_LEN_CONN) == 0) {
            c = parse_tok(sess, '\n', &hdr);
            if (!c) {
                return;
            }
            if (hdr != NULL) {
                if (net_strncasecmp(hdr, "Keep-Alive", HDR_LEN_KPA) == 0) {
                    hr->kpa = 1U;
                }
                else if (net_strncasecmp(hdr, "close", HDR_LEN_CLOSE) == 0) {
                    hr->kpa = 0U;
                }
                else {
                    /* do nothing */
                }
                hdr = NULL;
            }
        }

        else if (net_strncasecmp(hdr, "Upgrade:", 8) == 0) {
            c = parse_tok(sess, '\n', &hdr);
            if (!c) {
                return;
            }
            if (hdr != NULL) {
                sess->upgrade = TRUE;
                hdr = NULL;
            }
        }

        else if (net_strncasecmp(hdr, "Content-Type:", HDR_LEN_CTYPE) == 0) {
            c = parse_tok(sess, '\n', &hdr);
            if (!c) {
                return;
            }
            if (hdr != NULL) {
                *(sess->req - LEN_CRLF) = '\0';    /* \r\n <- \r = 0 */
                hr->ctype = trim_ows(hdr);
                hdr = NULL;
            }
        }

        else if (net_strncasecmp(hdr, "Content-Length:", HDR_LEN_CLEN) == 0) {
            c = parse_tok(sess, '\n', &hdr);
            if (!c) {
                return;
            }
            if (hdr != NULL) {
                *(sess->req - LEN_CRLF) = 0U;    /* \r\n <- \r = 0 */
                hr->ContentLen = (UW)net_atoi(trim_ows(hdr));
                hdr = NULL;
            }
        }

#ifdef ENA_USER_EXT
        else if (net_strncasecmp(hdr, "Authorization:", HDR_LEN_AUTH) == 0) {
            c = parse_tok(sess, '\n', &hdr);
            if (!c) {
                return;
            }
            if (hdr != NULL) {
                *(sess->req - LEN_CRLF) = 0U;    /* \r\n <- \r = 0 */
                hr->auth = trim_ows(hdr);
                hdr = NULL;
            }
        }
        else if (net_strncasecmp(hdr, "Cookie:", HDR_LEN_COOKIE) == 0) {
            c = parse_tok(sess, '\n', &hdr);
            if (!c) {
                return;
            }
            if (hdr != NULL) {
                *(sess->req - LEN_CRLF) = 0U;    /* \r\n <- \r = 0 */
                hr->cookie = trim_ows(hdr);
                hdr = NULL;
            }
        }
#endif
        else {
            /* do nothing */
        }
    }
}



static ER HttpReadHeader(T_HTTP_SERVER *http)
{
    T_HTTP_HEADER *hdr = &http->hdr;
    ER ercd;
    UB c;

    do {
        ercd = E_OBJ;
        if (http->rdlen < HDR_LEN_GET) {
            break;
        }

        http->req = http->rbuf;
        http->len = http->rdlen;

        /* 1. Method (upper case) */
        hdr->method = (char *)http->req;
        if (net_strncasecmp((const char *)http->req, "GET ", HDR_LEN_GET) == 0) {
            *(http->req + (HDR_LEN_GET - 1U)) = '\0';
            http->len -= HDR_LEN_GET;
            http->req = http->req + HDR_LEN_GET;
        }
        else if (net_strncasecmp((const char *)http->req, "HEAD ", HDR_LEN_HEAD) == 0) {
            *(http->req + (HDR_LEN_HEAD - 1U)) = '\0';
            http->len -= HDR_LEN_HEAD;
            http->req += HDR_LEN_HEAD;
        }
        else if (net_strncasecmp((const char *)http->req, "POST ", HDR_LEN_POST) == 0) {
            *(http->req + (HDR_LEN_POST - 1U)) = '\0';
            http->len -= HDR_LEN_POST;
            http->req += HDR_LEN_POST;
        }
        else if (net_strncasecmp((const char *)http->req, "PUT ", HDR_LEN_PUT) == 0) {
            *(http->req + (HDR_LEN_PUT - 1U)) = '\0';
            http->len -= HDR_LEN_PUT;
            http->req += HDR_LEN_PUT;
        }
        else {
            break;
        }

        /* 2. Path - URI */

        c = parse_tok(http, ' ', &hdr->url);
        if (!c) {
            break;
        }

        if (hdr->url != NULL) {
            *(http->req - 1) = '\0';
            hdr->url_q = hdr->url;
            while (*hdr->url_q != '\0') {
                if (*hdr->url_q == '?') {
                    *hdr->url_q = '\0';
                    hdr->url_q++;
                    break;
                }
                hdr->url_q++;
            }
            if (*hdr->url_q == '\0') {
                hdr->url_q = NULL;
            }
        }

        /* check sess->len? */

        /* 3. HTTP Version (upper case) */
        hdr->ver = (char *)http->req;
        if (net_strncasecmp((const char *)http->req, "HTTP/1.1", HDR_LEN_HTTP) == 0) {
            *(http->req + HDR_LEN_HTTP) = '\0';
            http->len -= HDR_LEN_HTTP;
            http->req += HDR_LEN_HTTP;
            http->hdr.kpa = 1U;
        }
        else if (net_strncasecmp((const char *)http->req, "HTTP/1.0", HDR_LEN_HTTP) == 0) {
            *(http->req + HDR_LEN_HTTP) = '\0';
            http->len -= HDR_LEN_HTTP;
            http->req += HDR_LEN_HTTP;
        }
        else {
            ercd = E_PAR;   /* Version not supported */
            break;
        }

        /* 4. Optional Headers */

        HttpReadOptHeader(http);

        if (hdr->ContentLen != 0U) {
            hdr->Content = (char *)http->req;
        }

        /* Validate Headers */
        if ((net_strncasecmp(hdr->ver, "HTTP/1.1", HDR_LEN_HTTP) == 0) && (hdr->host == NULL))  {
            ercd = E_PAR;   /* HOST: required for HTTP 1.1 */
            break;
        }

        ercd = E_OK;
    } while (0);

    return ercd;
}

static ER HttpSocReadln(T_HTTP_SERVER *http, UB **str, UH *len)
{
    ER (*rcv)(SID, VP, UH);
    ER ercd;
    UW flen;
    UB *s;

    rcv = rcv_soc;
#ifdef HTTPD_SSL_SUP
    if (0 != (http->flag & HTTPD_FLG_SSL)) {
        rcv = rcv_ssoc;
    }
#endif

    *len = 0U;

    s = http->rbuf + http->rdlen;
    *str = s;

    while (1) {

        if (http->rdlen == http->rxlen) {
            flen = http->rbufsz - http->rxlen;
            if (flen == 0U) {
                ercd = E_NOMEM;
                break;
            }
            if (flen > UH_MAX_VAL) {
                flen = UH_MAX_VAL;
            }
            ercd = rcv(http->SocketID, http->rbuf + http->rxlen, (UH)flen);
            if (ercd <= 0) {
                *str = NULL;
                ercd = E_CLS;
                break;
            }
            http->rxlen += (UW)ercd;
        }

        while (http->rdlen < http->rxlen) {
            (*len)++;
            http->rdlen++;
            if (*s == '\n') {
                return (ER)*len;
            }
            s++;
        }
    }

    return ercd;
}

static ER HttpRecvRequest(T_HTTP_SERVER *http)
{
    UB *str;
    UH len;
    ER ercd;

    net_memset(&http->hdr, 0, sizeof(T_HTTP_HEADER));

    while (1) {
        ercd = HttpSocReadln(http, (UB **)&str, &len);
        if (len == 0U) {
            /* broken http header */
            ercd = E_OBJ;
            break;
        }

        if (len == LEN_CRLF) {
            /* eof http header */
            ercd = E_OK;
            break;
        }

    }

    return ercd;
}

static ER HttpReadContent(T_HTTP_SERVER *http)
{
    ER ercd;
    UW len;
    T_HTTP_HEADER *hdr = &http->hdr;

    /* Content not present */
    if (hdr->ContentLen == 0U) {
        return E_OK;
    }

    /* Content already in recv buffer */
    len = http->rxlen - http->rdlen;
    if (len >= hdr->ContentLen) {
        /* we assume the content is end at '\0' */
        *(hdr->Content + hdr->ContentLen) = '\0';
        return E_OK;
    }

    /* Contents size is too big */
    if ((http->rbufsz - http->rdlen) < hdr->ContentLen) {
        return E_NOMEM;
    }

    /* recv partial or full content */
    len = hdr->ContentLen - len;
    HttpTcpRecv(http, http->rbuf+http->rxlen, len, &ercd);
    if (ercd <= 0) {
        return E_OBJ;
    }
    HttpTcpRecvClr(http);

    /* we assume the content is end at '\0' */
    *(hdr->Content + hdr->ContentLen) = '\0';

    return E_OK;
}

static ER HttpServerProcess(T_HTTP_SERVER *http)
{
    T_HTTP_FILE *fp;
    ER   ercd;
    T_HTTP_HEADER *hdr = &http->hdr;

    /* 1. Receive Request */

    /* Receive HTTP Header */
    ercd = HttpRecvRequest(http);
    if (ercd != E_OK) {
        return ercd;
    }

    http->hdr.method = "GET";   /* set default method */
    http->upgrade = FALSE;
    /* Parse Header             */
    ercd = HttpReadHeader(http);
    if (ercd != E_OK) {
        if (ercd == E_OBJ) {
            (void)HttpSendErrorResponse(http, "405 Unsupported method\r\n");
        }
        else {
            (void)HttpSendErrorResponse(http, "400 Bad request\r\n");
        }
        return E_OK;
    }

    if (http->upgcbk != NULL) {
      /* If upgrade != false send websocket response with key */
      if(http->upgrade != FALSE) {
          http->req = http->rbuf;     /* reset request offet and length */
          http->len = http->rdlen;

          ercd = http->upgcbk(http);    /* switch to other protocol */
          if(ercd != E_OK) {
            return E_CLS;    /* protocol process complete! close the session */
          }                  /* protocol process ignored! continue with HTTP */
      }
    }

    /* 2. Process Request */
    fp = GetHttpFile(hdr->url);
    if (fp == NULL) {
        (void)HttpReadContent(http);
        (void)HttpSendErrorResponse(http, "404 Not found\r\n");
        return E_OK;
    }

#if defined(ENA_USER_EXT) && defined(ENA_DIVRECV_CGI)
    if (fp->cbk != NULL) {
        if (0U != (fp->ext & HTTPD_EXT_MPCGI)) {
            return httpd_mp_cgi(http, fp);
        }
        else if (0U != (fp->ext & HTTPD_EXT_SMCGI)) {
            HttpCgiHandler(http, fp);
            return E_OK;
        }
    }
#else
#endif

    /* Read Content             */
    if (hdr->ContentLen > 0U) {
        ercd = HttpReadContent(http);
        if (ercd == E_NOMEM) {
            (void)HttpSendErrorResponse(http, "413 Request Entity Too Large\r\n");
#ifdef HTTPD_SSL_SUP
            if (0 != (http->flag & HTTPD_FLG_SSL)) {
                (void)cls_ssoc(http->SocketID);
            }
            else {
            (void)cls_soc(http->SocketID, SOC_TCP_SHT);
            }
#else
            (void)cls_soc(http->SocketID, SOC_TCP_SHT);
#endif
            HttpTcpRecvClr(http);
        }

        if (ercd != E_OK) {
            return ercd;
        }
    }


#ifdef ENA_USER_EXT
    if (0U != (fp->ext & HTTPD_EXT_AUTH)) {
        if (NULL == gHTTP_EXT_CBK) {
            (void)HttpSendErrorResponse(http, "404 Not found\r\n");
            return E_OK;
        }

        ercd = E_OBJ;
        if (NULL != hdr->auth) {
            if (NULL != gHTTP_EXT_CBK) {
                ercd = gHTTP_EXT_CBK(http, fp, HTTPD_EXT_AUTH);
            }
        }
        if (E_OK != ercd) {     /* Unauthorized */
            hdr->auth = NULL;
            (void)HttpSetContent(http, 0);
            (void)HttpSetContent(http, "HTTP/1.1 401 Unauthorized\r\n");
            gHTTP_EXT_CBK(http, fp, HTTPD_EXT_AUTH);
            (void)HttpSetContentLen(http, "Content-Length: ", net_strlen("401 Unauthorized") + TAG_LEN_H);
            (void)HttpSetContentKpa(http);
            (void)HttpSendBuffer(http, "\r\n<h1>401 Unauthorized</h1>", sizeof("\r\n<h1>401 Unauthorized</h1>")-1U);
            return E_OK;
        }
    }
#endif

    if (NULL == fp->cbk) {
        (void)HttpSetContent(http, 0);
        (void)HttpSetContent(http, "HTTP/1.1 200 OK\r\n");
        (void)HttpSetContent(http, "Content-Type: ");
        (void)HttpSetContent(http, fp->ctype);
        (void)HttpSetContent(http, "\r\n");
        (void)HttpSetContentLen(http, "Content-Length: ", fp->len);
        (void)HttpSetContentKpa(http);
#ifdef ENA_USER_EXT
        if (0U != (fp->ext & HTTPD_EXT_UHDR)) {
            if (NULL != gHTTP_EXT_CBK) {
                ercd = gHTTP_EXT_CBK(http, fp, HTTPD_EXT_UHDR);
            }
        }
#endif
        (void)HttpSetContent(http, "\r\n");
        if (*http->hdr.method == 'H') {     /* HEAD method */
            (void)HttpSendBuffer(http, NULL, 0U);
        }
        else {
            (void)HttpSendBuffer(http, fp->file, fp->len);
        }
    }
    else {
        HttpCgiHandler(http, fp);
    }

    return E_OK;
}

static ER HttpServerListen(T_HTTP_SERVER *http)
{
#ifdef HTTPD_SSL_SUP
    T_SSL_NODE ssl_host;
#endif
    T_NODE host;
    ER ercd;

#ifdef HTTPD_SSL_SUP
    net_memset(&ssl_host, 0, sizeof(ssl_host));
    ssl_host.host.num = http->NetChannel;
    ssl_host.host.ver = http->ver;
    ssl_host.node_type = SSL_SERVER_NODE;
#endif
    net_memset(&host, 0, sizeof(host));
    host.num = http->NetChannel;
    host.ver = http->ver;
#ifdef HTTPD_SSL_SUP
    if (0 != (http->flag & HTTPD_FLG_SSL)) {
        ercd = con_ssoc(http->SocketID, &ssl_host, 0, HTTPS_SSOC_TMO);
        if (ercd > 0) {
            ercd = E_OK;
        }
        else {
            if (ercd == 0) {
                ercd = E_NOID;
            }
        }
    }
    else
#endif
    {
    ercd = con_soc(http->SocketID, &host, SOC_SER);
    }
    return ercd;
}

static void HttpServerClose(T_HTTP_SERVER *http)
{
#ifdef HTTPD_SSL_SUP
    if (0 != (http->flag & HTTPD_FLG_SSL)) {
        (void)cls_ssoc(http->SocketID);
    }
    else
#endif
    {
    (void)cls_soc(http->SocketID, SOC_TCP_CLS);
    }
}

static void enq_http_server( T_HTTP_SERVER *node )
{
    if ( gHTTP_SERVER_STACK == NULL ) {
        /* set first stack(FILO)                                             */
        node->next = NULL;
        gHTTP_SERVER_STACK = node;
    }
    else {
        /* set stack(FILO)                                                   */
        node->next = gHTTP_SERVER_STACK;
        gHTTP_SERVER_STACK = node;
    }

    return;
}

#ifndef NET_HW_OS
static T_HTTP_SERVER *deq_http_server( void )
{
    T_HTTP_SERVER *node;                /* http node session handle          */

    /* get node(FILO)                                                        */
    node = gHTTP_SERVER_STACK;
    if ( node != NULL ) {
        /* set next node                                                     */
        gHTTP_SERVER_STACK = node->next;
    }

    return node;
}
#endif


ER http_server(T_HTTP_SERVER *http)
{
    T_NET_BUF *pkt[2];      // buffer for send and recv
    ER ercd;

    /* Validate parameter */
    if (http == NULL) {
        ercd = E_PAR;
    }
    else if ((http->SocketID == 0U) || (http->SocketID > (SID)NET_SOC_MAX)) {
        ercd = E_PAR;
    }
#if !defined(HTTPD_SSL_SUP)
    else if (0 != (http->flag & HTTPD_FLG_SSL)) {
        ercd = E_NOSPT;
    }
#endif
    else {
        ercd = ref_soc(http->SocketID, SOC_PRT_LOCAL, (VP)&http->Port);
        if (ercd != E_OK) {
            ercd = E_PAR;
        }
        else if (http->Port == 0U) {
            ercd = E_PAR;
        }
        else {
            /* do nothing */
        }
    }
    if (ercd != E_OK) {
        return ercd;
    }

    /* get own task id                                                       */
    ercd = get_tid( &http->server_tsk_id );
    if ( ercd != E_OK ) {
        return ercd;
    }

    pkt[0] = NULL;
    pkt[1] = NULL;
    if (http->rbuf == NULL) {
        ercd = net_buf_get(&pkt[0], HTTP_BUF_SIZ, TMO_POL);
        if (ercd != E_OK) {
            return E_NOMEM;
        }
        http->rbuf       = pkt[0]->hdr;
        http->rbufsz     = HTTP_BUF_SIZ;
    }
    http->rbufsz--; /* for end of contents */

    if (http->sbuf == NULL) {
        ercd = net_buf_get(&pkt[1], HTTP_BUF_SIZ, TMO_POL);
        if (ercd != E_OK) {
            if (pkt[0] != NULL) {
                net_buf_ret(pkt[0]);
            }
            return E_NOMEM;
        }
        http->sbuf       = pkt[1]->hdr;
        http->sbufsz     = HTTP_BUF_SIZ;
    }

    /* enqueue http server handle                                            */
    enq_http_server( http );

    /* set http server boot status                                           */
    http->server_tsk_stat = HTTP_SERVER_BOOT;

#ifdef ENA_KEEP_ALIVE
    while (1) {
        if (0U == http->hdr.kpa) {
            ercd = HttpServerListen(http);
            if (http->server_tsk_stat == HTTP_SERVER_STOP) {
                break;              /* http server stop proc */
            }
            if (ercd != E_OK) {
                (void)tslp_tsk(1);
                continue;
            }
            http->kpa_max = HTTP_KPA_MAX;
        }

        http->rxlen      = 0U;
        http->rdlen      = 0U;
        http->req        = NULL;
        http->len        = 0U;
        ercd = HttpServerProcess(http);
        if (http->server_tsk_stat == HTTP_SERVER_STOP) {
            break;              /* http server stop proc */
        }
        else if (0 > ercd) {
            http->hdr.kpa = 0U;
        }
        else if (0U == http->kpa_max) {
            http->hdr.kpa = 0U;
        }
        else {
            --http->kpa_max;
        }

        if (0U == http->hdr.kpa) {
            HttpServerClose(http);
        }
    }
#else
    while (1) {
        ercd = HttpServerListen(http);
        if (http->server_tsk_stat == HTTP_SERVER_STOP) {
            break;              /* http server stop proc */
        }
        if (ercd != E_OK) {
            (void)tslp_tsk(1);
            continue;
        }

        http->rxlen      = 0U;
        http->rdlen      = 0U;
        http->req        = NULL;
        http->len        = 0U;

        ercd = HttpServerProcess(http);
        if (http->server_tsk_stat == HTTP_SERVER_STOP) {
            break;              /* http server stop proc */
        }

        HttpServerClose(http);
    }
#endif
    if (http->server_tsk_stat == HTTP_SERVER_STOP) {
        ercd = E_RLWAI;
    }

    /*-----------------------------------------------------------------------*/
    /* last cleanup                                                          */
    /*-----------------------------------------------------------------------*/
    HttpServerClose(http);

    net_buf_ret(pkt[0]);
    http->rbuf = NULL;
    http->rbufsz = 0U;
    net_buf_ret(pkt[1]);
    http->sbuf = NULL;
    http->sbufsz = 0U;


    return ercd;
}

#ifndef NET_HW_OS
ER http_server_stop( UW retry )
{
    T_HTTP_SERVER   *http;
    T_RTST          tskinfo;            /* task status info                  */
    UW              i;                  /* loop counter                      */
    ER              ercd;               /* function result                   */
    ID              tsk_id;
    SID             sid;

    /*-----------------------------------------------------------------------*/
    /* initialize variable                                                   */
    /*-----------------------------------------------------------------------*/
    ercd  = E_OK;


    /*-----------------------------------------------------------------------*/
    /* http server task stop proc                                            */
    /* all session dequeue                                                   */
    /*-----------------------------------------------------------------------*/
    while (1) {
        http = deq_http_server();
        if (http == NULL) {
            break;
        }

        /* set http server stop flag                                         */
        tsk_id = http->server_tsk_id;
        sid = http->SocketID;
        http->server_tsk_stat = HTTP_SERVER_STOP;

        for (i = 0U; i <= retry; i++) {
            /* all cancel socket proc                                        */
            ercd = abt_soc( sid, SOC_ABT_ALL );
            if ( ercd != E_OK ) {
                break;
            }
            /* wait retrying                                                 */
            (void)dly_tsk( HTTP_SERVER_RETRY_WAIT );

            /* get task status                                               */
            ercd = ref_tst( tsk_id, &tskinfo );
            if ( ercd != E_OK ) {
                break;
            }
            /* stopped?                                                      */
            if ( tskinfo.tskstat == TTS_DMT ) {
                break;
            }
        }

        if (ercd != E_OK) {
            break;
        }
        /* retry over check                                                  */
        if (i > retry) {
            ercd = E_TMOUT;
            break;
        }
    }

    return ercd;
}
#endif
