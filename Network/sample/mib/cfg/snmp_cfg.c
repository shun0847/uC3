/*
    SNMP
    User configuration 
    Copyright (c) 2016-2023, eForce Co., Ltd. All rights reserved.
    
    2016-01-06 Created
    2022-10-27 Add max_oid_dep to snmp_cfg_usr
    2023-05-31 Changed character encoding to UTF8.
*/

#include "kernel.h"
#include "net_hdr.h"
#include "snmp.h"
#include "snmp_cfg.h"

/* Device ID of network interface */
/* 使用するネットワークデバイスのデバイス番号 */
#define NET_DEV_ID    1U

/* Netowrk configuration */
/* SNMPで使用するネットワークデバイスの設定 */
/* デバイス番号 1: SNMPとTrapを有効 */
const T_SNMP_CFG_NET snmp_cfg_net[CFG_SNMP_NET_USE_CNT] = {
    {NET_DEV_ID, 1, 1},    /* Network device 1, SNMP enable, Trap enable  */
};

#if 0
/* Permitted SNMP manager */
/* SNMPのメッセージ(パケット)の受信を許可するマネージャーの登録 */
/* T_NODEのportは0,verはIP_VER4を設定 */
static T_NODE snmp_cfg_mgr_nod_1 = {0/* Port */, IP_VER4, NET_DEV_ID, 0xc0a80165};	/* 192.168.1.101 */
static T_NODE snmp_cfg_mgr_nod_2 = {0/* Port */, IP_VER4, NET_DEV_ID, 0xc0a8016e};	/* 192.168.1.110 */

T_SNMP_CFG_MGR snmp_cfg_mgr[] = {
    {&snmp_cfg_mgr_nod_1},    /* Allowed manager 1 */
    {&snmp_cfg_mgr_nod_2},    /* Allowed manager 2 */
    0
};
#else
/* Allow all SNMP managers */
/* 全てのSNMPのメッセージ(パケット)を受信する（マネージャーの制限なし） */
T_SNMP_CFG_MGR snmp_cfg_mgr[] = {
    {0}
};
#endif

/* Community with access permission */
/* 受信を許可するコミュニティ名とアクセス権 */
static VB snmp_cfg_com_ro[] = "public";
static VB snmp_cfg_com_rw[] = "private";

T_SNMP_CFG_COM snmp_cfg_com[] = {
    {snmp_cfg_com_ro, STS_RO},      /* Read only */
    {snmp_cfg_com_rw, STS_RW},      /* Read and write */
    {0, 0}
};

/* Remote host address for normal trap */
/* 標準トラップの送信先アドレス、コミュニティ名、バージョンの登録 */
static VB snmp_cfg_trp_com_1[] = "public";
static VB snmp_cfg_trp_com_2[] = "public";
/* T_NODEのportは0,verはIP_VER4を設定 */
static T_NODE snmp_cfg_trp_nod_1 = {0/* Port */, IP_VER4, NET_DEV_ID, 0xc0a80165};	/* 192.168.1.101 */
static T_NODE snmp_cfg_trp_nod_2 = {0/* Port */, IP_VER4, NET_DEV_ID, 0xc0a8016e};	/* 192.168.1.110 */

T_SNMP_CFG_TRP snmp_cfg_trp[] = {
    {snmp_cfg_trp_com_1, &snmp_cfg_trp_nod_1, SNMP_VER_V2C, 0 /* 0を設定 */},
    {snmp_cfg_trp_com_2, &snmp_cfg_trp_nod_2, SNMP_VER_V1,  0 /* 0を設定 */},
    {0, 0, 0 ,0}
};

/* Default callback function */
/* ベンダーのMIBオブジェクトに対して、マネージャーからGet Request,
   Get Next Request, Set Requestを受信したときにコールバック関数(
   apl_snmp_cbk_0)を発行 
   ただし、snmp_mib_ven(snmp_mib_cfg.c)でMIBオブジェクトに対して
   個別にコールバック関数を設定した場合、本関数は発行しない */
extern ER apl_snmp_cbk_0(T_SNMP_CFG_CBK_DAT*);

T_SNMP_CFG_CBK snmp_cfg_cbk[] = {
    {apl_snmp_cbk_0},
    {0}
};


/* Do not change the following items */
/* 以下の項目は変更不要 */

/* OS and netowrk configuration */
extern void snmp_rcv_tsk(VP_INT);
extern void snmp_tim_tsk(VP_INT);
extern void snmp_trp_tsk(VP_INT);
const T_CTSK snmp_cfg_os_tsk_rcv = {TA_HLNG | TA_FPU, 0, (FP)snmp_rcv_tsk,  TSK_RCV_PRI, TSK_RCV_STK, 0, 0};
const T_CTSK snmp_cfg_os_tsk_tim = {TA_HLNG | TA_FPU, 0, (FP)snmp_tim_tsk,  TSK_TIM_PRI, TSK_TIM_STK, 0, 0};
const T_CTSK snmp_cfg_os_tsk_trp = {TA_HLNG | TA_FPU, 0, (FP)snmp_trp_tsk,  TSK_TRP_PRI, TSK_TRP_STK, 0, 0};
const T_CSEM snmp_cfg_os_sem_mib = {TA_TFIFO, 1, 1, 0};                 /* Semaphore for MIB */
const T_CSEM snmp_cfg_os_sem_trp = {TA_TFIFO, 1, 1, 0};                 /* Semaphore for trap */
const T_CFLG snmp_cfg_os_flg_sts = {TA_TFIFO | TA_WMUL, 0x0000, 0};     /* Flag for status */
const T_CFLG snmp_cfg_os_flg_trp = {TA_TFIFO | TA_WMUL, 0x0000, 0};     /* Flag for trap */
const T_CMBX snmp_cfg_os_mbx_trp = {TA_TFIFO | TA_MFIFO, 0, 0, 0};      /* Mailbox for trap */

/* User configuration */
const T_SNMP_CFG_USR snmp_cfg_usr = {
    CFG_SNMP_NET_DEV_CNT,    /* Number of network devices */
    CFG_SNMP_NET_USE_CNT,    /* Number of network devices for SNMP */
    CFG_SNMP_UDP_SND_TMO,    /* UDP socket send timeout */
    CFG_SNMP_UDP_RCV_TMO,    /* UDP socket receive timeout */
    CFG_SNMP_SEM_TMO,        /* Semaphore timeput period */
    CFG_SNMP_FNC_FLG,        /* SNMP agent */
    CFG_SNMP_MAX_TRP_CNT,    /* Number of traps at any time */
    CFG_SNMP_VEN_TRP_CNT,    /* Number of vendor traps at any time */
    CFG_SNMP_SND_MSG_LEN,    /* Maximum size of an SNMP message can send */
    CFG_SNMP_RCV_MSG_LEN,    /* Maximum size of an SNMP message can receive */
    CFG_SNMP_MSG_VAR_CNT,    /* Maximum number of variable bindings */
    CFG_SNMP_MIB_NOD_CNT,    /* Number of nodes in the MIB tree */
    CFG_SNMP_MAX_MIB_DEP,    /* Maximum depth of the MIB tree */
    CFG_SNMP_MIB_DAT_LEN,    /* Maximum size of the MIB data */
    CFG_SNMP_GEN_TRP_ENA,    /* Generic trap enabled */
    CFG_SNMP_MAX_OID_DEP     /* Maximum number of OID objects as MIB data */
};

/* Global variables */
#include "snmp_ber.h"
#include "snmp_def.h"
#include "snmp_mac.h"

/* TCP configuration */
const T_SNMP_CFG_TCP snmp_cfg_tcp = {
    CFG_SNMP_MAX_SOC_CNT,   /* Number of sockets */
    CFG_SNMP_MAX_TCP_CNT,   /* Number of TCP sockets */
    SNMP_MAC_ARP_DAT_CNT,   /* Number of ARP data buffer */
    SNMP_MAC_TCP_DAT_CNT    /* Number of TCP data buffer */
};

/* Data buffer */
UW snmp_cfg_buf[SNMP_MAC_BUF_LEN];             /* SNMP */
UW snmp_cfg_tcp_buf[SNMP_MAC_TCP_BUF_LEN];     /* TCP */

