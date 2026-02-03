/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    DHCP Client
    Copyright (c)  2008-2023, eForce Co., Ltd. All rights reserved.

    Version Information
      2008.11.30: Created
      2010.12.27: Modified BroadCast Reception at Compact
      2011.05.23: Clear networkbuffer when error occured
      2012.05.08: Set local variables initial value and corrected path for 
                  error cases.
      2012.08.10: Delete unnecessary operator in parsing
      2012.10.02: Modify to avoid use of string libraries.
      2013.04.10: H/W OS supported version
      2014.04.04: Remove line of "#include <stdlib.h>".
      2015.05.20: Add check of invalid option length
      2015.12.14: The socket ID replaced SID types
      2016.07.06: Execute static analysis tool to this source.
      2016.07.07: Suppressed warning of Zynq GCC compiler.
      2016.08.26: Update for H/W OS
      2016.12.06: Improved to receive DHCP messages up to 548 bytes.
      2017.01.26: Changed source address at request transmission to 0.0.0.0.
      2017.07.27: Support 64bit processor
      2019.02.05: Fixed behavior when multiple servers exist
      2021.06.28: Changed net_bufs used when using the API from 2 to 1.
      2022.10.14: Fixed incorrect parsing when 0-length DHCP options.
      2023.06.21: Changed not to receive packets after API processing.
 ***************************************************************************/

#include "kernel.h"
#include "net_hdr.h"
#include "net_def.h"
#include "dhcp_client.h"


#define DHCP_FLG_OFFER      0x0001U
#define DHCP_FLG_ACK        0x0002U
#define DHCP_FLG_NAK        0x0004U

/* Magic Cookie byte data */
#define MAGICCOOKIE_1   99U
#define MAGICCOOKIE_2   130U
#define MAGICCOOKIE_3   83U
#define MAGICCOOKIE_4   99U
#define MAGICCOOKIE_LEN 4U

#define IGNORE_ANOTHERSERVER_NAK    0       /* 1: ignore another server's NAK / 0: receive all NAK */

/* BOOTP udp port number */
#define BOOTP_PORT_SERVER   67U
#define BOOTP_PORT_CLIENT   68U

#define INITIAL_XID         0x13572468U
#define BROAD_CAST_ADDR     0xFFFFFFFFU
#define DHCP_UDP_RQSZ       1     

/* macro */
#define ALIGN_ARRAYSZ(i,j)      ((i)*(((j)+(8-1))&(~(8-1))))


typedef struct t_host_ctl {
    T_HOST_ADDR *addr;
    T_DHCP_MSG  *dhcp_tx_msg;   /* Tx message   */
    T_DHCP_MSG  *dhcp_rx_msg;   /* Rx message   */
    UB  ser_id[4];  /* DHCP server address  */
    UW  xid;        /* Random trans id      */
    UB  *opt_ptr;   /* dhcp_rcv processing   */
    UH  opt_len;    /* dhcp_rcv processing   */
    UH  flg;        /* dhcp_rcv processing   */
}T_HOST_CTL;


static void dhcp_cfg_netaddr(T_HOST_ADDR *dhcp)
{
    T_NET_ADR adr;

    adr.ipaddr  = dhcp->ipaddr;
    adr.mask    = dhcp->subnet;
    adr.gateway = dhcp->gateway;

    (void)net_cfg(dhcp->dev_num, NET_IP4_CFG, (VP)&adr);
}


static UB *dhcp_parse_opt_ptr(UB *ptr, UH len, UB opt)
{
    UB *p;
    UB code;
    UB tar;
    
    tar = 0;
    code = 1;
    /* parse target is DHCPv4 option format: [Code(1)][Len(1)][Data(Len)] */
    /* - option end when [Code] is 0xFF.            */
    /* - option padding when [Code] is 0x00.        */
    /* - If [Len] is 0, [Data] does not exist.      */
    for (p = ptr; p < (&ptr[len]); p++) {
        if (code) {     /* check [Code] */
            if (*p == DHCP_OPT_PAD) {           /* padding */
                continue;
            }
            else if (*p == DHCP_OPT_END) {      /* end */
                p = NULL;
                break;
            }
            tar = (*p == opt) ? 1 : 0 ;
            code = 0;
        }
        else {          /* check [Len] or [Data] */
            if (tar) {
                //p = (0 < *p) ? p + 1 : NULL ;   /* put NULL if there is no valid data */
                p++;   /* Data, or if Len is 0, next Code */
                break;
            }
            p += *p;            /* skip data */
            code = 1;           /* next code */
        }
    }
    
    /* If the pointer position exceeds the option size, return NULL. */
    if (p >= (&ptr[len])) {
        p = NULL;
    }
    
    return p;
}


static ER dhcpc_rcv(T_HOST_CTL *ctl, UH flg)
{
    ER ercd;
    TMO tmo_soc;
    SYSTIM tm1;
    SYSTIM tm2;
    UB *ptr;    
    UB ser_id[4];
    
    (void)ref_soc(ctl->addr->socid, SOC_TMO_RCV, (VP)&tmo_soc);
    (void)get_tim(&tm1);
    
    while (1) {
        (void)get_tim(&tm2);
#ifdef NET_HW_OS
        tm2 = tm2 - tm1;
        if ((SYSTIM)tmo_soc < tm2) {
            tm2 = (SYSTIM)tmo_soc;
        }
        (void)cfg_soc(ctl->addr->socid, SOC_TMO_RCV, (VP)((SYSTIM)tmo_soc - tm2));
#else
        tm2.ltime = tm2.ltime - tm1.ltime;
        if ((UW)tmo_soc < tm2.ltime) {
            tm2.ltime = (UW)tmo_soc;
        }
        (void)cfg_soc(ctl->addr->socid, SOC_TMO_RCV, (VP)((ADDR)(tmo_soc - tm2.ltime)));
#endif

        /* Wait for BOOTREPLY */
        ercd = rcv_soc(ctl->addr->socid, (VP)ctl->dhcp_rx_msg, (UH)sizeof(T_DHCP_MSG));
        if (ercd <= 0) {
            ercd = E_TMOUT;
            break;      /* exit dhcp process */
        }
        else if ((UH)ercd < DHCP_MSG_LEN) {    /* minimum size check */
            continue;
        }
        else {
            /* do nothing */
        }

        if (ctl->dhcp_rx_msg->op != DHCP_OPC_BOOTREPLY) {
            continue;
        }
        if (ctl->dhcp_rx_msg->xid != ctl->dhcp_tx_msg->xid) {
            continue;
        }
        
        ctl->opt_len = (UH)ercd - DHCP_MSG_LEN;
        ctl->opt_ptr = ctl->dhcp_rx_msg->opt + MAGICCOOKIE_LEN;
        ptr = dhcp_parse_opt_ptr(ctl->opt_ptr, ctl->opt_len, DHCP_OPT_SERVERIDENT);
        if (ptr == NULL) {
            continue;
        }
        net_memcpy(ser_id, ptr, sizeof(ser_id));
        
        ptr = dhcp_parse_opt_ptr(ctl->opt_ptr, ctl->opt_len, DHCP_OPT_DHCPMSGTYPE);
        if (ptr == NULL) {
            continue;
        }
        if ((*ptr == DHCP_MSG_OFFER) && (flg & DHCP_FLG_OFFER)) {
            ctl->flg = DHCP_FLG_OFFER;
            break;
        }
        if ((*ptr == DHCP_MSG_ACK) && (flg & DHCP_FLG_ACK)) {
            ctl->flg = DHCP_FLG_ACK;
            break;
        }
        if ((*ptr == DHCP_MSG_NAK) && (flg & DHCP_FLG_NAK)) {
#if IGNORE_ANOTHERSERVER_NAK
            if (0 != net_memcmp(ctl->ser_id, ser_id, sizeof(ser_id))) {
                continue;
            }
#endif
            ctl->flg = DHCP_FLG_NAK;
            break;
        }        
    }
    if (0 < ercd) {
        net_memcpy(ctl->ser_id, ser_id, sizeof(ctl->ser_id));
    }
    
    (void)cfg_soc(ctl->addr->socid, SOC_TMO_RCV, (VP)(ADDR)tmo_soc);
    return ercd;
}


ER dhcp_client(T_HOST_ADDR *addr)
{
    static UW dhcp_xid = INITIAL_XID;
    T_NODE host;
    T_NET *net;
    T_DHCP_MSG *dhcp_msg;
    T_NET_BUF *pkt[2];
    UW yiaddr;
    UB *ptr;
    INT n;
    ER ercd;
    SID sid;
    UB bcflg;
    
    T_HOST_CTL ctl = {0};

    ercd = E_OK;
    if (addr == NULL) {
        ercd = E_PAR;
    }
    else if (addr->dev_num > (UH)NET_DEV_MAX) {
        ercd = E_PAR;
    }
    else if ((addr->socid == 0U) || (addr->socid > (SID)NET_SOC_MAX)) {
        ercd = E_PAR;
    }
    else {
        /* do nothing */
    }
    if (E_OK != ercd) {
        return ercd;
    }

    pkt[0] = NULL;
    pkt[1] = NULL;
    bcflg = 1U;
    sid = 0U;

    do {
        /* Allocate net_buf to DHCP message buffers. */
        n = ALIGN_ARRAYSZ(2, sizeof(T_DHCP_MSG))+ ALIGN_ARRAYSZ(1, sizeof(T_NET_BUF)-2);
        ercd = net_buf_get(&pkt[0], (UW)n, TMO_POL);
        if (ercd != E_OK) {     /* If one net_buf does not have the required size */
            n = sizeof(T_DHCP_MSG)+ ALIGN_ARRAYSZ(1, sizeof(T_NET_BUF)-2);
            ercd = net_buf_get(&pkt[0], (UW)n, TMO_POL);
            if (ercd != E_OK) {
                ercd = E_NOMEM;
                break;      /* exit dhcp process */
            }
            ercd = net_buf_get(&pkt[1], (UW)n, TMO_POL);
            if (ercd != E_OK) {
                ercd = E_NOMEM;
                break;      /* exit dhcp process */
            }
        }

        ctl.addr = addr;
        ctl.dhcp_tx_msg = (T_DHCP_MSG *)(pkt[0]->hdr);
        if (pkt[1] == NULL) {
            ctl.dhcp_rx_msg = ctl.dhcp_tx_msg + 1;
        }
        else {
            ctl.dhcp_rx_msg = (T_DHCP_MSG *)(pkt[1]->hdr);
        }
        dhcp_msg = ctl.dhcp_tx_msg;

        host.num  = (UB)addr->dev_num;
        host.ipa  = INADDR_ANY;
        host.port = BOOTP_PORT_CLIENT;
        host.ver  = IP_VER4;
        sid = addr->socid;

        /* Refer BroadCast Reception       */
        ercd = net_ref((UH)host.num, NET_BCAST_RCV, (VP)&bcflg);
        if (ercd != E_OK) {
            break;      /* exit dhcp process */
        }

        /* Enable the receive queue to allow packet receive. */
        (void)cfg_soc(addr->socid, SOC_UDP_RQSZ, (VP)DHCP_UDP_RQSZ);        
        
        /* Source address set 0.0.0.0 (for some DHCP servers) */
        (void)cfg_soc(addr->socid, SOC_SRC_IP_RESET, (VP)1);
        
        /* Enable BroadCast Reception       */
        if (bcflg == 0U) {
            ercd = net_cfg((UH)host.num, NET_BCAST_RCV, (VP)1);
            if (ercd != E_OK) {
                bcflg = 1U;
                break;      /* exit dhcp process */
            }
        }

        /* Set Remote Host IP Address */
        host.num  = (UB)addr->dev_num;
        host.ipa  = BROAD_CAST_ADDR;
        host.port = BOOTP_PORT_SERVER;
        host.ver  = IP_VER4;
        ercd = con_soc(sid, &host, SOC_CLI);
        if (ercd != E_OK) {
            break;      /* exit dhcp process */
        }

        net = &gNET[(addr->dev_num != 0) ? (addr->dev_num - 1) : addr->dev_num ];
        net_memcpy(addr->mac, net->dev->cfg.eth.mac, sizeof(addr->mac));

        dhcp_xid += net_rand(); /*random*/

        /* DHCP Discover */
        net_memset((char*)dhcp_msg, 0, DHCP_MSG_SND_SZ);
        dhcp_msg->op = DHCP_OPC_BOOTREQ;
        dhcp_msg->htype = DHCP_ETH_TYPE;
        dhcp_msg->hlen  = DHCP_ETH_LEN;
        dhcp_msg->hops  = 0U;
        dhcp_msg->xid   = htonl(dhcp_xid);
        dhcp_msg->secs  = 0U;
        dhcp_msg->flags = htons(DHCP_FLG_BCAST);
        net_memcpy(dhcp_msg->chaddr, addr->mac, sizeof(addr->mac));
        dhcp_msg->ciaddr = 0U;
        dhcp_msg->yiaddr = 0U;
        dhcp_msg->siaddr = 0U;
        dhcp_msg->giaddr = 0U;

        /*MagicCookie 99,130,83,99 */
        n = 0;
        dhcp_msg->opt[n++] = MAGICCOOKIE_1;
        dhcp_msg->opt[n++] = MAGICCOOKIE_2;
        dhcp_msg->opt[n++] = MAGICCOOKIE_3;
        dhcp_msg->opt[n++] = MAGICCOOKIE_4;
        dhcp_msg->opt[n++] = DHCP_OPT_DHCPMSGTYPE;
        dhcp_msg->opt[n++] = 1U;                        /* message type len */
        dhcp_msg->opt[n++] = DHCP_MSG_DISCOVER;
        dhcp_msg->opt[n++] = DHCP_OPT_END;

        ercd = snd_soc(sid, (VP)dhcp_msg, DHCP_MSG_SND_SZ);
        if (ercd <= 0) {
            ercd = E_TMOUT;
            break;      /* exit dhcp process */
        }

        /* DHCP Offer   */
        ercd = dhcpc_rcv(&ctl, DHCP_FLG_OFFER);
        if (0 > ercd) {
            break;      /* exit dhcp process */
        }

        yiaddr = ctl.dhcp_rx_msg->yiaddr;

        /* DHCP Request */
        dhcp_xid = ntohl(ctl.dhcp_rx_msg->xid);
        net_memset((char*)dhcp_msg, 0, DHCP_MSG_SND_SZ);
        dhcp_msg->op    = DHCP_OPC_BOOTREQ;
        dhcp_msg->htype = DHCP_ETH_TYPE;
        dhcp_msg->hlen  = DHCP_ETH_LEN;
        dhcp_msg->hops  = 0U;
        dhcp_msg->xid   = htonl(dhcp_xid);
        dhcp_msg->secs  = 0U;
        dhcp_msg->flags = htons(DHCP_FLG_BCAST);
        net_memcpy(dhcp_msg->chaddr, addr->mac, sizeof(addr->mac));
        dhcp_msg->ciaddr = 0U;
        dhcp_msg->yiaddr = 0U;
        dhcp_msg->siaddr = 0U;
        dhcp_msg->giaddr = 0U;

        /*MagicCookie 99,130,83,99 */
        n = 0;
        dhcp_msg->opt[n++] = MAGICCOOKIE_1;
        dhcp_msg->opt[n++] = MAGICCOOKIE_2;
        dhcp_msg->opt[n++] = MAGICCOOKIE_3;
        dhcp_msg->opt[n++] = MAGICCOOKIE_4;
        dhcp_msg->opt[n++] = DHCP_OPT_DHCPMSGTYPE;
        dhcp_msg->opt[n++] = 1U;                        /* message type len */
        dhcp_msg->opt[n++] = DHCP_MSG_REQUEST;

        dhcp_msg->opt[n++] = DHCP_OPT_PRMLST;
        dhcp_msg->opt[n++] = 6U;        /* option parameter list - item number */
        dhcp_msg->opt[n++] = DHCP_OPT_SUBNET;
        dhcp_msg->opt[n++] = DHCP_OPT_ROUTER;
        dhcp_msg->opt[n++] = DHCP_OPT_DNS;
        dhcp_msg->opt[n++] = DHCP_OPT_IPLEASE;
        dhcp_msg->opt[n++] = DHCP_OPT_RENETM;
        dhcp_msg->opt[n++] = DHCP_OPT_REBITM;

        dhcp_msg->opt[n++] = DHCP_OPT_SERVERIDENT;
        dhcp_msg->opt[n++] = sizeof(ctl.ser_id);
        net_memcpy(&dhcp_msg->opt[n], ctl.ser_id, sizeof(ctl.ser_id));
        n += (INT)sizeof(ctl.ser_id);

        dhcp_msg->opt[n++] = DHCP_OPT_REQIPADDR;
        dhcp_msg->opt[n++] = sizeof(yiaddr);
        net_memcpy(&dhcp_msg->opt[n], (char*)&yiaddr, sizeof(yiaddr));
        n += (INT)sizeof(yiaddr);
        dhcp_msg->opt[n++] = DHCP_OPT_END;

        ercd = snd_soc(sid, (VP)dhcp_msg, DHCP_MSG_SND_SZ);
        if (ercd <= 0) {
            ercd = E_OBJ;
            break;      /* exit dhcp process */
        }

        /* DHCP ACK     */
        ercd = dhcpc_rcv(&ctl, (DHCP_FLG_ACK | DHCP_FLG_NAK));
        if (0 > ercd) {
            break;      /* exit dhcp process */
        }
        if (0U != (ctl.flg & DHCP_FLG_NAK)) {
            ercd = E_OBJ;
            return E_OBJ;
        }        

        /*DHCP Server*/
        ptr = dhcp_parse_opt_ptr(ctl.opt_ptr, ctl.opt_len, DHCP_OPT_SERVERIDENT);
        if (ptr != NULL) {
            addr->dhcp = ip_byte2n((char*)ptr);
        }

        /*Router*/
        ptr = dhcp_parse_opt_ptr(ctl.opt_ptr, ctl.opt_len, DHCP_OPT_ROUTER);
        if (ptr != NULL) {
            addr->gateway = ip_byte2n((char*)ptr);
        }

        /*SubNet Mask*/
        ptr = dhcp_parse_opt_ptr(ctl.opt_ptr, ctl.opt_len, DHCP_OPT_SUBNET);
        if (ptr != NULL) {
            addr->subnet = ip_byte2n((char*)ptr);
        }

        /*DNS*/
        ptr = dhcp_parse_opt_ptr(ctl.opt_ptr, ctl.opt_len, DHCP_OPT_DNS);
        if (ptr != NULL) {
            addr->dns[0] = ip_byte2n((char*)ptr);
        }

        /*IP Lease */
        ptr = dhcp_parse_opt_ptr(ctl.opt_ptr, ctl.opt_len, DHCP_OPT_IPLEASE);
        if (ptr != NULL) {
            addr->lease = ip_byte2n((char*)ptr);
        }
        else {
            addr->lease = (UW)-1;
        }

        /* Renew */
        ptr = dhcp_parse_opt_ptr(ctl.opt_ptr, ctl.opt_len, DHCP_OPT_RENETM);
        if (ptr != NULL) {
            addr->t1 = ip_byte2n((char*)ptr);
        }
        else {
            /* (0.5 * duration of lease) */
            addr->t1 = addr->lease >> 1;
        }

        /* Rebind */
        ptr = dhcp_parse_opt_ptr(ctl.opt_ptr, ctl.opt_len, DHCP_OPT_REBITM);
        if (ptr != NULL) {
            addr->t2 = ip_byte2n((char*)ptr);
        }
        else {
            /* (0.875 * druation of lease) */
            addr->t2 = addr->lease - (addr->lease >> 3);
        }

        addr->ipaddr = htonl(ctl.dhcp_rx_msg->yiaddr);

        dhcp_cfg_netaddr(addr);

        ercd = E_OK;
    } while (0);

    if (bcflg == 0U) {
        (void)net_cfg((UH)host.num, NET_BCAST_RCV, (VP)0);
    }
    (void)cfg_soc(addr->socid, SOC_SRC_IP_RESET, (VP)0);
    (void)cfg_soc(addr->socid, SOC_UDP_RQSZ, (VP)0);

    if (pkt[0] != NULL) {
        net_buf_ret(pkt[0]);
    }
    if (pkt[1] != NULL) {
        net_buf_ret(pkt[1]);
    }

    return ercd;
}

