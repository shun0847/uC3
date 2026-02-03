/*
    SNMP
    MIB for TCP/IP stack
    Copyright (c) 2014-2023, eForce Co., Ltd. All rights reserved.
    
    2014-03-06 Created
    2014-05-15 Total number of nodes was fixed
    2014-06-17 Number of TCP buffer was corrected
    2014-06-18 Compact OS was supported
    2016-01-06 Multiple network devices were supported
    2016-04-18 snmp_add_val_mib_nod and snmp_del_val_mib_nod were added
    2019-12-10 Fixed the operation of snmp_tcp_get_eth_lnk().
    2023-05-25 Suppression of warnings in 64bit environment.
*/

#include "kernel.h"
#include "snmp.h"
#include "snmp_ber.h"
#include "snmp_lib.h"
#include "snmp_def.h"
#include "snmp_mib.h"
#include "snmp_net.h"
#include "net_def.h"

extern const T_SNMP_CFG_USR snmp_cfg_usr;
extern const T_SNMP_CFG_TCP snmp_cfg_tcp;
extern UW snmp_cfg_tcp_buf[];
extern const VB snmp_mib_mib2_pre[];
extern UH snmp_mib_dat_cnt[SNMP_MIB2_CNT_GRP];
extern UW NET_DEV_MAX;
extern UW NET_SOC_MAX;
extern UW NET_TCP_MAX;
extern T_NET_ARP gNET_ARP[];    /* ARP table in TCP stack */

/* default configuration */
#if !defined(CFG_SNMP_GETETHLINK_R201912)
#define CFG_SNMP_GETETHLINK_R201912		1
#endif


/* Debug configuration */
#if defined(CFG_SNMP_DBG_ENA)
#define CFG_DBG_ENA    1
#else
#define CFG_DBG_ENA    0
#endif
#if (CFG_DBG_ENA == 1)
#if !defined(CFG_SNMP_DBG_PTF)
extern void apl_put_str(VB const*);
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
#define CFG_NET_DEV_CNT     snmp_cfg_usr.net_dev_cnt
#define CFG_MAX_MIB_DEP     snmp_cfg_usr.max_mib_dep

#define CFG_MAX_SOC_CNT     snmp_cfg_tcp.max_soc_cnt
#define CFG_MAX_TCP_CNT     snmp_cfg_tcp.tcp_soc_cnt
#define CFG_ARP_DAT_CNT     snmp_cfg_tcp.arp_dat_cnt
#define CFG_TCP_DAT_CNT     snmp_cfg_tcp.tcp_dat_cnt

/* Macros */
#define BUF_UW_LEN(x)    ((x + (sizeof(UW) - 1)) / sizeof(UW))

/* ARP entry state */
#define ARP_ENT_OTHER      1    /* other */
#define ARP_ENT_INV        2    /* invalid */
#define ARP_ENT_DYNAMIC    3    /* dynamic */
#define ARP_ENT_STATIC     4    /* static */

/* TCP socket state */
#define TCP_SOC_CLS         1     /* closed */
#define TCP_SOC_LIS         2     /* listen */
#define TCP_SOC_SYN_SENT    3     /* synSent */
#define TCP_SOC_SYN_RCV     4     /* synReceived */
#define TCP_SOC_EST         5     /* established */
#define TCP_SOC_FIN_WAI1    6     /* finWait1 */
#define TCP_SOC_FIN_WAI2    7     /* finWait2 */
#define TCP_SOC_CLS_WAI     8     /* closeWait */
#define TCP_SOC_LAST_ACK    9     /* lastAck */
#define TCP_SOC_CLS_ING     10    /* closing */
#define TCP_SOC_TIM_WAI     11    /* timeWait */
#define TCP_SOC_DEL_TCB     12    /* deleteTCB */

/* Other macros */
#define NET_DEV_NUM    0          /* Network device number */
#define LEN_INT        4          /* Length of INT,CNT,GAUGE,IP_ADR */
#define LEN_MAC        7          /* Length of MAC address with null */

/* Constant variables */
/* IP address table */
static const VB snmp_tcp_mib_4_20_1_1[] =  "4.20.1.1";    /* ipAdEntAddr */
static const VB snmp_tcp_mib_4_20_1_2[] =  "4.20.1.2";    /* ipAdEntIfIndex */
static const VB snmp_tcp_mib_4_20_1_3[] =  "4.20.1.3";    /* ipAdEntNetMask */
static const VB snmp_tcp_mib_4_20_1_4[] =  "4.20.1.4";    /* ipAdEntBcastAddr */
static const VB snmp_tcp_mib_4_20_1_5[] =  "4.20.1.5";    /* ipAdEntReasmMaxSize */
/* IP arp table */
static const VB snmp_tcp_mib_4_22_1_1[] =  "4.22.1.1";    /* ipNetToMediaIfIndex */
static const VB snmp_tcp_mib_4_22_1_2[] =  "4.22.1.2";    /* ipNetToMediaPhysAddress */
static const VB snmp_tcp_mib_4_22_1_3[] =  "4.22.1.3";    /* ipNetToMediaNetAddress */
static const VB snmp_tcp_mib_4_22_1_4[] =  "4.22.1.4";    /* ipNetToMediaType */
/* AT arp table */
static const VB snmp_tcp_mib_3_1_1_1[] =  "3.1.1.1";      /* atIfIndex */
static const VB snmp_tcp_mib_3_1_1_2[] =  "3.1.1.2";      /* atPhysAddress */
static const VB snmp_tcp_mib_3_1_1_3[] =  "3.1.1.3";      /* atNetAddress */
/* TCP table */
static const VB snmp_tcp_mib_6_13_1_1[] = "6.13.1.1";     /* tcpConnState */
static const VB snmp_tcp_mib_6_13_1_2[] = "6.13.1.2";     /* tcpConnLocalAddress */
static const VB snmp_tcp_mib_6_13_1_3[] = "6.13.1.3";     /* tcpConnLocalPort */
static const VB snmp_tcp_mib_6_13_1_4[] = "6.13.1.4";     /* tcpConnRemAddress */
static const VB snmp_tcp_mib_6_13_1_5[] = "6.13.1.5";     /* tcpConnRemPort */
/* UDP table */
static const VB snmp_tcp_mib_7_5_1_1[] = "7.5.1.1";       /* udpLocalAddress */
static const VB snmp_tcp_mib_7_5_1_2[] = "7.5.1.2";       /* udpLocalPort */

/* MIB data */
/* IP address table */
#define MIB_TBL_IP_ADR_CNT    5
static const T_SNMP_MIB snmp_tcp_mib_ip_adr[MIB_TBL_IP_ADR_CNT] = {
    {snmp_tcp_mib_4_20_1_1, LEN_INT,    TYP_IP_ADR,  STS_RO},    /* ipAdEntAddr         (IpAddress) */
    {snmp_tcp_mib_4_20_1_2, LEN_INT,    TYP_INT,     STS_RO},    /* ipAdEntIfIndex      (Integer) */
    {snmp_tcp_mib_4_20_1_3, LEN_INT,    TYP_IP_ADR,  STS_RO},    /* ipAdEntNetMask      (IpAddress) */
    {snmp_tcp_mib_4_20_1_4, LEN_INT,    TYP_INT,     STS_RO},    /* ipAdEntBcastAddr    (Integer[0..1]) */
    {snmp_tcp_mib_4_20_1_5, LEN_INT,    TYP_INT,     STS_RO},    /* ipAdEntReasmMaxSize (Integer) */
};
/* IP arp table */
#define MIB_TBL_IP_ARP_CNT    4
static const T_SNMP_MIB snmp_tcp_mib_ip_arp[MIB_TBL_IP_ARP_CNT] = {
    {snmp_tcp_mib_4_22_1_1, LEN_INT,    TYP_INT,     STS_RO},    /* ipNetToMediaIfIndex     (Integer) */
    {snmp_tcp_mib_4_22_1_2, LEN_MAC,    TYP_OCT_STR, STS_RO},    /* ipNetToMediaPhysAddress (PhysAddress) */
    {snmp_tcp_mib_4_22_1_3, LEN_INT,    TYP_IP_ADR,  STS_RO},    /* ipNetToMediaNetAddress  (IpAddress) */
    {snmp_tcp_mib_4_22_1_4, LEN_INT,    TYP_INT,     STS_RO},    /* ipNetToMediaType        (Integer[1..4]) */
};
/* AT arp table */
#define MIB_TBL_AT_ARP_CNT    3
static const T_SNMP_MIB snmp_tcp_mib_at_arp[MIB_TBL_AT_ARP_CNT] = {
    {snmp_tcp_mib_3_1_1_1,  LEN_INT,    TYP_INT,     STS_RO},    /* atIfIndex     (Integer) */
    {snmp_tcp_mib_3_1_1_2,  LEN_MAC,    TYP_OCT_STR, STS_RO},    /* atPhysAddress (PhysAddress) */
    {snmp_tcp_mib_3_1_1_3,  LEN_INT,    TYP_IP_ADR,  STS_RO},    /* atNetAddress  (IpAddress) */
};
/* TCP table */
#define MIB_TBL_TCP_PORT_CNT    5
static const T_SNMP_MIB snmp_tcp_mib_tcp_port[MIB_TBL_TCP_PORT_CNT] = {
    {snmp_tcp_mib_6_13_1_1, LEN_INT,    TYP_INT,     STS_RO},    /* tcpConnState        (Integer[1..12]) */
    {snmp_tcp_mib_6_13_1_2, LEN_INT,    TYP_IP_ADR,  STS_RO},    /* tcpConnLocalAddress (IpAddress) */
    {snmp_tcp_mib_6_13_1_3, LEN_INT,    TYP_INT,     STS_RO},    /* tcpConnLocalPort    (Integer32) */
    {snmp_tcp_mib_6_13_1_4, LEN_INT,    TYP_IP_ADR,  STS_RO},    /* tcpConnRemAddress   (IpAddress) */
    {snmp_tcp_mib_6_13_1_5, LEN_INT,    TYP_INT,     STS_RO},    /* tcpConnRemPort      (Integer32) */
};
/* UDP table */
#define MIB_TBL_UDP_PORT_CNT    2
static const T_SNMP_MIB snmp_tcp_mib_udp_port[MIB_TBL_UDP_PORT_CNT] = {
    {snmp_tcp_mib_7_5_1_1,  LEN_INT,    TYP_IP_ADR,  STS_RO},    /* udpLocalAddress (IpAddress) */
    {snmp_tcp_mib_7_5_1_2,  LEN_INT,    TYP_INT,     STS_RO},    /* udpLocalPort    (Integer) */
};

/* Buffer size */
#define TCP_MIB_BUF_CNT    MIB_TBL_TCP_PORT_CNT    /* Buffer size (UW) */

/* TCP MIB type */
#define TCP_MIB_TYP_IP     0x0001    /* IP */
#define TCP_MIB_TYP_TCP    0x0002    /* TCP */
#define TCP_MIB_TYP_UDP    0x0004    /* UDP */

/* TCP MIB data type */
#define TCP_MIB_DAT_INT    -1    /* Integer */

/* MIB command */
#define MIB_CMD_DEL    0x0000    /* Create */
#define MIB_CMD_CRE    0x0001    /* Delete */

/* Semaphore state */                                                    
#define SEM_MIB_DIS    0x0000    /* Disable */
#define SEM_MIB_ENA    0x0001    /* Enable */

/* Ethernet status */
#define STS_ETH_INV        0x0001U    /* Invalid */
#define STS_ETH_LNK_UP     0x0002U    /* Linkup */
#define STS_ETH_LNK_DWN    0x0000U    /* Linkdown */

/* Status */
#define STS_INV    0x0000    /* Invalid */
#define STS_INI    0x0001    /* Initialized */
#define STS_ENA    0x0004    /* Enable */
#define STS_DIS    0x0008    /* Disable */
#define STS_ERR    0x8000    /* Error */

/* OID string block */
typedef struct t_snmp_tcp_oid_dat {
    VB* str;    /* String data buffer */
    UH len;     /* String size */
} T_SNMP_TCP_OID_DAT;

/* TCP address block */
typedef struct t_snmp_tcp_adr_dat {
    UW loc_adr;         /* Local address */
    UW rmt_adr;         /* Remote address */
    UH loc_port;        /* Local address */
    UH rmt_port;        /* Remote address */
} T_SNMP_TCP_ADR_DAT;

/* TCP MIB data */
typedef struct t_snmp_tcp_mib_dat {
    const T_SNMP_MIB* mib;          /* MIB */
    T_SNMP_TCP_ADR_DAT adr;         /* MIB object address */
    UH typ;                         /* MIB type */
    UH id;                          /* Network device ID [0..3] */
} T_SNMP_TCP_MIB_DAT;

/* Manager */
typedef struct t_snmp_tcp_mgr {
    T_NET_ADR* net_adr_dat;                                 /* Network address buffer */
    T_SNMP_MIB_TCP_DAT* tcp_dat;                            /* TCP object data buffer */
    T_SNMP_MIB_ARP_DAT* arp_dat;                            /* ARP object data buffer */
    T_SNMP_OID_DAT* soc_oid_dat[MIB_TBL_TCP_PORT_CNT];      /* Socket OID string buffer */
    T_SNMP_OID_DAT* arp_oid_dat[MIB_TBL_IP_ARP_CNT];        /* ARP OID string buffer */
    T_SNMP_OID_DAT* at_oid_dat[MIB_TBL_AT_ARP_CNT];         /* AT OID string buffer */
    VB* oid_str_buf;                /* OID string buffer for MIB */
    UH* eth_sts;                    /* Network device status */
    T_SNMP_TCP_MIB_DAT mib_dat;     /* Data block */
    ID id_sem_mib;                  /* Semaphore for MIB */
    UH mib2_pre_len;                /* MIB-2 prefix size */
    UH oid_str_len;                 /* OID string buffer size */
    UH tcp_dat_cnt;                 /* Number of variable MIB data TCP */
    UH arp_dat_cnt;                 /* Number of variable MIB data MAC */
    UH sts;                         /* Status */
} T_SNMP_TCP_MGR;

/* Variables */
static ADDR snmp_tcp_dat_buf[TCP_MIB_BUF_CNT];     /* MIB data buffer */
static T_SNMP_TCP_MGR snmp_tcp_mgr = { 0 };      /* Manager */

/* Conversion macros */
#define dat_buf     snmp_tcp_dat_buf
#define mgr         snmp_tcp_mgr

static T_SNMP_MIB_TCP_DAT* snmp_tcp_buf_get(void)
{
    INT i;

    /* Allocate TCP data buffer */

    for (i = 0; i < mgr.tcp_dat_cnt; i++) {
        if (mgr.tcp_dat[i].sts == MIB_SOC_INV) {
            return &mgr.tcp_dat[i];
        }
    }

    mgr.sts |= STS_ERR;

    return 0;
}

static ER snmp_tcp_buf_rel(T_SNMP_MIB_TCP_DAT* buf)
{
    /* Release TCP data buffer */

    if (buf == 0x00) {
        return E_PAR;
    }

    buf->sts = MIB_SOC_INV;

    return E_OK;
}

static VB* snmp_tcp_itoa(VB* buf, UH* len, UH dat)
{
    INT i;
    VB c[5];

    /* Convert to string from digit */

    if (buf == 0x00 || len == 0x00) {
        return 0;
    }
    if (*len < 2) {
        return 0;    /* Not enough (digit, NULL) */
    }

    for (i = 0; i < 5; i++) {
        c[i] = '0' + (dat % 10);
        dat = dat / 10;
        if (dat == 0) {
            break;
        }
    }
    for (; i >= 0; i--) {
        *buf = c[i];
        buf++;
        (*len)--;
        if (*len == 0) {
            return 0;
        }
    }

    *buf = '\0';    /* NULL */

    return buf;
}

static VB* snmp_tcp_oid_cpy(VB* buf, UH* len, const VB* oid)
{
    UW len_oid;

    /* String copy */

    if (buf == 0x00 || len == 0x00 || *len == 0) {
        return 0;
    }

    len_oid = mgr.mib2_pre_len + snmp_strlen(oid);
    if (len_oid + 1 > (UW)*len) {
        return 0;
    }

    snmp_strcpy(buf, snmp_mib_mib2_pre);
    snmp_strcat(buf, oid);

    *len -= len_oid;

    return buf + len_oid;
}

static VB* snmp_tcp_oid_add_dig(VB* buf, UH* len, UH num)
{
    /* Append a digit into string */

    if (buf == 0x00 || len == 0x00) {
        return 0;
    }
    if (*len < 3) {
        return 0;    /* Not enough (dot, digit, NULL) */
    }

    /* Dot */
    snmp_strcat(buf, ".");
    buf++;
    (*len)--;

    /* Number */
    buf = snmp_tcp_itoa(buf, len, num);

    return buf;
}

static VB* snmp_tcp_oid_add_ip(VB* buf, UH* len, UW ip_adr)
{
    INT i;
    UW dat;

    /* Convert to OID from IP address */

    if (buf == 0x00 || len == 0x00) {
        return 0;
    }
    if (*len < 3) {
        return 0;    /* Not enough (dot, digit, NULL) */
    }

    *buf = '.';    /* Dot */
    buf++;
    (*len)--;

    for (i = 0; i < 4; i++) {
        dat = (ip_adr >> 8 * (3 - i)) & 0x00ff;
        buf = snmp_tcp_itoa(buf, len, (UH)dat);
        if (buf == 0x00 || *len == 0) {
            return 0;
        }
        if (i == 3) {
            break;
        }
        *buf = '.';
        buf++;
        (*len)--;
    }

    *buf = '\0';    /* NULL */

    return buf;
}

static ER snmp_tcp_oid_cre(VB* buf, UH* len, T_SNMP_TCP_MIB_DAT* mib_dat)
{
    UH buf_len;

    /* Create OID string for TCP or UDP port */

    if (buf == 0x00 || len == 0x00 || mib_dat == 0x00 || mib_dat->mib == 0x00) {
        return E_PAR;
    }
    buf_len = *len;

    /* OID number */
    buf = snmp_tcp_oid_cpy(buf, len, mib_dat->mib->str);
    
    /* Index */
    if (mib_dat->typ == TCP_MIB_TYP_IP) {
        buf = snmp_tcp_oid_add_dig(buf, len, mib_dat->id);
    }

    /* Local IP */
    buf = snmp_tcp_oid_add_ip(buf, len, mib_dat->adr.loc_adr);

    /* Local port */
    if (mib_dat->typ == TCP_MIB_TYP_TCP || mib_dat->typ == TCP_MIB_TYP_UDP) {
        buf = snmp_tcp_oid_add_dig(buf, len, mib_dat->adr.loc_port);
    }

    /* Remote IP and port */
    if (mib_dat->typ == TCP_MIB_TYP_TCP) {
        buf = snmp_tcp_oid_add_ip(buf, len, mib_dat->adr.rmt_adr);
        buf = snmp_tcp_oid_add_dig(buf, len, mib_dat->adr.rmt_port);
    }

    if (buf == 0x00) {
        return E_OBJ;
    }

    *len = buf_len - *len;

    return E_OK;
}

static ER snmp_tcp_mib_cre(T_SNMP_OID_DAT* oid, const T_SNMP_MIB* mib, ADDR dat, INT arp_dat_id)
{
    ER ercd;
    T_SNMP_MIB_TCP_DAT** dat_ptr;
    T_SNMP_MIB_TCP_DAT* tcp_dat;
    T_SNMP_MIB_ARP_DAT* arp_dat;

    /* Append nodes for TCP or UDP port */

    if (mib == 0x00) {
        return E_PAR;
    }
    if (arp_dat_id != TCP_MIB_DAT_INT) {
        if (arp_dat_id >= mgr.arp_dat_cnt) {
            return E_PAR;
        }
    }

    /* Append */
    ercd = snmp_mib_add(oid->buf, oid->len, MIB_NOD_STD, (VP*)&dat_ptr, 0);
    if (ercd != E_OK) {
        mgr.sts |= STS_ERR;
        dbg_put_str("Error: snmp_tcp_mib_cre(add)\r\n");
        return E_OBJ;
    }

    /* Node data buffer */
    tcp_dat = *dat_ptr;
    if (tcp_dat == 0x00) {
        tcp_dat = snmp_tcp_buf_get();
        if (tcp_dat == 0x00) {
            ercd = E_OBJ;
        }
    }
    if (ercd != E_OK) {
        snmp_mib_del(oid->buf, oid->len, 0x00);
        mgr.sts |= STS_ERR;
        dbg_put_str("Error: snmp_tcp_mib_cre(buf)\r\n");
        return E_OBJ;
    }

    /* Object data */
    if (arp_dat_id != TCP_MIB_DAT_INT) {
        arp_dat = &mgr.arp_dat[arp_dat_id];
        snmp_memcpy(arp_dat->mac, (VP)dat, MAC_ADR_LEN);
        arp_dat->mac[MAC_ADR_LEN] = '\0';
        tcp_dat->buf = arp_dat->mac;
    } else {
        tcp_dat->buf = (VP)dat;
    }
    tcp_dat->typ = mib->typ;
    tcp_dat->acs = mib->acs;
    tcp_dat->sts = MIB_SOC_ENA;
    *dat_ptr = tcp_dat;

    return E_OK;
}

static ER snmp_tcp_mib_ip(UH dev_num, UH cmd, UH flg_sem)
{
    ER ercd;
    UH i;
    T_SNMP_OID_DAT oid;
    T_SNMP_MIB_TCP_DAT* tcp_dat;

    /* Create or delete MIB address table */

    if (dev_num == 0 || dev_num > CFG_NET_DEV_CNT) {
        return E_PAR;
    }
    dev_num--;

    if (cmd == MIB_CMD_CRE) {
        if (flg_sem == SEM_MIB_ENA) {
            ercd = net_ref(dev_num + 1, NET_IP4_CFG, &mgr.net_adr_dat[dev_num]);
            if (ercd != E_OK) {
                return E_OBJ;
            }
        } else {
            /* Called from TCP stack (locked) */
            mgr.net_adr_dat[dev_num].ipaddr = gNET_ADR[dev_num].ipaddr;
            mgr.net_adr_dat[dev_num].mask = gNET_ADR[dev_num].mask;
            mgr.net_adr_dat[dev_num].gateway = gNET_ADR[dev_num].gateway;
        }
    }

    if (flg_sem != SEM_MIB_DIS) {
        ercd = snmp_sem_wai(mgr.id_sem_mib);
        if (ercd != E_OK) {
            return ercd;
        }
    }

    mgr.mib_dat.typ = TCP_MIB_TYP_IP;
    /* Address */
    mgr.mib_dat.id = dev_num + 1;
    mgr.mib_dat.adr.loc_adr = mgr.net_adr_dat[dev_num].ipaddr;
    /* Object data */
    dat_buf[0] = mgr.net_adr_dat[dev_num].ipaddr;   /* ipAdEntAddr */
    dat_buf[1] = dev_num + 1;                       /* ipAdEntIfIndex */
    dat_buf[2] = mgr.net_adr_dat[dev_num].mask;     /* ipAdEntNetMask */
    dat_buf[3] = SNMP_BCAST_ADR & 0x0001;           /* ipAdEntBcastAddr */
    dat_buf[4] = SNMP_REASM_MAX_LEN;                /* ipAdEntReasmMaxSize */
    for (i = 0; i < MIB_TBL_IP_ADR_CNT; i++) {
        /* Create OID string */
        mgr.mib_dat.mib = &snmp_tcp_mib_ip_adr[i];
        oid.buf = mgr.oid_str_buf;
        oid.len = mgr.oid_str_len;
        ercd = snmp_tcp_oid_cre(oid.buf, &oid.len, &mgr.mib_dat);
        if (cmd == MIB_CMD_CRE) {
            /* Create node */
            if (ercd == E_OK) {
                ercd = snmp_tcp_mib_cre(&oid, &snmp_tcp_mib_ip_adr[i], dat_buf[i], TCP_MIB_DAT_INT);
            }
            if (ercd != E_OK) {
                mgr.sts |= STS_ERR;
                dbg_put_str("Error: snmp_tcp_mib_ip(create)\r\n");
                break;
            }
        } else {
            /* Delete node */
            if (ercd == E_OK) {
                ercd = snmp_mib_del(oid.buf, oid.len, (VP*)&tcp_dat);
                snmp_tcp_buf_rel(tcp_dat);
                if (ercd != E_OK) {
                    mgr.sts |= STS_ERR;
                    dbg_put_str("Error: snmp_tcp_mib_ip(delete)\r\n");
                }
            }
        }
    }

    if (flg_sem != SEM_MIB_DIS) {
        snmp_sem_sig(mgr.id_sem_mib);
    }

    return ercd;
}

ER snmp_tcp_ini(ID id_sem_mib, VB* oid_str_buf, UH oid_str_len)
{
    INT id;
    UH len;
    UH i;
    UH j;

    /* Initialization */

    mgr.sts = STS_INV;
    mgr.id_sem_mib = id_sem_mib;
    mgr.mib2_pre_len = (UH)snmp_strlen(snmp_mib_mib2_pre);
    mgr.oid_str_buf = oid_str_buf;
    mgr.oid_str_len = oid_str_len;
    mgr.tcp_dat_cnt = CFG_TCP_DAT_CNT;
    mgr.arp_dat_cnt = CFG_ARP_DAT_CNT;

    /* Variable MIB data */
    id = 0;
    mgr.tcp_dat = (T_SNMP_MIB_TCP_DAT*)&snmp_cfg_tcp_buf[id];    /* Top */
    len = BUF_UW_LEN(sizeof(T_SNMP_MIB_TCP_DAT)) * mgr.tcp_dat_cnt;
    id = len;
    mgr.arp_dat = (T_SNMP_MIB_ARP_DAT*)&snmp_cfg_tcp_buf[id];
    len = BUF_UW_LEN(sizeof(T_SNMP_MIB_ARP_DAT)) * mgr.arp_dat_cnt;
    id += len;
    if (snmp_mib_dat_cnt[SNMP_MIB2_CNT_TCP] != 0
        || snmp_mib_dat_cnt[SNMP_MIB2_CNT_UDP] != 0) {
        len = BUF_UW_LEN(sizeof(T_SNMP_OID_DAT)) * CFG_MAX_SOC_CNT;
        for (i = 0; i < MIB_TBL_TCP_PORT_CNT; i++) {
            mgr.soc_oid_dat[i] = (T_SNMP_OID_DAT*)&snmp_cfg_tcp_buf[id];
            id += len;
        }
    }
    len = BUF_UW_LEN(sizeof(T_SNMP_OID_DAT)) * mgr.arp_dat_cnt;
    if (snmp_mib_dat_cnt[SNMP_MIB2_CNT_IP] != 0) {
        for (i = 0; i < MIB_TBL_IP_ARP_CNT; i++) {
            mgr.arp_oid_dat[i] = (T_SNMP_OID_DAT*)&snmp_cfg_tcp_buf[id];
            id += len;
        }
    }
    if (snmp_mib_dat_cnt[SNMP_MIB2_CNT_AT] != 0) {
        for (i = 0; i < MIB_TBL_AT_ARP_CNT; i++) {
            mgr.at_oid_dat[i] = (T_SNMP_OID_DAT*)&snmp_cfg_tcp_buf[id];
            id += len;
        }
    }
    len = BUF_UW_LEN(6 * (CFG_MAX_MIB_DEP + 1));
    if (snmp_mib_dat_cnt[SNMP_MIB2_CNT_TCP] != 0
        || snmp_mib_dat_cnt[SNMP_MIB2_CNT_UDP] != 0) {
        for (i = 0; i < MIB_TBL_TCP_PORT_CNT; i++) {
            for (j = 0; j < CFG_MAX_SOC_CNT; j++) {
                mgr.soc_oid_dat[i][j].buf = (VB*)&snmp_cfg_tcp_buf[id];
                mgr.soc_oid_dat[i][j].buf[0] = '\0';
                mgr.soc_oid_dat[i][j].len = 0;
                id += len;
            }
        }
    }
    if (snmp_mib_dat_cnt[SNMP_MIB2_CNT_IP] != 0) {
        for (i = 0; i < MIB_TBL_IP_ARP_CNT; i++) {
            for (j = 0; j < mgr.arp_dat_cnt; j++) {
                mgr.arp_oid_dat[i][j].buf = (VB*)&snmp_cfg_tcp_buf[id];
                mgr.arp_oid_dat[i][j].buf[0] = '\0';
                mgr.arp_oid_dat[i][j].len = 0;
                id += len;
            }
        }
    }
    if (snmp_mib_dat_cnt[SNMP_MIB2_CNT_AT] != 0) {
        for (i = 0; i < MIB_TBL_AT_ARP_CNT; i++) {
            for (j = 0; j < mgr.arp_dat_cnt; j++) {
                mgr.at_oid_dat[i][j].buf = (VB*)&snmp_cfg_tcp_buf[id];
                mgr.at_oid_dat[i][j].buf[0] = '\0';
                mgr.at_oid_dat[i][j].len = 0;
                id += len;
            }
        }
    }
    if (snmp_mib_dat_cnt[SNMP_MIB2_CNT_IP] != 0) {
        mgr.net_adr_dat = (T_NET_ADR*)&snmp_cfg_tcp_buf[id];
        len = BUF_UW_LEN(sizeof(T_NET_ADR)) * CFG_NET_DEV_CNT;
        id += len;
    }
    mgr.eth_sts = (UH*)&snmp_cfg_tcp_buf[id];
    len = BUF_UW_LEN(sizeof(UH)) * CFG_NET_DEV_CNT;
    id += len;
    snmp_cfg_tcp_buf[id] = 0xffffaa55;    /* Terminal flag */

    for (i = 0; i < mgr.tcp_dat_cnt; i++) {
        mgr.tcp_dat[i].buf = 0;
        mgr.tcp_dat[i].typ = TYP_NONE;
        mgr.tcp_dat[i].acs = SNMP_STS_NO;
        mgr.tcp_dat[i].sts = MIB_SOC_INV;
    }
    for (i = 0; i < mgr.arp_dat_cnt; i++) {
        snmp_memset(mgr.arp_dat[i].mac, 0, MAC_ADR_LEN + 1);
    }

    /* Status */
    mgr.sts |= STS_INI;

    return E_OK;
}

ER snmp_tcp_ena(void)
{
    ER ercd;
    UH i;

    /* Enable */

    ercd = snmp_sem_wai(mgr.id_sem_mib);
    if (ercd != E_OK) {
        return ercd;
    }

    if (snmp_mib_dat_cnt[SNMP_MIB2_CNT_IP] != 0) {
        for (i = 0U; i < CFG_NET_DEV_CNT; i++) {
            ercd = snmp_tcp_mib_ip(i + 1, MIB_CMD_CRE, SEM_MIB_DIS);
            if (ercd != E_OK) {
                dbg_put_str("Error: snmp_tcp_mib_ip(snmp_tcp_ena)\r\n");
            }
        }
    }

    /* Clear ethernet status */
    for (i = 0U; i < CFG_NET_DEV_CNT; i++) {
        mgr.eth_sts[i] = STS_ETH_INV;
    }

    /* Status */
    mgr.sts |= STS_ENA;

    snmp_sem_sig(mgr.id_sem_mib);

    net_sts_snmp_ena();

    return E_OK;
}

ER snmp_tcp_dis(void)
{
    ER ercd;
    UH i;

    /* Disable */

    ercd = snmp_sem_wai(mgr.id_sem_mib);
    if (ercd != E_OK) {
        return ercd;
    }

    net_sts_snmp_dis();

    if (snmp_mib_dat_cnt[SNMP_MIB2_CNT_IP] != 0) {
        for (i = 0U; i < CFG_NET_DEV_CNT; i++) {
            ercd = snmp_tcp_mib_ip(i + 1, MIB_CMD_DEL, SEM_MIB_DIS);
            if (ercd != E_OK) {
                dbg_put_str("Error: snmp_tcp_mib_ip(snmp_tcp_dis)\r\n");
            }
        }
    }

    /* Status */
    mgr.sts &= ~STS_ENA;

    snmp_sem_sig(mgr.id_sem_mib);

    return E_OK;
}

ER snmp_tcp_nod_cnt(UH* cnt)
{
    UH cnt_nod;

    /* Maximum number of nodes for TCP MIB */

    if (cnt == 0x00) {
        return E_PAR;
    }

    cnt_nod = 0;

    /* IP address table (5: IP address + index or port) */
    if (snmp_mib_dat_cnt[SNMP_MIB2_CNT_IP] != 0) {
        cnt_nod += 5 * MIB_TBL_IP_ADR_CNT * CFG_NET_DEV_CNT;
    }

    /* IP arp table */
    if (snmp_mib_dat_cnt[SNMP_MIB2_CNT_IP] != 0) {
        cnt_nod += 5 * MIB_TBL_IP_ARP_CNT * snmp_cfg_tcp.arp_dat_cnt;
    }

    /* AT arp table */
    if (snmp_mib_dat_cnt[SNMP_MIB2_CNT_AT] != 0) {
        cnt_nod += 5 * MIB_TBL_AT_ARP_CNT * snmp_cfg_tcp.arp_dat_cnt;
    }

    /* TCP table */
    if (snmp_mib_dat_cnt[SNMP_MIB2_CNT_TCP] != 0) {
        cnt_nod += 10 * MIB_TBL_TCP_PORT_CNT * NET_TCP_MAX;
    }

    /* UDP table */
    if (snmp_mib_dat_cnt[SNMP_MIB2_CNT_UDP] != 0) {
       cnt_nod += 5 * MIB_TBL_UDP_PORT_CNT * NET_SOC_MAX;
    }

    *cnt = cnt_nod;

    return E_OK;
}

ER snmp_tcp_get_eth_lnk(UH dev_num, UH flg_sem)
{
    ER ercd;
    UH sts;

    /* Retrieve ethernet status */

    if (dev_num > CFG_NET_DEV_CNT) {
        return E_OBJ;
    }

    if (flg_sem != 0x00U) {
        ercd = snmp_sem_wai(mgr.id_sem_mib);
        if (ercd != E_OK) {
            return E_OBJ;
        }
    }

#if CFG_SNMP_GETETHLINK_R201912
    /* Always refer to the link status in consideration of the virtual device. */
    sts = PHY_STS_LINK_DOWN;
    ercd = gNET_DEV[dev_num - 1].ref(dev_num, REF_ETH_LINK_STS, &sts);  /* eth_ref() */
    if (ercd == E_OK) {	/* gNET_DEV[].ref() success */
        if (sts != PHY_STS_LINK_DOWN) {
            mgr.eth_sts[dev_num - 1] = STS_ETH_LNK_UP;
        } else {
            mgr.eth_sts[dev_num - 1] = STS_ETH_LNK_DWN;
        }
    }
    else {		/* gNET_DEV[].ref() failed : Driver link status is unknown. */
        mgr.eth_sts[dev_num - 1] = STS_ETH_INV;
    }

    /* If the link is up, E_OK is returned. Otherwise, E_OBJ is returned. */
    ercd = ((mgr.eth_sts[dev_num - 1] & STS_ETH_LNK_UP) != 0x00U) ? E_OK : E_OBJ;

#else /* For drivers whose link result of gNET_DEV[].ref() is abnormal in the Ethernet callback function. */
    if (mgr.eth_sts[dev_num - 1] == STS_ETH_INV) {
        sts = PHY_STS_LINK_DOWN;
        ercd = gNET_DEV[dev_num - 1].ref(dev_num, REF_ETH_LINK_STS, &sts);
        if (ercd == E_OK) {
            if (sts != PHY_STS_LINK_DOWN) {
                mgr.eth_sts[dev_num - 1] = STS_ETH_LNK_UP;
            } else {
                mgr.eth_sts[dev_num - 1] = STS_ETH_LNK_DWN;
            }
        }
    }
    ercd = ((mgr.eth_sts[dev_num - 1] & STS_ETH_LNK_UP) != 0x00U) ? E_OK : E_OBJ ;

#endif

    if (flg_sem != 0x00U) {
        snmp_sem_sig(mgr.id_sem_mib);
    }

    return ercd;
}

ER snmp_tcp_get_ip(T_NODE* nod, UH dev_id)
{
    ER ercd;
    T_NET_ADR adr;

    /* Retrieve IP address */

    ercd = net_ref(dev_id, NET_IP4_CFG, &adr);
    if (ercd != E_OK) {
        return E_OBJ;
    }
    nod->ipa = adr.ipaddr;

    return E_OK;
}

void snmp_tcp_cbk(UH dev_num, UH evt, VP sts)
{
    ER ercd;
    T_NET_ARP* arp;
    UH id;
    T_SNMP_OID_DAT* oid;
    INT i;
    INT dat_id;
    INT tbl_cnt;
    const T_SNMP_MIB* mib;
    T_SNMP_MIB_TCP_DAT* tcp_dat;
    T_NET_SOC* soc;

    /* Callback from network device or TCP stack */

    if ((mgr.sts & STS_ENA) == 0x00) {
        dbg_put_str("Error: snmp_tcp_cbk(sts)\r\n");
        return;
    }

    if (dev_num == 0 || dev_num > CFG_NET_DEV_CNT) {
        return;
    }
    dev_num--;

    ercd = snmp_sem_wai(mgr.id_sem_mib);
    if (ercd != E_OK) {
        return;
    }

    ercd = E_OBJ;
    if (evt == EV_CBK_DEV_LINK) {
        /* Network link */
        dev_num++;
        ercd = snmp_set_itf_last_chg(dev_num);
        if (ercd != E_OK) {
            mgr.sts |= STS_ERR;
        }
        dbg_put_str("snmp_trp_snd_lnk(");
        dbg_put_dig(dev_num);
        dbg_put_str(", ");
        dbg_put_dig((UW)sts);
        dbg_put_str(")\r\n");
        if ((UW)(ADDR)sts == PHY_STS_LINK_DOWN) {
            /* Trap of link down */
            mgr.eth_sts[dev_num - 1] = STS_ETH_LNK_DWN;
            snmp_trp_snd_lnk(SNMP_TRP_LINK_DOWN, dev_num);
        } else {
            /* Trap of link up */
        	mgr.eth_sts[dev_num - 1] = STS_ETH_LNK_UP;
            snmp_trp_snd_sta(dev_num);
            snmp_trp_snd_lnk(SNMP_TRP_LINK_UP, dev_num);
        }
    } else if ((evt & EV_CBK_ARP_MSK) != 0x00) {
        /* ARP */
        if (sts == 0x00 || (snmp_mib_dat_cnt[SNMP_MIB2_CNT_AT] == 0 
                            && snmp_mib_dat_cnt[SNMP_MIB2_CNT_IP] == 0)) {
            snmp_sem_sig(mgr.id_sem_mib);
            return;
        }
        arp = (T_NET_ARP*)sts;
        id = (UH)(((ADDR)arp - (ADDR)&gNET_ARP[0]) / sizeof(T_NET_ARP));
        if (id >= mgr.arp_dat_cnt) {
            snmp_sem_sig(mgr.id_sem_mib);
            return;
        }
        dbg_put_str("snmp_tcp_cbk(MIB ARP) id:");
        dbg_put_dig(id);
        dbg_put_str("\r\n");
        mgr.mib_dat.typ = TCP_MIB_TYP_IP;
        mgr.mib_dat.id = dev_num + 1;
        mgr.mib_dat.adr.loc_adr = arp->ipaddr;
        if (evt != EV_CBK_ARP_DEL) {
            dat_buf[0] = dev_num + 1;          /* ipNetToMediaIfIndex */
            dat_buf[1] = (ADDR)&arp->mac[0];     /* ipNetToMediaPhysAddress */ /* TODO: makino ‚¨‚»‚ç‚­ƒ|ƒCƒ“ƒ^‚ð‚¿‚á‚ñ‚Æ‚µ‚½‚Ù‚¤‚ª‚¢‚¢ */
            dat_buf[2] = arp->ipaddr;          /* ipNetToMediaNetAddress */
            dat_buf[3] = ARP_ENT_DYNAMIC;      /* ipNetToMediaType */
        }
        if (snmp_mib_dat_cnt[SNMP_MIB2_CNT_IP] != 0) {
            for (i = 0; i < MIB_TBL_IP_ARP_CNT; i++) {
                /* Delete node */
                oid = &mgr.arp_oid_dat[i][id];
                if (evt != EV_CBK_ARP_CHG_MAC && oid->buf[0] != '\0') {
                    ercd = snmp_mib_del(oid->buf, oid->len, (VP*)&tcp_dat);
                    snmp_tcp_buf_rel(tcp_dat);
                    oid->buf[0] = '\0';
                    if (ercd != E_OK) {
                        mgr.sts |= STS_ERR;
                        dbg_put_str("Error: snmp_tcp_cbk(MIB ARP CHG_IP delete)\r\n");
                        dbg_put_str("  OID: 1");
                        dbg_put_str(oid->buf + 1);
                        dbg_put_str("\r\n");
                    }
                }
                /* Create OID string */
                if (evt == EV_CBK_ARP_CRE || evt == EV_CBK_ARP_CHG_IP) {
                    mgr.mib_dat.mib = &snmp_tcp_mib_ip_arp[i];
                    oid->len = mgr.oid_str_len;
                    ercd = snmp_tcp_oid_cre(oid->buf, &oid->len, &mgr.mib_dat);
                    if (ercd != E_OK) {
                        oid->buf[0] = '\0';
                        mgr.sts |= STS_ERR;
                        dbg_put_str("Error: snmp_tcp_cbk(MIB ARP oid)\r\n");
                        break;
                    }
                }
                if (evt != EV_CBK_ARP_DEL) {
                    dbg_put_str("snmp_tcp_cbk(MIB ARP cre)\r\n");
                    dbg_put_str("  OID: ");
                    dbg_put_str(oid->buf);
                    dbg_put_str("\r\n");
                    /* Create new node */
                    dat_id = (i == 1) ? (INT)id : TCP_MIB_DAT_INT;
                    ercd = snmp_tcp_mib_cre(oid, &snmp_tcp_mib_ip_arp[i], dat_buf[i], (INT)dat_id);
                    if (ercd != E_OK) {
                        oid->buf[0] = '\0';
                        mgr.sts |= STS_ERR;
                        dbg_put_str("Error: snmp_tcp_cbk(MIB ARP create)\r\n");
                        break;
                    }
                }
            }
        }
        if (snmp_mib_dat_cnt[SNMP_MIB2_CNT_AT] != 0) {
            for (i = 0; i < MIB_TBL_AT_ARP_CNT; i++) {
                /* Delete node */
                oid = &mgr.at_oid_dat[i][id];
                if (evt != EV_CBK_ARP_CHG_MAC && oid->buf[0] != '\0') {
                    dbg_put_str("snmp_tcp_cbk(MIB AT ARP_CHG delete)\r\n");
                    dbg_put_str("  OID: ");
                    dbg_put_str(oid->buf);
                    dbg_put_str("\r\n");
                    ercd = snmp_mib_del(oid->buf, oid->len, (VP*)&tcp_dat);
                    snmp_tcp_buf_rel(tcp_dat);
                    oid->buf[0] = '\0';
                    if (ercd != E_OK) {
                        mgr.sts |= STS_ERR;
                        dbg_put_str("Error: snmp_tcp_cbk(MIB AT ARP_CHG delete)\r\n");
                        dbg_put_str("  OID: 1");
                        dbg_put_str(oid->buf + 1);
                        dbg_put_str("\r\n");
                    }
                }
                /* Create OID string */
                if (evt == EV_CBK_ARP_CRE || evt == EV_CBK_ARP_CHG_IP) {
                    mgr.mib_dat.mib = &snmp_tcp_mib_at_arp[i];
                    oid->len = mgr.oid_str_len;
                    ercd = snmp_tcp_oid_cre(oid->buf, &oid->len, &mgr.mib_dat);
                    if (ercd != E_OK) {
                        oid->buf[0] = '\0';
                        mgr.sts |= STS_ERR;
                        dbg_put_str("Error: snmp_tcp_cbk(MIB AT oid)\r\n");
                        break;
                    }
                }
                if (evt != EV_CBK_ARP_DEL) {
                    dbg_put_str("snmp_tcp_cbk(MIB AT cre)\r\n");
                    dbg_put_str("  OID: ");
                    dbg_put_str(oid->buf);
                    dbg_put_str("\r\n");
                    /* Create new node */
                    dat_id = (i == 1) ? (INT)id : TCP_MIB_DAT_INT;
                    ercd = snmp_tcp_mib_cre(oid, &snmp_tcp_mib_at_arp[i], dat_buf[i], (INT)dat_id);
                    if (ercd != E_OK) {
                        oid->buf[0] = '\0';
                        mgr.sts |= STS_ERR;
                        dbg_put_str("Error: snmp_tcp_cbk(MIB AT create)\r\n");
                        break;
                    }
                }
            }
        }
    } else if ((evt & EV_CBK_SOC_MSK) != 0x00) {
        if (sts == 0x00 || (snmp_mib_dat_cnt[SNMP_MIB2_CNT_TCP] == 0
                            && snmp_mib_dat_cnt[SNMP_MIB2_CNT_UDP] == 0)) {
            snmp_sem_sig(mgr.id_sem_mib);
            return;
        }
        soc = (T_NET_SOC*)sts;
        id = soc->sid;    /* TCP:[0..], UDP:[NET_TCP_MAX..] */
        if ((evt == EV_CBK_SOC_CRE) || (evt == EV_CBK_SOC_DEL) || (evt == EV_CBK_SOC_CHG)) {
            /* Delete old node */
            dbg_put_str("snmp_tcp_cbk(MIB SOC del) id:");
            dbg_put_dig(id);
            dbg_put_str("\r\n");
            for (i = 0; i < MIB_TBL_TCP_PORT_CNT; i++) {                
                oid = &mgr.soc_oid_dat[i][id];
                if (oid->buf[0] != '\0') {
                    ercd = snmp_mib_del(oid->buf, oid->len, (VP*)&tcp_dat);
                    snmp_tcp_buf_rel(tcp_dat);
                    oid->buf[0] = '\0';
                    if (ercd != E_OK) {
                        mgr.sts |= STS_ERR;
                        dbg_put_str("Error: snmp_tcp_cbk(MIB TCP table delete)\r\n");
                    }
                }
            }
        }
        if (evt != EV_CBK_SOC_DEL) {
            if (soc->proto == IP_PROTO_TCP) {
                /* TCP */
                dbg_put_str("snmp_tcp_cbk(MIB SOC TCP) id:");
                dbg_put_dig(id);
                dbg_put_str("\r\n");
                if (id >= CFG_MAX_TCP_CNT || snmp_mib_dat_cnt[SNMP_MIB2_CNT_TCP] == 0) {
                    snmp_sem_sig(mgr.id_sem_mib);
                    return;
                }
                mgr.mib_dat.typ = TCP_MIB_TYP_TCP;
                tbl_cnt = MIB_TBL_TCP_PORT_CNT;
                mib = &snmp_tcp_mib_tcp_port[0];
                if (soc->tcp == 0x00) {
                    snmp_sem_sig(mgr.id_sem_mib);
                    return;
                }
                switch (soc->tcp->bsd) {
                    case TCP_ESTABLISHED:
                        dbg_put_str("snmp_tcp_cbk(SOC_EST)\r\n");
                        dat_buf[0] = TCP_SOC_EST;
                        break;
                    case TCP_LISTEN:
                        dbg_put_str("snmp_tcp_cbk(SOC_LIS)\r\n");
                        dat_buf[0] = TCP_SOC_LIS;
                        break;
                    case TCP_SYN_SENT:
                        dbg_put_str("snmp_tcp_cbk(SOC_SYN_SNT)\r\n");
                        dat_buf[0] = TCP_SOC_SYN_SENT;
                        break;
                    case TCP_CLOSED:
                        dbg_put_str("snmp_tcp_cbk(SOC_CRE(CLS))\r\n");
                        dat_buf[0] = TCP_SOC_CLS;
                        break;
                    case TCP_SYN_RECEIVED:
                        dbg_put_str("snmp_tcp_cbk(SOC_SYN_RCV)\r\n");
                        dat_buf[0] = TCP_SOC_SYN_RCV;
                        break;
                    case TCP_FIN_WAIT1:
                        dbg_put_str("snmp_tcp_cbk(SOC_FIN_WAI1)\r\n");
                        dat_buf[0] = TCP_SOC_FIN_WAI1;
                        break;
                    case TCP_FIN_WAIT2:
                        dbg_put_str("snmp_tcp_cbk(SOC_FIN_WAI2)\r\n");
                        dat_buf[0] = TCP_SOC_FIN_WAI2;
                        break;
                    case TCP_CLOSE_WAIT:
                        dbg_put_str("snmp_tcp_cbk(SOC_CLS_WAI)\r\n");
                        dat_buf[0] = TCP_SOC_CLS_WAI;
                        break;
                    case TCP_LAST_ACK:
                        dbg_put_str("snmp_tcp_cbk(SOC_LST_ACK)\r\n");
                        dat_buf[0] = TCP_SOC_LAST_ACK;
                        break;
                    case TCP_CLOSING:
                        dbg_put_str("snmp_tcp_cbk(SOC_CLS_ING)\r\n");
                        dat_buf[0] = TCP_SOC_CLS_ING;
                        break;
                    case TCP_TIME_WAIT:
                        dbg_put_str("snmp_tcp_cbk(SOC_TIM_WAI)\r\n");
                        dat_buf[0] = TCP_SOC_TIM_WAI;
                        break;
                    default:
                        break;
                }
                dat_buf[1] = gNET_ADR[dev_num].ipaddr;    /* tcpConnLocalAddress */
                dat_buf[2] = soc->lport;                  /* tcpConnLocalPort */
                dat_buf[3] = soc->raddr;                  /* tcpConnRemAddress */
                dat_buf[4] = soc->rport;                  /* tcpConnRemPort */
            } else {
                /* UDP */
                dbg_put_str("snmp_tcp_cbk(MIB SOC UDP) id:");
                dbg_put_dig(id);
                dbg_put_str("\r\n");
                if (snmp_mib_dat_cnt[SNMP_MIB2_CNT_UDP] == 0) {
                    snmp_sem_sig(mgr.id_sem_mib);
                    return;
                }
                mgr.mib_dat.typ = TCP_MIB_TYP_UDP;
                tbl_cnt = MIB_TBL_UDP_PORT_CNT;
                mib = &snmp_tcp_mib_udp_port[0];
                dat_buf[0] = gNET_ADR[dev_num].ipaddr;    /* udpLocalAddress */
                dat_buf[1] = soc->lport;                  /* udpLocalPort */
            }
            if (evt == EV_CBK_SOC_CRE || evt == EV_CBK_SOC_CHG) {
                mgr.mib_dat.id = 0;    /* Unused */
                mgr.mib_dat.adr.loc_adr = gNET_ADR[dev_num].ipaddr;
                mgr.mib_dat.adr.rmt_adr = soc->raddr;
                mgr.mib_dat.adr.loc_port = soc->lport;
                mgr.mib_dat.adr.rmt_port = soc->rport;
                for (i = 0; i < tbl_cnt; i++) {                
                    /* Create OID string */
                    mgr.mib_dat.mib = &mib[i];
                    oid = &mgr.soc_oid_dat[i][id];
                    oid->len = mgr.oid_str_len;
                    ercd = snmp_tcp_oid_cre(oid->buf, &oid->len, &mgr.mib_dat);
                    if (ercd != E_OK) {
                        oid->buf[0] = '\0';
                        mgr.sts |= STS_ERR;
                        dbg_put_str("Error: snmp_tcp_cbk(MIB TCP oid)\r\n");
                        break;
                    }
                    /* Create or change status */
                    ercd = snmp_tcp_mib_cre(oid, &mib[i], dat_buf[i], TCP_MIB_DAT_INT);
                    if (ercd != E_OK) {
                        mgr.sts |= STS_ERR;
                        dbg_put_str("Error: snmp_tcp_cbk(MIB TCP create)\r\n");
                    }
                }
            } else {
                /* TCP status changed */
                mgr.mib_dat.mib = &mib[0];
                oid = &mgr.soc_oid_dat[0][id];
                ercd = snmp_tcp_mib_cre(oid, &mib[0], dat_buf[0], TCP_MIB_DAT_INT);
                if (ercd != E_OK) {
                    mgr.sts |= STS_ERR;
                    dbg_put_str("Error: snmp_tcp_cbk(MIB TCP changed)\r\n");
                }
            }
        }
    } else if (evt == EV_CBK_DEV_CHG_IP) {
        if (snmp_mib_dat_cnt[SNMP_MIB2_CNT_IP] != 0) {
            /* IP address was changed */
            snmp_tcp_mib_ip(dev_num + 1, MIB_CMD_DEL, SEM_MIB_DIS);
            snmp_tcp_mib_ip(dev_num + 1, MIB_CMD_CRE, SEM_MIB_DIS);
        }
    }

    snmp_sem_sig(mgr.id_sem_mib);

    return;
}

