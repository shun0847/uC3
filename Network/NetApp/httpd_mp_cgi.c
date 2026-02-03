/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    HTTP Server extention (Multipart receive CGI)
    Copyright (c)  2016-2024, eForce Co., Ltd. All rights reserved.

    Version Information
      2016.03.30: Created
      2016.07.20: Corrected by definition change of T_HTTP_SERVER structure.
      2017.03.23: Fix parsing of httpd_mp_setup() under specified conditions.
      2017.08.02: Deleted unreachable path.
      2024.04.03: Suppressed warning of the 64bit GCC compiler.
      2024.10.07: HTTPS support.
 ***************************************************************************/

#include "httpd_mp_cgi.h"
#ifdef HTTPD_SSL_SUP
#include "ssl_hdr.h"
#endif

#define MPCGI_LEN_CRLF  2U

/*---------------------------------------------------------------------*/
static const char* net_memchr(const char *mem, char ch, UW len)
{
    if (0 != mem) {
        while (0U < len) {
            if (*mem == (char)ch) {
                break;
            }
            mem++;
            len--;
        }
        if (0U == len) {
            mem = 0;
        }
    }

    return mem;
}

static const char* net_memmem(const char *mem1, const char *mem2, UW len1, UW len2)
{
    const char *ret;
    
    ret = mem1;
    while (1) {
        ret = net_memchr(ret, mem2[0], len1 - (UW)(ret - mem1));
        if (0 == ret) {
            break;
        }
        if ((len1 - (UW)(ret - mem1)) < len2) {
            ret = 0;
            break;
        }
        
        if (net_memcmp(ret, mem2, len2) == 0) {
            break;
        }
        else {
            ++ret;
        }
    }
    
    return ret;
}

/* search string and pointer increment */    
static VB* strstr_inc(const VB *str1, const VB *str2)
{
    VB *tmp;
    
    tmp = net_strstr(str1, str2);
    if (0 != tmp) {
        tmp += net_strlen(str2);
    }
    return tmp;
}

/* terminate string */
static void setterm(VB* str1, const VB *str2)
{
    VB *tmp;
    
    if (0 != str1) {
        tmp = net_strstr(str1, str2);
        if (0 != tmp) {
            *tmp = '\0';
        }
    }
}

/* slide buffer data */
static UW slide_buf(VB *buf, UW sta, UW len)
{
    if (0U != sta) {
        net_memcpy(buf, &buf[sta], len);
    }
    buf[len] = '\0';
    
    return len;
}

/* slide HTTP receive buffer data */
static void http_rbuf_slide(T_HTTP_SERVER *http)
{
    http->rxlen = slide_buf((VB*)http->rbuf, http->rdlen, http->rxlen - http->rdlen);
    http->rdlen = 0U;
}

/* decompose HTTP receive buffer */
static UB* http_rbufsz_dec(T_HTTP_SERVER *http, UW dec)
{
    UB *rbuf_bak = http->rbuf;
    
    http->rbuf = &http->rbuf[dec];
    http->rbufsz -= dec;
    http->rxlen = (UW)((dec > http->rxlen) ? 0U : (http->rxlen - dec));
    http->rdlen = (UW)((dec > http->rdlen) ? 0U : (http->rdlen - dec));
    
    return rbuf_bak;        /* original top position */
}

/* reset decompose HTTP receive buffer */
static UW http_rbufsz_rst(T_HTTP_SERVER *http, UB *rbuf_bak)
{
    UW ofs = (UW)(http->rbuf - rbuf_bak);

    http->rbuf = rbuf_bak;
    http->rbufsz += ofs;
    
    /* data clear */
    http->rdlen = 0U;
    http->rxlen = 0U;

    return ofs;             /* offset of original */
}

/*---------------------------------------------------------------------*/
/* Setup multipart receive CGI. */
ER httpd_mp_setup(T_HTTP_SERVER *http)
{
    ER ercd;
    UW rdsz;
    const VB *mp_s1;
    const VB *mp_s2;
    
    ercd = E_OK;
    if (!http) {
        ercd = E_PAR;
    }
    else if (0U <  http->rxlen) {
        /* Find body part (http->rbuf include multiple '\0'.) */
        mp_s1 = net_memmem((VB*)http->rbuf, "\r\n\r\n", http->rxlen, sizeof("\r\n\r\n") - 1U);
        mp_s2 = net_memmem((VB*)http->rbuf, "\0\n\r\n", http->rxlen, sizeof("\0\n\r\n") - 1U);
        if (0 != mp_s2) {
            if ((0 == mp_s1) || (mp_s2 < mp_s1)) {
                mp_s1 = mp_s2;
            }
        }

        /* Content already in recv buffer */
        if (0 != mp_s1) {
            rdsz = (UW)(mp_s1 - (VB*)http->rbuf) + MPCGI_LEN_CRLF;         /* body top */
            http->rdlen = (rdsz < http->rdlen) ? rdsz : http->rdlen;        /* unread body part */
        
            rdsz = http->rxlen - http->rdlen;       /* recv Contents size */
            http->hdr.ContentLen = http->hdr.ContentLen - rdsz + MPCGI_LEN_CRLF;
        }
    }
    else {
        /* do nothing (no receive contents data) */
    }
    
    return ercd;
}

/* Validate multipart header. */
ER httpd_mp_valid(VB *hdr_ctype, VB *boundary)
{
    ER ercd;
    const VB *sta;
    const VB *end1;
    const VB *end2;
    
    if ((!hdr_ctype) || (!boundary)) {
        ercd = E_PAR;
    }
    else {
        ercd = E_NOSPT;
        sta = net_strstr(hdr_ctype, "multipart/form-data");
        if (0 != sta) {
            /* check include boundary from Content-Type */
            sta = strstr_inc(sta, "boundary=");
            if (0 != sta) {
                /* terminate ('\0' or '\r') */
                end1 = net_memchr(sta, '\0', net_strlen(sta) + 1U);
                end2 = net_strchr(sta, '\r');
                if ((0 != end1) && (0 != end2)) {
                    if (end1 > end2) {
                        end1 = end2;
                    }
                }
                else if (0 == end1) {
                    end1 = end2;
                }
                else {
                    /* do nothing */
                }
                
                if (0 != end1) {
                    net_strncpy(boundary, sta, (UW)(end1 - sta) + 1U);
                    ercd = E_OK;
                }
            }
        }
    }
    
    return ercd;
}

/* structure for mp_find_chk() */
typedef struct t_mp_find_chk {
    VB *str;
    UW len;
    const VB **chk_list;
    UB list_num;
} T_MP_FIND_CHK;

/* find string, or string of part */
static VB* mp_find_chk(T_MP_FIND_CHK *ms, UB *ret)
{
    const VB *end;
    VB *sta;
    VB *tmp;
    UH slen;
    UH ilen;
    
    /* Calculation of find string number. */
    ilen = 0U;
    for (slen = 0U; slen < (UH)ms->list_num; ++slen) {
        ilen += (UH)net_strlen(ms->chk_list[slen]);
    }

    /* Find target string. */
    tmp = ms->str;
    sta = ms->str;
    end = ms->chk_list[0];
    slen = ilen;
    ms->list_num = 0U;
    for (; 0U < ms->len; --ms->len) {
        if (*end == '\0') {
            ms->list_num++;
            end = ms->chk_list[ms->list_num];
        }
        
        if (*tmp == *end) {
            end++;
            slen--;
            if (0U == slen) {
                break;
            }
        }
        else {
            if (end != ms->chk_list[0]) {
                end = ms->chk_list[0];
                ms->list_num = 0U;
                slen = ilen;
                ms->len++;
                tmp--;
            }
            sta = tmp + 1;
        }
        tmp++;
    }
    
    /* Set the find results to the return value */
    if (0 != ret) {
        if (slen != ilen) {
            *ret = (UB)((0U == slen) ? MPBDRY_CHK_FOUND : MPBDRY_CHK_PART);
        }
        else {
            *ret = MPBDRY_CHK_NOFOUND;
        }
    }
    return (slen != ilen) ? sta : 0 ;
}

/* Verify contains header/end of multipart in the buffer. */
VB* httpd_mp_bdry_chk(VB *str, UH len, const VB *boundary, UB *ret)
{
    T_MP_FIND_CHK ms;
    const VB *chk_list[3] = {0};
    VB *p1;
    VB *p2;
    UB r1;
    UB r2;
    
    chk_list[0] = "\r\n--";
    chk_list[1] = boundary;
    chk_list[2] = 0;

    /* parameter check */
    if ((!str) || (!len) || (!boundary)) {
        *ret = MPBDRY_CHK_ERROR;
        return 0;
    }
    else if (!ret) {
        return 0;
    }
    else {
        /* continue following process */
    }
    
    *ret = 0U;
    /* part     "\r\n--{boundary}" */
    ms.str = str;
    ms.len = len;
    ms.chk_list = chk_list;
    ms.list_num = (UB)(sizeof(chk_list)/sizeof(*chk_list)) - 1U;
    p1 = mp_find_chk(&ms, ret);
    if (MPBDRY_CHK_FOUND != *ret) {
        str = p1;
        return str;
    }
    
    /* Update find ptr */
    len -= (UH)(p1 - str);
    str = p1;
    if (0U == len) {     /* no data */
        return str;
    }
    
    /* header   "\r\n--{boundary}\r\n" */
    chk_list[2] = "\r\n";
    ms.str = str;
    ms.len = len;
    ms.list_num = sizeof(chk_list)/sizeof(*chk_list);
    p1 = mp_find_chk(&ms, &r1);
    if (MPBDRY_CHK_FOUND == r1) {
        *ret = MPBDRY_POS_HEADER | r1;
        str = p1;
        return str;
    }
    
    /* end      "\r\n--{boundary}--\r\n"*/
    chk_list[2] = "--\r\n";
    ms.str = str;
    ms.len = len;
    ms.list_num = sizeof(chk_list)/sizeof(*chk_list);
    p2 = mp_find_chk(&ms, &r2);
    if (MPBDRY_CHK_FOUND == r2) {
        *ret = MPBDRY_POS_END | r2;
        str = p2;
        return str;
    }
    
    /* judge */
    if (0 == p1) {
        if (0 == p2) {
            *ret = MPBDRY_CHK_NOFOUND;
            str = 0;
        }
        else {
            *ret = MPBDRY_POS_END | r2;
            str = p2;
        }
    }
    else {
        if (0 == p2) {
            *ret = MPBDRY_POS_HEADER | r1;
            str = p1;
        }
        else if (p1 <= p2) {
            *ret = MPBDRY_POS_HEADER | r1;
            str = p1;
        }
        else {
            *ret = MPBDRY_POS_END | r2;
            str = p2;
        }
    }
    return str;
}


/* Receive processing of content */
ER httpd_mp_content_rcv(T_HTTP_SERVER *http)
{
    UW rdsz;
    ER ercd;
    ER (*rcv)(SID, VP, UH);
    
    rcv = rcv_soc;
#ifdef HTTPD_SSL_SUP
    if (0 != (http->flag & HTTPD_FLG_SSL)) {
        rcv = rcv_ssoc;
    }  
#endif    
    if (!http) {
        return E_PAR;
    }
    
    if (0U < http->rdlen) {
        if (http->rdlen >= http->rxlen) {
            http->rdlen = http->rxlen = 0U;
        }
        else {      /* Content already in recv buffer */
            http_rbuf_slide(http);
        }
    }
        
    rdsz = (UW)((http->rbufsz < 0xFFFFU) ? http->rbufsz : 0xFFFFU);
    if (rdsz <= http->rxlen) {
        ercd = E_NOMEM;
    }
    else if (0U < http->hdr.ContentLen) {         /* available receive data */
        rdsz -= http->rxlen;
        ercd = rcv(http->SocketID, &http->rbuf[http->rxlen], (UH)(rdsz - 1U));
        if (0 < ercd) {
            http->rbuf[http->rxlen + (UW)ercd] = '\0';      /* safe seach */
            http->rxlen += (UW)ercd;
            if ((UW)ercd > http->hdr.ContentLen) {
                /* abnormal content size */
                ercd = E_SYS;     /* Provisional */
            }
            else {
                http->hdr.ContentLen -= (UW)ercd;
                ercd = (ER)http->rxlen;
            }
        }
    }
    else {   /* no receive data */
        ercd = (ER)http->rxlen;
    }
    
    return ercd;
    
}

/* Get multipart header information */
ER httpd_mp_hdr_info(T_HTTP_MPHDR *mp_hdr, VB* str, UH len)
{
    ER ercd;
    VB bak;
    
    ercd = E_OK;
    if ((!mp_hdr) || (!str) || (0U == len)) {
        ercd = E_PAR;
    }
    else {
        bak = str[len];
        str[len] = '\0';    /* safe search */
        net_memset(mp_hdr, 0, sizeof(*mp_hdr));
        
        mp_hdr->disp = strstr_inc(str, "Content-Disposition: ");
        if (0 != mp_hdr->disp) {
            mp_hdr->name = strstr_inc(mp_hdr->disp, " name=\"");
            mp_hdr->file = strstr_inc(mp_hdr->disp, " filename=\"");
        }
        mp_hdr->ctype = strstr_inc(str, "Content-Type: ");
        setterm(mp_hdr->disp, ";");
        setterm(mp_hdr->name, "\"");
        setterm(mp_hdr->file, "\"");
        setterm(mp_hdr->ctype, "\r");
        
        str[len] = bak;
    }
    return ercd;
}

/* usercallback setting & call */
#define USER_NOTIFY_CBK(_evt_, _dat_, _len_, _ret_)                    \
    do { \
        hm.http = http;  hm.stat = (UB)(_evt_);  hm.dat = (_dat_);  hm.len = (UH)(_len_);  \
        *(_ret_) = notify_cbk(&hm);   \
    } while (0)
      
    
/* multipart receive CGI (core process) */
ER httpd_mp_cgi(T_HTTP_SERVER *http, const T_HTTP_FILE *fp)
{
    T_HTTP_MPCGI hm = {0};
    VB *boundary;
    VB *temp;
    VB *next;
    ER (*notify_cbk)(T_HTTP_MPCGI*);
    UB *rbuf_bak;
    ER ercd;
    ER cbke;
    UW len;
    UB ret;
    UB stat;
    
    notify_cbk = (ER (*)(T_HTTP_MPCGI*))fp->cbk;
    
    
    USER_NOTIFY_CBK(MPCGI_STAT_START, 0, 0, &cbke);
    if (E_OK != cbke) {
        return cbke;
    }
    
    /* check validate multipart data */
    ercd = httpd_mp_valid(http->hdr.ctype, (VB*)http->rbuf);
    if (E_OK != ercd) {
        USER_NOTIFY_CBK(MPCGI_STAT_END, (VP)(ADDR)ercd, sizeof(ercd), &cbke);
        return ercd;
    }
    
    /* Save boundary */
    boundary = (VB*)http->rbuf;
    rbuf_bak = http_rbufsz_dec(http, net_strlen(boundary) + 1U);
    
    /* Initialize multipart process */
    (void)httpd_mp_setup(http);
    stat = MPCGI_STAT_MPHEAD;
    
    do {
        /* receive contents */
        ercd = httpd_mp_content_rcv(http);
        if (0 > ercd) {
            break;
        }
        
        /* multipart receive step */
        ercd = E_OK;
        switch (stat) {
        case MPCGI_STAT_MPHEAD:         /* multipart header */
            /* find body part from receive buffer */
            next = net_strstr((VB*)http->rbuf, "\r\n\r\n");
            if (0 != next) {
                next[1] = '\0';     /* search safe */
                next += MPCGI_LEN_CRLF;     /* skip '\r\n' (1/2) */
            }
            else {
                /* Cheking terminate */
                temp = httpd_mp_bdry_chk((VB*)http->rbuf, (UH)http->rxlen, boundary, &ret);
                if (ret == (MPBDRY_POS_END | MPBDRY_CHK_FOUND)) {
                    http->hdr.ContentLen = 0U;
                    http->rxlen = 0U;
                    break;
                }
                
                if (0U < http->hdr.ContentLen) {     /* avaiable receive data */
                    continue;
                }
                else {                              /* no receive data */
                    ercd = E_CLS;
                    break;  /* end loop */
                }
            }
            
            temp = httpd_mp_bdry_chk((VB*)http->rbuf, (UH)(next - (VB*)http->rbuf), boundary, &ret);
            if (ret != (MPBDRY_POS_HEADER | MPBDRY_CHK_FOUND)) {
                /* abnormal multipart format */
                break;
            }
            next += MPCGI_LEN_CRLF;     /* skip '\r\n' (2/2) */
            
            USER_NOTIFY_CBK(MPCGI_STAT_MPHEAD, temp, (next - temp), &cbke);
            if (E_OK != cbke) {
                ercd = cbke;
                break;  /* end loop */
            }
            
            /* read buffer count until top body part position */
            http->rdlen = (UW)(next - (VB*)http->rbuf);
            stat = MPCGI_STAT_MPBODY;
            break;
            
        case MPCGI_STAT_MPBODYEND:      /* multipart body (possibility of terminate) */
        case MPCGI_STAT_MPBODY:         /* multipart body */
            /* find next part from receive buffer */
            next = httpd_mp_bdry_chk((VB*)http->rbuf, (UH)http->rxlen, boundary, &ret);
            if (0 != next) {    /* possibility of terminate */
                /* It is processed only body part */
                next[0] = '\0';         /* search safe */
                len = (UH)(next - (VB*)http->rbuf);     /* body part size */
                stat = (UB)((0U != (ret & MPBDRY_CHK_FOUND)) ? MPCGI_STAT_MPBODYEND : MPCGI_STAT_MPBODY );
                USER_NOTIFY_CBK(stat, http->rbuf, len, &cbke);
                if (E_OK != cbke) {
                    ercd = cbke;
                    break;  /* end loop */
                }
                next[0] = '\r';         /* restore data */
                
                /* next step is ... */
                if (0U != (ret & MPBDRY_CHK_FOUND)) {   /* boundary complete */
                    stat = MPCGI_STAT_MPHEAD;        /* next part */
                }
                else {                                  /* boundary possibility */
                    stat = MPCGI_STAT_MPBODYEND;     /* continue this body part */
                }
                http->rdlen = len ;
            }
            else {          /* all buffer data is body part */
                USER_NOTIFY_CBK(MPCGI_STAT_MPBODY, http->rbuf, http->rxlen, &cbke);
                if (E_OK != cbke) {
                    ercd = cbke;
                    break;  /* end loop */
                }
                http->rdlen = http->rxlen;
            }
            break;

        default:    /* invalid branch */
            ercd = E_SYS;
            break;
        }
        if (0 > ercd) {
            break;
        }
    
    } while (0U < ((UW)http->hdr.ContentLen + http->rxlen));
    
    USER_NOTIFY_CBK(MPCGI_STAT_END, (VP)(ADDR)ercd, sizeof(ercd), &cbke);
    
    http_rbufsz_rst(http, rbuf_bak);        /* restore HTTP rbuf */
    
    return ercd;
}

