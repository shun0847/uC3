/************************************************************************
    TCP/IP Configuration File
    
    Copyright (c) 2022 eForce Co., Ltd. All rights reserved.
    
    2017-06-22: Created
    2022-09-29: Updated IPv6 process.
 ************************************************************************/

#include "kernel.h"
#include "net_hdr.h"
#include "net_def.h"
#include "net_sts.h"
#include "net_cfg.h"
#include <string.h>

/*******************************************
    Define Local IP Address
********************************************/
T_NET_ADR gNET_ADR[] = {
    {
        0x0,            /* Reserved */
        0x0,            /* Reserved */
        0xC0A80169,     /* IP address  (192.168.  1.105) */
        0xC0A80101,     /* Gateway     (192.168.  1.  1) */
        0xFFFFFF00,     /* Subnet mask (255.255.255.  0) */
    }
};

#ifdef IPV6_SUP
UB ip6_manual_m[] = {
    FALSE,                   /* Enable site local address            */
};
UW ip6_rtr_manual_m[][4]= {
    {0xFE800000, 0, 0, 1},  /* Site local router address (FE80::1)  */
};
UW ip6_host_manual_m[][4]= {
    {0xFE800000, 0, 0, 2},  /* Site local host address   (FE80::2)  */
};
UB pref_len_manual_m[] = {
    64,                     /* Site local prefix length             */
};
#endif

/*******************************************
    Define Ethernet Driver Information
********************************************/
extern ER eth_ini(UH dev_num);
extern ER eth_cls(UH dev_num);
extern ER eth_cfg(UH dev_num, UH opt, VP val);
extern ER eth_ref(UH dev_num, UH opt, VP val);
extern ER eth_snd(UH dev_num, T_NET_BUF *pkt);
extern void apl_eth_cbk(UH dev_num, UH evt, VP sts);    /* main.c */

T_NET_DEV gNET_DEV[] = {
    {
        "lan0",            /* Device Name      */
        1,                 /* Device Number    */
        NET_DEV_TYPE_ETH,  /* Device Type      */
        0,                 /* Status           */
        0,                 /* Flags            */
        eth_ini,           /* Device Init      */
        eth_cls,           /* Device Close     */
        eth_cfg,           /* Device Configure */
        eth_ref,           /* Device Status    */
        eth_snd,           /* Device Transmit  */
		apl_eth_cbk,       /* Device Callback  */
        0,
        {{{ 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC }}}, /* MAC Address */
        ETH_HDR_SZ,                                 /* Link Header Size */
        CFG_NET_BUF_OFFSET,                         /* Network buffer data Offset */
        0                  /* Misc */
    }
};

/*******************************************
    uNet3 Resources
********************************************/
UW NET_SOC_MAX = CFG_NET_SOC_MAX;       /* Maximum Number of Sockets */ 
UW NET_TCP_MAX = CFG_NET_TCP_MAX;       /* Maximum Number of TCP Sockets */ 
UW NET_DEV_MAX = CFG_NET_DEV_MAX;       /* Maximum Number of Network Interface */
UW NET_ARP_MAX = CFG_NET_ARP_MAX;       /* Maximum Number of ARP Cache */
UW NET_MGR_MAX = CFG_NET_MGR_MAX;       /* Maximum Number of Multicast Table */
UW NET_IPR_MAX = CFG_NET_IPR_MAX;       /* Maximum Number of IP Reassembly Queue */
UW NET_BUF_SZ  = CFG_NET_BUF_SZ;        /* Network buffer size */
UW NET_STS_RES = CFG_STS_UPD_RES;       /* Update interval of status */
#ifdef IPV6_SUP
UW NET_NEIGH_CACHE = CFG_NEIGH_CACHE;
UW NET_DST_CACHE   = CFG_DST_CACHE;
UW NET_RTR_LST     = CFG_RTR_LST;
UW NET_PRFX_LST    = CFG_PRFX_LST;
UW NET_PMTU_CACHE  = CFG_PMTU_CACHE;
#endif

/*******************************************
    Define TCP Resources
********************************************/
T_NET gNET[CFG_NET_DEV_MAX];            /* Network Interface control block */
T_NET_DEV gNET_DEV[CFG_NET_DEV_MAX];    /* Network Device control block */
T_NET_ARP gNET_ARP[CFG_NET_ARP_MAX];    /* ARP Cache */
T_NET_MGR gNET_MGR[CFG_NET_MGR_MAX];    /* Multicast Group Table */
T_NET_IPR gNET_IPR[CFG_NET_IPR_MAX];    /* IP Reassembly Queue */
T_NET_SOC gNET_SOC[CFG_NET_SOC_MAX];    /* Socket control block */
T_NET_TCP gNET_TCP[CFG_NET_TCP_MAX];    /* TCP control block */
UB gTCP_SND_BUF[CFG_NET_TCP_MAX * CFG_TCP_SND_WND];     /* TCP Tx Buffer */
#ifdef IPV6_SUP
T_NEIGH_CACHE gNEIGHBOR_CACHE[CFG_NEIGH_CACHE]; /* Neighbor Cache */
T_DST_CACHE   gDEST_CACHE[CFG_DST_CACHE];       /* Destination Cache */
T_RTR_LST     gRTR_LST[CFG_RTR_LST];            /* Router Cache */
T_PREFIX_LST  gPRFX_LST[CFG_PRFX_LST];          /* Prefix Cache */
PMTU_CACHE    gPMTU_CACHE[CFG_PMTU_CACHE];      /* Path MTU Cache */
T_NET6_ADR    gNET6_ADR[CFG_NET_DEV_MAX];  /* IPv6 host address */
UB ip6_manual;
UW ip6_rtr_manual[4];
UW ip6_host_manual[4];
UB pref_len_manual;
#endif

/*******************************************
    Initialize TCP/IP Globals
********************************************/
const VP net_inftbl[] = {
    0,                  /* Reserved */
    (VP)gNET_SOC,       /* Base pointer of Socket control block */
    (VP)gNET_TCP,       /* Base pointer of TCP Socket control block */
    (VP)gNET_IPR,       /* Base pointer of IP Reassembly table */
    (VP)gNET_MGR,       /* Base pointer of Multicast Group table */
    (VP)gTCP_SND_BUF,   /* Base pointer of TCP send buffer */
};

/*******************************************
    Define TCP/IP Default Parameters
********************************************/
T_NET_CFG gNET_CFG[]  = {
    {
        TRUE,               /* 1- All Network Interfaces using Same Parameters */
        CFG_PATH_MTU,
        CFG_ARP_RET_CNT,
        CFG_ARP_RET_TMO,
        CFG_ARP_CLR_TMO,
        #ifdef ACD_SUP
        CFG_ARP_PRB_WAI,
        CFG_ARP_PRB_NUM,
        CFG_ARP_PRB_MIN,
        CFG_ARP_PRB_MAX,
        CFG_ARP_ANC_WAI,
        CFG_ARP_ANC_NUM,
        CFG_ARP_ANC_INT,
        #endif
        CFG_IP4_TTL,
        CFG_IP4_TOS,
        CFG_IP4_IPR_TMO,
        CFG_IP4_MCAST_TTL,  /* Default Multicast TTL */
        CFG_IGMP_V1_TMO,
        CFG_IGMP_REP_TMO,
        CFG_TCP_MSS,
        #ifdef IPV6_SUP
        CFG_TCP_MSS_IPV6,
        #endif
        CFG_TCP_RTO_INI,
        CFG_TCP_RTO_MIN,
        CFG_TCP_RTO_MAX,
        CFG_TCP_SND_WND,
        CFG_TCP_RCV_WND,
        CFG_TCP_DUP_CNT,
        CFG_TCP_CON_TMO,
        CFG_TCP_SND_TMO,
        CFG_TCP_CLS_TMO,
        CFG_TCP_CLW_TMO,
        CFG_TCP_ACK_TMO,
        #ifdef KEEPALIVE_SUP
        CFG_TCP_KPA_CNT,
        CFG_TCP_KPA_INT,
        CFG_TCP_KPA_TMO,
        #endif
        CFG_PKT_RCV_QUE,
        CFG_PKT_CTL_FLG,
#ifdef IGMPv3_SUP
        CFG_IGMP_V3_ENABLE,
#endif
        CFG_TCP_RCV_OSQ_MAX,
    }
};

/*******************************************
    Define TCP/IP Kernel resource
********************************************/
const T_CMPF c_net_mpf = {TA_TFIFO, CFG_NET_BUF_CNT, CFG_NET_BUF_SZ, NULL, NULL};
const T_CTSK c_net_tsk = {TA_HLNG, NULL, (FP)net_tim_tsk,  CFG_NET_TSK_PRI,  CFG_NET_TSK_SIZ, NULL, NULL};
const T_CSEM c_net_sem = {TA_TFIFO, 1, 1, NULL};  /* for exclusive control */

/*******************************
     Memory function
 *******************************/
void* net_memset(void *d, int c, SIZE n)
{
#ifdef _KERNEL_MEMSET_
    return _kernel_memset(d, c, n);
#else
    return memset(d, c, n);
#endif
}

void* net_memcpy(void *d, const void *s, SIZE n)
{
#ifdef _KERNEL_MEMCPY_
    return _kernel_memcpy(d, s, n);
#else
    return memcpy(d, s, n);
#endif
}

int net_memcmp(const void *d, const void *s, SIZE n)
{
#ifdef _KERNEL_MEMCMP_
    return _kernel_memcmp(d, s, n);
#else
    return memcmp(d, s, n);
#endif
}

#ifdef STS_SUP
/*******************************************
    Status
********************************************/
/* Network device status */
static T_NET_STS_DEV net_cfg_sts_dev[CFG_NET_DEV_MAX];

/* Interface */
static T_NET_STS_IFS net_cfg_sts_ifs[CFG_NET_DEV_MAX];
static T_NET_STS_IFS net_cfg_sts_ifs_tmp[CFG_NET_DEV_MAX];

/* ARP status */
static T_NET_STS_ARP net_cfg_sts_arp[CFG_NET_ARP_MAX];

/* Socket status */
static T_NET_STS_SOC net_cfg_sts_soc[CFG_NET_SOC_MAX];

/* Status pointer */
static VP net_cfg_sts_ptr[4 + CFG_NET_DEV_MAX];
static VP net_cfg_sts_ptr_tmp[4 + CFG_NET_DEV_MAX];

/* Configuration */
extern void snmp_tcp_cbk(UH dev_num, UH evt, VP sts);

T_NET_STS_CFG gNET_STS_CFG = {
    net_cfg_sts_dev,         /* Network device */
    net_cfg_sts_ifs,         /* Interface table */
    net_cfg_sts_ifs_tmp,     /* Interface table (Temporary) */
    net_cfg_sts_arp,         /* ARP status */
    net_cfg_sts_soc,         /* Socket status */
    net_cfg_sts_ptr,         /* Status table */
    net_cfg_sts_ptr_tmp,     /* Status table (Temporary) */
    0                        /* Callback function for SNMP */
};
#endif


/*-------- Network Protocol stack and Application
 * Initialization-----------------*/

#define ID_NET_DEV 1U /* Network device number */

/*******************************
    Network Initialization
 *******************************/
ER net_setup(void)
{
    ER ercd;

    /* Initialize TCP/IP Stack */
    ercd = net_ini();
    if (ercd != E_OK) {
        return ercd;
    }

    /* Initialize Ethernet Driver */
    ercd = net_dev_ini(ID_NET_DEV);
    if (ercd != E_OK) {
        return ercd;
    }
    (void)dly_tsk(1000U);

#ifdef IPV6_SUP
    /* Initialize IPv6 Stack */
    ercd = net6_ini();
#endif
    return ercd;
}
