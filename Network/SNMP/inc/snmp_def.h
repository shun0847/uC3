/*
    SNMP
    Definition
    Copyright (c) 2014-2019, eForce Co., Ltd. All rights reserved.
    
    2014-03-06 Created
    2014-06-10 Size of ifPhysAddress was fixed
    2016-01-06 Multiple network devices were supported
    2016-04-18 snmp_add_val_mib_nod and snmp_del_val_mib_nod were added
    2019-02-25 Add some definition.(E_NA, ERR_AUTH_ERR)
*/

#ifndef SNMP_DEF_H
#define SNMP_DEF_H

/* Internal error */
#define E_BASE      -130    /* Base of internal error */
#define E_RO        -132    /* Read only */
#define E_TYP       -133    /* Type unmatched */
#define E_LEN       -134    /* Length error */
#define E_ENC       -135    /* Encode error */
#define E_DAT       -136    /* Data error */
#define E_COMMIT    -137    /* Commit error */
#define E_UNDO      -138    /* Undone error*/
#define E_NOINS     -139    /* No instance */
#define E_NA        -140    /* No access */

/* Internal type */
typedef VP    VP_UW;    /* VP or UW */

/* MIB object type */
#define TYP_UNI         0x00    /* Universal */
#define TYP_APL         0x40    /* Application */
#define TYP_CTX         0x80    /* Context-specific */
#define TYP_PRI         0xc0    /* Private */

#define TYP_GET_REQ     0xA0    /* Get request */
#define TYP_GET_NXT     0xA1    /* Get next request */
#define TYP_GET_RES     0xA2    /* Get response */
#define TYP_SET_REQ     0xA3    /* Set request */
#define TYP_TRP         0xA4    /* Trap */
#define TYP_GET_BULK    0xA5    /* GetBulkRequest */
#define TYP_INF_REQ     0xA6    /* InformRequest */
#define TYP_V2_TRP      0xA7    /* Trap v2c */

/* MIB node type */
#define MIB_NOD_TYP     0x0f00    /* Type */
#define MIB_NOD_FLG     0xf000    /* Flag */
#define MIB_NOD_ID      0x00ff    /* ID */
#define MIB_NOD_INV     0x0000    /* Invalid */
#define MIB_NOD_STD     0x0100    /* Standard */
#define MIB_NOD_VEN     0x0200    /* Vendor */
#define MIB_NOD_VAL     0x0800    /* Variable */
#define MIB_NOD_TBL     0x2000    /* Table item */

/* MIB socket data status */
#define MIB_SOC_INV     0x00    /* Invalid */
#define MIB_SOC_ENA     0x01    /* Enabled */
#define MIB_SOC_TCP     0x10    /* TCP socket */
#define MIB_SOC_UDP     0x20    /* UDP socket */

/* Callback status */
#define CB_STS_DIS      0       /* Disabled */
#define CB_STS_ENA      1       /* Enabled */

/* Error status */
#define ERR_NO_ERR          0       /* noError */
#define ERR_TOO_BIG         1       /* tooBig */
#define ERR_NO_SUCH_NAME    2       /* noSuchName */
#define ERR_BAD_VAL         3       /* badValue */
#define ERR_READ_ONLY       4       /* readOnly */
#define ERR_GEN_ERR         5       /* GenError */
#define ERR_NO_ACS          6       /* noAccess */
#define ERR_WRONG_TYP       7       /* wrongType */
#define ERR_WRONG_LEN       8       /* wrongLength */
#define ERR_WRONG_ECODING   9       /* wrongEcoding */
#define ERR_WRONG_VAL       10      /* wrongValue */
#define ERR_NO_CREATION     11      /* noCreation */
#define ERR_INCONSIST_VAL   12      /* inconsistentValue */
#define ERR_RSRC_UNAVAL     13      /* resourceUnavailable */
#define ERR_COMMIT_FAILD    14      /* commitFaild */
#define ERR_UNDO_FAILD      15      /* undoFaild */
#define ERR_AUTH_ERR        16      /* authorizationError */
#define ERR_NOT_WRITABLE    17      /* notWriteable */
#define ERR_INCONSIST_NAME  18      /* inconsistentName */

/* Physical address size */
#define MAC_ADR_LEN    6

/* Socket resource */
typedef struct t_snmp_udp_soc {
    ID sid;        /* Socket ID */
    UH dev_num;    /* Device number */
    UH sts;        /* Status */
} T_SNMP_UDP_SOC;

/* TCP configuration */
typedef struct t_snmp_cfg_tcp {
    UH max_soc_cnt;    /* Number of sockets */
    UH tcp_soc_cnt;    /* Number of TCP sockets */
    UH arp_dat_cnt;    /* Number of ARP data buffer */
    UH tcp_dat_cnt;    /* Number of TCP data buffer */
} T_SNMP_CFG_TCP;

/* Variable MIB data TCP */
typedef struct t_snmp_mib_tcp_dat {
    VP buf;     /* Object data */
    UH sts;     /* Status */
    UB typ;     /* Type */
    UB acs;     /* Accsess */
} T_SNMP_MIB_TCP_DAT;

/* Variable MIB data ARP */
typedef struct t_snmp_mib_arp_dat {
    UB mac[MAC_ADR_LEN + 1];    /* MAC address */
} T_SNMP_MIB_ARP_DAT;

/* OID string */
typedef struct t_snmp_oid_dat {
    VB* buf;    /* Data buffer */
    UH len;     /* Size */
} T_SNMP_OID_DAT;

/* MIB node */
typedef struct t_snmp_mib_nod {
    VP parent;      /* Parent node */
    VP child;       /* Child node */
    VP next;        /* Brother node */
    VP prev;        /* Pre-brother node */
    VP dat;         /* Pointer of MIB array */
    UW num;         /* Branch OID number */
    UH typ;         /* Type */
} T_SNMP_MIB_NOD;

/* Variable binding */
typedef struct t_snmp_msg_var {
    T_SNMP_BER_ANY tag;    /* Tag */
    T_SNMP_BER_OID oid;    /* OID */
    T_SNMP_BER_BUF dat;    /* Data */
} T_SNMP_MSG_VAR;

/* Message data v2 */
typedef struct t_snmp_msg_v2 {
    T_SNMP_BER_ANY tag;        /* Tag */
    T_SNMP_BER_INT ver;        /* Version */
    T_SNMP_BER_OCT com;        /* Community */
    T_SNMP_BER_ANY dat;        /* Data */
    T_SNMP_BER_INT req_id;     /* Request ID */
    T_SNMP_BER_INT err_sts;    /* Error status */
    T_SNMP_BER_INT err_idx;    /* Error index */
    T_SNMP_BER_ANY var_tag;    /* Variable bindings tag */
    T_SNMP_MSG_VAR* var;       /* Variable bindings */
    UH var_cnt;                /* Number of variable data */
} T_SNMP_MSG_V2;

/* Trap message data v1 */
typedef struct t_snmp_trp_msg_v1 {
    T_SNMP_BER_ANY tag;        /* Tag */
    T_SNMP_BER_INT ver;        /* Version */
    T_SNMP_BER_OCT com;        /* Community */
    T_SNMP_BER_ANY trp;        /* Trap */
    T_SNMP_BER_OID ent_oid;    /* Enterprise OID */
    T_SNMP_BER_IP ip_adr;      /* IP address */
    T_SNMP_BER_INT gen_trp;    /* Generic trap */
    T_SNMP_BER_INT spc_trp;    /* Specific trap */
    T_SNMP_BER_TIM tim;        /* Timestamp */
    T_SNMP_BER_ANY var_tag;    /* Variable bindings tag */
    T_SNMP_MSG_VAR* var;       /* Variable bindings */
    UH var_cnt;                /* Number of variable data */
} T_SNMP_TRP_MSG_V1;

/* Message buffer */
typedef struct t_snmp_msg_buf {
    union {
        T_SNMP_MSG_V2     dmy0;
        T_SNMP_TRP_MSG_V1 dmy1;
    } snmp_msg_buf_all;
} T_SNMP_MSG_BUF;

/* Trap command */
typedef struct t_snmp_trp_cmd {
    T_MSG head;             /* Header area of mailbox */
    T_NODE* nod_ptr;        /* Remote node */
    T_SNMP_TRP* trp_ptr;    /* User trap command */
    T_SNMP_MSG_BUF* msg;    /* Message buffer */
    T_NODE nod;             /* Remote node buffer */
    T_SNMP_TRP trp;         /* User trap command buffer */
    VP buf;                 /* Encode buffer */
    UW sys_tim;             /* System uptime (Inform Request) */
    FLGPTN flg_id;          /* Bitmap ID */
    UINT abt_id;            /* Abort ID */
    ID soc_id;              /* Socket ID */
    TMO soc_tmo;            /* Socket timeout */
    INT req_id;             /* Request ID (Inform Request) */
    ER ercd;                /* Error code */
    UH dev_num;             /* Device number for link status */
    UH buf_len;             /* Encode buffer size */
    UH tmo;                 /* Timeout (Inform Request) */
    UH rty_cnt;             /* Number of retries (Inform Request) */
    UH sts;                 /* Status flag */
} T_SNMP_TRP_CMD;

/* Function Prototypes */
ER snmp_sem_wai(ID);
void snmp_sem_sig(ID);
INT snmp_soc_idx_trp(UH);
ER snmp_trp_snd_on_tsk(INT, UH);
ER snmp_trp_snd_sta(UH);
ER snmp_trp_snd_lnk(INT, UH);
ER snmp_get_mib_datp(VP_UW*, UH, UH, UH, UH*);
ER snmp_set_mib_dat(VP_UW, UH, UH, UH, UH);
ER snmp_set_itf_last_chg(UH);
ER snmp_set_sys_tim(void);
ER snmp_mib_ini(INT*);
ER snmp_mib_ext(void);
ER snmp_mib_cre(VP, VP, UH, UH*);
ER snmp_mib_has(T_SNMP_BER_OID*);
ER snmp_mib_add(VB*, UH, UH, VP*, UH*);
ER snmp_mib_del(VB*, UH, VP*);
ER snmp_mib_get_ber_dat_oid(T_SNMP_BER_BUF*, T_SNMP_BER_OID*, UH);
ER snmp_mib_get_ber_dat_id(T_SNMP_BER_BUF*, UH, UH);
ER snmp_mib_set_ber_dat(T_SNMP_BER*, T_SNMP_BER_OID*, T_SNMP_MIB_NOD**, VP);
ER snmp_mib_set_dat_oid(VP, T_SNMP_BER_OID*);
ER snmp_mib_get_oid_id(T_SNMP_BER_OID*, UH, UH, VP);
ER snmp_tcp_ini(ID, VB*, UH);
ER snmp_tcp_ena(void);
ER snmp_tcp_dis(void);
ER snmp_tcp_nod_cnt(UH*);
ER snmp_tcp_get_eth_lnk(UH, UH);
ER snmp_tcp_get_ip(T_NODE*, UH);

#endif

