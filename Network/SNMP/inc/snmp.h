/*
    SNMP
    Header file
    Copyright (c) 2014-2019, eForce Co., Ltd. All rights reserved.
    
    2014-03-06 Created
    2015-02-19 Divided trap socket and modified timeout trap process
    2015-04-21 SNMP_REQ_SET_SYS was added
    2016-01-06 Multiple network devices were supported
    2016-04-18 snmp_add_val_mib_nod and snmp_del_val_mib_nod were added
    2017-04-20 Support OID type for private MIB data
    2019-02-25 Support Counter64(SNMP_TYP_CNT64) type for private MIB data.
               Support accessible-for-notify(SNMP_STS_AN) status for private MIB data.
    2019-05-07 Added API. (snmp_cfg(), snmp_ref())
               Created an event notification callback.
*/

#ifndef SNMP_H
#define SNMP_H

#include "net_hdr.h"

/* Module version */
#define SNMP_AGT_MOD_VER    0x0200U

/* Function */
#define SNMP_AGT        0x01    /* Agent */

/* Version */
#define SNMP_VER_V1     0    /* v1 */
#define SNMP_VER_V2C    1    /* v2c */

/* Type of basic encoding rules */
#define SNMP_TYP_NONE       0x0000    /* None */
#define SNMP_TYP_INT        0x0002    /* Integer */
#define SNMP_TYP_OCT_STR    0x0004    /* Octet string */
#define SNMP_TYP_OBJ_ID     0x0006    /* Object identifier */
#define SNMP_TYP_SEQ        0x0030    /* Sequence */
#define SNMP_TYP_IP_ADR     0x0040    /* IP address */
#define SNMP_TYP_CNT        0x0041    /* Counter (32) */
#define SNMP_TYP_GAUGE      0x0042    /* Gauge (32) */
#define SNMP_TYP_TIM_TIC    0x0043    /* Time ticks */
/*#define SNMP_TYP_OPAQUE     0x0044  */  /* Opaque */
#define SNMP_TYP_CNT64      0x0046    /* Counter (64) */

/* Access status */
#define SNMP_STS_NO    0x01U        /* Not accessible */
#define SNMP_STS_RO    0x02U        /* Read only */
#define SNMP_STS_WO    0x04U        /* Write only */
#define SNMP_STS_RW    0x06U        /* Read and write */
#define SNMP_STS_AN    0x11U        /* accessible-for-notify */


/* Generic trap type */
#define SNMP_TRP_COLD_STA      0    /* coldStart */
#define SNMP_TRP_WARM_STA      1    /* warmStart */
#define SNMP_TRP_LINK_DOWN     2    /* linkDown */
#define SNMP_TRP_LINK_UP       3    /* linkUp */
#define SNMP_TRP_AUTH_FAIL     4    /* authenticationFailure */
#define SNMP_TRP_EPG_LOSS      5    /* egpNeighborLoss */
#define SNMP_TRP_ENT_SPEC      6    /* enterpriseSpecific */

/* Generic trap bitmap */
#define SNMP_TRP_COLD_STA_BIT       0x0001    /* coldStart */
#define SNMP_TRP_WARM_STA_BIT       0x0002    /* warmStart */
#define SNMP_TRP_LINK_DOWN_BIT      0x0004    /* linkDown */
#define SNMP_TRP_LINK_UP_BIT        0x0008    /* linkUp */
#define SNMP_TRP_AUTH_FAIL_BIT      0x0010    /* authenticationFailure */
#define SNMP_TRP_EPG_LOSS_BIT       0x0020    /* egpNeighborLoss */
#define SNMP_TRP_ALL_BIT            0x003f    /* All */

/* Trap command flag */
#define SNMP_TRP_GEN_ENA    0x0010     /* Generic trap number enable */
#define SNMP_TRP_INF_ENA    0x0200     /* Inform request enable */

/* Request type for callback function */
#define SNMP_REQ_GET    0x0001    /* Get request */
#define SNMP_REQ_SET    0x0002    /* Set request */

#define SNMP_REQ_SET_SYS    0x0102     /* System group set request */
#define SNMP_REQ_GET_VAL    0x0401     /* Variable vendor MIB set request */
#define SNMP_REQ_SET_VAL    0x0402     /* Variable vendor MIB get request */

/* Macros for trap */
#define SNMP_TRP_VAR_ID(x, y)    ((VP)(((UH)(x) & 0x00ff) << 16 | (UH)(y)))

/* Event for callback function */
#define SNMP_EVT_ERROR          0x1000
#define SNMP_EVT_SET_START      0x0001
#define SNMP_EVT_SET_END        0x0002
#define SNMP_EVT_SET_ERROR      (SNMP_EVT_SET_END | SNMP_EVT_ERROR)
#define SNMP_EVT_GET_START      0x0010
#define SNMP_EVT_GET_END        0x0020
#define SNMP_EVT_GET_ERROR      (SNMP_EVT_GET_END | SNMP_EVT_ERROR)

/* snmp_cfg/snmp_ref parameters */
#define SNMP_EVT_CBK            1

/* Error status for SNMP_EVT_ERROR event (same as ERR_xx_xxx) */
#define SNMP_EVERR_NO_ERR          0       /* noError */
#define SNMP_EVERR_TOO_BIG         1       /* tooBig */
#define SNMP_EVERR_NO_SUCH_NAME    2       /* noSuchName */
#define SNMP_EVERR_BAD_VAL         3       /* badValue */
#define SNMP_EVERR_READ_ONLY       4       /* readOnly */
#define SNMP_EVERR_GEN_ERR         5       /* GenError */
#define SNMP_EVERR_NO_ACS          6       /* noAccess */
#define SNMP_EVERR_WRONG_TYP       7       /* wrongType */
#define SNMP_EVERR_WRONG_LEN       8       /* wrongLength */
#define SNMP_EVERR_WRONG_ECODING   9       /* wrongEcoding */
#define SNMP_EVERR_WRONG_VAL       10      /* wrongValue */
#define SNMP_EVERR_NO_CREATION     11      /* noCreation */
#define SNMP_EVERR_INCONSIST_VAL   12      /* inconsistentValue */
#define SNMP_EVERR_RSRC_UNAVAL     13      /* resourceUnavailable */
#define SNMP_EVERR_COMMIT_FAILD    14      /* commitFaild */
#define SNMP_EVERR_UNDO_FAILD      15      /* undoFaild */
#define SNMP_EVERR_AUTH_ERR        16      /* authorizationError */
#define SNMP_EVERR_NOT_WRITABLE    17      /* notWriteable */
#define SNMP_EVERR_INCONSIST_NAME  18      /* inconsistentName */


/* Manager */
typedef struct t_snmp_cfg_mgr {
    T_NODE* nod;    /* Remote node */
} T_SNMP_CFG_MGR;

/* Community */
typedef struct t_snmp_cfg_com {
    VB* str;        /* Community strings */
    UB sts;         /* Access status */
} T_SNMP_CFG_COM;

/* Trap */
typedef struct t_snmp_cfg_trp {
    VB* str;        /* Community strings */
    T_NODE* nod;    /* Remote node */
    UB ver;         /* Protocol version */
    ID id;          /* ID (Reserve) */
} T_SNMP_CFG_TRP;

/* Data block of callback function */
typedef struct t_snmp_cfg_cbk_dat {
    VP buf;         /* Data pointer */
    UH req;         /* SNMP request type */
    UH mib_id;      /* Vendor MIB ID */
    UH obj_id;      /* MIB object ID */
    UH typ;         /* Object type */
    UH dat_len;     /* Data size (byte) */
    UH buf_len;     /* Buffer size (byte) */
} T_SNMP_CFG_CBK_DAT;

/* Callback functions */
typedef struct t_snmp_cfg_cbk {
    ER (*fnc)(T_SNMP_CFG_CBK_DAT*);
} T_SNMP_CFG_CBK;

/* OS and network configuration values */
typedef struct t_snmp_cfg_os {
    ID id_tsk_rcv;    /* Task for receiving message (ID_SNMP_TSK_RCV) */
    ID id_tsk_tim;    /* Task for timer             (ID_SNMP_TSK_TIM) */
    ID id_tsk_trp;    /* Task for trap              (ID_SNMP_TSK_TRP) */
    ID id_sem_mib;    /* Semaphore for MIB          (ID_SNMP_SEM_MIB) */
    ID id_sem_trp;    /* Semaphore for trap         (ID_SNMP_SEM_TRP) */
    ID id_flg_sts;    /* Flag for status            (ID_SNMP_FLG_STS) */
    ID id_flg_trp;    /* Flag for trap              (ID_SNMP_FLG_TRP) */
    ID id_mbx_trp;    /* Mailbox for trap           (ID_SNMP_MBX_TRP) */
} T_SNMP_CFG_OS;

/* Network configuration values */
typedef struct t_snmp_cfg_net {
    UH dev_num;       /* Network device number (1...65535) */
    ID id_udp_soc;    /* UDP socket            (ID_SNMP_UDP_SOCx) */
    ID id_trp_soc;    /* UDP socket for trap   (ID_SNMP_UDP_TRP_SOCx) */
} T_SNMP_CFG_NET;

/* User configuration values */
typedef struct t_snmp_cfg_usr {
    UH net_dev_cnt;         /* Number of network devices */
    UH net_use_cnt;         /* Number of network devices for SNMP */
    TMO udp_snd_tmo;        /* UDP socket send timeout */
    TMO udp_rcv_tmo;        /* UDP socket receive timeout */
    TMO sem_tmo;            /* Semaphore timeput period */
    UH fnc_flg;             /* Function flags */
    UH max_trp_cnt;         /* Number of traps at any time */
    UH ven_trp_cnt;         /* Number of vendor traps at any time */
    UH snd_msg_len;         /* Maximum size of an SNMP message can send */
    UH rcv_msg_len;         /* Maximum size of an SNMP message can receive */
    UH msg_var_cnt;         /* Maximum number of variable bindings */
    UH mib_nod_cnt;         /* Number of nodes in the MIB tree */
    UH max_mib_dep;         /* Maximum depth of the MIB tree */
    UH max_mib_dat;         /* Maximum size of the MIB data */
    UH gen_trp_ena;         /* Generic trap enabled */
    UH max_oid_dep;         /* Maximum number of OID objects as MIB data */
} T_SNMP_CFG_USR;

/* MIB object */
typedef struct t_snmp_mib {
    const VB* str;  /* Object string */
    UH len;         /* Size (byte) */
    UB typ;         /* Type */
    UB acs;         /* Accsess */
} T_SNMP_MIB;

/* MIB data or data pointer */
typedef VP T_SNMP_MIB_DAT;

/* Vendor MIB table */
typedef struct t_snmp_mib_tbl {
    const VB* pre;                  /* Prefix */
    const T_SNMP_MIB* mib;          /* Objects */
    T_SNMP_MIB_DAT* dat;            /* Data */
    const T_SNMP_CFG_CBK* cbk;      /* Callback functions */
    UH cnt;                         /* Reserve */
} T_SNMP_MIB_TBL;

/* Trap command */
typedef struct t_snmp_trp {
    VB* com;        /* Community */
    VB* ent_oid;    /* OID of Enterprise(v1) or snmpTrapOID(v2c) */
    VP var_oid;     /* ID(s) of variable binding OID */
    INT gen_trp;    /* Generic trap number(Only for v1) */
    INT spc_trp;    /* Specific trap number(Only for v1) */
    UH var_cnt;     /* Number of variable bindings */
    UH tmo;         /* Timeout (Only for InformRequest) */
    UH rty_cnt;     /* Number of retries (Only for InformRequest) */
    UH flg;         /* Option flag */
    UB ver;         /* Protocol version */
} T_SNMP_TRP;

typedef unsigned long long  UD_SNMP;     /* 64bit data */


/* Function Prototypes */
ER snmp_ini(UH*);                               /* Initialization */
ER snmp_ext(void);                              /* Termination */
ER snmp_ena(void);                              /* Enable */
ER snmp_dis(void);                              /* Disable */
ER snmp_get_mib_obj(VP, UH*, UH, UH);           /* Get MIB object */
ER snmp_set_mib_obj(VP, UH, UH, UH);            /* Set MIB object */
ER snmp_ena_trp(UH);                            /* Generic trap enable */
ER snmp_dis_trp(UH);                            /* Generic trap disable */
ER snmp_snd_trp(T_NODE*, T_SNMP_TRP*, TMO);     /* Send trap */
ER snmp_add_val_mib_nod(UH, UH, UH*);           /* Vendor variable MIB Add MIB node */
ER snmp_del_val_mib_nod(UH, UH);                /* Vendor variable MIB Delete MIB node */
ER snmp_get_val_mib_obj(VP, UH*, UH, UH);       /* Vendor variable MIB Get MIB object */
ER snmp_set_val_mib_obj(VP, UH, UH, UH);        /* Vendor variable MIB Set MIB object */
ER snmp_cfg(UB code, VP val);
ER snmp_ref(UB code, VP val);

/* Conversion macros */
#ifndef CFG_SNMP_CNV_DIS
#define TYP_NONE        SNMP_TYP_NONE
#define TYP_INT         SNMP_TYP_INT
#define TYP_OCT_STR     SNMP_TYP_OCT_STR
#define TYP_OBJ_ID      SNMP_TYP_OBJ_ID
#define TYP_SEQ         SNMP_TYP_SEQ
#define TYP_IP_ADR      SNMP_TYP_IP_ADR
#define TYP_CNT         SNMP_TYP_CNT
#define TYP_GAUGE       SNMP_TYP_GAUGE
#define TYP_TIM_TIC     SNMP_TYP_TIM_TIC
/*#define TYP_OPAQUE      SNMP_TYP_OPAQUE*/
#define TYP_CNT64       SNMP_TYP_CNT64
#define STS_NO          SNMP_STS_NO
#define STS_RO          SNMP_STS_RO
#define STS_RW          SNMP_STS_RW
#define STS_WO          SNMP_STS_WO
#define STS_AN          SNMP_STS_AN
#define TRP_COLD_STA    SNMP_TRP_COLD_STA
#define TRP_WARM_STA    SNMP_TRP_WARM_STA
#define TRP_LINK_DOWN   SNMP_TRP_LINK_DOWN
#define TRP_LINK_UP     SNMP_TRP_LINK_UP
#define TRP_AUTH_FAIL   SNMP_TRP_AUTH_FAIL
#define TRP_EPG_LOSS    SNMP_TRP_EPG_LOSS
#define TRP_ENT_SPEC    SNMP_TRP_ENT_SPEC
#define COLD_STA_BIT    SNMP_TRP_COLD_STA_BIT
#define WARM_STA_BIT    SNMP_TRP_WARM_STA_BIT
#define LINK_DOWN_BIT   SNMP_TRP_LINK_DOWN_BIT
#define LINK_UP_BIT     SNMP_TRP_LINK_UP_BIT
#define AUTH_FAIL_BIT   SNMP_TRP_AUTH_FAIL_BIT
#define EPG_LOSS_BIT    SNMP_TRP_EPG_LOSS_BIT
#define TRP_ALL_BIT     SNMP_TRP_ALL_BIT
#define TRP_OID_STR     SNMP_TRP_OID_STR
#define TRP_GEN_ENA     SNMP_TRP_GEN_ENA
#define TRP_INF_ENA     SNMP_TRP_INF_ENA
#define TRP_VAR_ID      SNMP_TRP_VAR_ID
#define SNMP_TRP        T_SNMP_TRP
#define get_mib_obj     snmp_get_mib_obj
#define set_mib_obj     snmp_set_mib_obj
#define ena_trp         snmp_ena_trp
#define dis_trp         snmp_dis_trp
#define snd_trp         snmp_snd_trp
#define add_val_mib_nod snmp_add_val_mib_nod
#define del_val_mib_nod snmp_del_val_mib_nod
#define get_val_mib_obj snmp_get_val_mib_obj
#define set_val_mib_obj snmp_set_val_mib_obj
#endif

#endif

