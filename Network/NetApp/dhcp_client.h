/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    DHCP Client header file
    Copyright (c)  2008-2022, eForce Co., Ltd. All rights reserved.

    Version Information
      2008.11.30: Created
      2014.04.11: Corrected to "UH" a type of "dev_num".
      2015.12.14: The socket ID replaced SID types
      2016.02.10: Add include files for warning avoidance
      2016.07.06: Execute static analysis tool to this source.
      2016.12.06: Changed the size of T_DHCP_MSG::opt[].
      2022.10.14: Added definition "DHCP_OPT_PAD".
 ***************************************************************************/

#ifndef DHCP_CLIENT_H
#define DHCP_CLIENT_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "kernel.h"
#include "net_hdr.h"

/* DHCP Client State            */

#define DHCP_STS_INITREBOOT     0
#define DHCP_STS_INIT           1
#define DHCP_STS_REBOOTING      2
#define DHCP_STS_REQUESTING     3
#define DHCP_STS_BOUND          4
#define DHCP_STS_SELECTING      5
#define DHCP_STS_REBINDING      6
#define DHCP_STS_RENEWING       7

/*DHCP Messsage*/

typedef struct t_dhcp_msg {
    UB   op;
    UB   htype;
    UB   hlen;
    UB   hops;
    UW   xid;
    UH   secs;
    UH   flags;
    UW   ciaddr;
    UW   yiaddr;
    UW   siaddr;
    UW   giaddr;
    char chaddr[16];
    char sname[64];
    char file[128];
    UB   opt[312];
}T_DHCP_MSG;

#define DHCP_MSG_SND_SZ     300U    /* Send DHCP Message size */
#define DHCP_MSG_LEN        240U    /* Up to magiccookie opt[4] */

/* DHCP Message Fields      */
#define DHCP_OPC_BOOTREQ    1U
#define DHCP_OPC_BOOTREPLY  2U
#define DHCP_ETH_TYPE       1U      /*Ethernet(10MB) IANA:arp-parameters*/
#define DHCP_ETH_LEN        6U
#define DHCP_FLG_BCAST      0x8000U

/* DHCP Messages Type (RFC 2132)*/
#define DHCP_MSG_DISCOVER   1U
#define DHCP_MSG_OFFER      2U
#define DHCP_MSG_REQUEST    3U
#define DHCP_MSG_DECLINE    4U
#define DHCP_MSG_ACK        5U
#define DHCP_MSG_NAK        6U
#define DHCP_MSG_RELEASE    7U

/* DHCP Options */
#define DHCP_OPT_SUBNET         1U
#define DHCP_OPT_ROUTER         3U
#define DHCP_OPT_DNS            6U
#define DHCP_OPT_REQIPADDR      50U     /*:4*/
#define DHCP_OPT_IPLEASE        51U     /*:1*/
#define DHCP_OPT_DHCPMSGTYPE    53U     /*:1*/
#define DHCP_OPT_SERVERIDENT    54U     /* Server Identifier */
#define DHCP_OPT_PRMLST         55U     /* Parameter Request List */
#define DHCP_OPT_RENETM         58U     /* Renewal Time */
#define DHCP_OPT_REBITM         59U     /* Rebinding Time */
#define DHCP_OPT_CLIENT         61U     /* Client ID */
#define DHCP_OPT_END            255U    /* End Option */
#define DHCP_OPT_PAD            0U      /* padding */

/* For Multichannel */
typedef struct t_host_addr {
    UW ipaddr;
    UW subnet;
    UW gateway;
    UW dhcp;
    UW dns[2];
    UW lease;   /* IP Lease time */
    UW t1;      /* Renew  Time   */
    UW t2;      /* Rebind Time   */
    UB mac[6];
    UH dev_num;
    UB state;
    SID socid;
}T_HOST_ADDR;

ER dhcp_client(T_HOST_ADDR *addr);

#ifdef __cplusplus
}
#endif
#endif /* DHCP_CLIENT_H */

