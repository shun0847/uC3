/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    DHCP Server header file
    Copyright (c) 2010-2016, eForce Co., Ltd. All rights reserved.

    Version Information
      2010.06.01: Created
      2014.04.24: Modified to work with multiple network devices
      2015.03.23: 1. Define new callback ID DHCP_SERVER_ALT_EXPIREPOOL.
                     This ID is used when lease time is expired.
                  2. Define new callback ID DHCP_SERVER_ALT_RELPOOL.
                     This ID is used when DHCP release be received.
                  3. Modify callback ID
                     DCHP_SERVER_ALT_xxxx -> DHCP_SERVER_ALT.
      2015.06.02: Add define DHCPD_BOOTP_MIN_LEN
      2015.12.14: The socket ID replaced SID types
      2016.07.13: Execute static analysis tool to this source.
      2016.08.26: Corrected of the build error problems when with DHCP client.
      2016.11.01: Execute static analysis tool to this source.
      2017.10.06: Added the following functions.
        1. Supported multiple DHCP scopes.
        2. Added a mechanism to notify users of some events.
 ***************************************************************************/

#ifndef DHCP_SEVER_H
#define DHCP_SEVER_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "kernel.h"
#include "net_hdr.h"
#include "dhcp_server_cfg.h"

/* DHCP UDP Port                */

#ifndef DHCP_SERVER_PORT
#define DHCP_SERVER_PORT        67
#endif
#ifndef DHCP_CLIENT_PORT
#define DHCP_CLIENT_PORT        68
#endif
#ifndef DHCP_AGENT_PORT
#define DHCP_AGENT_PORT         67
#endif

/* DHCP Option kind                                                          */
#define DHCP_OPT_KIND_STRING    0U      /* string format                     */
#define DHCP_OPT_KIND_ADDRESS   1U      /* address format                    */
#define DHCP_OPT_KIND_BINARY    2U      /* binary format                     */

/* DHCP Option convert mode                                                  */
#define DHCP_MODE_LITTLE_ENDIAN 0U      /* little endian convert             */
#define DHCP_MODE_BIG_ENDIAN    1U      /* bit endian convert                */

/* DHCP Option                                                               */
typedef struct t_dhcpd_msg_option {
    UB  kind;                           /* option kind                       */
    UB  number;                         /* option number                     */
    UB  len;                            /* option length                     */
    UB  conv_mode;                      /* convert mode                      */
    VP  *data;                          /* option data                       */
} T_DHCPD_MSG_OPTION;


/* DHCP Messsage */

typedef struct t_dhcpd_msg {
    UB op;
    UB htype;
    UB hlen;
    UB hops;
    UW xid;
    UH secs;
    UH flags;
    UW ciaddr;
    UW yiaddr;
    UW siaddr;
    UW giaddr;
    char chaddr[16];
    char sname[64];
    char file[128];
    UB opt[312];
}T_DHCPD_MSG;

#define DHCPD_HDR_LEN        236U
#define DHCPD_BOOTP_MIN_LEN  300U       /* minimal length of BOOTP message   */
#define DHCPD_MSG_SZ         548U

/* DHCP Server Control                                                       */
#define DHCP_SERVER_RETRY_WAIT  100     /* wait retrying(ms)                 */
#define DHCP_SERVER_RCVMBX_POLL 1000    /* rcvmbx polling(ms)                */
#define DHCP_SERVER_BOOT        1U      /* dhcp server boot flag             */
#define DHCP_SERVER_STOP        0U      /* dhcp server stop flag             */

/* DHCP Message Fields      */
#define DHCP_OPC_BOOTREQ    1U
#define DHCP_OPC_BOOTREPLY  2U
#define DHCP_ETH_TYPE       1U          /*Ethernet(10MB) IANA:arp-parameters*/
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
#define DHCP_OPT_PAD            0U   /*:1*/
#define DHCP_OPT_SUBNET         1U   /*:4*/
#define DHCP_OPT_ROUTER         3U   /*:4n*/
#define DHCP_OPT_DNS            6U   /*:4n*/
#define DHCP_OPT_REQIPADDR      50U  /*:4*/
#define DHCP_OPT_IPLEASE        51U  /*:1*/
#define DHCP_OPT_DHCPMSGTYPE    53U  /*:1*/
#define DHCP_OPT_SERVERIDENT    54U  /*:4*/
#define DHCP_OPT_PRMLST         55U  /* Parameter Request List */
#define DHCP_OPT_RENETM         58U  /*:4 Renewal Time */
#define DHCP_OPT_REBITM         59U  /*:4 Rebinding Time */
#define DHCP_OPT_CLIENT         61U  /*:n Client ID */
#define DHCP_OPT_AGENTINFO      82U  /*:  Agent Information Option */
#define DHCP_OPT_END            255U /*:1*/

/* DHCP defines                                                              */
#define DHCP_SERVER_NOT_LEASE   0U  /* not lease                             */
                                    /* infinite lease                        */
#define DHCP_SERVER_INFINITE_LEASE  0xFFFFFFFFU

typedef struct t_dhcp_ctl {
    UW server;              /* DHCP server address  */
    UW xid;                 /* Random trans id      */
    UB *opt_ptr;            /* rcv_msg processing   */
    UH opt_len;             /* rcv_msg processing   */
    UH flg;                 /* rcv_msg processing   */
    T_DHCPD_MSG snd_msg;    /* Tx message   */
    T_DHCPD_MSG rcv_msg;    /* Rx message   */
    T_NET_BUF *pkt[2];      /* top address  */
    UW cli_ipa;             /* Client IP address */
    UB *agent_info;         /* Agent Information Option */
}T_DHCP_CTL;


/*------------ DHCP Server Definition ------------*/
#define DHCP_RCV_TSK_PRI  3
#define DHCP_LEASE_NUM    (DHCP_POOL_SIZE)
#define DHCP_MSG_BUFF_CNT 5
#define DHCP_MSG_BUFF_SZ  (DHCP_MSG_SZ + 4)

/* DHCP Server Alert Type */
#define DHCP_SERVER_ALT_UNSUPPORTED 1   /* receive unsupported DHCP messge */
#define DHCP_SERVER_ALT_ENTRYPOOL   2   /* lease a new IP addr             */
#define DHCP_SERVER_ALT_FULLPOOL    3   /* lease addr is nothing           */
#define DHCP_SERVER_ALT_EXPIREPOOL  4   /* lease addr is expired           */
#define DHCP_SERVER_ALT_RELPOOL     5   /* Release resource                */
#define DHCP_SERVER_ALT_DECLINE     6   /* IP Address Conflict Detect      */
#define DHCP_SERVER_ALT_OTHER       10  /* other case                      */


/* DHCP Server Callback Event */
#define DHCP_SERVER_EVT_INIT        1   /* initial complete event */
#define DHCP_SERVER_EVT_RECV        2   /* receive packet */

typedef struct t_dhcp_resource {
    UW ipaddr;          /* Lease IP address     */
    UB mac[6];          /* Reserve MAC address  */
    UW expiration;      /* Lease expiration     */
}T_DHCP_RESOURCE;

typedef void (*DHCP_SERVER_ALT)(ID type, T_DHCP_RESOURCE * entry);

typedef struct t_dhcp_scope {
    struct t_dhcp_scope *next;
    T_DHCP_RESOURCE *pool;
    UB **reserve_mac;   /* reserve MAC address  */
    UW lease_period;    /* lease period unit(sec) */
    T_DHCPD_MSG_OPTION *other_opt;

    UW starting_addr;   /* Starting IP pool     */
    UW subnet;          /* Subnet               */
    UW gateway;         /* Gateway              */
    UB lease_num;       /* Number of lease IP   */
    UB reserve_num;     /* reserve MAC number   */
    UB pcidx;           /* Pool Current Index (same lease_num type) */
} T_DHCP_SCOPE;



typedef struct t_dhcp_server {
    T_DHCP_SCOPE *scope;        /* Lease Scope */
    T_DHCP_SCOPE *current_sp;   /* Current Scope */

    T_DHCP_CTL ctl;         /* some member are't used */
    DHCP_SERVER_ALT alt;    /* callback function for alert */
    ER (*evt)(struct t_dhcp_server *dhcp, UB evt);      /* callback function for event */

    T_DHCP_RESOURCE pool_mst[DHCP_POOL_SIZE];

    ID mpf_id;
    ID mbx_id;
    ID tsk_id;          /* dhcp rcv task id                                  */
    ID flg_id;
    ID server_tsk_id;   /* dhcp server task id                               */
                        /* dhcp other options                                */

    SID sid;
    UB dev_num;         /* Interface number     */
    UB server_tsk_stat; /* dhcp server status flag                           */
    H  lease_pcidx;
}T_DHCP_SERVER;

typedef struct t_dhcp_mbx_msg{
    T_MSG *next;            /* next message in mbx  */
    T_DHCPD_MSG dhcp_msg;   /* DHCP message         */
}T_DHCP_MBX_MSG;

/* DHCP Server API's */
ER dhcp_server(T_DHCP_SERVER *dhcp);
ER dhcp_server_stop( UW dev_num, UW retry );


/* DHCP Server API's for Use Callback */
UB* dhcp_opt_get(T_DHCP_SERVER *dhcp, UB opt);

ER dhcpd_chk_scope(T_DHCP_SERVER *dhcp, T_DHCP_SCOPE *chk);
ER dhcpd_add_scope(T_DHCP_SERVER *dhcp, T_DHCP_SCOPE *add, UB chk);
ER dhcpd_del_scope(T_DHCP_SERVER *dhcp, T_DHCP_SCOPE *del);
ER dhcpd_cmp_scope(T_DHCP_SCOPE *sp1, T_DHCP_SCOPE *sp2);

ER dhcpd_change_scope(T_DHCP_SERVER *dhcp, UW ipa);
ER dhcpd_next_lease(T_DHCP_SERVER *dhcp, UW ipa, UB *mac);


void dhcp_rcv_tsk(VP_INT exinf);    /* Task */

/** Global variable definition **/
extern T_DHCP_SERVER *gDHCP_SERVER[];


#ifdef __cplusplus
}
#endif
#endif /* DHCP_SEVER_H */

