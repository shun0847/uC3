/*
    SNMP
    Macro definition
    Copyright (c) 2014-2016, eForce Co., Ltd. All rights reserved.
    
    2014-03-06 Created
    2014-05-15 User macros was closed in parentheses
    2014-06-17 Number of TCP buffer was corrected
    2016-01-06 Multiple network devices were supported
    2017-04-20 Support OID type for private MIB data
*/

#ifndef SNMP_MAC_H
#define SNMP_MAC_H

/* Error directive */
#if (CFG_SNMP_NET_DEV_CNT > 16)
#error Error CFG_SNMP_NET_DEV_CNT (snmp_cfg.h)
#endif
#if (CFG_SNMP_NET_USE_CNT > 16)
#error Error CFG_SNMP_NET_USE_CNT (snmp_cfg.h)
#endif
#if (CFG_SNMP_VEN_TRP_CNT > 32)
#error Error CFG_SNMP_VEN_TRP_CNT (snmp_cfg.h)
#endif

/* Buffer size */
#define BUF_UW_LEN(x)      ((x + (sizeof(UW) - 1)) / sizeof(UW))

#define SND_MSG_BUF_LEN    (BUF_UW_LEN(CFG_SNMP_SND_MSG_LEN))
#define RCV_MSG_BUF_LEN    (BUF_UW_LEN(CFG_SNMP_RCV_MSG_LEN))
#define SND_MSG_LEN        (BUF_UW_LEN(sizeof(T_SNMP_MSG_BUF)))
#define RCV_MSG_LEN        (BUF_UW_LEN(sizeof(T_SNMP_MSG_V2)))
#if ((CFG_SNMP_MAX_TRP_CNT + CFG_SNMP_VEN_TRP_CNT) > 0)
#define TRP_MSG_LEN        (BUF_UW_LEN(sizeof(T_SNMP_MSG_BUF)))
#else
#define TRP_MSG_LEN        (0)
#endif
#define MSG_VAR_LEN        (BUF_UW_LEN(sizeof(T_SNMP_MSG_VAR)))
#define MIB_NOD            (BUF_UW_LEN(sizeof(T_SNMP_MIB_NOD)))
#define MSG_OID_LEN        (CFG_SNMP_MAX_MIB_DEP)
#define VAR_DAT_LEN        (1 + BUF_UW_LEN(CFG_SNMP_MIB_DAT_LEN))
#define NOD_DAT_LEN        (1)
#define TRP_CMD_LEN        (BUF_UW_LEN(sizeof(T_SNMP_TRP_CMD)))
#define PHY_DAT_LEN        (BUF_UW_LEN(MAC_ADR_LEN))
#define UDP_SOC_LEN        (BUF_UW_LEN(sizeof(T_SNMP_UDP_SOC)))
#define TRP_STS_LEN        (BUF_UW_LEN(sizeof(UH)))

#define OID_STR_LEN        (BUF_UW_LEN(6 * ((CFG_SNMP_MAX_MIB_DEP) + 1)))
#if ((CFG_SNMP_MAX_TRP_CNT + CFG_SNMP_VEN_TRP_CNT) > 0)
#define MSG_BUF_LEN        (((SND_MSG_BUF_LEN) * 2) + (RCV_MSG_BUF_LEN))
#else
#define MSG_BUF_LEN        ((SND_MSG_BUF_LEN) + (RCV_MSG_BUF_LEN))
#endif
#define MSG_LEN_X2         ((SND_MSG_LEN) + (RCV_MSG_LEN))
#if ((CFG_SNMP_MAX_TRP_CNT + CFG_SNMP_VEN_TRP_CNT) > 0)
#define MSG_VAR_LEN_X      (MSG_VAR_LEN * (CFG_SNMP_MSG_VAR_CNT) * 3)
#else
#define MSG_VAR_LEN_X      (MSG_VAR_LEN * (CFG_SNMP_MSG_VAR_CNT) * 2)
#endif
#define MIB_NOD_LEN        (MIB_NOD * (CFG_SNMP_MIB_NOD_CNT))
#if ((CFG_SNMP_MAX_TRP_CNT + CFG_SNMP_VEN_TRP_CNT) > 0)
#define MSG_OID_LEN_X      (MSG_OID_LEN * ((CFG_SNMP_MSG_VAR_CNT) * 5 + 1))
#else
#define MSG_OID_LEN_X      (MSG_OID_LEN * (CFG_SNMP_MSG_VAR_CNT) * 3)
#endif
#define VAR_DAT_LEN_X      (VAR_DAT_LEN * (CFG_SNMP_MSG_VAR_CNT))
#define NOD_DAT_LEN_X      (NOD_DAT_LEN * (CFG_SNMP_MSG_VAR_CNT))
#define TRP_CMD_LEN_X      (TRP_CMD_LEN * (CFG_SNMP_MAX_TRP_CNT + CFG_SNMP_VEN_TRP_CNT))
#if ((CFG_SNMP_MAX_TRP_CNT + CFG_SNMP_VEN_TRP_CNT) > 0)
#define OID_STR_LEN_X      (OID_STR_LEN * 2)
#else
#define OID_STR_LEN_X      (OID_STR_LEN)
#endif
#define UDP_SOC_LEN_X2     (UDP_SOC_LEN * CFG_SNMP_NET_USE_CNT * 2)
#define TRP_STS_LEN_X      (TRP_STS_LEN * CFG_SNMP_NET_DEV_CNT)

#define DEC_OID_LEN        (BUF_UW_LEN(sizeof(T_SNMP_BER_OID)))
#define DEC_OID_BUF_LEN    (BUF_UW_LEN(sizeof(UW) * (CFG_SNMP_MAX_OID_DEP)))
#define DEC_OID_STR_BUF_LEN (BUF_UW_LEN(6 * (CFG_SNMP_MAX_OID_DEP)))

#define SNMP_MAC_BUF_LEN   (MSG_BUF_LEN + MSG_LEN_X2 + TRP_MSG_LEN + MSG_VAR_LEN_X \
                            + MIB_NOD_LEN + MSG_OID_LEN_X + VAR_DAT_LEN_X + NOD_DAT_LEN_X \
                            + TRP_CMD_LEN_X + OID_STR_LEN_X + PHY_DAT_LEN + UDP_SOC_LEN_X2 \
                            + TRP_STS_LEN_X + DEC_OID_LEN + DEC_OID_BUF_LEN + DEC_OID_STR_BUF_LEN + 1)

/* TCP data buffer size */
#if ((CFG_SNMP_MIB2_IP_ENA) == 1)
#define TBL_ADR_CNT    (5 * (CFG_SNMP_NET_DEV_CNT))
#else
#define TBL_ADR_CNT    (0)
#endif
#if ((CFG_SNMP_MIB2_AT_ENA) == 1)
#define TBL_AT_CNT     (3 * (CFG_SNMP_MAX_ARP_CNT))
#else
#define TBL_AT_CNT     (0)
#endif
#if ((CFG_SNMP_MIB2_IP_ENA) == 1)
#define TBL_ARP_CNT    (4 * (CFG_SNMP_MAX_ARP_CNT))
#else
#define TBL_ARP_CNT    (0)
#endif
#if ((CFG_SNMP_MIB2_TCP_ENA) == 0) && ((CFG_SNMP_MIB2_UDP_ENA) == 0)
#define TBL_SOC_CNT    (0)
#elif ((CFG_SNMP_MIB2_TCP_ENA) == 1) && ((CFG_SNMP_MIB2_UDP_ENA) == 0)
#define TBL_SOC_CNT    (5 * (CFG_SNMP_MAX_TCP_CNT))
#else
#define TBL_SOC_CNT    (5 * (CFG_SNMP_MAX_SOC_CNT))
#endif
#if ((CFG_SNMP_MIB2_IP_ENA) == 1)
#define NET_ADR_LEN    (BUF_UW_LEN(sizeof(T_NET_ADR)))
#else
#define NET_ADR_LEN    (0)
#endif

#define SNMP_MAC_TCP_DAT_CNT    (TBL_AT_CNT + TBL_ADR_CNT + TBL_ARP_CNT + TBL_SOC_CNT)
#if ((CFG_SNMP_MIB2_IP_ENA) == 1 || (CFG_SNMP_MIB2_AT_ENA) == 1)
#define SNMP_MAC_ARP_DAT_CNT    (CFG_SNMP_MAX_ARP_CNT)    /* Physical address data */
#else
#define SNMP_MAC_ARP_DAT_CNT    (0)
#endif

#if ((CFG_SNMP_MIB2_TCP_ENA) == 0) && ((CFG_SNMP_MIB2_UDP_ENA) == 0)
#define OID_TCP_CNT             (0)
#else
#define OID_TCP_CNT             (5 * (CFG_SNMP_MAX_SOC_CNT))
#endif

#define SNMP_TCP_LEN        (BUF_UW_LEN(sizeof(T_SNMP_MIB_TCP_DAT)))
#define SNMP_ARP_LEN        (BUF_UW_LEN(sizeof(T_SNMP_MIB_ARP_DAT)))
#define OID_DAT_LEN         (BUF_UW_LEN(sizeof(T_SNMP_OID_DAT)))
#define ETH_STS_LEN         (BUF_UW_LEN(sizeof(UH)))

#define SNMP_TCP_LEN_X      (SNMP_TCP_LEN * SNMP_MAC_TCP_DAT_CNT)
#define SNMP_ARP_LEN_X      (SNMP_ARP_LEN * SNMP_MAC_ARP_DAT_CNT)
#define OID_DAT_LEN_X       (OID_DAT_LEN * (OID_TCP_CNT + TBL_ARP_CNT + TBL_AT_CNT))
#define TCP_OID_STR_LEN_X   ((OID_STR_LEN) * (OID_TCP_CNT + TBL_ARP_CNT + TBL_AT_CNT))
#define NET_ADR_LEN_X       (NET_ADR_LEN * (CFG_SNMP_NET_DEV_CNT))
#define ETH_STS_LEN_X       (ETH_STS_LEN * CFG_SNMP_NET_DEV_CNT)

#define SNMP_MAC_TCP_BUF_LEN    (SNMP_TCP_LEN_X + SNMP_ARP_LEN_X + OID_DAT_LEN_X \
                                 + TCP_OID_STR_LEN_X + NET_ADR_LEN_X + ETH_STS_LEN_X + 1)

#endif

