/***************************************************************************
    MICRO C CUBE / COMPACT, NETWORK
    ICMP Protocol
    Copyright (c)  2008-2022, eForce Co., Ltd. All rights reserved.

    Version Information
      2008.11.19: Created
      2010.01.18: ETH_HDR_SZ to dev->hhdrsz
      2010.08.17: Support SOC_RCV_PKT_INF socket option
      2010.11.02: Updated for IPv6 support
      2011.03.07: Updated for received packet queue
      2011.03.11: Updated for IPv6 multiple interface
      2011.03.30: Updated for TX blocking device
      2011.05.20: Set up networkbuffer offset
      2011.07.29: Corrected rqsz when icmp_rcv() is called
      2012.06.06: ICMP error message MUST NOT be sent as
                  the result of receiving(RFC Requirements)
      2012.10.02:  Modify to avoid use of string libraries.
      2013.04.10:  H/W OS supported version
      2014.03.06: SNMP option was added
      2015.06.18: Change packet size limit for checksum
      2015.12.14: The socket ID replaced SID types
      2016.02.19: Change the size specifying when to get network buffer
      2016.03.09: Devide HW_CS flag into ICMP and TCP/UDP
      2016.04.19: Deleted processing of data length check for icmp_snd.
      2016.06.09: Add the process when specify the MSG_PEEK option.
      2018.08.23: Support set or reset the don't fragment flag in IP header
      2020.05.28: Fixed the null pointer reference in icmp_rcv().
      2021.03.17: Add link of peer option
      2022.04.01: Add ignore ping route in icmp_pkt_rcv()
 ***************************************************************************/

#include <stdio.h>
#include "kernel.h"
#include "net_hdr.h"
#include "net_def.h"
#include "net_sts_id.h"

void icmp_pkt_rcv(T_NET_BUF *pkt)
{
    ER ercd;
    T_NET      *net;
    T_IP4_HDR  *ip4hdr;
    T_ICMP_HDR *icmphdr;
#ifdef PING_SUP
    T_NET_SOC *soc, *icmpsoc;
    SID        sid;
#endif

    net_sts_inc(NET_STS_ICMP, NET_STS_ICMP_IN_MSG);

    if (pkt->dat_len < ICMP_HDR_SZ) {
        net_buf_ret(pkt);
        net_sts_inc(NET_STS_ICMP, NET_STS_ICMP_IN_ERR);
        return;
    }

    if (pkt->flg & HW_CS_RX_ICMP) {
        if (pkt->flg & HW_CS_DATA_ERR) {
            net_buf_ret(pkt);
            net_sts_inc(NET_STS_ICMP, NET_STS_ICMP_IN_ERR);
            return;
        }
    }
    else {
        if (icmp_csum(pkt) != 0) {
            net_buf_ret(pkt);
            net_sts_inc(NET_STS_ICMP, NET_STS_ICMP_IN_ERR);
            return;
        }
    }

    net = pkt->net;
    ip4hdr = (T_IP4_HDR *)pkt->hdr;
    icmphdr = (T_ICMP_HDR*)pkt->dat;

    switch (icmphdr->type)
    {
        case ICMP_ECHO_REQUEST:
            if (PING_IGNORE) {  /* ignore request for ping */
                net_buf_ret(pkt);
                net_sts_inc(NET_STS_ICMP, NET_STS_ICMP_IN_ECHO);
                return;
            }

            icmphdr->type = ICMP_ECHO_REPLY;
            icmphdr->cs   = 0;
            if ((pkt->dat_len <= 1480) && (net->flag & HW_CS_TX_ICMP))
                pkt->flg |= HW_CS_TX_ICMP;
            else
                icmphdr->cs   = icmp_csum(pkt);

            ip4hdr->da  = ip4hdr->sa;
            ip4hdr->sa  = htonl(net->adr->ipaddr);
            ip4hdr->hc  = 0;
            if (net->flag & HW_CS_TX_IPH4)
                pkt->flg |= HW_CS_TX_IPH4;
            else
                ip4hdr->hc = ip4_csum(pkt);

            pkt->hdr_len = pkt->hdr_len + pkt->dat_len;
            pkt->dat = NULL;
            pkt->ercd = pkt->dat_len;
            ercd = net->out(pkt);
            net_sts_inc(NET_STS_ICMP, NET_STS_ICMP_IN_ECHO);
            if ((ercd == E_WBLK) || (ercd == E_OK)) {
                net_sts_inc(NET_STS_ICMP, NET_STS_ICMP_OUT_ECHO_REP);
                net_sts_inc(NET_STS_ICMP, NET_STS_ICMP_OUT_MSG);
            }
            return;
        case ICMP_ECHO_REPLY:       /* Echo Reply */
            net_sts_inc(NET_STS_ICMP, NET_STS_ICMP_IN_ECHO_REP);
            break;
        case ICMP_DST_UNREACH:      /* Destination Unreachable */
            net_sts_inc(NET_STS_ICMP, NET_STS_ICMP_IN_DEST_UNREACH);
            break;
        case ICMP_SRC_QUENC:        /* Source Quench */
            net_sts_inc(NET_STS_ICMP, NET_STS_ICMP_IN_SRC_QUENCH);
            break;
        case ICMP_REDIRECT:         /* Redirect */
            net_sts_inc(NET_STS_ICMP, NET_STS_ICMP_IN_REDIRECT);
            break;
        case ICMP_TIME_EXCEED:      /* Time Exceeded */
            net_sts_inc(NET_STS_ICMP, NET_STS_ICMP_IN_TIME_EXCD);
            break;
        case ICMP_PRM_PROB:         /* Parameter Problem */
            net_sts_inc(NET_STS_ICMP, NET_STS_ICMP_IN_PARM_PROB);
            break;
        case ICMP_TIMES_REQUEST:    /* Timestamp Request        */
            net_sts_inc(NET_STS_ICMP, NET_STS_ICMP_IN_TIME_STAMP);
            break;
        case ICMP_TIMES_REPLY:      /* Timestamp Reply          */
            net_sts_inc(NET_STS_ICMP, NET_STS_ICMP_IN_TIME_STAMP_REP);
            break;
        case ICMP_ADDR_REQUEST:     /* AddressMask Request      */
            net_sts_inc(NET_STS_ICMP, NET_STS_ICMP_IN_ADDR_MASK);
            break;
        case ICMP_ADDR_REPLY:       /* AddressMask Reply        */
            net_sts_inc(NET_STS_ICMP, NET_STS_ICMP_IN_ADDR_MASK_REP);
            break;
        default:
            break;
    }

#ifdef PING_SUP

    /* Deliver to ICMP Listening Socket */
    icmpsoc = NULL;
    for (sid=0;sid<NET_SOC_MAX;sid++) {
        soc = &pNET_SOC[sid];
        if ((soc->state == SOC_CREATED)  &&
            (soc->proto == IP_PROTO_ICMP) &&
            ((soc->net == NULL) || (soc->net == net))) {
                icmpsoc = soc;
                break;
        }
    }

    if (icmpsoc == NULL) {
        net_buf_ret(pkt);
        return;
    }

    /* Drop if receive queue is full */
    if (icmpsoc->rqsz == 0) {
        net_buf_ret(pkt);
        return;
    }
    icmpsoc->rqsz--;
    add2que_end(&icmpsoc->rdatque, pkt);

    /* Deliver data to waiting process */

    if (icmpsoc->wptn & EV_SOC_RCV) {
        soc_event(icmpsoc, EV_SOC_RCV, pkt->hdr_len + pkt->dat_len);
    }

#else
    net_buf_ret(pkt);
#endif

    return;
}

#ifdef PING_SUP

ER icmp_rcv(T_NET_SOC *soc, VP data, UH len)
{
    T_NET_BUF *pkt;
    T_IP4_HDR *ip4hdr = NULL;
#ifdef IPV6_SUP
    T_IP6_HDR *ip6hdr = NULL;
#endif
#ifdef LINK_INF_SUP
    T_NET_DEV *dev;
#endif

    if (len == 0)
        return E_PAR;

    if (soc->state == SOC_UNUSED) {
        return E_OBJ;
    }

    if (soc->rdatque == NULL) {
        return E_WBLK;
    }

    pkt = (T_NET_BUF *)soc->rdatque;
    if (len > (pkt->hdr_len + pkt->dat_len)) {
        len = (pkt->hdr_len + pkt->dat_len);
    }

#ifdef IPV6_SUP
    if(soc->ver == IP_VER6)
        ip6hdr = (T_IP6_HDR *)pkt->hdr;
    else
#endif
    ip4hdr = (T_IP4_HDR *)pkt->hdr;

    soc->rpi.src_port = 0;
    soc->rpi.dst_port = 0;
#ifdef IPV6_SUP
    if(soc->ver == IP_VER6)
        ip6_addr_cpy(soc->rpi.ip6sa, ip6hdr->sa);
    else
#endif
    soc->rpi.src_ipa  = htonl(ip4hdr->sa);

#ifdef IPV6_SUP
    if(soc->ver == IP_VER6)
        ip6_addr_cpy(soc->rpi.ip6da, ip6hdr->da);
    else
#endif
    {
      soc->rpi.dst_ipa  = htonl(ip4hdr->da);
      soc->rpi.ttl      = ip4hdr->ttl;
      soc->rpi.tos      = ip4hdr->tos;
    }
    soc->rpi.num      = pkt->net->dev->num;
    soc->rpi.ver      = soc->ver;

#ifdef LINK_INF_SUP
    dev = pkt->dev;
    if ((dev != NULL) && (dev->type == NET_DEV_TYPE_ETH)) {
        net_memcpy(soc->rpi.link, &pkt->buf[0] + dev->hhdrofs, sizeof(soc->rpi.link));
    }
#endif

    if (len > pkt->hdr_len) {
        net_memcpy((char*)data, (char*)pkt->hdr, pkt->hdr_len); /* IP Header */
        net_memcpy((char*)data + pkt->hdr_len, (char*)pkt->dat, len - pkt->hdr_len); /* ICMP Header + data */
    }
    else {
        net_memcpy((char*)data, (char*)pkt->hdr, len);
    }

#ifdef MSG_PEEK_SUP
    if ( (soc->flg & SOC_FLG_MSGPEEK) == 0 ) {
#endif
        rmv4que_top(&soc->rdatque);
        soc->rqsz++;
        net_buf_ret(pkt);
#ifdef MSG_PEEK_SUP
    } else {
        soc->flg &= ~SOC_FLG_MSGPEEK;
    }
#endif

    return len;
}

ER icmp_snd(T_NET_SOC *soc, VP data, UH len)
{
    T_NET     *net;
    T_NET_BUF *pkt;
    T_ICMP_HDR *icmphdr;
    T_IP4_HDR *iph;
#ifdef IPV6_SUP
    T_IP6_HDR *ip6h;
#endif
    ER        ercd;

    if (soc->state == SOC_UNUSED) {
        return E_OBJ;
    }

#ifdef IPV6_SUP
    if(soc->ver == IP_VER6) {
        if(ip6_addr_type(soc->raddr6) == IP6_ADDR_UNSPEC) {
            return E_OBJ;
        }
    }
    else
#endif
    if (soc->raddr == INADDR_ANY) {
        return E_OBJ;
    }

    if (len == 0) {
        return E_PAR;
    }

    if (soc->sdatque != NULL) {
        net_sts_inc(NET_STS_ICMP, NET_STS_ICMP_OUT_ERR);
        return E_QOVR;
    }

    net = soc->net;
    if (net == NULL) {
        net_sts_inc(NET_STS_ICMP, NET_STS_ICMP_OUT_ERR);
        return E_OBJ;
    }

    ercd = net_buf_get(&pkt, ICMP_BUF_SIZE(len), TMO_POL);
    if (ercd != E_OK) {
        net_sts_inc(NET_STS_ICMP, NET_STS_ICMP_OUT_ERR);
        return E_NOMEM;
    }

    pkt->hdr = pkt->hdr + net->dev->hhdrsz + net->dev->hhdrofs;

#ifdef IPV6_SUP
    if(soc->ver == IP_VER6)
        pkt->hdr_len = IP6_HDR_SZ;
    else
#endif    
    pkt->hdr_len = IP4_HDR_SZ;
    pkt->dat = pkt->hdr + pkt->hdr_len;     /*Modified to support IPV6*/
    pkt->dat_len = len; /* ICMP Header + Data len */

#ifdef IPV6_SUP
    if(soc->ver == IP_VER6) {
        ip6h = (T_IP6_HDR *)pkt->hdr;
        ip6h->pl = htons(pkt->dat_len);
        ip6_addr_cpy(ip6h->da, soc->raddr6);
        set_ip6_src(ip6h->sa, ip6h->da, NULL, net->dev->num);
        ip6h->nh    = IP_PROTO_ICMP;
        ip6h->hop = net->ip6adr->hop;
    }
    else {
#endif
    /* IP4 Header */
    iph = (T_IP4_HDR *)pkt->hdr;

    iph->ver     = 0x45;
    iph->tos     = soc->tos;
    iph->tl      = htons(pkt->hdr_len + pkt->dat_len);
    iph->id      = htons(net->ident++);
    if (soc->flg & SOC_FLG_IPDF) {
        iph->fo  = htons(IP_FLG_DF << 13);
    } else {
        iph->fo  = 0;
    }
    iph->ttl     = soc->ttl;
    iph->hc      = 0;
    iph->sa      = htonl(net->adr->ipaddr);
    iph->da      = htonl(soc->raddr);
    iph->prot    = IP_PROTO_ICMP;
    if (net->flag & HW_CS_TX_IPH4)
        pkt->flg |= HW_CS_TX_IPH4;
    else
        iph->hc = ip4_csum(pkt);
#ifdef IPV6_SUP
    }
#endif
    /* ICMP Header & Data */
    net_memcpy((char*)pkt->dat, (char*)data, len);

    /* Checksum */
    icmphdr = (T_ICMP_HDR *)pkt->dat;
    icmphdr->cs = 0;
    if ((len <= 1480) && (net->flag & HW_CS_TX_ICMP))
        pkt->flg |= HW_CS_TX_ICMP;
    else
        icmphdr->cs = icmp_csum(pkt);

    soc->slen    = len;
    pkt->net = net;
    pkt->ercd = len;

#ifdef IPV6_SUP
    if(soc->ver == IP_VER6) {
        ercd = icmp6_pkt_snd(pkt, 0);
        if (ercd == E_WBLK) {
            soc->sdatque = (UW*)pkt;
            pkt->soc = soc;
        }
        return ercd;
    }
#endif
    
    pkt->hdr_len += pkt->dat_len;
    pkt->dat = NULL;
    pkt->dat_len = 0;

    ercd = net->out(pkt);
    if (ercd == E_WBLK) {
        soc->sdatque = (UW*)pkt;
        pkt->soc = soc;
    }
    if ((ercd == E_WBLK) || (ercd == E_OK)) {
        net_sts_inc(NET_STS_ICMP, NET_STS_ICMP_OUT_MSG);
        switch (icmphdr->type)
        {
            case ICMP_ECHO_REQUEST: /* Echo Request */
                net_sts_inc(NET_STS_ICMP, NET_STS_ICMP_OUT_ECHO);
                break;
            case ICMP_ECHO_REPLY:       /* Echo Reply */
                net_sts_inc(NET_STS_ICMP, NET_STS_ICMP_OUT_ECHO_REP);
                break;
            case ICMP_DST_UNREACH:      /* Destination Unreachable */
                net_sts_inc(NET_STS_ICMP, NET_STS_ICMP_OUT_DEST_UNREACH);
                break;
            case ICMP_SRC_QUENC:        /* Source Quench */
                net_sts_inc(NET_STS_ICMP, NET_STS_ICMP_OUT_SRC_QUENCH);
                break;
            case ICMP_REDIRECT:         /* Redirect */
                net_sts_inc(NET_STS_ICMP, NET_STS_ICMP_OUT_REDIRECT);
                break;
            case ICMP_TIME_EXCEED:      /* Time Exceeded */
                net_sts_inc(NET_STS_ICMP, NET_STS_ICMP_OUT_TIME_EXCD);
                break;
            case ICMP_PRM_PROB:         /* Parameter Problem */
                net_sts_inc(NET_STS_ICMP, NET_STS_ICMP_OUT_PARM_PROB);
                break;
            case ICMP_TIMES_REQUEST:    /* Timestamp Request        */
                net_sts_inc(NET_STS_ICMP, NET_STS_ICMP_OUT_TIME_STAMP);
                break;
            case ICMP_TIMES_REPLY:      /* Timestamp Reply          */
                net_sts_inc(NET_STS_ICMP, NET_STS_ICMP_OUT_TIME_STAMP_REP);
                break;
            case ICMP_ADDR_REQUEST:     /* AddressMask Request      */
                net_sts_inc(NET_STS_ICMP, NET_STS_ICMP_OUT_ADDR_MASK);
                break;
            case ICMP_ADDR_REPLY:       /* AddressMask Reply        */
                net_sts_inc(NET_STS_ICMP, NET_STS_ICMP_OUT_ADDR_MASK_REP);
                break;
            default:
                break;
        }
    }

    return ercd;
}

ER icmp_error(T_NET_BUF *pkt)
{
    T_NET     *net;
    T_ICMP_HDR *icmphdr;
    T_IP4_HDR *iph;
    ER        ercd;
    UW        dst;

    net = pkt->net;

    /* IP4 Header */
    iph = (T_IP4_HDR *)pkt->hdr;

    dst = ntohl(iph->da);
    if (IS_MCAST_IP(dst) || IS_BCAST_IP(dst, net->adr->ipaddr, net->adr->mask)) {
        net_buf_ret(pkt);
        return E_OBJ;
    }
    iph->ver     = 0x45;
    iph->tos     = IP4_TOS;
    iph->tl      = htons(pkt->hdr_len + pkt->dat_len);
    iph->id      = htons(net->ident++);
    iph->fo      = 0;
    iph->ttl     = IP4_TTL;
    iph->hc      = 0;
    iph->da      = iph->sa;
    iph->sa      = htonl(net->adr->ipaddr);
    iph->prot    = IP_PROTO_ICMP;
    if (!(net->flag & HW_CS_TX_IPH4)) {
        iph->hc      = ip4_csum(pkt);
    }else{
        pkt->flg |= HW_CS_TX_IPH4;
    }

    /* Checksum */
    icmphdr = (T_ICMP_HDR *)pkt->dat;
    icmphdr->cs = 0;

    if (net->flag & HW_CS_TX_ICMP) {
        pkt->flg |= HW_CS_TX_ICMP;
    } else {
        icmphdr->cs = icmp_csum(pkt);
    }

    pkt->soc = NULL;
    pkt->net = net;
    pkt->hdr_len += pkt->dat_len;
    pkt->dat = NULL;
    pkt->dat_len = 0;
    pkt->ercd = pkt->hdr_len;
    ercd = net->out(pkt);

    if ((ercd == E_WBLK) || (ercd == E_OK)) {
        switch (icmphdr->type) {
            case ICMP_DST_UNREACH:    /* Destination Unreachable */
                net_sts_inc(NET_STS_ICMP, NET_STS_ICMP_OUT_DEST_UNREACH);
                break;
            case ICMP_TIME_EXCEED:    /* Time Exceeded            */
                net_sts_inc(NET_STS_ICMP, NET_STS_ICMP_OUT_TIME_EXCD);
                break;
            default:
                break;
        }
    }

    return ercd;
}
#endif  /* PING_SUP */

