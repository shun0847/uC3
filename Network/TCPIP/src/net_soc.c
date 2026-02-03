/***************************************************************************
    MICRO C CUBE / COMPACT, NETWORK
    Socket APIs
    Copyright (c)  2008-2021, eForce Co., Ltd. All rights reserved.

    Version Information
      2008.11.19: Created
      2010.03.24: SOC_FLG_PRT check
      2010.08.17: Support SOC_RCV_PKT_INF socket option
      2010.08.30: Correct abt_soc() each code case
      2010.09.15: Check callback function is set
      2010.11.02: Updated for IPv6 support
      2010.11.08: Modified callback event pattern(cptn->wptn)
      2010.11.10: Updated for any network interface
      2011.01.06: Correct con_soc() configurable TOS/TTL
      2011.03.07: Updated for received packet queue
      2011.03.11: Updated for IPv6 multiple interface
      2011.04.21: Callback function event pattern check
      2011.05.20: Moved NET_TCP_MAX from lib to application
      2011.11.17: Update for resetting local port
      2012.10.02  Modify to avoid use of string libraries.
      2013.01.15: Corrected to allow UDP port is duplicated
                  if different IP version
      2013.07.11: Disabled abt_soc() in not running socket API
      2013.07.23  Implemented ready I/O event notification.
      2013.08.06: Implememted multicast per socket
      2013.08.30: Set SOC_FLG_PRT in cfg_soc()
      2013.11.22: Change to return E_NOMEM if there is no
                  available resource in cre_soc()
      2013.12.16: Correct to set rqsz in clr_rcv_que()
      2014.03.06: SNMP option was added
      2014.01.28: Correct the timing to clear soc->wptn bit
      2014.01.30: Improved to release networkbuffer immediately
                  once snd_soc() is suspended in ARP resolving. 
      2014.06.18  SNMP compact was supported
      2014.07.16: Update not to notify duplicate RX event
      2014.09.02: Add deletion of RX packet in cls_soc() for UDP
      2014.12.01: When socket is waiting state, modified to 
                  result of cfg_soc(SOC_CBK_FLG) in an error.
      2015.02.04: Fixed error of device check at changing UDP port
      2015.03.20: BSD socket to support uNet3/Compact.
      2015.03.31: Fixed to checking of cfg_soc(SOC_CBK_FLG).
      2015.12.03: Add SOC_UDP_RQSZ option
      2015.12.03: Add SOC_SRC_IP_RESET option
      2015.12.14: The socket ID replaced SID types
      2015.12.18: Changed to increase UDP rqsz in clr_rcv_que()
      2016.01.07: Add MLDv1 functions
      2016.02.05: Add SOC_UDP_EARLY_TX option
      2016.02.24: Suppressed the warning for Kpit GNU tools
      2016.06.08: Add SOC_RCV_PKT_SIZE option
      2016.06.09: Add SOC_MSG_PEEK option
      2016.06.16: Add SOC_PRT_LOCAL_DUP option
      2016.09.12: Fixed to checking of cfg_soc(SOC_PRT_LOCAL_DUP).
      2016.10.06: Fixed the SOC_PRT_LOCAL_DUP option.
      2016.10.11: Fixed the SOC_PRT_LOCAL option.
      2017.01.18: Support 64bit processor
      2017.03.08: Allow the same port socket with different IP ver
      2017.03.29  Support for out of sequence data queue (routque).
      2017.05.02: Add soc_ext() API
      2017.07.06  Call clr_rcv_que() from cls_soc() instead from soc_event().
                  This allows rcv_soc() to read data even in closing state and 
                  until cls_soc() is called.
      2017.08.14: Call clr_rcv_que() from soc_ext(), soc_reset().
      2017.08.18: Change to not clear socket configration at soc_ext().
      2018.03.01: Change execution condition of SOC_RCV_PKT_INF option.
      2018.08.23: Support set or reset the don't fragment flag in IP header
      2020.05.19: Fixed a problem that con_soc () keeps a buffer on error.
      2020.05.28: Fixed NULL pointer refer of con_soc() when IPv6 is enabled.
      2020.10.16: Extended number of multicast groups per socket.
      2021.03.16: Fixed soc_event() to not execute unnecessary wup_tsk().
 ***************************************************************************/

#include <stdio.h>
#include "kernel.h"
#include "net_hdr.h"
#include "net_def.h"

static void clr_rcv_que(T_NET_SOC *soc);
#ifdef REUSE_ADDR_SUP
static void reg_reuse_addr(T_NET_SOC *soc, UH lport);
static void rel_reuse_addr(T_NET_SOC *soc);
#endif

#ifdef NET_C

static ER soc_ccre(T_SOC_TABLE *stb)
{
    T_NET_SOC *soc;
    T_NET *net;
    ER ercd;
    SID sid;
    UH local_port;
    UB proto;
#ifdef IPV6_SUP
    UB ver;
#endif
#ifdef MCAST_SOC_SUP
    UW *mgr_idx_list;
#endif

    if (stb == NULL) {
        return E_PAR;
    }

    if ((stb->sid == 0) || (stb->sid > (NET_SOC_MAX+1))) {
        return E_PAR;
    }

    /* 1. Validate Parameters */
    proto      = stb->proto;
    local_port = stb->port;

#ifdef IPV6_SUP
    ver = stb->ver;
#endif

    /* Validate 'proto' */
    ercd = E_OK;
    switch (proto) {
#ifdef TCP_SUP
        case IP_PROTO_TCP:
            break;
#endif
#ifdef UDP_SUP
        case IP_PROTO_UDP:
            break;
#endif
#ifdef PING_SUP
        case IP_PROTO_ICMP:
            break;
#endif
#ifdef IPV6_SUP
        case IP_PROTO_ICMPV6:
            break;
#endif
        default:
            ercd = E_NOSPT;
            break;
    }

    if (ercd != E_OK) {
        return ercd;
    }

    /* Local Interface */
    if (stb->dev_num != DEV_ANY) {
        net = &gNET[stb->dev_num-1];
    }
    else {
        net = NULL;
    }

#ifdef PING_SUP
    if (proto == IP_PROTO_ICMP
#ifdef IPV6_SUP
        || proto == IP_PROTO_ICMPV6
#endif
    ) {
        local_port = 0;
    }
    else
#endif
    if (local_port == PORT_ANY) {
        local_port = get_eport(proto); /* Get a random port */
    }

    /* Validate socket entities */
    /* Duplicate socket if socket proto, local port and interface are same */
    /* except for TCP sockets, TCP sockets are validated during connection */

    ercd = E_OK;

    if (proto != IP_PROTO_TCP) {

        for (sid = 0; sid < NET_SOC_MAX; sid++) {
            soc = &pNET_SOC[sid];
            if (soc->state != SOC_UNUSED &&
                soc->proto == proto      &&
                soc->lport == local_port &&
#ifdef IPV6_SUP
                soc->ver == ver          &&
#endif
                soc->net   == net) {
                ercd = E_PAR;
                break;
            }
        }
    }

    if (ercd != E_OK) {
        return ercd;
    }

    /* Allocate socket */
    sid = stb->sid-1;
    soc = &pNET_SOC[sid];

    soc->tcp = NULL;

#ifdef TCP_SUP
    if (proto == IP_PROTO_TCP) {

        if (stb->sbufsz == 0)
            return E_PAR;

        if (stb->rbufsz == 0)
            return E_PAR;

        ercd = tcp_cre(soc);
        if (ercd != E_OK) {
            return ercd;
        }

        soc->tcp->sbufsz = stb->sbufsz;
        soc->tcp->rbufsz = stb->rbufsz;
    }
#endif

    soc->sid   = sid;
    soc->state = SOC_CREATED;
    soc->proto = proto;
    soc->lport = local_port;
    soc->rport = 0;
#ifdef IPV6_SUP
    soc->ver = ver;
    if(soc->ver == IP_VER6)
        net_memset(soc->raddr6, 0, 16);
    else
#endif
    soc->raddr = 0;
    soc->net   = net;
    soc->usr_net = net;

    if (net) {
        soc->tos = IP4_TOS;
        soc->ttl = IP4_TTL;
        soc->rqsz= PKT_RCV_QUE;
    }
    else {
        soc->tos = DEF_IP4_TOS;
        soc->ttl = DEF_IP4_TTL;
        soc->rqsz= DEF_PKT_RCV_QUE;
    }

    soc->flg     = 0;
    soc->ruanum  = 0;
    soc->sdatque = NULL;
    soc->rdatque = NULL;

    soc->ctskid = 0;
    soc->stskid = 0;
    soc->rtskid = 0;
    soc->wptn   = 0;
    soc->cptn   = 0;
    soc->cbk    = NULL;

    soc->snd_tmo = stb->snd_tmo;
    soc->rcv_tmo = stb->rcv_tmo;
    soc->con_tmo = stb->con_tmo;
    soc->cls_tmo = stb->cls_tmo;

    soc->ercd    = E_OK;

    net_memset((char *)&soc->rpi, 0, sizeof(T_RCV_PKT_INF));
#ifdef MCAST_SOC_SUP
    if (proto != IP_PROTO_TCP) {
        if (NET_MGR_MAX > 32) {
            if(net_inftbl[NETINF_MGR_INDEX] != NULL) {
            /* if MGR index exceeds 32 replace with extended area instead of mgr_idx */
                mgr_idx_list = (UW*)net_inftbl[NETINF_MGR_INDEX];
                mgr_idx_list += ((NET_MGR_MAX+31)/32) * (sid-NET_TCP_MAX); /* UDP index origin */
                soc->mgr_idx = (UW)mgr_idx_list;
            } else {
                return E_OBJ;
            }
        }
        clr_mgr_all_idx(soc);
    }
#endif

    if (stb->port == PORT_ANY) {
        soc->flg |= SOC_FLG_PRT;    /* ANY PORT is specified */
    }

    soc->chg_flg = NET_STS_CHG;

    return (ER)(sid+1);
}

#endif

void soc_ini(void)
{
    T_NET_SOC *soc;
    T_NET_TCP *tcp;
    UB *buf;
    UH i;

    for (i=0;i<NET_SOC_MAX;i++) {
        soc = &pNET_SOC[i];
        soc->sid   = i;
        soc->state = SOC_UNUSED;
        soc->chg_flg = NET_STS_CLR;
    }

    buf = (UB *)net_inftbl[NETINF_TCP_TXBUF];

#ifdef TCP_SUP
    for (i=0;i<NET_TCP_MAX;i++) {
        tcp = &pNET_TCP[i];
        tcp->soc   = &pNET_SOC[i];
        tcp->bsd   = TCP_UNUSED;
#ifndef NET_C
        tcp->snd_buf = buf + (i * TCP_SND_WND_MAX);
        tcp->sbufsz  = 0;
        tcp->rbufsz  = 0;
#endif
        tcp->chg_flg = NET_STS_CLR;
    }
#endif

#ifdef NET_C
    for (i=0;i<NET_SOC_MAX;i++) {
        if (!(pSOC_TABLE[i].sid || pSOC_TABLE[i].proto)) {
            break;  /* terminate */
        }
        soc_ccre(&pSOC_TABLE[i]);
    }
#endif

#ifdef NET_C
    for (i=0;i<NET_TCP_MAX;i++) {
        soc = &pNET_SOC[i];
        tcp = soc->tcp;
#ifdef POSIX_API_SUP
        if (!tcp) {     /* TCP part has not been generated if BSD socket */
            tcp_cre(soc);
            soc->tcp->sbufsz = BSD_TCP_SBUFSZ;
            soc->tcp->rbufsz = BSD_TCP_RBUFSZ;
            tcp = soc->tcp;
        }
#endif
        if (tcp) {
            tcp->snd_buf = buf;
            buf += tcp->sbufsz;
        }
    }
#endif

}

#ifndef NET_C
ER cre_soc(UB proto, T_NODE *host)
{
    T_NET_SOC *soc;
    T_NET *net;
    ER ercd;
    UH sid, local_port;
#ifdef MCAST_SOC_SUP
    UW *mgr_idx_list;
#endif

    /* 1. Validate Parameters */

    /* Validate 'proto' */

    ercd = E_OK;
    switch (proto) {
#ifdef TCP_SUP
        case IP_PROTO_TCP:
            break;
#endif
#ifdef UDP_SUP
        case IP_PROTO_UDP:
            break;
#endif
#ifdef PING_SUP
        case IP_PROTO_ICMP:
            break;
#endif
#ifdef IPV6_SUP
        case IP_PROTO_ICMPV6:
            break;
#endif
        default:
            ercd = E_NOSPT;
            break;
    }

    if (ercd != E_OK) {
        return ercd;
    }

    /* Validate 'host' */

    if (host == NULL) {
        return E_PAR;
    }

    if (host->ver != IP_VER4) {
#ifdef IPV6_SUP
        if (host->ver != IP_VER6)
#endif
            return E_NOSPT;
    }
    
#ifdef IPV6_SUP
    if(host->ver == IP_VER6) {
        if(ip6_addr_type(host->ip6a) != IP6_ADDR_UNSPEC)
        return E_NOSPT;
    }
    else
    {
#endif

    if (host->ipa != INADDR_ANY) {
        /* Not supported, Multiple address for single interface */
        return E_NOSPT;
    }
#ifdef IPV6_SUP
    }
#endif

    net = NULL;

    loc_tcp();

    /* 2. Validate IP Address, Port */

    /* Local Interface */
    if (host->num != DEV_ANY) {
        net = get_net_bynum(host->num-1);
        if (net == NULL) {
            ulc_tcp();
            return E_PAR;
        }
    }

    local_port = host->port;
#ifdef PING_SUP
    if (proto == IP_PROTO_ICMP
#ifdef IPV6_SUP
        || proto == IP_PROTO_ICMPV6
#endif
        ) {
        local_port = 0;
    }
    else
#endif
    if (local_port == PORT_ANY) {
        local_port = get_eport(proto); /* Get a random port */
    }

    /* Validate socket entities */
    /* Duplicate socket if socket proto, local port and interface are same */
    /* except for TCP sockets, TCP sockets are validated during connection */

    ercd = E_OK;
    soc  = NULL;

    if (proto != IP_PROTO_TCP) {

        for (sid = NET_TCP_MAX; sid < NET_SOC_MAX; sid++) {
            soc = &pNET_SOC[sid];
            if (soc->state != SOC_UNUSED &&
                soc->proto == proto      &&
                soc->lport == local_port &&
#ifdef IPV6_SUP
                soc->ver == host->ver    &&
#endif
                soc->net   == net) {
                ercd = E_PAR;
                break;
            }
        }
    }

    if (ercd != E_OK) {
        ulc_tcp();
        return ercd;
    }

    /* Allocate socket */
    /* TCP socket will be allocated within NET_TCP_MAX -1          */
    /* range for other socket is from NET_TCP_MAX till NET_SOC_MAX */
    ercd = E_ID;
    if(proto==IP_PROTO_TCP) {
        for (sid=0;sid<NET_TCP_MAX;sid++) {
            soc = &pNET_SOC[sid];
            if (soc->state == SOC_UNUSED) {
                ercd = E_OK;
                break;
            }
        }
    }
    else {
        for (sid=NET_TCP_MAX;sid<NET_SOC_MAX;sid++) {
            soc = &pNET_SOC[sid];
            if (soc->state == SOC_UNUSED) {
                ercd = E_OK;
                break;
            }
        }
    }

    if (ercd != E_OK) {
        ulc_tcp();
        return E_NOMEM;
    }

#ifdef TCP_SUP
    if (proto == IP_PROTO_TCP) {
        ercd = tcp_cre(soc);
        if (ercd != E_OK) {
          ulc_tcp();
          return ercd;
        }
    }
#endif

    soc->sid   = sid;
    soc->state = SOC_CREATED;
    soc->proto = proto;
    soc->lport = local_port;
    soc->rport = 0;
    
#ifdef IPV6_SUP
    soc->ver = host->ver;
    if(host->ver == IP_VER6)
        net_memset(soc->raddr6, 0, 16);
    else
#endif
    soc->raddr = 0;
    soc->net   = net;
    soc->usr_net = net;

    if (net) {
        soc->tos = IP4_TOS;
        soc->ttl = IP4_TTL;
        soc->rqsz= PKT_RCV_QUE;
    }
    else {
        soc->tos = DEF_IP4_TOS;
        soc->ttl = DEF_IP4_TTL;
        soc->rqsz= DEF_PKT_RCV_QUE;
    }

    soc->flg     = 0;
    soc->ruanum  = 0;
    soc->sdatque = NULL;
    soc->rdatque = NULL;

    soc->ctskid = 0;
    soc->stskid = 0;
    soc->rtskid = 0;
    soc->wptn   = 0;
    soc->cptn   = 0;
    soc->cbk    = NULL;

    soc->snd_tmo = TMO_FEVR;
    soc->rcv_tmo = TMO_FEVR;
    soc->con_tmo = TMO_FEVR;
    soc->cls_tmo = TMO_FEVR;

    soc->ercd    = E_OK;

    net_memset((char *)&soc->rpi, 0, sizeof(T_RCV_PKT_INF));
#ifdef MCAST_SOC_SUP
    if (proto != IP_PROTO_TCP) {
        if (NET_MGR_MAX > 32) {
            if(net_inftbl[NETINF_MGR_INDEX] != NULL) {
            /* if MGR index exceeds 32 replace with extended area instead of mgr_idx */
                mgr_idx_list = (UW*)net_inftbl[NETINF_MGR_INDEX];
                mgr_idx_list += ((NET_MGR_MAX+31)/32) * (sid-NET_TCP_MAX); /* UDP index origin */
                soc->mgr_idx = (UW)mgr_idx_list;
            } else {
                ulc_tcp();
                return E_OBJ;
            }
        }
        clr_mgr_all_idx(soc);
    }
#endif

    if (host->port == PORT_ANY) {
        soc->flg |= SOC_FLG_PRT;    /* ANY PORT is specified */
    }

    soc->chg_flg = NET_STS_CHG;

    ulc_tcp();

    return (ER)(sid+1);
}

ER del_soc(SID sid)
{
    T_NET_SOC *soc;
    ER ercd;
    T_NET_BUF *pkt;
#ifdef MCAST_SOC_SUP
    int i;
#endif

    if ((sid == 0) || (sid > NET_SOC_MAX)) {
        return E_ID;
    }

    loc_tcp();

    sid--;
    soc = &pNET_SOC[sid];

    if (soc->state == SOC_UNUSED) {
        ulc_tcp();
        return E_NOEXS;
    }

#ifdef TCP_SUP
    if (soc->proto == IP_PROTO_TCP) {
        ercd = tcp_del(soc);
        if (ercd != E_OK) {
            ulc_tcp();
            return ercd;
        }
    }
#endif

    soc->state = SOC_UNUSED;

#if defined(UDP_SUP) || defined(PING_SUP)
    if (soc->proto != IP_PROTO_TCP) {

        if (soc->sdatque != NULL) {
            /*net_buf_ret((T_NET_BUF*)soc->sdatque);*/
            soc->sdatque = NULL;
        }

        while (soc->rdatque) {
            pkt = (T_NET_BUF*)soc->rdatque;
            rmv4que_top(&soc->rdatque);
            soc->rqsz++;
            net_buf_ret(pkt);
        }

        if (soc->wptn) {
            if (soc->wptn & EV_SOC_SND) {
                soc->slen = E_CLS;
                if ((soc->cptn & EV_SOC_SND) && (soc->cbk)) {
                    soc->cbk(soc->sid+1, EV_SOC_SND, E_CLS);
                }
                else {
                    wup_tsk(soc->stskid);
                    soc->stskid = 0;
                }
            }
            if (soc->wptn & EV_SOC_RCV) {
                soc->rlen = E_CLS;
                if ((soc->cptn & EV_SOC_RCV) && (soc->cbk)) {
                    soc->cbk(soc->sid+1, EV_SOC_RCV, E_CLS);
                }
                else {
                    wup_tsk(soc->rtskid);
                    soc->rtskid = 0;
                }
            }
        }
#ifdef MCAST_SOC_SUP
        if (soc->mgr_idx) {
            for (i = 0; i < NET_MGR_MAX; i++) {
                if (isset_mgr_idx(soc, i)) {
                    igmp_leave(pNET_MGR[i].net, pNET_MGR[i].ga);
                }
            }
        }
#endif
#ifdef REUSE_ADDR_SUP
        /* Release REUSEADDR list */
        rel_reuse_addr( soc );
#endif
    }
#endif

    soc->chg_flg = NET_STS_CHG;

    ulc_tcp();

    return E_OK;
}
#endif

ER con_soc(SID sid, T_NODE *host, UB con_flg)
{
    T_NET_TCP *tcp;
    T_NET_SOC *soc;
    T_NET     *net;
    ER ercd;

    if ((sid == 0) || (sid > NET_SOC_MAX)) {
        return E_ID;
    }

    loc_tcp();

    sid--;

    soc = &pNET_SOC[sid];

    if (soc->state == SOC_UNUSED) {
        ulc_tcp();
        return E_NOEXS;
    }

#ifdef IPV6_SUP
    if ((host != NULL) && (soc->ver != host->ver)) {
        ulc_tcp();
        return E_PAR;
    }
#endif

    if (soc->proto != IP_PROTO_TCP) {
        con_flg = SOC_CLI;          /* ADD20090414 */
    }

    if ((con_flg != SOC_SER) && (con_flg != SOC_CLI)) {
        ulc_tcp();
        return E_NOSPT;
    }

    if (con_flg == SOC_CLI) {

        if (host == NULL) {
            ulc_tcp();
            return E_PAR;
        }
#ifndef PING_SUP
        if (host->port == PORT_ANY) {
            ulc_tcp();
            return E_PAR;
        }
#else
        if (con_flg == SOC_CLI) {
#ifdef IPV6_SUP
            if(host->ver == IP_VER6) {
                if ((host->port == PORT_ANY) && (soc->proto != IP_PROTO_ICMPV6)) {
                    ulc_tcp();
                    return E_PAR;
                }
            }
            else {
#endif          
            if ((host->port == PORT_ANY) && (soc->proto != IP_PROTO_ICMP)) {
                ulc_tcp();
                return E_PAR;
            }
#ifdef IPV6_SUP
        }
#endif
        }
#endif

#ifdef IPV6_SUP
        if(host->ver == IP_VER6)
        {
            if(ip6_addr_type(host->ip6a) == IP6_ADDR_UNSPEC) {
                ulc_tcp();
                return E_NOSPT;
            }
        }
        else
#endif
        if (host->ipa == INADDR_ANY) {
            ulc_tcp();
            return E_PAR;
        }
    }

#ifdef TCP_SUP
    if (soc->proto == IP_PROTO_TCP) {
        tcp = soc->tcp;

        if (tcp == NULL) {
            ulc_tcp();
            return E_SYS;
        }

        if (tcp->bsd != TCP_CLOSED) {
            ulc_tcp();
            return E_OBJ;
        }
    }
#endif

    net = soc->usr_net;
    if (net == NULL) {
        if (host) {
            if (host->num != DEV_ANY) {
                net = get_net_bynum(host->num-1);
                if (net == NULL) {
                    ulc_tcp();
                    return E_PAR;   /* Interface not exists */
                }
            }
        }
        if ((net == NULL) && (con_flg == SOC_CLI)) {
            net = get_net_default();
            if (net == NULL) {
                ulc_tcp();
                return E_OBJ;   /* Network Interface not exists */
            }
        }
    }

    soc->net = net;             /* Socket local interface */

    if (net && (soc->usr_net == NULL)) {
        if (soc->tos == DEF_IP4_TOS)
            soc->tos = IP4_TOS;
        if (soc->ttl == DEF_IP4_TTL)
            soc->ttl = IP4_TTL;
    }

    if (con_flg == SOC_CLI) {
#ifdef IPV6_SUP
    if(host->ver == IP_VER6)
        ip6_addr_cpy(soc->raddr6, host->ip6a);
    else
#endif        
        soc->raddr = host->ipa;
        soc->rport = host->port;
    }
    else {
#ifdef IPV6_SUP
        net_memset(soc->raddr6, 0, 16);
#endif
        soc->raddr = 0;     /* Filled by receive packet */
        soc->rport = 0;
    }

#if defined(UDP_SUP) || defined(PING_SUP)
    if (soc->proto != IP_PROTO_TCP) {
        ulc_tcp();
        return E_OK;
    }
#endif

#ifdef TCP_SUP
    if (soc->wptn & EV_SOC_CON) {
        ulc_tcp();        /* Already pending? */
        return E_QOVR;    /* never return from here, check tcp->bsd */
    }
    soc->wptn |= EV_SOC_CON;

    if (!(soc->cptn & EV_SOC_CON)) {
        get_tid(&soc->ctskid);
        can_wup(TSK_SELF);
    }

    ercd = tcp_con(soc, host, con_flg);
    if (ercd != E_WBLK) {
        soc->wptn &= ~(EV_SOC_CON);
        soc->ctskid = 0;
        ulc_tcp();
        return ercd;
    }

    if (soc->cptn & EV_SOC_CON) {
        ulc_tcp();
        return E_WBLK;
    }

    ulc_tcp();
    ercd = tslp_tsk(soc->con_tmo);
    loc_tcp();
    soc->wptn &= ~(EV_SOC_CON);
    soc->ctskid = 0;
    if (ercd == E_OK) {
        ercd = soc->ercd;
    }
    else {
        /* Timeout */
        tcp_abt(soc->tcp, 2);
    	clr_rcv_que(soc);
    }
#endif
    ulc_tcp();

    return ercd;
}

ER cls_soc(SID sid, UB cls_flg)
{
    T_NET_SOC *soc;
    ER ercd;

    if ((sid == 0) || (sid > NET_SOC_MAX)) {
        return E_ID;
    }

    loc_tcp();

    sid--;
    soc = &pNET_SOC[sid];

    if (soc->state == SOC_UNUSED) {
        ulc_tcp();
        return E_NOEXS;
    }

#if defined(UDP_SUP) || defined(PING_SUP)
    if (soc->proto != IP_PROTO_TCP) {
        soc->net = NULL;
        soc->rport = 0;
#ifdef IPV6_SUP
        if(soc->ver == IP_VER6)
            net_memset(soc->raddr6, 0, 16);
        else
#endif
        soc->raddr = 0;
        clr_rcv_que(soc);
        ulc_tcp();
        return E_OK;
    }
#endif

#ifdef TCP_SUP
    if (soc->wptn & EV_SOC_CLS) {
        ulc_tcp();
        return E_QOVR;   /* Already pending? */
    }

    ercd = tcp_cls(soc, cls_flg);
    if (cls_flg != SOC_TCP_SHT) {
        /* socket is closing! */ 
        clr_rcv_que(soc); /* rcv: clear any pending data in rcv que */
    }
    if (ercd != E_WBLK) {
        /*soc->wptn &= ~(EV_SOC_CLS);*/
        /*soc->ctskid = 0;*/
        ulc_tcp();
        return ercd;
    }

    soc->wptn |= EV_SOC_CLS;
    if (soc->cptn & EV_SOC_CLS) {
        ulc_tcp();
        return E_WBLK;
    }

    get_tid(&soc->ctskid);
    can_wup(TSK_SELF);

    ulc_tcp();
    ercd = tslp_tsk(soc->cls_tmo);
    loc_tcp();
    soc->wptn &= ~(EV_SOC_CLS);
    soc->ctskid = 0;
    if (ercd == E_OK) {
        ercd = soc->ercd;
    }
    else {
        /* Timeout */
        tcp_abt(soc->tcp, 2);
        soc_event(soc, EV_SOC_ALL, E_TMOUT);
    }

    ulc_tcp();
#endif
    return ercd;
}

ER snd_soc(SID sid, VP data, UH len)
{
    T_NET_SOC *soc;
    UW *pkt;
    ER ercd;

    if ((sid == 0) || (sid > NET_SOC_MAX)) {
        return E_ID;
    }

    if ((data == NULL) || (len == 0)) {
        return E_PAR;    /* ERR: Invalid Parameters */
    }

    loc_tcp();

    sid--;
    soc = &pNET_SOC[sid];
    pkt = NULL;

    if (soc->state == SOC_UNUSED) {
        ulc_tcp();
        return E_NOEXS;
    }

    if (soc->wptn & EV_SOC_SND) {
        ulc_tcp();
        return E_QOVR;   /* Already pending? */
    }
    soc->wptn |= EV_SOC_SND;

    if (!(soc->cptn & EV_SOC_SND)) {
        get_tid(&soc->stskid);
        can_wup(TSK_SELF);
    }

    switch (soc->proto) {
#ifdef TCP_SUP
        case IP_PROTO_TCP:
            ercd = tcp_snd(soc, data, len);
            break;
#endif
#ifdef UDP_SUP
        case IP_PROTO_UDP:
            ercd = udp_snd(soc, data, len);
            break;
#endif
#ifdef PING_SUP
#ifdef IPV6_SUP
        case IP_PROTO_ICMPV6:
#endif
        case IP_PROTO_ICMP:
            ercd = icmp_snd(soc, data, len);
            break;
#endif
        case IP_PROTO_RAW:
            ercd = raw_snd(soc, data, len);
            break;
        default:
            ercd = E_NOSPT;
            break;
    }

    if (ercd != E_WBLK) {
        soc->wptn &= ~(EV_SOC_SND);
        soc->stskid = 0;
        ulc_tcp();
        return ercd;
    }

    if (soc->cptn & EV_SOC_SND) {
        ulc_tcp();
        return E_WBLK;
    }

    ulc_tcp();
    ercd = tslp_tsk(soc->snd_tmo);
    loc_tcp();
    soc->wptn &= ~(EV_SOC_SND);
    soc->stskid = 0;
    if (ercd == E_OK) {
        ercd = soc->slen;
#ifdef TCP_SUP
        if (ercd > 0 && soc->proto == IP_PROTO_TCP) {
            ercd = tcp_snd(soc, data, len);
        }
#endif
    }
    else {
        /* Timeout */
        pkt = soc->sdatque;
        soc->sdatque = NULL;
        /*soc_abort(soc, EV_SOC_SND, E_TMOUT);*/
        arp_rmv_que((T_NET_BUF*)pkt, E_TMOUT);
    }

    ulc_tcp();

    return ercd;
}

ER rcv_soc(SID sid, VP data, UH len)
{
    T_NET_SOC *soc;
    ER ercd;

    if ((sid == 0) || (sid > NET_SOC_MAX)) {
        return E_ID;
    }

    loc_tcp();

    sid--;
    soc = &pNET_SOC[sid];

    if (soc->state == SOC_UNUSED) {
        ulc_tcp();
        return E_NOEXS;
    }

    if (soc->wptn & EV_SOC_RCV) {
        ulc_tcp();
        return E_QOVR;   /* Already pending? */
    }
    soc->wptn |= EV_SOC_RCV;

    if (!(soc->cptn & EV_SOC_RCV)) {
        get_tid(&soc->rtskid);
        can_wup(TSK_SELF);
    }

    switch (soc->proto)
    {
#ifdef PING_SUP
#ifdef IPV6_SUP
        case IP_PROTO_ICMPV6:
#endif
        case IP_PROTO_ICMP:
            ercd = icmp_rcv(soc, data, len);
            break;
#endif
#ifdef UDP_SUP
        case IP_PROTO_UDP:
            ercd = udp_rcv(soc, data, len);
            break;
#endif
#ifdef TCP_SUP
        case IP_PROTO_TCP:
            ercd = tcp_rcv(soc, data, len);
            break;
#endif
        case IP_PROTO_RAW:
            ercd = raw_rcv(soc, data, len);
            break;
        default:
            ercd = E_OBJ;
            break;
    }

    if (ercd != E_WBLK) {
        soc->wptn &= ~(EV_SOC_RCV);
        soc->rtskid = 0;
        ulc_tcp();
        return ercd;
    }

    if (soc->cptn & EV_SOC_RCV) {
        ulc_tcp();
        return E_WBLK;
    }

    ulc_tcp();
    ercd = tslp_tsk(soc->rcv_tmo);
    loc_tcp();
    soc->wptn &= ~(EV_SOC_RCV);
    if (ercd == E_OK) {
        ercd = soc->rlen;
    }
    else {
        /* Time out */
        /* Do not need to abort */
    }

    if (ercd <= 0) {
        soc->rtskid = 0;
        ulc_tcp();
        return ercd;
    }

    switch (soc->proto)
    {
#ifdef PING_SUP
#ifdef IPV6_SUP
        case IP_PROTO_ICMPV6:
#endif
        case IP_PROTO_ICMP:
            ercd = icmp_rcv(soc, data, len);
            break;
#endif
#ifdef UDP_SUP
        case IP_PROTO_UDP:
            ercd = udp_rcv(soc, data, len);
            break;
#endif
#ifdef TCP_SUP
        case IP_PROTO_TCP:
            ercd = tcp_rcv(soc, data, len);
            break;
#endif
        default:
            break;
    }

    ulc_tcp();

    return ercd;
}

ER abt_soc(SID sid, UB code)
{
    T_NET_SOC *soc;
    UW *pkt;
    UH ptn;

    if ((sid == 0) || (sid > NET_SOC_MAX)) {
        return E_ID;
    }

    loc_tcp();

    sid--;
    soc = &pNET_SOC[sid];

    if (soc->state == SOC_UNUSED) {
        ulc_tcp();
        return E_NOEXS;
    }

    ptn = 0;
    pkt = NULL;

    switch (code) {
        case SOC_ABT_ALL:
            ptn = (UH)(-1);
            pkt = soc->sdatque;
            soc->sdatque = NULL;
            if (soc->tcp)
                tcp_abt(soc->tcp, 2);
            break;
        case SOC_ABT_SND:
            if (soc->wptn & EV_SOC_SND) {
                ptn |= EV_SOC_SND;
                pkt = soc->sdatque;
                soc->sdatque = NULL;
            }
            break;
        case SOC_ABT_RCV:
            if (soc->wptn & EV_SOC_RCV) {
                ptn |= EV_SOC_RCV;
            }
            break;
        case SOC_ABT_CON:
            if (soc->wptn & EV_SOC_CON) {
                ptn |= EV_SOC_CON;
                if (soc->tcp)
                    tcp_abt(soc->tcp, 2);
            }
            break;
        case SOC_ABT_CLS:
            if (soc->wptn & EV_SOC_CLS) {
                ptn |= EV_SOC_CLS;
                if (soc->tcp)
                    tcp_abt(soc->tcp, 2);
            }
            break;
        default:
            break;
    }

    if (ptn) {
        soc_event(soc, ptn, E_RLWAI);
    }

    if (pkt) {
        arp_rmv_que((T_NET_BUF*)pkt, E_RLWAI);
    }
    ulc_tcp();

    return E_OK;
}

ER ref_soc(SID sid, UB code, VP val)
{
    T_RCV_PKT_INF *rpi;
    T_NET_SOC *soc;
    T_NODE    *host;
    ER ercd;
#ifdef RCV_PKT_SIZE_SUP
    T_NET_BUF *pkt;
#endif

    if ((sid == 0) || (sid > NET_SOC_MAX)) {
        return E_ID;
    }

    loc_tcp();

    sid--;
    soc = &pNET_SOC[sid];

    if (soc->state == SOC_UNUSED) {
        ulc_tcp();
        return E_NOEXS;
    }

    ercd = E_OK;
    switch (code) {
        case SOC_TMO_CON:
            *(TMO *)(val) = soc->con_tmo;
            break;
        case SOC_TMO_CLS:
            *(TMO *)(val) = soc->cls_tmo;
            break;
        case SOC_TMO_SND:
            *(TMO *)(val) = soc->snd_tmo;
            break;
        case SOC_TMO_RCV:
            *(TMO *)(val) = soc->rcv_tmo;
            break;
        case SOC_IP_TTL:
            *(UB *)(val) = soc->ttl;
            break;
        case SOC_IP_TOS:
            *(UB *)(val) = soc->tos;
            break;
        case SOC_IP_LOCAL:
            host     = (T_NODE *)val;
            host->ver  = soc->ver;
            host->port = soc->lport;
            host->ipa  = 0;
#ifdef IPV6_SUP            
            if(soc->ver == IP_VER6)
                net_memset(host->ip6a, 0, IPV6_ADDR_LEN);
#endif          
            host->num  = 0;
            if (soc->net) {
#ifdef IPV6_SUP
                if(soc->ver == IP_VER6)
                    set_ip6_src(host->ip6a, soc->raddr6, NULL, soc->net->dev->num);
                else
#endif                  
                host->ipa = soc->net->adr->ipaddr;
                host->num = soc->net->dev->num;
            }
            break;
        case SOC_IP_REMOTE:
            host      = (T_NODE *)val;
            host->num = 0;
            if (soc->proto == IP_PROTO_TCP) {
                host->ver  = soc->ver;
                host->port = soc->rport;
#ifdef IPV6_SUP
                if(soc->ver == IP_VER6)
                    ip6_addr_cpy(host->ip6a, soc->raddr6);
                else
#endif
                host->ipa  = soc->raddr;
                if (soc->net) {
                    host->num = soc->net->dev->num;
                }
            }
            else {
                host->ver  = soc->rpi.ver;
                host->port = soc->rpi.src_port;
#ifdef IPV6_SUP
                if(soc->rpi.ver == IP_VER6)
                    ip6_addr_cpy(host->ip6a, soc->rpi.ip6sa);
                else
#endif
                host->ipa  = soc->rpi.src_ipa;
                host->num  = soc->rpi.num;
            }
            break;
        case SOC_RCV_PKT_INF:
            if (soc->proto == IP_PROTO_TCP) {
                if (soc->tcp->bsd == TCP_CLOSED) {
                    ercd = E_OBJ;
                    break;
                }
            }
            rpi = (T_RCV_PKT_INF *)val;
            net_memcpy((char *)rpi, (char *)&soc->rpi, sizeof(T_RCV_PKT_INF));
            break;
        case SOC_TCP_STATE:
            if (soc->proto == IP_PROTO_TCP) {
                *(UB *)val = soc->tcp->bsd;
            }
            else {
                ercd = E_OBJ;
            }
            break;
        case SOC_PRT_LOCAL:
            *(UH *)(val) = soc->lport;
            break;
#ifdef RCV_PKT_SIZE_SUP
        case SOC_RCV_PKT_SIZE:
            if ( soc->proto == IP_PROTO_TCP ) {
                // Get tcp recv size
                *(UW *)val = soc->tcp->rdatsz;
            } else if ( soc->proto == IP_PROTO_UDP ) {
                // Calc udp recv size(top block only)
                *(UW *)val = 0U;
                pkt = (T_NET_BUF *)soc->rdatque;
                if ( pkt != NULL ) {
                    *(UW *)val = (UW)(pkt->dat_len - UDP_HDR_SZ);
                }
            } else {
                ercd = E_NOSPT;
            }
            break;
#endif
        case SOC_IP_FLG_DF:
            (*(UB *)val) = (soc->flg & SOC_FLG_IPDF);
            break;

        default:
            ercd = E_NOSPT;
            break;
    }

    ulc_tcp();

    return ercd;
}

ER cfg_soc(SID sid, UB code, VP val)
{
    T_NET_SOC *soc;
    ER ercd;
    T_NET_BUF *pkt, **tmp;
    UH lport;
#ifdef MCAST_SOC_SUP
    T_NET *net;
#ifdef IPV6_SUP
    T_IPV6_ADDR *adr;
#endif
#endif
    H     index;

    if ((sid == 0) || (sid > NET_SOC_MAX)) {
        return E_ID;
    }

    loc_tcp();

    sid--;
    soc = &pNET_SOC[sid];

    if (soc->state == SOC_UNUSED) {
        ulc_tcp();
        return E_NOEXS;
    }

    ercd = E_OK;
    switch (code) {
        case SOC_CBK_HND:
            soc->cbk = (SOC_HND)((ADDR)val);
            break;
        case SOC_CBK_FLG:
            if ((soc->wptn & ((UH)((ADDR)val))) == (soc->wptn & soc->cptn)) {
                soc->cptn = (UH)((ADDR)val);
            }
            else {
                ercd = E_OBJ;
            }
            break;
        case SOC_TMO_CON:
            soc->con_tmo = (TMO)((ADDR)val);
            break;
        case SOC_TMO_CLS:
            soc->cls_tmo = (TMO)((ADDR)val);
            break;
        case SOC_TMO_SND:
            soc->snd_tmo = (TMO)((ADDR)val);
            break;
        case SOC_TMO_RCV:
            soc->rcv_tmo = (TMO)((ADDR)val);
            break;
        case SOC_IP_TTL:
            soc->ttl     = (UB)((ADDR)val);
            break;
        case SOC_IP_TOS:
            soc->tos     = (UB)((ADDR)val);
            break;
        case SOC_PRT_LOCAL:
            lport = (UH)((ADDR)val);
            if (lport == PORT_ANY) {
                lport = get_eport(soc->proto);
                soc->flg |= SOC_FLG_PRT;    /* ANY PORT is specified */
            } else {
                soc->flg &= ~SOC_FLG_PRT;
            }

            if (soc->proto == IP_PROTO_TCP) {
                if (soc->tcp->bsd != TCP_CLOSED) {
                    ercd = E_OBJ;
                    break;
                }
                soc->lport = lport;
            } else {
                if (soc->wptn    != 0    ||
                    soc->sdatque != NULL ||
                    soc->rdatque != NULL) {
                    ercd = E_OBJ;
                    break;
                }
                for (sid = 0; sid < NET_SOC_MAX; sid++) {
                    if (&pNET_SOC[sid]      != soc        &&
                        pNET_SOC[sid].state != SOC_UNUSED &&
                        pNET_SOC[sid].proto == soc->proto &&
                        pNET_SOC[sid].ver   == soc->ver   &&
                       (pNET_SOC[sid].usr_net == NULL  ||
                        soc->usr_net          == NULL  ||
                        pNET_SOC[sid].usr_net == soc->usr_net) &&
                        pNET_SOC[sid].lport == lport ) {
                        ercd = E_PAR;
                        break;
                    }
                }
                if (ercd != E_PAR) {
#ifdef REUSE_ADDR_SUP
                    /* Leave REUSEADDR */
                    rel_reuse_addr( soc );
#endif
                    soc->lport = lport;
                }
            }
            break;
#ifdef REUSE_ADDR_SUP
        case SOC_PRT_LOCAL_DUP:
            lport = (UH)((UW)((ADDR)val));
            if ( lport == PORT_ANY ) {
                lport = get_eport(soc->proto);
                soc->flg |= SOC_FLG_PRT;    /* ANY PORT is specified */
            } else {
                soc->flg &= ~SOC_FLG_PRT;
            }

            if ( soc->proto == IP_PROTO_TCP ) {
                if ( soc->tcp->bsd != TCP_CLOSED ) {
                    ercd = E_OBJ;
                    break;
                }
            } else {
                if ( soc->wptn    != 0    ||
                     soc->sdatque != NULL ||
                     soc->rdatque != NULL) {
                    ercd = E_OBJ;
                    break;
                }
                /* Leave REUSEADDR */
                rel_reuse_addr( soc );
                /* Join REUSEADDR */
                reg_reuse_addr( soc, lport );
            }
            soc->lport = lport;
            break;
#endif
#ifdef MCAST_SOC_SUP
        case SOC_MCAST_JOIN:
            net = soc->usr_net;
            if (net == NULL) {
                net = get_net_default();
            }
            index = mgroup_index(net, (UW)((ADDR)val));
            if (0 <=index &&  index < NET_MGR_MAX) {
                /* socket is already member */
                if (isset_mgr_idx(soc, index)) {
                    ercd = E_OK;
                    break;
                }
            }
            ercd = igmp_join(net, (UW)((ADDR)val));
            if (ercd != E_OK) {
                break;
            }
            index = mgroup_index(net, (UW)((ADDR)val));
            set_mgr_idx(soc, index);
            break;

        case SOC_MCAST_DROP:
            net = soc->usr_net;
            if (net == NULL) {
                net = get_net_default();
            }
            index = mgroup_index(net, (UW)((ADDR)val));
            if (index < 0 || NET_MGR_MAX <= index) {
                ercd = E_OBJ;
                break;
            }
            if (isset_mgr_idx(soc, index) == 0) {
                /* not member of mgroup */
                ercd = E_OBJ;
                break;
            }
            clr_mgr_idx(soc, index);
            ercd = igmp_leave(net, (UW)((ADDR)val));
            break;
#ifdef IPV6_SUP
        case SOC_MLD_START:
            net = soc->usr_net;
            if (net == NULL) {
                net = get_net_default();
            }
            adr = chk_if_ip6_addr((UW*)val, net->dev->num);
            if (adr != NULL && adr->state != MLD_NON_LSTNR) {
                if ((adr->flag & MLD_SOLICITING) == 0) {
                    index = adr->flag & MLD_INDEX_MSK;
                    /* socket is already member */
                    if (isset_mgr_idx(soc, index)) {
                        ercd = E_OK;
                        break;
                    }
                }
            }
            ercd = mld_add_adr((UW*)val, net->dev->num);
            if (ercd != E_OK) {
                break;
            }
            adr = chk_if_ip6_addr((UW*)val, net->dev->num);
            index = adr->flag & MLD_INDEX_MSK;
            set_mgr_idx(soc, index);
            break;

        case SOC_MLD_STOP:
            net = soc->usr_net;
            if (net == NULL) {
                net = get_net_default();
            }
            adr = chk_if_ip6_addr((UW*)val, net->dev->num);
            if (adr == NULL || adr->state == MLD_NON_LSTNR) {
                ercd = E_OBJ;
                break;
            }
            index = adr->flag & MLD_INDEX_MSK;
            if (isset_mgr_idx(soc, index) == 0) {
                /* not member of mgroup */
                ercd = E_OBJ;
                break;
            }
            clr_mgr_idx(soc, index);
            ercd = mld_rmv_adr((UW*)val, net->dev->num);
            break;
#endif /*IPv6 */
#endif
        case SOC_UDP_RQSZ:
            if (soc->proto != IP_PROTO_UDP) {
                ercd = E_NOSPT;
                break;
            }
            soc->rqsz = (UB)((ADDR)val);
            tmp = (T_NET_BUF**)&soc->rdatque;
            for (index = 0; index < soc->rqsz && *tmp != NULL; index++) {
                tmp = (T_NET_BUF**)&((*tmp)->next);
            }
            soc->rqsz -= index;
            while (*tmp) {
                pkt = *tmp;
                rmv4que_top((UW**)tmp);
                net_buf_ret(pkt);
            }
            break;
        case SOC_SRC_IP_RESET:
            if (soc->proto != IP_PROTO_UDP) {
                ercd = E_NOSPT;
                break;
            }
            if (val) {
                soc->flg |= SOC_FLG_NOSRC;
            } else {
                soc->flg &= ~SOC_FLG_NOSRC;
            }
            break;
        case SOC_UDP_EARLY_TX:
            if (soc->proto != IP_PROTO_UDP) {
                ercd = E_NOSPT;
                break;
            }
            if (val) {
                soc->flg |= SOC_FLG_EARLYTX;
            } else {
                soc->flg &= ~SOC_FLG_EARLYTX;
            }
            break;
#ifdef MSG_PEEK_SUP
        case SOC_MSG_PEEK:
            if (val) {
                soc->flg |= SOC_FLG_MSGPEEK;
            } else {
                soc->flg &= ~SOC_FLG_MSGPEEK;
            }
            break;
#endif
        case SOC_IP_FLG_DF:
            if (soc->proto != IP_PROTO_UDP) {
                ercd = E_NOSPT;
                break;
            }
            if (val) {
                soc->flg |= SOC_FLG_IPDF;
            } else {
                soc->flg &= ~SOC_FLG_IPDF;
            }
            break;
        default:
            ercd = E_NOSPT;
            break;
    }
    ulc_tcp();

    return ercd;
}

#ifdef IO_READY_SUP
ER snd_rdy(SID sid)
{
    T_NET_SOC *soc;
    ER ercd;

    if ((sid == 0) || (sid > NET_SOC_MAX)) {
        return E_ID;
    }

    loc_tcp();

    sid--;
    soc = &pNET_SOC[sid];
    ercd = E_WBLK;

    if (soc->proto == IP_PROTO_TCP) {
        /* TCP stataus */
        if (soc->tcp->bsd == TCP_ESTABLISHED || soc->tcp->bsd == TCP_CLOSE_WAIT) {
            /* tx path is not close yet ?*/
            if ((soc->tcp->sflg & TCP_FLG_FIN) == 0) {
                /* free space in TX buffer ?*/
                if (soc->tcp->snd_len < soc->tcp->sbufsz) {
                    ercd = E_OK;
                }
            }
        }
    } else {
        if (soc->sdatque == NULL) {
            ercd = E_OK;
        }
    }
    ulc_tcp();
    return ercd;
}

ER rcv_rdy(SID sid)
{
    T_NET_SOC *soc;
    ER ercd;

    if ((sid == 0) || (sid > NET_SOC_MAX)) {
        return E_ID;
    }

    loc_tcp();

    sid--;
    soc = &pNET_SOC[sid];
    ercd = E_WBLK;

    if (soc->proto == IP_PROTO_TCP) {
        if (0 < soc->tcp->rdatsz && soc->tcp->rdatque != NULL) {
            ercd = E_OK;
        } else if (soc->tcp->bsd == TCP_CLOSED    ||
                   soc->tcp->bsd == TCP_CLOSE_WAIT||
                   soc->tcp->bsd == TCP_LAST_ACK) {
            /* EOF */
            ercd = E_OK;
        }
    } else {
        if (soc->rdatque != NULL) {
            ercd = E_OK;
        }
    }

    ulc_tcp();
    return ercd;
}
#endif

static void clr_rcv_que(T_NET_SOC *soc)
{
    T_NET_BUF *pkt, *top, *tcp_osq;
    
    if (soc == NULL) {
        return;
    }

    tcp_osq = NULL;
    if (soc->proto == IP_PROTO_TCP) {
        if (soc->tcp == NULL) {
            return;
        }
        top = (T_NET_BUF *)soc->tcp->rdatque;
        tcp_osq = (T_NET_BUF *)soc->tcp->routque;
        soc->tcp->rdatsz = 0;
        soc->tcp->rdatque = NULL;
        soc->tcp->routque = NULL;
        soc->tcp->routque_cnt = 0;
    }
    else {
        top = (T_NET_BUF *)soc->rdatque;
        soc->rdatque = NULL;
    }

    while (tcp_osq) {
        pkt = tcp_osq;
        tcp_osq = (T_NET_BUF *)pkt->next;
        pkt->soc = NULL;
        net_buf_ret(pkt);
    }

    if (top == NULL) {
        return;
    }

    for (;;) {
        pkt = top;
        if (pkt == NULL) {
            break;
        }
        top = (T_NET_BUF *)pkt->next;
        pkt->soc = NULL;
        net_buf_ret(pkt);
        if (soc->proto != IP_PROTO_TCP) {
            soc->rqsz++;
        }
    }
}

ER soc_event(T_NET_SOC *soc, UH ptn, ER ercd)
{
    UH ntf_evt;
#ifdef IO_READY_SUP
    UH rdy_evt = 0;
#endif

    if (ptn & EV_SOC_CLS) {
        ptn |= EV_SOC_RCV;
    }

#ifdef IO_READY_SUP
    /*
    Note: EV_RDY_SND / EV_RDY_RCV
      These are the event indicating ready to call.
      TCP connected evnet does not generate event.
      If socket is waiting for snd/rcv events already, does not notify.
      This event is notified to only application that is set by cfg_soc().
    */
    if (ercd != E_RLWAI) {   /* E_RLWAI equals API cancel */
        if ((ptn & EV_SOC_SND) && ((ptn & EV_SOC_CLS) == 0)) {
            rdy_evt |= EV_RDY_SND;
        }
        if (ptn & EV_SOC_RCV) {
            rdy_evt |= EV_RDY_RCV;
        }
        if (soc->wptn & EV_SOC_SND) {
            rdy_evt &= ~EV_RDY_SND;
        }
        if (soc->wptn & EV_SOC_RCV) {
            rdy_evt &= ~EV_RDY_RCV;
        }
    }
#endif

    /* TCP: CON, CLS, SND & RCV Events */
    /* UDP: SND & RCV Events           */
    if (soc->wptn & ptn) {
        if (soc->cptn & ptn) {
            ntf_evt = (ptn & soc->wptn & soc->cptn);
            soc->wptn &= ~(UH)(soc->cptn & ptn);
            if (soc->cbk && ntf_evt) {
                soc->cbk(soc->sid + 1, ntf_evt, ercd);
            }
            ptn = (soc->wptn & ptn);
        }
        if ((ptn & (EV_SOC_CON|EV_SOC_CLS)) && soc->ctskid) {
            soc->ercd = ercd;
            wup_tsk(soc->ctskid);
            soc->ctskid = 0;
        }
        if ((ptn & EV_SOC_SND) && soc->stskid) {
            soc->slen = ercd;
            wup_tsk(soc->stskid);
            soc->stskid = 0;
            ptn &= ~(UH)(EV_SOC_SND);
        }
        if ((ptn & EV_SOC_RCV) && soc->rtskid) {
            soc->rlen = ercd;
            wup_tsk(soc->rtskid);
            soc->rtskid = 0;
            ptn &= ~(UH)(EV_SOC_RCV);
        }
    }

#ifdef IO_READY_SUP
    if (soc->cbk) {
        if (rdy_evt & EV_RDY_SND & soc->cptn) {
            soc->cbk(soc->sid + 1, EV_RDY_SND, ercd);
        }
        if (rdy_evt & EV_RDY_RCV & soc->cptn) {
            soc->cbk(soc->sid + 1, EV_RDY_RCV, ercd);
        }
    }
#endif

    return E_OK;
}

void soc_wakeup(T_NET_BUF *pkt)
{
    T_NET_SOC *soc;

    if (pkt == NULL) {
        return;
    }

    soc = (T_NET_SOC *)pkt->soc;
    if (soc && (soc->sdatque == (UW*)pkt)) {
        soc->sdatque = NULL;
        soc_event(soc, EV_SOC_SND, pkt->ercd);
    }
}

void soc_reset(T_NET *net)
{
    T_NET_SOC *soc;
    UH i;

    for (i = 0; i < NET_TCP_MAX; i++) {
        soc = &pNET_SOC[i];
        if (soc->net == net) {
            if (soc->tcp->bsd > TCP_LISTEN) {
                tcp_abt(soc->tcp, 2);
                soc_event(soc, EV_SOC_ALL, E_SYS);
                clr_rcv_que(soc);
            }
        }
    }
}

ER soc_ext(UH dev_num)
{
    T_NET *net;
    T_NET_SOC *soc;
    SID sid;

    if (dev_num > NET_DEV_MAX) {
        return E_PAR;
    }

    if (dev_num == DEV_ANY) {
        net = NULL;
    } else {
        net = get_net_bynum(dev_num-1);
    }

    loc_tcp();

    for (sid = 1; sid <= NET_SOC_MAX; sid++) {
        soc = &pNET_SOC[sid-1];
        if (net == NULL || soc->net == net) {
            ulc_tcp();
            abt_soc(sid, SOC_ABT_ALL);
            loc_tcp();
            soc->net  = soc->usr_net;
            soc->wptn = 0;
            soc->sdatque = NULL;
            clr_rcv_que(soc);
            soc->ctskid = 0;
            soc->stskid = 0;
            soc->rtskid = 0;
        }
    }
    ulc_tcp();
    return E_OK;
}

#ifdef REUSE_ADDR_SUP
static void reg_reuse_addr(T_NET_SOC *soc, UH lport)
{
    SID sid;

    for ( sid = 0U; sid < NET_SOC_MAX; sid++ ) {
        if ( &pNET_SOC[sid]      != soc &&
             pNET_SOC[sid].state != SOC_UNUSED &&
             pNET_SOC[sid].proto == soc->proto &&
             pNET_SOC[sid].ver   == soc->ver   &&
             (pNET_SOC[sid].usr_net == NULL || pNET_SOC[sid].usr_net == soc->usr_net) &&
             pNET_SOC[sid].lport == lport ) {
            pNET_SOC[sid].ruanum++;
            soc->ruanum++;
        }
    }
}

static void rel_reuse_addr(T_NET_SOC *soc)
{
    SID sid;

    if ( soc->ruanum == 0 ) {
        /* Skip */
        return;
    }

    for ( sid = 0U; sid < NET_SOC_MAX; sid++ ) {
        if ( &pNET_SOC[sid]      != soc &&
             pNET_SOC[sid].state != SOC_UNUSED &&
             pNET_SOC[sid].proto == soc->proto &&
             pNET_SOC[sid].ver   == soc->ver   &&
             (pNET_SOC[sid].usr_net == NULL || pNET_SOC[sid].usr_net == soc->usr_net) &&
             pNET_SOC[sid].lport == soc->lport ) {
            pNET_SOC[sid].ruanum--;
            soc->ruanum--;
        }
    }
}
#endif
/*EOF*/
