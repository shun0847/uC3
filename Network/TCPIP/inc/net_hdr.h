/***************************************************************************
    MICRO C CUBE / COMPACT, NETWORK
    Protocol headers, internal structure & macro definitions
    Copyright (c)  2008-2024, eForce Co., Ltd. All rights reserved.

    Version Information  2008.11.19: Created
                         2010.01.18: 'hhdrsz' field in T_NET_DEV, PPP_HDR_SZ
                         2010.03.24: SOC_FLG_PRT check
                         2010.08.10: Updated for broadcast IP address check
                         2010.08.10: Added net_rand() & net_rand_seed()
                         2010.08.17: Support SOC_RCV_PKT_INF socket option
                         2010.10.02: Updated for IPv6 support
                         2010.11.10: Updated for any network interface
                         2011.01.31: Added laddr6[4] as member of t_net_soc
                         2011.03.07: Updated for received packet queue
                         2011.04.04: Added definition of TCP/MSS for IPv6
                         2011.05.20: Set up networkbuffer offset
                         2011.11.14: Added ACD parameter
                         2011.11.14: Implemented Address Conflict Detection
                         2012.05.21: Implemented Keep-Alive
                         2012.06.06: Implemented port unreachable with ICMPv4
                         2012.10.02  Modify to avoid use of string libraries.
                         2013.04.10  H/W OS supported version
                         2013.07.23  Implemented ready I/O event notification.
                         2013.08.06: Implememted multicast per socket
                         2013.09.25: Implememted local loopback
                         2013.12.06  Change to external function to allocate buffer
                         2014.03.06: SNMP option was added
                         2014.03.31  Moved internal definitions to net_def.h
                         2014.04.15: Add member of T_NET_DEV for device option
                         2014.08.20: Add net_sts_ext function.
                         2014.10.03: Add MAC/PHY common status definitions
                         2014.11.04: Add get_net_sec() function
                         2014.11.26: Add ip fowarding function
                         2014.12.09: Moved definition of internal data to net_def.h
                         2015.05.11: Implemented arp_req() and arp_set()
                         2015.12.03: Add SOC_UDP_RQSZ option
                         2015.12.03: Add SOC_SRC_IP_RESET option
                         2015.12.14: Add socket type id and the socket ID replaced SID types
                         2015.12.17: Add parameter definitions of PHY mode settings.
                         2016.01.07: Add MLDv1 functions
                         2016.01.20: Add parameter definitions of MAC filter setting
                         2016.02.05: Implemented arp_clr()
                         2016.02.05: Add SOC_UDP_EARLY_TX option
                         2016.02.08: Update for raw socket
                         2016.02.18  Change the second parameter type of net_buf_get
                         2016.03.09  Devide HW_CS flag into ICMP and TCP/UDP
                         2016.03.09  Add definition of device options
                         2016.03.19  IGMP Version 3 support
                         2016.04.28  Add include file for HWOS
                         2016.05.12  Add definition of INADDR_BROADCAST
                         2016.06.08  Add SOC_RCV_PKT_SIZE option
                         2016.06.09  Add SOC_MSG_PEEK option
                         2016.06.24  Add link of peer option
                         2016.06.16  Add SOC_PRT_LOCAL_DUP option
                         2017.01.17: Clarified the type for constant
                         2017.05.02: Add soc_ext() API
                         2018.02.14: Add member of T_NODE as node name
                         2018.08.23: Support set or reset the don't fragment flag in IP header
                         2018.08.23: Support POSIX IP_MULTICAST_LOOP feature
                         2018.12.14: Update arp_set() and add arp_del()
                         2020.12.28: Add the definition of REF_ETH_FDB_TBL option.
                         2021.01.29: Add ARP_CUSTOM_GARP
                         2021.03.17: Add ARP_API_SUP option support
                         2022.04.01: Add CFG_VLAN_ID in net_dev_ctl(), net_dev_ref() for VLAN
                         2022.04.01: Add NET_PING_IGNORE in net_cfg(), net_ref() for setting ignore ping
                         2024.03.15: Moved device definition structure from net_def.h
                         2024.03.18: Add NET_IF_INACT for interface inactivate setting.
 **************************************************************************/

#ifndef NETHDR_H
#define NETHDR_H
#ifdef __cplusplus
extern "C" {
#endif

#include "net_sup.h"

/***************************************************************************
 *  Protocol Header Definition
 */

/***************************************************************************
    Type definition
***************************************************************************/
typedef UH SID;             /* Socket type ID */

/***************************************************************************
    PPP
***************************************************************************/
#define PPP_HDR_SZ      2   /* for network buffer header alignment */

/***************************************************************************
    Ethernet
***************************************************************************/

#define ETH_TYPE_IP4   0x0800U  /* Ethernet Type of IP Protocol  */
#define ETH_TYPE_ARP   0x0806U  /* Ethernet Type of ARP Protocol */
#define ETH_TYPE_IP6   0x86DDU  /* Ethernet Type of IP6 Protocol */
#define ETH_TYPE_ALL   0x0000U  /* Ethernet all type             */

typedef struct t_eth_hdr {
    UB da[6];
    UB sa[6];
    UH type;
}T_ETH_HDR;
#define ETH_HDR_SZ      14U

/**************************************************************************
    ARP
**************************************************************************/

#define ARP_TYPE_ETH    0x0001U  /* Hardware type: Ethernet  */
#define ARP_PROTO_IP    0x0800U  /* Protocol type: IP        */
#define ARP_HA_LEN      0x06U    /* Hardware Address length  */
#define ARP_PA_LEN      0x04U    /* Protocol Address length  */
#define ARP_OPC_REQ     0x0001U  /* ARP Request  : 1         */
#define ARP_OPC_REP     0x0002U  /* ARP Reply    : 2         */
#define ARP_OPC_RREQ    0x0003U  /* RARP Request : 3         */
#define ARP_OPC_RREP    0x0004U  /* RARP Reply   : 4         */

typedef struct t_arp_hdr {
    UH hw;              /* Hardware Address Space  */
    UH proto;           /* Protocol Address Space  */
    UB ha_len;          /* Hardware Address Length */
    UB pa_len;          /* Protocol Address Length */
    UH op;              /* Opcode: Request | Reply */
    UB sha[6];          /* Sender Hardware Address */
    UB spa[4];          /* Sender Protocol Address */
    UB tha[6];          /* Target Hardware Address */
    UB tpa[4];          /* Target Protocol Address */
}T_ARP_HDR;
#define ARP_HDR_SZ      28U

/**************************************************************************
    IP4
**************************************************************************/

#define IP_HDR_VER4     0x04U
#define IP_VER          0x0FU
#define IP_OFFSET       0x1FFFU
#define IP_FLG_MF       0x01U
#define IP_FLG_DF       0x02U

#define IP_PROTO_ICMP        1U       /* ICMP Protocol        */
#define IP_PROTO_IGMP        2U       /* IGMP Protocol        */
#define IP_PROTO_TCP         6U       /* TCP  Protocol        */
#define IP_PROTO_UDP        17U       /* UDP  Protocol        */
#define IP_PROTO_UDPL      136U       /* UDP-Lite Protocol    */
#define IP_PROTO_RAW       255U       /* RAW socket           */

typedef struct t_ip4_hdr {
    UB   ver;                   /* Version, Internal Header Length */
    UB   tos;                   /* Type of Service */
    UH   tl;                    /* Total Length */
    UH   id;                    /* Identification */
    UH   fo;                    /* Flags, Fragment Offset */
    UB   ttl;                   /* Time to Live */
    UB   prot;                  /* Protocol Number */
    UH   hc;                    /* Header Checksum */
    UW   sa;                    /* Source IP Address */
    UW   da;                    /* Destination IP Address */
}T_IP4_HDR;
#define IP4_HDR_SZ      20U

typedef struct t_ip4_pseudo {
    UW sa;
    UW da;
    UH proto;
    UH len;
}T_IP4_PSEUDO;

/**************************************************************************
    UDP
**************************************************************************/

typedef struct t_udp_hdr {
    UH sp;                  /* Source Port */
    UH dp;                  /* Destination Port */
    UH len;                 /* Data Length */
    UH cs;                  /* Checksum */
} T_UDP_HDR;
#define UDP_HDR_SZ      8U

/**************************************************************************
    TCP
**************************************************************************/

#define TCP_UNUSED          0x00U
#define TCP_CLOSED          0x01U
#define TCP_LISTEN          0x02U
#define TCP_SYN_SENT        0x03U
#define TCP_SYN_RECEIVED    0x04U
#define TCP_ESTABLISHED     0x05U
#define TCP_FIN_WAIT1       0x06U
#define TCP_FIN_WAIT2       0x07U
#define TCP_CLOSE_WAIT      0x08U
#define TCP_CLOSING         0x09U
#define TCP_LAST_ACK        0x0AU
#define TCP_TIME_WAIT       0x0BU

#define TCP_FLG_FIN         0x0001U
#define TCP_FLG_SYN         0x0002U
#define TCP_FLG_RST         0x0004U
#define TCP_FLG_PSH         0x0008U
#define TCP_FLG_ACK         0x0010U
#define TCP_FLG_URG         0x0020U

typedef struct t_tcp_hdr {
    UH  sp;     /* Source port */
    UH  dp;     /* Destination port */
    UW  seq;    /* Sequence value */
    UW  ack;    /* Acknowledgment value */
    UH  flag;   /* Data offset 3 and TCP Flags */
    UH  win;    /* TCP window value */
    UH  cs;     /* TCP Checksum */
    UH  up;     /* Urgent pointer */
}T_TCP_HDR;
#define TCP_HDR_SZ      20U

/**************************************************************************
    ICMP
**************************************************************************/

/* ICMP Type :- RFC 792 */
#define ICMP_ECHO_REPLY     0U      /* Echo Reply               */
#define ICMP_DST_UNREACH    3U      /* Destination Unreachable  */
#define ICMP_SRC_QUENC      4U      /* Source Quench            */
#define ICMP_REDIRECT       5U      /* Redirect                 */
#define ICMP_ECHO_REQUEST   8U      /* Echo Request             */
#define ICMP_TIME_EXCEED    11U     /* Time Exceeded            */
#define ICMP_PRM_PROB       12U     /* Parameter Problem        */
#define ICMP_TIMES_REQUEST  13U     /* Timestamp Request        */
#define ICMP_TIMES_REPLY    14U     /* Timestamp Reply          */
#define ICMP_ADDR_REQUEST   17U     /* AddressMask Request      */
#define ICMP_ADDR_REPLY     18U     /* AddressMask Reply        */

typedef struct t_icmp_hdr {
    UB type;                /* Type */
    UB code;                /* Code */
    UH cs;                  /* Checksum */
    UH id;                  /* Identifier */
    UH seq;                 /* Sequence Number */
}T_ICMP_HDR;
#define ICMP_HDR_SZ     8U

/**************************************************************************
    IGMP
**************************************************************************/

#define IGMP_QUERY       0x11U   /* Query            */
#define IGMP_REPORT_V1   0x12U   /* Version1 Report  */
#define IGMP_REPORT      0x16U   /* Version2 Report  */
#define IGMP_LEAVE       0x17U   /* Leave            */
#define IGMP_REPORT_V3   0x22U   /* Version3 Report  */

typedef struct t_igmp_hdr {
    UB type;    /* Type          */
    UB mrt;     /* Max Resp Time */
    UH cs;      /* Checksum      */
    UW ga;      /* Group Address */
#ifdef IGMPv3_SUP
    UB qrv;     /* Querier's Robustness Variable */
    UB qqic;    /* Querier's Query Interval Code */
    UH num;     /* Number of Sources             */
    UW sa;      /* Source Address                */
#endif
} T_IGMP_HDR;
#define IGMP_HDR_SZ     8U

#ifdef IGMPv3_SUP
typedef struct t_igmpv3_gr {
    UB rtype;   /* Record Type                   */
    UB auxlen;  /* Auxiliary Data Length         */
    UH num;     /* Number of Sources             */
    UW ga;      /* Group Address                 */
    UW sa;      /* Source Address(s)             */
} T_IGMPV3_GR;

typedef struct t_igmpv3_rep {
    UB type;    /* Type                          */
    UB rsv1;    /* Reserve                       */
    UH cs;      /* Checksum                      */
    UH rsv2;    /* Reserve                       */
    UH num;     /* Number of Group Address       */
    T_IGMPV3_GR gr; /* Group Record(s)           */
} T_IGMPV3_REP;

#define IGMP_FLT_MODE_INC   0x0U     /* INCLUDE mode */
#define IGMP_FLT_MODE_EXC   0x1U     /* EXCLUDE mode */
#endif

/***************************************************************************
    Network Buffer
***************************************************************************/

#define IP_RCV_BCAST    0x0001U
#define IP_RCV_MCAST    0x0002U
#define TCP_DAT_BUF     0x0004U
#define LOCAL_LOOPBK    0x0008U
#define MCAST_LOOPBK    0x1000U
#define NET_IF_INACT    0x2000U

#define HW_CS_RX_IPH4   0x0010U
#define HW_CS_RX_TCPUDP 0x0020U
#define HW_CS_RX_ICMP   0x0040U
#define HW_CS_RX_DATA   (HW_CS_RX_TCPUDP | HW_CS_RX_ICMP)
#define HW_CS_TX_IPH4   0x0080U
#define HW_CS_TX_TCPUDP 0x0100U
#define HW_CS_TX_ICMP   0x0200U
#define HW_CS_TX_DATA   (HW_CS_TX_TCPUDP | HW_CS_TX_ICMP)

#define HW_CS_IPH4_ERR  0x0400U
#define HW_CS_DATA_ERR  0x0800U

#define PKT_FLG_BCAST   IP_RCV_BCAST
#define PKT_FLG_MCAST   IP_RCV_MCAST
#define PKT_FLG_TCP     TCP_DAT_BUF

typedef struct t_net_buf {
    UW  *next;
    ID  mpfid;
    struct t_net  *net;
    struct t_net_dev  *dev;
    struct t_net_soc  *soc;
    ER  ercd;       /* Socket Error                 */
    UH  flg;        /* Broadcast/Multicast          */
    UH  seq;        /* IP Fragment Sequence         */
    UH  hdr_len;    /* *hdr length                  */
    UH  dat_len;    /* *dat length                  */
    UB  *hdr;       /* 2byte Aligned                */
    UB  *dat;       /* 4byte Aligned                */
    UB  buf[2];     /* Packet Data                  */
}T_NET_BUF;


/***************************************************************************
    Network Device
***************************************************************************/

/* Device Type */
#define NET_DEV_TYPE_LOOP   0U   /* LoopBack Device  */
#define NET_DEV_TYPE_ETH    1U   /* Ethernet Device  */
#define NET_DEV_TYPE_PPP    2U   /* PPP              */
#define NET_DEV_TYPE_PPPOE  3U   /* PPPoE            */
#define NET_DEV_TYPE_VIR    4U   /* Virtual Device   */

/* Device Status */
#define NET_DEV_STS_NON     0U    /* Device not exists       */
#define NET_DEV_STS_INI     1U    /* Device Initializized    */
#define NET_DEV_STS_DIS     2U    /* Device Disabled         */

/* Device Flag */
#define NET_DEV_FLG_PROMISC 1U    /* Device Promiscous        */
#define NET_DEV_FLG_BCAST   2U    /* Device support Broadcast */
#define NET_DEV_FLG_MCAST   4U    /* Device support Multicast */

/* Device Configuration */
enum net_dev_ctl {
NET_DEV_CFG_MAC,                 /* MAC Address configuration */
NET_DEV_CFG_MTU,                 /* MTU                       */
NET_DEV_CFG_PROMISC,             /* Enable/Disable Promiscous */
NET_DEV_CFG_BCAST,               /* Enable/Disable MultiCast  */
NET_DEV_CFG_MCAST                /* Enable/Disable BroadCast  */
};

#define DEV_STR_LEN     8       /* Device Name Length */
#define ETH_ADR_LEN     6

typedef struct t_net_dev {
    UB name[DEV_STR_LEN];       /* Device Name                  */
    UH num;                     /* Device Number                */
    UH type;                    /* Device Type                  */
    UH sts;                     /* Device Status                */
    UH flg;                     /* Dummy                        */
    ER (*ini)(UH);              /* Initialize the device        */
    ER (*cls)(UH);              /* Uninitialize the device      */
    ER (*ctl)(UH,UH,VP);        /* Configure the device         */
    ER (*ref)(UH,UH,VP);        /* Read status of the device    */
    ER (*out)(UH,T_NET_BUF*);   /* Write frame to the device    */
    void (*cbk)(UH,UH,VP);      /* Device event notifier        */
    UW *tag;                    /* Device specific              */
    union  {                    /* Address                      */
    struct {
    UB mac[ETH_ADR_LEN];
    }eth;
    }cfg;
    UH  hhdrsz;                 /* Device header length         */
    UH  hhdrofs;                /* Device header offset         */
    VP  opt;                    /* Misc                         */
}T_NET_DEV;

/* Callback events */
#define EV_CBK_DEV_LINK         0x0001U     /* Device link change event */
#define EV_CBK_DEV_CHG_IP       0x0002U     /* Device IP change event   */
#define EV_CBK_DEV_INIT         0x0004U     /* Device Init              */
#define EV_CBK_ARP_CRE          0x0010U     /* ARP create               */
#define EV_CBK_ARP_DEL          0x0020U     /* ARP delete               */
#define EV_CBK_ARP_CHG_IP       0x0040U     /* ARP IP dev_num changed   */
#define EV_CBK_ARP_CHG_MAC      0x0080U     /* ARP MAC changed          */
#define EV_CBK_SOC_CRE          0x0100U     /* Socket create            */
#define EV_CBK_SOC_DEL          0x0200U     /* Socket delete            */
#define EV_CBK_SOC_CHG          0x0400U     /* Socket status changed    */
#define EV_CBK_SOC_CHG_TCP      0x0800U     /* Socket TCP stat changed  */

#define EV_CBK_DEV_MSK          0x000FU     /* Device mask              */
#define EV_CBK_ARP_MSK          0x00F0U     /* ARP mask                 */
#define EV_CBK_SOC_MSK          0x0F00U     /* Socket mask              */

#define PHY_STS_LINK_DOWN       0x0000U     /* PHY media link down      */
#define PHY_STS_10HD            0x0100U     /* PHY 10M/Half-Duplex      */
#define PHY_STS_10FD            0x0200U     /* PHY 10M/Full-Duplex      */
#define PHY_STS_100HD           0x0400U     /* PHY 100M/Half-Duplex     */
#define PHY_STS_100FD           0x0800U     /* PHY 100M/Full-Duplex     */
#define PHY_STS_1000HD          0x1000U     /* PHY 1000M/Half-Duplex    */
#define PHY_STS_1000FD          0x2000U     /* PHY 1000M/Full-Duplex    */
#define PHY_STS_MSK             0xff00U     /* PHY mask */
#define ETH_STS_INV             0x0000U     /* Invalid */
#define ETH_STS_INI             0x0001U     /* Initialized */
#define ETH_STS_RST             0x0010U     /* Reset */
#define ETH_STS_ERR             0x0080U     /* Error */
#define ETH_STS_MSK             0x00ffU     /* Mask */


/*************************************************************************
 * Reference/Configuration options definitions:
 ************************************************************************/
/* Reference options */
#define REF_ETH_LINK_STS    0x0001U     /* Link status */
#define REF_ETH_UPD_STS     0x0002U     /* Update status */
#define REF_ETH_FDB_TBL     0x0003U     /* Address Fowarding Table        */

/* Reference/Configuration options */
#define CFG_PHY_MODE        0x0010U     /* Configure PHY mode */
#define CFG_PHY_MODE_EXT    0x0020U     /* Configure PHY mode (Extension) */
#define CFG_MC4_FIL_SET     0x0030U     /* Set m/cast filter with IPv4    */
#define CFG_MC4_FIL_CLR     0x0040U     /* Clear m/cast filter with IPv4  */
#define CFG_MC6_FIL_SET     0x0050U     /* Set m/cast filter with IPv6    */
#define CFG_MC6_FIL_CLR     0x0060U     /* Clear m/cast filter with IPv6  */
#define CFG_MAC_FIL_SET     0x0070U     /* Set m/cast address filter      */
#define CFG_MAC_FIL_CLR     0x0080U     /* Clear m/cast address filter    */
#define CFG_MCRX_MODE       0x0090U     /* Configure m/cast reception mode*/
#define CFG_RX_FRAME_CBK    0x00A0U     /* Configure reception callback   */
#define CFG_TX_FRAME_CBK    0x00B0U     /* Configure transmission callback*/
#define CFG_VLAN_ID         0x0100U     /* Configure VLAN ID */


/* CFG_PHY_MODE option values */
#define PM_AUTO             0U          /* Auto select mode */
#define PM_10M_HALF         1U          /* 10M Half Duplex manual mode */
#define PM_10M_FULL         7U          /* 10M Full Duplex manual mode */
#define PM_10M_AUTO         2U          /* 10M Full/Half (Duplex auto select mode) */
#define PM_100M_HALF        3U          /* 100M Half Duplex manual mode */
#define PM_100M_FULL        8U          /* 100M Full Duplex manual mode */
#define PM_100M_AUTO        4U          /* 100M Full/Half (Duplex auto select mode) */
#define PM_1000M_HALF       5U          /* 1000M Half Duplex manual mode */
#define PM_1000M_FULL       9U          /* 1000M Full Duplex manual mode */
#define PM_1000M_AUTO       6U          /* 1000M Full/Half (Duplex auto select mode) */

/* CFG_PHY_MODE_EXT option values */
#define PME_SUP_ANE         0x0001U     /* Support Auto negotiation */
#define PME_SUP_FD          0x0002U     /* Support Full duplex */
#define PME_SUP_10M         0x0010U     /* Support 10M Speed */
#define PME_SUP_100M        0x0020U     /* Support 100M Speed */
#define PME_SUP_1000M       0x0040U     /* Support 1000M Speed */
#define PME_CFG_NSET        0x8000U     /* Configuration is not set */
#define PME_SUP_MASK    (PME_SUP_ANE | PME_SUP_FD | PME_SUP_10M | PME_SUP_100M | PME_SUP_1000M)

/* CFG_MCRX_MODE option values */
#define MC_RX_ALLOW         0U           /* Receive all of multicast address      */
#define MC_RX_DENY          1U           /* Does not recevie multicast address    */
#define MC_RX_FILTER        2U           /* Receive multicast address with filter */


#define ARP_RESOLVE_IP         0U
#define ARP_PROBE_IP           1U
#define ARP_ANNOUNCE_IP        2U
#define ARP_GRATUITOUS         3U
#define ARP_CUSTOM_GARP        4U        /* Custom ARP to make THA its own address */

#ifdef ACD_SUP
typedef struct t_net_acd {
    UW ip;
    UB mac[6];
}T_NET_ACD;

typedef ER (*ACD_HND)(T_NET_ACD*);
#endif /* ACD_SUP */

typedef struct t_net_adr {
    UB ver;         /* IP4/IPv6         */
    UB mode;        /* STATIC or DHCP   */
    UW ipaddr;      /* Default IP Addr  */
    UW gateway;     /* Default Gateway  */
    UW mask;        /* Subnet Mask      */
}T_NET_ADR;

/***************************************************************************
 *  Socket
 */

typedef struct t_node {
    UH  port;   /* Port number 1 - 65535, 0 -> PORT any */
    UB  ver;    /* IP Address type */
    UB  num;    /* Device number   */
    UW  ipa;    /* IPv4 Address    */
#ifdef IPV6_SUP
    UW  ip6a[4];/* IPv6 Address    */
#endif
    const char *name;/* Node name */
}T_NODE;

typedef struct t_rcv_pkt_inf {
   UW   src_ipa;    /* Source IP Address */
   UW   dst_ipa;    /* Destination IP Address */
#ifdef IPV6_SUP
    UW  ip6sa[4];   /* IPv6 Source Address     */
    UW  ip6da[4];   /* IPv6 Destination Address    */
#endif
   UH   src_port;   /* Source Port */
   UH   dst_port;   /* Destination Port */
   UB   ttl;        /* IP TTL */
   UB   tos;        /* IP TOS */
   UB   ver;        /* IP Version */
   UB   num;        /* Received Device Number */
#ifdef LINK_INF_SUP
   UB  link[18];
#endif
}T_RCV_PKT_INF;

/* Socket callback  prototype */

typedef void(*SOC_HND)(UH,UH,ER);

#define EV_SOC_CON      0x0001U
#define EV_SOC_CLS      0x0002U
#define EV_SOC_SND      0x0004U
#define EV_SOC_RCV      0x0008U
#define EV_SOC_SRY      0x0010U
#define EV_SOC_RRY      0x0020U
#define EV_RDY_SND      0x0100U
#define EV_RDY_RCV      0x0200U
#define EV_SOC_ALL      0xFFFFU


#define INADDR_ANY          ((UW)0U)            /* Auto IP ADDRESS */
#define INADDR_BROADCAST    ((UW)0xFFFFFFFFU)   /* Broadcast IP ADDRESS */
#define PORT_ANY            ((UH)0U)            /* Auto PORT       */
#define DEV_ANY             ((UH)0U)
/* Supported IP VERSION */

#define IP_VER4             0U      /* IPv4 */
#define IP_VER6             1U      /* IPv6 */

/* SOC_CON */

#define SOC_SER             0U      /* Wait for connection      */
#define SOC_CLI             1U      /* Establish connection     */

/* SOC_CLS */

#define SOC_TCP_CLS         0U      /* CLOSE Connection */
#define SOC_TCP_SHT         1U      /* CLOSE Transmission */

/* SOC_ABT */

#define SOC_ABT_ALL         0U      /* ABORT All Activities             */
#define SOC_ABT_SND         1U      /* ABORT Send Process               */
#define SOC_ABT_RCV         2U      /* ABORT Recv Process               */
#define SOC_ABT_CON         3U      /* ABORT Connection Process         */
#define SOC_ABT_CLS         4U      /* ABORT Connection closing process */

/* SOC_CFG */
/* SOC_REF */
                                    /* Socket configuration             */
#define SOC_IP_TTL          0U
#define SOC_IP_TOS          1U
#define SOC_IP_MTU          2U
#define SOC_TCP_MTU         3U
#define SOC_MCAST_TTL       4U
#define SOC_MCAST_LOOP      5U
#define SOC_MCAST_JOIN      6U
#define SOC_MCAST_DROP      7U
#define SOC_BCAST_RECV      8U
#define SOC_TMO_SND         9U
#define SOC_TMO_RCV         10U
#define SOC_TMO_CON         11U
#define SOC_TMO_CLS         12U
#define SOC_IP_LOCAL        13U
#define SOC_IP_REMOTE       14U
#define SOC_CBK_HND         15U
#define SOC_CBK_FLG         16U
#define SOC_TCP_STATE       17U
#define SOC_RCV_PKT_INF     18U
#define SOC_PRT_LOCAL       19U
#define SOC_UDP_RQSZ        20U
#define SOC_SRC_IP_RESET    21U
#define SOC_MLD_START       22U
#define SOC_MLD_STOP        23U
#define SOC_UDP_EARLY_TX    24U
#define SOC_RCV_PKT_SIZE    25U
#define SOC_MSG_PEEK        26U
#define SOC_PRT_LOCAL_DUP   27U
#define SOC_IP_FLG_DF       28U

#define NET_IP4_CFG         0U
#define NET_IP4_TTL         1U
#define NET_BCAST_RCV       2U
#define NET_MCAST_JOIN      3U
#define NET_MCAST_DROP      4U
#define NET_MCAST_TTL       5U
#ifdef ACD_SUP
#define NET_ACD_CBK         6U
#endif
#define NET_MLD_START       7U
#define NET_MLD_STOP        8U
#define NET_MCAST_LOOP      9U
#define NET_IF_ACTIVE       10U
#define NET_PING_IGNORE     16U     /* ignore ping */


/* Gateway */
#define FLT_PROT_ALL     0x00U
#define FLT_PROT_ICMP    IP_PROTO_ICMP
#define FLT_PROT_TCP     IP_PROTO_TCP
#define FLT_PROT_UDP     IP_PROTO_UDP
#define FLT_PORT_ALL     0x0000U
#define FLT_ICMPTYPE_ALL 0xFFFFU

typedef void(*IP_FWD_EV_CBK)(UW ev, VP inf);

/* evnet success and failure */
#define FWD_EV_OK                0x10000000U
#define FWD_EV_ERR               0x20000000U

/* evnet trigger */
#define FWD_EV_PKT               0x01000000U
#define FWD_EV_API               0x02000000U
#define FWD_EV_TIM               0x04000000U

/* event paket forwarding procedure */
#define FWD_EV_PRC_SUCCESS       0x00000001U
#define FWD_EV_NATTBL_IN         0x00000002U
#define FWD_EV_NATTBL_IN_GPK     0x00000004U
#define FWD_EV_NATTBL_OUT_TCP    0x00000008U
#define FWD_EV_NATTBL_OUT_TMO    0x00000010U
#define FWD_EV_NATTBL_OUT_FUL    0x00000020U
#define FWD_EV_FILTER_ALLOW      0x00000040U
#define FWD_EV_FLTTBL_IN         0x00000080U
#define FWD_EV_FLTTBL_OUT        0x00000100U
#define FWD_EV_PRC_ERR_LOW       0x00000200U
#define FWD_EV_PRC_ERR_SAMEIF    0x00000400U
#define FWD_EV_PRC_ERR_NORTE     0x00000800U
#define FWD_EV_PRC_ERR_TTL       0x00001000U
#define FWD_EV_PRC_ERR_MEM       0x00002000U

/* evnet mask */
#define FWD_EV_MSK_ALL           0xffffffffU


#define EV_LINK     -97
#define EV_ADDR     -98


/***************************************************************************
 *  API
 */

/* TCP/IP Stack */

ER net_ini(void);
ER net_cfg(UH dev_num, UH opt, VP val);
ER net_ref(UH dev_num, UH opt, VP val);
ER net_ext(void);

/* Network Buffer */

ER net_buf_ini(void);
ER net_buf_get(T_NET_BUF **buf, UW len, TMO tmo);
void net_buf_ret(T_NET_BUF *buf);

/* Network Device */

ER net_dev_ini(UH dev_num);
ER net_dev_cls(UH dev_num);
ER net_dev_sts(UH dev_num, UH opt, VP val);
ER net_dev_ctl(UH dev_num, UH opt, VP val);
void net_pkt_rcv(T_NET_BUF *pkt);

/* Socket */
#ifndef NET_C
ER cre_soc(UB proto, T_NODE *host);
ER del_soc(SID sid);
#endif
ER con_soc(SID sid, T_NODE *host, UB con_flg);
ER snd_soc(SID sid, VP data, UH len);
ER rcv_soc(SID sid, VP data, UH len);
ER cls_soc(SID sid, UB cls_flg);
ER cfg_soc(SID sid, UB code, VP val);
ER ref_soc(SID sid, UB code, VP val);
ER abt_soc(SID sid, UB code);
ER soc_ext(UH dev_num);

/* Gateway */
#ifdef IP4_FWD_SUP
ER add_flt_ip(UW dstip, UB prot, UH dstport, UW fwdip, UH fwdport);
ER add_flt_tcp(UH dstport, UW fwdip, UH fwdport);
ER add_flt_udp(UH dstport, UW fwdip, UH fwdport);
ER add_flt_icmp(UW fwdip);
ER rmv_flt(ID id);
ER set_glb_if(UINT dev_num);
void set_usr_cbk(IP_FWD_EV_CBK cbk, UW cbk_flt);
#else
#define add_flt_ip(a, b, c, d, e)        (E_NOSPT)
#define add_flt_tcp(a, b, c)             (E_NOSPT)
#define add_flt_udp(a, b, c)             (E_NOSPT)
#define add_flt_icmp(a)                  (E_NOSPT)
#define rmv_flt(a)                       (E_NOSPT)
#define set_glb_if(a)                    (E_NOSPT)
#define set_usr_cbk(a, b)                ((void)0)
#endif

void net_tim_tsk(VP_INT exinf);

ER snd_rdy(SID sid);
ER rcv_rdy(SID sid);

#ifdef LO_IF_SUP
typedef ER(*LO_PKT_HDL)(UH,  T_NET_BUF*);
#endif

#ifndef NET_C
#define soc_cre cre_soc
#define soc_del del_soc
#endif
#define soc_con con_soc
#define soc_snd snd_soc
#define soc_rcv rcv_soc
#define soc_cls cls_soc
#define soc_cfg cfg_soc
#define soc_ref ref_soc
#define soc_abt abt_soc

/* Misc */

UW ip_aton(const char *str);
void ip_ntoa(const char *s, UW ipaddr);
UW ip_byte2n(char *s);
void ip_n2byte(char *s, UW ip);

void net_rand_seed(UW seed);
UW net_rand(void);

ER arp_set(UH dev_num, UW ip, UB *mac);
ER arp_ref(UH dev_num, UW ip, UB *mac);
ER arp_req(UH dev_num, UW ip);
ER arp_clr(void);
ER arp_del(UH dev_num, UW ip);

#ifdef ACD_SUP
ER net_acd(UH dev_num, T_NET_ACD *acd);
#endif

#ifdef IGMPv3_SUP
ER igmp_flt_set(UH dev_num, UW ga, UB mode, UW *ip, UH ip_num);
#else
#define igmp_flt_set(a, b, c, d, e)        (E_NOSPT)
#endif

UW get_net_sec(void);
void* net_memset(void *d, int c, SIZE n);
void* net_memcpy(void *d, const void *s, SIZE n);
int net_memcmp(const void *d, const void *s, SIZE n);


typedef struct t_soc_table {
    SID  sid;       /* Socket ID,   0 - Unused, 1 to NET_SOC_MAX + 1  */
    UB  proto;      /* IP Protocol, 0 - Unused, 6 - TCP, 17 - UDP  */
    UH  port;       /* Local Port,  0 - Default, 1 to 65535 */
    TMO snd_tmo;
    TMO rcv_tmo;
    TMO con_tmo;
    TMO cls_tmo;
    UW  sbufsz;
    UW  rbufsz;
    UH  dev_num;    /* Device Number */
    UB  ver;        /* IP version */
}T_SOC_TABLE;


#ifdef _UC3_ENDIAN_LITTLE
UH ntohs(UH val);
UW ntohl(UW val);
#else
#define ntohs(x) (x)
#define ntohl(x) (x)
#endif /* _UC3_ENDIAN_LITTLE */

#define htons(x) ntohs(x)
#define htonl(x) ntohl(x)

extern UW NET_DEV_MAX;
extern UW NET_SOC_MAX;
extern UW NET_TCP_MAX;
extern UW NET_ARP_MAX;
extern UW NET_MGR_MAX;
extern UW NET_IPR_MAX;
extern UW NET_BUF_CNT;
extern UW NET_BUF_SZ;
extern UW TCP_SND_WND_MAX;

#ifndef NULL
#ifdef __cplusplus
#define NULL    (0)
#else
#define NULL    ((void *)0)
#endif
#endif

#ifdef IPV6_SUP
#include "net_hdr6.h"
#endif

#ifdef __cplusplus
}
#endif
#endif /* NETHDR_H */
