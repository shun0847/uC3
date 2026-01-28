/***************************************************************************
    MICRO C CUBE / COMPACT, NETWORK
    ARP Protocol
    Copyright (c)  2008-2024, eForce Co., Ltd. All rights reserved.

    Version Information
      2008.11.19: Created
      2009.03.03: RARP Reply support
      2010.07.20: Should not release network buffer when
                  net->out() returns E_OK or E_WBLK
      2010.07.20: Multicast group address should be used
                  to set the destination MAC address.
      2010.08.10: Updated for broadcast IP address check
      2011.03.30: Updated for TX blocking device
      2011.05.20: Set up networkbuffer offset
      2011.11.09: Implemented Address Conflict Detection
      2012.06.14: Corrected THA of ARP Announce
      2012.07.10: Corrected reply to GARP in configuration
                  which callback function is not set.
      2012.10.02  Modify to avoid use of string libraries.
      2012.10.18: Reversed order of transmitted packet que
      2013.09.25: Implememted local loopback
      2014.01.30: Implemented arp_rmv_que()
      2014.01.30: Implemented arp_static(), arp_ref() 
      2014.03.06: SNMP option was added
      2014.04.04: Modify to set pkt->net always sending
      2015.05.11: Implemented arp_req() and arp_set()
      2016.02.05: Implemented arp_clr()
      2016.02.19: Change the size specifying when to get network buffer
      2017.10.27: Add option to zero THA
      2018.08.23: Support POSIX IP_MULTICAST_LOOP feature
      2018.12.14: Update arp_set() and add arp_del()
      2021.01.29: Add custom garp mode
      2021.02.05: Deny reception after stopping protocol stack
      2022.10.12: Fixed an issue where incorrect CustomGARP was sent.
      2023.11.22: Distinguish between ARP Probe and Directed ARP (RFC1433)
      2024.03.21: No response to ARP Request during net_acd() processing.
      2024.03.21: Deactivate interface when an address conflict is detected.
 ***************************************************************************/

#include <stdio.h>
#include "kernel.h"
#include "net_hdr.h"
#include "net_def.h"

/*
    Initialize ARP Resources
*/

void arp_ini(void)
{
    UH i;

    for (i=0;i<NET_ARP_MAX;i++) {
        gNET_ARP[i].sts = ARP_UNUSED;
    }
}

/*
    This routine is called whenever an ARP-IP Entry is resolved.
    Any IP packet queued for that entry is removed from the queue
    and transmitted to the network.
*/

static void arp_clr_que(T_NET_ARP *arp, ER err)
{
    T_NET_BUF *pkt;

    for (;;) {

        if (arp->ipq == NULL)
            break;

        pkt = (T_NET_BUF *)arp->ipq;
        arp->ipq = pkt->next;
        pkt->next = NULL;
        if (err == E_OK) {
            /* Transmit to network */
            arp->net->out(pkt);
        }
        else {
            /* Releae the packet and notify error */
            pkt->ercd = err;
            net_buf_ret(pkt);
        }
    }

}

static T_NET_ARP *arp_new_entry(void)
{
    T_NET_ARP *arp, *new_arp;
    UH i;
    UW tmo;

    /* Try to get a unused entry */
    new_arp = NULL;
    for (i=0;i<NET_ARP_MAX;i++) {
        arp = &gNET_ARP[i];
        if (arp->sts == ARP_UNUSED) {
            new_arp = arp;
            break;
        }
    }

    if (new_arp != NULL) {
        return new_arp;
    }

    /* Try to get an aged entry */
    tmo = 0;
    for (i=0;i<NET_ARP_MAX;i++) {
        arp = &gNET_ARP[i];
        if (arp->sts == ARP_VALID) {
            if (tmo == 0 || (TIM_LEQ(arp->tmo, tmo))) {
                tmo = arp->tmo;
                new_arp = arp;
            }
        }
    }

    if (new_arp != NULL) {
        new_arp->sts = ARP_UNUSED;
        new_arp->chg_flg = NET_STS_CHG;
        return new_arp;
    }

    /* No entry found */
    return NULL;
}

/*
    Creates a new IP-ARP entry in ARP Cache. If ARP Cache is already
    full then an oldest entry is used.
*/
static ER arp_create(T_NET *net, UW ip, UB *mac)
{
    T_NET_ARP *arp;

    arp = arp_new_entry();
    if (arp == NULL) {
        NET_LOG(printf("\n ARP Entries are FULL"));
        return E_NOMEM;
    }

    /* Update ARP entry */
    arp->sts = ARP_VALID;
    arp->net = net;
    arp->cnt = 0;
    arp->tmo = ARP_CLR_TMO + NET_TICK;
    arp->ipq = NULL;

    arp->ipaddr = ip;
    net_memcpy(arp->mac, mac, 6);
    arp->chg_flg = NET_STS_CHG;

    NET_LOG(printf("\n arp_create 0x%x",ip));
    return E_OK;
}

/*
    This routine is called whenever an ARP packet is received from network.
    If the received packet's IP address is already exists in ARP Cache then
    the mac address is updated in the ARP Cache.
*/

static ER arp_update(T_NET *net, UW ip, UB *mac)
{
    T_NET_ARP *arp;
    UH i;

    /* Update if entry already exists */
    for (i=0;i<NET_ARP_MAX;i++) {

        arp = &gNET_ARP[i];

        if ((arp->sts != ARP_UNUSED) && (arp->net == net) && (arp->ipaddr == ip)) {

            if (arp->sts == ARP_VALID) {
                net_memcpy(&gNET_ARP[i].mac, mac, 6);
                arp->tmo = ARP_CLR_TMO + NET_TICK;
                arp->chg_flg = NET_STS_CHG;
                NET_LOG(printf("\n Val: arp_update[%d] 0x%x", i, ip));
                return E_OK;
            }

            if (arp->sts == ARP_RESOLVING) {
                /* Release IP Queue */
                arp->sts = ARP_VALID;
                arp->cnt = 0;
                arp->tmo = ARP_CLR_TMO + NET_TICK;
                net_memcpy(&gNET_ARP[i].mac, mac, 6);
                if (arp->ipq) {
                    arp_clr_que(arp, E_OK);
                }
                arp->ipq = NULL;
                arp->chg_flg = NET_STS_CHG;
                NET_LOG(printf("\n Res: arp_update[%d] 0x%x", i, ip));
                return E_OK;
            }
#ifdef ARP_API_SUP
            if (arp->sts & ARP_STATIC) {
                /* no effect */
                return E_OK;
            }
#endif
        }
    }

    return E_OBJ;
}

/*
    This 1 sec cyclic timer routine checks all the timer events for ARP.
    ARP Request is retransmitted for Resolving entries after ARP_RET_TMO timeout.
    Resolving entries are cleared after ARP_RET_CNT number of retransmissions.
    Valid entries are cleared after ARP_CLR_TMO timeout.
*/
void arp_timer(UW ctick)
{
    T_NET     *net;
    T_NET_BUF *pkt;
    T_NET_ARP *arp;
    UH i;

    for (i=0;i<NET_ARP_MAX;i++) {

        arp = &gNET_ARP[i];

#ifdef ARP_API_SUP
        if (arp->sts & ARP_STATIC) {
            continue;
        }
#endif
        if (arp->sts & 0x3) {   /* ARP_VALID | ARP_RESOLVING */
            if (TIM_LEQ(arp->tmo, ctick)) {

                net = arp->net;

                if (arp->sts == ARP_VALID) {
                    arp->sts = ARP_UNUSED;
                    arp->chg_flg = NET_STS_CHG;
                    NET_LOG(printf("\r\n ARP_TIMER:[%d] Cache Cleared 0x%x", i, arp->ipaddr));
                }
                else {
                    /* ARP Retry */
                    pkt = (T_NET_BUF *)arp->ipq;
                    if (pkt == NULL) {
                        arp->sts = ARP_UNUSED;          /* No request to send? */
                        NET_LOG(printf("\r\n ARP_TIMER[%d]: Cache Idle %x", i, arp->ipaddr));
                    }
                    else if (arp->cnt >= ARP_RET_CNT) {
                        arp_clr_que(arp, E_TMOUT);      /* Retry count Expires */
                        arp->ipq = NULL;
                        arp->sts = ARP_UNUSED;
                        NET_LOG(printf("\r\n ARP_TIMER[%d]: Retry Timeout %x", i, arp->ipaddr));
                    }
                    else {
                        arp->cnt++;
                        arp->tmo = ARP_RET_TMO + ctick;
                        arp_send(arp->net, arp->ipaddr, ARP_RESOLVE_IP);
                        NET_LOG(printf("\r\n ARP_TIMER[%d]: Retry 0x%x Cnt %d", i, arp->ipaddr, arp->cnt));
                    }
                }
            }
        }

    }
}

/*
    This routine resolves the MAC address for a given IP address.
    If the Target IP address is,
        Broadcast IP then set MAC as FF:FF:FF:FF:FF:FF
        Multicast IP then set MAC as Multicast MAC
        Unicast   IP then search the target IP in ARP Cache and set the corresponding MAC.
           If an entry is not found then Broadcast an ARP Request for that target IP address
           and queue that packet to process after resolving the IP address.
           If the IP address is not a local network address then the packet is directed to the
           Gateway address.
*/

ER arp_resolve(T_NET_BUF *pkt)
{
    T_NET     *net;
    T_NET_ADR *adr;
    T_NET_ARP *arp;
    T_IP4_HDR *ip4hdr;
    T_ETH_HDR *eth;
    UH i;
    UW ip4addr; 

    net = pkt->net;
    adr = net->adr;
    eth    = (T_ETH_HDR*)(&pkt->buf[0] + net->dev->hhdrofs);
    ip4hdr = (T_IP4_HDR*)pkt->hdr;
    ip4addr = htonl(ip4hdr->da);

    /* Learn destination MAC address */

    if (IS_MCAST_IP(ip4addr)) {
        /* Multicast IP -> IEEE802 Multicast MAC */
        /* Map 24 Bit OUI -> 01-00-5E            */

        eth->da[0] = 0x01;
        eth->da[1] = 0x00;
        eth->da[2] = 0x5E;
        eth->da[3] = (UB)((ip4addr >> 16) & 0x7F);
        eth->da[4] = (UB)((ip4addr >> 8) & 0xFF);
        eth->da[5] = (UB)(ip4addr & 0xFF);

        if (net->flag & MCAST_LOOPBK) {
            pkt->flg |= (LOCAL_LOOPBK | PKT_FLG_MCAST);
        }
        return E_OK;
    }

    if (IS_BCAST_IP(ip4addr, adr->ipaddr, adr->mask)) {
        eth->da[0] = 0xFF;
        eth->da[1] = 0xFF;
        eth->da[2] = 0xFF;
        eth->da[3] = 0xFF;
        eth->da[4] = 0xFF;
        eth->da[5] = 0xFF;

        return E_OK;
    }

#ifdef LO_IF_SUP
    if (ip4addr == adr->ipaddr || IS_LBACK_IP(ip4addr)) {
        pkt->flg |= LOCAL_LOOPBK;

        return E_OK;
    }
#endif

    /* Gateway Address */
    if (!(IS_ROUTE_IP(ip4addr, adr->ipaddr, adr->mask))) {
        ip4addr = adr->gateway;
        if (ip4addr == 0) {
            NET_ERR(printf("\r\n ERR: arp_resolve: Gateway Not Specified!"));
            return EV_ADDR;
        }
    }

    /* IP-MAC Already resolved? */
    for (i=0;i<NET_ARP_MAX;i++) {

        arp = &gNET_ARP[i];

        if ((arp->sts != ARP_UNUSED) && (arp->net == net) && (arp->ipaddr == ip4addr)) {

            if (arp->sts & ARP_VALID) {
                net_memcpy(eth->da, arp->mac, 6);
                return E_OK;
            }

            if (arp->sts == ARP_RESOLVING) {
                /* Add to ARP Queue */
                add2que_end(&arp->ipq, pkt);
                NET_LOG(printf("\r\n arp_resolve: Que 0x%x nbuf 0x%x", arp->ipaddr, pkt));
                return E_WBLK;
            }
        }
    }

    /* ARP Request */
    arp_send(net, ip4addr, ARP_RESOLVE_IP);

    arp = arp_new_entry();
    if (arp == NULL) {
        NET_ERR(printf("\r\n arp_resolve E_NOMEM"));
        return E_NOMEM; /* No memory */
    }

    /* Set entry as ARP_RESOLVING */
    arp->ipaddr = ip4addr;
    arp->sts = ARP_RESOLVING;
    arp->net = net;
    arp->cnt = 0;
    arp->tmo = ARP_RET_TMO + NET_TICK;
    pkt->next = NULL;
    arp->ipq = (UW *)pkt;

    NET_LOG(printf("\r\n arp_resolve: ARP_RESOLVE 0x%x", arp->ipaddr));

    return E_WBLK;
}

/*
    Process the ARP packets received from network.
*/
void arp_recv(T_NET_BUF *pkt)
{
    T_NET     *net;
    T_ETH_HDR *eth;
    T_ARP_HDR *arphdr;
    UW t_ip, spa, tpa;
    ER ercd;
    T_RTST tst;
#ifdef ACD_SUP
    T_NET_ACD acd;
    UB snd_cgarp;
    snd_cgarp = 0;
    net_memset(&acd, 0, sizeof(T_NET_ACD));
#endif

    ref_tst(ID_NET_TSK, &tst);
    if (tst.tskstat == TTS_DMT) {
        NET_ERR(printf("\r\n ERR: arp_recv: uNet3 Stopped"));
        net_buf_ret(pkt);
        return;
    }

    net = pkt->net;

    if (net->adr->ipaddr == 0) {                         /* BOOTP Protocol */
        NET_ERR(printf("\r\n ERR: arp_recv: ARP Disabled"));
        net_buf_ret(pkt);
        return;
    }

    /* 1. Validate receive length  */

    if (pkt->dat_len < ARP_HDR_SZ) {
        NET_ERR(printf("\r\n ERR: arp_recv: Invalid pkt len %d", pkt->dat_len));
        net_buf_ret(pkt);
        return;
    }

    eth    = (T_ETH_HDR*)pkt->hdr;
    arphdr = (T_ARP_HDR*)pkt->dat;

    /* 2. Validate ARP Header Fields  (RFC826) */
    if ((htons(arphdr->hw) != ARP_TYPE_ETH)   ||
        (htons(arphdr->proto) != ARP_PROTO_IP)||
        (arphdr->ha_len  != ARP_HA_LEN)  ||
        (arphdr->pa_len  != ARP_PA_LEN)) {

        /* Ignore packet */
        net_buf_ret(pkt);

        NET_ERR(printf("\r\n ERR: arp_recv: Invalid header "));
        return;
    }

#ifdef RARP_SUP
        /* RARP Request for host MAC ?      */
        /* Send a RARP reply with host IP   */
        if (htons(arphdr->op) == ARP_OPC_RREQ) {

            /* Request should match with host mac address */
            if (net_memcmp(arphdr->tha, net->dev->cfg.eth.mac, ARP_HA_LEN) != 0) {
                net_buf_ret(pkt);
                return;
            }

            /* Host IP Address */
            t_ip = htonl(net->adr->ipaddr);

            /* Send RARP Reply */
            arphdr->op = htons(ARP_OPC_RREP);
            net_memcpy(arphdr->tha, arphdr->sha, ARP_HA_LEN);
            net_memcpy(arphdr->sha, net->dev->cfg.eth.mac, ARP_HA_LEN);
            net_memcpy(arphdr->tpa, arphdr->spa, ARP_PA_LEN);
            net_memcpy(arphdr->spa, (char*)&t_ip, ARP_PA_LEN);

            net_memcpy(eth->da, arphdr->tha, ARP_HA_LEN);
            net_memcpy(eth->sa, arphdr->sha, ARP_HA_LEN);
            eth->type = htons(ETH_TYPE_ARP);

            pkt->dat = NULL;
            pkt->hdr_len = ETH_HDR_SZ + ARP_HDR_SZ;
            ercd = net->dev->out(net->dev->num, pkt);
            if (ercd != E_WBLK) {
                net_buf_ret(pkt);
            }
            return;
        }
#endif

    /* 5. Update ARP Cache Entry (RFC 826) */
    spa = ip_byte2n((char*)arphdr->spa);

#ifdef ACD_SUP
    if ((spa == net->adr->ipaddr) &&
        (net_memcmp(arphdr->sha, net->dev->cfg.eth.mac, ARP_HA_LEN) != 0)) {

        if ((net->prbtskid != 0) && (net->acd != NULL)) {
            net->acd->ip = spa;
            net->flag |= NET_IF_INACT; /* deactivate network interface */
            net_memcpy(net->acd->mac, arphdr->sha, ARP_HA_LEN);
            wup_tsk(net->prbtskid);
            net_buf_ret(pkt);
            return;
        }

        if (net->acdcbk != NULL) {
            acd.ip = spa;
            net_memcpy(acd.mac, arphdr->sha, ARP_HA_LEN);
            ercd = net->acdcbk(&acd);
            if (ercd <= E_OK) {
                if (ercd == E_OK) {
                    arp_send(net, spa, ARP_ANNOUNCE_IP);
                } else {
                    net->flag |= NET_IF_INACT; /* deactivate network interface */
                }
                net_buf_ret(pkt);
                return;
            }
            /* ACD callback returned a positive number (custom garp mode) */
            snd_cgarp = 1;
        }
    }
#endif

    /*spa = htonl(spa);*/ /* 20090518 AK ip_byte2n */
    ercd = arp_update(net, spa, &arphdr->sha[0]); /* Update the entry if already exists */

    /* 6. Create an ARP Entry (RFC 826) */

    /* Should be same as Host IP Address */
    tpa = ip_byte2n((char*)arphdr->tpa);
    /*t_ip = htonl(tpa); */ /* 20090518 AK ip_byte2n */
    if (tpa == net->adr->ipaddr) {

#ifdef ACD_SUP
        if ((net->prbtskid != 0) && (net->acd != NULL)) {
            if ((spa == 0) &&
                (net_memcmp(arphdr->tha, net->dev->cfg.eth.mac, ARP_HA_LEN) != 0) && /* not direct */
                (net_memcmp(arphdr->sha, net->dev->cfg.eth.mac, ARP_HA_LEN) != 0)) {
                net->acd->ip = tpa;
                net->flag |= NET_IF_INACT; /* deactivate network interface */
                net_memcpy(net->acd->mac, arphdr->sha, ARP_HA_LEN);
                wup_tsk(net->prbtskid);
            }
            net_buf_ret(pkt);
            return;
        }
#endif
        if (ercd != E_OK) { /* IF entry is not already available create it now */
            ercd = arp_create(net, spa, &arphdr->sha[0]);
            if (ercd != E_OK) {
                NET_LOG(printf("\r\n arp_create ercd %d", ercd));
            }
        }

        if (htons(arphdr->op) == ARP_OPC_REQ) {

            NET_LOG(printf("\r\n arp_reply 0x%x", arphdr->spa));

            /* Send ARP Reply   (RFC 826)   */
            t_ip = htonl(tpa);
            arphdr->op = htons(ARP_OPC_REP);
            net_memcpy(arphdr->tha, arphdr->sha, 6);
            net_memcpy(arphdr->sha, net->dev->cfg.eth.mac, 6);
            net_memcpy(arphdr->tpa, arphdr->spa, 4);
            net_memcpy(arphdr->spa, (char*)&t_ip, 4);

            net_memcpy(eth->da, arphdr->tha, ARP_HA_LEN);
            net_memcpy(eth->sa, arphdr->sha, ARP_HA_LEN);
            eth->type = htons(ETH_TYPE_ARP);

            pkt->dat = NULL;
            pkt->hdr_len = ETH_HDR_SZ + ARP_HDR_SZ;
            ercd = net->dev->out(net->dev->num, pkt);
            if (ercd != E_WBLK) {
                net_buf_ret(pkt);
                return;
            }
#ifdef ACD_SUP
            /* Custom ARP announce */
            if (snd_cgarp) {
                if (acd.ip == spa) {
                    ulc_tcp();
                    tslp_tsk(ARP_ANC_WAI);
                    loc_tcp();
                    arp_send(net, spa, ARP_CUSTOM_GARP);
                }
            }
#endif
            return;
        }
    }

    net_buf_ret(pkt);

    return;
}

/*
    This routine is called by upper layer to transmit a broadcast ARP Request.
*/

ER arp_send(T_NET *net, UW tpa, UB type)
{
    T_NET_BUF *pkt;
    T_ETH_HDR *eth;
    T_ARP_HDR *arphdr;
    ER ercd;
    UW own;
    UB *mac;
    UB tha;

    mac = net->dev->cfg.eth.mac;
    own = net->adr->ipaddr;

    if (own == 0) {     /* BOOTP Protocol */
        return E_OBJ;
    }

    /* Allocate memory for nbuf */

    ercd = net_buf_get(&pkt, ARP_BUF_SIZE, TMO_POL);
    if (ercd != E_OK) {
        return E_NOMEM;
    }

    pkt->hdr += net->dev->hhdrofs;
    arphdr = (T_ARP_HDR*)(pkt->hdr + ETH_HDR_SZ);

    /* Construct ARP Request    */

    arphdr->hw    = htons(ARP_TYPE_ETH);
    arphdr->proto = htons(ARP_PROTO_IP);
    arphdr->ha_len = ARP_HA_LEN;
    arphdr->pa_len = ARP_PA_LEN;

    arphdr->op   = htons(ARP_OPC_REQ);

    own    = htonl(own);
    tpa    = htonl(tpa);
    if (ARP_THA_ZERO) {
        tha    = 0x00;
    } else {
        tha    = 0xFF;
    }
#ifdef ACD_SUP
    if (type == ARP_PROBE_IP) {
        own = 0;
        tha = 0;    /* RFC 5227 2.1.1 Probe Details*/
    } else if (type == ARP_ANNOUNCE_IP) {
        tha = 0;    /* RFC 5227 2.3/2.4(b) */
    }
#endif

    net_memcpy(arphdr->spa, (char*)&own, ARP_PA_LEN);
    net_memcpy(arphdr->tpa, (char*)&tpa, ARP_PA_LEN);
    net_memcpy(arphdr->sha, mac, ARP_HA_LEN);
#ifdef ACD_SUP
    if (type == ARP_CUSTOM_GARP) {
        net_memcpy(arphdr->tha, mac, ARP_HA_LEN);
    } else
#endif
    {
        net_memset(arphdr->tha, tha, ARP_HA_LEN);       /* May set as Broadcast address RFC 816 */
    }

    /* Prepare to send out */
    eth = (T_ETH_HDR*)pkt->hdr;
    eth->type = htons(ETH_TYPE_ARP);
    net_memset(&eth->da, (-1), ARP_HA_LEN);
    net_memcpy(&eth->sa, mac, ARP_HA_LEN);

    pkt->dat = NULL;
    pkt->hdr_len = ARP_HDR_SZ + ETH_HDR_SZ;
    pkt->net = net;
    ercd = net->dev->out(net->dev->num, pkt);
    if (ercd == E_WBLK) {
        ercd = E_OK;
    } else {
        net_buf_ret(pkt);
    }
    return ercd;
}

void arp_rmv_que(T_NET_BUF *pkt, ER err)
{
    T_NET_ARP *arp;
    T_NET_BUF **ipqp;
    UH i;

    for (i=0;i<NET_ARP_MAX;i++) {
        arp = &gNET_ARP[i];
        if ((arp->sts&ARP_RESOLVING) == 0 || arp->ipq == NULL) {
            continue;
        }
        ipqp = (T_NET_BUF**)&arp->ipq;
        while (*ipqp) {
            if (*ipqp == pkt) {
                *ipqp = (T_NET_BUF*)pkt->next;
                pkt->ercd = err;
                net_buf_ret(pkt);
                break;
            }
            ipqp = (T_NET_BUF**)&(*ipqp)->next;
        }
    }
}

#ifdef ARP_API_SUP
ER arp_set(UH dev_num, UW ip, UB *mac)
{
    T_NET_ARP *arp_ent, *arp;
    T_NET *net;
    UH i;
    ER ercd;

    if ((dev_num == 0) || (dev_num > NET_DEV_MAX)) {
        return E_ID;
    }

    if ((ip == 0) || (mac == NULL)) {
        return E_PAR;
    }

    arp = NULL;
    ercd = E_NOMEM;

    loc_tcp();

    net = &gNET[dev_num-1];
    for (i = 0; i < NET_ARP_MAX; i++) {
        arp_ent = &gNET_ARP[i];
        if ((arp_ent->sts & ARP_STATIC) && (arp_ent->net == net) && (arp_ent->ipaddr == ip)) {
            net_memcpy(arp_ent->mac, mac, 6); /* update mac address only */
            arp_ent->chg_flg = NET_STS_CHG;
            ercd = E_OK;
            break;
        }
        if ((arp == NULL) && (arp_ent->sts == ARP_UNUSED)) {
            arp = arp_ent;
        }
    }

    if ((i == NET_ARP_MAX) && (arp != NULL)) {
        /* new entry */
        arp->sts = (ARP_STATIC | ARP_VALID);
        arp->net = net;
        arp->cnt = 0;
        arp->tmo = 0;
        arp->ipq = NULL;
        arp->ipaddr = ip;
        net_memcpy(arp->mac, mac, 6);
        arp->chg_flg = NET_STS_CHG;
        ercd = E_OK;
    }

    ulc_tcp();
    return ercd;
}

ER arp_ref(UH dev_num, UW ip, UB *mac)
{
    T_NET_ARP *arp;
    T_NET *net;
    UH i;

    if ((dev_num == 0) || (dev_num > NET_DEV_MAX)) {
        return E_ID;
    }

    loc_tcp();
    net = &gNET[dev_num-1];

    for (i=0;i<NET_ARP_MAX;i++) {
        arp = &gNET_ARP[i];
        if ((arp->sts & ARP_VALID) && (arp->net == net) && (arp->ipaddr == ip)) {
            net_memcpy(mac, arp->mac, 6);
            ulc_tcp();
            return E_OK;
        }
    }
    ulc_tcp();
    return E_NOEXS;
}

ER arp_req(UH dev_num, UW ip)
{
    T_NET *net;
    ER ercd;

    if ((dev_num == 0) || (dev_num > NET_DEV_MAX)) {
        return E_ID;
    }

    loc_tcp();
    net = &gNET[dev_num-1];
    if (net->dev->type != NET_DEV_TYPE_ETH) {
        ulc_tcp();
        return E_PAR;
    }

    ercd = arp_send(net, ip, ARP_RESOLVE_IP);
    ulc_tcp();

    return ercd;
}

ER arp_clr(void)
{
    T_NET_ARP *arp;
    UH i;

    loc_tcp();
    for (i=0;i<NET_ARP_MAX;i++) {
        arp = &gNET_ARP[i];
        if (arp->sts != ARP_RESOLVING) {
            arp->sts = ARP_UNUSED;
            arp->chg_flg = NET_STS_CHG;
            if (arp->ipq) {
                net_buf_ret((T_NET_BUF *)arp->ipq);
                arp->ipq = 0;
            }
        }
    }
    ulc_tcp();
    return E_OK;
}

ER arp_del(UH dev_num, UW ip)
{
    T_NET_ARP *arp_ent;
    T_NET *net;
    UH i;

    if ((dev_num == 0) || (dev_num > NET_DEV_MAX)) {
        return E_ID;
    }

    loc_tcp();

    net = &gNET[dev_num-1];
    for (i = 0; i < NET_ARP_MAX; i++) {
        arp_ent = &gNET_ARP[i];
        if ((arp_ent->sts & ARP_STATIC) && (arp_ent->net == net) && (arp_ent->ipaddr == ip)) {
            arp_ent->sts = ARP_UNUSED;
            arp_ent->sts = ARP_UNUSED;
            arp_ent->chg_flg = NET_STS_CHG;
            if (arp_ent->ipq) {
                net_buf_ret((T_NET_BUF *)arp_ent->ipq);
                arp_ent->ipq = 0;
            }
        }
    }
    ulc_tcp();
    return E_OK;
}

#endif

