/***********************************************************************
    MICRO C CUBE / COMPACT
    SSL Sample Program
    Copyright (c) 2016, eForce Co., Ltd. All rights reserved.
    
    Version Information
      2016.05.28: Created.
 ***********************************************************************/

#include "kernel.h"
#include "sample_netapp_cfg.h"      /* include Sample Application Settings */

#if SAMPLE_ENA_SSL
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "net_hdr.h"
#include "ssl_hdr.h"
//#include "net_id.h"

extern void shell_printf(VP ctrl, const char *fmt, ...);
extern void simple_sprintf(char *str, const char *fmt, ...);
extern void shell_dump_buf(VP ctrl, const VB *buf, UINT len, UINT print_addr);

#define SSL_CER_PEM
#include "sample_ssl_cert.h"

#define SSL_PORT        443                     /* Port */
#define HTTP_CON_TMO    60000                   /* Timeout */

/* Data buffer */

#define APL_BUF_LEN (1460) /* Sizeof data in TCP packet (1460byte) */
VB apl_buf[APL_BUF_LEN];
UH apl_buf_rxlen;

ER apl_ssl_http_status_line(VP ctrl);
ER apl_ssl_http_res_hdr(VP ctrl);
ER apl_ssl_http_res_body(VP ctrl, UINT content_len);
ER apl_ssl_recv_line(void);
UINT apl_ssl_parse_content_len(void);

ER apl_ssl_ini(void)
{
    T_SSL_RAW_CERT ca;
    
    ca.dat = (VP)apl_ssl_cer;
#ifdef SSL_CER_PEM
    ca.len = sizeof(apl_ssl_cer) - 1;
    ca.type = CERT_TYPE_PEM;
#else
    ca.len = sizeof(apl_ssl_cer);
    ca.type = CERT_TYPE_DER;
#endif
    
    return ssl_cli_ini(&ca, 1);
}

#if (0 == SAMPLE_USE_GENSRC)
SID ID_SSL_SOC;

ER sample_ssl_ini()
{
    ER ercd;
    T_NODE lo_host = {0};

    lo_host.num = SAMPLE_SOCDEV_CLI;
    lo_host.ver = IP_VER4;
    lo_host.ipa = INADDR_ANY;

    ercd         = cre_soc(IP_PROTO_TCP, &lo_host);
    if (0 >= ercd) {
        return ercd;
    }
    ID_SSL_SOC = (SID)ercd;
    (void)cfg_soc((SID)ercd, SOC_TMO_SND, (VP)25000U);
    (void)cfg_soc((SID)ercd, SOC_TMO_RCV, (VP)25000U);
    (void)cfg_soc((SID)ercd, SOC_TMO_CON, (VP)25000U);
    (void)cfg_soc((SID)ercd, SOC_TMO_CLS, (VP)25000U);

    return apl_ssl_ini();
}

#else
ER sample_ssl_ini()
{
    return apl_ssl_ini();
}
#endif


/* Command 'ssl server_ip path' */
ER shell_usr_cmd_ssl(VP ctrl, INT argc, VB *argv[])
{
    T_NODE server = {0};
    ID sid;
    ER ercd;
    UINT cnt;
    UINT i;
    UINT len;
    UINT msg_len;
    T_VALID_TIM tim;
    
    VB *server_str = argv[1];
    VB *path       = argv[2];

    shell_printf(ctrl, "\r\n");
    
    if (argc != 3) {
        shell_printf(ctrl, "usage : %s server_ip path\r\n", argv[0]);
        return E_PAR;
    }

    /* ------ Connect ------------------------------------------------------*/
    server.ipa  = ip_aton(argv[1]);
    if (server.ipa == 0) {
        shell_printf(ctrl, "Input ip address is bad format. (input = %s)\r\n", argv[1]);
        return E_PAR;
    }
    
    server.port = SSL_PORT;
    
    ercd = con_ssoc(ID_SSL_SOC, &server, 0, HTTP_CON_TMO);
    if (ercd <= E_OK) {
        shell_printf(ctrl, "con_ssoc() return error.\r\n");
        return E_OBJ;
    }
    
    sid = ercd;
    
    /* ------ Print Certificate information --------------------------------*/
    shell_printf(ctrl, "\r\n-- Certificate --\r\n");
    
    /* Subject */
    ercd = get_sbj_name(sid, apl_buf, (APL_BUF_LEN - 1));
    if (ercd > 0) {
        cnt = (ercd > (APL_BUF_LEN -1)) ? (APL_BUF_LEN -1) : ercd;
        apl_buf[cnt] = '\0';
        shell_printf(ctrl, "\r\n  [Subject] %s", apl_buf);
    }
    
    /* Issuer */
    ercd = get_issu_name(sid, (VB *)apl_buf, (APL_BUF_LEN -1));
    if (ercd > 0) {
        cnt = (ercd > (APL_BUF_LEN -1)) ? (APL_BUF_LEN -1) : ercd;
        apl_buf[cnt] = '\0';
        shell_printf(ctrl, "\r\n  [Issuer ] %s", apl_buf);
    }
    
    /* DNS */
    ercd = get_dns_cnt(sid);
    if (ercd > 0) {
        
        shell_printf(ctrl, "\r\n  [DNS    ] ");
        
        cnt = ercd;
        for (i = 0; i < cnt; i++) {
            ercd = get_dns_name(sid, (VB *)apl_buf, APL_BUF_LEN-1, i);
            if (ercd > 0) {
                cnt = (ercd > (APL_BUF_LEN -1)) ? (APL_BUF_LEN -1) : ercd;
                apl_buf[cnt] = '\0';
                shell_printf(ctrl, "\r\n    %s", apl_buf);
            }
        }
    }
    
    /* Verify the signature */
    shell_printf(ctrl, "\r\n  [Verify the signature]\r\n    ");
    ercd = get_sig_verify(sid);
    if (ercd == E_OK) {
        shell_printf(ctrl, "OK");
    } else if (ercd == EV_NOT_VLD) {
        shell_printf(ctrl, "NG");
    }
    
    /* Validity period */
    shell_printf(ctrl, "\r\n  [Validity period]");
    ercd = get_valid_start(sid, &tim);
    if (ercd == E_OK) {
        shell_printf(ctrl, "\r\n    Start : %d/%d/%d %d:%d:%d", tim.year, tim.mon, tim.day, tim.hour, tim.min, tim.sec);
    }
    
    ercd = get_valid_end(sid, &tim);
    if (ercd == E_OK) {
        shell_printf(ctrl, "\r\n    End   : %d/%d/%d %d:%d:%d", tim.year, tim.mon, tim.day, tim.hour, tim.min, tim.sec);
    }
    
    shell_printf(ctrl, "\r\n");
    
    /* ------ Send HTTP GET request --------------------------------*/
    shell_printf(ctrl, "\r\n-- Get Request --\r\n\r\n");
    
    /* Set send data to apl_buf */
    simple_sprintf((char *)apl_buf, "GET %s HTTP/1.1\r\nHost: %s\r\n\r\n", path, server_str);

    /* Send */
    len = 0;
    msg_len = strlen((char *)apl_buf);
    
    while (len < msg_len) {
        ercd = snd_ssoc(ID_SSL_SOC, apl_buf + len, msg_len - len);
        if (ercd <= E_OK) {
            cls_ssoc(ID_SSL_SOC);
            return E_OBJ;
        }
        len += ercd;
    }
    
    /* ------ Receive HTTP response ------------------------------------*/
    apl_buf_rxlen = 0;
    memset(apl_buf, 0, APL_BUF_LEN);
    
    /* Statu Line */
    ercd = apl_ssl_http_status_line(ctrl);
    if (ercd != E_OK) {
        cls_ssoc(ID_SSL_SOC);
        return ercd;
    }
    
    /* Response Header */
    ercd = apl_ssl_http_res_hdr(ctrl);
    if (ercd <= E_OK) {
        cls_ssoc(ID_SSL_SOC);
        return ercd;
    }
    
    /* Response Body */
    ercd = apl_ssl_http_res_body(ctrl, ercd);
    
    cls_ssoc(ID_SSL_SOC);
    
    return ercd;
}

ER apl_ssl_http_status_line(VP ctrl)
{
    ER ercd;
    UINT l;
    
    shell_printf(ctrl, "\r\n"
                       "-----------------------------------\r\n");
    shell_printf(ctrl, "    HTTP Status Line\r\n");
    shell_printf(ctrl, "-----------------------------------\r\n");
    
    ercd = apl_ssl_recv_line();
    if (ercd == E_OK) {
        shell_printf(ctrl, "%s\r\n", apl_buf);
        l = strlen(apl_buf);
        l += 2;
        memmove(apl_buf, apl_buf+l, apl_buf_rxlen -l);
        apl_buf_rxlen -= l;
        apl_buf[apl_buf_rxlen] = '\0';
    }
    
    return ercd;
}

ER apl_ssl_http_res_hdr(VP ctrl)
{
    UINT content_len = 0;
    ER ercd;
    UINT l;
    
    shell_printf(ctrl, "\r\n"
                       "-----------------------------------\r\n");
    shell_printf(ctrl, "    HTTP response header\r\n");
    shell_printf(ctrl, "-----------------------------------\r\n");
    
    /* Break when empty line  */
    while (TRUE) {
         ercd = apl_ssl_recv_line();
         if (ercd == E_OK) {
             shell_printf(ctrl, "%s\r\n", apl_buf);
             if (strncmp("Content-Length", apl_buf, sizeof("Content-Length")-1) == 0) {
                 content_len = apl_ssl_parse_content_len();
             } else if (strcmp("", apl_buf) == 0) {
                 break; /* Empty line */
             }
             
             l = strlen(apl_buf);
             l += 2; /* space of "\r\n" */
             
             memmove(apl_buf, apl_buf+l, apl_buf_rxlen - l);
             apl_buf_rxlen -= l;
             apl_buf[apl_buf_rxlen] = '\0';
             
         } else {
             return ercd;
         }
    }
    
    memmove(apl_buf, apl_buf+2, apl_buf_rxlen - 2);
    apl_buf_rxlen -= 2;
    apl_buf[apl_buf_rxlen] = '\0';
    
    return (ER)content_len;
}

ER apl_ssl_http_res_body(VP ctrl, UINT content_len)
{
    UINT l = 0;
    UINT rx_len;
    ER ercd;
    
    l = apl_buf_rxlen;
    
    shell_printf(ctrl, "\r\n"
                       "-----------------------------------\r\n");
    shell_printf(ctrl, "    HTTP response body\r\n");
    shell_printf(ctrl, "-----------------------------------\r\n");
    
    if (l > 0) {
    
        /* print buffer */
        shell_dump_buf(ctrl, apl_buf, apl_buf_rxlen, 0);
        apl_buf_rxlen = 0;
    }
    
    while (l < content_len) {
        
        rx_len = content_len - l;
        if (rx_len > APL_BUF_LEN) {
            rx_len = APL_BUF_LEN;
        }
        
        /* receive */
        while (apl_buf_rxlen < rx_len) {
            ercd = rcv_ssoc(ID_SSL_SOC, apl_buf + apl_buf_rxlen, rx_len - apl_buf_rxlen);
            if (ercd <= 0) {
                return E_OBJ;
            }
            
            apl_buf_rxlen += (UINT)ercd;
        }
        
        /* print buffer */
        shell_dump_buf(ctrl, apl_buf, rx_len, l);
        
        l += rx_len;
        apl_buf_rxlen = 0;
    }
    
    return E_OK;
}

ER apl_ssl_recv_line(void)
{
    char *p;
    ER ercd;
    
    if (apl_buf_rxlen != 0) {
        p = strstr((const char *)apl_buf, "\r\n");
        if (p != NULL) {
            (*p) = '\0';
            return E_OK;
        }
    }
    
    while (apl_buf_rxlen < APL_BUF_LEN) {
        
        ercd = rcv_ssoc(ID_SSL_SOC, apl_buf + apl_buf_rxlen, APL_BUF_LEN - apl_buf_rxlen);
        if (ercd <= 0) {
            return E_OBJ; /* ERROR */
        }
        
        apl_buf_rxlen += (UH)(ercd);
        p = strstr((const char *)apl_buf, "\r\n");
        if (p != NULL) {
            (*p) = '\0';
            return E_OK;
        }
    }
    
    return E_OBJ;
}

UINT apl_ssl_parse_content_len(void)
{
    char *p;
    char str[16];
    UINT i;
    
    p = apl_buf + (sizeof("Content-Length:") - 1);
    
    while (*p == ' ') {
        p++;
    }
    
    i = 0;
    while (isdigit(*p)) {
        if (i < (sizeof(str) - 1)) {
            str[i] = *p;
            i++;
        }
        p++;
    }
    
    str[i] = '\0';
    
    return (UINT)atoi(str);
}

#endif /* #if SAMPLE_ENA_SSL */

