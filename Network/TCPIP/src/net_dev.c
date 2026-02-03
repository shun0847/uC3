/***************************************************************************
    MICRO C CUBE / COMPACT, NETWORK
    Device Driver Interface
    Copyright (c)  2008-2024, eForce Co., Ltd. All rights reserved.

    Version Information  2008.11.19: Created
                         2010.01.18: Added ppp_pkt_out(), ppp_pkt_rcv()
                         2010.11.02: Updated for IPv6 support
                         2011.03.30: Updated for TX blocking device
                         2011.05.20: Set up networkbuffer offset
                         2012.10.02  Modify to avoid use of string libraries.
                         2013.09.25: Implememted local loopback
                         2014.03.06: SNMP option was added
                         2018.06.13: Cleared the next field in pkt not to be
                                     used in receive process.
                         2018.08.23: Support POSIX IP_MULTICAST_LOOP feature
                         2021.03.17: Added a condition for selecting the default
                                     device (device number is 0).
                         2021.03.17: Added random seed update process
                         2024.03.18: Add NET_IF_INACT for interface inactivate setting.
 ***************************************************************************/

#include <stdio.h>
#include "kernel.h"
#include "net_hdr.h"
#include "net_def.h"
#include "net_sts_id.h"

T_NET *get_net_bynum(UH dev_num)
{
    T_NET *net;

    if (dev_num >= NET_DEV_MAX) {
        return NULL;
    }

    net = &gNET[dev_num];

    return net;
}

T_NET *get_net_default(void)
{
    T_NET *net;
    UH dev_num;

    for (dev_num = 0; dev_num < NET_DEV_MAX; dev_num++) {
        net = &gNET[dev_num];
        if (net->dev->sts != NET_DEV_STS_NON) {
            break;
        }
        net = NULL;
    }
    return net;
}


ER net_dev_ini(UH dev_num)
{
#ifdef MCAST_SUP
    T_NET_DEV *dev;
#endif
    ER ercd;

    if ((dev_num == 0) || (dev_num > NET_DEV_MAX)) {
        return E_ID;
    }

    loc_tcp();
    ercd = gNET_DEV[dev_num-1].ini(dev_num);
    if (ercd == E_OK) {
        gNET_DEV[dev_num-1].sts = NET_DEV_STS_INI;
#ifdef MCAST_SUP
        dev = &gNET_DEV[dev_num-1];
        if (dev->ctl) {
            dev->ctl(dev->num, CFG_MC4_FIL_SET, (VP)IGMP_ALL_SYSTEMS);
        }
#endif
    }
    ulc_tcp();

    return ercd;
}

ER net_dev_cls(UH dev_num)
{
    ER ercd;

    if ((dev_num == 0) || (dev_num > NET_DEV_MAX)) {
        return E_ID;
    }

    loc_tcp();
    ercd = gNET_DEV[dev_num-1].cls(dev_num);
    ulc_tcp();

    return ercd;
}

ER net_dev_sts(UH dev_num, UH opt, VP val)
{
    ER ercd;

    if ((dev_num == 0) || (dev_num > NET_DEV_MAX)) {
        return E_ID;
    }

    loc_tcp();
    ercd = gNET_DEV[dev_num-1].ref(dev_num, opt, val);
    ulc_tcp();

    return ercd;
}

ER net_dev_ctl(UH dev_num, UH opt, VP val)
{
    ER ercd;

    if ((dev_num == 0) || (dev_num > NET_DEV_MAX)) {
        return E_ID;
    }

    loc_tcp();
    ercd = gNET_DEV[dev_num-1].ctl(dev_num, opt, val);
    ulc_tcp();

    return ercd;
}

void ppp_pkt_rcv(T_NET_BUF *pkt)
{
    ip4_rcv(pkt);
}

void eth_pkt_rcv(T_NET_BUF *pkt)
{
    T_ETH_HDR *eth;

    if (pkt->hdr_len < ETH_HDR_SZ) {
        net_buf_ret(pkt);
        return;
    }

    eth = (T_ETH_HDR *)pkt->hdr;
    if (eth == NULL) {
        net_buf_ret(pkt);
        return;
    }

    net_rand_seed(NET_TICK + eth->sa[5]);

    /* ARP */
    if (htons(eth->type) == ETH_TYPE_ARP) {
        arp_recv(pkt);
        return;
    }

    /* IP */
    if (htons(eth->type) == ETH_TYPE_IP4) {
        ip4_rcv(pkt);
        return;
    }
#ifdef IPV6_SUP /* IPv6 Packet */
    if (htons(eth->type) == ETH_TYPE_IP6) {
        ip6_rcv(pkt);
        return;
    }
#endif
    else {
        raw_pkt_rcv(pkt);
    }
}

void net_pkt_rcv(T_NET_BUF *pkt)
{
    T_NET *net;

    loc_tcp();

    pkt->next = NULL;
    net = &gNET[pkt->dev->num - 1];

    if (net->flag & NET_IF_INACT) { /* network interface is inactive */
        net_buf_ret(pkt);
    } else {
        pkt->net = net;
        if (pkt->dev->type == NET_DEV_TYPE_ETH) {
            eth_pkt_rcv(pkt);
        }
        else if (pkt->dev->type == NET_DEV_TYPE_PPP) {
            ppp_pkt_rcv(pkt);
        }
        else {
            net_buf_ret(pkt);
        }
    }
    ulc_tcp();

    return;
}

ER ppp_pkt_out(T_NET_BUF *pkt)
{
    T_NET_DEV *dev;
    T_NET *net;
    ER ercd;

    net = pkt->net;
    dev = net->dev;

    if ((dev == NULL) || (dev->type != NET_DEV_TYPE_PPP)) {
        ercd = E_OBJ;
        goto PPP_TX_CMPLT;
    }

#ifdef IPR_SUP
    /* IP Fragment */
    if (pkt->hdr_len > PATH_MTU) {
        ercd = ipf_snd(pkt);
        goto PPP_TX_CMPLT;
    }
#endif

    /* Transmit to network */
    ercd = dev->out(dev->num, pkt);
PPP_TX_CMPLT:
    if (ercd == E_WBLK) {
        return ercd;
    }
    if (ercd == E_OK) {
        ercd = pkt->ercd;
    }

    net_buf_ret(pkt);
    return ercd;
}

ER eth_pkt_out(T_NET_BUF *pkt)
{
    T_NET_DEV *dev;
    T_ETH_HDR *eth;
    T_NET *net;
#ifdef LO_IF_SUP
    T_NET_BUF *rep_pkt;
#endif
    ER ercd;
    UB ip_type = (*(pkt->hdr) & 0xF0)>>4;

    net_sts_inc(NET_STS_IP, NET_STS_IP_OUT_REQ);

    net = pkt->net;
    dev = net->dev;

    if ((dev == NULL) || (dev->type != NET_DEV_TYPE_ETH)) {
        ercd = E_OBJ;
        goto ETH_TX_CMPLT;
    }

    if (net->flag & NET_IF_INACT) { /* network interface is inactive */
        ercd = E_OBJ;
        goto ETH_TX_CMPLT;
    }

#ifdef IPV6_SUP
    if (ip_type != IP_HDR_VER6) {
#endif
    /* Resolve ARP */
    ercd = arp_resolve(pkt);
    if (ercd != E_OK) {
        net_sts_inc(NET_STS_IP, NET_STS_IP_OUT_NO_ROUTE);
        goto ETH_TX_CMPLT;
    }

#ifdef IPR_SUP
    /* IP Fragment */
    if (pkt->hdr_len > PATH_MTU) {
        ercd = ipf_snd(pkt);
        if (ercd == E_OK) {
            net_sts_inc(NET_STS_IP, NET_STS_IP_FRAG_OK);
        } else {
            net_sts_inc(NET_STS_IP, NET_STS_IP_FRAG_FAIL);
        }
        goto ETH_TX_CMPLT;
    }
#endif
#ifdef IPV6_SUP
    }
#endif

    /* Transmit to network */
    pkt->hdr = &pkt->buf[0] + dev->hhdrofs;
    pkt->hdr_len += ETH_HDR_SZ;

    eth = (T_ETH_HDR*)pkt->hdr;

    /* Setting Eth header according to IP header type */
    if(ip_type == IP_HDR_VER4)
        eth->type = htons(ETH_TYPE_IP4);
#ifdef IPV6_SUP     
    else if(ip_type == IP_HDR_VER6)
        eth->type = htons(ETH_TYPE_IP6);
#endif

    net_memcpy(eth->sa, dev->cfg.eth.mac, ARP_HA_LEN);

    /* Transmit to network */
#ifdef LO_IF_SUP
    if (pkt->flg & LOCAL_LOOPBK && net_inftbl[NETINF_LOOP_IF]) {
        LO_PKT_HDL* hdl = (LO_PKT_HDL*)&net_inftbl[NETINF_LOOP_IF];
        if (pkt->flg & PKT_FLG_MCAST) {
            /* replicate packet for loopback */
            ercd = net_buf_get(&rep_pkt, pkt->hdr_len, TMO_POL);
            if (ercd != E_OK) {
                pkt->ercd = E_NOMEM;
                goto ETH_TX_CMPLT;
            }
            rep_pkt->net     = pkt->net; /* get origin device */
            dev = rep_pkt->dev = pkt->net->dev;
            rep_pkt->hdr_len = pkt->hdr_len;
            rep_pkt->dat_len = pkt->dat_len;
            rep_pkt->hdr     = rep_pkt->buf + dev->hhdrofs;
            net_memcpy(rep_pkt->hdr, pkt->hdr, pkt->hdr_len);

            /* skip checksum for reception */
            rep_pkt->flg    |= (HW_CS_RX_IPH4 | HW_CS_RX_DATA);
            ercd = (*hdl)(0, rep_pkt);
            if (ercd != E_WBLK) {
                net_buf_ret(rep_pkt);
            }
        } else {
            ercd = (*hdl)(0, pkt);
            goto ETH_TX_CMPLT;
        }
    }
#endif

    ercd = dev->out(dev->num, pkt);
ETH_TX_CMPLT:
    if (ercd == E_WBLK) {
        return ercd;
    }
    if (ercd == E_OK) {
        ercd = pkt->ercd;
    }

    net_buf_ret(pkt);
    return ercd;
}
