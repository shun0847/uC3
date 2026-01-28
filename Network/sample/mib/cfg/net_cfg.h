/***************************************************************************
    MICRO C CUBE / NETWORK
    User configuration header
    Copyright (c)  2014, eForce Co., Ltd. All rights reserved.

    Version Information  2014.03.31  Created.
 **************************************************************************/

#ifndef NETCFG_H
#define NETCFG_H
#ifdef __cplusplus
extern "C" {
#endif

#define CFG_NET_DEV_MAX         1
#define CFG_NET_SOC_MAX         20
#define CFG_NET_TCP_MAX         10
#define CFG_NET_ARP_MAX         8
#define CFG_NET_MGR_MAX         8
#define CFG_NET_IPR_MAX         2
#if (_kernel_SIZE_SIZE==8)
#define CFG_NET_BUF_SZ          1600
#else
#define CFG_NET_BUF_SZ          1576
#endif
#define CFG_NET_BUF_CNT         64
#define CFG_NET_BUF_OFFSET      2
#define CFG_PATH_MTU            1500
#define CFG_ARP_RET_CNT         3
#define CFG_ARP_RET_TMO         1*1000
#define CFG_ARP_CLR_TMO         20*60*1000
#define CFG_IP4_TTL             64
#define CFG_IP4_TOS             0
#define CFG_IP4_IPR_TMO         10*1000
#define CFG_IP4_MCAST_TTL       1
#define CFG_IGMP_V1_TMO         400*1000
#define CFG_IGMP_REP_TMO        10*1000
#define CFG_TCP_MSS             (CFG_PATH_MTU-40)
#define CFG_TCP_MSS_IPV6        (CFG_PATH_MTU-60)
#define CFG_TCP_RTO_INI         3*1000
#define CFG_TCP_RTO_MIN         500
#define CFG_TCP_RTO_MAX         60*1000     /* 60 sec   */
#define CFG_TCP_SND_WND         1024*16
#define CFG_TCP_RCV_WND         1024*16
#define CFG_TCP_DUP_CNT         4
#define CFG_TCP_CON_TMO         75*1000     /* 75 secs */
#define CFG_TCP_SND_TMO         64*1000     /* 64 secs */
#define CFG_TCP_CLS_TMO         75*1000     /* 75 secs */
#define CFG_TCP_CLW_TMO         20*1000     /* 20 secs */
#define CFG_TCP_ACK_TMO         200
#ifdef KEEPALIVE_SUP
#define CFG_TCP_KPA_CNT         0
#define CFG_TCP_KPA_INT         1*1000
#define CFG_TCP_KPA_TMO         7200*1000
#endif
#define CFG_PKT_RCV_QUE         1
#define CFG_TCP_RCV_OSQ_MAX     6
#define CFG_PKT_CTL_FLG         0x0000
#ifdef ACD_SUP
#define CFG_ARP_PRB_WAI         1*1000
#define CFG_ARP_PRB_NUM         3
#define CFG_ARP_PRB_MIN         1*1000
#define CFG_ARP_PRB_MAX         2*1000
#define CFG_ARP_ANC_WAI         2*1000
#define CFG_ARP_ANC_NUM         2
#define CFG_ARP_ANC_INT         2*1000
#endif
#define CFG_NET_TSK_PRI         4
#define CFG_NET_TSK_SIZ         1024
#define CFG_STS_UPD_RES         2000
#ifdef IGMPv3_SUP
#define CFG_IGMP_V3_ENABLE      1
#endif
#ifdef IPV6_SUP
#define CFG_NET6_BUF_SZ         DEF_NET6_BUF_SZ
#define CFG_NEIGH_CACHE         5
#define CFG_DST_CACHE           5
#define CFG_RTR_LST             2
#define CFG_PRFX_LST            5
#define CFG_PMTU_CACHE          5
#endif

/* uNet3/BSD wrapper settings */
#define CFG_BSD_TCP_MAX         4
#define CFG_BSD_TCP_TOP         1
#define CFG_BSD_TCP_SBUFSZ      1024
#define CFG_BSD_TCP_RBUFSZ      1024
#define CFG_BSD_UDP_MAX         4
#define CFG_BSD_UDP_TOP         5
#define CFG_TASK_MAX            16

#ifdef __cplusplus
}
#endif
#endif /* NETCFG_H */
