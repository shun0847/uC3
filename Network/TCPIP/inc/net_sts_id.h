/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK
    Network status header, macro definitions
    Copyright (c) 2014-2023, eForce Co., Ltd. All rights reserved.
    
    Version Information
      2014.03.06: Created
      2023.05.25: Fixed interface MIB is invalid in 64bit environment.
 **************************************************************************/

#ifndef NETSTSID_H
#define NETSTSID_H
#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************
    Macros
***************************************************************************/
/* Protocol */
#define NET_STS_PRTO_IP4     0x0001    /* IPv4 */
#define NET_STS_PRTO_IP6     0x0002    /* IPv6 */

/* Interface type */
#define NET_STS_TYPE_OTHER      1     /* Other */
#define NET_STS_TYPE_ETH        6     /* Ethernet CSMACD */
#define NET_STS_TYPE_PPP        23    /* PPP */
#define NET_STS_TYPE_SOFT_LOOP  24    /* Software loop back */

/* Interface status */
#define NET_STS_LINK_UP         1     /* Link up */
#define NET_STS_LINK_DOWN       2     /* Link down */
#define NET_STS_LINK_TEST       3     /* Test */
#define NET_STS_LINK_UNKNOWN    4     /* Unknown */

/* Socket status */
#define NET_STS_SOC_CLS         0x0001    /* Close */
#define NET_STS_SOC_CON         0x0002    /* Connection */

/* Callback */
#define NET_STS_CBK_DIS         0x0000    /* Disable */
#define NET_STS_CBK_ENA         0x0001    /* Enable */

/***************************************************************************
    Status Object ID
***************************************************************************/
/* Group ID */
#define NET_STS_IP      0       /* IP */
#define NET_STS_ICMP    1       /* ICMP */
#define NET_STS_TCP     2       /* TCP */
#define NET_STS_UDP     3       /* UDP */
#define NET_STS_IFS     4       /* Interface */

/* Interfaces group */
#define NET_STS_IF_BASE                   (0)
#define NET_STS_IF_INDEX                  (NET_STS_IF_BASE)          /* ifIndex */
#define NET_STS_IF_DESCR                  (NET_STS_IF_BASE + 1)      /* ifDescr */
#define NET_STS_IF_TYPE                   (NET_STS_IF_BASE + 2)      /* ifType */
#define NET_STS_IF_MTU                    (NET_STS_IF_BASE + 3)      /* ifMtu */
#define NET_STS_IF_SPEED                  (NET_STS_IF_BASE + 4)      /* ifSpeed */
#define NET_STS_IF_PHY_ADDR               (NET_STS_IF_BASE + 5)      /* ifPhysAddress */
#define NET_STS_IF_ADMIN_STS              (NET_STS_IF_BASE + 6)      /* ifAdminStatus */
#define NET_STS_IF_OPER_STS               (NET_STS_IF_BASE + 7)      /* ifOperStatus */
#define NET_STS_IF_LAST_CHG               (NET_STS_IF_BASE + 8)      /* ifLastChanges */
#define NET_STS_IF_IN_OCTET               (NET_STS_IF_BASE + 9)      /* ifInOctets */
#define NET_STS_IF_IN_UCAST_PKT           (NET_STS_IF_BASE + 10)     /* ifInUcastPkts */
#define NET_STS_IF_IN_NUCAST_PKT          (NET_STS_IF_BASE + 11)     /* ifInNUcastPkts */
#define NET_STS_IF_IN_DISCARD             (NET_STS_IF_BASE + 12)     /* ifInDiscards */
#define NET_STS_IF_IN_ERR                 (NET_STS_IF_BASE + 13)     /* ifInErrors */
#define NET_STS_IF_IN_UNKNOWN_PROTO       (NET_STS_IF_BASE + 14)     /* ifInUnknownProtos */
#define NET_STS_IF_OUT_OCTET              (NET_STS_IF_BASE + 15)     /* ifOutOctets */
#define NET_STS_IF_OUT_UCAST_PKT          (NET_STS_IF_BASE + 16)     /* ifOutUcastPkts */
#define NET_STS_IF_OUT_NUCAST_PKT         (NET_STS_IF_BASE + 17)     /* ifOutNUcastPkts */
#define NET_STS_IF_OUT_DISCARD            (NET_STS_IF_BASE + 18)     /* ifOutDiscards */
#define NET_STS_IF_OUT_ERR                (NET_STS_IF_BASE + 19)     /* ifOutErrors */
#define NET_STS_IF_OUT_QLEN               (NET_STS_IF_BASE + 20)     /* ifOutQLen */
#if (_kernel_SIZE_SIZE==8)
#define NET_STS_IF_DESCR_U32              (NET_STS_IF_BASE + 21)     /* upper32bit - ifDescr */
#define NET_STS_IF_PHY_ADDR_U32           (NET_STS_IF_BASE + 22)     /* upper32bit - ifPhysAddress */
#endif
    
    
/* IP group */
#define NET_STS_IP_BASE                  (0)
#define NET_STS_IP_FORWARD               (NET_STS_IP_BASE)           /* ipForwarding */
#define NET_STS_IP_DEF_TTL               (NET_STS_IP_BASE + 1)       /* ipDefaultTTL */
#define NET_STS_IP_IN_RCV                (NET_STS_IP_BASE + 2)       /* ipInReceives */
#define NET_STS_IP_IN_HDR_ERR            (NET_STS_IP_BASE + 3)       /* ipInHdrErrors */
#define NET_STS_IP_IN_ADDR_ERR           (NET_STS_IP_BASE + 4)       /* ipInAddrErrors */
#define NET_STS_IP_FWD_DGRAM             (NET_STS_IP_BASE + 5)       /* ipForwDatagrams */
#define NET_STS_IP_UNKNOWN_PROTO         (NET_STS_IP_BASE + 6)       /* ipInUnknownProtos */
#define NET_STS_IP_IN_DISCARD            (NET_STS_IP_BASE + 7)       /* ipInDiscards */
#define NET_STS_IP_IN_DELIVER            (NET_STS_IP_BASE + 8)       /* ipInDelivers */
#define NET_STS_IP_OUT_REQ               (NET_STS_IP_BASE + 9)       /* ipOutRequests */
#define NET_STS_IP_OUT_DISCARD           (NET_STS_IP_BASE + 10)      /* ipOutDiscards */
#define NET_STS_IP_OUT_NO_ROUTE          (NET_STS_IP_BASE + 11)      /* ipOutNoRoutes */
#define NET_STS_IP_REASM_TMOUT           (NET_STS_IP_BASE + 12)      /* ipReasmTimeout */
#define NET_STS_IP_REASM_REQ             (NET_STS_IP_BASE + 13)      /* ipReasmReqds */
#define NET_STS_IP_REASM_OK              (NET_STS_IP_BASE + 14)      /* ipReasmOKs */
#define NET_STS_IP_REASM_FAIL            (NET_STS_IP_BASE + 15)      /* ipReasmFails */
#define NET_STS_IP_FRAG_OK               (NET_STS_IP_BASE + 16)      /* ipFragOKs */
#define NET_STS_IP_FRAG_FAIL             (NET_STS_IP_BASE + 17)      /* ipFragFails */
#define NET_STS_IP_FRAG_CREATE           (NET_STS_IP_BASE + 18)      /* ipFragCreates */
#define NET_STS_IP_ROUTING_DISCARD       (NET_STS_IP_BASE + 19)      /* ipRoutingDiscards */

/* ICMP group */
#define NET_STS_ICMP_BASE                 (0)
#define NET_STS_ICMP_IN_MSG               (NET_STS_ICMP_BASE)        /* icmpInMsgs */
#define NET_STS_ICMP_IN_ERR               (NET_STS_ICMP_BASE + 1)    /* icmpInErrors */
#define NET_STS_ICMP_IN_DEST_UNREACH      (NET_STS_ICMP_BASE + 2)    /* icmpInDestUnreachs */
#define NET_STS_ICMP_IN_TIME_EXCD         (NET_STS_ICMP_BASE + 3)    /* icmpInTimeExcds */
#define NET_STS_ICMP_IN_PARM_PROB         (NET_STS_ICMP_BASE + 4)    /* icmpInParmProbs */
#define NET_STS_ICMP_IN_SRC_QUENCH        (NET_STS_ICMP_BASE + 5)    /* icmpInSrcQuenchs */
#define NET_STS_ICMP_IN_REDIRECT          (NET_STS_ICMP_BASE + 6)    /* icmpInRedirects */
#define NET_STS_ICMP_IN_ECHO              (NET_STS_ICMP_BASE + 7)    /* icmpInEchos */
#define NET_STS_ICMP_IN_ECHO_REP          (NET_STS_ICMP_BASE + 8)    /* icmpInEchoReps */
#define NET_STS_ICMP_IN_TIME_STAMP        (NET_STS_ICMP_BASE + 9)    /* icmpInTimestamps */
#define NET_STS_ICMP_IN_TIME_STAMP_REP    (NET_STS_ICMP_BASE + 10)   /* icmpInTimestampReps */
#define NET_STS_ICMP_IN_ADDR_MASK         (NET_STS_ICMP_BASE + 11)   /* icmpInAddrMasks */
#define NET_STS_ICMP_IN_ADDR_MASK_REP     (NET_STS_ICMP_BASE + 12)   /* icmpInAddrMaskReps */
#define NET_STS_ICMP_OUT_MSG              (NET_STS_ICMP_BASE + 13)   /* icmpOutMsgs */
#define NET_STS_ICMP_OUT_ERR              (NET_STS_ICMP_BASE + 14)   /* icmpOutErrors */
#define NET_STS_ICMP_OUT_DEST_UNREACH     (NET_STS_ICMP_BASE + 15)   /* icmpOutDestUnreachs */
#define NET_STS_ICMP_OUT_TIME_EXCD        (NET_STS_ICMP_BASE + 16)   /* icmpOutTimeExcds */
#define NET_STS_ICMP_OUT_PARM_PROB        (NET_STS_ICMP_BASE + 17)   /* icmpOutParmProbs */
#define NET_STS_ICMP_OUT_SRC_QUENCH       (NET_STS_ICMP_BASE + 18)   /* icmpOutSrcQuenchs */
#define NET_STS_ICMP_OUT_REDIRECT         (NET_STS_ICMP_BASE + 19)   /* icmpOutRedirects */
#define NET_STS_ICMP_OUT_ECHO             (NET_STS_ICMP_BASE + 20)   /* icmpOutEchos */
#define NET_STS_ICMP_OUT_ECHO_REP         (NET_STS_ICMP_BASE + 21)   /* icmpOutEchoReps */
#define NET_STS_ICMP_OUT_TIME_STAMP       (NET_STS_ICMP_BASE + 22)   /* icmpOutTimestamps */
#define NET_STS_ICMP_OUT_TIME_STAMP_REP   (NET_STS_ICMP_BASE + 23)   /* icmpOutTimestampReps */
#define NET_STS_ICMP_OUT_ADDR_MASK        (NET_STS_ICMP_BASE + 24)   /* icmpOutAddrMasks */
#define NET_STS_ICMP_OUT_ADDR_MASK_REP    (NET_STS_ICMP_BASE + 25)   /* icmpOutAddrMaskReps */

/* TCP group */
#define NET_STS_TCP_BASE                  (0)
#define NET_STS_TCP_RTO_ALGO              (NET_STS_TCP_BASE)         /* tcpRtoAlgorithm */
#define NET_STS_TCP_RTO_MIN               (NET_STS_TCP_BASE + 1)     /* tcpRtoMin */
#define NET_STS_TCP_RTO_MAX               (NET_STS_TCP_BASE + 2)     /* tcpRtoMax */
#define NET_STS_TCP_MAX_CONN              (NET_STS_TCP_BASE + 3)     /* tcpMaxConn */
#define NET_STS_TCP_ACTIVE_OPEN           (NET_STS_TCP_BASE + 4)     /* tcpActiveOpens */
#define NET_STS_TCP_PASSIVE_OPEN          (NET_STS_TCP_BASE + 5)     /* tcpPassiveOpens */
#define NET_STS_TCP_ATTEMPT_FAIL          (NET_STS_TCP_BASE + 6)     /* tcpAttemptFails */
#define NET_STS_TCP_ESTAB_RESET           (NET_STS_TCP_BASE + 7)     /* tcpEstabResets */
#define NET_STS_TCP_CURR_ESTAB            (NET_STS_TCP_BASE + 8)     /* tcpCurrEstab */
#define NET_STS_TCP_IN_SEG                (NET_STS_TCP_BASE + 9)     /* tcpInSegs */
#define NET_STS_TCP_OUT_SEG               (NET_STS_TCP_BASE + 10)    /* tcpOutSegs */
#define NET_STS_TCP_RETRANS_SEG           (NET_STS_TCP_BASE + 11)    /* tcpRetransSegs */
#define NET_STS_TCP_IN_ERR                (NET_STS_TCP_BASE + 12)    /* tcpInErrs */
#define NET_STS_TCP_OUT_RST               (NET_STS_TCP_BASE + 13)    /* tcpOutRsts */

/* UDP group */
#define NET_STS_UDP_BASE                  (0)
#define NET_STS_UDP_IN_DATAGRAM           (NET_STS_UDP_BASE)         /* udpInDatagrams */
#define NET_STS_UDP_NO_PORT               (NET_STS_UDP_BASE + 1)     /* udpNoPorts */
#define NET_STS_UDP_IN_ERR                (NET_STS_UDP_BASE + 2)     /* udpInErrors */
#define NET_STS_UDP_OUT_DATAGRAM          (NET_STS_UDP_BASE + 3)     /* udpOutDatagrams */

#ifdef __cplusplus
}
#endif
#endif

