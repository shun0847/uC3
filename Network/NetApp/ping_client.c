/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    Ping Client
    Copyright (c)  2012-2022, eForce Co., Ltd. All rights reserved.

    Version Information
      2012.06.07: Created for new configurator
      2012.10.02  Modify to avoid use of string libraries.
      2013.01.21  Update for IPv6
      2013.07.10  Change IPv6 minimum packet length
      2014.04.04: Remove line of "#include <stdlib.h>".
      2015.07.10: When devnum of parameter is 0 does not use default device
      2015.12.14: The socket ID replaced SID types
      2016.07.06: Execute static analysis tool to this source.
                  Modified to continue to receive and ignore the bad packets.
      2016.08.26: Update for H/W OS
      2017.07.27: Support 64bit processor
      2022.06.30: Fixed an issue with clear_rcvq () entering an endless loop.
 ***************************************************************************/

#include "kernel.h"
#include "net_hdr.h"
#include "net_def.h"
#include "ping_client.h"

#define ICMP_LEN_PINGHDR    8U

static ER clear_rcvq(SID sid, UB *buf, UH len)
{
    ER ercd = E_OK;

    (void)cfg_soc(sid, SOC_TMO_RCV, (VP)0);
    do {
        ercd = rcv_soc(sid, buf, len);
    } while (0 < ercd);    
    return E_OK;
}

#ifdef IPV6_SUP
static ER ping6_rcv(T_PING_CLIENT_V6 *pc, UB *buf)
{
    T_ICMPV6_HDR *icmp6hdr;
    ER ercd;
    TMO tmo_soc;
    SYSTIM tm1;
    SYSTIM tm2;
    UH id;
    
    /* get send information */
    icmp6hdr = (T_ICMPV6_HDR *)(VP)buf;
    id = icmp6hdr->msg_data.echo.id;
    
    (void)ref_soc(pc->sid, SOC_TMO_RCV, (VP)&tmo_soc);
    (void)get_tim(&tm1);
    
    while (1) {
        (void)get_tim(&tm2);
#ifdef NET_HW_OS
        tm2 = tm2 - tm1;
        if ((SYSTIM)tmo_soc < tm2) {
            tm2 = (SYSTIM)tmo_soc;
        }
        (void)cfg_soc(pc->sid, SOC_TMO_RCV, (VP)((SYSTIM)tmo_soc - tm2));
#else
        tm2.ltime = tm2.ltime - tm1.ltime;
        if ((UW)tmo_soc < tm2.ltime) {
            tm2.ltime = (UW)tmo_soc;
        }
        (void)cfg_soc(pc->sid, SOC_TMO_RCV, (VP)((ADDR)(tmo_soc - tm2.ltime)));
#endif

        /* Wait for ICMP Echo Reply (add length IP fields) */
        ercd = rcv_soc(pc->sid, (VP)buf, pc->len + IP6_HDR_SZ);
        if (ercd <= 0) {
            ercd = E_TMOUT;
            break;      /* exit ping process */
        }

        /* Reply includes (IP + ICMP Header + Data) */
        icmp6hdr = (T_ICMPV6_HDR *)(buf + IP6_HDR_SZ);
        if (icmp6hdr->msg_type != ICMPV6_TYPE_ECHO_REPLY) {
            continue;
        }
        if (icmp6hdr->msg_data.echo.id != id) {
            continue;
        }
        
        ercd = E_OK;
        break;      /* Reply success */
    }
    
    (void)cfg_soc(pc->sid, SOC_TMO_RCV, (VP)(ADDR)tmo_soc);
    return ercd;
}

ER ping6_client(T_PING_CLIENT_V6 *pc)
{
    T_ICMPV6_HDR *icmp6hdr;
    T_NET_BUF *buf;
    T_NODE host;
    UB *ping_data;
    ER ercd, i;
    UH rlen;
    char ptn;

    if (!pc) {
        return E_PAR;
    }
    if (!pc->ip6addr) {
        return E_PAR;
    }
    if (ip6_addr_type(pc->ip6addr) == IP6_ADDR_UNSPEC) {
        return E_PAR;
    }
    if (pc->devnum > (UH)NET_DEV_MAX) {
        return E_PAR;
    }

    if (pc->tmo == 0) {
        pc->tmo = PING_TIMEOUT;
    }
    if (pc->len > PING6_LEN_MAX) {
        pc->len = PING6_LEN_MAX;
    }
    if (pc->len < PING6_LEN_MIN) {
        pc->len = PING6_LEN_MIN;
    }
    rlen = pc->len;

    /* Create ICMP Socket */
    net_memset(host.ip6a, 0, sizeof(host.ip6a));
    host.num = (UB)pc->devnum;
    host.ver = IP_VER6;
    host.port = 0;          /* Port should 0 for ICMP */
    if (pc->sid == 0) {
        return E_PAR;
    }
    
    pc->len += ICMP_LEN_PINGHDR;    /* add header length */
    do {
        buf = NULL;
        ercd = net_buf_get(&buf, (UW)pc->len + IP6_HDR_SZ, TMO_POL);
        if ((ercd != E_OK) || (!buf)) {
            ercd = E_NOMEM;
            break;      /* exit ping process */
        }
        ping_data = buf->hdr;
        (void)clear_rcvq(pc->sid, ping_data, pc->len);

        /* Set Transmission Timeout             */
        ercd = cfg_soc(pc->sid, SOC_TMO_SND, (VP)(ADDR)pc->tmo);
        if (ercd != E_OK) {
            break;      /* exit ping process */
        }

        /* Set Reception Timeout                */
        ercd = cfg_soc(pc->sid, SOC_TMO_RCV, (VP)(ADDR)pc->tmo);
        if (ercd != E_OK) {
            break;      /* exit ping process */
        }

        /* Set Remote Host IP Address */
        ip6_addr_cpy(host.ip6a, pc->ip6addr);
        ercd = con_soc(pc->sid, &host, SOC_CLI);
        if (ercd != E_OK) {
            break;      /* exit ping process */
        }

        /* Construct Ping Message */
        icmp6hdr = (T_ICMPV6_HDR *)ping_data;
        net_memset(icmp6hdr, 0, sizeof(*icmp6hdr));
        icmp6hdr->msg_type = ICMPV6_TYPE_ECHO_REQ;
        icmp6hdr->msg_data.echo.id = htons(net_rand() & 0xFFFFU);
        
        /* Set Data Pattern */
        ptn = 0U;
        for (i = ICMP_LEN_PINGHDR; i < pc->len; i++) {
            ping_data[i] = ptn++;
        }

        /* Transmit ICMP Echo Request */
        ercd = snd_soc(pc->sid, (VP)ping_data, pc->len);
        if (ercd <= 0) {
            ercd = E_TMOUT;
            break;      /* exit ping process */
        }

        /* Wait for ICMP Echo Reply */
        ercd = ping6_rcv(pc, ping_data);
        if (ercd != E_OK) {
            break;      /* exit ping process */
        }
    } while (0);
    pc->len = rlen;

    if (buf) {
        net_buf_ret(buf);
    }
    
    return ercd;
}
#endif


static ER ping_rcv(T_PING_CLIENT *pc, UB *buf)
{
    T_ICMP_HDR *icmphdr;
    ER ercd;
    TMO tmo_soc;
    SYSTIM tm1;
    SYSTIM tm2;
    UH id;
    
    /* get send information */
    icmphdr = (T_ICMP_HDR *)(VP)buf;
    id = icmphdr->id;
    
    (void)ref_soc(pc->sid, SOC_TMO_RCV, (VP)&tmo_soc);
    (void)get_tim(&tm1);
    
    while (1) {
        (void)get_tim(&tm2);
#ifdef NET_HW_OS
        tm2 = tm2 - tm1;
        if ((SYSTIM)tmo_soc < tm2) {
            tm2 = (SYSTIM)tmo_soc;
        }
        (void)cfg_soc(pc->sid, SOC_TMO_RCV, (VP)((SYSTIM)tmo_soc - tm2));
#else
        tm2.ltime = tm2.ltime - tm1.ltime;
        if ((UW)tmo_soc < tm2.ltime) {
            tm2.ltime = (UW)tmo_soc;
        }
        (void)cfg_soc(pc->sid, SOC_TMO_RCV, (VP)((ADDR)(tmo_soc - tm2.ltime)));
#endif
        
        /* Wait for ICMP Echo Reply (add length IP fields) */
        ercd = rcv_soc(pc->sid, (VP)buf, pc->len + IP4_HDR_SZ);
        if (ercd <= 0) {
            ercd = E_TMOUT;
            break;      /* exit ping process */
        }

        /* Reply includes (IP + ICMP Header + Data) */
        icmphdr = (T_ICMP_HDR *)(buf + IP4_HDR_SZ);
        if (icmphdr->type != ICMP_ECHO_REPLY) {
            continue;
        }
        if (icmphdr->id != id) {
            continue;
        }
        
        ercd = E_OK;
        break;      /* Reply success */
    }
    
    (void)cfg_soc(pc->sid, SOC_TMO_RCV, (VP)(ADDR)tmo_soc);
    return ercd;
}

ER ping_client(T_PING_CLIENT *pc)
{
    T_ICMP_HDR *icmphdr;
    T_NET_BUF *buf;
    T_NODE host;
    UB *ping_data;
    ER ercd;
    UH i;
    UH rlen;
    UB ptn;

    ercd = E_OK;
    if (!pc) {
        ercd = E_PAR;
    }
    else if (pc->ipa == 0U) {
        ercd = E_PAR;
    }
    else if (pc->devnum > (UH)NET_DEV_MAX) {
        ercd = E_PAR;
    }
    else if ((pc->sid == 0U) || (pc->sid > (SID)NET_SOC_MAX)) {
        ercd = E_PAR;
    }
    if (E_OK != ercd) {
        return ercd;
    }
    
    if (pc->tmo == 0) {
        pc->tmo = PING_TIMEOUT;
    }
    if (pc->len > PING_LEN_MAX) {
        pc->len = PING_LEN_MAX;
    }
    if (pc->len < PING_LEN_MIN) {
        pc->len = PING_LEN_MIN;
    }
    rlen = pc->len;

    /* Create ICMP Socket */
    net_memset(&host, 0, sizeof(host));
    host.num = (UB)pc->devnum;
    host.ver = IP_VER4;  /* this API is IPv4 only*/
    host.ipa = INADDR_ANY;
    host.port = PORT_ANY;/* Port should 0 for ICMP */

    pc->len += ICMP_LEN_PINGHDR;    /* add header length */
    do {
        buf = NULL;
        ercd = net_buf_get(&buf, (UW)pc->len + IP4_HDR_SZ, TMO_POL);
        if ((ercd != E_OK) || (!buf)) {
            ercd = E_NOMEM;
            break;      /* exit ping process */
        }
        ping_data = buf->hdr;
        (void)clear_rcvq(pc->sid, ping_data, pc->len);

        /* Set Reception Timeout                */
        ercd = cfg_soc(pc->sid, SOC_TMO_RCV, (VP)(ADDR)pc->tmo);
        if (ercd != E_OK) {
            break;      /* exit ping process */
        }

        /* Set Remote Host IP Address */
        host.ipa = pc->ipa;
        ercd = con_soc(pc->sid, &host, SOC_CLI);
        if (ercd != E_OK) {
            break;      /* exit ping process */
        }

        /* Construct Ping Message */
        icmphdr = (T_ICMP_HDR *)(VP)ping_data;
        net_memset(icmphdr, 0, sizeof(*icmphdr));
        icmphdr->type = ICMP_ECHO_REQUEST;
        icmphdr->id = htons(net_rand() & 0xFFFFU);

        /* Set Data Pattern */
        ptn = 0U;
        for (i = ICMP_LEN_PINGHDR; i < pc->len; i++) {
            ping_data[i] = ptn++;
        }

        /* Transmit ICMP Echo Request */
        ercd = snd_soc(pc->sid, (VP)ping_data, pc->len);
        if (ercd <= 0) {
            ercd = E_TMOUT;
            break;      /* exit ping process */
        }
        
        /* Wait for ICMP Echo Reply */
        ercd = ping_rcv(pc, ping_data);
        if (ercd != E_OK) {
            break;      /* exit ping process */
        }
    } while (0);
    pc->len = rlen;

    if (0 != buf) {
        net_buf_ret(buf);
    }

    return ercd;
}
