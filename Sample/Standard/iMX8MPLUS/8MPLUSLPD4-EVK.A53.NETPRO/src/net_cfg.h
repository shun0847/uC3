/***************************************************************************
    MICRO C CUBE / NETWORK
    User configuration header

 **************************************************************************/

#ifndef NETCFG_H
#define NETCFG_H
#ifdef __cplusplus
extern "C" {
#endif

#define CFG_NET_DEV_MAX         (1U)
#define CFG_NET_SOC_MAX         (30U)
#define CFG_NET_TCP_MAX         (15U)
#define CFG_NET_ARP_MAX         (8U)
#define CFG_NET_MGR_MAX         (8U)
#define CFG_NET_IPR_MAX         (2U)
#define CFG_NET_BUF_SZ          (1664U)
#define CFG_NET_BUF_CNT         (64U)
#define CFG_NET_BUF_OFFSET      (56+2)          /* Adjust (&pkt->buf[dev->hhdrofs-2]) to 512-bit alignment. */

#define CFG_PATH_MTU            (1500U)
#define CFG_ARP_RET_CNT         (3U)
#define CFG_ARP_RET_TMO         (1U*1000U)
#define CFG_ARP_CLR_TMO         (20U*60U*1000U)
#define CFG_IP4_TTL             (64U)
#define CFG_IP4_TOS             (0U)
#define CFG_IP4_IPR_TMO         (10U*1000U)
#define CFG_IP4_MCAST_TTL       (DEF_IP4_MCAST_TTL)
#define CFG_IGMP_V1_TMO         (DEF_IGMP_V1_TMO)
#define CFG_IGMP_REP_TMO        (DEF_IGMP_REP_TMO)
#define CFG_TCP_MSS             (CFG_PATH_MTU-40U)
#define CFG_TCP_MSS_IPV6        (CFG_PATH_MTU-60U)
#define CFG_TCP_RTO_INI         (DEF_TCP_RTO_INI)
#define CFG_TCP_RTO_MIN         (DEF_TCP_RTO_MIN)
#define CFG_TCP_RTO_MAX         (DEF_TCP_RTO_MAX)
#define CFG_TCP_SND_WND         1024*16
#define CFG_TCP_RCV_WND         1024*16
#define CFG_TCP_DUP_CNT         (DEF_TCP_DUP_CNT)
#define CFG_TCP_CON_TMO         (75U*1000U)
#define CFG_TCP_SND_TMO         (64U*1000U)
#define CFG_TCP_CLS_TMO         (75U*1000U)
#define CFG_TCP_CLW_TMO         (DEF_TCP_CLW_TMO)
#define CFG_TCP_ACK_TMO         (DEF_TCP_ACK_TMO)
#ifdef KEEPALIVE_SUP
#define CFG_TCP_KPA_CNT         (0U)
#define CFG_TCP_KPA_INT         (1U*1000U)
#define CFG_TCP_KPA_TMO         (7200U*1000U)
#endif
#define CFG_PKT_RCV_QUE         (1U)
#define CFG_TCP_RCV_OSQ_MAX     (6U)
#define CFG_PKT_CTL_FLG         (0x0000U)
#ifdef ACD_SUP
#define CFG_ARP_PRB_WAI         (DEF_ARP_PRB_WAI)
#define CFG_ARP_PRB_NUM         (DEF_ARP_PRB_NUM)
#define CFG_ARP_PRB_MIN         (DEF_ARP_PRB_MIN)
#define CFG_ARP_PRB_MAX         (DEF_ARP_PRB_MAX)
#define CFG_ARP_ANC_WAI         (DEF_ARP_ANC_WAI)
#define CFG_ARP_ANC_NUM         (DEF_ARP_ANC_NUM)
#define CFG_ARP_ANC_INT         (DEF_ARP_ANC_INT)
#endif
#define CFG_NET_TSK_PRI         (4U)
#if (_kernel_SIZE_SIZE==8)
#define CFG_NET_TSK_SIZ         (1024*3)
#else
#define CFG_NET_TSK_SIZ         1024
#endif
#define CFG_STS_UPD_RES         (2000U)
#define CFG_RTE_NUM             (0U)
#define CFG_NAT_NUM             (0U)
#define CFG_FLT_NUM             (0U)
#define CFG_NAT_TCP_TTL         (0U)
#define CFG_NAT_UDP_TTL         (0U)
#define CFG_NAT_ICMP_TTL        (0U)
#ifdef IGMPv3_SUP
#define CFG_IGMP_V3_ENABLE      (0U)
#endif

#ifdef __cplusplus
}
#endif
#endif /* NETCFG_H */
