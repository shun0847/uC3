/**************************************************************************
* CGI Sample Program
**************************************************************************/

#include "kernel.h"
#include "sample_netapp_cfg.h"

#if SAMPLE_ENA_HTTPd

#include "http_server.h"
#include "dns_client.h"
#include "sntp_client.h"
#include "ping_client.h"

#ifndef NET_C
extern SID ID_ICMP;
#ifdef IPV6_SUP
extern SID ID_ICMP6;
#endif
extern SID ID_SOC_SNTPC;
extern SID ID_SOC_DNS;
extern SID ID_SOC_HTTP1;
extern ID  ID_CONTENTS_MPF;
#endif

#define CONTENTS_SIZE  256 /* response contents size */

TMO LedTmo = 1000;
static char tmp[16];

/*******************************
  CGI Script
 ******************************/
static void itoa_std(UW num, char* str)
{
    char c, *p, *q;

    p = q = str;

    /* Convert to ascii */
    do {
        c = num%10;
        *p++ = '0'+ c;
        num = num/10;
    } while(num);
    *p-- = '\0';
    
    /* Reverse the string */
    do {
       c = *p;
      *p = *q;
      *q = c;
      p--; q++;
    } while (q < p);
}

static ER get_contents_buf(char**buf)
{
    ER ercd;
    *buf = (char*)0;
    ercd = tget_mpf(ID_CONTENTS_MPF, (VP*)buf, TMO_POL);
    if (ercd == E_OK && *buf) {
        net_memset(*buf, 0, CONTENTS_SIZE);
    }
    return ercd;
}

static void ret_contents_buf(char *buf)
{
    if (!buf) {
        return;
    }
    rel_mpf(ID_CONTENTS_MPF, buf);
    return;
}

ER led_blink(T_HTTP_SERVER *http, int cnt, char *name[], char *value[])
{
    char *contents;
    ER ercd;
    int i;
    for (i = 0; i < cnt; i++) {
        if (net_strcmp(name[i], "led") == 0) {
            LedTmo = net_atoi(value[i]) * 100;
            if (LedTmo < 0) {
                LedTmo = 0;
            }
            break;
        }
    }
    if (i == cnt) {
        return E_OBJ;
    }
    ercd = get_contents_buf(&contents);
    if (ercd != E_OK) {
        return ercd;
    }

    net_strcat(contents, "<html><body><center>Set LED interval ");
    net_strcat(contents, value[i]);
    net_strcat(contents, "<p><a href=/>[Return]<a></center></body></html>");

    HttpSendText(http, contents, net_strlen(contents));
    ret_contents_buf(contents);
    return ercd;
}

static UB get_bind_devnum(T_HTTP_SERVER *http)
{
    T_NODE node = {0};
    
    (void)ref_soc(http->SocketID, SOC_IP_LOCAL, (VP)&node);
    
    return node.num;
}


ER ping_send(T_HTTP_SERVER *http, int cnt, char *name[], char *value[])
{
    T_PING_CLIENT ping = {0};
    char *contents;
    ER ercd;
    int i, j;

    for (i = 0, j = 0; i < cnt; i++) {
        if (net_strcmp(name[i], "remote") == 0) {
            ping.ipa = ip_aton(value[i]);
            j = i;
        }
        if (net_strcmp(name[i], "timeout") == 0) {
            ping.tmo = net_atoi(value[i]);
        }
    }
    ercd = get_contents_buf(&contents);
    if (ercd != E_OK) {
        return ercd;
    }

    ping.sid = ID_ICMP;
    ping.devnum = get_bind_devnum(http);
    ping.tmo = PING_TIMEOUT;
    ercd = ping_client(&ping);

    net_strcat(contents, "<html><body><center>");
    if (ercd == E_OK) {
        net_strcat(contents, "Success reply from ");
    } else {
        net_strcat(contents, "No response from ");
    }
    net_strcat(contents, value[j]);
    net_strcat(contents, "<p><a href=/>[Return]<a></center></body></html>");

    HttpSendText(http, contents, net_strlen(contents));
    ret_contents_buf(contents);
    return E_OK;
}

#ifdef IPV6_SUP

void convURIesc(char *uri)
{
    char *w, c, t;
    w = uri;
    while (*uri) {
        c = *(uri++);
        if (c == '%') {
            t = *(uri++);
            if ('0' <= t && t <= '9') t -= 48;
            else if ('a' <= t && t <= 'f') t -= 87;
            else if ('A' <= t && t <= 'F') t -= 55;
            c = t << 4;

            t = *(uri++);
            if ('0' <= t && t <= '9') t -= 48;
            else if ('a' <= t && t <= 'f') t -= 87;
            else if ('A' <= t && t <= 'F') t -= 55;
            c += t;
        }
        *w++ = c;
    }
    *w = '\0';
}

ER ping6_send(T_HTTP_SERVER *http, int cnt, char *name[], char *value[])
{
    T_PING_CLIENT_V6 ping = {0};
    UW ip6addr[4];

    char *contents;
    ER ercd;
    int i, j;

    for (i = 0, j = 0; i < cnt; i++) {
        if (net_strcmp(name[i], "remote6") == 0) {
            convURIesc(value[i]);
            ip6_aton(value[i], ip6addr);
            ping.ip6addr = ip6addr;
            j = i;
        }
        if (net_strcmp(name[i], "timeout6") == 0) {
            ping.tmo = net_atoi(value[i]);
        }
    }
    ercd = get_contents_buf(&contents);
    if (ercd != E_OK) {
        return ercd;
    }

    ping.sid = ID_ICMP6;
    ping.devnum = get_bind_devnum(http);
    ping.tmo = PING_TIMEOUT;
    ercd = ping6_client(&ping);

    net_strcat(contents, "<html><body><center>");
    if (ercd == E_OK) {
        net_strcat(contents, "Success reply from ");
    } else {
        net_strcat(contents, "No response from ");
    }
    net_strcat(contents, value[j]);
    net_strcat(contents, "<p><a href=/>[Return]<a></center></body></html>");

    HttpSendText(http, contents, net_strlen(contents));
    ret_contents_buf(contents);
    return E_OK;
}
#endif

ER sntp_send(T_HTTP_SERVER *http, int cnt, char *name[], char *value[])
{
    T_SNTP_CLIENT sntp;
    char *contents;
    UW time[2];
    ER ercd;
    int i;

    net_memset(&sntp, 0, sizeof(sntp));
    net_memset(time, 0, sizeof(time));

    for (i = 0; i < cnt; i++) {
        if (net_strcmp(name[i], "sntp") == 0) {
            sntp.ipa = ip_aton(value[i]);
            break;
        }
    }
    if (i == cnt) {
        return E_OBJ;
    }
    ercd = get_contents_buf(&contents);
    if (ercd != E_OK) {
        return ercd;
    }

    sntp.sid = ID_SOC_SNTPC;
    sntp.devnum = get_bind_devnum(http);
    ercd = sntp_client(&sntp, &time[0], &time[1]);

    net_strcat(contents, "<html><body><center>");
    if (ercd == E_OK) {
        itoa_std((time[0]-NTP_BASE_TIME), tmp);
        net_strcat(contents, "<script type=\"text/javascript\">");
        net_strcat(contents, "var d=new Date(");
        net_strcat(contents, tmp);
        net_strcat(contents, "*1000);");
        net_strcat(contents, "document.write(d.toString());</script>");
    } else {
        net_strcat(contents, "No response from ");
        net_strcat(contents, value[i]);
    }
    net_strcat(contents, "<p><a href=/>[Return]<a></center></body></html>");

    HttpSendText(http, contents, net_strlen(contents));
    ret_contents_buf(contents);
    return E_OK;
}

ER dns_resolve(T_HTTP_SERVER *http, int cnt, char *name[], char *value[])
{
    T_DNS_CLIENT dc = {0};
    char *contents;
    ER ercd;
    UW ip,dns;
    int i,j;

    for (i = 0, j = 0, dns = 0; i < cnt; i++) {
        if (net_strcmp(name[i], "dns") == 0) {
            dns = ip_aton(value[i]);
        }
        if (net_strcmp(name[i], "fqdn") == 0) {
            j = i;
        }
    }
    ercd = get_contents_buf(&contents);
    if (ercd != E_OK) {
        return ercd;
    }

    dc.code     = RR_TYPE_A;
    dc.name     = value[j];
    dc.ipaddr   = &ip;
    dc.ipa      = dns;
    dc.sid      = ID_SOC_DNS;
    dc.dev_num  = get_bind_devnum(http);
    dc.retry_cnt = 0;
    ercd = dns_query_ext(&dc);
    net_strcat(contents, "<html><body><center>");
    if (ercd == E_OK) {
        ip_ntoa(tmp, ip);
        net_strcat(contents, value[j]);
        net_strcat(contents, "<p>");
        net_strcat(contents, tmp);
    } else {
        net_strcat(contents, "Can not resolve ");
        net_strcat(contents, value[j]);
    }
    net_strcat(contents, "<p><a href=/>[Return]<a></center></body></html>");

    HttpSendText(http, contents, net_strlen(contents));
    ret_contents_buf(contents);
    return E_OK;
}

void sample_fnc(T_HTTP_SERVER *http)
{
    int cnt, formcnt = 0;
    ER ercd;
    char *formname[10];
    char *formvalue[10];

    /* POST request */
    if (http->hdr.Content) {
        formcnt = sizeof(formname)/sizeof(formname[0]);
        CgiGetParamN(http->hdr.Content, (INT)http->hdr.ContentLen, formname, formvalue, &formcnt);
    }

    ercd = E_OBJ;
    for (cnt = 0; cnt < formcnt; cnt++) {
        if (net_strcmp(formname[cnt], "btn")) {
            continue;
        }
        if (net_strcmp(formvalue[cnt], "LED") == 0) {
            ercd = led_blink(http, formcnt, formname, formvalue);
        } else if (net_strcmp(formvalue[cnt], "PING") == 0) {
            ercd = ping_send(http, formcnt, formname, formvalue);
#ifdef IPV6_SUP
        } else if (net_strcmp(formvalue[cnt], "PING6") == 0) {
            ercd = ping6_send(http, formcnt, formname, formvalue);
#endif
        } else if (net_strcmp(formvalue[cnt], "SNTP") == 0) {
            ercd = sntp_send(http, formcnt, formname, formvalue);
        } else if (net_strcmp(formvalue[cnt], "DNS") == 0) {
            ercd = dns_resolve(http, formcnt, formname, formvalue);
        }
        break;
    }

    if (ercd != E_OK) {
        HttpSendErrorResponse(http, "500 Internal Server Error\r\n");
    }
}

#endif  /* #if SAMPLE_ENA_HTTPd */
