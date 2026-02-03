/***************************************************************************
    HTTP Sample (Operating as command in shell)
    Copyright (c)  2014-2025, eForce Co., Ltd. All rights reserved.

    2014/04/02: Created.
    2017/01/01: Changed content reception processing.
    2017/09/04: Changed Chunked receive function.
                  - remove CRLF.
                  - receive CRLF after receive chunked size 0 (last-chunked)
                Changed return value check http_rcv_res().
                  treat as error when http_rcv_res() return value is not
                  equal argument.
    2017/09/05: Changed configuration string SSL_SUP -> HTTPC_SSL_SUP
                Remove compile error codes, when SSL enabled.
    2025.01.27: Supported Proxy Authentication.
    2025.07.30: Supported IPv6 communication using uNet3-IPv6.
 ***************************************************************************/
#include "kernel.h"
#include "sample_netapp_cfg.h"      /* include Sample Application Settings */

#if SAMPLE_ENA_HTTPc
#include "http_client.h"

static VB http_buf[1024];

#ifdef ENA_AUTH_BASIC
static VB basic_user[SPL_HTTP_USR_BUF]  = SPL_HTTP_B_USR;
static VB basic_pass[SPL_HTTP_PW_BUF]   = SPL_HTTP_B_PW;
#endif
#ifdef ENA_AUTH_DIGEST
static VB digest_user[SPL_HTTP_USR_BUF] = SPL_HTTP_D_USR;
static VB digest_pass[SPL_HTTP_PW_BUF]  = SPL_HTTP_D_PW;
#endif
static UB use_proxy = 0;
static UW prx_ipa;
static UW prx_port;

#ifdef HTTPC_SSL_SUP
/* printf like function */
extern void shell_printf(VP ctrl, const char *fmt, ...) ;
#endif

#if (0 == SAMPLE_USE_GENSRC)	/* no use configurator */
SID ID_SOC_HTTPC;
ID ID_HTTP_MSG_MPF;     /* fixed memory pool */

ER sample_httpc_ini()
{
	ER ercd;
    T_NODE lo_host;
    T_CMPF  k_cmpf;

    lo_host.num = SAMPLE_SOCDEV_CLI;
    lo_host.ver = IP_VER4;
    lo_host.ipa = INADDR_ANY;
    lo_host.port = PORT_ANY;

    /* HTTPc */
    ercd = cre_soc(IP_PROTO_TCP, &lo_host);
    if (0 >= ercd) {
        return ercd;
    }
    ID_SOC_HTTPC = ercd;

    net_memset(&k_cmpf, 0, sizeof(k_cmpf));
    k_cmpf.mpfatr = TA_TFIFO;
    k_cmpf.blkcnt = 2;
    k_cmpf.blksz  = 1024;
    ercd = acre_mpf(&k_cmpf);
    if (0 >= ercd) {
        return ercd;
    }
    ID_HTTP_MSG_MPF = ercd;

    return E_OK;
}
#endif

/* Receive chunk contents */
static ER rcv_chunked_contents(VP ctrl, T_HTTP_CLIENT *http)
{
    UB chunkhead;
    UH chunklen;
    ER ercd;
    int len;
    UH l;

    while (1) {
        /* read chunk head */
        chunklen = 0;
        while (1) {
            ercd = http_rcv_res(http, &chunkhead, 1);
            if (ercd != 1) {
                shell_puts(ctrl, "HTTP response error"SPL_LF);
                return ercd;
            }
            if (chunkhead == 0x0D) {
                continue;
            }
            if (chunkhead == 0x0A) {
                break;
            }
            if ('0' <= chunkhead && chunkhead <= '9') {
                chunkhead -= '0';
            } else if ('a' <= chunkhead && chunkhead <= 'f') {
                chunkhead -= ('a' - 10);
            } else if ('A' <= chunkhead && chunkhead <= 'F') {
                chunkhead -= ('A' - 10);
            } else {
                shell_puts(ctrl, "Recognize chunk header error"SPL_LF);
                return E_OBJ;
            }
            chunklen <<= 4;
            chunklen += chunkhead;
        }

        /* last-chunk */
        if (chunklen == 0) {
            break;
        }

        chunklen += 2;  /* +2 : length of CRLF */

        /* read data */
        while (chunklen > 0) {
            net_memset(http_buf, 0, sizeof(http_buf));
            len = sizeof(http_buf) - 1;
            if (len > chunklen) {
                len = chunklen;
            }
            ercd = http_rcv_res(http, (UB*)http_buf, len);
            if (ercd != len) {
                if (ercd < 0) {
                    return ercd;
                } else {
                    return E_TMOUT;
                }
            }
            http_buf[ercd] = '\0';

#if 1		/* remove CRLF (bounday of chunk-data) */
            l = chunklen - ercd;
            if (l == 1) {
                http_buf[ercd - 1] = '\0'; /* CR -> '\0' */
            }
            if (l == 0) {
                if (ercd == 1) {
                    http_buf[0] = '0';         /* LF -> '\0' */
                } else {
                    http_buf[ercd - 2] = '\0'; /* CR -> '\0' */
                    http_buf[ercd - 1] = '\0'; /* LF -> '\0' */
                }
            }
#endif
            shell_puts(ctrl, http_buf);
            chunklen -= ercd;
        }
    }

    /* receive CRLF (end of chunk-body) */
    ercd = http_rcv_res(http, (UB *)http_buf, 2);
    if (ercd != 2) {
        return E_TMOUT;
    }

    if ((http_buf[0] != 0x0d) || (http_buf[1] != 0x0a)) {
        return E_OBJ;
    }

    return E_OK;
}

/* Receive contents */
static ER rcv_contents(VP ctrl, T_HTTP_CLIENT *http)
{
    ER ercd;
    int len, i;

    i = 0;
    while (i < http->response.contentlen) {
        net_memset(http_buf, 0, sizeof(http_buf));
        len = sizeof(http_buf);
        len = len < http->response.contentlen-i ? len : http->response.contentlen-i;

        ercd = http_rcv_res(http, (UB*)http_buf, len);
        if (ercd <= 0) {
            break;
        }
        shell_puts(ctrl, http_buf);
        i += ercd;
    }
    return E_OK;
}

/* Receiving contents until it is disconnected from the server side */
static ER rcv_continuous_contents(VP ctrl, T_HTTP_CLIENT *http)
{
    ER ercd;

    while (1) {
        net_memset(http_buf, 0, sizeof(http_buf));
        ercd = http_rcv_res(http, (UB*)http_buf, sizeof(http_buf));
        if (ercd != sizeof(http_buf)) {
            break;
        }
        shell_puts(ctrl, http_buf);
    }
    return E_OK;
}

static ER rcv_contents_mng(VP ctrl, T_HTTP_CLIENT *http)
{
    ER ercd;

    /* Check HTTP response as to whether a message valid. */
    switch (http->response.code) {
    case 204:
    case 304:
        return E_OK;    /* message-body invalid */

    default:
        if (http->response.code < 200) {    /* 1xx */
            return E_OK;        /* message-body invalid */
        }
    }

    /* chunk format */
    if (http->response.chunked) {
        ercd = rcv_chunked_contents(ctrl, http);
    } else if (http->response.contentlen){
        ercd = rcv_contents(ctrl, http);
    } else {
        /* streaming ?*/
        ercd = rcv_continuous_contents(ctrl, http);
    }

    return ercd;
}

#ifdef HTTPC_SSL_SUP
/* Output SSL Infomation */
void shell_puts_SSL(VP ctrl, ID ssl_id)
{
    ER ercd;
    UB num;
    T_VALID_TIM tim;
    VB buf[128];

    /* Certificate information */
    shell_puts(ctrl, SPL_LF SPL_LF "-- Certificate --" SPL_LF);

    /* Subject */
    shell_puts(ctrl, SPL_LF"  [Subject] ");
    ercd = get_sbj_name(ssl_id, buf, sizeof(buf));
    if (0 < ercd) {
        buf[ercd] = '\0';
        shell_puts(ctrl, buf);
    } else {
        shell_printf(ctrl, SPL_LF" ... get_sbj_name() = %d" SPL_LF, ercd);
    }

    /* Issuer */
    shell_puts(ctrl, SPL_LF"  [Issuer ] ");
    ercd = get_issu_name(ssl_id, buf, sizeof(buf));
    if (0 < ercd) {
        buf[ercd] = '\0';
        shell_puts(ctrl, buf);
    } else {
        shell_printf(ctrl, SPL_LF" ... get_issu_name() = %d" SPL_LF, ercd);
    }

    /* DNS */
    shell_puts(ctrl, SPL_LF"  [DNS    ]");
    ercd = get_dns_cnt(ssl_id);
    if (0 <= ercd) {
        for (num = ercd; num; --num) {
            ercd = get_dns_name(ssl_id, buf, sizeof(buf), num - 1);
            shell_puts(ctrl, SPL_LF"    ");
            if (0 < ercd) {
                buf[ercd] = '\0';
                shell_puts(ctrl, buf);

                if (sizeof(buf) < ercd) {
                    shell_puts(ctrl, " (Buffer too short)");
                }
            } else {
                shell_printf(ctrl, SPL_LF" ... get_dns_name() = %d" SPL_LF, ercd);
            }
        }
    } else {
        shell_printf(ctrl, SPL_LF" ... get_dns_cnt() = %d" SPL_LF, ercd);
    }

    /* Verify the signature */
    shell_puts(ctrl,SPL_LF"  [Verify the signature]"SPL_LF "    ");
    ercd = get_sig_verify(ssl_id);
    shell_puts(ctrl, (ercd == E_OK) ? "OK" : "NG");

    /* Validity period */
    shell_puts(ctrl, SPL_LF"  [Validity period]"SPL_LF);
    shell_puts(ctrl, "    Start : ");
    ercd = get_valid_start(ssl_id, &tim);
    if (ercd == E_OK) {
        /* Print time */
        shell_printf(
            ctrl,
            "%04d/%02d/%02d %02d:%02d:%02d"SPL_LF,
            tim.year,
            tim.mon,
            tim.day,
            tim.hour,
            tim.min,
            tim.sec
        );
    } else {
        shell_printf(ctrl, SPL_LF" ... get_valid_start() = %d" SPL_LF, ercd);
    }

    shell_puts(ctrl, "    End   : ");
    ercd = get_valid_end(ssl_id, &tim);
    if (ercd == E_OK) {
        /* Print time */
        shell_printf(
            ctrl,
            "%04d/%02d/%02d %02d:%02d:%02d"SPL_LF,
            tim.year,
            tim.mon,
            tim.day,
            tim.hour,
            tim.min,
            tim.sec
        );
    } else {
        shell_printf(ctrl, SPL_LF" ... get_valid_end() = %d" SPL_LF, ercd);
    }

    shell_puts(ctrl, SPL_LF);
}
#endif

static T_HTTP_METHOD get_http_method(const VB *str)
{
    T_HTTP_METHOD ret;


    if (str == NULL) {
        ret = HTTP_METHOD_NOSUP;
    }
    else if (0 == (net_strcmp(str, "http_get"))) {
        ret = HTTP_METHOD_GET;
    }
    else if (0 == (net_strcmp(str, "http_post"))) {
        ret = HTTP_METHOD_POST;
    }
    else if (0 == (net_strcmp(str, "http_put"))) {
        ret = HTTP_METHOD_PUT;
    }
    else if (0 == (net_strcmp(str, "http_delete"))) {
        ret = HTTP_METHOD_DELETE;
    }
    else if (0 == (net_strcmp(str, "http_head"))) {
        ret = HTTP_METHOD_HEAD;
    }
    else {
        ret = HTTP_METHOD_NOSUP;
    }

    return ret;
}

static ER exec_http_cmd(T_HTTP_CLIENT *http, const char *url, T_HTTP_POSTDATA *data, T_HTTP_METHOD method)
{
    ER ercd;

    switch (method) {
    case HTTP_METHOD_GET:
        ercd = http_cmd_get(http, url);
        break;

    case HTTP_METHOD_POST:
        ercd = http_cmd_post(http, url, data);
        break;

    case HTTP_METHOD_PUT:
        ercd = http_cmd_put(http, url, data);
        break;

    case HTTP_METHOD_DELETE:
        ercd = http_cmd_delete(http, url);
        break;

    case HTTP_METHOD_HEAD:
        ercd = http_cmd_head(http, url);
        break;

    default:
    	ercd = E_NOSPT;
    	break;
    }

    return ercd;
}


/* Command 'http_xxx' */
ER shell_usr_cmd_http(VP ctrl, INT argc, VB *argv[])
{
#if defined(ENA_AUTH_BASIC) || defined(ENA_AUTH_DIGEST)
    UB auth_flg = 0;
#endif
    ER ercd;
    T_HTTP_CLIENT hc;
    VB *url = argv[1];
    T_HTTP_POSTDATA data;
    T_HTTP_METHOD method;
	T_NODE nod;

    method = get_http_method(argv[0]);
    if (method == HTTP_METHOD_NOSUP) {
        return E_NOSPT;
    }

    net_memset(&hc, 0, sizeof(hc));
    hc.http_sid = ID_SOC_HTTPC;
    if (use_proxy) {
        hc.svr.ipa = prx_ipa;
        hc.svr.port = prx_port;
        hc.svr.ver = IP_VER4;
        hc.svr.num = 0;
        hc.prx = use_proxy;
    }
    else {
        (void)ref_soc(hc.http_sid, SOC_IP_LOCAL, (VP)&nod);
        if (nod.ver == IP_VER4) {
            ercd = http_get_ipaddr(url, &hc.svr, ID_SOC_DNSC, SPL_DNS_SERVER);
        }
        else {
            ercd = http_get_ip6addr(url, &hc.svr, ID_SOC_DNSC, SPL_DNS_SERVER);
        }
        if (ercd != E_OK) {
            return ercd;
        }
    }
    hc.mpf_id = ID_HTTP_MSG_MPF;

    net_memset(&data, 0, sizeof(data));
    if (2 < argc) {
        data.buf = argv[2];
        data.buflen = net_strlen(data.buf);
    }
    if (3 < argc) {
		data.contenttype = argv[3];
    }

#if defined(ENA_AUTH_BASIC) || defined(ENA_AUTH_DIGEST)
RETRY:
#endif
    ercd = exec_http_cmd(&hc, argv[1], &data, method);
    if (ercd != E_OK) {
        goto END_CONNECT;
    }

    /* Display Response Code */
    net_strcpy(http_buf, SPL_LF" # Response-Code = ");
    net_itoa(hc.response.code, &http_buf[net_strlen(http_buf)], 10);
    net_strcat(http_buf, SPL_LF);
    shell_puts(ctrl, http_buf);

    /* Display Response Body */
    if (HTTP_METHOD_HEAD != method) {
        ercd = rcv_contents_mng(ctrl, &hc);
    }

    /* Authentication error */
    if ((hc.response.code == 401) || (hc.response.code == 407)) {
#if defined(ENA_AUTH_BASIC) || defined(ENA_AUTH_DIGEST)
        if (auth_flg) {
            /* Authentication failure */
            ercd = E_OBJ;
        }
#ifdef ENA_AUTH_BASIC
        else if ((hc.authtype == WWW_AUTH_BASIC) || (hc.authtype == PROXY_AUTH_BASIC)) {
            auth_flg = 1;
            hc.usr = basic_user;
            hc.pw = basic_pass;
            http_cls(&hc);
            goto RETRY;
        }
#endif
#ifdef ENA_AUTH_DIGEST
        else if ((hc.authtype == WWW_AUTH_DIGEST) || (hc.authtype == PROXY_AUTH_DIGEST)) {
            auth_flg = 1;
            hc.usr = digest_user;
            hc.pw = digest_pass;
            hc.flag |= HTTPC_FLG_CLS_NOT_RELBUF;
            http_cls(&hc);
            goto RETRY;
        }
#endif
#else
        /* Authentication failure */
        ercd = E_OBJ;
#endif
    }
#ifdef HTTPC_SSL_SUP
    else if (hc.response.code == 200) {
        if (hc.scm == HTTP_SCHEME_HTTPS) {
            shell_puts_SSL(ctrl, hc.ssl_id);

        }
    }
#endif

END_CONNECT:
    hc.flag &= ~HTTPC_FLG_CLS_NOT_RELBUF;
    http_cls(&hc);
    return ercd;
}

/* Command 'http_cfg' */
ER shell_usr_cmd_http_cfg(VP ctrl, INT argc, VB *argv[])
{
#define SPL_HTTP_BUF    ((SPL_HTTP_USR_BUF > SPL_HTTP_PW_BUF) ? SPL_HTTP_USR_BUF : SPL_HTTP_PW_BUF)
    VB tmp[SPL_HTTP_BUF];

#ifdef ENA_AUTH_BASIC
    shell_puts(ctrl, SPL_LF" Basic UserName: ");
    shell_gets(ctrl, tmp, sizeof(tmp));
    if (0 < net_strlen(tmp))    net_strcpy(basic_user, tmp);

    shell_puts(ctrl, SPL_LF" Basic Password: ");
    shell_gets(ctrl, tmp, sizeof(tmp));
    if (0 < net_strlen(tmp))    net_strcpy(basic_pass, tmp);
#endif

#ifdef ENA_AUTH_DIGEST
    shell_puts(ctrl, SPL_LF" Digest UserName: ");
    shell_gets(ctrl, tmp, sizeof(tmp));
    if (0 < net_strlen(tmp))    net_strcpy(digest_user, tmp);

    shell_puts(ctrl, SPL_LF" Digest Password: ");
    shell_gets(ctrl, tmp, sizeof(tmp));
    if (0 < net_strlen(tmp))    net_strcpy(digest_pass, tmp);
#endif

    /* Proxy Settings (no error check) */
    shell_puts(ctrl, SPL_LF" Use Proxy (0:No, 0<>:Yes): ");
    shell_gets(ctrl, tmp, sizeof(tmp));
    if (0 < net_strlen(tmp))    use_proxy = net_atoi(tmp);
    if (use_proxy) {
        shell_puts(ctrl, SPL_LF"  Proxy Server IP  : ");
        shell_gets(ctrl, tmp, sizeof(tmp));
        if (0 < net_strlen(tmp))    prx_ipa = ip_aton(tmp);

        shell_puts(ctrl, SPL_LF"  Proxy Server Port: ");
        shell_gets(ctrl, tmp, sizeof(tmp));
        if (0 < net_strlen(tmp))    prx_port = net_atoi(tmp);
    }

    return E_OK;
}

#endif  /* #if SAMPLE_ENA_HTTPc */
