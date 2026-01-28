/*
    SNMP
    Main
    Copyright (c) 2014-2023, eForce Co., Ltd. All rights reserved.
    
    2014-03-06 Created
    2015-05-15 Infinite loop for UDP socket was suppressed
    2014-06-10 Size of ifPhysAddress was fixed
    2014-06-18 Compact OS was supported
    2014-07-10 Initialization of local variables
    2014-12-16 Include "net_def.h"
    2015-02-19 Divided trap socket and modified timeout trap process
    2015-04-21 SNMP_REQ_SET_SYS was added
    2015-05-19 Corrected the reception of response for inform
    2015-05-20 Release the resource of trap to be sent asynchronously
    2015-09-08 Fixed synchronization process of packet reception
    2016-01-06 Multiple network devices were supported
    2016-04-18 snmp_add_val_mib_nod and snmp_del_val_mib_nod were added
    2016-04-29 Macro NET_HW_OS was added
    2016-05-06 Bug fixed callback function for integer data types
    2016-05-09 Bug fixed snmp_set_var for undo
    2017-04-17 Bug fixed snmp_get_mib_datp for TYP_OCT_STR are NULL, data is not output
    2017-04-20 Support OID type for private MIB data
    2019-02-25 Support Counter64(SNMP_TYP_CNT64) type for private MIB data.
               Support accessible-for-notify(SNMP_STS_AN) status for private MIB data.
               Fixed an issue not returning an error response.
               Fixed an issue that can not set empty string with set_mib_obj().
    2019-05-07 Added API. (snmp_cfg(), snmp_ref())
               Created an event notification callback.
    2019-12-10 Fixed authenticationFailure trap sending issue.
    2021-02-19 Fixed the process to get the length of Octet String type object.
    2021-02-19 Fixed a problem where BER objects with zero length could not be set.
    2023-05-25 Fixed a problem that some MIB acquisition is illegal under 64bit environment
*/

#include "kernel.h"
#include "net_hdr.h"
#include "net_def.h"
#include "net_sts_id.h"
#include "snmp.h"
#include "snmp_ber.h"
#include "snmp_lib.h"
#include "snmp_def.h"
#include "snmp_mib.h"

#if defined(NET_C_OS)
extern const T_SNMP_CFG_OS snmp_cfg_os;
#endif
#if defined(NET_HW_OS)
extern T_SNMP_CFG_OS snmp_cfg_os;
#endif
extern const T_SNMP_CFG_NET snmp_cfg_net[];
extern const T_SNMP_CFG_USR snmp_cfg_usr;
extern const T_SNMP_CFG_TCP snmp_cfg_tcp;
extern T_SNMP_CFG_MGR snmp_cfg_mgr[];
extern T_SNMP_CFG_COM snmp_cfg_com[];
extern T_SNMP_CFG_TRP snmp_cfg_trp[];
extern T_SNMP_CFG_CBK snmp_cfg_cbk[];
extern UW snmp_cfg_buf[];
extern UW snmp_cfg_tcp_buf[];
extern T_SNMP_MIB_TBL snmp_mib_ven[];
extern T_SNMP_MIB_TBL snmp_mib_ven_val[];
extern const T_SNMP_MIB snmp_mib[];
extern const UH snmp_mib_dat_cnt[SNMP_MIB2_CNT_GRP];
extern T_SNMP_MIB_DAT* snmp_mib_dat[3];
extern const VB snmp_mib_sys_obj_id[];
extern const VB snmp_mib_mib2_pre[];

#if !(defined(NET_C_OS) || defined(NET_HW_OS))
extern const T_CTSK snmp_cfg_os_tsk_rcv;
extern const T_CTSK snmp_cfg_os_tsk_tim;
extern const T_CTSK snmp_cfg_os_tsk_trp;
extern const T_CSEM snmp_cfg_os_sem_mib;
extern const T_CSEM snmp_cfg_os_sem_trp;
extern const T_CFLG snmp_cfg_os_flg_sts;
extern const T_CFLG snmp_cfg_os_flg_trp;
extern const T_CMBX snmp_cfg_os_mbx_trp;
#endif

/* Configuration */
#define FLG_TSK_TMO     1000            /* Timeout for flag */
#define MBX_TRP_TMO     1000            /* Timeout for trap mailbox */
#define SYS_TIM_DLY     (1000 * 60)     /* System timer delay */
#define MIN_REQ_ID      8               /* Minimum request ID */
#define MAX_REQ_ID      214783639       /* Maximum request ID */
#define ERR_SOC_WAI     1000            /* Socket error timeout */

#if !(defined(CFG_SNMP_AUTHFAIL_ONLY_REQ))
#define CFG_SNMP_AUTHFAIL_ONLY_REQ		0
#endif

/* Debug configuration */
#if defined(CFG_SNMP_DBG_ENA)
#define CFG_DBG_ENA    1
#else
#define CFG_DBG_ENA    0
#endif
#if (CFG_DBG_ENA == 1)
#if !defined(CFG_SNMP_DBG_PTF)
extern void apl_put_str(const VB*);
extern void apl_put_dig(UW);
extern void apl_put_hex(UW);
#define dbg_put_str(x)    apl_put_str(x)
#define dbg_put_dig(x)    apl_put_dig(x)
#define dbg_put_hex(x)    apl_put_hex(x)
#else
#include <stdio.h>
#define dbg_put_str(x)    printf(x);
#define dbg_put_dig(x)    printf("%d", x);
#define dbg_put_hex(x)    printf("0x%x", x);
#endif
#else
#define dbg_put_str(x)
#define dbg_put_dig(x)
#define dbg_put_hex(x)
#endif

/* Configuration values */
#define ID_TSK_RCV          snmp_cfg_os.id_tsk_rcv
#define ID_TSK_TIM          snmp_cfg_os.id_tsk_tim
#define ID_TSK_TRP          snmp_cfg_os.id_tsk_trp
#define ID_SEM_MIB          snmp_cfg_os.id_sem_mib
#define ID_SEM_TRP          snmp_cfg_os.id_sem_trp
#define ID_FLG_STS          snmp_cfg_os.id_flg_sts
#define ID_FLG_TRP          snmp_cfg_os.id_flg_trp
#define ID_MBX_TRP          snmp_cfg_os.id_mbx_trp

#define CFG_NET_DEV_CNT     snmp_cfg_usr.net_dev_cnt
#define CFG_NET_USE_CNT     snmp_cfg_usr.net_use_cnt
#define CFG_UDP_SND_TMO     snmp_cfg_usr.udp_snd_tmo
#define CFG_UDP_RCV_TMO     snmp_cfg_usr.udp_rcv_tmo
#define CFG_SEM_TMO         snmp_cfg_usr.sem_tmo
#define CFG_FNC_FLG         snmp_cfg_usr.fnc_flg
#define CFG_MAX_TRP_CNT     snmp_cfg_usr.max_trp_cnt
#define CFG_VEN_TRP_CNT     snmp_cfg_usr.ven_trp_cnt
#define CFG_SND_MSG_LEN     snmp_cfg_usr.snd_msg_len
#define CFG_RCV_MSG_LEN     snmp_cfg_usr.rcv_msg_len
#define CFG_RCV_MSG_LEN     snmp_cfg_usr.rcv_msg_len
#define CFG_MSG_VAR_CNT     snmp_cfg_usr.msg_var_cnt
#define CFG_MIB_NOD_CNT     snmp_cfg_usr.mib_nod_cnt
#define CFG_MAX_MIB_DEP     snmp_cfg_usr.max_mib_dep
#define CFG_MAX_MIB_DAT     snmp_cfg_usr.max_mib_dat
#define CFG_GEN_TRP_ENA     snmp_cfg_usr.gen_trp_ena
#define CFG_MAX_OID_DEP     snmp_cfg_usr.max_oid_dep

#define CFG_TCP_DAT_CNT     snmp_cfg_tcp.tcp_dat_cnt

/* Constant variables */
static const VB snmp_mib_trap_oid[] = "1.3.6.1.6.3.1.1.4.1.0";    /* snmpTrapOID.0 */
static const VB snmp_mib_trap_oid_0[] = "1.3.6.1.6.3.1.1.5.1";    /* coldStart */
static const VB snmp_mib_trap_oid_1[] = "1.3.6.1.6.3.1.1.5.2";    /* warmStart */
static const VB snmp_mib_trap_oid_2[] = "1.3.6.1.6.3.1.1.5.3";    /* linkDown */
static const VB snmp_mib_trap_oid_3[] = "1.3.6.1.6.3.1.1.5.4";    /* linkUp */
static const VB snmp_mib_trap_oid_4[] = "1.3.6.1.6.3.1.1.5.5";    /* authenticationFailure */
static const VB snmp_mib_trap_oid_5[] = "1.3.6.1.6.3.1.1.5.6";    /* epgNeighborLoss */
static const VB* snmp_mib_trap_oid_ptr[] = {                      /* Trap array */
    snmp_mib_trap_oid_0,
    snmp_mib_trap_oid_1,
    snmp_mib_trap_oid_2,
    snmp_mib_trap_oid_3,
    snmp_mib_trap_oid_4,
    snmp_mib_trap_oid_5
};

/* Macros */
#define SNMP_PORT        161    /* SNMP port */
#define TRP_PORT         162    /* Trap port */
#define LEN_INT          4      /* Length of INT,CNT,GAUGE,IP_ADR */
#define BUF_UW_LEN(x)    ((x + (sizeof(UW) - 1)) / sizeof(UW))

/* Semaphore state */
#define SEM_MIB_DIS    0x0000    /* Disable */
#define SEM_MIB_ENA    0x0001    /* Enable */

/* Interface group ID */                                                    
#define MIB2_DAT_ITF_NUM         0   /* ifNumber */
#define MIB2_DAT_ITF_INDEX       0   /* ifIndex */
#define MIB2_DAT_ITF_LAST_CHG    1   /* ifLastChanges */
#define MIB2_DAT_ITF_SPECIFIC    2   /* ifSpecific */
#define MIB2_DAT_ITF_OFFSET      3   /* Offset for network devices */
#define MIB2_IF_OFFSET           3   /* Offset for T_NET_STS_IFS */

/* TCP group ID */
#define MIB2_TCP_OFFSET     7   /* Offset for tcpInErrs */

/* SNMP group ID */
#define MIB2_SNMP_BASE                 (0)
#define MIB2_SNMP_IN_PKT               (MIB2_SNMP_BASE)         /* snmpInPkts */
#define MIB2_SNMP_OUT_PKT              (MIB2_SNMP_BASE + 1)     /* snmpOutPkts */
#define MIB2_SNMP_IN_BAD_VERSION       (MIB2_SNMP_BASE + 2)     /* snmpInBadVersions */
#define MIB2_SNMP_IN_BAD_COM_NAME      (MIB2_SNMP_BASE + 3)     /* snmpInBadCommunityNames */
#define MIB2_SNMP_IN_BAD_COM_USE       (MIB2_SNMP_BASE + 4)     /* snmpInBadCommunityUses */
#define MIB2_SNMP_IN_ASN_PARSE_ERR     (MIB2_SNMP_BASE + 5)     /* snmpInASNParseErrs */
#define MIB2_SNMP_IN_TOO_BIG           (MIB2_SNMP_BASE + 6)     /* snmpInTooBigs */
#define MIB2_SNMP_IN_NO_SUCH_NAME      (MIB2_SNMP_BASE + 7)     /* snmpInNoSuchNames */
#define MIB2_SNMP_IN_BAD_VALUE         (MIB2_SNMP_BASE + 8)     /* snmpInBadValues */
#define MIB2_SNMP_IN_READ_ONLY         (MIB2_SNMP_BASE + 9)     /* snmpInReadOnlys */
#define MIB2_SNMP_IN_GEN_ERR           (MIB2_SNMP_BASE + 10)    /* snmpInGenErrs */
#define MIB2_SNMP_IN_TOTAL_REQ_VAR     (MIB2_SNMP_BASE + 11)    /* snmpInTotalReqVars */
#define MIB2_SNMP_IN_TOTAL_SET_VAR     (MIB2_SNMP_BASE + 12)    /* snmpInTotalSetVars */
#define MIB2_SNMP_IN_GET_REQUEST       (MIB2_SNMP_BASE + 13)    /* snmpInGetRequests */
#define MIB2_SNMP_IN_GET_NEXT          (MIB2_SNMP_BASE + 14)    /* snmpInGetNexts */
#define MIB2_SNMP_IN_SET_REQUEST       (MIB2_SNMP_BASE + 15)    /* snmpInSetRequests */
#define MIB2_SNMP_IN_GET_RESPONSE      (MIB2_SNMP_BASE + 16)    /* snmpInGetResponses */
#define MIB2_SNMP_IN_TRAP              (MIB2_SNMP_BASE + 17)    /* snmpInTraps */
#define MIB2_SNMP_OUT_TOO_BIG          (MIB2_SNMP_BASE + 18)    /* snmpOutTooBigs */
#define MIB2_SNMP_OUT_NO_SUCH_NAME     (MIB2_SNMP_BASE + 19)    /* snmpOutNoSuchNames */
#define MIB2_SNMP_OUT_BAD_VALUE        (MIB2_SNMP_BASE + 20)    /* snmpOutBadValues */
#define MIB2_SNMP_OUT_GEN_ERR          (MIB2_SNMP_BASE + 21)    /* snmpOutGenErrs */
#define MIB2_SNMP_OUT_GET_REQUEST      (MIB2_SNMP_BASE + 22)    /* snmpOutGetRequests */
#define MIB2_SNMP_OUT_GET_NEXT         (MIB2_SNMP_BASE + 23)    /* snmpOutGetNexts */
#define MIB2_SNMP_OUT_SET_REQUEST      (MIB2_SNMP_BASE + 24)    /* snmpOutSetRequests */
#define MIB2_SNMP_OUT_GET_RESPONSE     (MIB2_SNMP_BASE + 25)    /* snmpOutGetResponses */
#define MIB2_SNMP_OUT_TRAP             (MIB2_SNMP_BASE + 26)    /* snmpOutTraps */
#define MIB2_SNMP_OUT_ENA_AUTH_TRAP    (MIB2_SNMP_BASE + 27)    /* snmpEnableAuthenTraps */

/* Trap command flags */
#define TRP_CMD_INV        0x0000     /* Invalid */
#define TRP_CMD_INI        0x0001     /* Initialized */
#define TRP_CMD_SND_TRP    0x0010     /* Send trap */
#define TRP_CMD_SND_INF    0x0020     /* Send inform */
#define TRP_CMD_RES_INF    0x0040     /* Response inform */
#define TRP_CMD_CRE_ENA    0x1000     /* Message created */

/* Trap status */
#define STS_TRP_INI        0x0000U    /* Initialize */
#define STS_TRP_STA_ENA    0x0001U    /* Start trap enable */
#define STS_TRP_STA_SND    0x0100U    /* Start trap sent */

/* Socket status */
#define STS_SOC_INI        0x0000U   /* Initialize */
#define STS_SOC_WBLK       0x0001U   /* Waiting for non-blocking socket */
#define STS_SOC_RCV        0x0002U   /* Received */

/* Event flags */
#define FLG_MOD_STA        0x0001U   /* Module start */
#define FLG_MOD_STP        0x0008U   /* Module stop */
#define FLG_TSK_RCV_STA    0x0010U   /* Receive task start */
#define FLG_TSK_RCV_STP    0x0020U   /* Receive task stop */
#define FLG_TSK_TIM_STA    0x0040U   /* Timer task start */
#define FLG_TSK_TIM_STP    0x0080U   /* Timer task stop */
#define FLG_TSK_TRP_STA    0x0100U   /* Trap task start */
#define FLG_TSK_TRP_STP    0x0200U   /* Trap task stop */

/* Status */
#define STS_INV             0x0000U    /* Invalid */
#define STS_INI             0x0001U    /* Initialized */
#define STS_ENA             0x0004U    /* Enable */
#define STS_DIS             0x0008U    /* Disable */
#define STS_STA_COLD        0x0010U    /* Cold start */
#define STS_STA_WARM        0x0020U    /* Warm start */
#define STS_ERR_SEM         0x1000U    /* Error semaphore */
#define STS_ERR             0x8000U    /* Error */

/* Manager */
typedef struct t_snmp_mgr {
    UB* snd_buf;                        /* Message buffer for send */
    UB* rcv_buf;                        /* Message buffer for receie */
    UB* trp_buf;                        /* Message buffer for trap */
    T_SNMP_MSG_BUF* snd_msg;            /* Message data for send */
    T_SNMP_MSG_V2* rcv_msg;             /* Message data for receive */
    T_SNMP_MSG_BUF* trp_msg;            /* Message data for trap */
    T_SNMP_MSG_VAR* snd_var_buf;        /* Variable bindings buffer for send */
    T_SNMP_MSG_VAR* rcv_var_buf;        /* Variable bindings buffer for receive */
    T_SNMP_MSG_VAR* trp_var_buf;        /* Variable bindings buffer for trap */
    UW* snd_oid_buf;                    /* OID data buffer for send */
    UW* trp_oid_buf;                    /* OID data buffer for trap */
    T_SNMP_TRP_CMD* trp_cmd_buf;        /* Trap command buffer */
    T_SNMP_TRP_CMD* ven_trp_cmd_buf;    /* Vendor trap command buffer */
    UB** var_dat_buf;                   /* Variable data buffer pointer for undo */
    T_SNMP_MIB_NOD** nod_buf;           /* MIB node buffer for set request */
    VB* mib_oid_str_buf;                /* OID string buffer for MIB */
    VB* trp_oid_str_buf;                /* OID string buffer for trap */
    T_SNMP_MIB_TCP_DAT* tcp_dat;        /* TCP variable data buffer */
    VB* mac_dat_buf;                    /* MAC address buffer */
    T_SNMP_UDP_SOC* udp_soc;            /* UDP socket of message and trap */
    UH* trp_sts;                        /* Trap status */
    SYSTIM sys_tim;                     /* System timer */
    INT req_id;                         /* Request ID */
    INT buf_end_id;                     /* Last ID for data buffer */
    UH udp_soc_cnt;                     /* Number of message and trap sockets */
    UH msg_soc_cnt;                     /* Number of message sockets */
    UH trp_soc_cnt;                     /* Number of trap sockets */
    UH snd_buf_len;                     /* Message buffer size for send */
    UH rcv_buf_len;                     /* Message buffer size for receive */
    UH trp_buf_len;                     /* Message buffer size for trap */
    UH oid_str_len;                     /* OID string buffer size */
    UH trp_ena;                         /* Trap enable flag */
    UH mib_sum_grp[SNMP_MIB2_CNT_GRP];  /* Total sum of MIB objects for group */
    UH mib_cnt;                         /* Number of MIB objects */
    UH mib_ven_cnt;                     /* Number of vendor MIB objects */
    UH mib_ven_val_cnt;                 /* Number of vendor variable MIB objects */
    UH sts;                             /* Status */
} T_SNMP_MGR;

/* Variables */
static T_SNMP_MGR snmp_mgr = { 0 };     /* Manager */
#if !(defined(NET_C_OS) || defined(NET_HW_OS))
static T_SNMP_CFG_OS snmp_cfg_os;       /* OS resource ID */
#endif

static ER (*snmp_evtcbk_fnc)(UW,VP) = 0;        /* event notify callback */

/* Conversion macros */
#define mgr    snmp_mgr



ER snmp_evt_cbk(UW evt, VP prm)
{
    ER ercd;
    
    ercd = E_OK;
    if (NULL != snmp_evtcbk_fnc) {
        ercd = snmp_evtcbk_fnc(evt, prm);
    }
    
    return ercd;
}

ER snmp_cfg(UB code, VP val)
{
    ER ercd;
    
    ercd = E_OK;
    switch (code) {
    case SNMP_EVT_CBK:
        snmp_evtcbk_fnc = (ER (*)(UW,VP))val;
        break;
        
    default:
        ercd = E_NOSPT;
        break;
    }
    
    return ercd;
}

ER snmp_ref(UB code, VP val)
{
    ER ercd;
    
    if (NULL == val) {
        ercd = E_PAR;
    }
    else {
        ercd = E_OK;
        switch (code) {
        case SNMP_EVT_CBK:
            *(UW *)(val) = (UW)(ADDR)(VP)snmp_evtcbk_fnc;
            break;
            
        default:
            ercd = E_NOSPT;
            break;
        }
    }
    
    return ercd;
}


#if (CFG_DBG_ENA == 1)
void snmp_dbg_put_trp_buf(void)
{
    UH i;

    dbg_put_str("\r\nTrap buf\r\n");

    dbg_put_str("\r\nNormal trap buf\r\n");
    for (i = 0U; i < CFG_MAX_TRP_CNT; i++) {
        dbg_put_str("  ");
        dbg_put_dig(i);
        dbg_put_str(": ");
        if (mgr.trp_cmd_buf[i].sts == TRP_CMD_INV) {
            dbg_put_str("TRP_CMD_INV");
        } else {
            dbg_put_str("TRP_CMD_ENA");
        }
        dbg_put_str("\r\n");
    }

    dbg_put_str("\r\nVendor trap buf\r\n");
    for (i = 0U; i < CFG_VEN_TRP_CNT; i++) {
        dbg_put_str("  ");
        dbg_put_dig(i);
        dbg_put_str(": ");
        if (mgr.ven_trp_cmd_buf[i].sts == TRP_CMD_INV) {
            dbg_put_str("TRP_CMD_INV");
        } else {
            dbg_put_str("TRP_CMD_ENA");
        }
        dbg_put_str("\r\n");
    }

    return;
}
#endif

ER snmp_sem_wai(ID id)
{
    ER ercd;

    /* Wait for semaphore */

    ercd = twai_sem(id, CFG_SEM_TMO);
    #if (CFG_DBG_ENA == 1)
    if (ercd != E_OK) {
        mgr.sts |= STS_ERR_SEM;
        #if (CFG_DBG_ENA == 1)
        if (id == ID_SEM_MIB) {
            dbg_put_str("Error: twai_sem(ID_SEM_MIB)\r\n");
        } else if (id == ID_SEM_TRP) {
            dbg_put_str("Error: twai_sem(ID_SEM_TRP)\r\n");
        } else {
            dbg_put_str("Error: twai_sem\r\n");
        }
        #endif
    }
    #endif

    return ercd;
}

void snmp_sem_sig(ID id)
{
    /* Release for semaphore */

    sig_sem(id);

    return;
}

INT snmp_soc_idx_trp(UH dev_num)
{
    ER ercd;
    UH i;
    INT idx;

    /* Trap socket index */

    idx = mgr.msg_soc_cnt;
    ercd = E_OBJ;
    for (i = 0U; i < mgr.trp_soc_cnt; i++) {
        if (mgr.udp_soc[idx].dev_num == dev_num) {
            ercd = E_OK;
            break;
        }
        idx++;
    }
    if (ercd != E_OK) {
        idx = -1;
    }

    return idx;
}

static ER snmp_mib_id_itf(UH* id, UH* net_dev_idx)
{
    ER ercd;

    /* Index of MIB and network device for interface group */

    if (id == 0) {
        return E_PAR;
    }
    if (net_dev_idx == 0) {
        return E_PAR;
    }

    ercd = E_OBJ;

    if (*id >= SNMP_MIB2_DEF_CNT_ITF_IDX) {
        *id = *id - SNMP_MIB2_DEF_CNT_ITF_IDX;
        *net_dev_idx = *id / SNMP_MIB2_DEF_CNT_ITF_TBL;
        
        *id = *id % SNMP_MIB2_DEF_CNT_ITF_TBL;
        *id = *id + SNMP_MIB2_DEF_CNT_ITF_IDX;
        ercd = E_OK;
    }

    return ercd;
}

static ER snmp_mib_snmp_inc(UH id, UH flg_sem)
{
    ER ercd;
    UW* dat;

    /* Increment of SNMP mib data */

    dat = (UW*)snmp_mib_dat[SNMP_MIB2_DAT_SNMP];
    if (dat == 0x00) {
        return E_OK;
    }
    if (id >= SNMP_MIB2_DEF_CNT_SNMP) {
        return E_OBJ;
    }

    if (flg_sem != SEM_MIB_DIS) {
        ercd = snmp_sem_wai(ID_SEM_MIB);
        if (ercd != E_OK) {
            return ercd;
        }
    }

    dat[id] = dat[id] + 1;

    if (flg_sem != SEM_MIB_DIS) {
        snmp_sem_sig(ID_SEM_MIB);
    }

    return E_OK;
}

static ER snmp_mib_snmp_add(UH id, UW num, UH flg_sem)
{
    ER ercd;
    UW* dat;

    /* Addition of SNMP mib data */

    dat = (UW*)snmp_mib_dat[SNMP_MIB2_DAT_SNMP];
    if (dat == 0x00) {
        return E_OK;
    }
    if (id >= SNMP_MIB2_DEF_CNT_SNMP) {
        return E_OBJ;
    }

    if (flg_sem != SEM_MIB_DIS) {
        ercd = snmp_sem_wai(ID_SEM_MIB);
        if (ercd != E_OK) {
            return ercd;
        }
    }

    dat[id] = dat[id] + num;

    if (flg_sem != SEM_MIB_DIS) {
        snmp_sem_sig(ID_SEM_MIB);
    }

    return E_OK;
}

static ER snmp_cmp_str(const VB* buf, UH len, const VB* str)
{
    INT i;

    /* Compare strings */

    if (len != snmp_strlen(str)) {
        return E_OBJ;
    }

    for (i = 0; i < len; i++) {
        if (buf[i] != str[i]) {
            return E_OBJ;
        }
    }

    return E_OK;
}

static ER snmp_chk_nod(T_NODE* nod)
{
    ER ercd;
    INT i;

    /* Check for remote node */

    if (snmp_cfg_mgr[0].nod == 0x00) {
        return E_OK;
    }

    ercd = E_OBJ;
    i = 0;
    while (snmp_cfg_mgr[i].nod != 0x00) {
        if ((snmp_cfg_mgr[i].nod->ver == nod->ver) &&
            (snmp_cfg_mgr[i].nod->num == nod->num) &&
            (snmp_cfg_mgr[i].nod->ipa == nod->ipa)) {
            ercd = E_OK;    /* Find manager node */
            break;
        }
        i++;
    }

    return ercd;
}

static ER snmp_chk_com(VB* com, UH len, UB cmd)
{
    ER ercd;
    INT i;

    /* Check for community */

    ercd = E_OBJ;
    i = 0;
    while (snmp_cfg_com[i].str != 0x00) {
        ercd = snmp_cmp_str(com, len, snmp_cfg_com[i].str);
        if (ercd == E_OK) {
            break;
        }
        i++;
    }
    if (ercd != E_OK) {
        snmp_mib_snmp_inc(MIB2_SNMP_IN_BAD_COM_NAME, SEM_MIB_DIS);
        return E_ID;    /* No such community */
    }

    if (cmd == TYP_SET_REQ) {
        if ((snmp_cfg_com[i].sts & SNMP_STS_WO) == 0x00U) {
            ercd = E_OBJ;    /* Write access inhibit */
        }
    } else {
        if ((snmp_cfg_com[i].sts & SNMP_STS_RO) == 0x00U) {
            ercd = E_OBJ;    /* Read access inhibit */
        }
    }
    if (ercd != E_OK) {
        snmp_mib_snmp_inc(MIB2_SNMP_IN_BAD_COM_USE, SEM_MIB_DIS);
    }

    return ercd;
}

static ER snmp_var_cre(T_SNMP_MSG_VAR* snd_var, T_SNMP_MSG_VAR* rcv_var, UB cmd)
{
    ER ercd;
    UH offset;

    /* Create for variable bindings */

    /* Tag */
    ber_set_any(&snd_var->tag, TYP_SEQ);
    /* OID */
    ber_cpy_oid(&snd_var->oid, &rcv_var->oid);
    /* Retrieved an OID BER data from MIB tree */
    offset = (cmd == TYP_GET_NXT) ? 1 : 0;
    ercd = snmp_mib_get_ber_dat_oid(&snd_var->dat, &snd_var->oid, offset);

    return ercd;
}

static ER snmp_var_cre_exc(T_SNMP_MSG_VAR* snd_var, T_SNMP_MSG_VAR* rcv_var, UB exc)
{
    /* Exception value for variable bindings */

    /* Tag */
    ber_set_any(&snd_var->tag, TYP_SEQ);
    /* OID */
    ber_cpy_oid(&snd_var->oid, &rcv_var->oid);
    /* Set exception value */
    ber_set_any((T_SNMP_BER_ANY*)&snd_var->dat, exc);

    return E_OK;
}

static ER snmp_msg_cre_base(T_SNMP_MSG_V2* snd_msg, T_SNMP_MSG_V2* rcv_msg)
{
    /* Create a base message */

    /* Tag */
    ber_set_any(&snd_msg->tag, TYP_SEQ);
    /* Version */
    ber_set_int(&snd_msg->ver, rcv_msg->ver.dat);
    /* Community */
    ber_set_oct(&snd_msg->com, rcv_msg->com.buf, rcv_msg->com.len);

    /* Data */
    ber_set_any(&snd_msg->dat, TYP_GET_RES);
    /* Request ID */
    ber_set_int(&snd_msg->req_id, rcv_msg->req_id.dat);
    /* Error status */
    ber_set_int(&snd_msg->err_sts, ERR_NO_ERR);
    /* Error index */
    ber_set_int(&snd_msg->err_idx, 0);

    /* Variable data tag */
    ber_set_any(&snd_msg->var_tag, TYP_SEQ);

    /* Variable data */
    snd_msg->var_cnt = 0;

    return E_OK;
}

static ER snmp_msg_cre(T_SNMP_MSG_V2* snd_msg, T_SNMP_MSG_V2* rcv_msg)
{
    ER ercd = E_OK;
    UB cmd;
    UH non_rep;
    UH max_rep;
    UH i;
    UH j;
    UH k;
    T_SNMP_BER_OID* ber_oid;
    T_SNMP_MSG_VAR* msg_var;
    UB exc;

    /* Create a response message */

    /* Base message */
    snmp_msg_cre_base(snd_msg, rcv_msg);

    /* Variable data */
    if (rcv_msg->dat.typ == TYP_GET_BULK) {
        cmd = TYP_GET_NXT;
        non_rep = rcv_msg->err_sts.dat;
        max_rep = rcv_msg->err_idx.dat;
    } else {
        cmd = rcv_msg->dat.typ;
        non_rep = rcv_msg->var_cnt;
        max_rep = 0;
    }
    snd_msg->var = mgr.snd_var_buf;
    snd_msg->var_cnt = 0;

    for (i = 0; (non_rep > 0) && (i < rcv_msg->var_cnt); i++) {
        /* Non repeat */
        if (CFG_MSG_VAR_CNT < (snd_msg->var_cnt + 1)) {
            /* Buffer overflow */
            snd_msg->err_sts.dat = ERR_TOO_BIG;
            return E_BOVR;
        }
        ber_oid = (T_SNMP_BER_OID*)&snd_msg->var[snd_msg->var_cnt].dat;
        ber_oid->buf = &mgr.snd_oid_buf[CFG_MAX_MIB_DEP * snd_msg->var_cnt];
        ber_oid->buf_len = CFG_MAX_MIB_DEP;
        
        ercd = snmp_var_cre(&snd_msg->var[snd_msg->var_cnt], &rcv_msg->var[i], cmd);
        if (ercd != E_OK) {
            if ((ercd == E_NOID) || (ercd == E_NOINS) || (ercd == E_QOVR)) {
                /* No such OID or end of MIB view */
                if (rcv_msg->ver.dat == SNMP_VER_V1) {
                    snd_msg->err_sts.dat = ERR_NO_SUCH_NAME;
                    break;
                } else if (rcv_msg->ver.dat == SNMP_VER_V2C) {
                    if (ercd == E_NOID) {
                        exc = TYP_EXC_NO_SUCH_OBJ;
                    } else if (ercd == E_NOINS) {
                        exc = TYP_EXC_NO_SUCH_INS;
                    } else {
                        exc = TYP_EXC_END_MIB_VIEW;
                    }
                    snmp_var_cre_exc(&snd_msg->var[snd_msg->var_cnt], &rcv_msg->var[i], exc);
                    ercd = E_OK;    /* Continue */
                }
            } else {
                /* General error */
                snd_msg->err_sts.dat = ERR_GEN_ERR;
                break;
            }
        } else {
            snmp_mib_snmp_inc(MIB2_SNMP_IN_TOTAL_REQ_VAR, SEM_MIB_DIS);
        }
        snd_msg->var_cnt++;
        non_rep--;
    }

    for (j = 0; (ercd == E_OK) && (i < rcv_msg->var_cnt) && (j < max_rep); j++) {
        for (k = i; k < rcv_msg->var_cnt; k++) {
            /* Repeat */
            if (CFG_MSG_VAR_CNT < (snd_msg->var_cnt + 1)) {
                /* Buffer overflow */
                break;
            }
            ber_oid = (T_SNMP_BER_OID*)&snd_msg->var[snd_msg->var_cnt].dat;
            ber_oid->buf = &mgr.snd_oid_buf[CFG_MAX_MIB_DEP * snd_msg->var_cnt];
            ber_oid->buf_len = CFG_MAX_MIB_DEP;
            
            msg_var = (j == 0) ? &rcv_msg->var[k] : &snd_msg->var[k + ((rcv_msg->var_cnt - i) * (j - 1))];
            ercd = snmp_var_cre(&snd_msg->var[snd_msg->var_cnt], msg_var, cmd);
            if (ercd != E_OK) {
                if (ercd == E_QOVR) {
                    /* end of MIB view */
                    snmp_var_cre_exc(&snd_msg->var[snd_msg->var_cnt], &rcv_msg->var[i], TYP_EXC_END_MIB_VIEW);
                    ercd = E_OK;    /* Continue */
                } else {
                    /* General error */
                    snd_msg->err_sts.dat = ERR_GEN_ERR;
                    break;
                }
            } else {
                snmp_mib_snmp_inc(MIB2_SNMP_IN_TOTAL_REQ_VAR, SEM_MIB_DIS);
            }
            snd_msg->var_cnt++;
        }
    }

    /* Error */
    if (snd_msg->err_sts.dat == ERR_NO_SUCH_NAME) {
        /* No such OID */
        snd_msg->err_idx.dat = i + 1;
        snmp_var_cre_exc(&snd_msg->var[snd_msg->var_cnt], &rcv_msg->var[i], TYP_NULL);
        snd_msg->var_cnt++;
        snmp_mib_snmp_inc(MIB2_SNMP_OUT_NO_SUCH_NAME, SEM_MIB_DIS);
    } else if (snd_msg->err_sts.dat == ERR_GEN_ERR) {
        /* General error */
        snd_msg->err_idx.dat = i + 1;
        if (rcv_msg->ver.dat == SNMP_VER_V1) {
            /* v1 */
            snmp_var_cre_exc(&snd_msg->var[snd_msg->var_cnt], &rcv_msg->var[i], TYP_NULL);
            snd_msg->var_cnt++;
        } else {
            /* v2c */
            snd_msg->var_cnt = 0;
            for (i = 0; i < rcv_msg->var_cnt; i++) {
                if (CFG_MSG_VAR_CNT < snd_msg->var_cnt + 1) {
                    /* Buffer overflow */
                    snd_msg->err_sts.dat = ERR_TOO_BIG;
                    if ((rcv_msg->dat.typ == TYP_GET_BULK) && (rcv_msg->ver.dat == SNMP_VER_V2C)) {
                        return E_OBJ;
                    }
                    return E_BOVR;
                }
                snmp_var_cre_exc(&snd_msg->var[snd_msg->var_cnt], &rcv_msg->var[i], TYP_NULL);
                snd_msg->var_cnt++;
            }
        }
        snmp_mib_snmp_inc(MIB2_SNMP_OUT_GEN_ERR, SEM_MIB_DIS);
    }

    return E_OK;
}

static ER snmp_msg_cre_err(T_SNMP_MSG_V2* snd_msg, T_SNMP_MSG_V2* rcv_msg, INT err_sts)
{
    /* Create a response message for error */

    /* Base message */
    snmp_msg_cre_base(snd_msg, rcv_msg);

    /* Error status */
    ber_set_int(&snd_msg->err_sts, err_sts);

    return E_OK;
}

static ER snmp_set_var(T_SNMP_MSG_V2* snd_msg, T_SNMP_MSG_V2* rcv_msg, INT* err_idx)
{
    ER ercd;
    UH i;
    UH j;

    /* Update OID */

    if (err_idx == 0) {
        return E_OBJ;
    }
    ercd = E_OK;

    /* Check */
    snd_msg->var_cnt = 0;
    for (i = 0; i < rcv_msg->var_cnt; i++) {
        if (CFG_MSG_VAR_CNT < (snd_msg->var_cnt + 1)) {
            /* Buffer overflow */
            snd_msg->err_sts.dat = ERR_TOO_BIG;
            return E_BOVR;
        }
        ercd = snmp_mib_set_ber_dat((T_SNMP_BER*)&rcv_msg->var[i].dat, &rcv_msg->var[i].oid, &mgr.nod_buf[i], 0);
        if (ercd != E_OK) {
            *err_idx = i + 1;
            return ercd;
        }
        snd_msg->var_cnt++;
    }

    /* Update */
    snd_msg->var_cnt = 0;
    for (i = 0; i < rcv_msg->var_cnt; i++) {
        ercd = snmp_mib_set_ber_dat((T_SNMP_BER*)&rcv_msg->var[i].dat, &rcv_msg->var[i].oid, &mgr.nod_buf[i], mgr.var_dat_buf[i]);
        if (ercd != E_OK) {
            *err_idx = i + 1;
            break;
        }
        snd_msg->var_cnt++;
    }

    /* Undone */
    if (ercd != E_OK) {
        snd_msg->var_cnt = 0;
        for (j = 0; j < (i + 1); j++) {
            ercd = snmp_mib_set_dat_oid(mgr.var_dat_buf[j], &rcv_msg->var[j].oid);
            if (ercd != E_OK) {
                *err_idx = 0;
                return E_UNDO;
            }
        }
        return E_COMMIT;
    }

    snmp_mib_snmp_add(MIB2_SNMP_IN_TOTAL_SET_VAR, snd_msg->var_cnt, SEM_MIB_DIS);

    return E_OK;
}

static ER snmp_set_err(T_SNMP_MSG_V2* snd_msg, T_SNMP_MSG_V2* rcv_msg, ER err, INT err_idx)
{
    const UB err_v2tov1[] = {   /* Error Status Mappings */
        /* SNMPv1 */        /* SNMPv2 */
        ERR_NO_ERR,         /* ERR_NO_ERR (noError) */
        ERR_TOO_BIG,        /* ERR_TOO_BIG (tooBig) */
        ERR_NO_SUCH_NAME,   /* ERR_NO_SUCH_NAME (noSuchName) */
        ERR_BAD_VAL,        /* ERR_BAD_VAL (badValue) */
        ERR_READ_ONLY,      /* ERR_READ_ONLY (readOnly) */
        ERR_GEN_ERR,        /* ERR_GEN_ERR (GenError) */
        ERR_NO_SUCH_NAME,   /* ERR_NO_ACS (noAccess) */
        ERR_BAD_VAL,        /* ERR_WRONG_TYP (wrongType) */
        ERR_BAD_VAL,        /* ERR_WRONG_LEN (wrongLength) */
        ERR_BAD_VAL,        /* ERR_WRONG_ECODING (wrongEcoding) */
        ERR_BAD_VAL,        /* ERR_WRONG_VAL (wrongValue) */
        ERR_NO_SUCH_NAME,   /* ERR_NO_CREATION (noCreation) */
        ERR_BAD_VAL,        /* ERR_INCONSIST_VAL (inconsistentValue) */
        ERR_GEN_ERR,        /* ERR_RSRC_UNAVAL (resourceUnavailable) */
        ERR_GEN_ERR,        /* ERR_COMMIT_FAILD (commitFaild) */
        ERR_GEN_ERR,        /* ERR_UNDO_FAILD (undoFaild) */
        ERR_NO_SUCH_NAME,   /* ERR_AUTH_ERR (authorizationError) */
        ERR_NO_SUCH_NAME,   /* ERR_NOT_WRITABLE (notWriteable) */
        ERR_NO_SUCH_NAME    /* ERR_INCONSIST_NAME (inconsistentName) */
    };
    ER ercd;
    INT sts;
    UH i;

    /* Create error message for set-request */

    /* Base message */
    snmp_msg_cre_base(snd_msg, rcv_msg);

    /* Error status (SNMPv2) */
    switch (err) {
        case E_BOVR:
            sts = ERR_TOO_BIG;
            break;
        case E_NOID:
            sts = ERR_NO_SUCH_NAME;
            break;
        case E_RO:
            sts = ERR_NOT_WRITABLE;
            break;
        case E_TYP:
            sts = ERR_WRONG_TYP;
            break;
        case E_LEN:
            sts = ERR_WRONG_LEN;
            break;
        case E_ENC:
            sts = ERR_WRONG_ECODING;
            break;
        case E_DAT:
            sts = ERR_WRONG_VAL;
            break;
        case E_COMMIT:
            sts = ERR_COMMIT_FAILD;
            break;
        case E_UNDO:
            sts = ERR_UNDO_FAILD;
            break;
        case E_NA:
            sts = ERR_NO_ACS;
            break;
        default:
            sts = ERR_GEN_ERR;
            break;
    }
    
    /* Convert error status (SNMPv2 -> SNMPv1) */
    if (rcv_msg->ver.dat == SNMP_VER_V1) {
        if (sts < sizeof(err_v2tov1)) {
            sts = (INT)err_v2tov1[sts];
        }
        else {
            sts = ERR_GEN_ERR;
        }
    }
    
    /* Processing of each error */
    switch (err) {
        case ERR_UNDO_FAILD:
            err_idx = 0;
            break;
        case ERR_GEN_ERR:
            err_idx = 0;
            snmp_mib_snmp_inc(MIB2_SNMP_OUT_GEN_ERR, SEM_MIB_DIS);
            break;
        case ERR_BAD_VAL:
            snmp_mib_snmp_inc(MIB2_SNMP_OUT_BAD_VALUE, SEM_MIB_DIS);
            break;
        default:
            break;
    }
    ber_set_int(&snd_msg->err_sts, sts);

    /* Error index */
    ber_set_int(&snd_msg->err_idx, err_idx);

    /* Variables */
    snd_msg->var = mgr.snd_var_buf;
    snd_msg->var_cnt = 0;
    for (i = 0; i < rcv_msg->var_cnt; i++) {
        /* Tag */
        ber_set_any(&snd_msg->var[i].tag, TYP_SEQ);
        /* OID */
        ercd = ber_cpy_oid(&snd_msg->var[i].oid, &rcv_msg->var[i].oid);
        if (ercd != E_OK) {
            return E_OBJ;
        }
        /* Data */
        ber_cpy_buf((T_SNMP_BER*)&snd_msg->var[i].dat, (T_SNMP_BER*)&rcv_msg->var[i].dat);
        if (ercd != E_OK) {
            return E_OBJ;
        }
        snd_msg->var_cnt++;
    }

    return E_OK;
}

static ER snmp_msg_enc(VP buf, UH buf_len, T_SNMP_MSG_V2* msg)
{
    VP buf_ptr;
    UH enc_len;
    INT i;
    INT len;
    T_SNMP_BER_INT* ber_int[3];

    /* Encode a message */

    if ((buf == 0) || (msg == 0)) {
        return E_OBJ;
    }
    buf_ptr = buf;

    /* Length */

    enc_len = 0;

    /* Variable bindings */
    for (i = 0; i < msg->var_cnt; i++) {
        /* OID */
        len = ber_enc_oid(0, 0, &msg->var[i].oid);
        if (len <= 0) {
            return (ER)len;
        }
        enc_len += (UH)len;
        msg->var[i].tag.len = (UH)len;
        /* Data */
        len = ber_enc(0, 0, &msg->var[i].dat);
        if (len <= 0) {
            return (ER)len;
        }
        enc_len += (UH)len;
        msg->var[i].tag.len += (UH)len;
        /* Tag */
        len = ber_enc_any(0, 0, &msg->var[i].tag);
        if (len <= 0) {
            return (ER)len;
        }
        enc_len += (UH)len;
    }
    /* Variable bindings tag */
    msg->var_tag.len = enc_len;
    len = ber_enc_any(0, 0, &msg->var_tag);
    if (len <= 0) {
        return (ER)len;
    }
    enc_len += (UH)len;

    /* Request and Error */
    ber_int[0] = &msg->req_id;
    ber_int[1] = &msg->err_sts;
    ber_int[2] = &msg->err_idx;
    for (i = 0; i < 3; i++) {
        len = ber_enc_int(0, 0, ber_int[i]);
        if (len <= 0) {
            return (ER)len;
        }
        enc_len += (UH)len;
    }
    /* Data */
    msg->dat.len = enc_len;
    len = ber_enc_any(0, 0, &msg->dat);
    if (len <= 0) {
        return (ER)len;
    }
    enc_len += (UH)len;

    /* Community */
    len = ber_enc_oct(0, 0, &msg->com);
    if (len <= 0) {
        return (ER)len;
    }
    enc_len += (UH)len;
    /* Version */
    len = ber_enc_int(0, 0, &msg->ver);
    if (len <= 0) {
        return (ER)len;
    }
    enc_len += (UH)len;
    /* Header tag */
    msg->tag.len = enc_len;
    len = ber_enc_any(0, 0, &msg->tag);
    if (len <= 0) {
        return (ER)len;
    }
    enc_len += (UH)len;

    /* Encode */

    /* Header tag */
    len = ber_enc_any(&buf_ptr, &buf_len, &msg->tag);
    if (len <= 0) {
        return (ER)len;
    }
    /* Version */
    len = ber_enc_int(&buf_ptr, &buf_len, &msg->ver);
    if (len <= 0) {
        return (ER)len;
    }
    /* Community */
    len = ber_enc_oct(&buf_ptr, &buf_len, &msg->com);
    if (len <= 0) {
        return (ER)len;
    }

    /* Data */
    len = ber_enc_any(&buf_ptr, &buf_len, &msg->dat);
    if (len <= 0) {
        return (ER)len;
    }
    for (i = 0; i < 3; i++) {
        len = ber_enc_int(&buf_ptr, &buf_len, ber_int[i]);
        if (len <= 0) {
            return (ER)len;
        }
    }

    /* Variable bindings tag */
    len = ber_enc_any(&buf_ptr, &buf_len, &msg->var_tag);
    if (len <= 0) {
        return (ER)len;
    }
    /* Variable bindings */
    for (i = 0; i < msg->var_cnt; i++) {
        /* Tag */
        len = ber_enc_any(&buf_ptr, &buf_len, &msg->var[i].tag);
        if (len <= 0) {
            return (ER)len;
        }
        /* OID */
        len = ber_enc_oid(&buf_ptr, &buf_len, &msg->var[i].oid);
        if (len <= 0) {
            return (ER)len;
        }
        /* Data */
        len = ber_enc(&buf_ptr, &buf_len, &msg->var[i].dat);
        if (len <= 0) {
            return (ER)len;
        }
    }

    return (ER)enc_len;
}

static ER snmp_msg_dec(T_SNMP_MSG_V2* msg, T_SNMP_MSG_VAR* var_buf, VP dat, UH len)
{
    ER ercd;
    T_SNMP_BER ber_dat;
    T_SNMP_BER_INT* ber_int[3];
    INT i;
    T_SNMP_BER* ber_buf;

    /* Decode a message */

    if ((msg == 0) || (dat == 0)) {
        return E_OBJ;
    }

    msg->var = var_buf;

    /* Header tag */
    ercd = ber_dec(&ber_dat, &dat, &len, BER_TAG);
    if ((ercd != E_OK) || (ber_dat.typ != TYP_SEQ)) {
        return E_OBJ;
    }
    ercd = ber_dec_any(&msg->tag, &ber_dat);
    if (ercd != E_OK) {
        return E_OBJ;
    }
    /* Version */
    ercd = ber_dec(&ber_dat, &dat, &len, BER_DAT);
    if (ercd == E_OK) {
        ercd = ber_dec_int(&msg->ver, &ber_dat);
    }
    if (ercd != E_OK) {
        return E_OBJ;
    }
    /* Community */
    ercd = ber_dec(&ber_dat, &dat, &len, BER_DAT);
    if (ercd == E_OK) {
        ercd = ber_dec_oct(&msg->com, &ber_dat);
    }
    if (ercd != E_OK) {
        return E_OBJ;
    }

    /* Data */
    ercd = ber_dec(&ber_dat, &dat, &len, BER_TAG);
    if (ercd == E_OK) {
        ercd = ber_dec_any(&msg->dat, &ber_dat);
    }
    if (ercd != E_OK) {
        return E_OBJ;
    }
    /* Request and Error */
    ber_int[0] = &msg->req_id;
    ber_int[1] = &msg->err_sts;
    ber_int[2] = &msg->err_idx;
    for (i = 0; i < 3; i++) {
        ercd = ber_dec(&ber_dat, &dat, &len, BER_DAT);
        if (ercd == E_OK) {
            ercd = ber_dec_int(ber_int[i], &ber_dat);
        }
        if (ercd != E_OK) {
            return E_OBJ;
        }
    }

    /* Variable data */
    msg->var_cnt = 0;
    if (var_buf == 0x00) {
        return E_OK;
    }
    ercd = ber_dec(&ber_dat, &dat, &len, BER_TAG);
    if (ercd == E_OK) {
        ercd = ber_dec_any(&msg->var_tag, &ber_dat);
    }
    if (ercd != E_OK) {
        return E_OBJ;
    }
    for (i = 0; i < CFG_MSG_VAR_CNT; i++) {
        /* Tag */
        ercd = ber_dec(&ber_dat, &dat, &len, BER_TAG);
        if (ercd == E_OK) {
            ercd = ber_dec_any(&msg->var[i].tag, &ber_dat);
        }
        if (ercd != E_OK) {
            return E_OBJ;
        }
        /* OID */
        ercd = ber_dec(&ber_dat, &dat, &len, BER_DAT);
        if (ercd == E_OK) {
            ercd = ber_dec_oid(&msg->var[i].oid, &ber_dat);
        }
        if (ercd != E_OK) {
            return E_OBJ;
        }
        /* Data */
        ber_buf = (T_SNMP_BER*)&msg->var[i].dat;
        ercd = ber_dec(ber_buf, &dat, &len, BER_DAT);
        if (ercd != E_OK) {
            return E_OBJ;
        }
        msg->var_cnt++;
        
        if (len == 0) {
            break;
        }
    }
    if (len != 0) {
        return E_BOVR;    /* Overflow */
    }

    return E_OK;
}

static ER snmp_ven_trp_alc_cmd_buf(T_SNMP_TRP_CMD** trp_cmd)
{
    ER ercd;
    UH i;

    /* Allocate for vendor trap command buffer */

    if (trp_cmd == 0) {
        return E_PAR;
    }
    *trp_cmd = 0;

    ercd = snmp_sem_wai(ID_SEM_TRP);
    if (ercd != E_OK) {
        return ercd;
    }

    for (i = 0U; i < CFG_VEN_TRP_CNT; i++) {
        if (mgr.ven_trp_cmd_buf[i].sts == TRP_CMD_INV) {
            mgr.ven_trp_cmd_buf[i].flg_id = 0x01U << i;
            mgr.ven_trp_cmd_buf[i].sts = TRP_CMD_INI;
            mgr.ven_trp_cmd_buf[i].nod_ptr = 0;
            mgr.ven_trp_cmd_buf[i].trp_ptr = 0;
            mgr.ven_trp_cmd_buf[i].msg = 0;
            *trp_cmd = &mgr.ven_trp_cmd_buf[i];
            break;
        }
    }

    snmp_sem_sig(ID_SEM_TRP);

    if (*trp_cmd == 0) {
        return E_OBJ;
    }

    return E_OK;
}

static ER snmp_ven_trp_get_inf_cmd(T_SNMP_TRP_CMD** trp_cmd, UH* idx)
{
    ER ercd;
    UH i;

    /* Retrive inform trap command buffer */

    if ((trp_cmd == 0) || (idx == 0)) {
        return E_PAR;
    }
    if (*idx >= CFG_VEN_TRP_CNT) {
        return E_OBJ;
    }
    *trp_cmd = 0;

    ercd = snmp_sem_wai(ID_SEM_TRP);
    if (ercd != E_OK) {
        return ercd;
    }

    for (i = *idx; i < CFG_VEN_TRP_CNT; i++) {
        if ((mgr.ven_trp_cmd_buf[i].sts & TRP_CMD_SND_INF) != 0x00U) {
            *trp_cmd = &mgr.ven_trp_cmd_buf[i];
            *idx = i + 1;
            break;
        }
    }

    snmp_sem_sig(ID_SEM_TRP);

    if (*trp_cmd == 0) {
        return E_OBJ;
    }

    return E_OK;
}

static void snmp_trp_upd_req_id(void)
{
    /* Updated request ID */

    mgr.req_id = (mgr.req_id <= MIN_REQ_ID) ? MAX_REQ_ID : (mgr.req_id - 1);

    return;
}

static ER snmp_trp_alc_cmd_buf(T_SNMP_TRP_CMD** trp_cmd)
{
    ER ercd;
    UH i;

    /* Allocate for trap command buffer */

    if (trp_cmd == 0) {
        return E_PAR;
    }
    *trp_cmd = 0;

    ercd = snmp_sem_wai(ID_SEM_TRP);
    if (ercd != E_OK) {
        return ercd;
    }

    for (i = 0U; i < CFG_MAX_TRP_CNT; i++) {
        if (mgr.trp_cmd_buf[i].sts == TRP_CMD_INV) {
            mgr.trp_cmd_buf[i].flg_id = 0x00U;
            mgr.trp_cmd_buf[i].sts = TRP_CMD_INI;
            mgr.trp_cmd_buf[i].nod_ptr = 0;
            mgr.trp_cmd_buf[i].trp_ptr = 0;
            mgr.trp_cmd_buf[i].msg = 0;
            *trp_cmd = &mgr.trp_cmd_buf[i];
            break;
        }
    }

    snmp_sem_sig(ID_SEM_TRP);

    if (*trp_cmd == 0) {
        return E_OBJ;
    }

    return E_OK;
}

static ER snmp_trp_rel_cmd_buf(T_SNMP_TRP_CMD* trp_cmd)
{
    ER ercd;

    /* Release for trap command buffer */

    if (trp_cmd == 0) {
        return E_PAR;
    }

    ercd = snmp_sem_wai(ID_SEM_TRP);
    if (ercd != E_OK) {
        return ercd;
    }

    trp_cmd->flg_id = 0x00U;
    trp_cmd->sts = TRP_CMD_INV;

    snmp_sem_sig(ID_SEM_TRP);

    return E_OK;
}

static ER snmp_trp_cre_var(T_SNMP_MSG_VAR* var, T_SNMP_TRP* trp, UH nod_typ, UH offset, UH oid_buf_id)
{
    ER ercd;
    UH i;
    ADDR oid;
    ADDR* oid_id_ptr;
    UH var_cnt;
    T_SNMP_BER_OID* ber_oid;
    UH typ;

    /* Create a variable data for trap */

    oid_id_ptr = 0;
    var_cnt = trp->var_cnt;

    if ((var_cnt == 0) && (trp->var_oid != 0)) {
        var_cnt = 1;
        oid = (ADDR)trp->var_oid;
        oid_id_ptr = &oid;
    } else {
        oid_id_ptr = (ADDR*)trp->var_oid;
    }
    if (var_cnt == 0) {
        return 0;
    }
    if (oid_id_ptr == 0) {
        return E_OBJ;
    }

    /* OID is macro ID */
    for (i = 0; i < var_cnt; i++) {
        if (CFG_MSG_VAR_CNT < (i + offset + 1)) {
            return E_OBJ;
        }
        
        typ = nod_typ | ((oid_id_ptr[i] >> 16) & MIB_NOD_ID);
        
        /* Tag */
        ber_set_any(&var[i + offset].tag, TYP_SEQ);
        /* OID */
        ercd = snmp_mib_get_oid_id(&var[i + offset].oid, (UH)oid_id_ptr[i], typ,
                                   mgr.trp_oid_str_buf);
        if (ercd != E_OK) {
            return E_OBJ;
        }
        /* Data */
        ber_oid = (T_SNMP_BER_OID*)&var[i + offset].dat;
        ber_oid->buf = &mgr.trp_oid_buf[oid_buf_id];
        ber_oid->buf_len = CFG_MAX_MIB_DEP;
        oid_buf_id += CFG_MAX_MIB_DEP;
        ercd = snmp_mib_get_ber_dat_id(&var[i + offset].dat, (UH)oid_id_ptr[i], typ);
        if (ercd != E_OK) {
            return E_OBJ;
        }
    }

    return (ER)var_cnt;
}

static ER snmp_trp_cre_v1(T_SNMP_TRP_MSG_V1* msg, T_NODE* nod, T_SNMP_TRP* trp, T_SNMP_TRP_CMD* trp_cmd)
{
    ER ercd;
    INT oid_buf_id;
    UH len;
    T_SNMP_BER_BUF ber_buf;
    T_NODE dev_nod;
    UH nod_typ;
    VP var_oid;
    ER ercd_oid;
    UW oid;

    /* Create a trap message v1 */

    oid_buf_id = 0;
    nod_typ = MIB_NOD_VEN;

    /* Tag */
    ber_set_any(&msg->tag, TYP_SEQ);
    /* Version */
    ber_set_int(&msg->ver, SNMP_VER_V1);
    /* Community */
    len = snmp_strlen(trp->com);
    ber_set_oct(&msg->com, trp->com, len);

    /* Trap */
    ber_set_any(&msg->trp, TYP_TRP);
    /* Enterprise OID */
    msg->ent_oid.buf = &mgr.trp_oid_buf[oid_buf_id];
    msg->ent_oid.buf_len = CFG_MAX_MIB_DEP;
    oid_buf_id += CFG_MAX_MIB_DEP;
    if (trp->ent_oid != 0x00) {
        len = snmp_strlen(trp->ent_oid);
        ercd = ber_set_oid(&msg->ent_oid, trp->ent_oid, len);
    } else {
        len = snmp_strlen(snmp_mib_sys_obj_id);
        ercd = ber_set_oid(&msg->ent_oid, (VP)snmp_mib_sys_obj_id, len);
    }
    if (ercd != E_OK) {
        return E_OBJ;
    }
    /* IP address */
    ercd = snmp_tcp_get_ip(&dev_nod, nod->num);
    if (ercd != E_OK) {
        return E_OBJ;
    }
    ber_set_ip(&msg->ip_adr, dev_nod.ipa);
    /* Generic trap */
    ber_set_int(&msg->gen_trp, trp->gen_trp);
    /* Specific trap */
    if (trp->gen_trp != SNMP_TRP_ENT_SPEC) {
        ber_set_int(&msg->spc_trp, 0);
    } else {
        ber_set_int(&msg->spc_trp, trp->spc_trp);
    }
    /* Timestamp */
    ercd = snmp_mib_get_ber_dat_id(&ber_buf, SNMP_MIB2_SYS_UPTIME, MIB_NOD_STD);
    if (ercd != E_OK) {
        return E_OBJ;
    }
    ber_set_tim(&msg->tim, ((T_SNMP_BER_TIM*)&ber_buf)->dat);

    /* Variable data */
    msg->var = mgr.trp_var_buf;
    /* Tag */
    ber_set_any(&msg->var_tag, TYP_SEQ);
    /* ifIndex */
    ercd_oid = E_OBJ;
    if ((trp->flg & SNMP_TRP_GEN_ENA) != 0x00U) {
        if ((snmp_mib_dat_cnt[SNMP_MIB2_CNT_ITF] != 0U) &&
            ((trp->gen_trp == SNMP_TRP_LINK_UP) || (trp->gen_trp == SNMP_TRP_LINK_DOWN))) {
            if (trp_cmd->dev_num >= 1) {
                nod_typ = MIB_NOD_STD;
                trp->var_cnt = 1;
                oid = mgr.mib_sum_grp[SNMP_MIB2_CNT_ITF];
                oid += (SNMP_MIB2_DEF_CNT_ITF_TBL * (trp_cmd->dev_num - 1)) + SNMP_MIB2_IF_INDEX;
                var_oid = (VP)oid;
                trp->var_oid = &var_oid;
                ercd_oid = E_OK;
            }
        }
    }
    ercd = snmp_trp_cre_var(msg->var, trp, nod_typ, 0, oid_buf_id);
    if (ercd_oid == E_OK) {
        trp->var_oid = 0;
    }
    if (ercd < E_OK) {
        return E_OBJ;
    }
    msg->var_cnt = (UH)ercd;

    return E_OK;
}

static ER snmp_trp_cre_v2(T_SNMP_MSG_V2* msg, T_SNMP_TRP* trp, T_SNMP_TRP_CMD* trp_cmd)
{
    ER ercd;
    INT oid_buf_id;
    T_SNMP_BER_OID* ber_oid;
    UH len;
    VP oid_ptr;
    UH nod_typ;
    VP var_oid[3];
    ER ercd_oid;
    UW oid;

    /* Create a trap message v2 */

    if (CFG_MSG_VAR_CNT < 1) {
        /* Overflow */
        return E_OBJ;
    }
    oid_buf_id = 0;
    nod_typ = MIB_NOD_VEN;

    /* Tag */
    ber_set_any(&msg->tag, TYP_SEQ);
    /* Version */
    ber_set_int(&msg->ver, SNMP_VER_V2C);
    /* Community */
    len = snmp_strlen(trp->com);
    ber_set_oct(&msg->com, trp->com, len);

    /* Trap */
    if ((trp->flg & SNMP_TRP_INF_ENA) == 0x00) {
        ber_set_any(&msg->dat, TYP_V2_TRP);
    } else {
        ber_set_any(&msg->dat, TYP_INF_REQ);
    }
    /* Request ID */
    if ((trp_cmd->sts & TRP_CMD_CRE_ENA) == 0x00) {
        trp_cmd->req_id = mgr.req_id;
        snmp_trp_upd_req_id();
    }
    ber_set_int(&msg->req_id, trp_cmd->req_id);
    /* Error status */
    ber_set_int(&msg->err_sts, 0);
    /* Error index */
    ber_set_int(&msg->err_idx, 0);

    /* Variable data */
    msg->var = mgr.trp_var_buf;
    msg->var_cnt = 0;

    /* Tag */
    ber_set_any(&msg->var_tag, TYP_SEQ);
    /* Tag, System uptime */
    ber_set_any(&msg->var[msg->var_cnt].tag, TYP_SEQ);
    /* OID */
    ercd = snmp_mib_get_oid_id(&msg->var[msg->var_cnt].oid, SNMP_MIB2_SYS_UPTIME,
                               MIB_NOD_STD, mgr.trp_oid_str_buf);
    if (ercd != E_OK) {
        return E_OBJ;
    }
    /* Data */
    if ((trp_cmd->sts & TRP_CMD_CRE_ENA) == 0x00) {
        ercd = snmp_mib_get_ber_dat_id(&msg->var[msg->var_cnt].dat, SNMP_MIB2_SYS_UPTIME, MIB_NOD_STD);
        trp_cmd->sys_tim = ((T_SNMP_BER_TIM*)&msg->var[msg->var_cnt].dat)->dat;
    } else {
        ercd = snmp_ber_set_tim((T_SNMP_BER_TIM*)&msg->var[msg->var_cnt].dat, trp_cmd->sys_tim);
    }
    if (ercd != E_OK) {
        return E_OBJ;
    }
    msg->var_cnt++;

    /* Trap OID */
    if (CFG_MSG_VAR_CNT < msg->var_cnt + 1) {
        return E_OBJ;
    }
    ercd_oid = E_OBJ;
    if ((trp->flg & SNMP_TRP_GEN_ENA) != 0x00) {
        oid_ptr = (VP)snmp_mib_trap_oid_ptr[trp->gen_trp];
        /* ifIndex, ifAdminStatus and ifOperStatus */
        if ((snmp_mib_dat_cnt[SNMP_MIB2_CNT_ITF] != 0U) &&
            ((trp->gen_trp == SNMP_TRP_LINK_UP) || (trp->gen_trp == SNMP_TRP_LINK_DOWN))) {
            if (trp_cmd->dev_num >= 1) {
                nod_typ = MIB_NOD_STD;
                trp->var_cnt = 3;
                oid = mgr.mib_sum_grp[SNMP_MIB2_CNT_ITF];
                oid += SNMP_MIB2_DEF_CNT_ITF_TBL * (trp_cmd->dev_num - 1);
                var_oid[0] = (VP)(oid + SNMP_MIB2_IF_INDEX);
                oid += SNMP_MIB2_IF_ADMIN_STS;
                var_oid[1] = (VP)(oid);
                var_oid[2] = (VP)(oid + 1);
                trp->var_oid = var_oid;
                ercd_oid = E_OK;
            }
        }
    } else {
        oid_ptr = trp->ent_oid;
    }
    /* Tag */
    ber_set_any(&msg->var[msg->var_cnt].tag, TYP_SEQ);
    /* OID */
    len = snmp_strlen(snmp_mib_trap_oid);
    ercd = ber_set_oid(&msg->var[msg->var_cnt].oid, (VP)snmp_mib_trap_oid, len);
    if (ercd != E_OK) {
        if (ercd_oid == E_OK) {
            trp->var_oid = 0;
        }
        return E_OBJ;
    }
    /* Data */
    ber_oid = (T_SNMP_BER_OID*)&msg->var[msg->var_cnt].dat;
    ber_oid->buf = &mgr.trp_oid_buf[oid_buf_id];
    ber_oid->buf_len = CFG_MAX_MIB_DEP;
    oid_buf_id += CFG_MAX_MIB_DEP;
    len = snmp_strlen(oid_ptr);
    ercd = ber_set_oid((T_SNMP_BER_OID*)&msg->var[msg->var_cnt].dat, (VP)oid_ptr, len);
    if (ercd != E_OK) {
        if (ercd_oid == E_OK) {
            trp->var_oid = 0;
        }
        return E_OBJ;
    }
    msg->var_cnt++;

    /* Variable data */
    ercd = snmp_trp_cre_var(msg->var, trp, nod_typ, msg->var_cnt, oid_buf_id);
    if (ercd < E_OK) {
        if (ercd_oid == E_OK) {
            trp->var_oid = 0;
        }
        return E_OBJ;
    }
    msg->var_cnt += (UH)ercd;

    trp_cmd->sts |= TRP_CMD_CRE_ENA;
    if (ercd_oid == E_OK) {
        trp->var_oid = 0;
    }

    return E_OK;
}

static ER snmp_trp_enc_v1(VP buf, UH buf_len, T_SNMP_TRP_MSG_V1* msg)
{
    VP buf_ptr;
    UH enc_len;
    INT i;
    INT len;

    /* Encode a trap message v1 */

    buf_ptr = buf;

    /* Length */

    enc_len = 0;

    /* Variable bindings */
    for (i = 0; i < msg->var_cnt; i++) {
        /* OID */
        len = ber_enc_oid(0, 0, &msg->var[i].oid);
        if (len <= 0) {
            return (ER)len;
        }
        enc_len += (UH)len;
        msg->var[i].tag.len = (UH)len;
        /* Data */
        len = ber_enc(0, 0, &msg->var[i].dat);
        if (len <= 0) {
            return (ER)len;
        }
        enc_len += (UH)len;
        msg->var[i].tag.len += (UH)len;
        /* Tag */
        len = ber_enc_any(0, 0, &msg->var[i].tag);
        if (len <= 0) {
            return (ER)len;
        }
        enc_len += (UH)len;
    }
    /* Variable bindings tag */
    msg->var_tag.len = enc_len;
    len = ber_enc_any(0, 0, &msg->var_tag);
    if (len <= 0) {
        return (ER)len;
    }
    enc_len += (UH)len;

    /* Timestamp */
    len = ber_enc_tim(0, 0, &msg->tim);
    if (len <= 0) {
        return (ER)len;
    }
    enc_len += (UH)len;
    /* Specific trap */
    len = ber_enc_int(0, 0, &msg->spc_trp);
    if (len <= 0) {
        return (ER)len;
    }
    enc_len += (UH)len;
    /* Generic trap */
    len = ber_enc_int(0, 0, &msg->gen_trp);
    if (len <= 0) {
        return (ER)len;
    }
    enc_len += (UH)len;
    /* IP address */
    len = ber_enc_ip(0, 0, &msg->ip_adr);
    if (len <= 0) {
        return (ER)len;
    }
    enc_len += (UH)len;
    /* Enterprise OID */
    len = ber_enc_oid(0, 0, &msg->ent_oid);
    if (len <= 0) {
        return (ER)len;
    }
    enc_len += (UH)len;
    /* Trap */
    msg->trp.len = enc_len;
    len = ber_enc_any(0, 0, &msg->trp);
    if (len <= 0) {
        return (ER)len;
    }
    enc_len += (UH)len;

    /* Community */
    len = ber_enc_oct(0, 0, &msg->com);
    if (len <= 0) {
        return (ER)len;
    }
    enc_len += (UH)len;
    /* Version */
    len = ber_enc_int(0, 0, &msg->ver);
    if (len <= 0) {
        return (ER)len;
    }
    enc_len += (UH)len;
    /* Header tag */
    msg->tag.len = enc_len;
    len = ber_enc_any(0, 0, &msg->tag);
    if (len <= 0) {
        return (ER)len;
    }
    enc_len += (UH)len;

    /* Encode */

    /* Header tag */
    len = ber_enc_any(&buf_ptr, &buf_len, &msg->tag);
    if (len <= 0) {
        return (ER)len;
    }
    /* Version */
    len = ber_enc_int(&buf_ptr, &buf_len, &msg->ver);
    if (len <= 0) {
        return (ER)len;
    }
    /* Community */
    len = ber_enc_oct(&buf_ptr, &buf_len, &msg->com);
    if (len <= 0) {
        return (ER)len;
    }

    /* Trap */
    len = ber_enc_any(&buf_ptr, &buf_len, &msg->trp);
    if (len <= 0) {
        return (ER)len;
    }
    /* Enterprise OID */
    len = ber_enc_oid(&buf_ptr, &buf_len, &msg->ent_oid);
    if (len <= 0) {
        return (ER)len;
    }
    /* IP address */
    len = ber_enc_ip(&buf_ptr, &buf_len, &msg->ip_adr);
    if (len <= 0) {
        return (ER)len;
    }
    /* Generic trap */
    len = ber_enc_int(&buf_ptr, &buf_len, &msg->gen_trp);
    if (len <= 0) {
        return (ER)len;
    }
    /* Specific trap */
    len = ber_enc_int(&buf_ptr, &buf_len, &msg->spc_trp);
    if (len <= 0) {
        return (ER)len;
    }
    /* Timestamp */
    len = ber_enc_tim(&buf_ptr, &buf_len, &msg->tim);
    if (len <= 0) {
        return (ER)len;
    }

    /* Variable bindings tag */
    len = ber_enc_any(&buf_ptr, &buf_len, &msg->var_tag);
    if (len <= 0) {
        return (ER)len;
    }
    /* Variable bindings */
    for (i = 0; i < msg->var_cnt; i++) {
        /* Tag */
        len = ber_enc_any(&buf_ptr, &buf_len, &msg->var[i].tag);
        if (len <= 0) {
            return (ER)len;
        }
        /* OID */
        len = ber_enc_oid(&buf_ptr, &buf_len, &msg->var[i].oid);
        if (len <= 0) {
            return (ER)len;
        }
        /* Data */
        len = ber_enc(&buf_ptr, &buf_len, &msg->var[i].dat);
        if (len <= 0) {
            return (ER)len;
        }
    }

    return (ER)enc_len;
}

static ER snmp_trp_snd(T_SNMP_TRP_CMD* trp_cmd, UH flg_sem)
{
    ER ercd;
    ID tsk_id;
    T_NODE* nod;
    T_SNMP_TRP* trp;
    INT req_id;
    T_NODE rmt_nod;
    UH trp_len;
    UB* buf;

    /* Send trap */

    if (trp_cmd == 0x00) {
        return E_PAR;
    }

    /* Node for IP address */
    nod = (trp_cmd->nod_ptr != 0x00) ? trp_cmd->nod_ptr : &trp_cmd->nod;
    trp = (trp_cmd->trp_ptr != 0x00) ? trp_cmd->trp_ptr : &trp_cmd->trp;
    if (trp->var_cnt > CFG_MSG_VAR_CNT) {
        return E_OBJ;
    }

    ercd = get_tid(&tsk_id);
    if (ercd != E_OK) {
        return ercd;
    }
    if (tsk_id != ID_TSK_RCV) {
        ercd = snmp_sem_wai(ID_SEM_MIB);
        if (ercd != E_OK) {
            return ercd;
        }
    }

    /* Create trap massage */
    if (trp->ver == SNMP_VER_V1) {
        ercd = snmp_trp_cre_v1((T_SNMP_TRP_MSG_V1*)trp_cmd->msg, nod, trp, trp_cmd);
    } else {
        req_id = trp_cmd->req_id;
        ercd = snmp_trp_cre_v2((T_SNMP_MSG_V2*)trp_cmd->msg, trp, trp_cmd);
        if (trp_cmd->req_id == 0) {
            trp_cmd->req_id = req_id;
        }
    }
    if (ercd != E_OK) {
        if (tsk_id != ID_TSK_RCV) {
            snmp_sem_sig(ID_SEM_MIB);
        }
        return E_OBJ;
    }

    /* Encode */
    if (trp->ver == SNMP_VER_V1) {
        ercd = snmp_trp_enc_v1(trp_cmd->buf, trp_cmd->buf_len, (T_SNMP_TRP_MSG_V1*)trp_cmd->msg);
    } else {
        ercd = snmp_msg_enc(trp_cmd->buf, trp_cmd->buf_len, (T_SNMP_MSG_V2*)trp_cmd->msg);
    }
    if (ercd < 0) {
        if (tsk_id != ID_TSK_RCV) {
            snmp_sem_sig(ID_SEM_MIB);
        }
        return E_OBJ;
    }
    trp_len = (UH)ercd;

    if (tsk_id != ID_TSK_RCV) {
        snmp_sem_sig(ID_SEM_MIB);
    }

    /* Send a message */
    ercd = E_OBJ;
    if (trp_len > 0) {
        ercd = cfg_soc(trp_cmd->soc_id, SOC_TMO_SND, (VP)trp_cmd->soc_tmo);
        if (ercd == E_OK) {
            rmt_nod.ver = IP_VER4;
            rmt_nod.ipa = nod->ipa;
            rmt_nod.port = TRP_PORT;
            rmt_nod.num = nod->num;
            ercd = con_soc(trp_cmd->soc_id, &rmt_nod, 0) ;
            if (ercd == E_OK) {
                buf = trp_cmd->buf;
                while (trp_len > 0) {
                    ercd = snd_soc(trp_cmd->soc_id, buf, trp_len);
                    if (ercd < 0) {
                        break;
                    }
                    buf += (UH)ercd;
                    trp_len -= (UH)ercd;
                }
                if (trp_len == 0) {
                    ercd = E_OK;
                }
            }
        }
    }
    if (ercd == E_OK) {
        dbg_put_str("snmp_trp_snd(E_OK)\r\n");
        snmp_mib_snmp_inc(MIB2_SNMP_OUT_PKT, flg_sem);
        snmp_mib_snmp_inc(MIB2_SNMP_OUT_TRAP, flg_sem);
    } else {
        dbg_put_str("snmp_trp_snd(error)\r\n");
    }

    return ercd;
}

static ER snmp_trp_snd_on_rcv_tsk(INT gen_trp, T_NODE* nod)
{
    ER ercd;
    T_SNMP_TRP_CMD trp_cmd;
    INT i;
    INT soc_idx;

    /* Send normal trap on receive task */

    if (nod == 0) {
        return E_PAR;
    }
    if (CFG_MAX_TRP_CNT == 0U) {
        return E_OBJ;
    }

    /* Check for enable bitmap */
    ercd = snmp_sem_wai(ID_SEM_TRP);
    if (ercd != E_OK) {
        return ercd;
    }
    if ((mgr.trp_ena & (0x01 << gen_trp)) == 0x00) {
        ercd = E_OBJ;
    }
    snmp_sem_sig(ID_SEM_TRP);
    if (ercd != E_OK) {
        return E_OBJ;
    }

    i = 0;
    while (snmp_cfg_trp[i].str != 0x00) {
#if CFG_SNMP_AUTHFAIL_ONLY_REQ
        if (snmp_cfg_trp[i].nod->ipa == nod->ipa) {
#endif
            soc_idx = snmp_soc_idx_trp(snmp_cfg_trp[i].nod->num);
            if (soc_idx >= 0) {
                trp_cmd.soc_id = mgr.udp_soc[soc_idx].sid;
                trp_cmd.msg = mgr.snd_msg; 
                trp_cmd.buf = mgr.snd_buf;
                trp_cmd.buf_len = mgr.snd_buf_len;
                trp_cmd.nod_ptr = snmp_cfg_trp[i].nod;
                trp_cmd.trp_ptr = 0x00;
                trp_cmd.soc_tmo = CFG_UDP_SND_TMO;
                
                snmp_memset(&trp_cmd.trp, 0, sizeof(T_SNMP_TRP));
                trp_cmd.trp.ver = snmp_cfg_trp[i].ver;
                trp_cmd.trp.com = snmp_cfg_trp[i].str;
                trp_cmd.trp.ent_oid = (VB*)snmp_mib_sys_obj_id;
                trp_cmd.trp.gen_trp = SNMP_TRP_AUTH_FAIL;
                trp_cmd.trp.spc_trp = 0;
                trp_cmd.trp.flg = SNMP_TRP_GEN_ENA;
                
                snmp_trp_snd(&trp_cmd, SEM_MIB_DIS);
            }
#if CFG_SNMP_AUTHFAIL_ONLY_REQ
        }
#endif
        i++;
    }

    return E_OK;
}

static ER snmp_trp_snd_gen(INT gen_trp, UH dev_num, ID soc_idx, INT cfg_trp_idx)
{
    ER ercd;
    T_SNMP_TRP_CMD* trp_cmd;

    /* Send general trap */

    /* Get a command buffer */
    ercd = snmp_trp_alc_cmd_buf(&trp_cmd);
    if (ercd != E_OK) {
        return E_OBJ;
    }

    /* Send a command to trap task */
    trp_cmd->flg_id = 0x00U;    /* No wait */
    trp_cmd->soc_id = mgr.udp_soc[soc_idx].sid;
    trp_cmd->nod_ptr = snmp_cfg_trp[cfg_trp_idx].nod;
    trp_cmd->trp_ptr = 0;
    trp_cmd->dev_num = dev_num;
    trp_cmd->soc_tmo = CFG_UDP_SND_TMO;
    trp_cmd->sts |= TRP_CMD_SND_TRP;

    snmp_memset(&trp_cmd->trp, 0, sizeof(T_SNMP_TRP));
    trp_cmd->trp.ver = snmp_cfg_trp[cfg_trp_idx].ver;
    trp_cmd->trp.com = snmp_cfg_trp[cfg_trp_idx].str;
    trp_cmd->trp.ent_oid = (VB*)snmp_mib_sys_obj_id;
    trp_cmd->trp.gen_trp = gen_trp;
    trp_cmd->trp.spc_trp = 0;
    trp_cmd->trp.flg = SNMP_TRP_GEN_ENA;

    dbg_put_str("snmp_trp_snd_gne(nod:");
    dbg_put_dig(snmp_cfg_trp[cfg_trp_idx].nod->num);
    dbg_put_str(", dev_num:");
    dbg_put_dig(trp_cmd->dev_num);
    dbg_put_str(")\r\n");

    ercd = snd_mbx(ID_MBX_TRP, (T_MSG*)trp_cmd);
    if (ercd != E_OK) {
        dbg_put_str("Error: snmp_trp_snd_gne(snd_mbx)\r\n");
        snmp_trp_rel_cmd_buf(trp_cmd);
    }

    return ercd;
}

ER snmp_trp_snd_sta(UH dev_num)
{
    ER ercd;
    INT gen_trp;
    UH sta_bit;
    INT i;
    INT soc_idx;

    /* Send startup trap on TCP task */

    if ((mgr.sts & STS_ENA) == 0x00) {
        return E_OBJ;
    }
    if (CFG_MAX_TRP_CNT == 0U) {
        return E_OBJ;
    }
    if (dev_num > CFG_NET_DEV_CNT) {
        return E_PAR;
    }

    if ((mgr.sts & STS_STA_COLD) != 0x00U) {
        gen_trp = SNMP_TRP_COLD_STA;
        sta_bit = SNMP_TRP_COLD_STA_BIT;
    } else {
        gen_trp = SNMP_TRP_WARM_STA;
        sta_bit = SNMP_TRP_WARM_STA_BIT;
    }

    /* Check for startup trap */
    ercd = snmp_sem_wai(ID_SEM_TRP);
    if (ercd != E_OK) {
        return ercd;
    }
    ercd = E_OBJ;
    if ((mgr.trp_ena & sta_bit) != 0x00U) {
        if ((mgr.trp_sts[dev_num - 1] & STS_TRP_STA_ENA) != 0x00U) {
            if ((mgr.trp_sts[dev_num - 1] & STS_TRP_STA_SND) == 0x00U) {
                ercd = E_OK;
            }
        }
    }
    snmp_sem_sig(ID_SEM_TRP);
    if (ercd != E_OK) {
        return E_OBJ;
    }

    /* Send startup trap */
    i = 0;
    while (snmp_cfg_trp[i].str != 0x00) {
        if (snmp_cfg_trp[i].nod->num == dev_num) {
            soc_idx = snmp_soc_idx_trp(snmp_cfg_trp[i].nod->num);
            if (soc_idx >= 0) {
                ercd = snmp_trp_snd_gen(gen_trp, 0U, soc_idx, i);
                if (ercd != E_OK) {
                    break;
                }
            }
        }
        i++;
    }
    if (ercd == E_OK) {
        ercd = snmp_sem_wai(ID_SEM_TRP);
        if (ercd == E_OK) {
            mgr.trp_sts[dev_num - 1] |= STS_TRP_STA_SND;
            snmp_sem_sig(ID_SEM_TRP);
        }
    }

    return ercd;
}

ER snmp_trp_snd_lnk(INT gen_trp, UH dev_num)
{
    ER ercd;
    INT i;
    INT soc_idx;

    /* Send link trap on TCP task */

    if ((mgr.sts & STS_ENA) == 0x00) {
        return E_OBJ;
    }
    if (CFG_MAX_TRP_CNT == 0U) {
        return E_OBJ;
    }
    if (!((gen_trp == SNMP_TRP_LINK_UP) || (gen_trp == SNMP_TRP_LINK_DOWN))) {
        return E_PAR;
    }
    if (dev_num > CFG_NET_DEV_CNT) {
        return E_PAR;
    }

    /* Check for enable bitmap */
    ercd = snmp_sem_wai(ID_SEM_TRP);
    if (ercd != E_OK) {
        return ercd;
    }
    if ((mgr.trp_ena & (0x01 << gen_trp)) == 0x00U) {
        ercd = E_OBJ;
    }
    snmp_sem_sig(ID_SEM_TRP);
    if (ercd != E_OK) {
        return E_OBJ;
    }

    /* Send link trap */
    i = 0;
    while (snmp_cfg_trp[i].str != 0x00) {
        soc_idx = snmp_soc_idx_trp(snmp_cfg_trp[i].nod->num);
        if (soc_idx >= 0) {
            ercd = snmp_tcp_get_eth_lnk(snmp_cfg_trp[i].nod->num, 0);    /* MIB Sem off */
            if (ercd == E_OK) {
                ercd = snmp_trp_snd_gen(gen_trp, dev_num, soc_idx, i);
                if (ercd != E_OK) {
                    break;
                }
            }
        }
        i++;
    }

    return ercd;
}

static ER snmp_trp_snd_sta_all(void)
{
    ER ercd;
    INT gen_trp;
    UH i;
    UH sts;
    INT j;
    INT soc_idx;

    /* Send startup trap for all devices */

    if ((mgr.sts & STS_ENA) == 0x00U) {
        return E_OBJ;
    }
    if (CFG_MAX_TRP_CNT == 0U) {
        return E_OBJ;
    }

    if ((mgr.sts & STS_STA_COLD) != 0x00U) {
        gen_trp = SNMP_TRP_COLD_STA;
    } else {
        gen_trp = SNMP_TRP_WARM_STA;
    }

    /* Check for enable bitmap */
    ercd = snmp_sem_wai(ID_SEM_TRP);
    if (ercd != E_OK) {
        return ercd;
    }
    if (gen_trp == SNMP_TRP_COLD_STA) {
        if ((mgr.trp_ena & SNMP_TRP_COLD_STA_BIT) == 0x00U) {
            ercd = E_OBJ;
        }
    } else {
        if ((mgr.trp_ena & SNMP_TRP_WARM_STA_BIT) == 0x00U) {
            ercd = E_OBJ;
        }
    }
    snmp_sem_sig(ID_SEM_TRP);
    if (ercd != E_OK) {
        return E_OK;
    }

    /* Start trap */
    for (i = 0U; i < CFG_NET_USE_CNT; i++) {
        if (snmp_cfg_net[i].id_trp_soc != 0) {
            ercd = snmp_sem_wai(ID_SEM_TRP);
            if (ercd == E_OK) {
                sts = mgr.trp_sts[snmp_cfg_net[i].dev_num - 1];
            } else {
                break;
            }
            snmp_sem_sig(ID_SEM_TRP);
            if (((sts & STS_TRP_STA_ENA) != 0x00U) && ((sts & STS_TRP_STA_SND) == 0x00U)) {
                soc_idx = snmp_soc_idx_trp(snmp_cfg_net[i].dev_num);
                if (soc_idx >= 0) {
                    ercd = snmp_tcp_get_eth_lnk(snmp_cfg_net[i].dev_num, 1);    /* MIB sem ON */
                    if (ercd == E_OK) {
                        j = 0;
                        while (snmp_cfg_trp[j].str != 0) {
                            if (snmp_cfg_trp[j].nod->num == snmp_cfg_net[i].dev_num) {
                                ercd = snmp_trp_snd_gen(gen_trp, 0U, soc_idx, j);
                                if (ercd != E_OK) {
                                    break;
                                }
                            }
                            j++;
                        }
                    }
                }
                if (ercd == E_OK) {
                    ercd = snmp_sem_wai(ID_SEM_TRP);
                    if (ercd == E_OK) {
                        mgr.trp_sts[i] |= STS_TRP_STA_SND;
                        snmp_sem_sig(ID_SEM_TRP);
                    } else {
                        break;
                    }
                }
            }
        }
    }

    return ercd;
}

static ER snmp_msg_res(T_SNMP_MSG_V2* rcv_msg, ID soc_id)
{
    ER ercd;
    T_NODE rmt_nod;
    T_SNMP_TRP_CMD* trp_cmd;

    /* Receive a response message */

    /* Get a remote node */
    ercd = ref_soc(soc_id, SOC_IP_REMOTE, &rmt_nod);
    if ((ercd != E_OK) || (rmt_nod.port == PORT_ANY) || (rmt_nod.ipa == 0x00)) {
        return E_OBJ;
    }

    /* Mail to trap task */
    ercd = snmp_trp_alc_cmd_buf(&trp_cmd);
    if (ercd != E_OK) {
        return E_QOVR;
    }
    trp_cmd->sts |= TRP_CMD_RES_INF;
    trp_cmd->nod = rmt_nod;
    trp_cmd->req_id = rcv_msg->req_id.dat;

    ercd = snd_mbx(ID_MBX_TRP, (T_MSG*)trp_cmd);
    if (ercd != E_OK) {
        snmp_trp_rel_cmd_buf(trp_cmd);
        return E_OBJ;
    }

    return E_OK;
}

static ER snmp_res_snd(ID soc_id, T_NODE *rmt_nod, UH snd_len)
{
    ER ercd;
    UB* buf;
    
    /* Send a message */
    ercd = E_OBJ;
    if (snd_len > 0) {
        ercd = con_soc(soc_id, rmt_nod, 0) ;
        if (ercd == E_OK) {
            buf = mgr.snd_buf;
            while (snd_len > 0) {
                ercd = snd_soc(soc_id, buf, snd_len);
                if (ercd < 0) {
                    break;
                }
                buf += (UH)ercd;
                snd_len -= (UH)ercd;
            }
            if (snd_len == 0) {
               ercd = E_OK;
            }
        }
    }
    return ercd;
}

static ER snmp_msg_rcv(T_SNMP_MSG_V2* rcv_msg, ER ercd_dec, ID soc_id)
{
    ER ercd;
    UB cmd;
    T_NODE rmt_nod;
    INT err_idx;
    
    /* Receive a message */

    if (!((rcv_msg->ver.dat == SNMP_VER_V1) || (rcv_msg->ver.dat == SNMP_VER_V2C))) {
        snmp_mib_snmp_inc(MIB2_SNMP_IN_BAD_VERSION, SEM_MIB_ENA);
        return E_OBJ;
    }

    ercd = snmp_sem_wai(ID_SEM_MIB);
    if (ercd != E_OK) {
        return ercd;
    }

    /* Command */
    cmd = rcv_msg->dat.typ;
    if (cmd == TYP_GET_RES) {
        ercd = snmp_msg_res(rcv_msg, soc_id);
        snmp_mib_snmp_inc(MIB2_SNMP_IN_GET_RESPONSE, SEM_MIB_DIS);
        snmp_sem_sig(ID_SEM_MIB);
        return ercd;
    }
    if (!((cmd == TYP_GET_REQ) || (cmd == TYP_GET_NXT) || (cmd == TYP_SET_REQ) || (cmd == TYP_GET_BULK))) {
        if ((cmd == TYP_TRP) || (cmd == TYP_V2_TRP)) {
            snmp_mib_snmp_inc(MIB2_SNMP_IN_TRAP, SEM_MIB_DIS);
        }
        snmp_sem_sig(ID_SEM_MIB);
        return E_OBJ;
    }
    
    switch (cmd) {
        case TYP_GET_REQ:
            snmp_mib_snmp_inc(MIB2_SNMP_IN_GET_REQUEST, SEM_MIB_DIS);
            break;
        case TYP_GET_NXT:
            snmp_mib_snmp_inc(MIB2_SNMP_IN_GET_NEXT, SEM_MIB_DIS);
            break;
        case TYP_SET_REQ:
            snmp_mib_snmp_inc(MIB2_SNMP_IN_SET_REQUEST, SEM_MIB_DIS);
            break;
        default:
            break;
    }
    if (rcv_msg->err_sts.dat != ERR_NO_ERR) {
        switch (rcv_msg->err_sts.dat) {
            case ERR_TOO_BIG:
                snmp_mib_snmp_inc(MIB2_SNMP_IN_TOO_BIG, SEM_MIB_DIS);
                break;
            case ERR_NO_SUCH_NAME:
                snmp_mib_snmp_inc(MIB2_SNMP_IN_NO_SUCH_NAME, SEM_MIB_DIS);
                break;
            case ERR_BAD_VAL:
                snmp_mib_snmp_inc(MIB2_SNMP_IN_BAD_VALUE, SEM_MIB_DIS);
                break;
            case ERR_GEN_ERR:
                snmp_mib_snmp_inc(MIB2_SNMP_IN_GEN_ERR, SEM_MIB_DIS);
                break;
            case ERR_READ_ONLY:
                snmp_mib_snmp_inc(MIB2_SNMP_IN_READ_ONLY, SEM_MIB_DIS);
                break;
            default:
                break;
        }
    }

    /* Get a remote node */
    ercd = ref_soc(soc_id, SOC_IP_REMOTE, &rmt_nod);
    if ((ercd != E_OK) || (rmt_nod.port == PORT_ANY) || (rmt_nod.ipa == 0x00)) {
        snmp_sem_sig(ID_SEM_MIB);
        return E_OBJ;
    }
    ercd = snmp_chk_nod(&rmt_nod);
    if (ercd != E_OK) {
        snmp_sem_sig(ID_SEM_MIB);
        return E_OBJ;
    }

    /* Check for a community */
    ercd = snmp_chk_com((VB*)rcv_msg->com.buf, rcv_msg->com.len, cmd);
    if (ercd != E_OK) {
        if (ercd == E_ID) {
            /* Send trap, No such community */
            snmp_trp_snd_on_rcv_tsk(SNMP_TRP_AUTH_FAIL, &rmt_nod);
        }
        else {
            /* Send response, Access inhibit */
            ercd = snmp_set_err((T_SNMP_MSG_V2*)mgr.snd_msg, rcv_msg, E_NA, 1);
            if (ercd == E_OK) {
                ercd = snmp_msg_enc(mgr.snd_buf, mgr.snd_buf_len, (T_SNMP_MSG_V2*)mgr.snd_msg);
                if (0 < ercd) {
                    ercd = snmp_res_snd(soc_id, &rmt_nod, (UH)ercd);                
                }
            }
        }
        snmp_sem_sig(ID_SEM_MIB);
        return E_OBJ;
    }

    if ((ercd_dec == E_OK) || (cmd == TYP_GET_BULK)) {
        /* Set request */
        if (cmd == TYP_SET_REQ) {
            snmp_evt_cbk(SNMP_EVT_SET_START, (VP)(INT)rcv_msg->var_cnt);
            err_idx = 0;
            ercd = snmp_set_var((T_SNMP_MSG_V2*)mgr.snd_msg, rcv_msg, &err_idx);
            if (ercd == E_OK) {
                /* Create response message */
                ercd = snmp_msg_cre((T_SNMP_MSG_V2*)mgr.snd_msg, rcv_msg);
            } else {
                /* Create error message */
                ercd = snmp_set_err((T_SNMP_MSG_V2*)mgr.snd_msg, rcv_msg, ercd, err_idx);
            }
            snmp_evt_cbk((ERR_NO_ERR == ((T_SNMP_MSG_V2*)mgr.snd_msg)->err_sts.dat) ?
                SNMP_EVT_SET_END : SNMP_EVT_SET_ERROR, (VP)((T_SNMP_MSG_V2*)mgr.snd_msg)->err_sts.dat);
        } else {
            snmp_evt_cbk(SNMP_EVT_GET_START, (VP)(INT)rcv_msg->var_cnt);
            /* Create response message */
            ercd = snmp_msg_cre((T_SNMP_MSG_V2*)mgr.snd_msg, rcv_msg);
            snmp_evt_cbk((ERR_NO_ERR == ((T_SNMP_MSG_V2*)mgr.snd_msg)->err_sts.dat) ?
                SNMP_EVT_GET_END : SNMP_EVT_GET_ERROR, (VP)((T_SNMP_MSG_V2*)mgr.snd_msg)->err_sts.dat);
        }
    } else {
        ercd = ercd_dec;    /* E_BOVR */
    }
    if (!((ercd == E_OK) || (ercd == E_BOVR))) {
        snmp_sem_sig(ID_SEM_MIB);
        return E_OBJ;
    }

    /* Encode */
    if (ercd == E_OK) {
        ercd = snmp_msg_enc(mgr.snd_buf, mgr.snd_buf_len, (T_SNMP_MSG_V2*)mgr.snd_msg);
    }
    if ((ercd == E_BOVR) && ((rcv_msg->ver.dat == SNMP_VER_V1) || (cmd == TYP_SET_REQ))) {
        /* Buffer overflow. Create error message */
        if (ercd_dec == E_BOVR) {
            ercd = snmp_set_err((T_SNMP_MSG_V2*)mgr.snd_msg, rcv_msg, E_BOVR, 0);
        } else {
            ercd = snmp_msg_cre_err((T_SNMP_MSG_V2*)mgr.snd_msg, rcv_msg, ERR_TOO_BIG);
        }
        if (ercd == E_OK) {
            ercd = snmp_msg_enc(mgr.snd_buf, mgr.snd_buf_len, (T_SNMP_MSG_V2*)mgr.snd_msg);
        }
        snmp_mib_snmp_inc(MIB2_SNMP_OUT_TOO_BIG, SEM_MIB_DIS);
    }
    snmp_sem_sig(ID_SEM_MIB);
    if (ercd < 0) {
        return E_OBJ;
    }
    
    /* Send a message */
    ercd = snmp_res_snd(soc_id, &rmt_nod, (UH)ercd);
    
    if (ercd == E_OK) {
        snmp_mib_snmp_inc(MIB2_SNMP_OUT_PKT, SEM_MIB_ENA);
        switch (((T_SNMP_MSG_V2*)mgr.snd_msg)->dat.typ) {
            case TYP_GET_REQ:
                snmp_mib_snmp_inc(MIB2_SNMP_OUT_GET_REQUEST, SEM_MIB_ENA);
                break;
            case TYP_GET_NXT:
                snmp_mib_snmp_inc(MIB2_SNMP_OUT_GET_NEXT, SEM_MIB_ENA);
                break;
            case TYP_SET_REQ:
                snmp_mib_snmp_inc(MIB2_SNMP_OUT_SET_REQUEST, SEM_MIB_ENA);
                break;
            case TYP_GET_RES:
                snmp_mib_snmp_inc(MIB2_SNMP_OUT_GET_RESPONSE, SEM_MIB_ENA);
                break;
            default:
                break;
        }
    } else {
        return E_OBJ;
    }

    return ercd;
}

static ER snmp_get_mib_datp_std(VP_UW* buf, UH id, UH* len)
{
    ER ercd;
    T_SNMP_MIB_DAT* mib_dat;
    UW tmp;
    UH dev_idx;
    INT i;

    /* Retrive a MIB-2 data */
    /* buf: Data pointer */

    if (buf == 0x00) {
        return E_PAR;
    }
    if (len != 0x00) {
        *len = 0;
    }

    ercd = E_OBJ;
    if (id < mgr.mib_sum_grp[SNMP_MIB2_CNT_AT]) {
        /* System */
        mib_dat = snmp_mib_dat[SNMP_MIB2_DAT_SYS];
        *buf = mib_dat[id];
        ercd = E_OK;
    } else if (id < mgr.mib_sum_grp[SNMP_MIB2_CNT_ICMP]) {
        /* IP */
        id -= mgr.mib_sum_grp[SNMP_MIB2_CNT_IP];
        if ((id <= SNMP_MIB2_IP_FRAG_CREATE) || (id == SNMP_MIB2_IP_ROUTING_DISCARD)) {
            ercd = net_sts_get(NET_STS_IP, id, (UW*)buf);
        }
    } else if (id < mgr.mib_sum_grp[SNMP_MIB2_CNT_TCP]) {
        /* ICMP */
        id -= mgr.mib_sum_grp[SNMP_MIB2_CNT_ICMP];
        ercd = net_sts_get(NET_STS_ICMP, id, (UW*)buf);
    } else if (id < mgr.mib_sum_grp[SNMP_MIB2_CNT_UDP]) {
        /* TCP */
        id -= mgr.mib_sum_grp[SNMP_MIB2_CNT_TCP];
        if ((id <= SNMP_MIB2_TCP_RETRANS_SEG) || (id >= SNMP_MIB2_TCP_IN_ERR)) {
            if (id >= SNMP_MIB2_TCP_IN_ERR) {
                id -= MIB2_TCP_OFFSET;
            }
            ercd = net_sts_get(NET_STS_TCP, id, (UW*)buf);
        }
    } else if (id < mgr.mib_sum_grp[SNMP_MIB2_CNT_SNMP]) {
        /* UDP */
        id -= mgr.mib_sum_grp[SNMP_MIB2_CNT_UDP];
        if (id <= SNMP_MIB2_UDP_OUT_DATAGRAM) {
            ercd = net_sts_get(NET_STS_UDP, id, (UW*)buf);
        }
    } else if (id < mgr.mib_sum_grp[SNMP_MIB2_CNT_ITF]) {
        /* SNMP */
        id -= mgr.mib_sum_grp[SNMP_MIB2_CNT_SNMP];
        mib_dat = snmp_mib_dat[SNMP_MIB2_DAT_SNMP];
        *buf = mib_dat[id];
        ercd = E_OK;
    } else {
        /* Interface */
        id -= mgr.mib_sum_grp[SNMP_MIB2_CNT_ITF];
        mib_dat = snmp_mib_dat[SNMP_MIB2_DAT_ITF];
        /* Convert to index of MIB data */
        if (id == SNMP_MIB2_IF_NUM) {
            *buf = mib_dat[MIB2_DAT_ITF_NUM];
            ercd = E_OK;
        } else if (id >= SNMP_MIB2_DEF_CNT_ITF_IDX) {
            /* Offset for network devices */
            dev_idx = 0U;
            ercd = snmp_mib_id_itf(&id, &dev_idx);
            if (ercd == E_OK) {
                mib_dat++;    /* Skip ifNumber */
                mib_dat += MIB2_DAT_ITF_OFFSET * dev_idx;    /* Skip network devices */
                if (id == SNMP_MIB2_IF_INDEX) {
                    *buf = mib_dat[MIB2_DAT_ITF_INDEX];
                    ercd = E_OK;
                } else if (id == SNMP_MIB2_IF_LAST_CHG) {
                    *buf = mib_dat[MIB2_DAT_ITF_LAST_CHG];
                    ercd = E_OK;
                } else if (id == SNMP_MIB2_IF_SPECIFIC) {
                    *buf = mib_dat[MIB2_DAT_ITF_SPECIFIC];
                    ercd = E_OK;
                } else {
                    if (id <= SNMP_MIB2_IF_OUT_QLEN) {
                        ercd = net_sts_get(NET_STS_IFS + dev_idx, id - MIB2_IF_OFFSET, (UW*)buf);
                        if (ercd == E_OK) {
                            if (id == SNMP_MIB2_IF_PHY_ADDR) {
#if (_kernel_SIZE_SIZE==8)  /* In a 64bit environment */
                                /* get the upper 32 bits and create an address. */
                                (void)net_sts_get(NET_STS_IFS + dev_idx, NET_STS_IF_PHY_ADDR_U32, (UW*)&tmp);
                                *buf = (VP)(((ADDR)tmp << 32) | (ADDR)*buf);
#endif
                                for (i = 0; i < MAC_ADR_LEN; i++) {
                                    mgr.mac_dat_buf[i] = ((char*)(*buf))[i];
                                }
                                *buf = (VP_UW)mgr.mac_dat_buf;
                                if (len != 0x00) {
                                    *len = MAC_ADR_LEN;
                                }
                            }
#if (_kernel_SIZE_SIZE==8)  /* In a 64bit environment */
                            if (id == SNMP_MIB2_IF_DESCR) {
                                /* get the upper 32 bits and create an address. */
                                (void)net_sts_get(NET_STS_IFS + dev_idx, NET_STS_IF_DESCR_U32, (UW*)&tmp);
                                *buf = (VP)(((ADDR)tmp << 32) | (ADDR)*buf);
                            }                            
#endif
                        }
                    }
                }
            }
        }
    }

    return ercd;
}

static ER snmp_set_mib_dat_std(VP_UW buf, UH len, UH id)
{
    ER ercd;
    T_SNMP_MIB_DAT* mib_dat;
    char* str;
    UH dev_idx;

    /* Update a MIB-2 data */
    /* buf: Data(UW) or string pointer */

    ercd = E_OBJ;
    if (id < mgr.mib_sum_grp[SNMP_MIB2_CNT_AT]) {
        /* System */
        mib_dat = snmp_mib_dat[SNMP_MIB2_DAT_SYS];
        if (snmp_mib[id].typ == TYP_OCT_STR || snmp_mib[id].typ == TYP_OBJ_ID) {
            if ((len + 1) > snmp_mib[id].len) {
                ercd = E_DAT;    /* Buffer not enough */
            } else {
                /* Terminal null */
                str = (char*)mib_dat[id];
                snmp_strncpy(str, (const char*)buf, len);
                str[len] = '\0';
                ercd = E_OK;
            }
        } else if (snmp_mib[id].typ == TYP_CNT64) {
            *((UD_SNMP *)mib_dat[id]) = *((UD_SNMP*)buf);
            ercd = E_OK;
        } else {
            mib_dat[id] = buf;
            ercd = E_OK;
        }
    } else if (id >= mgr.mib_sum_grp[SNMP_MIB2_CNT_ITF]) {
        /* Interface */
        id -= mgr.mib_sum_grp[SNMP_MIB2_CNT_ITF];
        mib_dat = snmp_mib_dat[SNMP_MIB2_DAT_ITF];
        dev_idx = 0U;
        ercd = snmp_mib_id_itf(&id, &dev_idx);
        if (ercd == E_OK) {
            mib_dat++;    /* Skip ifNumber */
            mib_dat += MIB2_DAT_ITF_OFFSET * dev_idx;    /* Skip network devices */
            if (id == SNMP_MIB2_IF_LAST_CHG) {
                mib_dat[MIB2_DAT_ITF_LAST_CHG] = buf;
                ercd = E_OK;
            }
        }
    }

    return ercd;
}

ER snmp_get_mib_datp(VP_UW* buf, UH id, UH nod_typ, UH flg_cbk, UH* len)
{
    ER ercd;
    UH nod_id;
    const T_SNMP_MIB* mib;
    T_SNMP_MIB_DAT dat;
    VP dat_ptr;
    const T_SNMP_CFG_CBK* cbk;
    T_SNMP_CFG_CBK_DAT cbk_dat;
    T_SNMP_MIB_TBL *mib_tbl;

    snmp_memset(&cbk_dat, 0, sizeof(T_SNMP_CFG_CBK_DAT));
    mib_tbl = snmp_mib_ven;

    /* Retrive a MIB data */
    /* buf: Data pointer */

    if (buf == 0x00) {
        return E_PAR;
    }

    nod_id = nod_typ & MIB_NOD_ID;
    nod_typ &= MIB_NOD_TYP;
    if (nod_typ == MIB_NOD_STD) {
        if (id > mgr.mib_cnt) {
            return E_PAR;
        }
    } else if (nod_typ == MIB_NOD_VEN) {
        if (nod_id >= mgr.mib_ven_cnt) {
            return E_PAR;
        }
        if (id >= snmp_mib_ven[nod_id].cnt) {
            return E_PAR;
        }
    } else if (nod_typ == (MIB_NOD_VAL | MIB_NOD_STD)) {
        if (id >= CFG_TCP_DAT_CNT) {
            return E_PAR;
        }
    } else if (nod_typ == (MIB_NOD_VAL | MIB_NOD_VEN)) {
        if (nod_id >= mgr.mib_ven_val_cnt) {
            return E_PAR;
        }
        if (id >= snmp_mib_ven_val[nod_id].cnt) {
            return E_PAR;
        }
        mib_tbl = snmp_mib_ven_val;
    } else {
        return E_PAR;
    }

    if (nod_typ == MIB_NOD_STD) {
        if (id == SNMP_MIB2_SYS_UPTIME) {
            snmp_set_sys_tim();
        }
    } 

    /* Callback function */
    if ((flg_cbk != 0x00) && ((nod_typ & MIB_NOD_VEN) != 0x00U)) {
        mib = &(mib_tbl[nod_id].mib[id]);
        if (snmp_cfg_cbk[0].fnc != 0x00) {
            if (nod_typ == (MIB_NOD_VAL | MIB_NOD_VEN)) {
                cbk_dat.req = SNMP_REQ_GET_VAL;
            } else {
                cbk_dat.req = SNMP_REQ_GET;
            }
            cbk_dat.mib_id = nod_id;
            cbk_dat.obj_id = id;
            
            cbk = 0;
            dat = mib_tbl[nod_id].dat[id];
            dat_ptr = &(mib_tbl[nod_id].dat[id]);
            if (mib_tbl[nod_id].cbk != 0) {
                cbk = &(mib_tbl[nod_id].cbk[id]);
            }
            cbk_dat.typ = mib->typ;
            if (mib->typ == TYP_OCT_STR) {
                cbk_dat.buf = dat;
                cbk_dat.dat_len = mib->len;
            } else if (mib->typ == TYP_OBJ_ID) {
                cbk_dat.buf = dat;
                cbk_dat.dat_len = snmp_strlen((const char*)dat);
            } else if (mib->typ == TYP_CNT64) {
                cbk_dat.buf = dat;
                cbk_dat.dat_len = 8;
            } else {
                cbk_dat.buf = dat_ptr;
                cbk_dat.dat_len = 4;
            }
            cbk_dat.buf_len = mib->len;
            if (cbk == 0) {
                snmp_cfg_cbk[0].fnc(&cbk_dat);
            } else {
                if (cbk->fnc != 0) {
                    cbk->fnc(&cbk_dat);
                }
            }
            if ((mib->acs & SNMP_STS_WO) != 0x00U) {
                if (mib->typ == TYP_OCT_STR || mib->typ == TYP_OBJ_ID) {
                    if (cbk_dat.dat_len < mib->len) {
                        ((VB*)cbk_dat.buf)[cbk_dat.dat_len + 1] = '\0';
                    }
                }
            }
            *len = cbk_dat.dat_len;
        }
        if (*len == 0) {
            *len = mib->len;
        }
    }

    ercd = E_OK;
    if (nod_typ == MIB_NOD_STD) {
        ercd = snmp_get_mib_datp_std(buf, id, len);
    } else if (nod_typ == MIB_NOD_VEN) {
        *buf = snmp_mib_ven[nod_id].dat[id];
    } else if (nod_typ == (MIB_NOD_VAL | MIB_NOD_STD)) {
        *buf = mgr.tcp_dat[id].buf;
    } else {
        *buf = snmp_mib_ven_val[nod_id].dat[id];
    }

    return ercd;
}

ER snmp_set_mib_dat(VP_UW buf, UH len, UH id, UH nod_typ, UH flg_cbk)
{
    ER ercd;
    UH nod_id;
    const T_SNMP_MIB* mib;
    T_SNMP_CFG_CBK_DAT cbk_dat;
    ER ercd_cbk;
    char* str;
    T_SNMP_MIB_TBL *tbl;

    /* Update a MIB data */
    /* buf: Data(UW) or string pointer */

    mib = 0x00;
    tbl = NULL;
    nod_id = nod_typ & MIB_NOD_ID;
    nod_typ &= MIB_NOD_TYP;
    if (nod_typ == MIB_NOD_STD) {
        if (id > mgr.mib_cnt) {
            return E_PAR;
        }
        mib = snmp_mib;
    } else if (nod_typ == MIB_NOD_VEN) {
        if (nod_id >= mgr.mib_ven_cnt) {
            return E_PAR;
        }
        if (id >= snmp_mib_ven[nod_id].cnt) {
            return E_PAR;
        }
        mib = snmp_mib_ven[nod_id].mib;
        tbl = &snmp_mib_ven[nod_id];
    } else if (nod_typ == (MIB_NOD_VAL | MIB_NOD_VEN)) {
        if (nod_id >= mgr.mib_ven_val_cnt) {
            return E_PAR;
        }
        if (id >= snmp_mib_ven_val[nod_id].cnt) {
            return E_PAR;
        }
        mib = snmp_mib_ven_val[nod_id].mib;
        tbl = &snmp_mib_ven_val[nod_id];
    } else {
        return E_PAR;
    }

    /* Check for octet buffer size */
    if (mib[id].typ == TYP_OCT_STR || mib[id].typ == TYP_OBJ_ID) {
        if (len + 1 > mib[id].len) {
            return E_DAT;    /* Buffer not enough */
        }
    }

    /* Callback function */
    ercd_cbk = E_OK;
    if (flg_cbk != 0x00) {
        if (snmp_cfg_cbk[0].fnc != 0x00) {
            if (nod_typ == MIB_NOD_STD) {
                if ((id == SNMP_MIB2_SYS_CONTACT) || (id == SNMP_MIB2_SYS_NAME)
                    || (id == SNMP_MIB2_SYS_LOCATION)) {
                    cbk_dat.req = SNMP_REQ_SET_SYS;
                    cbk_dat.mib_id = 0;
                    cbk_dat.obj_id = id;
                    cbk_dat.typ = snmp_mib[id].typ;
                    cbk_dat.buf = buf;
                    cbk_dat.dat_len = len;
                    ercd_cbk = snmp_cfg_cbk[0].fnc(&cbk_dat);    /* ercd_cbk is not refer */
                }
            } else {
                if (nod_typ == MIB_NOD_VEN) {
                    cbk_dat.req = SNMP_REQ_SET;
                } else {
                    cbk_dat.req = SNMP_REQ_SET_VAL;
                }
                cbk_dat.mib_id = nod_id;
                cbk_dat.obj_id = id;
                cbk_dat.typ = mib[id].typ;
                if (mib[id].typ == TYP_OCT_STR || mib[id].typ == TYP_OBJ_ID) {
                    cbk_dat.buf = buf;
                    cbk_dat.dat_len = len;
                } else if (mib[id].typ == TYP_CNT64) {
                    cbk_dat.buf = buf;
                    cbk_dat.dat_len = 8;
                } else {
                    cbk_dat.buf = &buf;
                    cbk_dat.dat_len = 4;
                }
                cbk_dat.buf_len = 0;
                if (tbl->cbk == 0x00) {
                    ercd_cbk = snmp_cfg_cbk[0].fnc(&cbk_dat);
                } else {
                    if (tbl->cbk[id].fnc != 0x00) {
                        ercd_cbk = tbl->cbk[id].fnc(&cbk_dat);
                    }
                }
            }
        }
    }

    ercd = E_OK;
    if (nod_typ == MIB_NOD_STD) {
        ercd = snmp_set_mib_dat_std(buf, len, id);
    } else {
        if (ercd_cbk == E_OK) {
            if (tbl->mib[id].typ == TYP_OCT_STR ||
                tbl->mib[id].typ == TYP_OBJ_ID) {
                /* Terminal null */
                str = (char*)tbl->dat[id];
                snmp_strncpy(str, (const char*)buf, len);
                str[len] = '\0';
            } else if (tbl->mib[id].typ == TYP_CNT64) {
                *((UD_SNMP *)tbl->dat[id]) = *((UD_SNMP*)buf);
            } else {
                tbl->dat[id] = buf;
            }            
        } else {
            ercd = E_DAT;
        }
    }

    return ercd;
}

ER snmp_set_itf_last_chg(UH dev_num)
{
    ER ercd;
    VP dat;
    UH id;

    /* Updated last chnage timer in interface group */

    if (dev_num > CFG_NET_DEV_CNT) {
        return E_PAR;
    }
    if (snmp_mib_dat_cnt[SNMP_MIB2_CNT_ITF] == 0U) {
        return E_OBJ;
    }

    ercd = snmp_get_mib_datp(&dat, SNMP_MIB2_SYS_UPTIME, MIB_NOD_STD, CB_STS_DIS, 0x00);
    if (ercd == E_OK) {
        id = mgr.mib_sum_grp[SNMP_MIB2_CNT_ITF];
        id += SNMP_MIB2_DEF_CNT_ITF_TBL * (dev_num - 1);
        id += SNMP_MIB2_IF_LAST_CHG;
        ercd = snmp_set_mib_dat(dat, LEN_INT, id, MIB_NOD_STD, CB_STS_DIS);
    }

    return ercd;
}

static ER snmp_tsk_sta(void)
{
    ER ercd;
    FLGPTN flg;
    FLGPTN flg_sts;

    /* Task start */

    ercd = clr_flg(ID_FLG_STS, 0x0000U);
    if (ercd == E_OK) {
        ercd = set_flg(ID_FLG_STS, FLG_MOD_STA);
    }
    if (ercd != E_OK) {
        return E_OBJ;
    }

    #if !defined(NET_HW_OS)
    act_tsk(ID_TSK_TIM);
    act_tsk(ID_TSK_RCV);
    if ((CFG_MAX_TRP_CNT + CFG_VEN_TRP_CNT) > 0U) {
        act_tsk(ID_TSK_TRP);
    }
    #else
    sta_tsk(ID_TSK_TIM, 0);
    sta_tsk(ID_TSK_RCV, 0);
    if ((CFG_MAX_TRP_CNT + CFG_VEN_TRP_CNT) > 0U) {
        sta_tsk(ID_TSK_TRP, 0);
    }
    #endif

    flg = FLG_TSK_RCV_STA | FLG_TSK_TIM_STA;
    if ((CFG_MAX_TRP_CNT + CFG_VEN_TRP_CNT) > 0U) {
        flg |= FLG_TSK_TRP_STA;
    }
    flg_sts = 0x0000U;
    ercd = twai_flg(ID_FLG_STS, flg, TWF_ANDW, &flg_sts, FLG_TSK_TMO);

    return ercd;
}

static ER snmp_tsk_stp(void)
{
    ER ercd;
    FLGPTN flg;
    FLGPTN flg_sts;

    /* Task kill */

    set_flg(ID_FLG_STS, FLG_MOD_STP);
    rel_wai(ID_TSK_RCV);

    flg = FLG_TSK_RCV_STP | FLG_TSK_TIM_STP;
    if ((CFG_MAX_TRP_CNT + CFG_VEN_TRP_CNT) > 0U) {
        flg |= FLG_TSK_TRP_STP;
    }
    flg_sts = 0x0000U;
    ercd = twai_flg(ID_FLG_STS, flg, TWF_ANDW, &flg_sts, CFG_UDP_RCV_TMO + FLG_TSK_TMO);

    return ercd;
}

ER snmp_set_sys_tim(void)
{
    T_SNMP_MIB_DAT* mib_dat;
    SYSTIM tim;
    ADDR dat;

    /* Updated system timer */

    mib_dat = snmp_mib_dat[SNMP_MIB2_DAT_SYS];
    dat = (ADDR)mib_dat[SNMP_MIB2_SYS_UPTIME];

    get_tim(&tim); 
    #if !defined(NET_HW_OS)
    if (tim.ltime >= mgr.sys_tim.ltime) {
        tim.ltime -= mgr.sys_tim.ltime;
    } else {
        tim.ltime += 0xffffffff - mgr.sys_tim.ltime;
    }
    tim.ltime /= 10;
    dat += tim.ltime;
    #else
    if (tim >= mgr.sys_tim) {
        tim -= mgr.sys_tim;
    } else {
        tim += 0xffffffff - mgr.sys_tim;
    }
    tim /= 10;
    dat += tim;
    #endif
    get_tim(&mgr.sys_tim); 

    mib_dat[SNMP_MIB2_SYS_UPTIME] = (VP)dat;

    return E_OK;
}

static ER snmp_soc_cbk(UH sid, UH ev, ER ercd)
{
    UH i;

    /* UDP socket callback function */

    if ((ev & EV_SOC_RCV) != 0x00U) {
        for (i = 0U; i < mgr.udp_soc_cnt; i++) {
            if (sid == mgr.udp_soc[i].sid) {
                mgr.udp_soc[i].sts |= STS_SOC_RCV;
                wup_tsk(ID_TSK_RCV);
                break;
            }
        }
    }

    return E_OK;
}

void snmp_rcv_tsk(VP_INT exinf)
{
    ER ercd;
    UH i;
    UH rcv_len;
    FLGPTN flg;
    T_SNMP_BER ber_buf;
    VP buf;
    UH len;
    ER ercd_dec;

    /* Receive task */

    flg = 0x00U;
    clr_flg(ID_FLG_STS, ~FLG_TSK_RCV_STP);
    set_flg(ID_FLG_STS, FLG_TSK_RCV_STA);

    for (i = 0U; i < mgr.udp_soc_cnt; i++) {
        cfg_soc(mgr.udp_soc[i].sid, SOC_CBK_HND, (VP)snmp_soc_cbk);
        cfg_soc(mgr.udp_soc[i].sid, SOC_CBK_FLG, (VP)EV_SOC_RCV);
        mgr.udp_soc[i].sts = STS_SOC_INI;
    }

    while ((flg & FLG_MOD_STP) == 0x00U) {
        for (i = 0U; i < mgr.udp_soc_cnt; i++) {
            if (((mgr.udp_soc[i].sts & STS_SOC_RCV) != 0x00U)
                || ((mgr.udp_soc[i].sts & STS_SOC_WBLK) == 0x00U))
            {
                mgr.udp_soc[i].sts &= ~STS_SOC_RCV;
                ercd = 1;
                while (ercd > 0) {
                    ercd = rcv_soc(mgr.udp_soc[i].sid, mgr.rcv_buf, mgr.rcv_buf_len);
                    if (ercd > 0) {
                        mgr.udp_soc[i].sts &= ~STS_SOC_WBLK;
                        /* Debug print */
                        dbg_put_str("Rcv len:"); dbg_put_dig(ercd); dbg_put_str("\r\n");
                        /* Decode a first tag */
                        rcv_len = ercd;
                        buf = mgr.rcv_buf;
                        len = rcv_len;
                        ercd_dec = ber_dec(&ber_buf, &buf, &len, BER_TAG);
                        if (ercd_dec == E_OK) {
                            /* Decode a message */
                            ercd_dec = snmp_msg_dec(mgr.rcv_msg, mgr.rcv_var_buf, mgr.rcv_buf, rcv_len);
                            snmp_mib_snmp_inc(MIB2_SNMP_IN_PKT, SEM_MIB_ENA);
                            if ((ercd_dec == E_OK) || (ercd_dec == E_BOVR)) {
                                /* Receive a message */
                                snmp_msg_rcv(mgr.rcv_msg, ercd_dec, mgr.udp_soc[i].sid);
                            } else {
                                snmp_mib_snmp_inc(MIB2_SNMP_IN_ASN_PARSE_ERR, SEM_MIB_ENA);
                            }
                        } else {
                            snmp_mib_snmp_inc(MIB2_SNMP_IN_ASN_PARSE_ERR, SEM_MIB_ENA);
                        }
                    } else if (ercd == E_WBLK) {
                        mgr.udp_soc[i].sts |= STS_SOC_WBLK;
                    } else {
                        abt_soc(mgr.udp_soc[i].sid, SOC_ABT_RCV);
                        #if !defined(NET_HW_OS)
                        dly_tsk(ERR_SOC_WAI);
                        #else
                        tslp_tsk(ERR_SOC_WAI);
                        #endif
                        ercd = 1;    /* Retry */
                    }
                }
            }
        }
        
        /* Wait for UDP packet */
        ercd = tslp_tsk(CFG_UDP_RCV_TMO);
        if (ercd == E_OK) {
            continue;    /* Wake up from callback of UDP socket */
        } else if (ercd != E_TMOUT) {
            break;       /* Error stop task */
        }
        
        /* Update flags */
        pol_flg(ID_FLG_STS, FLG_MOD_STP, TWF_ORW, &flg);
    }

    for (i = 0U; i < mgr.udp_soc_cnt; i++) {
        abt_soc(mgr.udp_soc[i].sid, SOC_ABT_RCV);
    }
    set_flg(ID_FLG_STS, FLG_TSK_RCV_STP);

    return;
}

void snmp_tim_tsk(VP_INT exinf)
{
    ER ercd;
    FLGPTN flg;

    /* Timer task */

    flg = 0x00;
    clr_flg(ID_FLG_STS, ~FLG_TSK_TIM_STP);
    set_flg(ID_FLG_STS, FLG_TSK_TIM_STA);

    /* System timer set zero */

    while ((flg & FLG_MOD_STP) == 0x00) {
        ercd = snmp_sem_wai(ID_SEM_MIB);
        if (ercd == E_OK) {
            snmp_set_sys_tim();
            snmp_sem_sig(ID_SEM_MIB);
        }
        
        /* Delay and update flags */
        twai_flg(ID_FLG_STS, FLG_MOD_STP, TWF_ORW, &flg, SYS_TIM_DLY);
    }

    set_flg(ID_FLG_STS, FLG_TSK_TIM_STP);

    return;
}

void snmp_trp_tsk(VP_INT exinf)
{
    ER ercd;
    FLGPTN flg;
    T_SNMP_TRP_CMD* trp_cmd;
    T_SNMP_TRP_CMD* trp_cmd_wai;
    FLGPTN flg_id;
    UH idx;

    /* Trap task */

    flg = 0x00;
    clr_flg(ID_FLG_STS, ~FLG_TSK_TRP_STP);
    set_flg(ID_FLG_STS, FLG_TSK_TRP_STA);

    /* System timer set zero */

    while ((flg & FLG_MOD_STP) == 0x00) {
        ercd = trcv_mbx(ID_MBX_TRP, (T_MSG**)&trp_cmd, MBX_TRP_TMO);
        if (ercd == E_OK) {
            /* Receive message */
            dbg_put_str("snmp_trp_tsk(trcv_mbx)\r\n");
            if ((trp_cmd->sts & (TRP_CMD_SND_TRP | TRP_CMD_SND_INF)) != 0x00U) {
                /* Trap or inform */
                trp_cmd->msg = mgr.trp_msg;
                trp_cmd->buf = mgr.trp_buf;
                trp_cmd->buf_len = mgr.trp_buf_len;
                trp_cmd->ercd = snmp_trp_snd(trp_cmd, SEM_MIB_ENA);
                
                if ((trp_cmd->sts & TRP_CMD_SND_TRP) != 0x00) {
                    /* Trap */
                    flg_id = trp_cmd->flg_id;
                    if (flg_id != 0x00U) {
                        set_flg(ID_FLG_TRP, flg_id);
                    } else {
                        snmp_trp_rel_cmd_buf(trp_cmd);
                    }
                } else {
                    /* Inform */
                    if (trp_cmd->ercd != E_OK) {
                        trp_cmd->req_id = 0;
                        flg_id = trp_cmd->flg_id;
                        set_flg(ID_FLG_TRP, flg_id);
                    }
                }
            } else if ((trp_cmd->sts & TRP_CMD_RES_INF) != 0x00) {
                /* Receive inform response */
                idx = 0;
                ercd = snmp_ven_trp_get_inf_cmd(&trp_cmd_wai, &idx);
                while (ercd == E_OK) {
                    if (trp_cmd_wai->req_id == trp_cmd->req_id) {
                        trp_cmd_wai->ercd = E_OK;
                        flg_id = trp_cmd_wai->flg_id;
                        set_flg(ID_FLG_TRP, flg_id);
                        break;
                    }
                    ercd = snmp_ven_trp_get_inf_cmd(&trp_cmd_wai, &idx);
                }
                snmp_trp_rel_cmd_buf(trp_cmd);
            }
        } else if (ercd == E_TMOUT) {
            /* Timeout */
            idx = 0;
            ercd = snmp_ven_trp_get_inf_cmd(&trp_cmd_wai, &idx);
            while (ercd == E_OK) {
                trp_cmd_wai->tmo = (trp_cmd_wai->tmo < MBX_TRP_TMO) ? 0 : (trp_cmd_wai->tmo - MBX_TRP_TMO);
                if (trp_cmd_wai->tmo == 0) {
                    if (trp_cmd_wai->rty_cnt > 0) {
                        /* Retry for inform request */
                        trp_cmd_wai->ercd = snmp_trp_snd(trp_cmd_wai, SEM_MIB_ENA);
                        trp_cmd_wai->rty_cnt--;
                        trp_cmd_wai->tmo = trp_cmd_wai->trp_ptr->tmo;
                    } else {
                        /* Timeout for inform request */
                        trp_cmd_wai->ercd = E_TMOUT;
                        flg_id = trp_cmd_wai->flg_id;
                        set_flg(ID_FLG_TRP, flg_id);
                    }
                }
                ercd = snmp_ven_trp_get_inf_cmd(&trp_cmd_wai, &idx);
            }
        }
        
        /* Update flags */
        pol_flg(ID_FLG_STS, FLG_MOD_STP, TWF_ORW, &flg);
    }

    set_flg(ID_FLG_STS, FLG_TSK_TRP_STP);

    return;
}

ER snmp_ini(UH* mib_nod_cnt)
{
    ER ercd;
    ER ercd_ven;
    INT id;
    INT len;
    UH i;
    UH j;
    UH cnt;
    T_SNMP_MIB_DAT* mib_dat;
    UW* mib_root;
    #if !defined(NET_C_OS)
    T_NODE nod;
    #endif

    /* Initialization */

    #if (CFG_DBG_ENA == 1)
    if (sizeof(UB*) != 4) {
        return E_OBJ;
    }
    if (sizeof(UW) != 4) {
        return E_OBJ;
    }
    if (sizeof(T_SNMP_TRP_MSG_V1) < sizeof(T_SNMP_MSG_V2)) {
        return E_OBJ;
    }
    dbg_put_str("\r\n");
    dbg_put_str("CFG_SND_MSG_LEN: ");
    dbg_put_dig(CFG_SND_MSG_LEN);
    dbg_put_str(" byte\r\n");
    dbg_put_str("CFG_RCV_MSG_LEN: ");
    dbg_put_dig(CFG_RCV_MSG_LEN);
    dbg_put_str(" byte\r\n");
    dbg_put_str("T_SNMP_MSG_V2: ");
    dbg_put_dig(sizeof(T_SNMP_MSG_V2));
    dbg_put_str(" byte\r\n");
    dbg_put_str("T_SNMP_TRP_MSG_V1: ");
    dbg_put_dig(sizeof(T_SNMP_TRP_MSG_V1));
    dbg_put_str(" byte\r\n");
    dbg_put_str("T_SNMP_MSG_BUF: ");
    dbg_put_dig(sizeof(T_SNMP_MSG_BUF));
    dbg_put_str(" byte\r\n");
    dbg_put_str("T_SNMP_BER_BUF: ");
    dbg_put_dig(sizeof(T_SNMP_BER_BUF));
    dbg_put_str(" byte\r\n");
    #endif

    /* Manager */
    mgr.snd_buf_len = (BUF_UW_LEN(CFG_SND_MSG_LEN)) * sizeof(UW);
    mgr.rcv_buf_len = (BUF_UW_LEN(CFG_RCV_MSG_LEN)) * sizeof(UW);
    mgr.trp_buf_len = mgr.snd_buf_len;
    mgr.tcp_dat = (T_SNMP_MIB_TCP_DAT*)&snmp_cfg_tcp_buf[0];
    mgr.req_id = MAX_REQ_ID;
    mgr.trp_ena = CFG_GEN_TRP_ENA;
    mgr.mib_cnt = 0;
    mgr.mib_ven_cnt = 0;
    mgr.mib_ven_val_cnt = 0;

    dbg_put_str("mgr.snd_buf_len: ");
    dbg_put_dig(mgr.snd_buf_len);
    dbg_put_str(" byte\r\n");
    dbg_put_str("mgr.rcv_buf_len: ");
    dbg_put_dig(mgr.rcv_buf_len);
    dbg_put_str(" byte\r\n");

    /* Number of MIB object */
    ercd = E_OBJ;
    for (i = 0U; i < 0xffffU; i++) {
        if (snmp_mib[i].str == 0x00) {
            ercd = E_OK;
            break;
        }
        mgr.mib_cnt++;
    }
    if (ercd != E_OK) {
        return E_OBJ;
    }

    /* Number of vendor MIB object */
    ercd_ven = E_OBJ;
    for (i = 0U; i < 0xffU; i++) {
        if (snmp_mib_ven[i].mib == 0) {
            ercd_ven = E_OK;
            break;
        }
        ercd = E_OBJ;
        for (j = 0U; j < 0xffffU; j++) {
            if (snmp_mib_ven[i].mib[j].str == 0) {
                snmp_mib_ven[i].cnt = j;
                ercd = E_OK;
                break;
            }
        }
        if (ercd != E_OK) {
            return E_OBJ;
        }
        mgr.mib_ven_cnt++;
    }
    if (ercd_ven != E_OK) {
        return E_OBJ;
    }

    /* Number of vendor variable MIB object */
    ercd_ven = E_OBJ;
    for (i = 0U; i < 0xffU; i++) {
        if (snmp_mib_ven_val[i].mib == 0) {
            ercd_ven = E_OK;
            break;
        }
        ercd = E_OBJ;
        for (j = 0U; j < 0xffffU; j++) {
            if (snmp_mib_ven_val[i].mib[j].str == 0) {
                snmp_mib_ven_val[i].cnt = j;
                ercd = E_OK;
                break;
            }
        }
        if (ercd != E_OK) {
            return E_OBJ;
        }
        mgr.mib_ven_val_cnt++;
    }
    if (ercd_ven != E_OK) {
        return E_OBJ;
    }

    /* Buffer */
    mgr.snd_buf = (UB*)&snmp_cfg_buf[0];
    len = BUF_UW_LEN(CFG_SND_MSG_LEN);
    id = len;
    if ((CFG_MAX_TRP_CNT + CFG_VEN_TRP_CNT) > 0U) {
        mgr.trp_buf = (UB*)&snmp_cfg_buf[id];
        id += len;
    }
    mgr.rcv_buf = (UB*)&snmp_cfg_buf[id];
    len = BUF_UW_LEN(CFG_RCV_MSG_LEN);
    id += len;
    mgr.snd_msg = (T_SNMP_MSG_BUF*)&snmp_cfg_buf[id];
    len = BUF_UW_LEN(sizeof(T_SNMP_MSG_BUF));
    id += len;
    if ((CFG_MAX_TRP_CNT + CFG_VEN_TRP_CNT) > 0U) {
        mgr.trp_msg = (T_SNMP_MSG_BUF*)&snmp_cfg_buf[id];
        id += len;
    }
    mgr.rcv_msg = (T_SNMP_MSG_V2*)&snmp_cfg_buf[id];
    len = BUF_UW_LEN(sizeof(T_SNMP_MSG_V2));
    id += len;
    mgr.snd_var_buf = (T_SNMP_MSG_VAR*)&snmp_cfg_buf[id];
    len = BUF_UW_LEN(sizeof(T_SNMP_MSG_VAR)) * CFG_MSG_VAR_CNT;
    id += len;
    mgr.rcv_var_buf = (T_SNMP_MSG_VAR*)&snmp_cfg_buf[id];
    id += len;
    if ((CFG_MAX_TRP_CNT + CFG_VEN_TRP_CNT) > 0U) {
        mgr.trp_var_buf = (T_SNMP_MSG_VAR*)&snmp_cfg_buf[id];
        id += len;
    }
    mib_root = &snmp_cfg_buf[id];
    len = BUF_UW_LEN(sizeof(T_SNMP_MIB_NOD)) * CFG_MIB_NOD_CNT;
    snmp_memset(mib_root, 0, len * sizeof(UW));
    id += len;
    for (i = 0U; i < CFG_MSG_VAR_CNT; i++) {
        mgr.snd_var_buf[i].oid.buf = &snmp_cfg_buf[id];
        mgr.snd_var_buf[i].oid.buf_len = CFG_MAX_MIB_DEP;
        id += CFG_MAX_MIB_DEP;
    }
    for (i = 0U; i < CFG_MSG_VAR_CNT; i++) {
        mgr.rcv_var_buf[i].oid.buf = &snmp_cfg_buf[id];
        mgr.rcv_var_buf[i].oid.buf_len = CFG_MAX_MIB_DEP;
        id += CFG_MAX_MIB_DEP;
    }
    if ((CFG_MAX_TRP_CNT + CFG_VEN_TRP_CNT) > 0U) {
        for (i = 0U; i < CFG_MSG_VAR_CNT; i++) {
            mgr.trp_var_buf[i].oid.buf = &snmp_cfg_buf[id];
            mgr.trp_var_buf[i].oid.buf_len = CFG_MAX_MIB_DEP;
            id += CFG_MAX_MIB_DEP;
        }
    }
    mgr.snd_oid_buf = &snmp_cfg_buf[id];
    id += CFG_MAX_MIB_DEP * CFG_MSG_VAR_CNT;
    if ((CFG_MAX_TRP_CNT + CFG_VEN_TRP_CNT) > 0U) {
        mgr.trp_oid_buf = &snmp_cfg_buf[id];
        id += CFG_MAX_MIB_DEP * (CFG_MSG_VAR_CNT + 1);
    }
    mgr.var_dat_buf = (UB**)&snmp_cfg_buf[id];    /* Undo data buffer pointer */
    id += CFG_MSG_VAR_CNT;
    len = BUF_UW_LEN(CFG_MAX_MIB_DAT);    /* Buffer size with null */
    for (i = 0U; i < CFG_MSG_VAR_CNT; i++) {
        mgr.var_dat_buf[i] = (UB*)&snmp_cfg_buf[id];    /* Buffer pointer */
        id += len;
    }
    mgr.nod_buf = (T_SNMP_MIB_NOD**)&snmp_cfg_buf[id];
    id += CFG_MSG_VAR_CNT;
    if (CFG_MAX_TRP_CNT > 0U) {
        mgr.trp_cmd_buf = (T_SNMP_TRP_CMD*)&snmp_cfg_buf[id];
        for (i = 0U; i < CFG_MAX_TRP_CNT; i++) {
            mgr.trp_cmd_buf[i].sts = TRP_CMD_INV;
        }
        id += BUF_UW_LEN(sizeof(T_SNMP_TRP_CMD)) * CFG_MAX_TRP_CNT;
    } else {
        mgr.trp_cmd_buf = 0;
    }
    if (CFG_VEN_TRP_CNT > 0U) {
        mgr.ven_trp_cmd_buf = (T_SNMP_TRP_CMD*)&snmp_cfg_buf[id];
        for (i = 0U; i < CFG_VEN_TRP_CNT; i++) {
            mgr.ven_trp_cmd_buf[i].sts = TRP_CMD_INV;
        }
        id += BUF_UW_LEN(sizeof(T_SNMP_TRP_CMD)) * CFG_VEN_TRP_CNT;
    } else {
        mgr.ven_trp_cmd_buf = 0;
    }
    mgr.mib_oid_str_buf = (VB*)&snmp_cfg_buf[id];    /* Max 6-byte (65536.) */
    len = BUF_UW_LEN(6 * (CFG_MAX_MIB_DEP + 1));
    id += len;
    mgr.oid_str_len = len * sizeof(UW);
    if ((CFG_MAX_TRP_CNT + CFG_VEN_TRP_CNT) > 0U) {
        mgr.trp_oid_str_buf = (VB*)&snmp_cfg_buf[id];
        id += len;
    } else {
        mgr.trp_oid_str_buf = 0x00;
    }
    mgr.mac_dat_buf = (VB*)&snmp_cfg_buf[id];
    id += BUF_UW_LEN(MAC_ADR_LEN);
    mgr.mac_dat_buf[MAC_ADR_LEN] = '\0';
    mgr.udp_soc = (T_SNMP_UDP_SOC*)&snmp_cfg_buf[id];
    id += BUF_UW_LEN(sizeof(T_SNMP_UDP_SOC)) * CFG_NET_USE_CNT * 2;
    mgr.trp_sts = (UH*)&snmp_cfg_buf[id];
    id += BUF_UW_LEN(sizeof(UH)) * CFG_NET_DEV_CNT;
    mgr.buf_end_id = id;

    /* OS resources */
    #if !(defined(NET_C_OS) || defined(NET_HW_OS))
    ercd = acre_tsk((T_CTSK*)&snmp_cfg_os_tsk_rcv);
    if (ercd <= 0) {
        snmp_ext();
        return E_SYS;
    }
    ID_TSK_RCV = ercd;

    ercd = acre_tsk((T_CTSK*)&snmp_cfg_os_tsk_tim);
    if (ercd <= 0) {
        snmp_ext();
        return E_SYS;
    }
    ID_TSK_TIM = ercd;

    if ((CFG_MAX_TRP_CNT + CFG_VEN_TRP_CNT) > 0U) {
        ercd = acre_tsk((T_CTSK*)&snmp_cfg_os_tsk_trp);
        if (ercd <= 0) {
            snmp_ext();
            return E_SYS;
        }
        ID_TSK_TRP = ercd;
    } else {
        ID_TSK_TRP = 0;
    }

    ercd = acre_sem((T_CSEM*)&snmp_cfg_os_sem_mib);
    if (ercd <= 0) {
        snmp_ext();
        return E_SYS;
    }
    ID_SEM_MIB = ercd;

    if ((CFG_MAX_TRP_CNT + CFG_VEN_TRP_CNT) > 0U) {
        ercd = acre_sem((T_CSEM*)&snmp_cfg_os_sem_trp);
        if (ercd <= 0) {
            snmp_ext();
            return E_SYS;
        }
        ID_SEM_TRP = ercd;
    } else {
        ID_SEM_TRP = 0;
    }

    ercd = acre_flg((T_CFLG*)&snmp_cfg_os_flg_sts);
    if (ercd <= 0) {
        snmp_ext();
        return E_SYS;
    }
    ID_FLG_STS = ercd;

    if ((CFG_MAX_TRP_CNT + CFG_VEN_TRP_CNT) > 0U) {
        ercd = acre_flg((T_CFLG*)&snmp_cfg_os_flg_trp);
        if (ercd <= 0) {
            snmp_ext();
            return E_SYS;
        }
        ID_FLG_TRP = ercd;
    } else {
        ID_FLG_TRP = 0;
    }

    if ((CFG_MAX_TRP_CNT + CFG_VEN_TRP_CNT) > 0U) {
        ercd = acre_mbx((T_CMBX*)&snmp_cfg_os_mbx_trp);
        if (ercd <= 0) {
            snmp_ext();
            return E_SYS;
        }
        ID_MBX_TRP = ercd;
    } else {
        ID_MBX_TRP = 0;
    }
    #endif

    /* Clear MIB data */
    mib_dat = snmp_mib_dat[SNMP_MIB2_DAT_SYS];
    mib_dat[SNMP_MIB2_SYS_UPTIME] = 0;
    if (snmp_mib_dat[SNMP_MIB2_DAT_ITF] != 0x00) {
        mib_dat = snmp_mib_dat[SNMP_MIB2_DAT_ITF];
        mib_dat++;    /* Skip ifNumber */
        for (i = 0U; i < CFG_NET_DEV_CNT; i++) {
            mib_dat[MIB2_DAT_ITF_LAST_CHG] = 0;    /* ifLastChanges */
            mib_dat += MIB2_DAT_ITF_OFFSET;        /* Skip to next device */
        }
    }
    if (snmp_mib_dat[SNMP_MIB2_DAT_SNMP] != 0x00) {
        mib_dat = snmp_mib_dat[SNMP_MIB2_DAT_SNMP];
        snmp_memset(mib_dat, 0, sizeof(T_SNMP_MIB_DAT) * snmp_mib_dat_cnt[SNMP_MIB2_CNT_SNMP]);
    }
    /* Number of MIB data in group */
    cnt = 0;
    for (i = 0U; i < SNMP_MIB2_CNT_GRP; i++) {
        mgr.mib_sum_grp[i] = cnt;
        cnt += snmp_mib_dat_cnt[i];
    }

    /* Initialize MIB */
    ercd = snmp_mib_ini(&id);
    if (ercd == E_OK) {
        snmp_cfg_buf[id] = 0xffffaa55U;    /* Terminal flag */
        /* Create MIB tree */
        ercd = snmp_mib_cre(mib_root, mgr.mib_oid_str_buf, mgr.oid_str_len, mib_nod_cnt);
        if (ercd == E_OK) {
            /* Initialize MIB for TCP */
            ercd = snmp_tcp_ini(ID_SEM_MIB, mgr.mib_oid_str_buf, mgr.oid_str_len);
            if (ercd == E_OK) {
                ercd = snmp_tcp_nod_cnt(&cnt);
            }
        }
    }
    if (ercd != E_OK) {
        return ercd;
    }
    if (mib_nod_cnt != 0x00) {
        *mib_nod_cnt += cnt;
    }
    /* Initialize MIB for SNMP */
    if (snmp_mib_dat[SNMP_MIB2_DAT_SNMP] != 0x00) {
        if ((CFG_GEN_TRP_ENA & SNMP_TRP_AUTH_FAIL_BIT) != 0x00) {
            mib_dat = snmp_mib_dat[SNMP_MIB2_DAT_SNMP];
            mib_dat[MIB2_SNMP_OUT_ENA_AUTH_TRAP] = (VP)1;    /* Enable */
        }
    }

    /* Network socket */
    /* Clear */
    for (i = 0U; i < CFG_NET_USE_CNT * 2; i++) {
        mgr.udp_soc[i].dev_num = 0U;
        mgr.udp_soc[i].sid = 0;
        mgr.udp_soc[i].sts = STS_SOC_INI;
    }
    /* Number of sockets */
    mgr.msg_soc_cnt = 0U;
    mgr.trp_soc_cnt = 0U;
    for (i = 0U; i < CFG_NET_USE_CNT; i++) {
        if (snmp_cfg_net[i].id_udp_soc != 0) {
            mgr.udp_soc[mgr.msg_soc_cnt].dev_num = snmp_cfg_net[i].dev_num;
            #if defined(NET_C_OS)    /* Compact */
            mgr.udp_soc[mgr.msg_soc_cnt].sid = snmp_cfg_net[i].id_udp_soc;
            #endif
            mgr.msg_soc_cnt++;
        }
    }
    if ((CFG_MAX_TRP_CNT + CFG_VEN_TRP_CNT) > 0U) {
        for (i = 0U; i < CFG_NET_USE_CNT; i++) {
            if (snmp_cfg_net[i].id_trp_soc != 0) {
                mgr.udp_soc[mgr.msg_soc_cnt + mgr.trp_soc_cnt].dev_num = snmp_cfg_net[i].dev_num;
                #if defined(NET_C_OS)    /* Compact */
                mgr.udp_soc[mgr.msg_soc_cnt + mgr.trp_soc_cnt].sid = snmp_cfg_net[i].id_trp_soc;
                #endif
                mgr.trp_soc_cnt++;
            }
        }
    }
    mgr.udp_soc_cnt = mgr.msg_soc_cnt + mgr.trp_soc_cnt;
    /* Create sockets for SNMP message and trap */
    #if !defined(NET_C_OS)
    nod.port = SNMP_PORT;    /* Message port */
    nod.ver = IP_VER4;
    nod.ipa = INADDR_ANY;
    for (i = 0U; i < mgr.udp_soc_cnt; i++) {
        nod.num = mgr.udp_soc[i].dev_num;
        if (i >= mgr.msg_soc_cnt) {
            nod.port = PORT_ANY;    /* Trap port */
        }
        ercd = cre_soc(IP_PROTO_UDP, &nod);
        if (ercd > 0) {
            mgr.udp_soc[i].sid = (ID)ercd;
            ercd = cfg_soc(mgr.udp_soc[i].sid, SOC_TMO_SND, (VP)CFG_UDP_SND_TMO);
            if (ercd == E_OK) {
                ercd = cfg_soc(mgr.udp_soc[i].sid, SOC_TMO_RCV, (VP)CFG_UDP_RCV_TMO);
            }
        } else {
            ercd = E_OBJ;
        }
        if (ercd != E_OK) {
            break;
        }
    }
    if (ercd != E_OK) {
        snmp_ext();
        return E_SYS;
    }
    #endif
    /* Check for device number and socket ID */
    ercd = E_OK;
    for (i = 0U; i < mgr.udp_soc_cnt; i++) {
        if (mgr.udp_soc[i].dev_num == 0U) {
            ercd = E_OBJ;
            break;
        }
        if (mgr.udp_soc[i].sid <= 0) {
            ercd = E_OBJ;
            break;
        }
    }
    if (ercd != E_OK) {
        snmp_ext();
        return E_PAR;
    }

    /* Startup trap enable */
    for (i = 0U; i < CFG_NET_DEV_CNT; i++) {
        mgr.trp_sts[i] = STS_TRP_INI;    /* Clear */
        
        if (CFG_MAX_TRP_CNT > 0U) {
            ercd = E_OBJ;
            id = snmp_soc_idx_trp(i + 1);
            if (id >= 0) {
                j = 0U;
                while (snmp_cfg_trp[j].str != 0) {
                    if (snmp_cfg_trp[j].nod->num == (i + 1)) {
                        ercd = E_OK;
                        break;
                    }
                    j++;
                }
                if (ercd == E_OK) {
                    mgr.trp_sts[i] = STS_TRP_STA_ENA;
                }
            }
        }
    }

    mgr.sts = STS_INI;

    return E_OK;
}

ER snmp_ext(void)
{
    #if !(defined(NET_C_OS) || defined(NET_HW_OS))
    UH i;
    #endif

    /* Termination */

    if ((mgr.sts & STS_ENA) != 0x00U) {
        return E_OBJ;
    }

    /* OS resources */
    #if !(defined(NET_C_OS) || defined(NET_HW_OS))
    if (ID_TSK_RCV > 0) {
        del_tsk(ID_TSK_RCV);
        ID_TSK_RCV = 0;
    }
    if (ID_TSK_TIM > 0) {
        del_tsk(ID_TSK_TIM);
        ID_TSK_TIM = 0;
    }
    if (ID_TSK_TRP > 0) {
        del_tsk(ID_TSK_TRP);
        ID_TSK_TRP = 0;
    }
    if (ID_SEM_MIB > 0) {
        del_sem(ID_SEM_MIB);
        ID_SEM_MIB = 0;
    }
    if (ID_SEM_TRP > 0) {
        del_sem(ID_SEM_TRP);
        ID_SEM_TRP = 0;
    }
    if (ID_FLG_STS > 0) {
        del_flg(ID_FLG_STS);
        ID_FLG_STS = 0;
    }
    if (ID_FLG_TRP > 0) {
        del_flg(ID_FLG_TRP);
        ID_FLG_TRP = 0;
    }
    if (ID_MBX_TRP > 0) {
        del_mbx(ID_MBX_TRP);
        ID_MBX_TRP = 0;
    }
    for (i = 0U; i < mgr.udp_soc_cnt; i++) {
        del_soc(mgr.udp_soc[i].sid);
        mgr.udp_soc[i].sid = 0;
    }
    #endif

    snmp_mib_ext();

    /* Status */
    mgr.sts &= ~STS_INI;

    return E_OK;
}

ER snmp_ena(void)
{
    ER ercd;
    UH i;

    /* Enable */

    if ((mgr.sts & STS_INI) == 0x00U) {
        return E_OBJ;
    }
    if ((mgr.sts & STS_ENA) != 0x00U) {
        return E_OK;
    }

    /* System timer */
    get_tim(&mgr.sys_tim); 

    /* Update trap buffer and status */
    if (CFG_MAX_TRP_CNT > 0U) {
        ercd = snmp_sem_wai(ID_SEM_TRP);
        if (ercd == E_OK) {
            for (i = 0U; i < CFG_MAX_TRP_CNT; i++) {
                mgr.trp_cmd_buf[i].sts = TRP_CMD_INV;
            }
            
            if ((mgr.sts & STS_DIS) == 0x00U) {
                /* Cold start */
                mgr.sts &= ~STS_STA_WARM;
                mgr.sts |= STS_STA_COLD;
            } else {
                /* Warm start */
                mgr.sts &= ~STS_STA_COLD;
                mgr.sts |= STS_STA_WARM;
            }
            for (i = 0U; i < CFG_NET_DEV_CNT; i++) {
                mgr.trp_sts[i] &= ~STS_TRP_STA_SND;
            }
            snmp_sem_sig(ID_SEM_TRP);
        } else {
            return E_OBJ;
        }
    }

    /* Start task */
    ercd = snmp_tsk_sta();
    if (ercd != E_OK) {
        dbg_put_str("Error: snmp_tsk_sta\r\n");
        snmp_tsk_stp();
        return E_SYS;
    }

    ercd = snmp_tcp_ena();
    if (ercd != E_OK) {
        snmp_tsk_stp();
        return E_OBJ;
    }

    mgr.sts &= ~STS_DIS;
    mgr.sts |= STS_ENA;

    /* Start trap */
    snmp_trp_snd_sta_all();

    return E_OK;
}

ER snmp_dis(void)
{
    ER ercd;

    /* Disable */

    if ((mgr.sts & STS_ENA) == 0x00U) {
        return E_OBJ;
    }

    ercd = snmp_tcp_dis();
    if (ercd != E_OK) {
        return E_OBJ;
    }

    ercd = snmp_tsk_stp();
    if (ercd != E_OK) {
        dbg_put_str("Error: snmp_tsk_stp\r\n");
        return E_SYS;
    }

    mgr.sts &= ~STS_ENA;
    mgr.sts |= STS_DIS;

    return E_OK;
}

ER snmp_get_mib_obj(VP buf, UH* len, UH mib_id, UH obj_id)
{
    ER ercd;
    INT typ;
    UH nod_typ;
    VP datp;
    UH dat_len;

    /* Get MIB object */

    if (buf == 0x00) {
        return E_PAR;
    }
    if (len == 0) {
        return E_BOVR;
    }
    if (mib_id >= mgr.mib_ven_cnt) {
        return E_PAR;
    }
    if (obj_id >= snmp_mib_ven[mib_id].cnt) {
        return E_PAR;
    }

    /* Object type */
    ercd = E_OK;
    typ = 0x00;
    switch(snmp_mib_ven[mib_id].mib[obj_id].typ) {
        case TYP_INT:
        case TYP_CNT:
        case TYP_GAUGE:
        case TYP_TIM_TIC:
        case TYP_IP_ADR:
            typ = TYP_INT;
            break;
        case TYP_OCT_STR:
        case TYP_OBJ_ID:
        case TYP_CNT64:
            typ = snmp_mib_ven[mib_id].mib[obj_id].typ;
            break;
        default:
            ercd = E_NOSPT;
            break;
    }
    if (ercd != E_OK) {
        return ercd;
    }

    nod_typ = MIB_NOD_VEN | (mib_id & MIB_NOD_ID);
    ercd = snmp_sem_wai(ID_SEM_MIB);
    if (ercd != E_OK) {
        return ercd;
    }
    ercd = snmp_get_mib_datp(&datp, obj_id, nod_typ, CB_STS_DIS, 0x00);
    if (ercd != E_OK) {
        snmp_sem_sig(ID_SEM_MIB);
        return ercd;
    }
    if (typ == TYP_INT) {
        dat_len = 4;
    } else if (typ == TYP_CNT64) {
        dat_len = 8;
    } else {
        dat_len = snmp_strlen((const char*)datp);
    }
    if (*len < dat_len) {
        snmp_sem_sig(ID_SEM_MIB);
        *len = dat_len;
        return E_BOVR;
    }
    *len = dat_len;

    if (typ == TYP_INT) {
        *((UW*)buf) = (UW)(ADDR)datp;
    } else {
        snmp_strncpy((char*)buf, (const char*)datp, dat_len);
    }
    snmp_sem_sig(ID_SEM_MIB);

    return E_OK;
}

ER snmp_set_mib_obj(VP buf, UH len, UH mib_id, UH obj_id)
{
    ER ercd;
    INT typ;
    UH nod_typ;
    UW dat;

    if (buf == 0x00) {
        return E_PAR;
    }
    if ((len == 0) && (snmp_mib_ven[mib_id].mib[obj_id].typ != TYP_OCT_STR)) {
        return E_OBJ;
    }
    if (mib_id >= mgr.mib_ven_cnt) {
        return E_PAR;
    }
    if (obj_id >= snmp_mib_ven[mib_id].cnt) {
        return E_PAR;
    }

    /* Object type */
    ercd = E_OK;
    typ = 0x00;
    switch(snmp_mib_ven[mib_id].mib[obj_id].typ) {
        case TYP_INT:
        case TYP_CNT:
        case TYP_GAUGE:
        case TYP_TIM_TIC:
        case TYP_IP_ADR:
            typ = TYP_INT;
            break;
        case TYP_OCT_STR:
        case TYP_OBJ_ID:
        case TYP_CNT64:
            typ = snmp_mib_ven[mib_id].mib[obj_id].typ;
            break;
        default:
            ercd = E_NOSPT;
            break;
    }
    if (ercd != E_OK) {
        return ercd;
    }

    nod_typ = MIB_NOD_VEN | (mib_id & MIB_NOD_ID);
    if (typ == TYP_INT) {
        if (len != 4) {
            return E_OBJ;
        }
        dat = *((UW*)buf);
        ercd = snmp_sem_wai(ID_SEM_MIB);
        if (ercd != E_OK) {
            return ercd;
        }
        ercd = snmp_set_mib_dat((VP_UW)dat, len, obj_id, nod_typ, CB_STS_DIS);
    } else {
        ercd = snmp_sem_wai(ID_SEM_MIB);
        if (ercd != E_OK) {
            return ercd;
        }
        ercd = snmp_set_mib_dat(buf, len, obj_id, nod_typ, CB_STS_DIS);
    }
    snmp_sem_sig(ID_SEM_MIB);
    if (ercd != E_OK) {
        return E_OBJ;
    }

    return E_OK;
}

ER snmp_ena_trp(UH trp_bit)
{
    ER ercd;
    T_SNMP_MIB_DAT* mib_dat;

    /* Trap enabled */

    if (CFG_MAX_TRP_CNT == 0U) {
        return E_OBJ;
    }

    ercd = snmp_sem_wai(ID_SEM_TRP);
    if (ercd != E_OK) {
        return ercd;
    }
    mgr.trp_ena |= trp_bit;
    snmp_sem_sig(ID_SEM_TRP);

    /* MIB object for SNMP */
    if (snmp_mib_dat[SNMP_MIB2_DAT_SNMP] != 0x00) {
        if ((trp_bit & SNMP_TRP_AUTH_FAIL_BIT) != 0x00) {
            ercd = snmp_sem_wai(ID_SEM_MIB);
            if (ercd == E_OK) {
                mib_dat = snmp_mib_dat[SNMP_MIB2_DAT_SNMP];
                mib_dat[MIB2_SNMP_OUT_ENA_AUTH_TRAP] = (VP)1;    /* Enable */
                snmp_sem_sig(ID_SEM_MIB);
            }
        }
    }

    return E_OK;
}

ER snmp_dis_trp(UH trp_bit)
{
    ER ercd;
    T_SNMP_MIB_DAT* mib_dat;

    /* Trap disabled */

    if (CFG_MAX_TRP_CNT == 0U) {
        return E_OBJ;
    }

    ercd = snmp_sem_wai(ID_SEM_TRP);
    if (ercd != E_OK) {
        return ercd;
    }
    mgr.trp_ena &= ~trp_bit;
    snmp_sem_sig(ID_SEM_TRP);

    /* MIB object for SNMP */
    if (snmp_mib_dat[SNMP_MIB2_DAT_SNMP] != 0x00) {
        if ((trp_bit & SNMP_TRP_AUTH_FAIL_BIT) != 0x00) {
            ercd = snmp_sem_wai(ID_SEM_MIB);
            if (ercd == E_OK) {
                mib_dat = snmp_mib_dat[SNMP_MIB2_DAT_SNMP];
                mib_dat[MIB2_SNMP_OUT_ENA_AUTH_TRAP] = (VP)0;    /* Disable */
                snmp_sem_sig(ID_SEM_MIB);
            }
        }
    }

    return E_OK;
}

ER snmp_snd_trp(T_NODE* nod, T_SNMP_TRP* trp, TMO tmo)
{
    ER ercd;
    INT soc_idx;
    ID soc_id;
    FLGPTN flg;
    T_SNMP_TRP_CMD* trp_cmd;
    UINT flg_id;
    TMO tmo_res;

    /* Send SNMP trap under user task */

    if (nod == 0x00) {
        return E_PAR;
    }
    if (trp == 0x00) {
        return E_PAR;
    }
    if ((mgr.sts & STS_ENA) == 0x00U) {
        return E_OBJ;
    }
    if (CFG_VEN_TRP_CNT == 0U) {
        return E_OBJ;
    }

    /* Select socket ID */
    soc_idx = snmp_soc_idx_trp(nod->num);
    if (soc_idx < 0) {
        return E_PAR;
    }
    soc_id = mgr.udp_soc[soc_idx].sid;

    /* Get a command buffer */
    ercd = snmp_ven_trp_alc_cmd_buf(&trp_cmd);
    if (ercd != E_OK) {
        return E_QOVR;
    }
    flg_id = trp_cmd->flg_id;

    /* Send a command to trap task */
    clr_flg(ID_FLG_TRP, ~flg_id);
    trp_cmd->soc_id = soc_id;
    trp_cmd->nod_ptr = nod;
    trp_cmd->trp_ptr = trp;
    trp_cmd->soc_tmo = tmo;
    trp_cmd->tmo = 0U;
    trp_cmd->rty_cnt = 0U;
    tmo_res = TMO_FEVR;
    if ((trp->flg & SNMP_TRP_INF_ENA) == 0x00) {
        trp_cmd->sts |= TRP_CMD_SND_TRP;
    } else {
        trp_cmd->sts |= TRP_CMD_SND_INF;
        trp_cmd->tmo = trp->tmo;
        trp_cmd->rty_cnt = trp->rty_cnt;
    }

    ercd = snd_mbx(ID_MBX_TRP, (T_MSG*)trp_cmd);
    if (ercd == E_OK) {
        /* Wait for response */
        flg = (FLGPTN)0x0000U;
        ercd = twai_flg(ID_FLG_TRP, flg_id, TWF_ORW, &flg, tmo_res);
        if (ercd == E_OK) {
            ercd = trp_cmd->ercd;
        } else {
            ercd = E_OBJ;
        }
    } else {
        ercd = E_OBJ;
    }
    snmp_trp_rel_cmd_buf(trp_cmd);

    return ercd;
}

ER snmp_add_val_mib_nod(UH val_mib_id, UH val_obj_id, UH* mib_nod_cnt)
{
    ER ercd;
    UH typ;
    SIZE len;
    T_SNMP_BER_OID ber_oid;
    T_SNMP_MIB** dat_ptr;

    /* Appends MIB node */

    if (val_mib_id >= mgr.mib_ven_val_cnt) {
        return E_PAR;
    }
    if (val_obj_id >= snmp_mib_ven_val[val_mib_id].cnt) {
        return E_PAR;
    }

    typ = MIB_NOD_VEN | (val_mib_id & MIB_NOD_ID);

    ercd = snmp_sem_wai(ID_SEM_MIB);
    if (ercd == E_OK) {
        /* OID string */
        snmp_strcpy(mgr.mib_oid_str_buf, snmp_mib_ven_val[val_mib_id].pre);
        snmp_strcat(mgr.mib_oid_str_buf, snmp_mib_ven_val[val_mib_id].mib[val_obj_id].str);
        len = snmp_strlen(mgr.mib_oid_str_buf);
        if (len <= 65535) {
            /* Retrieve node */
            ber_oid.buf = &mgr.snd_oid_buf[0];
            ber_oid.buf_len = CFG_MAX_MIB_DEP;
            ber_set_oid(&ber_oid, mgr.mib_oid_str_buf, (UH)(len & 0xffffU));
            ercd = snmp_mib_has(&ber_oid);
            if (ercd != E_OK) {
                /* Create new node */
                ercd = snmp_mib_add(mgr.mib_oid_str_buf, (UH)(len & 0xffffU), typ, (VP*)&dat_ptr,
                                    mib_nod_cnt);
                if (ercd == E_OK) {
                    *dat_ptr = (T_SNMP_MIB*)&(snmp_mib_ven_val[val_mib_id].mib[val_obj_id]);
                } else {
                    if (ercd != E_NOMEM) {
                        ercd = E_OBJ;
                    }
                }
            } else {
                ercd = E_QOVR;    /* Node already exists */
            }
        } else {
            ercd = E_OBJ;
        }
        snmp_sem_sig(ID_SEM_MIB);
    } else {
        if (ercd != E_TMOUT) {
            ercd = E_OBJ;
        }
    }

    return ercd;
}

ER snmp_del_val_mib_nod(UH val_mib_id, UH val_obj_id)
{
    ER ercd;
    SIZE len;
    T_SNMP_BER_OID ber_oid;

    /* Delete MIB node */

    if (val_mib_id >= mgr.mib_ven_val_cnt) {
        return E_PAR;
    }
    if (val_obj_id >= snmp_mib_ven_val[val_mib_id].cnt) {
        return E_PAR;
    }

    ercd = snmp_sem_wai(ID_SEM_MIB);
    if (ercd == E_OK) {
        /* OID string */
        snmp_strcpy(mgr.mib_oid_str_buf, snmp_mib_ven_val[val_mib_id].pre);
        snmp_strcat(mgr.mib_oid_str_buf, snmp_mib_ven_val[val_mib_id].mib[val_obj_id].str);
        len = snmp_strlen(mgr.mib_oid_str_buf);
        if (len <= 65535) {
            /* Retrieve node */
            ber_oid.buf = &mgr.snd_oid_buf[0];
            ber_oid.buf_len = CFG_MAX_MIB_DEP;
            ber_set_oid(&ber_oid, mgr.mib_oid_str_buf, (UH)(len & 0xffffU));
            ercd = snmp_mib_has(&ber_oid);
            if (ercd == E_OK) {
                /* Delete node */
                ercd = snmp_mib_del(mgr.mib_oid_str_buf, (UH)(len & 0xffffU), 0);
                if (ercd != E_OK) {
                    ercd = E_OBJ;
                }
            } else {
                ercd = E_NOID;    /* Node not exists */
            }
        } else {
            ercd = E_OBJ;
        }
        snmp_sem_sig(ID_SEM_MIB);
    } else {
        if (ercd != E_TMOUT) {
            ercd = E_OBJ;
        }
    }

    return ercd;
}

ER snmp_get_val_mib_obj(VP buf, UH* len, UH val_mib_id, UH val_obj_id)
{
    ER ercd;
    INT typ;
    UH nod_typ;
    VP datp;
    UH dat_len;

    /* Get object data for vendor variable MIB */

    if (buf == 0) {
        return E_PAR;
    }
    if (len == 0) {
        return E_BOVR;
    }
    if (val_mib_id >= mgr.mib_ven_val_cnt) {
        return E_PAR;
    }
    if (val_obj_id >= snmp_mib_ven_val[val_mib_id].cnt) {
        return E_PAR;
    }

    /* Object type */
    ercd = E_OK;
    typ = 0x00;
    switch(snmp_mib_ven_val[val_mib_id].mib[val_obj_id].typ) {
        case TYP_INT:
        case TYP_CNT:
        case TYP_GAUGE:
        case TYP_TIM_TIC:
        case TYP_IP_ADR:
            typ = TYP_INT;
            break;
        case TYP_OCT_STR:
        case TYP_OBJ_ID:
        case TYP_CNT64:
            typ = snmp_mib_ven_val[val_mib_id].mib[val_obj_id].typ;
            break;
        default:
            ercd = E_NOSPT;
            break;
    }
    if (ercd != E_OK) {
        return ercd;
    }

    nod_typ = MIB_NOD_VAL | MIB_NOD_VEN | (val_mib_id & MIB_NOD_ID);
    ercd = snmp_sem_wai(ID_SEM_MIB);
    if (ercd == E_OK) {
        ercd = snmp_get_mib_datp(&datp, val_obj_id, nod_typ, CB_STS_DIS, 0x00);
        snmp_sem_sig(ID_SEM_MIB);
        if (ercd == E_OK) {
            if (typ == TYP_INT) {
                dat_len = 4;
            } else if (typ == TYP_CNT64) {
                dat_len = 8;
            } else {
                dat_len = snmp_strlen((const char*)datp);
            }
            if (*len >= dat_len) {
                if (typ == TYP_INT) {
                    *((UW*)buf) = (UW)(ADDR)datp;
                } else {
                    snmp_strncpy((char*)buf, (const char*)datp, dat_len);
                }
            } else {
                ercd = E_BOVR;
            }
            *len = dat_len;
        }
    }

    if ((ercd != E_OK) && (ercd != E_BOVR)) {
        ercd = E_OBJ;
    }

    return ercd;
}

ER snmp_set_val_mib_obj(VP buf, UH len, UH val_mib_id, UH val_obj_id)
{
    ER ercd;
    INT typ;
    UH nod_typ;
    UW dat;

    /* Set object data for vendor variable MIB */

    if (buf == 0x00) {
        return E_PAR;
    }
    if ((len == 0) && (snmp_mib_ven_val[val_mib_id].mib[val_obj_id].typ != TYP_OCT_STR)) {
        return E_OBJ;
    }
    if (val_mib_id >= mgr.mib_ven_val_cnt) {
        return E_PAR;
    }
    if (val_obj_id >= snmp_mib_ven_val[val_mib_id].cnt) {
        return E_PAR;
    }

    /* Object type */
    ercd = E_OK;
    typ = 0x00;
    switch(snmp_mib_ven_val[val_mib_id].mib[val_obj_id].typ) {
        case TYP_INT:
        case TYP_CNT:
        case TYP_GAUGE:
        case TYP_TIM_TIC:
        case TYP_IP_ADR:
            typ = TYP_INT;
            break;
        case TYP_OCT_STR:
        case TYP_OBJ_ID:
        case TYP_CNT64:
            typ = snmp_mib_ven_val[val_mib_id].mib[val_obj_id].typ;
            break;
        default:
            ercd = E_NOSPT;
            break;
    }
    if (ercd != E_OK) {
        return ercd;
    }

    nod_typ = MIB_NOD_VAL | MIB_NOD_VEN | (val_mib_id & MIB_NOD_ID);
    if (typ == TYP_INT) {
        if (len == 4) {
            dat = *((UW*)buf);
            ercd = snmp_sem_wai(ID_SEM_MIB);
            if (ercd == E_OK) {
                ercd = snmp_set_mib_dat((VP_UW)dat, len, val_obj_id, nod_typ, CB_STS_DIS);
                snmp_sem_sig(ID_SEM_MIB);
            }
        } else {
            ercd = E_OBJ;
        }
    } else {
        ercd = snmp_sem_wai(ID_SEM_MIB);
        if (ercd == E_OK) {
            ercd = snmp_set_mib_dat(buf, len, val_obj_id, nod_typ, CB_STS_DIS);
            snmp_sem_sig(ID_SEM_MIB);
        }
    }

    if (ercd != E_OK) {
        return E_OBJ;
    }

    return E_OK;
}

