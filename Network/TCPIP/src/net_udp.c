/***************************************************************************
    MICRO C CUBE / COMPACT, NETWORK
    UDP Protocol
    Copyright (c)  2008-2021, eForce Co., Ltd. All rights reserved.

    Version Information  2008.11.19: Created
                         2009.06.12: Corrected soc_rcv return length
                         2010.01.18: ETH_HDR_SZ to dev->hhdrsz
                         2010.08.17: Support SOC_RCV_PKT_INF socket option
                         2010.11.02: Updated for IPv6 support
                         2011.03.07: Updated for received packet queue
                         2011.03.11: Updated for IPv6 multiple interface
                         2011.03.30: Updated for TX blocking device
                         2011.04.27: Choose socket matching IP/Socket version.
                         2011.05.20: Set up networkbuffer offset
                         2012.06.06: Implemented port unreachable with ICMPv4
                         2012.10.02  Modify to avoid use of string libraries.
                         2012.12.07: Corrected to choose IPv4 socket when IPv6
                                     sockets are mixed
                         2013.07.23  Implemented ready I/O event notification
                         2013.08.06: Implememted multicast per socket
                         2014.02.06: Change byte order for ICMPv6 error payload
                         2014.03.06: SNMP option was added
                         2015.10.16: Modify for udp checksum.
                         2015.11.17: Discard the IPv6 packet checksum is zero
                         2015.12.03: Add SOC_SRC_IP_RESET option
                         2015.12.14: The socket ID replaced SID types
                         2016.01.08: Update for IPv6 MLD
                         2016.02.04: Update for early transmit
                         2016.02.18: Add parameter check for transmit overflow
                         2016.03.09  Devide HW_CS flag into ICMP and TCP/UDP
                         2016.06.09  Add the process when specify the MSG_PEEK option.
                         2016.06.16  Support the REUSEADDR option for UDP
                         2016.06.24  Add link of peer option
                         2016.10.06  Fixed the SOC_PRT_LOCAL_DUP option.
                         2017.01.17: Clarified the type for constant
                         2017.03.08: Allow the same port socket with different IP ver
                         2018.02.01: Change receive length check processing.
                         2018.08.23: Support set or reset the don't fragment flag in IP header
                         2020.10.16: Extended number of multicast groups per socket.
 ***************************************************************************/

#include <stdio.h>
#include "kernel.h"
#include "net_hdr.h"
#include "net_def.h"
#include "net_sts_id.h"

#ifdef UDP_SUP

#ifdef MAC_RESOLVE

static void udp_reply(T_NET_BUF *pkt)
{
    T_IP4_HDR *ip4hdr;
    T_UDP_HDR *udphdr;
    T_NET *net;
    UW t_ip;
    UB *dat;
    ER ercd;

    if (pkt->dat_len < (UDP_HDR_SZ + 12)) {
        net_buf_ret(pkt);
        return;
    }

    dat = pkt->dat + UDP_HDR_SZ;
    if (!(dat[0] == 0x43 && dat[1] == 0x21)) {
        net_buf_ret(pkt);
        return;
    }

    net = pkt->net;

    if (net_memcmp(&dat[2], net->dev->cfg.eth.mac, 6) != 0) {
        net_buf_ret(pkt);
        return;
    }

    t_ip = htonl(net->adr->ipaddr);
    net_memcpy(&dat[8], (char*)&t_ip, 4);

    ip4hdr     = (T_IP4_HDR *)pkt->hdr;
    ip4hdr->da = ip4hdr->sa;
    ip4hdr->sa = t_ip;
    ip4hdr->hc = 0;
    if (net->flag & HW_CS_TX_IPH4)
        pkt->flg |= HW_CS_TX_IPH4;
    else
        ip4hdr->hc = ip4_csum(pkt);

    udphdr     = (T_UDP_HDR*)pkt->dat;
    udphdr->dp = udphdr->sp;
    udphdr->sp = htons(UDP_MAC_PORT);
    /*udphdr->dp = htons(UDP_MAC_PORT);*/
    udphdr->len = htons(udphdr->len);
    udphdr->cs = 0;
    if (net->flag & HW_CS_TX_TCPUDP)
        pkt->flg |= HW_CS_TX_TCPUDP;
    else
    {
        udphdr->cs = udp_csum(pkt);
        if ( udphdr->cs == 0 ) {
            udphdr->cs = 0xFFFF;
        }
    }

    pkt->soc = NULL;
    pkt->net = net;
    /*pkt->hdr = pkt->buf + 2;*/
    pkt->hdr_len += pkt->dat_len;
    pkt->dat = NULL;
    pkt->dat_len = 0;

    ercd = net->out(pkt);
    if ((ercd == E_WBLK) || (ercd == E_OK)) {
        net_sts_inc(NET_STS_UDP, NET_STS_UDP_OUT_DATAGRAM);
    }

    return;
}
#endif

#ifdef REUSE_ADDR_SUP
static void dup_pkt_rcv(T_NET_SOC *src_soc, T_NET_BUF *pkt, B index)
{
    SID sid;
    ID mpfid;
    T_NET_SOC *dup_soc;
    T_NET_BUF *dup_pkt;
    T_NET *net;
    ER ercd;


    net = pkt->net;

    for (sid = NET_TCP_MAX; sid < NET_SOC_MAX; sid++) {
        dup_soc = &pNET_SOC[sid];
        if ( dup_soc != src_soc               &&
             dup_soc->ruanum > 0              &&
             dup_soc->state == SOC_CREATED    &&
             dup_soc->proto == IP_PROTO_UDP   &&
             dup_soc->ver   == src_soc->ver   &&
             dup_soc->lport == src_soc->lport &&
             (dup_soc->net == NULL || dup_soc->net == net) ) {

            if (isset_mgr_idx(dup_soc, index) == 0) {
                continue;
            }

            /* If receive queue is full */
            if ( dup_soc->rqsz == 0 ) {
                continue;
            }

            /* Get duplicate memory */
            ercd = net_buf_get( &dup_pkt, UDP_BUF_SIZE(pkt->dat_len), TMO_POL );
            if ( ercd != E_OK ) {
                /* No memory */
                break;
            }

            /* Copy packet frame for duplicate */
            mpfid = dup_pkt->mpfid;
            net_memcpy( dup_pkt, pkt, UDP_BUF_SIZE(pkt->dat_len) );
            dup_pkt->mpfid = mpfid;

            dup_pkt->next = NULL;
            /* IP header */
            dup_pkt->hdr = dup_pkt->buf + pkt->dev->hhdrofs + pkt->dev->hhdrsz;
            dup_pkt->hdr_len = pkt->hdr_len;
            /* UDP header + payload */
            dup_pkt->dat = dup_pkt->hdr + dup_pkt->hdr_len;
            dup_pkt->dat_len = pkt->dat_len;

            /* Enqueue duplicate packet */
            dup_soc->rqsz--;
            add2que_end( &dup_soc->rdatque, dup_pkt );

            /* Deliver data to waiting process */
            soc_event( dup_soc, EV_SOC_RCV, dup_pkt->dat_len - UDP_HDR_SZ );
        }
    }

    return;
}
#endif

void udp_pkt_rcv(T_NET_BUF *pkt)
{
    T_NET     *net;
    T_UDP_HDR *udphdr;
    T_NET_SOC *soc, *udpsoc;
    SID       sid;
    UH        udplen;
    T_ICMP_HDR *icmphdr;
#ifdef MCAST_SOC_SUP
    UW        da;
#ifdef IPV6_SUP
    T_IPV6_ADDR *adr;
    UW        *da6;
#endif
#endif
#if defined(MCAST_SOC_SUP) || defined(REUSE_ADDR_SUP)
    H         index;
    index = -1;
#endif

    net_sts_inc(NET_STS_UDP, NET_STS_UDP_IN_DATAGRAM);

    udphdr = (T_UDP_HDR*)pkt->dat;
    net    = pkt->net;

    /* Validate the receive length */
    if (pkt->dat_len < UDP_HDR_SZ) {
        net_buf_ret(pkt);
        net_sts_inc(NET_STS_UDP, NET_STS_UDP_IN_ERR);
        return;
    }

    if (!SKIP_UDPCS) {
    /* Validate UDP Header */
    if (pkt->flg & HW_CS_RX_TCPUDP) {
        if (pkt->flg & HW_CS_DATA_ERR) {
            net_buf_ret(pkt);
            net_sts_inc(NET_STS_UDP, NET_STS_UDP_IN_ERR);
            return;
        }
    }
    else {
#ifdef IPV6_SUP
        if(*(pkt->hdr)>>4 == IP_HDR_VER6) {
            if(udphdr->cs == 0 || ip6_cksum((T_IP6_HDR *)pkt->hdr, pkt->dat) != 0) {
                net_buf_ret(pkt);
                net_sts_inc(NET_STS_UDP, NET_STS_UDP_IN_ERR);
                return;
            }
        }
        else
#endif
        
        if (udphdr->cs != 0) {
            if (udp_csum(pkt) != 0) {
                net_buf_ret(pkt);
                net_sts_inc(NET_STS_UDP, NET_STS_UDP_IN_ERR);
                return;
            }
        }
    }
    } /* skip checksum */

    udphdr->len = ntohs(udphdr->len);
    udplen = pkt->dat_len - UDP_HDR_SZ;

    if (udplen != (udphdr->len - UDP_HDR_SZ)) {
        net_buf_ret(pkt);
        net_sts_inc(NET_STS_UDP, NET_STS_UDP_IN_ERR);
        return;
    }

    /* Validate Port     */
    udphdr->dp = ntohs(udphdr->dp);
    if (udphdr->dp == 0) {
        net_buf_ret(pkt);
        net_sts_inc(NET_STS_UDP, NET_STS_UDP_IN_ERR);
        return;
    }

#ifdef MAC_RESOLVE
    if (udphdr->dp == UDP_MAC_PORT) {
        udp_reply(pkt);
        return;
    }
#endif

    /* Find the matching socket */
    soc = udpsoc = NULL;

    for (sid = 0; sid < NET_SOC_MAX; sid++) {
        soc = &pNET_SOC[sid];
        if ((soc->state == SOC_CREATED)  &&
            (soc->proto == IP_PROTO_UDP) &&
            (soc->lport == udphdr->dp)   &&
            ((soc->net == NULL) || (soc->net == net))) {

#ifdef IPV6_SUP
            if(((*(pkt->hdr)>>4 == IP_HDR_VER6) && (soc->ver != IP_VER6))
            || ((*(pkt->hdr)>>4 == IP_HDR_VER4) && (soc->ver != IP_VER4))) {
                continue;
            }
#endif

#ifdef MCAST_SOC_SUP
            if (pkt->flg & IP_RCV_MCAST) {
                if (soc->ver == IP_VER4) {
                    /* join group? */
                    da = ((T_IP4_HDR *)pkt->hdr)->da;
                    da = ntohl(da);
                    if (da != IGMP_ALL_SYSTEMS) { /* not all node */
                        index = mgroup_index(net, da);
                        if (index < 0 || NET_MGR_MAX <= index) {
                            continue;
                        }
                        if (isset_mgr_idx(soc, index) == 0) {
                            continue;
                        }
                    }
#ifdef IPV6_SUP
                } else if (soc->ver == IP_VER6) {
                    da6 = ((T_IP6_HDR *)pkt->hdr)->da;
                    adr = chk_if_ip6_addr(da6, net->dev->num);
                    if (adr == NULL) {
                        continue;
                    }
                    if ((adr->flag & MLD_SOLICITING) == 0) {
                        index = adr->flag & MLD_INDEX_MSK;
                        if (isset_mgr_idx(soc, index) == 0) {
                            continue;
                        }
                    }
#endif
                } else {
                    continue;
                }
            }
#endif
            udpsoc = soc;
            break;
        }
    }

    if (udpsoc == NULL) {
#ifdef IPV6_SUP     
        /* Send ICMPv6 Error */
        if(*(pkt->hdr)>>4 == IP_HDR_VER6) {
            udphdr->len = ntohs(udphdr->len);
            udphdr->dp = ntohs(udphdr->dp);
            icmp6_err_snd(pkt, ICMPV6_DEST_UNREACH, ICMPV6_PORT_UNREACH, 0);
            net_buf_ret(pkt);
            net_sts_inc(NET_STS_UDP, NET_STS_UDP_NO_PORT);
            return;
        }
#endif
        /* ICMP Error? */
        if (REP_UNREACH) {
            udphdr->dp = ntohs(udphdr->dp);
            udphdr->len = ntohs(udphdr->len);
            net_memcpy(pkt->dat + ICMP_HDR_SZ, pkt->hdr, IP4_HDR_SZ + UDP_HDR_SZ);
            pkt->dat_len = ICMP_HDR_SZ + IP4_HDR_SZ + UDP_HDR_SZ;

            icmphdr = (T_ICMP_HDR *)pkt->dat;
            icmphdr->type = ICMP_DST_UNREACH;
            icmphdr->code = 3;  /* Port unreachable */
            icmphdr->id = 0;
            icmphdr->seq = 0;
            icmp_error(pkt);
        } else {
            net_buf_ret(pkt);
        }
        net_sts_inc(NET_STS_UDP, NET_STS_UDP_NO_PORT);
        return;
    }

#ifdef REUSE_ADDR_SUP
    if ( udpsoc->ruanum != 0 && udplen != 0U ) {
        dup_pkt_rcv( udpsoc, pkt, index);
    }
#endif

    /* Drop if receive queue is full or receive length is 0 */
    if (udpsoc->rqsz == 0 || udplen == 0U) {
        net_buf_ret(pkt);
        return;
    }
    udpsoc->rqsz--;
    add2que_end(&udpsoc->rdatque, pkt);

    /* Deliver data to waiting process */
    soc_event(udpsoc, EV_SOC_RCV, udplen);

    return;
}

ER udp_rcv(T_NET_SOC *soc, VP data, UH len)
{
    T_NET_BUF *pkt;
    T_IP4_HDR *ip4hdr = NULL;
#ifdef IPV6_SUP
    T_IP6_HDR *ip6hdr = NULL;
#endif
    T_UDP_HDR *udphdr;
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
    if (len > (pkt->dat_len - UDP_HDR_SZ)) {
        len = (pkt->dat_len - UDP_HDR_SZ);
    }

    /* store remote host information */
#ifdef IPV6_SUP
    if(soc->ver == IP_VER6)
        ip6hdr = (T_IP6_HDR *)pkt->hdr;
    else
#endif
    ip4hdr = (T_IP4_HDR *)pkt->hdr;
    udphdr = (T_UDP_HDR *)pkt->dat;

    soc->rpi.src_port = htons(udphdr->sp);
    soc->rpi.dst_port = udphdr->dp; /* Byte order already converted */
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
    soc->rpi.dst_ipa  = htonl(ip4hdr->da);

#ifdef IPV6_SUP
    if(soc->ver != IP_VER6) {
#endif
    soc->rpi.ttl      = ip4hdr->ttl;
    soc->rpi.tos      = ip4hdr->tos;
#ifdef IPV6_SUP
    }
#endif
    soc->rpi.num      = pkt->net->dev->num;
    soc->rpi.ver      = soc->ver;

#ifdef LINK_INF_SUP
    dev = pkt->dev;
    if ((dev != NULL) && (dev->type == NET_DEV_TYPE_ETH)) {
        net_memcpy(soc->rpi.link, &pkt->buf[0] + dev->hhdrofs, sizeof(soc->rpi.link));
    }
#endif

    net_memcpy((char*)data, (char*)pkt->dat + UDP_HDR_SZ, len);

#ifdef MSG_PEEK_SUP
    if ( (soc->flg & SOC_FLG_MSGPEEK) != 0 ) {
        /* If the reception is finished, the message peak option auto clear */
        soc->flg &= ~SOC_FLG_MSGPEEK;
    }
    else
#endif
    {
        rmv4que_top(&soc->rdatque);
        soc->rqsz++;
        net_buf_ret(pkt);
    }


    return len;
}

ER udp_snd(T_NET_SOC *soc, VP data, UH len)
{
    T_NET     *net;
    T_NET_BUF *pkt;
    T_UDP_HDR *udphdr;
    T_IP4_HDR *iph = NULL;
#ifdef IPV6_SUP
    T_IP6_HDR *ip6h = NULL;
#endif
    ER        ercd;
    UW        get_len;

    if (soc->state == SOC_UNUSED) {
        return E_OBJ;
    }

#ifdef IPV6_SUP
    if(soc->ver == IP_VER6) {
        if (ip6_addr_type(soc->raddr6) == IP6_ADDR_UNSPEC || (soc->rport == PORT_ANY))
            return E_OBJ;
    }
    else
#endif
    if ((soc->raddr == INADDR_ANY) || (soc->rport == PORT_ANY)) {
        return E_OBJ;
    }

    if (len == 0) {
        return E_PAR;
    }

    if (soc->sdatque != NULL) {
        return E_QOVR;
    }

    net = soc->net;
    if (net == NULL) {
        return E_OBJ;
    }

#ifdef IPV6_SUP
    if(soc->ver == IP_VER6)
    {
        get_len = len + UDP_HDR_SZ + IP6_HDR_SZ;
    }
    else
#endif
    {
        get_len = len + UDP_HDR_SZ + IP4_HDR_SZ;
    }
    if (get_len > 0xffffUL) {
        return E_PAR;
    }
    get_len += (sizeof(T_NET_BUF) & ~3U);
    get_len += net->dev->hhdrofs + net->dev->hhdrsz;
    ercd = net_buf_get(&pkt, get_len, TMO_POL);
    if (ercd != E_OK) {
        return E_NOMEM;
    }

    pkt->hdr = pkt->hdr + net->dev->hhdrsz + net->dev->hhdrofs;
#ifdef IPV6_SUP
    if(soc->ver == IP_VER6)
        pkt->hdr_len = IP6_HDR_SZ;
    else
#endif
    pkt->hdr_len = IP4_HDR_SZ;
    pkt->dat = pkt->hdr + pkt->hdr_len; /*PK: Modified to support both IP headers*/
    pkt->dat_len = UDP_HDR_SZ;

#ifdef IPV6_SUP
    if(soc->ver == IP_VER6) {
        ip6h = (T_IP6_HDR *)pkt->hdr;
        
        ip6h->pl = htons(pkt->dat_len + len);
        ip6_addr_cpy(ip6h->da, soc->raddr6);
        set_ip6_src(ip6h->sa, soc->raddr6, NULL, net->dev->num);
        ip6h->nh    = IP_PROTO_UDP;
        ip6h->hop = net->ip6adr->hop;
    }
    else {
#endif
    /* IP4 Header */

    iph = (T_IP4_HDR *)pkt->hdr;

    iph->ver     = 0x45;
    iph->tos     = soc->tos;
    iph->tl      = htons(pkt->hdr_len + pkt->dat_len + len);
    iph->id      = htons(net->ident++);
    if (soc->flg & SOC_FLG_IPDF) {
        iph->fo  = htons(IP_FLG_DF << 13);
    } else {
        iph->fo  = 0;
    }
    if (IS_MCAST_IP(soc->raddr))
        iph->ttl = IP4_MCAST_TTL;
    else
        iph->ttl = soc->ttl;
    iph->hc      = 0;
    if (soc->flg & SOC_FLG_NOSRC) {
        iph->sa      = 0;
    } else {
        iph->sa      = htonl(net->adr->ipaddr);
    }
    iph->da      = htonl(soc->raddr);
    iph->prot    = IP_PROTO_UDP;
    if (net->flag & HW_CS_TX_IPH4)
        pkt->flg |= HW_CS_TX_IPH4;
    else
        iph->hc      = ip4_csum(pkt);

#ifdef IPV6_SUP
    }
#endif
    /* UDP Header */
    udphdr = (T_UDP_HDR *)pkt->dat;
    pkt->dat_len += len;

    udphdr->sp = htons(soc->lport);
    udphdr->dp = htons(soc->rport);
    udphdr->len = htons(pkt->dat_len);
    udphdr->cs = 0;

    /* Data */
    net_memcpy((char*)(pkt->dat + UDP_HDR_SZ), (char*)data, len);

    if (net->flag & HW_CS_TX_TCPUDP)
        pkt->flg |= HW_CS_TX_TCPUDP;
    else
    {
#ifdef IPV6_SUP
        if(soc->ver == IP_VER6)
            udphdr->cs = ntohs(ip6_cksum(ip6h, pkt->dat));
        else
#endif    
        udphdr->cs = udp_csum(pkt);

        if ( udphdr->cs == 0 ) {
            udphdr->cs = 0xFFFF;
        }
    }

    soc->slen    = len;
    pkt->net = net;
    pkt->ercd = len;
    
#ifdef IPV6_SUP
    if(soc->ver == IP_VER6) {
        ercd = ip6_snd(pkt);
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
#ifdef POSIX_API_SUP
        if (soc->flg & SOC_FLG_EARLYTX) {
            ercd = len;
        }
        else
#endif
        {
            soc->sdatque = (UW*)pkt;
            pkt->soc = soc;
        }
    }
    if ((ercd == E_WBLK) || (ercd >= E_OK)) {
        net_sts_inc(NET_STS_UDP, NET_STS_UDP_OUT_DATAGRAM);
    }

    return ercd;
}

#endif
