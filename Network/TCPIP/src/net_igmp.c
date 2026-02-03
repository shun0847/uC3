/***************************************************************************
    MICRO C CUBE / COMPACT, NETWORK
    IGMP Protocol
    Copyright (c)  2009-2019, eForce Co., Ltd. All rights reserved.

    Version Information  2009.01.05: Created
                         2010.01.18: ETH_HDR_SZ to dev->hhdrsz
                         2010.08.10: Random message report intervals
                                     Default reception of 224.0.0.1 message
                                     Corrected IGMP leave address byte order
                         2011.03.30: Updated for TX blocking device
                         2011.05.20: Set up networkbuffer offset
                         2012.10.02  Modify to avoid use of string libraries.
                         2013.05.07  Corrected to send JOIN report it was sent
                                     from all devices
                         2013.08.06: Implememted multicast per socket
                         2014.04.21: Corrected wrong byte order in comparison
                         2016.01.20: Add processing for the evnet of IP layer
                         2016.02.19: Change the size specifying when to get network buffer
                         2016.03.19  IGMP Version 3 support
                         2017.01.18: Support 64bit processor
                         2017.06.30: Corrected initilization of 'max_rpt' in igmp_rcv().
                         2018.01.26: Fixed behavior in v1 mode at receiving v2 queries
                         2019.01.28: TTL of IGMP report is always set to 1.
                         2020.10.16: Extended number of multicast groups per socket.
                         2021.03.31: IGMPv3 queries are no longer discarded.
                         2021.04.15: Corrected the MGR during v2 retraction mode.
                         2021.04.15: Reset the compatibility timer during v2 retraction mode.
 ***************************************************************************/

#include <stdio.h>
#include "kernel.h"
#include "net_hdr.h"
#include "net_def.h"

#ifdef MCAST_SUP
#ifdef IGMPv3_SUP
extern void ini_src_flt(void);
extern void mgr_clr(T_NET_MGR *mgr);
extern void igmp_cancel_v3(T_NET *net);
extern ER igmp3_join(T_NET *net, UW ga);
extern ER igmp3_leave(T_NET *net, UW ga);
#endif

#ifndef IGMP_V3_ENABLE
#define IGMP_V3_ENABLE 0
#endif

/*
    Initialize IGMP Resources
    Join All host Group
*/
void igmp_init(void)
{
    T_NET *net;
    T_NET_MGR    *mgr;
    UH  i;

    for (i=0;i<NET_MGR_MAX;i++) {
        mgr = &pNET_MGR[i];
        mgr->state = IGMP_NON_MEMBER;
#ifdef IGMPv3_SUP
        mgr_clr(mgr);   /* reset IGMP 3 variables */
#endif
    }

    for (i=0;i<NET_DEV_MAX;i++) {
        net = &gNET[i];
        net->igmp_ver = 2;
#ifdef IGMPv3_SUP
        if (IGMP_V3_ENABLE) {
            net->igmp_ver = 3;
        }
        net->igmp3_tmo_on = 0;
#endif
        net->igmp_exists   = 0;     /* No Multicast Listener      */
    }

    /* Join All-Host-Group */

    IGMP_TIMER_ON = 0;
    IGMPV1_TIMER_ON = 0;
#ifdef IGMPv3_SUP
    IGMPV3_TIMER_ON = 0;
    ini_src_flt();  /* reset source filter list */
#endif
}

/* Compatibilty mode changing from v2 to v3. Cancel v2 timers. */
static void igmp_cancel_v2(T_NET *net)
{
    T_NET_MGR *mgr;
    UH j;

    /* stop v2 timer */
    IGMP_TIMER_ON = 0;

    /* move delay members state to idle (v3 state) */
    for (j=0;j<NET_MGR_MAX;j++) {
        mgr = &pNET_MGR[j];
        if (mgr->state == IGMP_DLY_MEMBER) {
            mgr->state = IGMP_IDLE_MEMBER;
            mgr->flg   = 1;
        }
    }
}

/*
    Check IGMP_DLY_MEMBER time out.
    When timeout
        Send Report, Set Flag and
        state = IGMP_IDLE_MEMBER
*/
void igmp_timer(UW ctimval)
{
    T_NET_MGR    *mgr;
    UH  j;

    IGMP_TIMER_ON = 0;
    for (j=0;j<NET_MGR_MAX;j++) {
        mgr = &pNET_MGR[j];
        if (mgr->state == IGMP_DLY_MEMBER) {

            IGMP_TIMER_ON = 1;

            if (TIM_LEQ(mgr->tmo,ctimval)) {
                mgr->flg   = 1;
                mgr->state = IGMP_IDLE_MEMBER;
                igmp_snd(mgr->net, mgr->ga, IGMP_REPORT);
            }
        }
    }
}

/*
    Check IGMPv1 Router present timeout.
*/

void igmpv1_timer(UW ctimval)
{
    T_NET *net;
    UH i;

    IGMPV1_TIMER_ON = 0;
    for (i=0;i<NET_DEV_MAX;i++) {
        net = &gNET[i];
        if ((IGMP_V3_ENABLE && net->igmp_ver == 2) || net->igmp_ver == 1) {
            if (TIM_LEQ(net->igmpv1_tmo,ctimval)) {
                if (net->igmp_ver == 1) { /*v1: move to v2 */
                    net->igmp_ver = 2;
                    IGMPV1_TIMER_ON = 1;
                    net->igmpv1_tmo = IGMP_V1_TMO + NET_TICK;
                }
                else { /* v2: move to v3 if enabled */
                    if (IGMP_V3_ENABLE && net->igmp_ver == 2) {
                        net->igmp_ver = 3;
                        igmp_cancel_v2(net); /* cancel v2 timers */
                    }
                }
            }
            else {
                IGMPV1_TIMER_ON = 1;
            }
        }
    }
}

UB is_mgroup_in(T_NET *net, UW mcast, UW src)
{
    T_NET_MGR    *mgr;
    UH  i;

    if (!(net->igmp_exists))
        return 0;

    if (mcast == IGMP_ALL_SYSTEMS) {
        return 1;
    }

    for (i=0;i<NET_MGR_MAX;i++) {
        mgr = &pNET_MGR[i];
        if ((mgr->state != IGMP_NON_MEMBER) &&
            (mgr->ga == mcast && mgr->net == net)) {
#ifdef IGMPv3_SUP
            return igmpv3_allow_src(mgr, src);
#else
            return 1;
#endif
        }
    }

    return 0;
}

#ifdef MCAST_SOC_SUP
H mgroup_index(T_NET *net, UW mcast)
{
    T_NET_MGR    *mgr;
    UH  i;

    for (i=0;i<NET_MGR_MAX;i++) {
        mgr = &pNET_MGR[i];
        if ((mgr->state != IGMP_NON_MEMBER) &&
            (mgr->ga == mcast && mgr->net == net)) {
            return i;
        }
    }
    return -1;
}

void set_mgr_idx(T_NET_SOC *soc, UH index)
{
    UW *mgr_idx_list;

    if (NET_MGR_MAX > 32) {
        mgr_idx_list = (UW*)soc->mgr_idx;
        mgr_idx_list[index/32] |= (1 << index%32);
    } else {
        soc->mgr_idx |= (1 << index);
    }
}

void clr_mgr_idx(T_NET_SOC *soc, UH index)
{
    UW *mgr_idx_list;

    if (NET_MGR_MAX > 32) {
        mgr_idx_list = (UW*)soc->mgr_idx;
        mgr_idx_list[index/32] &= ~(1 << index%32);
    } else {
        soc->mgr_idx &= ~(1 << index);
    }
}

void clr_mgr_all_idx(T_NET_SOC *soc)
{
    UW *mgr_idx_list;
    int i;

    if (NET_MGR_MAX > 32) {
        mgr_idx_list = (UW*)soc->mgr_idx;
        for (i = 0; i < (NET_MGR_MAX+31)/32; i++) {
            mgr_idx_list[i] = 0;
        }
    } else {
        soc->mgr_idx = 0;
    }
}

UB isset_mgr_idx(T_NET_SOC *soc, UH index)
{
    UB ret;
    UW *mgr_idx_list;

    if (NET_MGR_MAX > 32) {
        mgr_idx_list = (UW*)soc->mgr_idx;
        ret = (UB)((mgr_idx_list[index/32] >> index%32) & 1);
    } else {
        ret = (UB)(soc->mgr_idx>>index & 1);
    }
    return ret;
}

#endif

/*
    Join to a Multicast Group
    Send Report
    Set  Flag
    Start Report Timer
    State = IGMP_DLY_MEMBER
*/
ER igmp_join(T_NET *net, UW ga)
{
    T_NET_MGR    *mgr, *tmp;
    UH  i;

    if (ga == 0) {
        return E_PAR;
    }

    if (!(IS_MCAST_IP(ga))) {
        return E_PAR;
    }

    /* Do nothing if already an member */
    for (i=0;i<NET_MGR_MAX;i++) {
        mgr = &pNET_MGR[i];
        if ((mgr->state != IGMP_NON_MEMBER) &&
            (mgr->ga == ga && mgr->net == net)) {
            /* Group Already Exists */
            mgr->ref++;
            return E_OK;
        }
    }

#ifdef IGMPv3_SUP
    if (net->igmp_ver == 3) {
        /* use IGMP version 3 */
        return igmp3_join(net, ga);
    }
    /* for igmpv3, the below process is done in igmp3_join */
#endif

    /* Join new */
    tmp = NULL;
    for (i=0;i<NET_MGR_MAX;i++) {
        mgr = &pNET_MGR[i];
        if (mgr->state == IGMP_NON_MEMBER) {
            tmp = mgr;
            break;
        }
    }

    if (tmp == NULL) {
        NET_ERR(printf("\r\n ERR: igmp_join: IGMP Resource"));
        return E_NOMEM;
    }

    tmp->ga  = ga;
    tmp->tmo = (net_rand() % IGMP_REP_TMO) + NET_TICK;
    tmp->cnt = 1;   /* Report retry count    */
    tmp->flg = 1;   /* We were the last host */
    tmp->ref = 1;
    tmp->net = net;
    tmp->state = IGMP_DLY_MEMBER;
#ifdef IGMPv3_SUP
    mgr_clr(tmp);   /* reset igmp 3 variables anyway */
    tmp->mode = IGMP_FLT_MODE_EXC;
#endif
    igmp_snd(net, ga, IGMP_REPORT);

    net->igmp_exists = 1;

    IGMP_TIMER_ON = 1;

    /* notify to lower layer */
    if (net->dev->ctl) {
        net->dev->ctl(net->dev->num, CFG_MC4_FIL_SET, (VP)((ADDR)ga));
    }

    return E_OK;
}

/*
    Leave a Multicast Group
    Send IGMP Leave if Flag is set
    State = IGMP_NON_MEMBER
*/

ER igmp_leave(T_NET *net, UW ga)
{
    T_NET_MGR    *mgr, *tmp;
    UH  i;

    /* Do nothing if not a member */
    mgr = tmp = NULL;
    for (i=0;i<NET_MGR_MAX;i++) {
        mgr = &pNET_MGR[i];
        if ((mgr->state != IGMP_NON_MEMBER) &&
            (mgr->ga == ga && mgr->net == net)) {
            tmp = mgr;
            break;
        }
    }

    if (tmp == NULL) {
        return E_OK;
    }
#ifdef MCAST_SOC_SUP
    /* Do nothing if sockets are still joined */
    if (tmp->ref > 1) {
        tmp->ref--;
        return E_OK;
    }
#endif

    /* Do not process igmp if no member in group */
    net->igmp_exists = 0;
    for (i=0;i<NET_MGR_MAX;i++) {
        tmp = &pNET_MGR[i];
        if ((tmp->state != IGMP_NON_MEMBER) &&
            (tmp->net == net)) {
            net->igmp_exists = 1;
            break;
        }
    }

#ifdef IGMPv3_SUP
    if (net->igmp_ver == 3) {
        /* use IGMP version 3 */
        return igmp3_leave(net, ga);
    }
    /* for igmpv3, the below process is done in igmp3_leave */
#endif

    /* Leave Group */
    mgr->state = IGMP_NON_MEMBER;
    if (mgr->flg) {
        igmp_snd(net, ga, IGMP_LEAVE);
    }

    /* notify to lower layer */
    if (net->dev->ctl) {
        net->dev->ctl(net->dev->num, CFG_MC4_FIL_CLR, (VP)((ADDR)ga));
    }
    return E_OK;
}


/*
    Process incoming IGMP packets
*/
void igmp_rcv(T_NET_BUF *pkt)
{
    T_NET_MGR   *mgr;
    T_IGMP_HDR  *igmphdr;
    T_NET       *net;
    UH len, i, max_rpt;

    net = pkt->net;

    igmphdr = (T_IGMP_HDR *)(pkt->dat);
    len     = pkt->dat_len;

    /* Validate Length */

    if (len < IGMP_HDR_SZ) {
        net_buf_ret(pkt);
        NET_ERR(printf("\r\n ERR: igmp_rcv: Bogus igmp header %d", len));
        return;
    }

    /* Validate Checksum */

    if (igmp_csum(pkt) != 0) {
        net_buf_ret(pkt);
        NET_ERR(printf("\r\n ERR: igmp_rcv: Bad Checksum"));
        return;
    }

    switch (igmphdr->type) {

        case IGMP_QUERY:
            max_rpt = 0;
#ifdef IGMPv3_SUP
            /* V3 query and IGMP V3 protocol is supported */
            if (IGMP_V3_ENABLE && len >= 12) {
                if (net->igmp_ver == 3) {   /* compatibility mode is version 3 */
                    igmp3_rcv(pkt);         /* use version 3 protocol */
                    break;  /* proceed to release 'pkt' */
                }
                /* else process as older version */
            }
#endif
            if (len < IGMP_HDR_SZ) {
                break;  /* Invalid V1 or V2 or later query. Drop it. */
            }
            if (igmphdr->mrt == 0) {            /* V1 query */
                net->igmp_ver = 1;              /* change compatibility mode = 1*/
                net->igmpv1_tmo = IGMP_V1_TMO + NET_TICK;
                IGMPV1_TIMER_ON = 1;
                max_rpt = IGMP_REP_TMO / 100;
            }
            else {  /* V2 Query */
                max_rpt = igmphdr->mrt;
#ifdef IGMPv3_SUP
                if (net->igmp_ver == 3)
                {
                    net->igmp_ver = 2;      /* change compatibility mode = 2 */
                    net->igmpv1_tmo = IGMP_V1_TMO + NET_TICK;
                    IGMPV1_TIMER_ON = 1;
                    igmp_cancel_v3(net); /* cancel v3 timers */
                }
#endif
                /* accept v2 query */
                if ((net->igmp_ver == 2) && (IGMPV1_TIMER_ON)) {
                    net->igmpv1_tmo = IGMP_V1_TMO + NET_TICK; /* reset timer */
                }
            }

            if (igmphdr->ga == 0) {
                /* General Query Dst 224.0.0.1 && GroupAddress = 0*/
                /* Report all members with random timeout         */
                for (i=0;i<NET_MGR_MAX;i++) {
                    mgr = &pNET_MGR[i];
                    if ((mgr->net == net) && (mgr->state == IGMP_IDLE_MEMBER)) {
                        mgr->tmo   = ((net_rand() % max_rpt) * 100) + NET_TICK;
                        mgr->state = IGMP_DLY_MEMBER;
                        IGMP_TIMER_ON = 1;
                    }
                }
            }
            else {
                /* Group Specific Query                     */
                /* Report the member with random timeout    */
                for (i=0;i<NET_MGR_MAX;i++) {
                    mgr = &pNET_MGR[i];
                    if ((mgr->net == net) && (mgr->ga == ntohl(igmphdr->ga))) {
                        if (mgr->state == IGMP_IDLE_MEMBER) {
                            mgr->tmo   = ((net_rand() % max_rpt) * 100) + NET_TICK;
                            mgr->state = IGMP_DLY_MEMBER;
                            IGMP_TIMER_ON = 1;
                        }
                        break;
                    }
                }
            }

            break;

        case IGMP_REPORT:
        case IGMP_REPORT_V1:

            /* Stop IGMP Report timer */

            for (i=0;i<NET_MGR_MAX;i++) {
                mgr = &pNET_MGR[i];
                if ((mgr->net == net) && (mgr->ga == ntohl(igmphdr->ga))) {
                    if (mgr->state == IGMP_DLY_MEMBER) {
                        mgr->state = IGMP_IDLE_MEMBER;
                        mgr->flg   = 0;
                    }
                    break;
                }
            }

            break;
        case IGMP_REPORT_V3:
            break;
        default:
            NET_ERR(printf("\r\n ERR: igmp_rcv: Unsupported Type %x", igmphdr->type));
            break;
    }

    net_buf_ret(pkt);
    return;
}

/*
    IGMP Transmission process
*/
void igmp_snd(T_NET *net, UW ga, UB type)
{
    T_IP4_HDR *ip4hdr;
    T_NET_BUF *pkt;
    T_IGMP_HDR *igmp;
    ER ercd;
    UB *ip_opt;

    /* When IGMP Version 1 Router Present do not send
       IGMP2 messages.
    */

    if (net->igmp_ver == 1) {
        if (type != IGMP_REPORT) {
            return;
        }
        type = IGMP_REPORT_V1;
    }

    /* Prepare to Send      */
    ercd = net_buf_get(&pkt, IGMP_BUF_SIZE, TMO_POL);
    if (ercd != E_OK) {
        NET_ERR(printf("\r\n ERR: igmp_report: net_buf_get %d", ercd));
        return;
    }

    pkt->hdr = pkt->hdr + net->dev->hhdrsz + net->dev->hhdrofs;
    pkt->hdr_len = IP4_HDR_SZ;

    if (type == IGMP_REPORT) {
        /* Router Alert Option - RFC 2113 */
        ip_opt = pkt->hdr + pkt->hdr_len;
        ip_opt[0] = 0x94;
        ip_opt[1] = 0x04;
        ip_opt[2] = 0x00;
        ip_opt[3] = 0x00;
        pkt->hdr_len += 4;
    }

    pkt->dat = pkt->hdr + pkt->hdr_len;
    pkt->dat_len = IGMP_HDR_SZ;

    igmp = (T_IGMP_HDR *)pkt->dat;

    /* Construct IGMP Message */

    igmp->type = type;
    igmp->mrt = 0;   /* Max Response Time is 0 for Reports */
    igmp->ga  = htonl(ga);
    igmp->cs  = 0;
    igmp->cs  = igmp_csum(pkt);

    if (type == IGMP_LEAVE) {
        ga = IGMP_ALL_ROUTERS;
    }

    /* Construct IP Header */
    ip4hdr = (T_IP4_HDR *)pkt->hdr;

    if (type == IGMP_REPORT) {
        ip4hdr->ver  = 0x46;
    }
    else {
        ip4hdr->ver  = 0x45;
    }

    ip4hdr->tos  = 0;
    ip4hdr->tl   = htons(pkt->hdr_len + pkt->dat_len);
    ip4hdr->id   = htons(net->ident++);
    ip4hdr->fo   = 0;
    ip4hdr->ttl  = 1;
    ip4hdr->hc   = 0;
    ip4hdr->prot = IP_PROTO_IGMP;
    ip4hdr->hc = 0;
    ip4hdr->sa = htonl(net->adr->ipaddr);
    ip4hdr->da = htonl(ga);
    ip4hdr->hc  = ip4_csum(pkt);

    pkt->soc = NULL;
    pkt->net = net;
    pkt->hdr_len += pkt->dat_len;
    pkt->dat = NULL;
    pkt->dat_len = 0;
    pkt->ercd = pkt->hdr_len;

    net->out(pkt);

    return;
}

#endif /*MCAST_SUP*/

