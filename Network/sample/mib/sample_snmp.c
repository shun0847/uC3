/***************************************************************************
    SNMP Sample (Operating as command in shell)
    Copyright (c)  2017, eForce Co., Ltd. All rights reserved.

    2017-12-06: Created.
 ***************************************************************************/
#include "kernel.h"
//#include "kernel_id.h"
#include "sample_netapp_cfg.h"      /* include Sample Application Settings */
#include "snmp.h"
#include "snmp_cfg.h"

#if defined (__RENESAS__)
typedef unsigned long long UD;
#endif

extern T_SNMP_MIB_DAT snmp_mib_ven_dat_0[];

/* Configuration */
#define DBG_ENA     1    /* Debug enabled */

/* Debug print */
#define put_str(x)    apl_put_str(x)
#define put_dig(x)    apl_put_dig(x)
#define put_hex(x)    apl_put_hex(x)

/* SNMP agent */
extern T_SNMP_MIB_TBL snmp_mib_ven[];       /* Vendor MIB */
extern T_SNMP_CFG_TRP snmp_cfg_trp[];       /* Remote host address for trap */
#define ID_NET_DEV    1                     /* Device ID for network interface */
#define TRP_TMO       200                   /* Timeout for traps */
#define INF_TMO       8000                  /* Timeout for informs */
#define INF_RTY_CNT   4                     /* Retry count for informs */
VB apl_server_adr[] = "192.168.1.110";      /* SNMP manager IP address */
VB apl_snmp_com[] = "public";               /* Community */
VP apl_var_id[8];                           /* Variable binding ID */

/* ID vendor MIB(0) */
#define MIB_0_DESC         0    /* Descr */
#define MIB_0_VER          1    /* Version */
#define MIB_0_USR_NAME     2    /* User name */
#define MIB_0_TIME         3    /* Time ticks */
#define MIB_0_STS          4    /* Status */
#define MIB_0_IP_ADR       5    /* IP address */
#define MIB_0_CNT          6    /* Counter */
#define MIB_0_GUAGE        7    /* Gauge */

/* Object data */
const VB apl_decr_new[] = "Vendor MIB Active";
const VB apl_ver_new[] = "Version 1.2.3";
const VB apl_usr_root[] = "root";

/* Message */
const VB apl_msg_boot[] = "\r\n\r\n---- SNMP Sample Program ----\r\n";
const VB apl_msg_ip[] = "  [IP Address ] ";
const VB apl_msg_gw[] = "\r\n  [Gateway    ] ";
const VB apl_msg_msk[] ="\r\n  [Subnet mask] ";
const VB apl_msg_newline[] = "\r\n";
const VB apl_msg_mib_cnt[] = "\r\n-- Increase/decrease the number of CFG_SNMP_MIB_NOD_CNT(snmp_cfg.h)-- \r\n\r\n  ";
const VB apl_msg_adr_tar[] = "\r\n-- Target Address --\r\n\r\n";
//const VB apl_msg_cbk_eth[] = "\r\n-- Callback of ethernet --\r\n\r\n";
const VB apl_msg_cbk_trp_0[] = "\r\n-- Callback of vendor MIB (Default) --\r\n\r\n";
const VB apl_msg_cbk_trp_1[] = "\r\n-- Callback of vendor MIB 1 (Disk) --\r\n\r\n";
const VB apl_msg_cbk_trp_2[] = "\r\n-- Callback of vendor MIB 1 (Memory) --\r\n\r\n";
const VB apl_msg_adr_svr[] = "  To SNMP Manager: ";
const VB apl_msg_pass[] = "Pass\r\n";
const VB apl_msg_err[] = "Error";
const VB apl_msg_err_tmout[] = " (Timeout)";
const VB apl_msg_err_cmd[] = "Invalid command\r\n";
const VB apl_msg_err_cmd_long[] = "The input command is too long";
const VB apl_msg_usage[] = "Usage:\r\n" \
                          "  get mib_id obj_id      : Get Vendor MIB Data\r\n" \
                          "  set mib_id obj_id data : Set Vendor MIB Data\r\n" \
                          "  trp v1    : Send Traps (Ver.1)\r\n" \
                          "  trp v2    : Send Traps (Ver.2c)\r\n" \
                          "  trp v1vb1 : Send Traps (Ver.2c) with Variable-bindings(1)\r\n" \
                          "  trp v2vb3 : Send Traps (Ver.2c) with Variable-bindings(3)\r\n" \
                          "  inf       : Send Inform\r\n" \
                          "  ip        : My IP Address\r\n" \
                          "\r\n";

/* Data buffer */
#define MAX_STR_LEN    64                       /* Maximum string buffer size */
#define MAX_DAT_LEN    CFG_SNMP_MIB_DAT_LEN     /* Maximum buffer size */
static UW apl_str_buf[MAX_STR_LEN / sizeof(UW)];
static UW apl_oid_buf[MAX_STR_LEN / sizeof(UW)];
static UW apl_dat_buf[MAX_DAT_LEN / sizeof(UW)];

/* Status */
#define STS_INV        0x0000    /* Invalid */
#define STS_INI        0x0001    /* Initialization */

/* Event flag */
#define FLG_CHG_ADR    0x0001    /* Change IP address */
#define FLG_CMD_MSK    0xffff    /* Command mask */

/* Time */
typedef struct t_apl_tm {
    UB tm_sec;
    UB tm_min;
    UB tm_hour;
    UB tm_mday;
    UB tm_mon;
    UH tm_year;
} T_APL_TM;

/* Manager */
typedef struct t_apl_mgr {
    ID id_flg;      /* Event flag */
    UW ip_adr;      /* IP Address */
    UH sts;         /* Status */
} T_APL_MGR;


static VB* str = (VB*)apl_str_buf;
static VB* oid = (VB*)apl_oid_buf;
static UW* dat = apl_dat_buf;

static T_APL_MGR apl_mgr = { 0 };
static VP shell_ctrl = NULL;
static T_APL_TM my_tim;


/*******************************
    Print a string
 *******************************/
static void apl_put_str(const VB* buf)
{
    shell_puts(shell_ctrl, buf);
}

/*******************************
    Print digits of a number
 *******************************/
static void apl_put_dig(UW num)
{
    VB c[10];

    net_itoa(num, c, 10);
    shell_puts(shell_ctrl, c);
}

/*******************************
    Print hexadecimal number
 *******************************/
void apl_put_hex(UW num)
{
    UW mask;
    INT i;
    UH n;
    VB c[2];

    mask = 0xf0000000;

    c[1]= '\0';
    for (i = 1; i <= 8; i++) {
        n = (num & mask) >> (32 - 4 * i);
        if (n < 0x0a) {
            c[0] = '0';
            c[0] += n;
        } else {
            c[0] = 'a';
            c[0] += n - 0x0a;
        }
        shell_puts(shell_ctrl, c);
        mask = mask >> 4;
    }

    return;
}


/*******************************
    Print remote IP address
 *******************************/
static void apl_put_rmt_adr(T_NODE* nod)
{
    VB* buf;

    buf = (VB*)apl_str_buf;

    ip_ntoa(buf, nod->ipa);
    put_str(buf);
    put_str(apl_msg_newline);

    return;
}

/*******************************
    Print callback data
 *******************************/
#if (DBG_ENA == 1)
static void apl_put_cbk(T_SNMP_CFG_CBK_DAT* cbk_dat)
{
    UW* dat;

    /* Print callback data */

    put_str("  Request type: ");
    if (cbk_dat->req == SNMP_REQ_GET) {
        put_str("SNMP_REQ_GET\r\n");
    } else if (cbk_dat->req == SNMP_REQ_SET) {
        put_str("SNMP_REQ_SET\r\n");
    }

    put_str("  OID: ");
    put_str(snmp_mib_ven[cbk_dat->mib_id].pre);
    put_str(snmp_mib_ven[cbk_dat->mib_id].mib[cbk_dat->obj_id].str);
    put_str("\r\n");

    put_str("  Vendor MIB ID: ");
    put_dig(cbk_dat->mib_id);
    put_str(" : ");
    put_dig(cbk_dat->obj_id);
    put_str("\r\n");

    put_str("  Data: ");
    dat = (UW*)cbk_dat->buf;
    if (snmp_mib_ven[cbk_dat->mib_id].mib[cbk_dat->obj_id].typ == TYP_OCT_STR) {
        put_str((const VB*)dat);
    } else {
        put_dig(*dat);
    }
    put_str("\r\n");

    put_str("  Size: ");
    put_dig(cbk_dat->dat_len);
    put_str(" (Buffer size: ");
    put_dig(cbk_dat->buf_len);
    put_str(")\r\n");

    return;
}
#endif

#define APL_TMZONE_JST   /* Japanese standard time (UTC + 9) */

/* Leap year macros (1900-2123) */
static const UW apl_tim_leap_years[] = {
    0x11111110, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111011};
#define APL_IS_LEAP_YEAR(x)   (apl_tim_leap_years[(x) >> 5] & (1 << ((x) & (32 - 1))))

/* Number of days in the month */
static const UB apl_tim_mdays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
#define APL_SECS_PER_DAY     86400
#define APL_SECS_PER_HOUR    3600
#define APL_SECS_PER_MIN     60

/* Base date for days calculation */
static const struct t_apl_tm apl_tim_base_tm[2] = {
    {16, 28, 6,  7, 2, 2036},
    {8,  14, 3, 20, 1, 1968}
};

UH nod_cnt;


/***********************************
    Puts timestamp to system time
 ***********************************/
static ER apl_set_tim(struct t_apl_tm* tim)
{
    ER ercd;
    INT i;
    UW pos_sec;
    UH leap_year;
    UD sec;
    SYSTIM sys_tim;

    if (tim == 0) {
        return E_PAR;
    }

    pos_sec = 0;
    leap_year = 0;

    /* Difference time from 1900-1-1 */
    for (i = 0; i < tim->tm_year; i++) {
        pos_sec += APL_IS_LEAP_YEAR(i) ? 366 : 365 ;
    }
    leap_year = APL_IS_LEAP_YEAR(tim->tm_year) ? 1 : 0 ;

    for (i = 1; i < (tim->tm_mon + 1); i++) {
        pos_sec += apl_tim_mdays[i - 1] + ((2 == i) ? leap_year : 0);
    }
    pos_sec += tim->tm_mday - 1;
    pos_sec *= APL_SECS_PER_DAY;

    /* Time zone offset */
    #if defined(APL_TMZONE_JST)
    pos_sec -= 9 * APL_SECS_PER_HOUR;   /* UTC + 9 */
    #endif

    pos_sec += tim->tm_sec;
    pos_sec += tim->tm_min * APL_SECS_PER_MIN;
    pos_sec += tim->tm_hour * APL_SECS_PER_HOUR;

    /* Convert to msec */
    sec = pos_sec;
    sec = sec * 1000;
    sys_tim.utime = (UW)(sec >> 32);
    sys_tim.ltime = (UW)sec;
    ercd = set_tim(&sys_tim);

    return ercd;
}

/***********************************
    Gets timestamp from system time
 ***********************************/
static ER apl_get_tim(struct t_apl_tm* tim)
{
    ER ercd;
    SYSTIM sys_tim;
    UD sec;
    UW pos_sec;
    UH leap_year;
    const struct t_apl_tm* base_tm;

    if (tim == 0) {
        return E_PAR;
    }

    /* Convert to sec */
    ercd = get_tim(&sys_tim);
    if (ercd != E_OK) {
        return E_OBJ;
    }
    sec = sys_tim.utime;
    sec = sec << 32;
    sec |= sys_tim.ltime;
    sec = sec / 1000;
    pos_sec = (UW)sec;

    base_tm = &apl_tim_base_tm[(pos_sec & 0x80000000) ? 1 : 0];
    pos_sec &= ~0x80000000;

    /* Time zone offset */
    #if defined(APL_TMZONE_JST)
    pos_sec += 9 * APL_SECS_PER_HOUR;   /* UTC + 9 */
    #endif

    /* Calculation */
    pos_sec += base_tm->tm_sec;
    tim->tm_sec = pos_sec % 60;
    pos_sec /= 60;

    pos_sec += base_tm->tm_min;
    tim->tm_min = pos_sec % 60;
    pos_sec /= 60;

    pos_sec += base_tm->tm_hour;
    tim->tm_hour = pos_sec % 24;
    pos_sec /= 24;

    pos_sec += (base_tm->tm_mday - 1);

    for (tim->tm_year = base_tm->tm_year; ; tim->tm_year++) {
        leap_year = APL_IS_LEAP_YEAR(tim->tm_year - 1900) ? 366 : 365;
        if (pos_sec >= leap_year) {
            pos_sec -= leap_year;
        } else {
            break;
        }
    }
    leap_year -= 365;

    for (tim->tm_mon = base_tm->tm_mon;;) {
        tim->tm_mday = apl_tim_mdays[tim->tm_mon - 1] + ((2 == tim->tm_mon) ? leap_year : 0);
        if (pos_sec >= tim->tm_mday) {
            pos_sec -= tim->tm_mday;
            if (++tim->tm_mon > 12) {
                tim->tm_mon = 1;
                ++tim->tm_year;
                leap_year = APL_IS_LEAP_YEAR(tim->tm_year - 1900) ? 1 : 0;
            }
        } else {
            break;
        }
    }

    tim->tm_mday = 1 + pos_sec;

    /* Convert to time */
    tim->tm_mon -= 1;
    tim->tm_year -= 1900;

    return E_OK;
}


/*******************************
    MIB data Initialized
 *******************************/
static void apl_snmp_mib_ven_ini(INT sts)
{
    /* MIB 0 data */
    snmp_mib_ven_dat_0[4] = (VP)((ADDR)sts);    /* Status */

    return;
}

/*******************************
    Send traps
 *******************************/
static ER apl_snmp_snd_trp(T_SNMP_TRP* trp, TMO tmo)
{
    ER ercd;
    INT i;

    i = 0;
    while (snmp_cfg_trp[i].str != 0x00) {
        apl_put_str(apl_msg_adr_svr);
        apl_put_rmt_adr(snmp_cfg_trp[i].nod);
        ercd = snd_trp(snmp_cfg_trp[i].nod, trp, TRP_TMO);
        if (ercd != E_OK) {
            break;
        }
        i++;
    }

    return ercd;
}

/***********************************************
    Default callback function of vendor MIB 0
 ***********************************************/
ER apl_snmp_cbk_0(T_SNMP_CFG_CBK_DAT* cbk_dat)
{
    ER ercd;
    struct t_apl_tm tim = {0};
    UW tim_msec;
    UW* dat;
    VB* str;
    UH len;
    UH i;
    INT res;
    FLGPTN flg;

   put_str(apl_msg_cbk_trp_0);

    ercd = E_OK;

    if (cbk_dat->req == SNMP_REQ_GET) {
        /* Get request */
        if (cbk_dat->mib_id == 0) {
            /* MIB ID 0 */
            switch (cbk_dat->obj_id) {
                case MIB_0_DESC:
                    /* Descriptor */
                    len = net_strlen(apl_decr_new);
                    if (len < cbk_dat->buf_len) {
                        /* strcpy */
                        net_strcpy((char*)cbk_dat->buf, apl_decr_new);
                        cbk_dat->dat_len = len;
                    }
                    break;
                case MIB_0_VER:
                    /* Version descriptor */
                    len = net_strlen(apl_ver_new);
                    if (len < cbk_dat->buf_len) {
                        str = (VB*)cbk_dat->buf;
                        /* Copy */
                        for (i = 0; i < len; i++) {
                            str[i] = apl_ver_new[i];
                        }
                        cbk_dat->dat_len = len;
                    }
                    break;
                    
                case MIB_0_USR_NAME:
                    cbk_dat->dat_len = net_strlen((VB*)cbk_dat->buf);
                    break;
                    
                case MIB_0_TIME:
                    /* Time tick */
                    /* Convert to time ticks from day-hour-min-sec */
                    apl_get_tim(&tim);
                    tim_msec = tim.tm_sec;
                    tim_msec += tim.tm_min * APL_SECS_PER_MIN;
                    tim_msec += tim.tm_hour * APL_SECS_PER_HOUR;
                    tim_msec += tim.tm_mday * APL_SECS_PER_DAY;
                    tim_msec *= 100;
                    dat = (UW*)cbk_dat->buf;
                    *dat = tim_msec;
                    break;
                case MIB_0_CNT:
                    /* Counter */
                    dat = (UW*)cbk_dat->buf;
                    *dat += 1;
                    break;
                case MIB_0_GUAGE:
                    /* Gauge */
                    dat = (UW*)cbk_dat->buf;
                    *dat += 10;
                    break;
                default:
                    break;
            }

        }
    } else if (cbk_dat->req == SNMP_REQ_SET) {
        /* Set request */
        if (cbk_dat->mib_id == 0) {
            /* MIB ID 0 */
            switch (cbk_dat->obj_id) {
                case MIB_0_USR_NAME:
                    /* User name */
                    len = net_strlen(apl_usr_root);
                    res = net_strncmp((const char*)cbk_dat->buf, (const char*)apl_usr_root, len);
                    if (res == 0) {
                        ercd = E_OBJ;    /* root is inhibitted */
                    }
                    break;
                case MIB_0_IP_ADR:
                    /* IP address */
                    dat = (UW*)cbk_dat->buf;
                    if ((*dat & 0xffff0000) != 0xc0a80000) {
                        ercd = E_OBJ;    /* Invalid IP address */
                    } else {
                        /* Change IP address */
                        ercd = pol_flg(apl_mgr.id_flg, FLG_CHG_ADR, TWF_ORW, &flg);
                        if (ercd == E_TMOUT) {
                            apl_mgr.ip_adr = *dat;
                            set_flg(apl_mgr.id_flg, FLG_CHG_ADR);
                            ercd = E_OK;
                        } else {
                            ercd = E_OBJ;
                        }
                    }
                    break;
                default:
                    break;
            }
        }
    }

    #if (DBG_ENA == 1)
    apl_put_cbk(cbk_dat);    /* Debug print */
    #endif

    return ercd;
}

/***********************************************
    Callback function of vendor MIB 1 (Disk)
 ***********************************************/
ER apl_snmp_cbk_1(T_SNMP_CFG_CBK_DAT* cbk_dat)
{
    put_str(apl_msg_cbk_trp_1);

    #if (DBG_ENA == 1)
    apl_put_cbk(cbk_dat);    /* Debug print */
    #endif

    return E_OK;
}

/***********************************************
    Callback function of vendor MIB 1 (Memory)
 ***********************************************/
ER apl_snmp_cbk_2(T_SNMP_CFG_CBK_DAT* cbk_dat)
{
    put_str(apl_msg_cbk_trp_2);

    #if (DBG_ENA == 1)
    apl_put_cbk(cbk_dat);    /* Debug print */
    #endif

    return E_OK;
}


/* Command 'get' */
ER shell_usr_cmd_snmp_get(VP ctrl, INT argc, VB *argv[])
{
    ER ercd;
    UH mib_id;
    UH obj_id;
    UH len;

    mib_id = net_atoi(argv[1]);
    obj_id = net_atoi(argv[2]);

    len = MAX_DAT_LEN;
    ercd = get_mib_obj(dat, &len, mib_id, obj_id);
    if (ercd == E_OK) {
        put_str(SPL_LF"  ");
        put_str(snmp_mib_ven[mib_id].pre);
        put_str(snmp_mib_ven[mib_id].mib[obj_id].str);
        put_str(": ");
        if (snmp_mib_ven[mib_id].mib[obj_id].typ == TYP_OCT_STR) {
            /* TYP_OCT_STR */
            ((VB*)dat)[len] = '\0';
            put_str((const VB*)dat);
        } else if (snmp_mib_ven[mib_id].mib[obj_id].typ == TYP_IP_ADR) {
            /* TYP_IP_ADR (IP Address (4-byte)) */
            ip_ntoa(str, *dat);
            put_str(str);
        } else {
            /* TYP_INT, TYP_CNT, TYP_GAUGE, TYP_TIM_TIC (4-byte) */
            put_dig(*dat);
        }
    }

    return ercd;
}


/* Command 'set' */
ER shell_usr_cmd_snmp_set(VP ctrl, INT argc, VB *argv[])
{
    ER ercd;
    UH mib_id;
    UH obj_id;
    UH len;

    mib_id = net_atoi(argv[1]);
    obj_id = net_atoi(argv[2]);

    if (snmp_mib_ven[mib_id].mib[obj_id].typ == TYP_OCT_STR) {
        /* TYP_OCT_STR */
        len = net_strlen((const char*)argv[3]);
        ercd = set_mib_obj(argv[3], len, mib_id, obj_id);
    } else {
        /* TYP_INT, TYP_CNT, TYP_GAUGE, TYP_IP_ADR, TYP_TIM_TIC (4-byte) */
        *dat = net_atol(argv[3]);
        ercd = set_mib_obj(dat, 4, mib_id, obj_id);
    }

    return ercd;
}

/* Command 'trp' */
ER shell_usr_cmd_snmp_trp(VP ctrl, INT argc, VB *argv[])
{
    ER ercd;
    T_SNMP_TRP trp;

    apl_put_str(SPL_LF);
	if (!net_strcmp(argv[1], "v1")) {
        /* Trap (v1) */
        apl_put_str("-- Trap v1 --\r\n");

        net_memset(&trp, 0, sizeof(trp));
        trp.ver = SNMP_VER_V1;         /* Version v1 */
        trp.com = apl_snmp_com;        /* Community */
        trp.gen_trp = TRP_ENT_SPEC;    /* Vendor trap */
        trp.spc_trp = 1234;            /* Specific */
        ercd = apl_snmp_snd_trp(&trp, TRP_TMO);
        /* trp.ent_oid = 0: Enterprise OID is CFG_SNMP_MIB_SYS_OBJECTID(snmp_mib_cfg.h) */
        /* trp.var_cnt = 0, trp.var_oid = 0: Without variable-bindings */
    } else if (!net_strcmp(argv[1], "v2")) {
        /* Trap (v2c) */
        apl_put_str("-- Trap v2c --\r\n");

        net_memset(&trp, 0, sizeof(trp));
        trp.ver = SNMP_VER_V2C;    /* Version v2c */
        trp.com = apl_snmp_com;    /* Community */
                                   /* Vendor MIB: 1.3.6.1.4.1.1234.1.2 */
        net_strcpy(oid, snmp_mib_ven[0].pre);
        net_strcat(oid, snmp_mib_ven[0].mib[2].str);
        trp.ent_oid = oid;         /* snmpTrapOID */
        ercd = apl_snmp_snd_trp(&trp, TRP_TMO);
        /* trp.var_cnt = 0, trp.var_oid = 0: Without variable-bindings */
    } else if (!net_strcmp(argv[1], "v1vb1")) {
        /* Trap (v1) with variable-binding(1) */
        apl_put_str("-- Trap v1 with variable(1) --\r\n");

        net_memset(&trp, 0, sizeof(trp));
        trp.ver = SNMP_VER_V1;         /* Version v1 */
        trp.com = apl_snmp_com;        /* Community */
        trp.flg = 0x00;                /* Trap */
        trp.gen_trp = TRP_ENT_SPEC;    /* Vendor trap */
        trp.spc_trp = 1234;            /* Specific */
        trp.ent_oid = "1.3.6.1.4.1.9876.1234";    /* Enterprise OID */
        trp.var_cnt = 0;               /* Number of variable-binding (1) */
        trp.var_oid = TRP_VAR_ID(0, 2);    /* ID of variable-binding */
        ercd = apl_snmp_snd_trp(&trp, TRP_TMO);
    } else if (!net_strcmp(argv[1], "v2vb3")) {
        /* Trap (v2c) with variable-bindings(3) */
        apl_put_str("-- Trap v2c with variable(3) --\r\n");

        net_memset(&trp, 0, sizeof(trp));
        trp.ver = SNMP_VER_V2C;    /* Version v2c */
        trp.com = apl_snmp_com;    /* Community */
                                   /* Vendor MIB: 1.3.6.1.4.1.1234.1.3 */
        net_strcpy(oid, snmp_mib_ven[0].pre);
        net_strcat(oid, snmp_mib_ven[0].mib[2].str);
        trp.ent_oid = oid;
        trp.var_cnt = 3;    /* Number of variable bindings (3) */
        apl_var_id[0] = TRP_VAR_ID(0, 2);
        apl_var_id[1] = TRP_VAR_ID(1, 7);
        apl_var_id[2] = TRP_VAR_ID(1, 19);
        trp.var_oid = apl_var_id;    /* Array of variable bindings */
        ercd = apl_snmp_snd_trp(&trp, TRP_TMO);
    } else {
        ercd = E_PAR;
    }


    return ercd;
}

/* Command 'inf' */
ER shell_usr_cmd_snmp_inf(VP ctrl, INT argc, VB *argv[])
{
    ER ercd;
    T_SNMP_TRP trp;
    T_NODE nod;

    /* Inform */
    apl_put_str(SPL_LF"-- Inform --\r\n");

    nod.ver = IP_VER4;                    /* IPv4 */
    nod.ipa = ip_aton(apl_server_adr);    /* IP address of SNMP manager */
    /* nod.port = 162; */                 /* Disused */
    nod.num = ID_NET_DEV;                 /* Device ID of ethernet */

    net_memset(&trp, 0, sizeof(trp));
    trp.ver = SNMP_VER_V2C;    /* Version v2c */
    trp.com = apl_snmp_com;    /* Community */
    trp.flg = TRP_INF_ENA;     /* Inform */
                               /* Vendor MIB: 1.3.6.1.4.1.1234.1.2 */
    net_strcpy(oid, snmp_mib_ven[0].pre);
    net_strcat(oid, snmp_mib_ven[0].mib[2].str);
    trp.ent_oid = oid;
    trp.tmo = INF_TMO;             /* Timeout of inform (msec) */
    trp.rty_cnt = INF_RTY_CNT;     /* Number of inform retries */
    ercd = snd_trp(&nod, &trp, TRP_TMO);



    return ercd;
}


/* Command 'nod' */
ER shell_usr_cmd_snmp_nod(VP ctrl, INT argc, VB *argv[])
{
    put_str(apl_msg_mib_cnt);
    put_dig(CFG_SNMP_MIB_NOD_CNT);
    put_str("(CFG_SNMP_MIB_NOD_CNT) -> ");
    put_dig(nod_cnt);
    put_str("\r\n");

    return E_OK;
}

ER apl_snmp_tsk(VP_INT exinf)
{
    ER ercd;
    T_NET_ADR adr;
    FLGPTN flg;

    /* Wait for application command */
    ercd = E_OK;
    while (ercd == E_OK) {
        ercd = wai_flg(apl_mgr.id_flg, FLG_CMD_MSK, TWF_ORW, &flg);
        if (ercd == E_OK) {
            if ((flg & FLG_CHG_ADR) != 0x00) {
                /* Change IP address */
                dly_tsk(200);
                ercd = net_ref(ID_NET_DEV, NET_IP4_CFG, (VP)&adr);
                adr.ipaddr = apl_mgr.ip_adr;
                //adr.mask = gNET_ADR[ID_NET_DEV - 1].mask;
                //adr.gateway = gNET_ADR[ID_NET_DEV - 1].gateway;
                ercd = net_cfg(ID_NET_DEV, NET_IP4_CFG, (VP)&adr);
                clr_flg(apl_mgr.id_flg, ~FLG_CHG_ADR);
            }
        }
    }
    
    return ercd;
}

ER apl_snmp_ini(VP ctrl)
{
    ER ercd;

    /* Event flag */
#if defined(NET_C_OS)
    apl_mgr.id_flg = ID_APL_FLG;
#else
    T_CFLG cflg = {TA_TFIFO | TA_WMUL, 0x00000000, "SnmpAplFlg"};

    ercd = acre_flg((T_CFLG*)&cflg);
    if (ercd <= E_OK) {
        return ercd;
    }
    apl_mgr.id_flg = ercd;
#endif

    /* Real time(2014-02-14 15:02:38) set into system time */
    my_tim.tm_sec = 38;
    my_tim.tm_min = 2;
    my_tim.tm_hour = 15;
    my_tim.tm_mday = 14;
    my_tim.tm_mon = 2 - 1;
    my_tim.tm_year = 2014 - 1900;
    ercd = apl_set_tim(&my_tim);
    if (ercd != E_OK) {
        put_str(apl_msg_err);
        return ercd;
    }

    /* MIB data initialized (snmp_mib_cfg.c) */
    apl_snmp_mib_ven_ini(apl_mgr.sts);    /* Do call if you need */

    apl_mgr.sts = STS_INI;

    /* shell_puts/gets target set */
    shell_ctrl = ctrl;

#if (0 == SAMPLE_USE_GENSRC)	/* no use configurator */
    /* Initialize SNMP */
    ercd = snmp_ini(&nod_cnt);
    if (ercd != E_OK) {
        return ercd;
    }       
    
    /* Enable SNMP */
    ercd = snmp_ena();
#endif
    
    return ercd;
}
