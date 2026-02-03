/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK
    BSD socket wrapper/Socket option function
    Copyright (c) 2014-2024, eForce Co., Ltd. All rights reserved.

    Version Information
        2014.04.28: Created
        2014.07.10: Corrected set_devid_soc()
        2015.02.09: Modify set_devid_soc()
        2015.03.18: Fixed address byte order at IP_ADD_MEMBERSHIP, IP_DROP_MEMBERSHIP
        2015.08.25: Fixed TCP_MAXSEG option
        2016.05.12: Modify the broadcast options
        2016.06.16  Support the SO_REUSEADDR option
        2016.09.23: Update IP_ADD_MEMBERSHIP and IP_DROP_MEMBERSHIP options
        2018.08.23: Update IP_DONTFRAGMENT, IP_MULTICAST_IF and IP_MULTICAST_LOOP options
        2020.03.23: Suppressed warning of the 64bit GCC compiler.
        2020.09.07: Supported IPv6.
        2023.06.23: Changed UDP receive queue setting processing of so_rcvbuf().
        2024.04.03: Suppressed warning of the 64bit GCC compiler. (for 2023.06.23 update)
        2024.04.16: Fixed issue where KeepAlive option was only applied to 1st device.
 ***************************************************************************/

#include "kernel.h"
#include "net_hdr.h"
#include "net_def.h"
#include "net_strlib.h"

#include "unet3_socket.h"
#include "bsd_param.h"
#include "unet3_wrap.h"

#define SETOPTION  1 
#define GETOPTION  2 

#define VALID_MSG_OPTLEN(m,t)   ((API_ID_GETSOCKOPT == (m)->apiid) || (sizeof(t) <= (m)->par.sockopt.optlen))


void loc_bsd(void)
{
}

void ulc_bsd(void)
{
}

int get_devid_addr(UW ip)
{
    int i;

    loc_tcp();
    for (i = 0; i < NET_DEV_MAX; i++) {
        if (gNET_ADR[i].ipaddr == ip) {
            ulc_tcp();
            return i+1;
        }
    }
    ulc_tcp();
    return DEV_ANY;
}

#ifdef IPV6_SUP
int get_devid_ip6addr(UW *ip6addr)
{
    int i;
    int cnt;
    T_NET6_ADR *adr6;

    loc_tcp();
    for (i = 0; i < NET_DEV_MAX; i++) {
        adr6 = &gNET6_ADR[i];
        for (cnt = 0; cnt < DEF_UCAST_CNT; cnt++) {
            if ((adr6->ip6addr_ucast[cnt].state == IP6_ADDR_PREFERRED) &&
                (ip6_addr_cmp(ip6addr, adr6->ip6addr_ucast[cnt].addr_uw) == TRUE)) {
                ulc_tcp();
                return i+1;
            }
        }
    }
    ulc_tcp();
    return DEV_ANY;
}
#endif

int get_devid_soc(int sockfd)
{
    int i;

    if (sockfd < 1) {
        return DEV_ANY;
    }

    loc_tcp();
    for (i = 0; i < NET_DEV_MAX; i++) {
        if (pNET_SOC[sockfd-1].net == &gNET[i]) {
            ulc_tcp();
            return i+1;
        }
    }
    ulc_tcp();
    return DEV_ANY;
}

int get_devid_name(const char *name)
{
    int i;

    loc_tcp();
    for (i = 0; i < NET_DEV_MAX; i++) {
        if (net_strcmp(name, (const char*)gNET_DEV[i].name) == 0) {
            ulc_tcp();
            return i+1;
        }
    }
    ulc_tcp();
    return DEV_ANY;
}

void set_devid_soc(int sockfd, int devid)
{
    if (sockfd < 1 || sockfd > NET_SOC_MAX || devid > NET_DEV_MAX) {
        return;
    }
    if (devid == DEV_ANY) {
        devid = 1;
    }

    loc_tcp();
    pNET_SOC[sockfd-1].usr_net = pNET_SOC[sockfd-1].net = &gNET[devid-1];
    ulc_tcp();
}


UB can_read(int sockfd)
{
    T_UNET3_BSD_SOC *soc;
    ER ercd;
    UB sts;

    soc = &gNET_BSD_SOC[sockfd-1];

    if (soc->proto == IP_PROTO_TCP && soc->listenq != NULL) {
        ref_soc(((T_UNET3_BSD_SOC*)soc->listenq)->sockfd, SOC_TCP_STATE, &sts);
        if (sts == TCP_ESTABLISHED || sts == TCP_CLOSE_WAIT) {
            return 1;
        }
        return 0;
    }

    ercd = rcv_rdy(sockfd);
    if (ercd == E_OK) {
        return 1;
    }
    return 0;
}

UB can_write(int sockfd)
{
    ER ercd;
    ercd = snd_rdy(sockfd);
    if (ercd == E_OK) {
        return 1;
    }
    return 0;
}


/* SO_ACCEPTCONN */
void so_acceptconn(T_UNET3_API_MSG *msg)
{
    T_PARAM_SOCKOPT *opt = &msg->par.sockopt;

    if (!VALID_MSG_OPTLEN(msg, int)) {
        msg->ercd = E_PAR;
        set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
        return;
    }

    if (msg->apiid == API_ID_GETSOCKOPT) {
        if (gNET_BSD_SOC[msg->sockfd-1].listenq) {
            *(int*)opt->optval = 1;
        } else {
            *(int*)opt->optval = 0;
        }
        msg->ercd = E_OK;
    } 
}

/* SO_BROADCAST */
void so_broadcast(T_UNET3_API_MSG *msg)
{
    T_PARAM_SOCKOPT *opt = &msg->par.sockopt;

    if (!VALID_MSG_OPTLEN(msg, UW)) {
        msg->ercd = E_PAR;
        set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
        return;
    }

    /* only datagram */
    if (gNET_BSD_SOC[msg->sockfd-1].proto == IP_PROTO_TCP) {
        return;
    }

    if (msg->apiid == API_ID_GETSOCKOPT) {
        *((UB *)&opt->optval[0]) = (UB)gNET_BSD_SOC[msg->sockfd-1].broadcast;
        msg->ercd = E_OK;
    } else {
        gNET_BSD_SOC[msg->sockfd-1].broadcast = (BOOL)*((UB *)&opt->optval[0]);
        msg->ercd = E_OK;
    }
}

/* SO_DOMAIN */
void so_domain(T_UNET3_API_MSG *msg)
{
    T_PARAM_SOCKOPT *opt = &msg->par.sockopt;

    if (!VALID_MSG_OPTLEN(msg, int)) {
        msg->ercd = E_PAR;
        set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
        return;
    }

    loc_tcp();
    if (msg->apiid == API_ID_GETSOCKOPT) {
        *(int*)opt->optval = pNET_SOC[msg->sockfd-1].ver;
        msg->ercd = E_OK;
    }
    ulc_tcp();
}

/* SO_ERROR */
void so_error(T_UNET3_API_MSG *msg)
{
    T_PARAM_SOCKOPT *opt = &msg->par.sockopt;

    if (!VALID_MSG_OPTLEN(msg, ER)) {
        msg->ercd = E_PAR;
        set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
        return;
    }

    if (msg->apiid == API_ID_GETSOCKOPT) {
        *(ER*)opt->optval = gNET_BSD_SOC[msg->sockfd-1].so_err;
        gNET_BSD_SOC[msg->sockfd-1].so_err = 0;
        msg->ercd = E_OK;
    }
}

/* SO_KEEPALIVE */
void tcp_keepcnt(T_UNET3_API_MSG *msg);
void so_keepalive(T_UNET3_API_MSG *msg)
{
    T_PARAM_SOCKOPT *opt = &msg->par.sockopt;

    if (!VALID_MSG_OPTLEN(msg, int)) {
        msg->ercd = E_PAR;
        set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
        return;
    }

    /* only tcp */
    if (gNET_BSD_SOC[msg->sockfd-1].proto != IP_PROTO_TCP) {
        return;
    }

    if (msg->apiid == API_ID_SETSOCKOPT) {
        if (*(int*)opt->optval) {
            *(int*)opt->optval = 9; /* default */
        }
        /* uNet3 spec. set keep alive count to be enable */
        tcp_keepcnt(msg);
        msg->ercd = E_OK;
    }
}

/* SO_RCVBUF */
void so_rcvbuf(T_UNET3_API_MSG *msg)
{
    T_PARAM_SOCKOPT *opt = &msg->par.sockopt;
    UB tmp;

    if (!VALID_MSG_OPTLEN(msg, UW)) {
        msg->ercd = E_PAR;
        set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
        return;
    }

    loc_tcp();
    if (msg->apiid == API_ID_GETSOCKOPT) {
        /* TCP */
        if (gNET_BSD_SOC[msg->sockfd-1].proto == IP_PROTO_TCP) {
            *(UW*)opt->optval = pNET_TCP[msg->sockfd-1].rbufsz;
        } else {
            *(UW*)opt->optval = pNET_SOC[msg->sockfd-1].rqsz;
        }
        msg->ercd = E_OK;
    } else {
        /* TCP */
        if (gNET_BSD_SOC[msg->sockfd-1].proto == IP_PROTO_TCP) {
            pNET_TCP[msg->sockfd-1].rbufsz = *(UW*)opt->optval;
        } else {
            // rbufsz is UB type, check if it does not exceed 255
            if (255U < *(UW*)opt->optval) {
                msg->ercd = E_PAR;  /* EINVAL */
                set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
                ulc_tcp();
                return;                
            } 
            tmp = (UB)*(UW*)opt->optval;
            ulc_tcp();
			cfg_soc(msg->sockfd, SOC_UDP_RQSZ, (VP)(ADDR)tmp);
            loc_tcp();
        }
        msg->ercd = E_OK;
    }
    ulc_tcp();
}

/* SO_RCVTIMEO */
void so_rcvtimeo(T_UNET3_API_MSG *msg)
{
    T_PARAM_SOCKOPT *opt = &msg->par.sockopt;
    T_UNET3_BSD_SOC *soc = &gNET_BSD_SOC[msg->sockfd-1];

    if (!VALID_MSG_OPTLEN(msg, unet3_timeval)) {
        msg->ercd = E_PAR;
        set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
        return;
    }

    if (msg->apiid == API_ID_GETSOCKOPT) {
        if (opt->optlen >= sizeof(unet3_timeval)) {
            opt->optlen = sizeof(unet3_timeval);
        }
        net_memcpy(opt->optval, &soc->rcvtm, opt->optlen);
        msg->ercd = E_OK;
    } else {
        if (opt->optlen >= sizeof(unet3_timeval)) {
            opt->optlen = sizeof(unet3_timeval);
        }
        net_memcpy(&soc->rcvtm, opt->optval, opt->optlen);
        if (soc->rcvtm.tv_sec < 0 || TIME_MAX_SEC < soc->rcvtm.tv_sec) {
            soc->rcvtm.tv_sec  = TIME_MAX_SEC;
            soc->rcvtm.tv_usec = TIME_MAX_SEC_USEC;
        }
        msg->ercd = E_OK;
    }
}

/* SO_SNDTIMEO */
void so_sndtimeo(T_UNET3_API_MSG *msg)
{
    T_PARAM_SOCKOPT *opt = &msg->par.sockopt;
    T_UNET3_BSD_SOC *soc = &gNET_BSD_SOC[msg->sockfd-1];

    if (!VALID_MSG_OPTLEN(msg, unet3_timeval)) {
        msg->ercd = E_PAR;
        set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
        return;
    }

    if (msg->apiid == API_ID_GETSOCKOPT) {
        if (opt->optlen >= sizeof(unet3_timeval)) {
            opt->optlen = sizeof(unet3_timeval);
        }
        net_memcpy(opt->optval, &soc->sndtm, opt->optlen);
        msg->ercd = E_OK;
    } else {
        if (opt->optlen >= sizeof(unet3_timeval)) {
            opt->optlen = sizeof(unet3_timeval);
        }
        net_memcpy(&soc->sndtm, opt->optval, opt->optlen);
        if (soc->sndtm.tv_sec < 0 || TIME_MAX_SEC < soc->sndtm.tv_sec) {
            soc->sndtm.tv_sec  = TIME_MAX_SEC;
            soc->sndtm.tv_usec = TIME_MAX_SEC_USEC;
        }
        msg->ercd = E_OK;
    }
}

/* SO_TYPE */
void so_type(T_UNET3_API_MSG *msg)
{
    T_PARAM_SOCKOPT *opt = &msg->par.sockopt;

    if (!VALID_MSG_OPTLEN(msg, int)) {
        msg->ercd = E_PAR;
        set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
        return;
    }

    if (msg->apiid == API_ID_GETSOCKOPT) {
        *(int*)opt->optval = gNET_BSD_SOC[msg->sockfd-1].proto;
        msg->ercd = E_OK;
    }
}

/* SO_REUSEADDR */
void so_reuseaddr(T_UNET3_API_MSG *msg)
{
    T_PARAM_SOCKOPT *opt = &msg->par.sockopt;
    T_UNET3_BSD_SOC *soc = &gNET_BSD_SOC[msg->sockfd-1];

    if (!VALID_MSG_OPTLEN(msg, int)) {
        msg->ercd = E_PAR;
        set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
        return;
    }

    if (msg->apiid == API_ID_GETSOCKOPT) {
        if ( (soc->so_flags & UNET3_OPT_REUSEADDR) != 0UL ) {
            *(int *)&opt->optval[0] = 1;
        } else {
            *(int *)&opt->optval[0] = 0;
        }
    } else {
        if ( *(int *)&opt->optval[0] != 0 ) {
            soc->so_flags |= UNET3_OPT_REUSEADDR;
        } else {
            soc->so_flags &= ~UNET3_OPT_REUSEADDR;
        }
    }

    msg->ercd = E_OK;

    return;
}

/* IP_ADD_MEMBERSHIP */
void ip_add_membership(T_UNET3_API_MSG *msg)
{
    T_NET *net;
    int devid;
    UW  ip;
    unet3_ip_mreqn *mreq;
    mreq = (unet3_ip_mreqn*)&msg->par.sockopt.optval[0];

    if (!VALID_MSG_OPTLEN(msg, unet3_ip_mreqn)) {
        msg->ercd = E_PAR;
        set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
        return;
    }

    if (msg->apiid == API_ID_SETSOCKOPT) {
        /* for multi device */
        ip = mreq->imr_address.s_addr;
        if (ip != INADDR_ANY) {
            devid = get_devid_addr(ntohl(ip));
            if (devid == DEV_ANY) {
                msg->ercd = E_PAR;
                set_errno(msg->aptskid, UNET3_ER_EADDRNOTAVAIL, msg->sockfd);
                return;
            }
            net = pNET_SOC[msg->sockfd-1].usr_net;
            loc_tcp();
            pNET_SOC[msg->sockfd-1].usr_net = &gNET[devid-1];
            ulc_tcp();
        }

        msg->ercd = cfg_soc(msg->sockfd, SOC_MCAST_JOIN, (VP)((ADDR)ntohl(mreq->imr_multiaddr.s_addr)));
        if (ip != INADDR_ANY) {
            loc_tcp();
            pNET_SOC[msg->sockfd-1].usr_net = net;
            ulc_tcp();
        }
    }
}

/* IP_DROP_MEMBERSHIP */
void ip_drop_membership(T_UNET3_API_MSG *msg)
{
    T_NET *net;
    int devid;
    UW  ip;
    unet3_ip_mreqn *mreq;
    mreq = (unet3_ip_mreqn*)&msg->par.sockopt.optval[0];

    if (!VALID_MSG_OPTLEN(msg, unet3_ip_mreqn)) {
        msg->ercd = E_PAR;
        set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
        return;
    }

    if (msg->apiid == API_ID_SETSOCKOPT) {
        /* for multi device */
        ip = mreq->imr_address.s_addr;
        if (ip != INADDR_ANY) {
            devid = get_devid_addr(ntohl(ip));
            if (devid == DEV_ANY) {
                msg->ercd = E_PAR;
                set_errno(msg->aptskid, UNET3_ER_EADDRNOTAVAIL, msg->sockfd);
                return;
            }
            net = pNET_SOC[msg->sockfd-1].usr_net;
            loc_tcp();
            pNET_SOC[msg->sockfd-1].usr_net = &gNET[devid-1];
            ulc_tcp();
        }

        msg->ercd = cfg_soc(msg->sockfd, SOC_MCAST_DROP, (VP)((ADDR)ntohl(mreq->imr_multiaddr.s_addr)));
        if (ip != INADDR_ANY) {
            loc_tcp();
            pNET_SOC[msg->sockfd-1].usr_net = net;
            ulc_tcp();
        }
    }
}

/* IP_HDRINCL */
void ip_hdrincl(T_UNET3_API_MSG *msg)
{
    T_PARAM_SOCKOPT *opt = &msg->par.sockopt;

    if (!VALID_MSG_OPTLEN(msg, int)) {
        msg->ercd = E_PAR;
        set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
        return;
    }

    if (msg->apiid == API_ID_GETSOCKOPT) {
        *(int*)opt->optval = gNET_BSD_SOC[msg->sockfd-1].hdrincl ? 1 : 0;
    } else {
        gNET_BSD_SOC[msg->sockfd-1].hdrincl = *(int*)opt->optval ? TRUE : FALSE;
    }
    msg->ercd = E_OK;
}

/* IP_MTU */
void ip_mtu(T_UNET3_API_MSG *msg)
{
    T_PARAM_SOCKOPT *opt = &msg->par.sockopt;
    T_NET *net;
    int devid;

    if (!VALID_MSG_OPTLEN(msg, UW)) {
        msg->ercd = E_PAR;
        set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
        return;
    }

    if (msg->apiid == API_ID_GETSOCKOPT) {
        devid = get_devid_soc(msg->sockfd);
        if (devid != DEV_ANY) {
            devid--;
        }
        net = &gNET[devid];
        *(UW*)opt->optval = PATH_MTU;
        msg->ercd = E_OK;
    }
}

/* IP_MULTICAST_TTL*/
void ip_multicast_ttl(T_UNET3_API_MSG *msg)
{
    T_PARAM_SOCKOPT *opt = &msg->par.sockopt;
    int devid;

    if (!VALID_MSG_OPTLEN(msg, int)) {
        msg->ercd = E_PAR;
        set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
        return;
    }

    devid = get_devid_soc(msg->sockfd);
    if (msg->apiid == API_ID_GETSOCKOPT) {
        net_ref(devid, NET_MCAST_TTL, opt->optval);
    } else {
        net_cfg(devid, NET_MCAST_TTL, (VP)(ADDR)((*((int*)opt->optval))));
    }
    msg->ercd = E_OK;
}

/* IP_PKTINFO*/
void ip_pktinfo(T_UNET3_API_MSG *msg)
{
    T_PARAM_SOCKOPT *opt = &msg->par.sockopt;

    if (!VALID_MSG_OPTLEN(msg, unet3_in_pktinfo)) {
        msg->ercd = E_PAR;
        set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
        return;
    }

    /* only datagram */
    if (gNET_BSD_SOC[msg->sockfd-1].proto == IP_PROTO_TCP) {
        return;
    }

    if (msg->apiid == API_ID_SETSOCKOPT) {
        gNET_BSD_SOC[msg->sockfd-1].pktinf.enable = *(int*)opt->optval ? TRUE : FALSE;
    }
    msg->ercd = E_OK;
}

/* IP_TOS */
void ip_tos(T_UNET3_API_MSG *msg)
{
    VB *tos = &msg->par.sockopt.optval[0];

    if (!VALID_MSG_OPTLEN(msg, int)) {
        msg->ercd = E_PAR;
        set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
        return;
    }

    if (msg->apiid == API_ID_GETSOCKOPT) {
        ref_soc(msg->sockfd, SOC_IP_TOS, tos);
    } else {
        cfg_soc(msg->sockfd, SOC_IP_TOS, (VP)((ADDR)(*(int*)tos)));
    }
    msg->ercd = E_OK;
}

/* IP_TTL */
void ip_ttl(T_UNET3_API_MSG *msg)
{
    VB *ttl = &msg->par.sockopt.optval[0];

    if (!VALID_MSG_OPTLEN(msg, int)) {
        msg->ercd = E_PAR;
        set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
        return;
    }

    if (msg->apiid == API_ID_GETSOCKOPT) {
        ref_soc(msg->sockfd, SOC_IP_TTL, ttl);
    } else {
        cfg_soc(msg->sockfd, SOC_IP_TTL, (VP)((ADDR)(*(int*)ttl)));
    }
    msg->ercd = E_OK;
}

/* IP_MULTICAST_IF */
void ip_multicast_if(T_UNET3_API_MSG *msg)
{
    UW ip;
    T_UNET3_BSD_SOC *soc;

    if (!VALID_MSG_OPTLEN(msg, UW)) {
        msg->ercd = E_PAR;
        set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
        return;
    }

    ip = (UW)((ADDR)&msg->par.sockopt.optval[0]);
    soc = &gNET_BSD_SOC[msg->sockfd-1];

    if (msg->apiid == API_ID_SETSOCKOPT) {
        soc->mcif = get_devid_addr(ntohl(ip));
    }
    msg->ercd = E_OK;
}


/* IP_MULTICAST_LOOP */
void ip_multicast_loop(T_UNET3_API_MSG *msg)
{
    VB *ml = &msg->par.sockopt.optval[0];
    int devid;

    if (!VALID_MSG_OPTLEN(msg, int)) {
        msg->ercd = E_PAR;
        set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
        return;
    }

    devid = get_devid_soc(msg->sockfd);
    if (msg->apiid == API_ID_GETSOCKOPT) {
        net_ref(devid, NET_MCAST_LOOP, ml);
    } else {
        net_cfg(devid, NET_MCAST_LOOP, (VP)((ADDR)(*(int*)ml)));
    }
    msg->ercd = E_OK;
}

/* IP_DONTFRAGMENT */
void ip_dontfragment(T_UNET3_API_MSG *msg)
{
    VB *df = &msg->par.sockopt.optval[0];

    if (!VALID_MSG_OPTLEN(msg, int)) {
        msg->ercd = E_PAR;
        set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
        return;
    }

    /* only datagram */
    if (gNET_BSD_SOC[msg->sockfd-1].proto != IP_PROTO_UDP) {
        msg->ercd = E_ID;
        set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
        return;
    }

    if (msg->apiid == API_ID_GETSOCKOPT) {
        ref_soc(msg->sockfd, SOC_IP_FLG_DF, df);
    } else {
        cfg_soc(msg->sockfd, SOC_IP_FLG_DF, (VP)((ADDR)(*(int*)df)));
    }
    msg->ercd = E_OK;
}

/* TCP_KEEPCNT */
void tcp_keepcnt(T_UNET3_API_MSG *msg)
{
    T_PARAM_SOCKOPT *opt = &msg->par.sockopt;
    T_NET *net;
    int i;

    if (!VALID_MSG_OPTLEN(msg, UW)) {
        msg->ercd = E_PAR;
        set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
        return;
    }

    /* only tcp */
    if (gNET_BSD_SOC[msg->sockfd-1].proto != IP_PROTO_TCP) {
        return;
    }

    loc_tcp();
    if (msg->apiid == API_ID_SETSOCKOPT) {
        for (i = 0; i < NET_DEV_MAX; i++) {
            net = &gNET[i];
            TCP_KPA_CNT = (*(UW*)opt->optval);
        }
        msg->ercd = E_OK;
    }
    ulc_tcp();
}

/* TCP_KEEPIDLE */
void tcp_keepidle(T_UNET3_API_MSG *msg)
{
    T_PARAM_SOCKOPT *opt = &msg->par.sockopt;
    T_NET *net;
    int i;

    if (!VALID_MSG_OPTLEN(msg, UW)) {
        msg->ercd = E_PAR;
        set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
        return;
    }

    /* only tcp */
    if (gNET_BSD_SOC[msg->sockfd-1].proto != IP_PROTO_TCP) {
        return;
    }

    loc_tcp();
    if (msg->apiid == API_ID_SETSOCKOPT) {
        for (i = 0; i < NET_DEV_MAX; i++) {
            net = &gNET[i];
            TCP_KPA_TMO = (*(UW*)opt->optval) * 1000;
        }
        msg->ercd = E_OK;
    }
    ulc_tcp();
}

/* TCP_KEEPINTVL */
void tcp_keepintvl(T_UNET3_API_MSG *msg)
{
    T_PARAM_SOCKOPT *opt = &msg->par.sockopt;
    T_NET *net;
    int i;

    if (!VALID_MSG_OPTLEN(msg, UW)) {
        msg->ercd = E_PAR;
        set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
        return;
    }

    /* only tcp */
    if (gNET_BSD_SOC[msg->sockfd-1].proto != IP_PROTO_TCP) {
        return;
    }

    loc_tcp();
    if (msg->apiid == API_ID_SETSOCKOPT) {
        for (i = 0; i < NET_DEV_MAX; i++) {
            net = &gNET[i];
            TCP_KPA_INT = (*(UW*)opt->optval) * 1000;
        }
        msg->ercd = E_OK;
    }
    ulc_tcp();

}

/* TCP_MAXSEG */
void tcp_maxseg(T_UNET3_API_MSG *msg)
{
    T_PARAM_SOCKOPT *opt = &msg->par.sockopt;
    int devid;
    T_NET *net;

    if (!VALID_MSG_OPTLEN(msg, UW)) {
        msg->ercd = E_PAR;
        set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
        return;
    }

    /* only tcp */
    if (gNET_BSD_SOC[msg->sockfd-1].proto != IP_PROTO_TCP) {
        return;
    }
    devid = get_devid_soc(msg->sockfd);
    if (devid != DEV_ANY) {
        devid--;
    }

    loc_tcp();
    net = &gNET[devid];
    if (msg->apiid == API_ID_GETSOCKOPT) {
        *(UW*)opt->optval = TCP_MSS;
    } else {
        TCP_MSS = *(UW*)opt->optval;
    }
    ulc_tcp();

    msg->ercd = E_OK;
}

#ifdef IPV6_SUP
/* IPV6_ADD_MEMBERSHIP */
void ipv6_add_membership(T_UNET3_API_MSG *msg)
{
    UW ipv6[4];
    unet3_ipv6_mreq *mreq;
    mreq = (unet3_ipv6_mreq*)&msg->par.sockopt.optval[0];

    if (!VALID_MSG_OPTLEN(msg, unet3_ipv6_mreq)) {
        msg->ercd = E_PAR;
        set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
        return;
    }

    if (msg->apiid == API_ID_SETSOCKOPT) {
        /* for multi device */
        ip6_addr_ntoh(ipv6, mreq->ipv6mr_multiaddr.unet3_s6_addr32);
        msg->ercd = cfg_soc(msg->sockfd, SOC_MLD_START, (VP)ipv6);
    }
}

/* IPV6_DROP_MEMBERSHIP */
void ipv6_drop_membership(T_UNET3_API_MSG *msg)
{
    UW ipv6[4];
    unet3_ipv6_mreq *mreq;
    mreq = (unet3_ipv6_mreq*)&msg->par.sockopt.optval[0];

    if (!VALID_MSG_OPTLEN(msg, unet3_ipv6_mreq)) {
        msg->ercd = E_PAR;
        set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
        return;
    }

    if (msg->apiid == API_ID_SETSOCKOPT) {
        /* for multi device */
        ip6_addr_ntoh(ipv6, mreq->ipv6mr_multiaddr.unet3_s6_addr32);
        msg->ercd = cfg_soc(msg->sockfd, SOC_MLD_STOP, (VP)ipv6);
    }
}
#endif

void wrap_socketoption(T_UNET3_API_MSG *msg)
{
    T_PARAM_SOCKOPT *opt = &msg->par.sockopt;

    msg->ercd = E_NOSPT;

    if (!VALID_SOC(msg->sockfd)) {
        msg->ercd = E_ID;
        set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
        return;
    }
    loc_bsd();

    if (opt->level == UNET3_SOL_SOCKET) {
        switch(opt->optname) {
        case UNET3_SO_ACCEPTCONN:
            so_acceptconn(msg);
            break;
        case UNET3_SO_BROADCAST:
            so_broadcast(msg);
            break;
        case UNET3_SO_DOMAIN:
            so_domain(msg);
            break;
        case UNET3_SO_ERROR:
            so_error(msg);
            break;
        case UNET3_SO_KEEPALIVE:
            so_keepalive(msg);
            break;
        case UNET3_SO_RCVBUF:
        case UNET3_SO_RCVBUFFORCE:
            so_rcvbuf(msg);
            break;
        case UNET3_SO_RCVTIMEO:
            so_rcvtimeo(msg);
            break;
        case UNET3_SO_SNDTIMEO:
            so_sndtimeo(msg);
            break;
        case UNET3_SO_TYPE:
            so_type(msg);
            break;
        case UNET3_SO_REUSEADDR:
            so_reuseaddr(msg);
            break;
        /* not support */
        case UNET3_SO_BINDTODEVICE:
        case UNET3_SO_BSDCOMPAT:
        case UNET3_SO_DEBUG:
        case UNET3_SO_DONTROUTE:
        case UNET3_SO_LINGER:
        case UNET3_SO_MARK:
        case UNET3_SO_OOBINLINE:
        case UNET3_SO_PASSCRED:
        case UNET3_SO_PEEK_OFF:
        case UNET3_SO_PEERCRED:
        case UNET3_SO_PRIORITY:
        case UNET3_SO_PROTOCOL:
        case UNET3_SO_RCVLOWAT:
        case UNET3_SO_SNDLOWAT:
        case UNET3_SO_SNDBUF:
        case UNET3_SO_SNDBUFFORCE:
        case UNET3_SO_TIMESTAMP:
        default:
            break;
        }
    } else if (opt->level == UNET3_IPPROTO_IP) {
        switch(opt->optname) {
        case UNET3_IP_ADD_MEMBERSHIP:
            ip_add_membership(msg);
            break;
        case UNET3_IP_DROP_MEMBERSHIP:
            ip_drop_membership(msg);
            break;
        case UNET3_IP_HDRINCL:
            ip_hdrincl(msg);
            break;
        case UNET3_IP_MTU:
            ip_mtu(msg);
            break;
        case UNET3_IP_MULTICAST_TTL:
            ip_multicast_ttl(msg);
            break;
        case UNET3_IP_PKTINFO:
            ip_pktinfo(msg);
            break;
        case UNET3_IP_TOS:
            ip_tos(msg);
            break;
        case UNET3_IP_TTL:
            ip_ttl(msg);
            break;
        case UNET3_IP_MULTICAST_IF:
            ip_multicast_if(msg);
            break;
        case UNET3_IP_MULTICAST_LOOP:
            ip_multicast_loop(msg);
            break;
        case UNET3_IP_DONTFRAGMENT:
            ip_dontfragment(msg);
            break;
        /* not support */
        case UNET3_IP_ADD_SOURCE_MEMBERSHIP:
        case UNET3_IP_BLOCK_SOURCE:
        case UNET3_IP_DROP_SOURCE_MEMBERSHIP:
        case UNET3_IP_FREEBIND:
        case UNET3_IP_MSFILTER:
        case UNET3_IP_MTU_DISCOVER:
        case UNET3_IP_PMTUDISC_WANT:
        case UNET3_IP_PMTUDISC_DONT:
        case UNET3_IP_PMTUDISC_DO:
        case UNET3_IP_PMTUDISC_PROBE:
        case UNET3_IP_MULTICAST_ALL:
        case UNET3_IP_NODEFRAG:
        case UNET3_IP_OPTIONS:
        case UNET3_IP_RECVERR:
        case UNET3_IP_RECVOPTS:
        case UNET3_IP_RECVORIGDSTADDR:
        case UNET3_IP_RECVTOS:
        case UNET3_IP_RECVTTL:
        case UNET3_IP_RETOPTS:
        case UNET3_IP_ROUTER_ALERT:
        case UNET3_IP_TRANSPARENT:
        case UNET3_IP_UNBLOCK_SOURCE:
        default:
            break;
        }

    } else if (opt->level == UNET3_IPPROTO_TCP) {
        switch(opt->optname) {
        case UNET3_TCP_KEEPCNT:
            tcp_keepcnt(msg);
            break;
        case UNET3_TCP_KEEPIDLE:
            tcp_keepidle(msg);
            break;
        case UNET3_TCP_KEEPINTVL:
            tcp_keepintvl(msg);
            break;
        case UNET3_TCP_MAXSEG:
            tcp_maxseg(msg);
            break;
        /* not support */
        case UNET3_TCP_NODELAY:
        case UNET3_TCP_CORK:
        case UNET3_TCP_DEFER_ACCEPT:
        case UNET3_TCP_INFO:
        case UNET3_TCP_LINGER2:
        case UNET3_TCP_QUICKACK:
        case UNET3_TCP_SYNCNT:
        case UNET3_TCP_WINDOW_CLAMP:
        default:
            break;
        }
#ifdef IPV6_SUP
    } else if (opt->level == UNET3_IPPROTO_IPV6) {
        switch(opt->optname) {
        case UNET3_IPV6_ADD_MEMBERSHIP:
            ipv6_add_membership(msg);
            break;
        case UNET3_IPV6_DROP_MEMBERSHIP:
            ipv6_drop_membership(msg);
            break;
        /* not support */
        case UNET3_IPV6_ADDRFORM:
        case UNET3_IPV6_MTU:
        case UNET3_IPV6_MTU_DISCOVER:
        case UNET3_IPV6_MULTICAST_HOPS:
        case UNET3_IPV6_MULTICAST_IF:
        case UNET3_IPV6_MULTICAST_LOOP:
        case UNET3_IPV6_RECVPKTINFO:
        case UNET3_IPV6_PKTINFO:
        case UNET3_IPV6_RTHDR:
        case UNET3_IPV6_AUTHHDR:
        case UNET3_IPV6_DSTOPTS:
        case UNET3_IPV6_HOPOPTS:
        case UNET3_IPV6_FLOWINFO:
        case UNET3_IPV6_HOPLIMIT:
        case UNET3_IPV6_RECVERR:
        case UNET3_IPV6_ROUTER_ALERT:
        case UNET3_IPV6_UNICAST_HOPS:
        case UNET3_IPV6_V6ONLY:
        default:
            break;
        }
#endif
    } else {
        msg->ercd = E_PAR;
    }
    ulc_bsd();
    set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
}
