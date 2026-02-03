/************************************************************************
    MICRO C CUBE / NETWORK
    TCP/IP Configuration File

 ************************************************************************/

#include <string.h>
#include "kernel.h"
#include "uC3sys.h"     /* use _kernel_memxxx() */
#include "net_hdr.h"
#include "net_sts.h"
#include "net_def.h"
#include "net_cfg.h"
#include "sys/socket.h"

#ifndef NULL
#define NULL    ((void*)0)
#endif

/* Network interface ID's */
#define ID_NETIF_DEV1   (1U)
#define USE_NETIF_ENET	(0U)	/* Choosing a device to use with Netif (0:EQOS, else:ENET) */

/*******************************************
          uNet3 resources
********************************************/
UW NET_SOC_MAX = CFG_NET_SOC_MAX; /* Maximum Number of Sockets          */
UW NET_TCP_MAX = CFG_NET_TCP_MAX; /* Maximum Number of TCP Sockets      */
UW NET_DEV_MAX = CFG_NET_DEV_MAX; /* Maximum No. of Network Interface   */
UW NET_ARP_MAX = CFG_NET_ARP_MAX; /* Maximum No. of ARP Cache           */
UW NET_MGR_MAX = CFG_NET_MGR_MAX; /* Maximum No. of Multicast Table     */
UW NET_IPR_MAX = CFG_NET_IPR_MAX; /* Maximum No. of IP Reassembly Queue */
UW NET_BUF_SZ = CFG_NET_BUF_SZ;
UW NET_STS_RES = CFG_STS_UPD_RES; /* Update interval of status */

#ifdef IPV6_SUP
UW NET6_BUF_SZ = CFG_NET6_BUF_SZ;
UW NET_NEIGH_CACHE = CFG_NEIGH_CACHE;
UW NET_DST_CACHE = CFG_DST_CACHE;
UW NET_RTR_LST = CFG_RTR_LST;
UW NET_PRFX_LST = CFG_PRFX_LST;
UW NET_PMTU_CACHE = CFG_PMTU_CACHE;
#endif

/*******************************************
      Define TCP Resources
********************************************/

T_NET gNET[CFG_NET_DEV_MAX];         /* Network Interface control block */
T_NET_DEV gNET_DEV[CFG_NET_DEV_MAX]; /* Network Device control block */
T_NET_ARP gNET_ARP[CFG_NET_ARP_MAX]; /* ARP Cache */
T_NET_MGR gNET_MGR[CFG_NET_MGR_MAX]; /* Multicast Group Table */
T_NET_IPR gNET_IPR[CFG_NET_IPR_MAX]; /* IP Reassembly Queue */
T_NET_SOC gNET_SOC[CFG_NET_SOC_MAX]; /* Socket control block */
T_NET_TCP gNET_TCP[CFG_NET_TCP_MAX]; /* TCP control block */
UB gTCP_SND_BUF[CFG_NET_TCP_MAX * CFG_TCP_SND_WND]; /* TCP Tx Buffer */
#ifdef IPV6_SUP
T_NEIGH_CACHE gNEIGHBOR_CACHE[CFG_NEIGH_CACHE]; /* Neighbor Cache */
T_DST_CACHE gDEST_CACHE[CFG_DST_CACHE];         /* Destination Cache */
T_RTR_LST gRTR_LST[CFG_RTR_LST];                /* Router Cache */
T_PREFIX_LST gPRFX_LST[CFG_PRFX_LST];           /* Prefix Cache */
PMTU_CACHE gPMTU_CACHE[CFG_PMTU_CACHE];         /* Path MTU Cache */
#endif

/*******************************
    Define Local IP Address
********************************/

T_NET_ADR gNET_ADR[] = {
    {0x0, 0x0, 0x0, 0x0, 0x0},
};

#ifdef IPV6_SUP
T_NET6_ADR gNET6_ADR[1];
UB         ip6_manual        = FALSE;
UW         ip6_rtr_manual[]  = {0x00000000, 0x00000000, 0x00000000, 0x00000000};
UW         ip6_host_manual[] = {0x00000000, 0x00000000, 0x00000000, 0x00000000};
UB         pref_len_manual   = 64;
#endif

/*******************************************
      Initialize TCP/IP Globals
********************************************/

const VP net_inftbl[] = {
    (VP)0,
    (VP)gNET_SOC,     /* Base pointer of Socket control block     */
    (VP)gNET_TCP,     /* Base pointer of TCP Socket control block */
    (VP)gNET_IPR,     /* Base pointer of IP Reassembly table      */
    (VP)gNET_MGR,     /* Base pointer of Multicast Group table    */
    (VP)gTCP_SND_BUF, /* Base pointer of TCP send buffer          */
    0,
    0
};

/*******************************************
      Define TCP/IP Default Parameters
********************************************/

T_NET_CFG gNET_CFG[]  = {
{
FALSE,               /* 1- All Network Interfaces using same parameters */
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
},
};

/*******************************************
     Define Driver Information
********************************************/
#if (0 == USE_NETIF_ENET)
extern ER _ddr_imx8_eqos_ini(UH dev_num);
extern ER _ddr_imx8_eqos_cls(UH dev_num);
extern ER _ddr_imx8_eqos_cfg(UH dev_num, UH opt, VP val);
extern ER _ddr_imx8_eqos_ref(UH dev_num, UH opt, VP val);
extern ER _ddr_imx8_eqos_snd(UH dev_num, T_NET_BUF* pkt);

#else
extern ER _ddr_imx8_enet_ini(UH dev_num);
extern ER _ddr_imx8_enet_cls(UH dev_num);
extern ER _ddr_imx8_enet_cfg(UH dev_num, UH opt, VP val);
extern ER _ddr_imx8_enet_ref(UH dev_num, UH opt, VP val);
extern ER _ddr_imx8_enet_snd(UH dev_num, T_NET_BUF* pkt);
#endif

T_NET_DEV gNET_DEV[] = {
#if (0 == USE_NETIF_ENET)
    {
        "eqos",           /* Device Name      */
        ID_NETIF_DEV1,    /* Device Number    */
        NET_DEV_TYPE_ETH, /* Device Type      */
        0U,               /* Status           */
        0U,               /* Flags            */
        _ddr_imx8_eqos_ini,          /* Device Init      */
        _ddr_imx8_eqos_cls,          /* Device Close     */
        _ddr_imx8_eqos_cfg,          /* Device Configure */
        _ddr_imx8_eqos_ref,          /* Device Status    */
        _ddr_imx8_eqos_snd,          /* Device Transmit  */
        NULL,                        /* Device Callback  */
        0U,
        {{{0x12U, 0x34U, 0x56U, 0x78U, 0x9AU, 0x24U}}}, /* MAC Address */
        ETH_HDR_SZ,                                     /* Link Header Size */
        CFG_NET_BUF_OFFSET /* Network buffer data Offset */
    },
#else
    {
        "enet",           /* Device Name      */
        ID_NETIF_DEV1,    /* Device Number    */
        NET_DEV_TYPE_ETH, /* Device Type      */
        0U,               /* Status           */
        0U,               /* Flags            */
        _ddr_imx8_enet_ini,          /* Device Init      */
        _ddr_imx8_enet_cls,          /* Device Close     */
        _ddr_imx8_enet_cfg,          /* Device Configure */
        _ddr_imx8_enet_ref,          /* Device Status    */
        _ddr_imx8_enet_snd,          /* Device Transmit  */
        NULL,                        /* Device Callback  */
        0U,
        {{{0x12U, 0x34U, 0x56U, 0x78U, 0x9AU, 0x25U}}}, /* MAC Address */
        ETH_HDR_SZ,                                     /* Link Header Size */
        CFG_NET_BUF_OFFSET /* Network buffer data Offset */
    },
#endif
};

/*******************************************
     Define TCP/IP Kernel resource
********************************************/
extern void net_tim_tsk(VP_INT exinf);

long long net_mpf_buf[(CFG_NET_BUF_CNT*CFG_NET_BUF_SZ)/sizeof(long long)] __attribute__((aligned(64)));
T_CTSK const c_net_tsk = {TA_HLNG | TA_FPU, NULL, (FP)net_tim_tsk, CFG_NET_TSK_PRI, CFG_NET_TSK_SIZ, (VP)NULL, "net_tim_tsk"};
T_CMPF const c_net_mpf = {TA_TFIFO, CFG_NET_BUF_CNT, CFG_NET_BUF_SZ, (VP)net_mpf_buf, "NET_MPF"};
T_CSEM const c_net_sem = {TA_TFIFO, 1U, 1U, "NET_SEM"};


/*******************************
     memory function
 *******************************/
void* net_memset(void* d, int c, SIZE n)
{
#ifdef _KERNEL_MEMSET_
    return _kernel_memset(d, c, n);
#else
    return memset(d, c, (size_t)n);
#endif
}

void* net_memcpy(void* d, const void* s, SIZE n)
{
#ifdef _KERNEL_MEMCPY_
    return _kernel_memcpy(d, s, n);
#else
    return memcpy(d, s, (size_t)n);
#endif
}

int net_memcmp(const void* d, const void* s, SIZE n)
{
#ifdef _KERNEL_MEMCMP_
    return _kernel_memcmp(d, s, n);
#else
    return memcmp(d, s, (size_t)n);
#endif
}

/* uNet3/BSD resources */
T_UNET3_BSD_SOC    gNET_BSD_SOC[BSD_SOCKET_MAX];
UW tsk_errno[NUM_OF_TASK_ERRNO];

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
static VP net_cfg_sts_ptr[4U + CFG_NET_DEV_MAX];
static VP net_cfg_sts_ptr_tmp[4U + CFG_NET_DEV_MAX];

/* Configuration */
// extern void snmp_tcp_cbk(UH dev_num, UH evt, VP sts);

T_NET_STS_CFG gNET_STS_CFG = {
    net_cfg_sts_dev,     /* Network device */
    net_cfg_sts_ifs,     /* Interface table */
    net_cfg_sts_ifs_tmp, /* Interface table (Temporary) */
    net_cfg_sts_arp,     /* ARP status */
    net_cfg_sts_soc,     /* Socket status */
    net_cfg_sts_ptr,     /* Status table */
    net_cfg_sts_ptr_tmp, /* Status table (Temporary) */
    0U                   /* Callback function for SNMP */
};
#endif


/*******************************
      Initialize Network
 *******************************/
ER net_setup(void)
{
    ER ercd;
    
    /* Initialize TCP/IP Stack */
    ercd = net_ini();
    if (ercd != E_OK) {
        return ercd;
    }

    /* Initialize Network Driver */
    ercd = net_dev_ini(ID_NETIF_DEV1);
    if (ercd != E_OK) {
        return ercd;
    }
    (void)dly_tsk(1000U);

    return ercd;
}
