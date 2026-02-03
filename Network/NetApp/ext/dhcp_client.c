/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    DHCP Client
    Copyright (c)  2008-2024, eForce Co., Ltd. All rights reserved.

    Version Information
      2008.11.30: Created
      2010.12.27: Modified BroadCast Reception at Compact
      2011.11.24: Supported RENEW, RELEASE, DECLINE, INFORM 
      2012.07.11: Set limited retry number of reception
      2012.08.06: Allowed to call dhcp_release() any status
      2012.10.02  Modify to avoid use of string libraries.
      2012.10.02: Accepted more than two DNS server address
      2014.04.04: Remove line of "#include <stdlib.h>".
      2015.05.01: Add the feature of any options reference.
      2015.05.20: Add check of invalid option length
      2015.06.02: Bugfix. DHCP packet is at least as big as a BOOTP packet.
      2015.12.14: The socket ID replaced SID types
      2016.02.26: Change the processing of dhcp_renew() function, 
                  and add a dhcp_rebind() fundtion.
      2016.07.06: Execute static analysis tool to this source.
      2016.11.04: Fixed the return value of dhcp_inform() function, 
      2016.12.12: Improvement to suppress warning of analysis tool.
      2017.01.26: Changed source address at request transmission to 0.0.0.0
                   when DHCP state aren't REBIND and RENEW.
      2017.03.31: Update for H/W OS
      2017.07.27: Support 64bit processor
      2018.05.17: Added processing to discard received packets before sending.
                  Added definition to ignore NAK other than selected server.
                  (default setting is disable. IGNORE_ANOTHERSERVER_NAK = 0)
      2019.04.10: Enabled "secs" field of DHCP frame.
      2020.01.30: Fixed processing of T_DHCP_UOPT macro.
      2021.06.28: Changed net_bufs used when using the API from 2 to 1.       
      2022.10.14: Fixed incorrect parsing when 0-length DHCP options.
      2023.06.21: Changed not to receive packets after API processing.
      2024.03.22: Activate network interface when address conflict detected.
 ***************************************************************************/

#include "kernel.h"
#include "net_hdr.h"
#include "net_def.h"
#include "net_strlib.h"

#include "dhcp_client.h"


#define DHCP_FLG_OFFER      0x0001U
#define DHCP_FLG_ACK        0x0002U
#define DHCP_FLG_NAK        0x0004U

#define BROAD_CAST_ADDR     0xFFFFFFFFU
#define LEN_IPADDR          (sizeof(UW))
#define LEN_MAC             6U
#define LEN_MAGICCOOKIE     4U

/* default number of request parameter list */     
#define DEF_NUM_PRMLST_INF      3U   /* INFORM message */
#define DEF_NUM_PRMLST_REQ      6U   /* DISCOVER or REQUEST message */

/* Magic Cookie byte data */
#define MAGICCOOKIE_1   99U
#define MAGICCOOKIE_2   130U
#define MAGICCOOKIE_3   83U
#define MAGICCOOKIE_4   99U
#define MAGICCOOKIE_LEN 4U

#define IGNORE_ANOTHERSERVER_NAK    0       /* 1: ignore another server's NAK / 0: receive all NAK */

#define DHCP_UDP_RQSZ       1   

/* macro */
#define ALIGN_ARRAYSZ(i,j)      ((i)*(((j)+(8-1))&(~(8-1))))


T_DHCP_UOPT *_gUOPT = NULL;

static ER dhcpc_check_prm(T_DHCP_CLIENT *dhcp)
{
    ER ercd;
    
    ercd = E_OK;
    if (dhcp == NULL) {
        ercd = E_PAR;
    }
    else if (dhcp->dev_num > (UH)NET_DEV_MAX) {
        ercd = E_PAR;
    }
    else if ((dhcp->socid == 0U) || (dhcp->socid > (SID)NET_SOC_MAX)) {
        ercd = E_PAR;
    }
    else {
        /* do nothing */
    }
    
    return ercd;
}

static void dhcp_cfg_netaddr(T_DHCP_CLIENT *dhcp)
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


static ER dhcp_soc_cre(T_DHCP_CLIENT *dhcp)
{
    T_NET *net;
    ER ercd;
    UW msg_len;

    dhcp->ctl.pkt[0] = dhcp->ctl.pkt[1] = NULL;

    do {
        /* Allocate net_buf to DHCP message buffers. */
        msg_len = ALIGN_ARRAYSZ(2, sizeof(T_DHCP_MSG)) + ALIGN_ARRAYSZ(1, sizeof(T_NET_BUF)-2);
        ercd = net_buf_get(&dhcp->ctl.pkt[0], msg_len, TMO_POL);
        if (ercd != E_OK) {     /* If one net_buf does not have the required size */
            /* Allocate Send & Receive buffer for DHCP messages */
            msg_len = DHCP_MSG_SZ + ALIGN_ARRAYSZ(1, sizeof(T_NET_BUF)-2);
            ercd = net_buf_get(&dhcp->ctl.pkt[0], msg_len, TMO_POL);
            if (ercd != E_OK) {
                ercd = E_NOMEM;
                break;
            }
            ercd = net_buf_get(&dhcp->ctl.pkt[1], msg_len, TMO_POL);
            if (ercd != E_OK) {
                ercd = E_NOMEM;
                break;
            }
        }
        
        dhcp->ctl.dhcp_tx_msg = (T_DHCP_MSG *)(dhcp->ctl.pkt[0]->hdr);
        if (dhcp->ctl.pkt[1] == NULL) {
            dhcp->ctl.dhcp_rx_msg = dhcp->ctl.dhcp_tx_msg + 1;
        }
        else {
            dhcp->ctl.dhcp_rx_msg = (T_DHCP_MSG *)(dhcp->ctl.pkt[1]->hdr);
        }

        /* Read interface MAC address */
        if (dhcp->dev_num == 0U) {
            net = &gNET[0];
        }
        else {
            net = &gNET[dhcp->dev_num - 1];
        }
        net_memcpy(dhcp->mac, net->dev->cfg.eth.mac, LEN_MAC);
        
        /* Enable the receive queue to allow packet receive. */
        (void)cfg_soc(dhcp->socid, SOC_UDP_RQSZ, (VP)DHCP_UDP_RQSZ);        
        
        ercd = E_OK;
    } while(0);
    

    return ercd;
}

static void dhcp_soc_del(T_DHCP_CLIENT *dhcp)
{
    (void)cfg_soc(dhcp->socid, SOC_UDP_RQSZ, (VP)0);
    
    if (0 != dhcp->ctl.pkt[0]) {
        net_buf_ret(dhcp->ctl.pkt[0]);
    }
    if (0 != dhcp->ctl.pkt[1]) {
        net_buf_ret(dhcp->ctl.pkt[1]);
    }
}

#ifdef ENA_DHCP_UOPT
static UB dhcp_set_prmlst_uopt(const T_DHCP_CLIENT *dhcp, UB *buf)
{
    UB idx;
    
    for (idx = 0U; idx < dhcp->uopt_len; ++idx) {
        *buf++ = dhcp->uopt[idx].code;
    }
    
    return idx;     /* return write count */
}

static ER dhcp_get_uopt(const T_DHCP_CLIENT *dhcp, UB *opt_ptr, UH opt_len)
{
    T_DHCP_UOPT *uopt;
    UB idx;
    UB *n1;
    UH *n2;
    UW *n4;
    UB *ptr;
    UB ary;
    
    if (opt_len == 0U) {
        return E_PAR;
    }
    
    /* User setting option */
    for (idx = 0U; idx < dhcp->uopt_len; ++idx) {
        uopt = &dhcp->uopt[idx];
        /* check valid parameter */
        if ((!uopt->len) || (!uopt->ary)) {
            continue;
        }
        /* the parameter value is already get */
        if (0U != (uopt->flag & DHCP_UOPT_STS_SET)) {
            continue;
        }
        
        ptr = dhcp_parse_opt_ptr(opt_ptr, opt_len, uopt->code);
        if (ptr != NULL) {
            /* determination of element count */
            ary = (UB)(*(ptr-1) / uopt->len);
            if (ary > uopt->ary) {
                ary = uopt->ary;
            }
            else {
                uopt->ary = ary;    /* get element count */
            }
            
            /* do processing for each element size */
            switch (uopt->len) {
            case 1U:    /* value is 1 Byte (include string) */
                n1 = uopt->val;
                for (; 0U < ary; --ary) {
                    *n1 = *((UB *)ptr);
                    ++n1;
                    ptr += sizeof(UB);
                }
                break;
                
            case 2U:    /* value is 2 Byte */
                n2 = uopt->val;
                for (; 0U < ary; --ary) {
                    *n2 = ntohs(*((UH *)ptr));
                    ++n2;
                    ptr += sizeof(UH);
                }
                break;
                
            case 4U:    /* value is 4 Byte (include IP address) */
                n4 = uopt->val;
                for (; 0U < ary; --ary) {
                    *n4 = (0U != (uopt->flag & DHCP_UOPT_IPA)) ?
                        ip_byte2n((char*)ptr) : ntohl(*((UW *)ptr));
                    ++n4;
                    ptr += sizeof(UW);
                }
                break;
                
            default:    /* non support value size */
                continue;
            }
            
            /* update parameter status */
            uopt->flag |= DHCP_UOPT_STS_SET;
        }
        else {
            /* not found the specified code in the packet */
        }
    }
    
    return E_OK;
}
#endif

static void dhcp_get_opt(T_DHCP_CLIENT *dhcp)
{
    UH opt_len;
    UB *ptr;
    UB *opt_ptr;
    UH len;
    UH num;

    opt_ptr = dhcp->ctl.opt_ptr;
    opt_len = dhcp->ctl.opt_len;

    /* DHCP Server */
    ptr = dhcp_parse_opt_ptr(opt_ptr, opt_len, DHCP_OPT_SERVERIDENT);
    if (ptr != NULL) {
        dhcp->dhcp_server = ip_byte2n((char*)ptr);
    }

    /* Router */
    ptr = dhcp_parse_opt_ptr(opt_ptr,opt_len, DHCP_OPT_ROUTER);
    if (ptr != NULL) {
        dhcp->gateway = ip_byte2n((char*)ptr);
    }

    /* SubNet Mask */
    ptr = dhcp_parse_opt_ptr(opt_ptr,opt_len, DHCP_OPT_SUBNET);
    if (ptr != NULL) {
        dhcp->subnet = ip_byte2n((char*)ptr);
    }

    /* DNS */
    num = 0U;
    ptr = opt_ptr;
    len = opt_len;
    while (1) {
        ptr = dhcp_parse_opt_ptr(ptr, len, DHCP_OPT_DNS);
        if (ptr != NULL) {
            len = (UH)(*(ptr-1) / LEN_IPADDR);      /* temporary DNS length*/
            while ((num < len) && (num < DNS_SERVER_NUM)) {
                dhcp->dns[num] = ip_byte2n((char*)ptr);
                num++;
                ptr += LEN_IPADDR;
            }

            if (num < DNS_SERVER_NUM) {
                len = opt_len;
                len -= (UH)(ptr - opt_ptr);
                continue;   /* next dns */
            }
        }
        break;
    }
    
#ifdef ENA_DHCP_UOPT
    /* User setting option */
    (void)dhcp_get_uopt(dhcp, opt_ptr, opt_len);
#endif
    
    /* IP Lease */
    ptr = dhcp_parse_opt_ptr(opt_ptr,opt_len, DHCP_OPT_IPLEASE);
    if (ptr != NULL) {
        dhcp->lease = ip_byte2n((char*)ptr);
    }
    else {
        dhcp->lease = (UW)-1;   /* never */
    }

    /* Renew */
    ptr = dhcp_parse_opt_ptr(opt_ptr,opt_len, DHCP_OPT_RENETM);
    if (ptr != NULL) {
        dhcp->t1 = ip_byte2n((char*)ptr);
    }
    else {
        /* (0.5 * duration of lease) */
        dhcp->t1 = dhcp->lease >> 1;
    }

    /* Rebind */
    ptr = dhcp_parse_opt_ptr(opt_ptr,opt_len, DHCP_OPT_REBITM);
    if (ptr != NULL) {
        dhcp->t2 = ip_byte2n((char*)ptr);
    }
    else {
        /* (0.875 * druation of lease) */
        dhcp->t2 = dhcp->lease - (dhcp->lease >> 3);
    }

    dhcp->ipaddr = htonl(dhcp->ctl.dhcp_rx_msg->yiaddr);
}

#ifdef ENA_FLDS_SECS
static UW dhcp_get_ltime(void)
{
    SYSTIM st;
    get_tim(&st);
#ifdef NET_HW_OS
    return (UW)st;
#else
    return st.ltime;
#endif
}

static void init_dhcp_secs(T_DHCP_CLIENT *dhcp)
{
    dhcp->base_ltime = dhcp_get_ltime();
}

static void update_dhcp_secs(T_DHCP_CLIENT *dhcp)
{
    UH secs;
    secs = (dhcp_get_ltime() - dhcp->base_ltime);
    secs = (0 == secs) ? 0 : secs / 1000 ;
    secs += dhcp->secs;
    
    dhcp->ctl.dhcp_tx_msg->secs = secs;
}

static void exit_dhcp_secs(T_DHCP_CLIENT *dhcp)
{
    UH secs;
    secs = (dhcp_get_ltime() - dhcp->base_ltime);
    secs = (0 == secs) ? 0 : secs / 1000 ;
    dhcp->exit_secs = dhcp->secs + secs;
}

#define GET_DHCPC_SECS(x)   ((x)->secs)

#else
#define init_dhcp_secs(x)
#define update_dhcp_secs(x)
#define exit_dhcp_secs(x)
#define GET_DHCPC_SECS(x)       0

#endif


static UH dhcp_set_hdr(T_DHCP_CLIENT *dhcp, UB msg)
{
    T_DHCP_MSG  *dhcp_msg;
    UH n;
#ifdef ENA_DHCP_UOPT    
    UB uopt_len;
#endif

    dhcp_msg = dhcp->ctl.dhcp_tx_msg;

    net_memset((char*)dhcp_msg, 0, DHCP_HDR_LEN);
    dhcp_msg->op = DHCP_OPC_BOOTREQ;
    dhcp_msg->htype = DHCP_ETH_TYPE;
    dhcp_msg->hlen  = DHCP_ETH_LEN;
    dhcp_msg->hops  = 0U;
    dhcp_msg->xid   = htonl(dhcp->ctl.xid);
    if ((msg == DHCP_MSG_DECLINE) || (msg == DHCP_MSG_RELEASE)){
        dhcp_msg->secs  = 0;
        dhcp_msg->flags = 0;
    }
    else {
        dhcp_msg->secs  = GET_DHCPC_SECS(dhcp);
        dhcp_msg->flags = htons(DHCP_FLG_BCAST);
    }
    net_memcpy(dhcp_msg->chaddr, dhcp->mac, sizeof(dhcp->mac));
    dhcp_msg->ciaddr = 0U;                       /* Set Later */
    dhcp_msg->yiaddr = 0U;
    dhcp_msg->siaddr = 0U;
    dhcp_msg->giaddr = 0U;

    /* Options */
    n = 0U;

    net_memset( &dhcp_msg->opt[0], (W)DHCP_OPT_PAD, sizeof(dhcp_msg->opt) );

    dhcp_msg->opt[n++] = MAGICCOOKIE_1;
    dhcp_msg->opt[n++] = MAGICCOOKIE_2;
    dhcp_msg->opt[n++] = MAGICCOOKIE_3;
    dhcp_msg->opt[n++] = MAGICCOOKIE_4;

    dhcp_msg->opt[n++] = DHCP_OPT_DHCPMSGTYPE;
    dhcp_msg->opt[n++] = sizeof(msg);
    dhcp_msg->opt[n++] = msg;

    if (msg == DHCP_MSG_INFORM) {
        dhcp_msg->opt[n++] = DHCP_OPT_PRMLST;
        dhcp_msg->opt[n++] = DEF_NUM_PRMLST_INF;
        dhcp_msg->opt[n++] = DHCP_OPT_SUBNET;
        dhcp_msg->opt[n++] = DHCP_OPT_ROUTER;
        dhcp_msg->opt[n++] = DHCP_OPT_DNS;
#ifdef ENA_DHCP_UOPT
        uopt_len = dhcp_set_prmlst_uopt(dhcp, &dhcp_msg->opt[n]);
        dhcp_msg->opt[n - DEF_NUM_PRMLST_INF - 1U] += uopt_len;
        n += (UH)uopt_len;
#endif
        dhcp_msg->opt[n++] = DHCP_OPT_END;

    } else if ((msg == DHCP_MSG_DISCOVER) || (msg == DHCP_MSG_REQUEST)){
        dhcp_msg->opt[n++] = DHCP_OPT_PRMLST;
        dhcp_msg->opt[n++] = DEF_NUM_PRMLST_REQ;
        dhcp_msg->opt[n++] = DHCP_OPT_SUBNET;
        dhcp_msg->opt[n++] = DHCP_OPT_ROUTER;
        dhcp_msg->opt[n++] = DHCP_OPT_DNS;
        dhcp_msg->opt[n++] = DHCP_OPT_IPLEASE;
        dhcp_msg->opt[n++] = DHCP_OPT_RENETM;
        dhcp_msg->opt[n++] = DHCP_OPT_REBITM;
#ifdef ENA_DHCP_UOPT
        uopt_len = dhcp_set_prmlst_uopt(dhcp, &dhcp_msg->opt[n]);
        dhcp_msg->opt[n - DEF_NUM_PRMLST_REQ - 1U] += uopt_len;
        n += (UH)uopt_len;
#endif
        dhcp_msg->opt[n++] = DHCP_OPT_END;

    } else {
        /* not set option RELEASE, DECLINE */
    }

    return n;
}

static ER dhcp_snd(T_DHCP_CLIENT *dhcp, UW addr, UH len)
{
    T_NODE  host;
    ER ercd;

    /* msg length < minimal BOOTP length                                     */
    if ( len < DHCP_BOOTP_MIN_LEN ) {
        /* set minimal len for BOOTP                                         */
        len = DHCP_BOOTP_MIN_LEN;
    }
    
    (void)cls_soc(dhcp->socid, 0);      /* clear previous response */

    host.num  = (UB)dhcp->dev_num;
    host.ipa  = addr;
    host.port = DHCP_SERVER_PORT;
    host.ver  = IP_VER4;
    ercd = con_soc(dhcp->socid, &host, SOC_CLI);
    if (ercd != E_OK) {
        return ercd;
    }

    update_dhcp_secs(dhcp);

    return snd_soc(dhcp->socid, (VP)dhcp->ctl.dhcp_tx_msg, len);
}

static ER dhcp_rcv(T_DHCP_CLIENT *dhcp, UH flg)
{
    T_DHCP_MSG *rcv_msg;
    T_DHCP_MSG *snd_msg;
    UB *ptr;
    ER ercd;
    TMO tmo_soc;
    SYSTIM tm1;
    SYSTIM tm2;
    UB bcflg;
#if IGNORE_ANOTHERSERVER_NAK
    UW svr_id;
#endif

    snd_msg = dhcp->ctl.dhcp_tx_msg;
    rcv_msg = dhcp->ctl.dhcp_rx_msg;

    /* Refer BroadCast Reception       */
    ercd = net_ref(dhcp->dev_num, NET_BCAST_RCV, (VP)&bcflg);
    if (ercd != E_OK) {
        return ercd;
    }

    /* Enable BroadCast Reception for this interace  */
    if (bcflg == 0U) {
        ercd = net_cfg(dhcp->dev_num, NET_BCAST_RCV, (VP)1);
        if (ercd != E_OK) {
            return ercd;
        }
    }
    
    (void)ref_soc(dhcp->socid, SOC_TMO_RCV, (VP)&tmo_soc);
    (void)get_tim(&tm1);
    
    while (1) {
        (void)get_tim(&tm2);
#ifdef NET_HW_OS
        tm2 = tm2 - tm1;
        if ((SYSTIM)tmo_soc < tm2) {
            tm2 = (SYSTIM)tmo_soc;
        }
        (void)cfg_soc(dhcp->socid, SOC_TMO_RCV, (VP)((SYSTIM)tmo_soc - tm2));
#else
        tm2.ltime = tm2.ltime - tm1.ltime;
        if ((UW)tmo_soc < tm2.ltime) {
            tm2.ltime = (UW)tmo_soc;
        }
        (void)cfg_soc(dhcp->socid, SOC_TMO_RCV, (VP)((ADDR)(tmo_soc - tm2.ltime)));
#endif

        ercd = rcv_soc(dhcp->socid, (VP)rcv_msg, DHCP_MSG_SZ);
        if (ercd <= 0) {
            break;      /* exit dhcp process */
        }
        if (rcv_msg->op != DHCP_OPC_BOOTREPLY) {
            /* Ignore invalid message */
            continue;
        }
        if (rcv_msg->xid != snd_msg->xid) { /* In Network endian */
            /* Ignore invalid message */
            continue;
        }
        if ((UH)ercd <= (DHCP_HDR_LEN + LEN_MAGICCOOKIE)) {
            /* No DHCP Options? Too Short Message */
            continue;
        }

        dhcp->ctl.opt_len = (UH)ercd - (DHCP_HDR_LEN + LEN_MAGICCOOKIE);
        dhcp->ctl.opt_ptr = rcv_msg->opt + LEN_MAGICCOOKIE;
        
#if IGNORE_ANOTHERSERVER_NAK
        ptr = dhcp_parse_opt_ptr(dhcp->ctl.opt_ptr, dhcp->ctl.opt_len, DHCP_OPT_SERVERIDENT);
        if (ptr == NULL) {
            continue;
        }
        net_memcpy((char *)&svr_id, ptr, LEN_IPADDR);
#endif
        
        ptr = dhcp_parse_opt_ptr(dhcp->ctl.opt_ptr, dhcp->ctl.opt_len, DHCP_OPT_DHCPMSGTYPE);
        if (ptr == NULL) {
            continue;
        }

        if ((*ptr == DHCP_MSG_OFFER) && (flg & DHCP_FLG_OFFER)) {
            dhcp->ctl.flg = DHCP_FLG_OFFER;
            break;
        }

        if ((*ptr == DHCP_MSG_ACK) && (flg & DHCP_FLG_ACK)) {
            dhcp->ctl.flg = DHCP_FLG_ACK;
            break;
        }

        if ((*ptr == DHCP_MSG_NAK) && (flg & DHCP_FLG_NAK)) {
#if IGNORE_ANOTHERSERVER_NAK
            if (dhcp->ctl.server != svr_id) {
                continue;
            }
#endif
            dhcp->ctl.flg = DHCP_FLG_NAK;
            break;
        }
    }
    (void)cfg_soc(dhcp->socid, SOC_TMO_RCV, (VP)(ADDR)tmo_soc);

    if (bcflg == 0U) {
        (void)net_cfg(dhcp->dev_num, NET_BCAST_RCV, (VP)0);
    }

    return ercd;
}

static ER dhcp_discover(T_DHCP_CLIENT *dhcp)
{
    ER ercd;
    UH snd_len;
    UB *ptr;
    UB cnt;

    /* Source address set 0.0.0.0 (for some DHCP servers) */
    (void)cfg_soc(dhcp->socid, SOC_SRC_IP_RESET, (VP)1);
    
    /* Construct DHCP Discover message */
    snd_len = dhcp_set_hdr(dhcp, DHCP_MSG_DISCOVER);

    for (cnt = 0U; cnt < dhcp->retry_cnt; cnt++) {
        ercd = dhcp_snd(dhcp, BROAD_CAST_ADDR, snd_len + DHCP_HDR_LEN);
        if (ercd <= 0) {
            break;
        }

        ercd = dhcp_rcv(dhcp, DHCP_FLG_OFFER);
        if (ercd > 0) {
            break;
        }
    }
    
    (void)cfg_soc(dhcp->socid, SOC_SRC_IP_RESET, (VP)0);
    
    if (ercd <= 0) {
        ercd = E_TMOUT;
    }
    else {
        ptr = dhcp_parse_opt_ptr(dhcp->ctl.opt_ptr, dhcp->ctl.opt_len, DHCP_OPT_SERVERIDENT);
        if (ptr == NULL) {
            ercd = E_TMOUT;
        }
    }
    if (ercd > 0) {
        net_memcpy((char *)&dhcp->ctl.server, ptr, LEN_IPADDR);
        dhcp->ipaddr = dhcp->ctl.dhcp_rx_msg->yiaddr;   /* yiaddr - Network order */
        dhcp->state  = DHCP_STS_SELECTING;
        ercd = E_OK;
    }

    return ercd;
}

#ifdef ACD_SUP
static ER dhcp_decline(T_DHCP_CLIENT *dhcp)
{
    ER ercd;
    UH snd_len;
    T_DHCP_MSG  *snd_msg;
    UW ip;

    if (dhcp->ctl.server == 0U) {
        return E_OBJ;
    }

    ercd = dhcp_soc_cre(dhcp);
    if (ercd != E_OK) {
        dhcp_soc_del(dhcp);
        return ercd;
    }

    dhcp->ctl.xid += net_rand();
    snd_len = dhcp_set_hdr(dhcp, DHCP_MSG_DECLINE);
    snd_msg = dhcp->ctl.dhcp_tx_msg;

    /* Set ciaddr 0 */
    snd_msg->ciaddr = 0U;

    snd_msg->opt[snd_len++] = DHCP_OPT_SERVERIDENT;
    snd_msg->opt[snd_len++] = LEN_IPADDR;
    net_memcpy(&snd_msg->opt[snd_len], (char *)&dhcp->ctl.server, LEN_IPADDR);
    snd_len += LEN_IPADDR;

    /* REQ IP */
    snd_msg->opt[snd_len++] = DHCP_OPT_REQIPADDR;
    snd_msg->opt[snd_len++] = LEN_IPADDR;
    ip = htonl(dhcp->ipaddr);
    net_memcpy(&snd_msg->opt[snd_len], (char *)&ip, LEN_IPADDR);    /* yiaddr - Network order */
    snd_len += LEN_IPADDR;

    snd_msg->opt[snd_len++] = DHCP_OPT_END;

    ercd = dhcp_snd(dhcp, BROAD_CAST_ADDR, snd_len + DHCP_HDR_LEN);
    if (ercd > 0) {
        ercd = E_OK;
    }
    else {
        ercd = E_TMOUT;
    }

    dhcp->state = DHCP_STS_INIT;

    dhcp_soc_del(dhcp);
    return ercd;
}
#endif

static ER dhcp_request(T_DHCP_CLIENT *dhcp)
{
    UW server_ip;
    ER ercd;
    UH snd_len;
    UB cnt;
    T_DHCP_MSG  *snd_msg;

    snd_len = dhcp_set_hdr(dhcp, DHCP_MSG_REQUEST);
    snd_msg = dhcp->ctl.dhcp_tx_msg;

    server_ip = BROAD_CAST_ADDR;
    snd_msg->ciaddr = 0U;

    snd_len--;  /* Append more options */
    switch (dhcp->state) {
        case DHCP_STS_INITREBOOT:
            snd_msg->ciaddr = 0U;
            /* REQ IP */
            snd_msg->opt[snd_len++] = DHCP_OPT_REQIPADDR;
            snd_msg->opt[snd_len++] = LEN_IPADDR;
            server_ip = htonl(dhcp->ipaddr);    /* 'server_ip' as temp var */
            net_memcpy(&snd_msg->opt[snd_len], (char *)&server_ip, LEN_IPADDR);
            snd_len += LEN_IPADDR;
            server_ip = BROAD_CAST_ADDR;

            break;
        case DHCP_STS_SELECTING:
            snd_msg->ciaddr = 0U;

            /* Set Server ID */
            snd_msg->opt[snd_len++] = DHCP_OPT_SERVERIDENT;
            snd_msg->opt[snd_len++] = LEN_IPADDR;
            net_memcpy(&snd_msg->opt[snd_len], (char *)&dhcp->ctl.server, LEN_IPADDR);
            snd_len += LEN_IPADDR;

            /* REQ IP */
            snd_msg->opt[snd_len++] = DHCP_OPT_REQIPADDR;
            snd_msg->opt[snd_len++] = LEN_IPADDR;
            net_memcpy(&snd_msg->opt[snd_len], (char *)&dhcp->ipaddr, LEN_IPADDR);    /* yiaddr - Network order */
            snd_len += LEN_IPADDR;

            break;
        case DHCP_STS_REBINDING:
            snd_msg->ciaddr = htonl(dhcp->ipaddr);
            break;
        case DHCP_STS_RENEWING:
            snd_msg->ciaddr = htonl(dhcp->ipaddr);
            server_ip = htonl(dhcp->ctl.server);   /* unicast */
            break;
        default:
            break;
    }

    /* Source address set 0.0.0.0 (for some DHCP servers) */
    if (!((dhcp->state == DHCP_STS_REBINDING) || (dhcp->state == DHCP_STS_RENEWING))) {
        (void)cfg_soc(dhcp->socid, SOC_SRC_IP_RESET, (VP)1);
    }
    
    snd_msg->opt[snd_len++] = DHCP_OPT_END;

    for (cnt = 0U; cnt < dhcp->retry_cnt; cnt++) {
        ercd = dhcp_snd(dhcp, server_ip, snd_len + DHCP_HDR_LEN);
        if (ercd <= 0) {
            break;
        }
        ercd = dhcp_rcv(dhcp, (DHCP_FLG_ACK | DHCP_FLG_NAK));
        if (ercd > 0) {
            break;
        }
    }
    
    (void)cfg_soc(dhcp->socid, SOC_SRC_IP_RESET, (VP)0);
    
    if (ercd <= 0) {
        return E_TMOUT;
    }

    if (0U != (dhcp->ctl.flg & DHCP_FLG_NAK)) {
        /* Refused */
        dhcp->state = DHCP_STS_INIT;
        return E_OBJ;
    }

    dhcp->state = DHCP_STS_BOUND;

    dhcp_get_opt(dhcp);
    return E_OK;
}

ER dhcp_client(T_DHCP_CLIENT *dhcp)
{
    ER ercd;
#ifdef ACD_SUP
    T_NET_ACD acd;
#endif

    if ((dhcp->state != DHCP_STS_INIT) &&
        (dhcp->state != DHCP_STS_INITREBOOT) &&
        (dhcp->state != DHCP_STS_RENEWING) &&
        (dhcp->state != DHCP_STS_REBINDING)) {
        return E_OBJ;
    }
    
    if (dhcp->retry_cnt == 0U) {
        dhcp->retry_cnt = DHCP_RETRY_CNT;
    }

    init_dhcp_secs(dhcp);

    ercd = dhcp_soc_cre(dhcp);
    if (ercd == E_OK) {
        switch (dhcp->state) {
            case DHCP_STS_INIT:
                dhcp->ctl.xid += net_rand();
                ercd = dhcp_discover(dhcp);
                if (ercd == E_OK) {
                    ercd = dhcp_request(dhcp);
                }
                break;
            case DHCP_STS_INITREBOOT:
                dhcp->ctl.xid += net_rand();
                ercd = dhcp_request(dhcp);
                break;
            case DHCP_STS_RENEWING:
                ercd = dhcp_request(dhcp);
                break;
            case DHCP_STS_REBINDING:
                ercd = dhcp_request(dhcp);
                break;
            default:
                break;
        }
        if (ercd == E_OK) {
            dhcp_cfg_netaddr(dhcp);
        }
        else {
            dhcp->state = DHCP_STS_INIT;
        }
    }

    dhcp_soc_del(dhcp);

#ifdef ACD_SUP
    if ((ercd == E_OK) && (dhcp->arpchk == ARP_CHECK_ON)) {
        ercd = net_acd(dhcp->dev_num, &acd);
        if (ercd == E_SYS) {
#ifdef NET_IF_ACTIVE
            (void)net_cfg(dhcp->dev_num, NET_IF_ACTIVE, (VP)1);
#endif
            (void)dhcp_decline(dhcp);
            dhcp->ipaddr = 0U;
            dhcp->subnet = 0U;
            dhcp->gateway = 0U;
            dhcp_cfg_netaddr(dhcp);
            dhcp->state = DHCP_STS_INIT;
        }
    }
#endif
    
    exit_dhcp_secs(dhcp);
    
    return ercd;
}

ER dhcp_bind(T_DHCP_CLIENT *dhcp)
{
    ER ercd;

    ercd = dhcpc_check_prm(dhcp);
    if (ercd == E_OK) {
        dhcp->state = DHCP_STS_INIT;
        ercd = dhcp_client(dhcp);
    }
    return ercd;
}

/* Renew Lease */
ER dhcp_renew(T_DHCP_CLIENT *dhcp)
{
    ER ercd;

    ercd = dhcpc_check_prm(dhcp);
    if (ercd == E_OK) {
        if (dhcp->state != DHCP_STS_BOUND) {
            ercd = E_OBJ;
        }
        else {
            dhcp->state = DHCP_STS_RENEWING;
            ercd = dhcp_client(dhcp);
        }
    }
    
    return ercd;
}

/* Rebinding Lease */
ER dhcp_rebind(T_DHCP_CLIENT *dhcp)
{
    ER ercd;

    ercd = dhcpc_check_prm(dhcp);
    if (ercd == E_OK) {
        if (dhcp->ipaddr == 0U) {
            ercd = E_OBJ;
        }
        else {
            dhcp->state = DHCP_STS_REBINDING;
            ercd = dhcp_client(dhcp);
        }
    }
    return ercd;
}

/* After boot */
ER dhcp_reboot(T_DHCP_CLIENT *dhcp)
{
    ER ercd;

    ercd = dhcpc_check_prm(dhcp);
    if (ercd == E_OK) {
        if (dhcp->ipaddr == 0U) {
            ercd = E_OBJ;
        }
        else {
            dhcp->state = DHCP_STS_INITREBOOT;
            ercd = dhcp_client(dhcp);
        }
    }
    return ercd;
}

ER dhcp_release(T_DHCP_CLIENT *dhcp)
{
    ER ercd;
    UH snd_len;
    T_DHCP_MSG  *snd_msg;

    ercd = dhcpc_check_prm(dhcp);
    if (ercd == E_OK) {
        if (dhcp->ctl.server == 0U) {
            ercd = E_OBJ;
        }
    }
    if (ercd != E_OK) {
        return ercd;
    }
    
    ercd = dhcp_soc_cre(dhcp);
    if (ercd == E_OK) {
        dhcp->ctl.xid += net_rand();
        snd_len = dhcp_set_hdr(dhcp, DHCP_MSG_RELEASE);
        snd_msg = dhcp->ctl.dhcp_tx_msg;

        /* Set ciaddr */
        snd_msg->ciaddr = htonl(dhcp->ipaddr);

        /* Set Server ID */
        snd_msg->opt[snd_len++] = DHCP_OPT_SERVERIDENT;
        snd_msg->opt[snd_len++] = sizeof(dhcp->ctl.server);
        net_memcpy(&snd_msg->opt[snd_len], (char *)&dhcp->ctl.server, sizeof(dhcp->ctl.server));
        snd_len += (UH)sizeof(dhcp->ctl.server);

        snd_msg->opt[snd_len++] = DHCP_OPT_END;

        ercd = dhcp_snd(dhcp, htonl(dhcp->ctl.server), snd_len + DHCP_HDR_LEN);
        if (ercd > 0) {
            ercd = E_OK;
        }
        else {
            ercd = E_TMOUT;
        }

        dhcp->state = DHCP_STS_INIT;
    }
    
    dhcp_soc_del(dhcp);
    return ercd;
}

ER dhcp_inform(T_DHCP_CLIENT *dhcp)
{
    ER ercd;
    UH snd_len;
    UB cnt;
    T_NET_ADR addr;

    ercd = dhcpc_check_prm(dhcp);
    if (ercd == E_OK) {
        ercd = net_ref(dhcp->dev_num, NET_IP4_CFG, &addr);
        if ((ercd != E_OK) || (addr.ipaddr == 0U)) {
            return E_OBJ;
        }
    }
    if (ercd != E_OK) {
        return ercd;
    }
    
    if (dhcp->retry_cnt == 0U) {
        dhcp->retry_cnt = DHCP_RETRY_CNT;
    }
    
    ercd = dhcp_soc_cre(dhcp);
    if (ercd == E_OK) {
        /* Construct DHCP INFORM message */
        snd_len = dhcp_set_hdr(dhcp, DHCP_MSG_INFORM);
        dhcp->ctl.dhcp_tx_msg->ciaddr = htonl(addr.ipaddr);

        if (dhcp->ctl.server == 0U) {
            dhcp->ctl.server = BROAD_CAST_ADDR;
        }

        for (cnt = 0U; cnt < dhcp->retry_cnt; cnt++) {
            ercd = dhcp_snd(dhcp, htonl(dhcp->ctl.server), snd_len + DHCP_HDR_LEN);
            if (ercd <= 0) {
                break;
            }
            ercd = dhcp_rcv(dhcp, (DHCP_FLG_ACK | DHCP_FLG_NAK));
            if (ercd > 0) {
                break;
            }
        }

        if ((ercd > 0) && (dhcp->ctl.flg & DHCP_FLG_ACK)) {
            dhcp_get_opt(dhcp);
        }
        
        if (ercd > 0) {
            ercd = E_OK;
        }
        else {
            ercd = E_TMOUT;
        }
    }
    
    dhcp_soc_del(dhcp);
    return ercd;
}

