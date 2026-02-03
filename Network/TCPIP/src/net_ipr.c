/***************************************************************************
    MICRO C CUBE / COMPACT, NETWORK
    IP Reassembly & Fragmentation
    Copyright (c)  2008-2021, eForce Co., Ltd. All rights reserved.

    Version Information  2008.11.19: Created
                         2010.01.18: ETH_HDR_SZ to dev->hhdrsz
                                     ipr_max check
                         2010.07.20: Should release network buffer when
                                     dev->out() returns other than E_WBLK.
                         2010.11.02: Updated for IPv6 support
                         2011.03.30: Updated for TX blocking device
                         2011.05.20: Set up networkbuffer offset
                         2012.10.02  Modify to avoid use of string libraries.
                         2013.09.25: Implememted local loopback
                         2014.01.24: Fixed an error the calculating ipr_max
                                     in ip4_reassembly()
                         2014.03.06: SNMP option was added
                         2014.03.25: Corrected the judgment of timer processing
                         2014.05.15: Update to use network buffer size is different
                         2015.10.20: Modify for fragment packet checksum.
                         2016.02.19: Change the size specifying when to get network buffer
                         2016.02.24: Suppressed the warning for Kpit GNU tools
                         2016.04.08: Fixed the fragment transmission of PPP packet
                         2019.03.19: Discard unexpected fragment offsets packet
                         2021.03.17: Define the macro for each element number of net_inftbl[]
 ***************************************************************************/

#include <stdio.h>
#include "kernel.h"
#include "net_hdr.h"
#include "net_def.h"
#include "net_sts_id.h"

#ifdef IPR_SUP

void ipr_init(void)
{
    T_NET_IPR *ipr;
    UH i;

    for (i=0;i<NET_IPR_MAX;i++) {
        ipr = &pNET_IPR[i];
        ipr->cnt = 0;
    }

    IPR_TIMER_ON = 0;

}

/*
    This 1 sec cyclic timer routine checks whether any existing
    reassembly process timeout expires or not. If so then cancel
    the process and release the resources.
*/

void ipr_timer(UW ctimval)
{
    T_NET_IPR *ipr;
    T_NET_BUF *frgpkt;
    UH i;
#ifdef PING_SUP
    T_ICMP_HDR *icmpmsg;
#endif

    IPR_TIMER_ON = 0;
    for (i=0;i<NET_IPR_MAX;i++) {

        ipr = &pNET_IPR[i];

        if (ipr->cnt != 0) {

            if (TIM_LEQ(ipr->tmo,ctimval)) {

                NET_LOG(printf("\r\n ipr_timer: IP 0x%x ID %d", ipr->sa, ipr->id));

#ifdef PING_SUP
                /* To send ICMP error message, use the last
                   queued fragment packet */
                ipr->cnt--;
#endif
                /* Release Fragment Queue */

                for (i=0;i<ipr->cnt;i++) {
                    frgpkt = (T_NET_BUF *)ipr->ipq;
                    ipr->ipq = frgpkt->next;
                    frgpkt->next = NULL;
#ifdef IPV6_SUP
                    /* ICMP Error Message For IPv6 packets */
                    if(((*(frgpkt->hdr)  & 0xF0)>>4)== IP_HDR_VER6) {
                        /* ICMP Time Exceeded -- Fragment Reassembly Time Exceeded message for first fragment */
                        if(frgpkt->seq == 0)
                            icmp6_err_snd((T_NET_BUF *)frgpkt, ICMPV6_TIME_EXCEED, ICMPV6_EXC_FRAGTIME, 0);
                    }
#endif
                    
                    net_buf_ret((T_NET_BUF *)frgpkt);
                }

                /* Free Reassembly hash queue */
                ipr->cnt = 0;

#ifdef PING_SUP
                frgpkt = (T_NET_BUF *)ipr->ipq;
                if (frgpkt == NULL) {
                    /* FATAL: should not occur */
                    continue;
                }
#ifdef IPV6_SUP
                if(((*(frgpkt->hdr)  & 0xF0)>>4)== IP_HDR_VER6) {
                    /* ICMP Time Exceeded -- Fragment Reassembly Time Exceeded message for first fragment */
                    if(frgpkt->seq == 0)
                        icmp6_err_snd((T_NET_BUF *)frgpkt, ICMPV6_TIME_EXCEED, ICMPV6_EXC_FRAGTIME, 0);

                    net_buf_ret((T_NET_BUF *)frgpkt);
                }
                else
#endif
                {
                /* ICMP Error Message         */
                icmpmsg = (T_ICMP_HDR *)frgpkt->dat;
                icmpmsg->type = ICMP_TIME_EXCEED;
                icmpmsg->code = 1;  /* Timeout during reassembly */

                /* ICMP Data (IP header) */
                frgpkt->hdr_len = IP4_HDR_SZ;
                frgpkt->dat_len = frgpkt->hdr_len + ICMP_HDR_SZ;
                net_memcpy(frgpkt->dat + ICMP_HDR_SZ, frgpkt->hdr, frgpkt->hdr_len);
                icmp_error(frgpkt);
                }
#endif
            }

            IPR_TIMER_ON = 1;
        }
    }
}

/*
    This routine assembles the incoming IP Fragmented packets.  If all
    fragments received then a new packet is allocated and the fragment
    data is moved to the new packet. This routine returns E_OK value if
    the reassembly process is successful otherwise it returns the
    appropriate error values. When successful the new packet address is
    passed to the IP layer.
*/

ER ip4_reassembly(T_NET_BUF **pkt, T_IP4_HDR *ip4hdr)
{
    T_NET_IPR *ipr, *ipr_tmp;
    T_NET_BUF *frgpkt, *newpkt;
    UH i, pkt_len, ihlen, fflg, ipr_max;
    UB *ptr;
    ER ercd;
    T_NET *net;
    ID mpfid;

    ip4hdr->fo = ntohs(ip4hdr->fo);

    fflg = ip4hdr->fo >> 13;
    ip4hdr->fo = (ip4hdr->fo & 0x1FFF) << 3;

    /* Not a Fragment Packet ? */
    if (ip4hdr->fo == 0 && !(fflg & IP_FLG_MF)) { 
        return E_OK;
    }

    frgpkt = (T_NET_BUF *)(*pkt);
    *pkt   = NULL;
    net    = frgpkt->net;

    ihlen   = frgpkt->hdr_len;
    pkt_len = frgpkt->dat_len;
    if ((pkt_len == 0) || (pkt_len == 65535U)) {
        NET_ERR(printf("\r\n ERR: ip4_reassembly: bogus IP Fragment len %d ", pkt_len));
        net_buf_ret(frgpkt);
        return E_OBJ;
    }

    /* Find the Reassembly Queue */
    ipr = ipr_tmp = NULL;
    for (i=0;i<NET_IPR_MAX;i++) {
        ipr = &pNET_IPR[i];
        if (ipr->cnt != 0) {
            if ((ipr->id == ip4hdr->id) &&
                (ipr->prot == ip4hdr->prot) &&
                (ipr->sa   == ip4hdr->sa) &&
                (ipr->da   == ip4hdr->da)) {

                ipr_tmp = ipr;
                break;
            }
        }
    }

    /* Create a New process */
    if (ipr_tmp == NULL) {
        for (i=0;i<NET_IPR_MAX;i++) {
            ipr = &pNET_IPR[i];
            if (ipr->cnt == 0) {
                /*ipr->cnt = 1;*/ /* Set this after queue the fragment packet */
                ipr->id = ip4hdr->id;
                ipr->prot = ip4hdr->prot;
                ipr->sa = ip4hdr->sa;
                ipr->da = ip4hdr->da;
                ipr->tmo = IP4_IPR_TMO + NET_TICK;
                ipr->flg = 0;
                ipr->rlen = 0;
                ipr->tlen = 0;
                ipr->top  = (UW*)frgpkt;
                ipr->iphdrlen = ihlen;  /* Also, update iphdr len of first fragment */
                ipr->ipq = NULL;
                ipr_tmp = ipr;
                IPR_TIMER_ON = 1;
                break;
            }
        }
    }

    if (ipr_tmp == NULL) {
        /* No resource for this packet */
        NET_ERR(printf("\r\n ERR: ip4_reassembly: Resource "));
        net_buf_ret(frgpkt);
        return E_NOMEM;
    }

    ipr_max = NET_BUF_SZ - ipr->iphdrlen - (net->dev->hhdrsz + sizeof(T_NET_BUF)) ;
    ipr_max -= (net->dev->hhdrofs - 2);

    if (((ipr_tmp->rlen + pkt_len)) > ipr_max) {
        NET_ERR(printf("\r\n ERR: ip4_reassembly: Invalid IP Fragment"));
        ercd = E_OBJ;
        net_buf_ret(frgpkt);
        goto IPR_CMPLT;
    }

    if (!(fflg & IP_FLG_MF)) {     /* Last Fragment received */

        /* Validate Total Length */
        if (((ip4hdr->fo + pkt_len)) > ipr_max) {
            NET_ERR(printf("\r\n ERR: ip4_reassembly: Invalid IP Fragment"));
            ercd = E_OBJ;
            net_buf_ret(frgpkt);
            goto IPR_CMPLT;
        }

        ipr_tmp->tlen = ip4hdr->fo + pkt_len;
        ipr_tmp->flg |= 0x02;   /* Last fragment received */
    }
    else {
        if (ip4hdr->fo == 0) {               /* First Fragment */
            ipr_tmp->flg |= 0x01;
            ipr_tmp->iphdrlen = ihlen;       /* update length */
            ipr->top = (UW*)frgpkt;
        }
    }

    if (ipr_tmp->tlen != 0 ) {
        if (((UW)(ipr_tmp->rlen + pkt_len)) > ipr_tmp->tlen) {
            NET_ERR(printf("\r\n ERR: ip4_reassembly: bogus IP Fragment %d", pkt_len));
            /* OverFlow? */
            ercd = E_OBJ;
            net_buf_ret(frgpkt);
            goto IPR_CMPLT;
        }
    }

    ipr_tmp->rlen += pkt_len;
    ipr_tmp->tmo  = IP4_IPR_TMO + NET_TICK;     /* Refresh Timeout? */

    /* Add to receive packet queue */
    frgpkt->seq = ip4hdr->fo; /* Fragment offset */
    frgpkt->dat_len = pkt_len; /* Fragment Length */

    frgpkt->next = ipr_tmp->ipq;

    ipr_tmp->ipq = (UW *)frgpkt;

    ipr_tmp->cnt++;     /* Add fragment to queue */

    /* Check whether all packets are received */

    if (ipr_tmp->flg != 3) {
        return E_OK;   /* First or Last packet is missing */
    }

    if (ipr->rlen != ipr->tlen) {
        return E_OK;   /* Some more fragments missing      */
    }

    /* Construct new IP packet with received fragments */

    ercd = net_buf_get(&newpkt, IP4_BUF_SIZE(ipr->tlen), TMO_POL);
    if (ercd != E_OK) {
        NET_ERR(printf("\r\n ERR: ip4_reassembly: net_longbuffer_get"));
        ercd = E_NOMEM;
        goto IPR_CMPLT;
    }

    /* Copy the fragment header details */

    frgpkt = (T_NET_BUF *)ipr->top;     /* First Fragment */

    /* Copy Control Header */
    /* excpt mpfid */
    mpfid = newpkt->mpfid;
    net_memcpy((char*)newpkt, (char*)frgpkt, sizeof(T_NET_BUF));
    newpkt->mpfid = mpfid;
    newpkt->hdr = &newpkt->buf[0]; /* correct hdr to point this buffer */

    /* Copy Link Header */
    newpkt->hdr     = newpkt->buf + net->dev->hhdrofs;
    newpkt->hdr_len = net->dev->hhdrsz;
    net_memcpy((char*)newpkt->hdr, (char*)&frgpkt->buf[net->dev->hhdrofs], newpkt->hdr_len);

    /* Copy IP Header of first fragment */
    newpkt->hdr = newpkt->hdr + newpkt->hdr_len;
    newpkt->hdr_len = frgpkt->hdr_len;        /* IP Header length */
    net_memcpy((char*)newpkt->hdr, (char*)frgpkt->hdr, newpkt->hdr_len);

    /* Update the IP Header of new packet */
    ip4hdr     = (T_IP4_HDR *)newpkt->hdr;
    ip4hdr->fo = 0;
    ip4hdr->tl = htons(ipr->tlen + newpkt->hdr_len);

    /* Copy the fragment packet data to new packet */
    newpkt->dat = newpkt->hdr + newpkt->hdr_len;
    newpkt->dat_len = ipr->tlen;
    ptr = newpkt->dat;

    frgpkt = (T_NET_BUF *)ipr->ipq;
    for (i=0;i<ipr->cnt;i++) {
        if (frgpkt == NULL) {
            NET_ERR(printf("\r\n ERR: ip4_reassembly: NULL ACCESS"));
            net_buf_ret(newpkt);
            goto IPR_CMPLT;
        }
        if (frgpkt->seq + frgpkt->dat_len > newpkt->dat_len) {
            NET_ERR(printf("\r\n ERR: ip4_reassembly: bogus IP Fragment %d", frgpkt->seq + frgpkt->dat_len));
            ercd = E_OBJ;
            net_buf_ret(newpkt);
            goto IPR_CMPLT;
        }
        net_memcpy(ptr + frgpkt->seq, (char*)(frgpkt->dat), frgpkt->dat_len);
        frgpkt = (T_NET_BUF *)frgpkt->next;
    }

    /* Handover the reassembled packet to IP layer */
    *pkt = (T_NET_BUF *)newpkt;

    ercd = E_OK;

IPR_CMPLT:

    /* Release Fragment Queue */

    for (i=0;i<ipr->cnt;i++) {
        frgpkt = (T_NET_BUF *)ipr->ipq;
        ipr->ipq = frgpkt->next;
        frgpkt->next = NULL;
        net_buf_ret(frgpkt);
    }

    /* Free Reassembly hash queue */
    ipr->cnt = 0;

    return ercd;
}

/*
    This IP Fragmentation routine, assumes that upper layer send IP packet in
    the following format.
    pkt->buf[2]  - Link Header
    pkt->hdr     - IP Packet (Header + Data)
    pkt->hdr_len - IP Packet len
    pkt->dat     - NULL
    pkt->dat_len - 0
    IP Options are copied to first fragment

    This function never return E_OK or E_WBLK, thus
    the upper layer releases the original packet (UDP/ICMP)
*/

ER ipf_snd(T_NET_BUF *pkt)
{
    T_NET *net;
    T_NET_DEV *dev;
    T_ETH_HDR *eth;
    T_IP4_HDR *ip4hdr, *frg_hdr;
    T_NET_BUF *frg;
    UH ihlen, frg_len, offset;
    INT tot_len;
    ER ercd;
    UB *pkt_dat;
    ID mpfid;

    /* IP Fragment process is non-blocking, */
    /* so snd_soc() API wont blocked        */
    /*pkt->soc = NULL;*/

    net = pkt->net;

    ip4hdr = (T_IP4_HDR*)pkt->hdr;
    if (ip4hdr == NULL) {
        pkt->ercd = E_SYS;
        return E_SYS;
    }

    ihlen = (ip4hdr->ver & 0x0F) * 4;
    if (ihlen < IP4_HDR_SZ) {
        pkt->ercd = E_SYS;
        return E_SYS;
    }

    ip4hdr->tl = ntohs(ip4hdr->tl);
    ip4hdr->fo = ntohs(ip4hdr->fo);

    tot_len = ip4hdr->tl;   /* IP Header + Payload */
    frg_len = PATH_MTU;     /* Max Fragment payload */

    pkt_dat = pkt->hdr + ihlen;
    offset = 0;

    /* if. the hardware checksum flag is Enable. */
    if ( pkt->flg & HW_CS_TX_DATA ) {
        /* Set software checksum */
        pkt->dat = pkt_dat;
        pkt->dat_len = pkt->hdr_len - ihlen;
        switch ( ip4hdr->prot ) {
#ifdef TCP_SUP
        case IP_PROTO_TCP:
            /* set software checksum for tcp */
            ((T_TCP_HDR *)pkt->dat)->cs = 0;
            ((T_TCP_HDR *)pkt->dat)->cs = tcp_csum(pkt);
            break;
#endif
#ifdef UDP_SUP
        case IP_PROTO_UDP:
            /* set software checksum for udp */
            ((T_UDP_HDR *)pkt->dat)->cs = 0;
            ((T_UDP_HDR *)pkt->dat)->cs = udp_csum(pkt);
            if ( ((T_UDP_HDR *)pkt->dat)->cs == 0 ) {
                /* no checksum */
                ((T_UDP_HDR *)pkt->dat)->cs = 0xFFFF;
            }        
            break;
#endif
#ifdef PING_SUP
        case IP_PROTO_ICMP:
            /* set software checksum for icmp */
            ((T_ICMP_HDR *)pkt->dat)->cs = 0;
            ((T_ICMP_HDR *)pkt->dat)->cs = icmp_csum(pkt);
            break;
#endif
        default:;
        }
        pkt->dat = NULL;
        pkt->dat_len = 0;
    }

    while (tot_len > IP4_HDR_SZ) {

        /* Allocate fragment packet */
        ercd = net_buf_get(&frg, IP4_BUF_SIZE(frg_len - ihlen), TMO_POL);
        if (ercd != E_OK) {
            pkt->ercd = E_NOMEM;
            break;
        }

        /* Create Fragment packet */

        /* Copy Control Header */
        /* excpt mpfid */
        mpfid = frg->mpfid;
        net_memcpy((char*)frg, (char*)pkt, sizeof(T_NET_BUF));
        frg->mpfid = mpfid;

        /* Copy Link Header */
        frg->hdr     = frg->buf + net->dev->hhdrofs;
        frg->hdr_len = net->dev->hhdrsz;
        net_memcpy((char*)frg->hdr, (char*)&pkt->buf[net->dev->hhdrofs], frg->hdr_len);

        /* Copy IP Header */
        frg->hdr = frg->hdr + frg->hdr_len;
        frg->hdr_len = ihlen;
        net_memcpy((char *)frg->hdr, (char *)ip4hdr, ihlen);

        /* Update IP header */
        frg_hdr     = (T_IP4_HDR *)frg->hdr;
        frg_hdr->tl = htons(frg_len);
        frg_hdr->fo = (offset >> 3);
        if (tot_len > PATH_MTU) {
            frg_hdr->fo |= (IP_FLG_MF << 13);   /* More fragments to be send */
        }
        frg_hdr->fo = htons(frg_hdr->fo);
        frg_hdr->hc = 0;
        frg->flg   &= ~(HW_CS_TX_DATA);
        if (!(frg->flg & HW_CS_TX_IPH4)) {
            frg_hdr->hc = ip4_csum(frg);
        }

        /* Copy IP data */
        frg->hdr = frg->hdr + frg->hdr_len;
        frg->hdr_len = frg_len;
        net_memcpy((char*)frg->hdr, pkt_dat, (frg_len - ihlen));
        pkt_dat += (frg_len - ihlen);

        /* Set variables for next fragments */

        offset = offset + (frg_len - ihlen);

        /* Adjust next fragment length */
        tot_len -= frg_len;

        /* Do not set IP Options in subsequent fragments */
        if (ihlen > IP4_HDR_SZ) {
            ihlen = IP4_HDR_SZ;
        }
        tot_len += ihlen;

        if (tot_len < PATH_MTU) {
            frg_len = tot_len;
        }

        /* Transmit to network */
        dev = net->dev;

        frg->soc = NULL;

        if (dev->type == NET_DEV_TYPE_ETH) {
            frg->hdr = &frg->buf[0] + dev->hhdrofs;
            frg->hdr_len += net->dev->hhdrsz;
            eth = (T_ETH_HDR*)frg->hdr;
            eth->type = htons(ETH_TYPE_IP4);
            net_memcpy(eth->sa, dev->cfg.eth.mac, ARP_HA_LEN);
        } else if (dev->type == NET_DEV_TYPE_PPP) {
            frg->hdr = &frg->buf[0] + dev->hhdrofs + dev->hhdrsz;
        }
#ifdef LO_IF_SUP
        if ((pkt->flg & LOCAL_LOOPBK) && net_inftbl[NETINF_LOOP_IF]) {
            LO_PKT_HDL* hdl = (LO_PKT_HDL*)&net_inftbl[NETINF_LOOP_IF];
            ercd = (*hdl)(0, pkt);
        }
        else
#endif
        ercd = dev->out(dev->num, frg);
        net_sts_inc(NET_STS_IP, NET_STS_IP_FRAG_CREATE);
        if (ercd == E_WBLK) {
            continue;
        }

        net_buf_ret(frg);
        if (ercd < E_OK) {
            return ercd;
        } 
    }
    return pkt->ercd;
}
#endif
