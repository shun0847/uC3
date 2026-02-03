/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    DNS Resolver header file
    Copyright (c)  2009-2018, eForce Co., Ltd. All rights reserved.

    Version Information
      2009.06.03: Created
      2010.11.02: Support IPv6
      2015.12.14: The socket ID replaced SID types
      2016.02.10: Add include files for warning avoidance
      2016.07.06: Execute static analysis tool to this source.
      2018.08.16: Support retry for send request.
 ***************************************************************************/

#ifndef DNS_CLIENT_H
#define DNS_CLIENT_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "kernel.h"
#include "net_hdr.h"

#define DNS_RES_TMO         5000 /* DNS Response Timeout */

#define DNS_SERVER_PORT     53U  /* Domain Name System */

typedef struct t_dns_hdr {
    UH  id;
    UH  flag;
    UH  qdcount;
    UH  ancount;
    UH  nscount;
    UH  arcount;
}T_DNS_HDR;
#define DNS_HDR_SZ          12U

/* 0: */
#define DNS_QR_QUERY        0
#define DNS_QR_RES          1

/* 1-4:4 */
#define DNS_OP_QUERY        0
#define DNS_OP_IQUERY       1
#define DNS_OP_STATUS       2

/* 5: AA*/

/* 27-31:4 */
#define DNS_RCODE_NONE      0
#define DNS_RCODE_FORMAT    1
#define DNS_RCODE_SERVER    2
#define DNS_RCODE_NAME      3
#define DNS_RCODE_NOT_IMP   4
#define DNS_RCODE_REFUSED   5

typedef struct t_dns_rr {
    UH  type;
    UH  class;
    UW  ttl; 
    UH  rdlength;
    UB  rdata[2];   /*variable length*/
}T_DNS_RR;

typedef struct t_dns_client {
    UW ipa;
    char *name;
    UW *ipaddr;
    SID sid;
    UH code;
    UB dev_num;
    UB retry_cnt;
} T_DNS_CLIENT;


#define DNS_RR_SZ           10U

/* TYPE */
#define RR_TYPE_A           1U      /* host address */
#define RR_TYPE_NS          2U      /* an authoritative name server */
#define RR_TYPE_MD          3U      /* a mail destination (Obsolete - use MX) */
#define RR_TYPE_MF          4U      /* a mail forwarder (Obsolete - use MX) */
#define RR_TYPE_CNAME       5U      /* the canonical name for an alias */
#define RR_TYPE_SOA         6U      /* marks the start of a zone of authority */
#define RR_TYPE_MB          7U      /* a mailbox domain name (EXPERIMENTAL) */
#define RR_TYPE_MG          8U      /* a mail group member (EXPERIMENTAL) */
#define RR_TYPE_MR          9U      /* a mail rename domain name (EXPERIMENTAL) */
#define RR_TYPE_NULL        10U     /* a null RR (EXPERIMENTAL) */
#define RR_TYPE_WKS         11U     /* a well known service description */
#define RR_TYPE_PTR         12U     /* a domain name pointer */
#define RR_TYPE_HINFO       13U     /* host information */
#define RR_TYPE_MINFO       14U     /* mailbox or mail list information */
#define RR_TYPE_MX          15U     /* mail exchange */
#define RR_TYPE_TXT         16U     /* text strings */
#define RR_TYPE_AAAA        28U     /* ip6 host address */
/* QTYPE */
#define RR_TYPE_AXFR        252U    /* A request for a transfer of an entire zone */
#define RR_TYPE_MAILB       253U    /* A request for mailbox-related records (MB, MG or MR) */
#define RR_TYPE_MAILA       254U    /* A request for mail agent RRs (Obsolete - see MX) */
#define RR_TYPE_ANY         255U    /* A request for all records */

/* CLASS */
#define RR_CLASS_IN         1U      /* Internet      */
#define RR_CLASS_CS         2U      /* CSNET <obsolete> */
#define RR_CLASS_CH         3U      /* CHAOS     */
#define RR_CLASS_HS         4U      /* Hesiod    */
/* QCLASS */
#define RR_CLASS_ANY        255U    /* Any Class (*) */

/* Misc */
#define LABEL_MAX_LEN       63U
#define DNAME_MAX_LEN       255U
#define DNS_MSG_MAX_LEN     512U

/* API */
ER dns_get_ipaddr(SID socid, UW dns_server, char *name, UW *ipaddr);
ER dns_get_name(SID socid, UW dns_server, char *name, UW *ipaddr);
ER dns_query(UH code, char *name, UW *ipaddr, UW dns_server, SID socid);
#ifdef IPV6_SUP
ER dns_get_ip6addr(SID socid, UW dns_server, char *name, UW *ipaddr);
#endif

ER dns_query_ext(T_DNS_CLIENT *dc);

#ifdef __cplusplus
}
#endif
#endif /* DNS_CLIENT_H */

