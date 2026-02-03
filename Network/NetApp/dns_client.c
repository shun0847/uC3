/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    DNS Resolver
    Copyright (c)  2009-2025, eForce Co., Ltd. All rights reserved.

    Version Information
      2009.06.03: Created
      2010.09.30: Corrected dns_query() return code
      2010.11.02: Support IPv6
      2012.03.30: Set to default value dns_transmit() var
      2012.06.26: Check malformed reception
      2012.10.02  Modify to avoid use of string libraries.
      2013.02.19: Delete unnecessary operator in parsing
      2014.09.02: Discard received packet before issuing query
      2015.12.14: The socket ID replaced SID types
      2016.07.06: Execute static analysis tool to this source.
      2018.08.16: Support retry for send request.
      2018.10.25: Delete UDP socket receive start/stop controls.
      2021.04.01: Fixed code that could cause a buffer overflow.
      2021.06.28: Changed net_bufs used when using the API from 2 to 1.       
      2025.08.13: Suppressed warning(maybe-uninitialized) by aarch64 GCC.
 ***************************************************************************/

#include "kernel.h"
#include "net_hdr.h"
#include "net_strlib.h"
#include "dns_client.h"

#define HDR_FLG_QR_R        0x8000U     /* message is a response */
#define HDR_FLG_QR_Q        0x0000U     /* message is a query */
#define HDR_FLG_OP_QUERY    0x0000U     /* opcode query */
#define HDR_FLG_TC          0x0200U     /* TrunCation */
#define HDR_FLG_RD          0x0100U     /* Recursion Desired */
#define HDR_FLG_RCODE       0x000FU     /* Response code (mask) */

#define HDR_FLG_TC_MASK     0x0200U     /* TrunCation mask */
#define HDR_FLG_OP_MASK     0x7800U     /* opcode mask */

#define MSG_COMP_BIT        0xC0U       /* domain message compression */
#define MSG_COMP_LEN        2           /* message compression field length */

#define TYPE_LEN_A          4U          /* A record length */
#define TYPE_LEN_AAAA       16U         /* AAAA record length */

/* macro */
#define DNS_DEBUG(x)
#define ALIGN_ARRAYSZ(i,j)      ((i)*(((j)+(8-1))&(~(8-1))))

     
static void SetQueryIp(char *s, UW ipaddr)
{
    INT i, j;
    UB a, b[3];
    char *str;

    str = s;
    for (i = 0; i < (INT)sizeof(UW); i++) {
        a = (ipaddr >> ((UW)i * 8U)) & 0xFFU;
        if (a == 0U) {
            *str++ = '0';
        }
        else {
            for (j = 0; j < (INT)(sizeof(b) / sizeof(*b)); j++) {
                b[j] = (a % 10U) + '0';
                a /= 10U;
                if (a == 0U) {
                    break;
                }
            }
            for (; j>= 0; j--) {
                *str++ = (char)b[j];
            }
        }
        *str++ = '.';
    }
    *--str = '\0';
}

/*
    Convert labels to string 
*/
static ER parse_to_name(char *name, const UB *label, const UB *end)
{
    ER ercd;
    UH len;
    UH str_len;

    *name  = '\0';
    str_len = 0U;
    ercd = E_OK;
    while (1) {
        len = *label++;
        if (len > LABEL_MAX_LEN) {
            ercd =  E_PAR;
            break;
        }
        else if (len == 0U) {
            if (0U < str_len) {
                name--;    /* avoid display the root . */
            }
            *name = '\0';
            break;
        }
        else if ((label + len) > end) {     /* invalid buffer */
            ercd = E_OBJ;
            break;
        }

        str_len += len + 1;
        if (str_len >= DNAME_MAX_LEN) {
            ercd = E_PAR;
            break;
        }

        while (0U < len) {
            *name++ = (char)*label++;
            len--;
        }
        *name = '.';
        name++;
    }
    if (E_OK == ercd) {
        ercd = (ER)str_len;
    }

    return ercd;
}

/*
    Convert string to labels
*/
static ER parse_to_label(char *name, UB *label)
{
    ER ercd;
    UB *str;
    UH len;
    UB tok_cnt;

    str = (UB*)name;

    tok_cnt = 0U;
    len     = 0U;
    label++;    /* Reserve for label length */

    ercd = E_OK;
    while (1) {
    
        /* End Of Lable or String? */
        if ((*str == '\0') || (*str == '.')) {

            /* Length of the parsed Label */
            if (0U < tok_cnt) {
                if (tok_cnt > LABEL_MAX_LEN) {
                    ercd = E_PAR;   /* Too long label */
                    break;
                }
                *(label - 1 - (INT)tok_cnt) = tok_cnt;
                label++;    /* Reserve for label length */
                len++;      /* for label length */
                if (len >= DNAME_MAX_LEN) {
                    ercd = E_PAR;           /* Too long name */
                    break;
                }
            }

            /* String end with this label */
            if (*str == '\0') {
                break;
            }

            /* Label cannot be zero length */
            if (tok_cnt == 0U) {
                ercd = E_PAR;   /* two sequential dots in string? */
                break;
            }

            tok_cnt = 0U;
        }
        else {
            /* Valid label: AlphaNumeric or - */
            if (!(((*str >= '0') && (*str <= '9')) ||
                  ((*str >= 'a') && (*str <= 'z')) ||
                  ((*str >= 'A') && (*str <= 'Z')) || (*str == '-'))) {
                ercd = E_PAR;
                break;
            }

            *label = *str;
            label++;
            tok_cnt++;
            len++;
            if (len >= DNAME_MAX_LEN) {
                ercd = E_PAR;           /* Too long name */
                break;
            }
        }

        str++;
    }

    if (E_OK == ercd) {
        if (len == 0U){
            ercd = E_PAR;
        }
        else {
            *(label - 1) = '\0';     /* End of label */
            ercd = (ER)len + 1;

        }
    }

    return ercd;
}

static ER dns_snd(T_DNS_CLIENT *dc, char *query, UH len)
{
    T_NODE host;
    ER ercd;

    do {
        /* Discard received packet */
        ercd = cls_soc(dc->sid, 0U);
        if (ercd != E_OK)   break;      /* exit dns process */
        
        /* Anti cash poisoning */
        (void)cfg_soc(dc->sid, SOC_PRT_LOCAL, (VP)PORT_ANY);

        /* Set DNS Server IP Address */
        host.ipa  = dc->ipa;
        host.port = DNS_SERVER_PORT;
        host.ver  = IP_VER4;
        host.num  = dc->dev_num;
        ercd = con_soc(dc->sid, &host, SOC_CLI);
        if (ercd != E_OK)   break;      /* exit dns process */
        
        /* Send DNS Query Message */
        ercd = snd_soc(dc->sid, (VP)query, len);
    } while (0);
    
    return ercd;
}


static ER dns_rcv(T_DNS_CLIENT *dc, char *response, UH tx_id)
{
    T_DNS_HDR *dns_hdr;
    TMO tmo_soc;
    SYSTIM tm1;
    SYSTIM tm2;
    ER ercd;
    
    (void)ref_soc(dc->sid, SOC_TMO_RCV, (VP)&tmo_soc);
    (void)get_tim(&tm1);
    
    while (1) {
        (void)get_tim(&tm2);
#ifdef NET_HW_OS
        tm2 = tm2 - tm1;
        if ((SYSTIM)tmo_soc < tm2) {
            tm2 = (SYSTIM)tmo_soc;
        }
        (void)cfg_soc(dc->sid, SOC_TMO_RCV, (VP)((SYSTIM)tmo_soc - tm2));
#else
        tm2.ltime = tm2.ltime - tm1.ltime;
        if ((UW)tmo_soc < tm2.ltime) {
            tm2.ltime = (UW)tmo_soc;
        }
        (void)cfg_soc(dc->sid, SOC_TMO_RCV, (VP)((ADDR)(tmo_soc - tm2.ltime)));
#endif

        ercd = rcv_soc(dc->sid, (VP)response, DNS_MSG_MAX_LEN);
        if (ercd <= 0) {
            break;      /* exit dns process */
        }
        
        /* Discard if response message is small */
        if (ercd < DNS_HDR_SZ) {
            DNS_DEBUG(printf("\r\n dns_response too small"));
            continue;   /* Ignore invalid message */
        }

        dns_hdr = (VP)response;
        /* Discard if id does not match */
        if (dns_hdr->id != tx_id) {
            DNS_DEBUG(printf("\r\n dns_hdr id %d %d", dns_hdr->id, tx_id));
            continue;   /* Ignore invalid message */
        }

        break;
    }
    (void)cfg_soc(dc->sid, SOC_TMO_RCV, (VP)(ADDR)tmo_soc);    
    
    return ercd;
}


static ER dns_transmit(char *query, UH len, char *response, T_DNS_CLIENT *dc)
{
    ER ercd;
    UB rcnt, cnt;
    
    rcnt = (0 == dc->retry_cnt) ? 1 : dc->retry_cnt ;
    ercd = E_PAR;
    for (cnt = 0; cnt < rcnt; cnt++) {
        ercd = dns_snd(dc, query, len);
        if (ercd <= 0) {
            break;
        }
        ercd = dns_rcv(dc, response, ((T_DNS_HDR *)query)->id);
        if (ercd > 0) {
            break;
        }
    }

    return ercd;
}

/*
    DNS Resolver
*/
ER dns_query_ext(T_DNS_CLIENT *dc)
{
    T_DNS_HDR *dns_hdr;
    T_DNS_RR  rrd, *rr;
    UB *dns_tx_msg, *dns_rx_msg;
    UB *msg;
    UH msg_len, rcode, tx_id;
    ER ercd;
    UB *ptr, *end;
    T_NET_BUF *pkt[2];

    /* Check parameters */
    ercd = E_OK;
    if (!dc) {
        ercd = E_PAR;
    }
    else if ((!dc->name) || (!dc->ipaddr) || (!dc->ipa)) {
        ercd = E_PAR;
    }
    else if ((dc->sid == 0U) || (dc->sid > (SID)NET_SOC_MAX)) {
        ercd = E_PAR;
    }
    else {
        switch (dc->code) {
        case RR_TYPE_PTR:
            if (*dc->ipaddr == 0U) {
                ercd = E_PAR;
            }
            break;
            
        case RR_TYPE_A:
#ifdef IPV6_SUP
        case RR_TYPE_AAAA:
#endif                 
            if (net_strcmp(dc->name, "") == 0) {
                ercd = E_PAR;
            }
            break;
                 
        default:
            ercd = E_NOSPT;     /* not support RR types */
            break;
        }
    }
    if (ercd != E_OK) {
        return ercd;
    }
    

    pkt[0] = pkt[1] = NULL;
    do {
        /* Allocate net_buf to DNS message buffers. */
        msg_len = ALIGN_ARRAYSZ(2, DNS_MSG_MAX_LEN) + ALIGN_ARRAYSZ(1, sizeof(T_NET_BUF)-2);
        ercd = net_buf_get(&pkt[0], (UW)msg_len, TMO_POL);
        if (ercd != E_OK) {     /* If one net_buf does not have the required size */
            msg_len = DNS_MSG_MAX_LEN + ALIGN_ARRAYSZ(1, sizeof(T_NET_BUF)-2);
            ercd = net_buf_get(&pkt[0], (UW)msg_len, TMO_POL);
            if (ercd != E_OK) {
                ercd = E_NOMEM;
                break;      /* exit dns process */
            }

            ercd = net_buf_get(&pkt[1], (UW)msg_len, TMO_POL);
            if (ercd != E_OK) {
                ercd = E_NOMEM;
                break;      /* exit dns process */
            }
        }

        dns_tx_msg = pkt[0]->hdr;
        if (pkt[1] == NULL) {
            dns_rx_msg = dns_tx_msg + DNS_MSG_MAX_LEN;
        }
        else {
            dns_rx_msg = pkt[1]->hdr;
        }
        tx_id = net_rand() & 0x0000FFFFU;

        /* Construct DNS Query Message */
        dns_hdr             =   (T_DNS_HDR *)dns_tx_msg;
        dns_hdr->id         =   htons(tx_id);
        dns_hdr->flag       =   htons(HDR_FLG_QR_Q | HDR_FLG_OP_QUERY | HDR_FLG_RD);
        dns_hdr->qdcount    =   htons(1U);
        dns_hdr->ancount    =   0U;
        dns_hdr->nscount    =   0U;
        dns_hdr->arcount    =   0U;
        msg_len             =   DNS_HDR_SZ;

        /* Query RR */
        msg = dns_tx_msg + msg_len;
        rr  = &rrd;

        if (dc->code == RR_TYPE_PTR) {
            /*sprintf(name,"%d.%d.%d.%d.IN-ADDR.ARPA.", ptr[0], ptr[1], ptr[2], ptr[3]);*/
            SetQueryIp(dc->name, *dc->ipaddr);  /* set IP in Reverse order */
            net_strcat(dc->name,".IN-ADDR.ARPA.");
        }

        ercd = parse_to_label(dc->name, msg);
        if (ercd <= 0) {
            ercd = E_PAR;
            break;      /* exit dns process */
        }
        msg_len += (UH)ercd;

        rr->type    =   htons(dc->code);
        rr->class   =   htons(RR_CLASS_IN);
        net_memcpy(msg + ercd, rr, sizeof(rr->type) + sizeof(rr->class));
        msg_len += (UH)(sizeof(rr->type) + sizeof(rr->class));

        /* Do socket process for send & recv DNS messages */
        ercd = dns_transmit((char *)dns_tx_msg, msg_len, (char *)dns_rx_msg, dc);
        if (ercd <= 0) {
            if (ercd != E_TMOUT) {
                ercd = E_OBJ;
            }
            break;      /* exit dns process */
        }
        end = dns_rx_msg + ercd;

        /* Process received DNS Response */
        ercd = E_OBJ;

        dns_hdr = (T_DNS_HDR *)dns_rx_msg;
        dns_hdr->flag = ntohs(dns_hdr->flag);

        /* Discard if not a response */
        if (!(dns_hdr->flag & HDR_FLG_QR_R)) {
            DNS_DEBUG(printf("\r\n dns not a response flg %x", dns_hdr->flag));
            break;      /* exit dns process */
        }

        /* Discard if opcode response */
        if (HDR_FLG_OP_QUERY != (dns_hdr->flag & HDR_FLG_OP_MASK)) {
            DNS_DEBUG(printf("\r\n dns not a response flg %x", dns_hdr->flag));
            break;      /* exit dns process */
        }
        /* Discard if truncated response */
        if (HDR_FLG_TC == (dns_hdr->flag & HDR_FLG_TC_MASK)) {
            DNS_DEBUG(printf("\r\n dns not a response flg %x", dns_hdr->flag));
            break;      /* exit dns process */
        }
        
        /* Discard if error code in response */
        rcode = dns_hdr->flag & HDR_FLG_RCODE;
        if (rcode != 0U) {
            DNS_DEBUG(printf("\r\n rcode %d", rcode));
            break;      /* exit dns process */
        }

        dns_hdr->qdcount = ntohs(dns_hdr->qdcount);
        dns_hdr->ancount = ntohs(dns_hdr->ancount);

        ptr     = dns_rx_msg + DNS_HDR_SZ;

        /* Skip Query RR */
        while (0U < dns_hdr->qdcount) {
            while (1) {
                if (ptr >= end) {
                    goto exit_err_dns_query;    /* exit dns process */
                }
                if (*ptr == 0) {
                    ptr++;
                    break;
                }
                if ((*ptr & MSG_COMP_BIT) == MSG_COMP_BIT) {
                    ptr += MSG_COMP_LEN;
                    break;
                }
                ptr += *ptr + 1;
            }
            ptr += 4;       /* skip type, class */
            dns_hdr->qdcount--;
        }

        /* Search for Answer RR */
        while (0U < dns_hdr->ancount) {
            while (1) {
                if (ptr >= end) {
                    goto exit_err_dns_query;    /* exit dns process */
                }
                if (*ptr == 0) {
                    ptr++;
                    break;
                }
                if ((*ptr & MSG_COMP_BIT) == MSG_COMP_BIT) {
                    ptr += MSG_COMP_LEN;
                    break;
                }
                ptr += *ptr + 1;
            }

            if ((ptr + DNS_RR_SZ) >= end) {
                goto exit_err_dns_query;    /* exit dns process */
            }
            net_memcpy((char*)rr, ptr, DNS_RR_SZ);
            ptr += DNS_RR_SZ;
            if ((ntohs(rr->class) == RR_CLASS_IN)
              && (ntohs(rr->type)  == dc->code)) {
                ercd = E_OK;
                break;
            }
            ptr += ntohs(rr->rdlength);
            dns_hdr->ancount--;
        }
        if (ptr >= end) {
            goto exit_err_dns_query;    /* exit dns process */
        }

        if (ercd == E_OK) { /* found Answer RR */
            switch (ntohs(rr->type)) {
                case RR_TYPE_A:
                    if ((ptr + TYPE_LEN_A) > end) {
                        ercd = E_OBJ;
                    }
                    else {
                        net_memcpy((UB*)dc->ipaddr, ptr, TYPE_LEN_A);
                        *dc->ipaddr = htonl(*dc->ipaddr);
                    }
                    break;
                case RR_TYPE_PTR:
                    ercd = parse_to_name(dc->name, ptr, end);
                    if (0 < ercd) {
                        ercd = E_OK;
                    }
                    break;
        #ifdef IPV6_SUP
                case RR_TYPE_AAAA:
                    if ((ptr + TYPE_LEN_AAAA) > end) {
                        ercd = E_OBJ;
                    }
                    else {
                        net_memcpy((UB*)dc->ipaddr, ptr, TYPE_LEN_AAAA);
                        ip6_addr_hton(dc->ipaddr, dc->ipaddr);
                    }
                    break;
        #endif
                default:
                    break;
            }
        }

        if (0) {
exit_err_dns_query:
            ercd = E_OBJ;
        }
    } while (0);

    if (pkt[0] != 0) {
        net_buf_ret(pkt[0]);
    }
    if (pkt[1] != 0) {
        net_buf_ret(pkt[1]);
    }

    return ercd;
}

ER dns_query(UH code, char *name, UW *ipaddr, UW dns_server, SID socid)
{
    T_DNS_CLIENT dc = {0};
    
    dc.code     = code;
    dc.name     = name;
    dc.ipaddr   = ipaddr;
    dc.ipa      = dns_server;
    dc.sid      = socid;
    //dc.dev_num  = 0;
    //dc.retry_cnt = 1;
    
    return dns_query_ext(&dc);
}

/*
    Resolve IPAddress for domain name
*/
ER dns_get_ipaddr(SID socid, UW dns_server, char *name, UW *ipaddr)
{
    return dns_query(RR_TYPE_A, name, ipaddr, dns_server, socid);
}

/*
    Resolve Domain name for IPAddress
*/

ER dns_get_name(SID socid, UW dns_server, char *name, UW *ipaddr)
{
    return dns_query(RR_TYPE_PTR, name, ipaddr, dns_server, socid);
}


