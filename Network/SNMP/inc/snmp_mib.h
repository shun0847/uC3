/*
    SNMP
    MIB definitions
    Copyright (c) 2014-2016, eForce Co., Ltd. All rights reserved.
    
    2014-03-06 Created
    2016-01-06 Multiple network devices were supported
*/

#ifndef SNMP_MIB_H
#define SNMP_MIB_H

/* MIB Object ID */

/* System */
#define SNMP_MIB2_SYS_DESCR                 0     /* sysDescr */
#define SNMP_MIB2_SYS_OBJ_ID                1     /* sysObjectID */
#define SNMP_MIB2_SYS_UPTIME                2     /* sysUpTime */
#define SNMP_MIB2_SYS_CONTACT               3     /* sysContact */
#define SNMP_MIB2_SYS_NAME                  4     /* sysName */
#define SNMP_MIB2_SYS_LOCATION              5     /* sysLocation */
#define SNMP_MIB2_SYS_SERVICE               6     /* sysServices */

/* Address translation group */
#define SNMP_MIB2_AT_TABLE                  0     /* atTable */
#define SNMP_MIB2_AT_ENTRY                  1     /* atEntry */
#define SNMP_MIB2_AT_IDX                    2     /* atIfIndex */
#define SNMP_MIB2_AT_PHY_ADR                3     /* atPhysAddress */
#define SNMP_MIB2_AT_NET_ADR                4     /* atNetAddress */

/* IP group */
#define SNMP_MIB2_IP_FORWARD                0     /* ipForwarding */
#define SNMP_MIB2_IP_DEF_TTL                1     /* ipDefaultTTL */
#define SNMP_MIB2_IP_IN_RCV                 2     /* ipInReceives */
#define SNMP_MIB2_IP_IN_HDR_ERR             3     /* ipInHdrErrors */
#define SNMP_MIB2_IP_IN_ADDR_ERR            4     /* ipInAddrErrors */
#define SNMP_MIB2_IP_FORW_DATAGRAM          5     /* ipForwDatagrams */
#define SNMP_MIB2_IP_UNKNOWN_PROTO          6     /* ipInUnknownProtos */
#define SNMP_MIB2_IP_IN_DISCARD             7     /* ipInDiscards */
#define SNMP_MIB2_IP_IN_DELIVER             8     /* ipInDelivers */
#define SNMP_MIB2_IP_OUT_REQ                9     /* ipOutRequests */
#define SNMP_MIB2_IP_OUT_DISCARD            10    /* ipOutDiscards */
#define SNMP_MIB2_IP_OUT_NO_ROUTE           11    /* ipOutNoRoutes */
#define SNMP_MIB2_IP_REASM_TIMEOUT          12    /* ipReasmTimeout */
#define SNMP_MIB2_IP_REASM_REQ              13    /* ipReasmReqds */
#define SNMP_MIB2_IP_REASM_OK               14    /* ipReasmOKs */
#define SNMP_MIB2_IP_REASM_FAIL             15    /* ipReasmFails */
#define SNMP_MIB2_IP_FRAG_OK                16    /* ipFragOKs */
#define SNMP_MIB2_IP_FRAG_FAIL              17    /* ipFragFails */
#define SNMP_MIB2_IP_FRAG_CREATE            18    /* ipFragCreates */
#define SNMP_MIB2_IP_ADDR_TABLE             19    /* ipAddrTable */
#define SNMP_MIB2_IP_ADDR_ENTRY             20    /* ipAddrEntry */
#define SNMP_MIB2_IP_ENT_ADDR               21    /* ipAdEntAddr */
#define SNMP_MIB2_IP_ENT_IF_INDEX           22    /* ipAdEntIfIndex */
#define SNMP_MIB2_IP_ENT_NET_MASK           23    /* ipAdEntNetMask */
#define SNMP_MIB2_IP_ENT_BCAST_ADDR         24    /* ipAdEntBcastAddr */
#define SNMP_MIB2_IP_ENT_REASM_MAX_SIZE     25    /* ipAdEntReasmMaxSize */
#define SNMP_MIB2_IP_ROUTE_TABLE            26    /* ipRouteTable */
#define SNMP_MIB2_IP_ROUTE_ENTRY            27    /* ipRouteEntry */
#define SNMP_MIB2_IP_NET_TO_MEDIA_TABLE     28    /* ipNetToMediaTable */
#define SNMP_MIB2_IP_NET_TO_MEDIA_ENTRY     29    /* ipNetToMediaEntry */
#define SNMP_MIB2_IP_NET_TO_MEDIA_IF_INDEX  30    /* ipNetToMediaIfIndex */
#define SNMP_MIB2_IP_NET_TO_MEDIA_PHY_ADDR  31    /* ipNetToMediaPhysAddress */
#define SNMP_MIB2_IP_NET_TO_MEDIA_NET_ADDR  32    /* ipNetToMediaNetAddress */
#define SNMP_MIB2_IP_NET_TO_MEDIA_TYPE      33    /* ipNetToMediaType */
#define SNMP_MIB2_IP_ROUTING_DISCARD        34    /* ipRoutingDiscards */

/* ICMP group */
#define SNMP_MIB2_ICMP_IN_MSG               0     /* icmpInMsgs */
#define SNMP_MIB2_ICMP_IN_ERR               1     /* icmpInErrors */
#define SNMP_MIB2_ICMP_IN_DEST_UNREACH      2     /* icmpInDestUnreachs */
#define SNMP_MIB2_ICMP_IN_TIME_EXCD         3     /* icmpInTimeExcds */
#define SNMP_MIB2_ICMP_IN_PARM_PROB         4     /* icmpInParmProbs */
#define SNMP_MIB2_ICMP_IN_SRC_QUENCH        5     /* icmpInSrcQuenchs */
#define SNMP_MIB2_ICMP_IN_REDIRECT          6     /* icmpInRedirects */
#define SNMP_MIB2_ICMP_IN_ECHO              7     /* icmpInEchos */
#define SNMP_MIB2_ICMP_IN_ECHO_REP          8     /* icmpInEchoReps */
#define SNMP_MIB2_ICMP_IN_TIME_STAMP        9     /* icmpInTimestamps */
#define SNMP_MIB2_ICMP_IN_TIME_STAMP_REP    10    /* icmpInTimestampReps */
#define SNMP_MIB2_ICMP_IN_ADDR_MASK         11    /* icmpInAddrMasks */
#define SNMP_MIB2_ICMP_IN_ADDR_MASK_REP     12    /* icmpInAddrMaskReps */
#define SNMP_MIB2_ICMP_OUT_MSG              13    /* icmpOutMsgs */
#define SNMP_MIB2_ICMP_OUT_ERR              14    /* icmpOutErrors */
#define SNMP_MIB2_ICMP_OUT_DEST_UNREACH     15    /* icmpOutDestUnreachs */
#define SNMP_MIB2_ICMP_OUT_TIME_EXCD        16    /* icmpOutTimeExcds */
#define SNMP_MIB2_ICMP_OUT_PARM_PROB        17    /* icmpOutParmProbs */
#define SNMP_MIB2_ICMP_OUT_SRC_QUENCH       18    /* icmpOutSrcQuenchs */
#define SNMP_MIB2_ICMP_OUT_REDIRECT         19    /* icmpOutRedirects */
#define SNMP_MIB2_ICMP_OUT_ECHO             20    /* icmpOutEchos */
#define SNMP_MIB2_ICMP_OUT_ECHO_REP         21    /* icmpOutEchoReps */
#define SNMP_MIB2_ICMP_OUT_TIME_STAMP       22    /* icmpOutTimestamps */
#define SNMP_MIB2_ICMP_OUT_TIME_STAMP_REP   23    /* icmpOutTimestampReps */
#define SNMP_MIB2_ICMP_OUT_ADDR_MASK        24    /* icmpOutAddrMasks */
#define SNMP_MIB2_ICMP_OUT_ADDR_MASK_REP    25    /* icmpOutAddrMaskReps */

/* TCP group */
#define SNMP_MIB2_TCP_RTO_ALGO              0     /* tcpRtoAlgorithm */
#define SNMP_MIB2_TCP_RTO_MIN               1     /* tcpRtoMin */
#define SNMP_MIB2_TCP_RTO_MAX               2     /* tcpRtoMax */
#define SNMP_MIB2_TCP_MAX_CONN              3     /* tcpMaxConn */
#define SNMP_MIB2_TCP_ACTIVE_OPEN           4     /* tcpActiveOpens */
#define SNMP_MIB2_TCP_PASSIVE_OPEN          5     /* tcpPassiveOpens */
#define SNMP_MIB2_TCP_ATTEMPT_FAIL          6     /* tcpAttemptFails */
#define SNMP_MIB2_TCP_ESTAB_RESET           7     /* tcpEstabResets */
#define SNMP_MIB2_TCP_CURR_ESTAB            8     /* tcpCurrEstab */
#define SNMP_MIB2_TCP_IN_SEG                9     /* tcpInSegs */
#define SNMP_MIB2_TCP_OUT_SEG               10    /* tcpOutSegs */
#define SNMP_MIB2_TCP_RETRANS_SEG           11    /* tcpRetransSegs */
#define SNMP_MIB2_TCP_CONN_TABLE            12    /* tcpConnTable */
#define SNMP_MIB2_TCP_CONN_ENTRY            13    /* tcpConnEntry */
#define SNMP_MIB2_TCP_CONN_STS              14    /* tcpConnState */
#define SNMP_MIB2_TCP_CONN_LOCAL_ADR        15    /* tcpConnLocalAddress */
#define SNMP_MIB2_TCP_CONN_LOCAL_RMT        16    /* tcpConnLocalPort */
#define SNMP_MIB2_TCP_CONN_RMT_ADR          17    /* tcpConnRemAddress */
#define SNMP_MIB2_TCP_CONN_RMT_PORT         18    /* tcpConnRemPort */
#define SNMP_MIB2_TCP_IN_ERR                19    /* tcpInErrs */
#define SNMP_MIB2_TCP_OUT_RST               20    /* tcpOutRsts */

/* UDP group */
#define SNMP_MIB2_UDP_IN_DATAGRAM           0     /* udpInDatagrams */
#define SNMP_MIB2_UDP_NO_PORT               1     /* udpNoPorts */
#define SNMP_MIB2_UDP_IN_ERR                2     /* udpInErrors */
#define SNMP_MIB2_UDP_OUT_DATAGRAM          3     /* udpOutDatagrams */
#define SNMP_MIB2_UDP_TBL                   4     /* udpTable */
#define SNMP_MIB2_UDP_ENTRY                 5     /* udpEntry */
#define SNMP_MIB2_UDP_LOCAL_ADDR            6     /* udpLocalAddress */
#define SNMP_MIB2_UDP_LOCAL_PORT            7     /* udpLocalPort */

/* SNMP group */
#define SNMP_MIB2_SNMP_IN_PKT               0     /* snmpInPkts */
#define SNMP_MIB2_SNMP_OUT_PKT              1     /* snmpOutPkts */
#define SNMP_MIB2_SNMP_IN_BAD_VERSION       2     /* snmpInBadVersions */
#define SNMP_MIB2_SNMP_IN_BAD_COM_NAME      3     /* snmpInBadCommunityNames */
#define SNMP_MIB2_SNMP_IN_BAD_COM_USE       4     /* snmpInBadCommunityUses */
#define SNMP_MIB2_SNMP_IN_ASN_PARSE_ERR     5     /* snmpInASNParseErrs */
#define SNMP_MIB2_SNMP_IN_TOO_BIG           6     /* snmpInTooBigs */
#define SNMP_MIB2_SNMP_IN_NO_SUCH_NAME      7     /* snmpInNoSuchNames */
#define SNMP_MIB2_SNMP_IN_BAD_VALUE         8     /* snmpInBadValues */
#define SNMP_MIB2_SNMP_IN_READ_ONLY         9     /* snmpInReadOnlys */
#define SNMP_MIB2_SNMP_IN_GEN_ERR           10    /* snmpInGenErrs */
#define SNMP_MIB2_SNMP_IN_TOTAL_REQ_VAR     11    /* snmpInTotalReqVars */
#define SNMP_MIB2_SNMP_IN_TOTAL_SET_VAR     12    /* snmpInTotalSetVars */
#define SNMP_MIB2_SNMP_IN_GET_REQUEST       13    /* snmpInGetRequests */
#define SNMP_MIB2_SNMP_IN_GET_NEXT          14    /* snmpInGetNexts */
#define SNMP_MIB2_SNMP_IN_SET_REQUEST       15    /* snmpInSetRequests */
#define SNMP_MIB2_SNMP_IN_GET_RESPONSE      16    /* snmpInGetResponses */
#define SNMP_MIB2_SNMP_IN_TRAP              17    /* snmpInTraps */
#define SNMP_MIB2_SNMP_OUT_TOO_BIG          18    /* snmpOutTooBigs */
#define SNMP_MIB2_SNMP_OUT_NO_SUCH_NAME     19    /* snmpOutNoSuchNames */
#define SNMP_MIB2_SNMP_OUT_BAD_VALUE        20    /* snmpOutBadValues */
#define SNMP_MIB2_SNMP_OUT_GEN_ERR          21    /* snmpOutGenErrs */
#define SNMP_MIB2_SNMP_OUT_GET_REQUEST      22    /* snmpOutGetRequests */
#define SNMP_MIB2_SNMP_OUT_GET_NEXT         23    /* snmpOutGetNexts */
#define SNMP_MIB2_SNMP_OUT_SET_REQUEST      24    /* snmpOutSetRequests */
#define SNMP_MIB2_SNMP_OUT_GET_RESPONSE     25    /* snmpOutGetResponses */
#define SNMP_MIB2_SNMP_OUT_TRAP             26    /* snmpOutTraps */
#define SNMP_MIB2_SNMP_OUT_ENA_AUTH_TRAP    27    /* snmpEnableAuthenTraps */

/* Interfaces group */
#define SNMP_MIB2_IF_NUM                    0     /* ifNumber */
#define SNMP_MIB2_IF_TABLE                  1     /* ifTable */
#define SNMP_MIB2_IF_ENTRY                  2     /* ifEntry */
#define SNMP_MIB2_IF_INDEX                  3     /* ifIndex 1 */
#define SNMP_MIB2_IF_DESCR                  4     /* ifDescr 1 */
#define SNMP_MIB2_IF_TYPE                   5     /* ifType 1 */
#define SNMP_MIB2_IF_MTU                    6     /* ifMtu 1 */
#define SNMP_MIB2_IF_SPEED                  7     /* ifSpeed 1 */
#define SNMP_MIB2_IF_PHY_ADDR               8     /* ifPhysAddress 1 */
#define SNMP_MIB2_IF_ADMIN_STS              9     /* ifAdminStatus 1 */
#define SNMP_MIB2_IF_OPER_STS               10    /* ifOperStatus 1 */
#define SNMP_MIB2_IF_LAST_CHG               11    /* ifLastChanges 1 */
#define SNMP_MIB2_IF_IN_OCTET               12    /* ifInOctets 1 */
#define SNMP_MIB2_IF_IN_UCAST_PKT           13    /* ifInUcastPkts 1 */
#define SNMP_MIB2_IF_IN_NUCAST_PKT          14    /* ifInNUcastPkts 1 */
#define SNMP_MIB2_IF_IN_DISCARD             15    /* ifInDiscards 1 */
#define SNMP_MIB2_IF_IN_ERR                 16    /* ifInErrors 1 */
#define SNMP_MIB2_IF_IN_UNKNOWN_PROTO       17    /* ifInUnknownProtos 1 */
#define SNMP_MIB2_IF_OUT_OCTET              18    /* ifOutOctets 1 */
#define SNMP_MIB2_IF_OUT_UCAST_PKT          19    /* ifOutUcastPkts 1 */
#define SNMP_MIB2_IF_OUT_NUCAST_PKT         20    /* ifOutNUcastPkts 1 */
#define SNMP_MIB2_IF_OUT_DISCARD            21    /* ifOutDiscards 1 */
#define SNMP_MIB2_IF_OUT_ERR                22    /* ifOutErrors 1 */
#define SNMP_MIB2_IF_OUT_QLEN               23    /* ifOutQLen 1 */
#define SNMP_MIB2_IF_SPECIFIC               24    /* ifSpecific 1 */

/* Number of objects */
#define SNMP_MIB2_DEF_CNT_SYS     7     /* System */
#define SNMP_MIB2_DEF_CNT_AT      5     /* Address translation */
#define SNMP_MIB2_DEF_CNT_IP      35    /* IP */
#define SNMP_MIB2_DEF_CNT_ICMP    26    /* ICMP */
#define SNMP_MIB2_DEF_CNT_TCP     21    /* TCP */
#define SNMP_MIB2_DEF_CNT_UDP     8     /* UDP */
#define SNMP_MIB2_DEF_CNT_SNMP    28    /* SNMP */
#define SNMP_MIB2_DEF_CNT_ITF_IDX 3     /* Interfaces index */
#define SNMP_MIB2_DEF_CNT_ITF_TBL 22    /* Interfaces table */

/* Index for number of objects */
#define SNMP_MIB2_CNT_SYS     0     /* System */
#define SNMP_MIB2_CNT_AT      1     /* Address translation */
#define SNMP_MIB2_CNT_IP      2     /* IP */
#define SNMP_MIB2_CNT_ICMP    3     /* ICMP */
#define SNMP_MIB2_CNT_TCP     4     /* TCP */
#define SNMP_MIB2_CNT_UDP     5     /* UDP */
#define SNMP_MIB2_CNT_SNMP    6     /* SNMP */
#define SNMP_MIB2_CNT_ITF     7     /* Interfaces */
#define SNMP_MIB2_CNT_GRP     8     /* Number of groups */

/* Data index */
#define SNMP_MIB2_DAT_SYS     0    /* System */
#define SNMP_MIB2_DAT_ITF     1    /* Interface */
#define SNMP_MIB2_DAT_SNMP    2    /* SNMP */

#endif

