/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    DHCP Client header file
    Copyright (c)  2008-2020, eForce Co., Ltd. All rights reserved.

    Version Information
      2008.11.30: Created
      2011.11.24: Supported RENEW, RELEASE, DECLINE, INFORM 
      2012.10.02: Accepted more than two DNS server address
      2014.04.11: Corrected to "UH" a type of "dev_num".
      2015.05.01: Add the feature of any options reference.
      2015.06.02: Add define DHCP_BOOTP_MIN_LEN
      2015.12.14: The socket ID replaced SID types
      2016.02.10: Add include files for warning avoidance
      2016.02.26: Change the processing of dhcp_renew() function, 
                  and add a dhcp_rebind() fundtion.
      2016.07.06: Execute static analysis tool to this source.
      2016.08.26: Rename the structure name from T_DHCP_CTL to T_DHCPC_CTL.
      2019.04.10: Enabled "secs" field of DHCP frame.
      2020.01.30: Fixed processing of T_DHCP_UOPT macro.
      2021.06.28: Renamed some member variables for T_DHCPC_CTL.
 ***************************************************************************/

#ifndef DHCP_CLIENT_H
#define DHCP_CLIENT_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "kernel.h"
#include "net_hdr.h"

/*----DHCP Configurables------------------------------------*/
#define DHCP_SND_TMO    5000U   /* UDP Tx time in sec   */
#define DHCP_RCV_TMO    5000U   /* UDP Rx time in sec   */
#define DHCP_RETRY_CNT     3U   /* DHCP msg retry count */
    
#define ENA_DHCP_UOPT           /* Enabled user option feature */
#define ENA_FLDS_SECS           /* Enabled secs fields */

/*----DHCP Configurables------------------------------------*/

/* DHCP UDP Port                */

#ifndef DHCP_SERVER_PORT
#define DHCP_SERVER_PORT        67U
#endif
#ifndef DHCP_CLIENT_PORT
#define DHCP_CLIENT_PORT        68U
#endif    

/* DHCP State                   */

#define DHCP_STS_INIT           0U
#define DHCP_STS_INITREBOOT     1U
#define DHCP_STS_REBOOTING      2U
#define DHCP_STS_REQUESTING     3U
#define DHCP_STS_BOUND          4U
#define DHCP_STS_SELECTING      5U
#define DHCP_STS_REBINDING      6U
#define DHCP_STS_RENEWING       7U

/* DHCP Messsage */

typedef struct t_dhcp_msg {
    UB      op;
    UB      htype;
    UB      hlen;
    UB      hops;
    UW      xid;
    UH      secs;
    UH      flags;
    UW      ciaddr;
    UW      yiaddr;
    UW      siaddr;
    UW      giaddr;
    char    chaddr[16];
    char    sname[64];
    char    file[128];
    UB      opt[312];
}T_DHCP_MSG;

#define DHCP_HDR_LEN        236U
#define DHCP_BOOTP_MIN_LEN  300U         /* minimal length of BOOTP message   */
#define DHCP_MSG_SZ         548U

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
#define DHCP_MSG_INFORM     8U

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
#define DHCP_OPT_END            255U /*:1*/


#ifdef ACD_SUP
#define ARP_CHECK_OFF       0U
#define ARP_CHECK_ON        1U
#endif

#define DNS_SERVER_NUM      2U

typedef struct t_dhcpc_ctl {
    UW  server;     /* DHCP server address  */
    UW  xid;        /* Random trans id      */
    UB  *opt_ptr;   /* rcv_msg processing   */
    UH  opt_len;    /* rcv_msg processing   */
    UH  flg;        /* rcv_msg processing   */
    T_DHCP_MSG  *dhcp_tx_msg;   /* Tx message   */
    T_DHCP_MSG  *dhcp_rx_msg;   /* Rx message   */
    T_NET_BUF   *pkt[2];    /* top address  */
}T_DHCPC_CTL;

#ifdef ENA_DHCP_UOPT
#define DHCP_UOPT_STR       0x80U   /* string */
#define DHCP_UOPT_IPA       0x40U   /* address */
#define DHCP_UOPT_BIN       0x20U   /* binary */

#define DHCP_UOPT_STS_SET   0x01U   /* set option */

/* User settins structure */
typedef struct t_dhcp_uopt {
    UB code;
    UB len;
    UB ary;
    UB flag;
    VP val;
} T_DHCP_UOPT;

extern T_DHCP_UOPT *_gUOPT;     /* Temporary variables for macros */

/* User settings macros */
#define SET_DHCP_UOPT(_uopt_,_code_,_pval_,_len_,_ary_,_type_)                              \
    do {                                                                                    \
        loc_cpu();                                                                          \
        _gUOPT = &(_uopt_);                                                                 \
        _gUOPT->len  = (_len_);    _gUOPT->ary  = (_ary_);    _gUOPT->flag = (_type_);      \
        _gUOPT->code = (_code_);   _gUOPT->val  = (_pval_);                                 \
        unl_cpu();                                                                          \
    } while (0);
    
/* for single element */
#define SET_DHCP_UOPT_BIN1(_uopt_,_code_,_pval_)        SET_DHCP_UOPT((_uopt_),(_code_),(_pval_),1,1,DHCP_UOPT_BIN)
#define SET_DHCP_UOPT_BIN2(_uopt_,_code_,_pval_)        SET_DHCP_UOPT((_uopt_),(_code_),(_pval_),2,1,DHCP_UOPT_BIN)
#define SET_DHCP_UOPT_BIN4(_uopt_,_code_,_pval_)        SET_DHCP_UOPT((_uopt_),(_code_),(_pval_),4,1,DHCP_UOPT_BIN)
#define SET_DHCP_UOPT_IPA(_uopt_,_code_,_pval_)         SET_DHCP_UOPT((_uopt_),(_code_),(_pval_),4,1,DHCP_UOPT_IPA)
#define SET_DHCP_UOPT_STR(_uopt_,_code_,_buf_,_len_)    SET_DHCP_UOPT((_uopt_),(_code_),(_buf_),1,(_len_),DHCP_UOPT_STR)

/* for multiple elements */
#define SET_DHCP_UOPTS_BIN1(_uopt_,_code_,_pval_,_len_)     SET_DHCP_UOPT((_uopt_),(_code_),(_pval_),1,(_len_),DHCP_UOPT_BIN)
#define SET_DHCP_UOPTS_BIN2(_uopt_,_code_,_pval_,_len_)     SET_DHCP_UOPT((_uopt_),(_code_),(_pval_),2,(_len_),DHCP_UOPT_BIN)
#define SET_DHCP_UOPTS_BIN4(_uopt_,_code_,_pval_,_len_)     SET_DHCP_UOPT((_uopt_),(_code_),(_pval_),4,(_len_),DHCP_UOPT_BIN)
#define SET_DHCP_UOPTS_IPA(_uopt_,_code_,_pval_,_len_)      SET_DHCP_UOPT((_uopt_),(_code_),(_pval_),4,(_len_),DHCP_UOPT_IPA)
    
#endif

typedef struct t_dhcp_client {
    T_DHCPC_CTL ctl;
    UW ipaddr;          /* Host addres          */
    UW subnet;          /* Subnet               */
    UW gateway;         /* Gateway              */
    UW dhcp_server;     /* DHCP server address  */
    UW dns[DNS_SERVER_NUM];
                        /* DNS  server address  */
    UW lease;           /* IP Lease time        */
    UW t1;              /* Renew  Time          */
    UW t2;              /* Rebind Time          */
    UB mac[6];          /* Interface HW address */
    UH dev_num;         /* Interface number     */
    SID socid;          /* UDP socket           */
#ifdef ENA_FLDS_SECS
    UW base_ltime;
    UH secs;            /* Seconds elapsed      */
    UH exit_secs;       /* Exit Seconds elapsed */
#endif
#ifdef ENA_DHCP_UOPT
    T_DHCP_UOPT *uopt;  /* DHCP user options */
    UB uopt_len;
#endif
#ifdef ACD_SUP
    UB arpchk;          /* Do ARP Check*/
#endif
    UB state;           /* DHCP client state    */
    UB retry_cnt;       /* DHCP msg retry count */
}T_DHCP_CLIENT;

/* DHCP Client API's */
ER dhcp_bind(T_DHCP_CLIENT *dhcp);
ER dhcp_renew(T_DHCP_CLIENT *dhcp);
ER dhcp_rebind(T_DHCP_CLIENT *dhcp);
ER dhcp_reboot(T_DHCP_CLIENT *dhcp);
ER dhcp_release(T_DHCP_CLIENT *dhcp);
ER dhcp_inform(T_DHCP_CLIENT *dhcp);

/* API (older version) */
#define T_HOST_ADDR T_DHCP_CLIENT
ER dhcp_client(T_DHCP_CLIENT *dhcp);

#ifdef __cplusplus
}
#endif
#endif /* DHCP_CLIENT_H */

