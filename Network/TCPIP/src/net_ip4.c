/***************************************************************************
    MICRO C CUBE / COMPACT, NETWORK
    IP Version4 Reception Process
    Copyright (c)  2008-2021, eForce Co., Ltd. All rights reserved.

    Version Information  2008.11.19: Created
                         2010.05.01: Allow in IP source address '0'
                         2010.08.10: Updated for broadcast IP address check
                         2012.10.02  Modify to avoid use of string libraries.
                         2013.09.25: Implememted local loopback
                         2014.03.06: SNMP option was added
                         2014.11.26: Add ip fowarding function
                         2015.03.17: Add ICMP error for the packet it could
                                     not be forwarded.
                         2016.03.19  is_mgroup_in() called with 'src' param
                                     to support  IGMP Version 3.
                         2016.04.08: Change the order of the transfer process
                                     and the reassembly process
                         2017.07.19: Limit the protocol of is_mgroup_in() to UDP
                         2021.03.17: Added random seed update process
 ***************************************************************************/

#include <stdio.h>
#include "kernel.h"
#include "net_hdr.h"
#include "net_def.h"
#include "net_sts_id.h"

static UB ip4_addr_chk(T_NET_BUF *pkt, T_IP4_HDR *ip4hdr)
{
    UW src, dst;
    T_NET *net = pkt->net;

    src = ntohl(ip4hdr->sa);

    /* Discard if SourceAddress is not a unicast address */
    if ((src & 0xFF000000) == 0) {
#ifdef UDP_SUP
        /* Allow zero address for IP address configuration (DHCP Server) */
        if (!((net->flag & IP_RCV_BCAST) && (ip4hdr->prot == IP_PROTO_UDP))) {
            return 0;
        }
#else
        return 0;   /* Zero Address */
#endif
    }

    if (IS_MCAST_IP(src)) {
        return 0;   /* Multicast */
    }

    if (IS_LBACK_IP(src)) {
        return 0;   /* LoopBack */
    }

    /* Destination Address */

    dst = ntohl(ip4hdr->da);

#ifdef LO_IF_SUP
    if (IS_LBACK_IP(dst)) {
        return 1;   /* LoopBack */
    }
#endif

    if (net->adr->ipaddr == 0) {

        if (IS_RES_BCAST_IP(src)) {
            return 0;   /* Broadcast source address */
        }

#ifdef UDP_SUP
        /* Now we allow only Broadcast and UDP */
        if (!(net->flag & IP_RCV_BCAST))
            return 0;

        if (!(IS_RES_BCAST_IP(dst)))
            return 0;

        if (ip4hdr->prot != IP_PROTO_UDP)
            return 0;

        pkt->flg |= IP_RCV_BCAST;

        return 1;
#else
        return 0;
#endif
    }

    if (IS_BCAST_IP(src, net->adr->ipaddr, net->adr->mask)) {
        return 0;   /* Broadcast source address */
    }

    if (net->adr->ipaddr == dst) {
        return 1;   /* UniCast Destination */
    }
#ifdef UDP_SUP
    else if (IS_MCAST_IP(dst)) {
#ifdef MCAST_SUP
        if (ip4hdr->prot == IP_PROTO_IGMP) {
            return 1;
        }
        if (is_mgroup_in(net, dst, src)) {
            pkt->flg |= IP_RCV_MCAST;
            return 1;
        }
#else
        return 0;
#endif
    }
    else if (IS_BCAST_IP(dst, net->adr->ipaddr, net->adr->mask)) {

        if ((net->flag & IP_RCV_BCAST) && (ip4hdr->prot == IP_PROTO_UDP)) {
                pkt->flg |= IP_RCV_BCAST;
                return 1;
        }
    }
    else {
        if (!(pkt->flg & IP_RCV_BCAST)) {
#ifdef IP4_FWD_SUP
          return 2;
#else
          return 0;
#endif
        }
        /* Zero or LoopBack or BadClass */
    }
#endif
    return 0;
}

/*
[IN]
    pkt->hdr     = ippacket
    pkt->hdr_len = ippacket length
[OUT]
    pkt->hdr     = ip4hdr;
    pkt->hdr_len = ip4hdr_len;
    pkt->dat     = tcp;
    pkt->dat_len = tcp_len
*/
void ip4_rcv(T_NET_BUF *pkt)
{
    T_NET *net;
    T_IP4_HDR  *ip4hdr;
    UH ihlen, offset, flg;
    UB ret;
#ifdef IPR_SUP
    ER ercd;
#endif
    T_ICMP_HDR *icmphdr;

    net = pkt->net;

    pkt->hdr     = pkt->dat;        /* IP Offset  */
    pkt->hdr_len = pkt->dat_len;    /* IP Payload */

    net_sts_inc(NET_STS_IP, NET_STS_IP_IN_RCV);

    /* 1. Validate receive length */
    if (pkt->hdr_len < IP4_HDR_SZ) {
        net_buf_ret(pkt);
        net_sts_inc(NET_STS_IP, NET_STS_IP_IN_HDR_ERR);
        return;
    }

    /* 2. Validate IP Header */
    ip4hdr = (T_IP4_HDR *)pkt->hdr;
    ihlen  = (ip4hdr->ver & 0x0F) * 4;

    /* IP Header Version shoule be 0x4 */
    if ((ip4hdr->ver & 0xF0) != 0x40) {
        net_buf_ret(pkt);
        net_sts_inc(NET_STS_IP, NET_STS_IP_IN_HDR_ERR);
        return;
    }

    /* Minimum IP Header Length should be 20 bytes  */
    if (ihlen < IP4_HDR_SZ) {
        net_buf_ret(pkt);
        net_sts_inc(NET_STS_IP, NET_STS_IP_IN_HDR_ERR);
        return;
    }

    pkt->hdr_len = ihlen;               /* IP Header Len  */

    if (!SKIP_IPCS) {
    /* Validate IP Header checksum */
    if (pkt->flg & HW_CS_RX_IPH4) {
        if (pkt->flg & HW_CS_IPH4_ERR) {
            net_buf_ret(pkt);
            net_sts_inc(NET_STS_IP, NET_STS_IP_IN_HDR_ERR);
            return;
        }
    }
    else {
        if (ip4_csum(pkt) != 0) {
            net_buf_ret(pkt);
            net_sts_inc(NET_STS_IP, NET_STS_IP_IN_HDR_ERR);
            return;
        }
    }
    } /* skip checksum */

    /* Bogus Header? */
    if (ntohs(ip4hdr->tl) > pkt->dat_len) {
        net_buf_ret(pkt);
        net_sts_inc(NET_STS_IP, NET_STS_IP_IN_HDR_ERR);
        return;
    }

    /* Validate total length */
    if ((ntohs(ip4hdr->tl)) > PATH_MTU) {
        net_buf_ret(pkt);
        net_sts_inc(NET_STS_IP, NET_STS_IP_IN_HDR_ERR);
        return;
    }

    net_rand_seed(ip4hdr->id);
    offset = ntohs(ip4hdr->fo);

    /* NAT */
nat_change:
    if ((offset == 0) && (net->adr->mode & NET_ADR_TYPE_GLOBAL)) {
        if (nat_from_global(ip4hdr) != E_OK) {
            net_buf_ret(pkt);
            return;
        }
    }

    /* 3. IP Address Match                          */
    ret = ip4_addr_chk(pkt, ip4hdr);
    if (ret == 0) {
        net_buf_ret(pkt);
        net_sts_inc(NET_STS_IP, NET_STS_IP_IN_ADDR_ERR);
        return;
    }

    pkt->dat     = pkt->hdr + ihlen;            /* IP Data Offset */
    pkt->dat_len = ntohs(ip4hdr->tl) - ihlen;   /* IP Data        */

    /* 4. Reassemble if received a Fragment packet  */
    if (offset != 0) { /* Flag or Offset is set */
        flg = offset >> 13;
        offset = (offset & 0x1FFF) << 3;
        net_sts_inc(NET_STS_IP, NET_STS_IP_REASM_REQ);
        if ((offset != 0) || (flg & IP_FLG_MF)) {
#ifndef IPR_SUP
            /* Drop fragment packets */
            net_buf_ret(pkt);
            net_sts_inc(NET_STS_IP, NET_STS_IP_REASM_FAIL);
            return;
#else
            /* Reassemble fragments */
            ercd = ip4_reassembly(&pkt, ip4hdr);
            if (ercd != E_OK) {
                net_buf_ret(pkt);
                net_sts_inc(NET_STS_IP, NET_STS_IP_REASM_FAIL);
                return;
            }

            if (pkt == NULL) {
                net_sts_inc(NET_STS_IP, NET_STS_IP_REASM_FAIL);
                return;
            }
#endif
        }
        net_sts_inc(NET_STS_IP, NET_STS_IP_REASM_OK);

        if (net->adr->mode & NET_ADR_TYPE_GLOBAL) {
            ip4hdr = (T_IP4_HDR *)pkt->hdr;
            offset = 0;
            goto nat_change;
        }
    }

    /* Fowarding */
    if (ret == 2) {
        ip4hdr = (T_IP4_HDR *)pkt->hdr;
        ip4hdr->fo = 0;
        pkt->dat_len += ihlen;
        if (ip4hdr->ttl <= 1) {
            /* icmp error */
            pkt->dat = pkt->hdr + ihlen;
            icmphdr = (T_ICMP_HDR *)(pkt->dat);
            net_memcpy(icmphdr + 1, pkt->hdr, pkt->hdr_len + pkt->dat_len);
            icmphdr->type = ICMP_TIME_EXCEED;
            icmphdr->code = 0;  /* Time to Live exceeded in Transit*/
            icmphdr->id = 0;
            icmphdr->seq = 0;
            pkt->dat_len = IP4_HDR_SZ + ICMP_HDR_SZ + pkt->dat_len;
            icmp_error(pkt);
            return;
        }
        /* change network */
        net = get_fwd_net(ip4hdr);
        if (net == NULL) {
            /* icmp error */
            pkt->dat = pkt->hdr + ihlen;
            icmphdr = (T_ICMP_HDR *)(pkt->dat);
            net_memcpy(icmphdr + 1, pkt->hdr, pkt->hdr_len + pkt->dat_len);
            icmphdr->type = ICMP_DST_UNREACH;
            icmphdr->code = 0;  /* Net Unreachable */
            icmphdr->id = 0;
            icmphdr->seq = 0;
            pkt->dat_len = IP4_HDR_SZ + ICMP_HDR_SZ + pkt->dat_len;
            icmp_error(pkt);
            return;
        }
        if (net->adr->mode & NET_ADR_TYPE_GLOBAL) {
            if (nat_to_global(ip4hdr) != E_OK) {
                net_buf_ret(pkt);
                return;
            }
        }
        ip4_fwd_pkt(pkt, ip4hdr, net);
        net_buf_ret(pkt);
        return;
    }

    /* 5. Deliver to upper layer                    */
    switch (ip4hdr->prot) {
        case IP_PROTO_ICMP:
            icmp_pkt_rcv(pkt);
            net_sts_inc(NET_STS_IP, NET_STS_IP_IN_DELIVER);
            break;
#ifdef UDP_SUP
        case IP_PROTO_UDP:
            udp_pkt_rcv(pkt);
            net_sts_inc(NET_STS_IP, NET_STS_IP_IN_DELIVER);
            break;
#endif
#ifdef MCAST_SUP
        case IP_PROTO_IGMP:
            igmp_rcv(pkt);
            net_sts_inc(NET_STS_IP, NET_STS_IP_IN_DELIVER);
            break;
#endif
#ifdef TCP_SUP
        case IP_PROTO_TCP:
            tcp_pkt_rcv(pkt);
            net_sts_inc(NET_STS_IP, NET_STS_IP_IN_DELIVER);
            break;
#endif
        default:
            /* Ignore the packet */
            net_buf_ret(pkt);
            net_sts_inc(NET_STS_IP, NET_STS_IP_UNKNOWN_PROTO);
            return;
    }

    return;
}
