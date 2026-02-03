/*
    SNMP
    MIB
    Copyright (c) 2014-2023, eForce Co., Ltd. All rights reserved.
    
    2014-03-06 Created
    2014-05-14 Function for node delete was fixed
    2014-06-10 Size of ifPhysAddress was fixed
    2014-06-18 Get-next was fixed
    2015-11-02 Bug fixed. Next-node of zero-terminated OID
    2016-01-06 Multiple network devices were supported
    2016-01-06 Function for node delete was fixed
    2016-04-18 snmp_add_val_mib_nod and snmp_del_val_mib_nod were added
    2016-05-06 Bug fixed SetRequest for variable vendor MIB
    2016-05-09 Bug fixed snmp_mib_set_dat_oid for variable vendor MIB
    2017-04-20 Support OID type for private MIB data
    2019-02-25 Support Counter64(SNMP_TYP_CNT64) type for private MIB data.
               Support accessible-for-notify(SNMP_STS_AN) status for private MIB data.
    2023-05-25 Suppression of warnings in 64bit environment.
*/

#include "kernel.h"
#include "snmp.h"
#include "snmp_ber.h"
#include "snmp_lib.h"
#include "snmp_def.h"
#include "snmp_mib.h"

extern const T_SNMP_CFG_USR snmp_cfg_usr;
extern UW snmp_cfg_tcp_buf[];
extern UW snmp_cfg_buf[];
extern const VB snmp_mib_mib2_pre[];
extern const T_SNMP_MIB snmp_mib[];
extern T_SNMP_MIB_TBL snmp_mib_ven[];
extern T_SNMP_MIB_TBL snmp_mib_ven_val[];

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

/* Configuration */
#define MAX_NOD_CNT     0xffff          /* Maximum number of nodes */

/* Configuration values */
#define CFG_NET_DEV_CNT     snmp_cfg_usr.net_dev_cnt
#define CFG_MIB_NOD_CNT     snmp_cfg_usr.mib_nod_cnt
#define CFG_MAX_MIB_DEP     snmp_cfg_usr.max_mib_dep
#define CFG_MAX_MIB_DAT     snmp_cfg_usr.max_mib_dat
#define CFG_MAX_OID_DEP     snmp_cfg_usr.max_oid_dep

/* Status */
#define STS_INV    0x0000    /* Invalid */
#define STS_INI    0x0001    /* Initialized */
#define STS_ENA    0x0004    /* Enable */
#define STS_DIS    0x0008    /* Disable */
#define STS_ERR    0x8000    /* Error */

/* Manager */
typedef struct t_snmp_mib_mgr {
    T_SNMP_MIB_NOD* nod_buf;        /* Node buffer */
    T_SNMP_MIB_TCP_DAT* tcp_dat;    /* TCP variable data buffer */
    T_SNMP_BER_OID* dec_oid;        /* OID decoded from BER for MIB data */
    VB* dec_oid_str;                /* String representation for dec_oid */
    UH nod_top_id;                  /* First index of node buffer */
    UH nod_buf_cnt;                 /* Number of node buffer */
    UH sts;                         /* Status */
} T_SNMP_MIB_MGR;

/* Variables */
static T_SNMP_MIB_MGR snmp_mib_mgr = { 0 };     /* Manager */

/* Conversion macros */
#define mgr    snmp_mib_mgr

/* Macros */
#define BUF_UW_LEN(x)    ((x + (sizeof(UW) - 1)) / sizeof(UW))

#if (CFG_DBG_ENA == 1)
void snmp_mib_dbg_put_tree(void)
{
    T_SNMP_MIB_NOD* node;
    UH i;
    T_SNMP_MIB* mib;
    T_SNMP_MIB_TCP_DAT* mib_tcp;
    const T_SNMP_MIB* mib_root;
    UH id_ven;
    UH id;

    /* Debug print of a MIB tree */

    dbg_put_str("\r\nMIB Tree (nod_top_id: ");
    dbg_put_dig(mgr.nod_top_id);
    dbg_put_str(", nod_buf_cnt: ");
    dbg_put_dig(mgr.nod_buf_cnt);
    dbg_put_str(")\r\n");
    dbg_put_str("      node,   parent,    child,     next,     prev,     data, typ, num, ac, str\r\n");

    node = &mgr.nod_buf[0];

    for (i = 0; i < CFG_MIB_NOD_CNT; i++) {
        dbg_put_str("  ");
        dbg_put_hex((UW)&node[i]);
        dbg_put_str(", ");
        dbg_put_hex((UW)node[i].parent);
        dbg_put_str(", ");
        dbg_put_hex((UW)node[i].child);
        dbg_put_str(", ");
        dbg_put_hex((UW)node[i].next);
        dbg_put_str(", ");
        dbg_put_hex((UW)node[i].prev);
        dbg_put_str(", ");
        dbg_put_hex((UW)node[i].dat);
        dbg_put_str(", ");
        switch(node[i].typ & MIB_NOD_TYP) {
            case MIB_NOD_INV:
                dbg_put_str("INV");
                break;
            case MIB_NOD_STD:
                if ((node[i].typ & MIB_NOD_FLG) == MIB_NOD_TBL) {
                    dbg_put_str("STB");
                } else {
                    dbg_put_str("STD");
                }
                break;
            case MIB_NOD_VEN:
                dbg_put_str("VEN");
                break;
            case (MIB_NOD_VAL | MIB_NOD_STD):
                dbg_put_str("STD/VAL");
                break;
            case (MIB_NOD_VAL | MIB_NOD_VEN):
                dbg_put_str("VEN/VAL");
                break;
        }
        dbg_put_str(", ");
        dbg_put_dig(node[i].num);
        if (((node[i].typ & MIB_NOD_TYP) != MIB_NOD_INV) && (node[i].dat != 0x00)) {
            dbg_put_str(", ");
            if ((node[i].typ & MIB_NOD_TYP) != (MIB_NOD_VAL | MIB_NOD_STD)) {
                mib = (T_SNMP_MIB*)node[i].dat;
                switch(mib->acs) {
                    case STS_NO:
                        dbg_put_str("NO");
                        break;
                    case STS_RO:
                        dbg_put_str("RO");
                        break;
                    case STS_RW:
                        dbg_put_str("RW");
                        break;
                    case STS_AN:
                        dbg_put_str("AN");
                        break;
                    default:
                        break;
                }
                dbg_put_str(", ");
                dbg_put_str(mib->str);
                if ((node[i].typ & MIB_NOD_TYP) == MIB_NOD_STD) {
                    mib_root = snmp_mib;
                } else if ((node[i].typ & MIB_NOD_TYP) == MIB_NOD_VEN) {
                    id_ven = node[i].typ & MIB_NOD_ID;
                    mib_root = snmp_mib_ven[id_ven].mib;
                } else {
                    id_ven = node[i].typ & MIB_NOD_ID;
                    mib_root = snmp_mib_ven_val[id_ven].mib;
                }
                id = ((UW)node[i].dat - (UW)&mib_root[0]) / sizeof(T_SNMP_MIB);
                dbg_put_str(", ID(");
                if ((node[i].typ & MIB_NOD_VEN) != 0x00U) {
                    dbg_put_dig(id_ven);
                    dbg_put_str("/");
                }
                dbg_put_dig(id);
                dbg_put_str(")");
            } else {
                mib_tcp = (T_SNMP_MIB_TCP_DAT*)node[i].dat;
                switch(mib_tcp->acs) {
                    case STS_NO:
                        dbg_put_str("NO");
                        break;
                    case STS_RO:
                        dbg_put_str("RO");
                        break;
                    case STS_RW:
                        dbg_put_str("RW");
                        break;
                    case STS_AN:
                        dbg_put_str("AN");
                        break;
                    default:
                        break;
                }
                dbg_put_str(", ---");
            }
        }
        dbg_put_str(", num(");
        dbg_put_dig(i);
        dbg_put_str(")\r\n");
    }

    return;
}
#endif

static T_SNMP_MIB_NOD* snmp_mib_nod_buf_get(void)
{
    INT i;

    /* Allocate node buffer */

    for (i = mgr.nod_top_id; i < CFG_MIB_NOD_CNT; i++) {
        if (mgr.nod_buf[i].typ == MIB_NOD_INV) {
            mgr.nod_buf_cnt--;
            return &mgr.nod_buf[i];
        }
    }

    return 0;
}

static ER snmp_mib_nod_buf_rel(T_SNMP_MIB_NOD* node)
{
    /* Release node buffer */

    if ((node->typ & MIB_NOD_VAL) == 0x00U) {
        return E_OBJ;
    }

    node->parent = 0;
    node->child = 0;
    node->next = 0;
    node->prev = 0;
    node->dat = 0;
    node->num = 0;
    node->typ = MIB_NOD_INV;

    mgr.nod_buf_cnt++;

    return E_OK;
}

static ER snmp_mib_get_nod_oid_str(T_SNMP_MIB_NOD** dst_nod, const VB* oid_str, UH oid_len)
{
    ER ercd;
    T_SNMP_MIB_NOD* node;
    UW num;
    const VB* str_end;
    UH flg_limit;

    /* Retrieve a node for OID string */

    if (dst_nod == 0x00 || oid_str == 0x00 || oid_len == 0) {
        return E_PAR;
    }
    *dst_nod = 0;
    
    /* Root */
    node = &mgr.nod_buf[0];
    str_end = oid_str + oid_len;

    while (oid_str < str_end) {
        num = ber_atoi(&oid_str, str_end - oid_str);
        ercd = E_OBJ;
        flg_limit = 0;
        while (node != 0x00) {
            if (num == node->num) {
                ercd = E_OK;
                break;
            }
            node = node->next;
            flg_limit++;
            if (flg_limit >= MAX_NOD_CNT) {
                break;
            }
        }
        if (ercd != E_OK) {
            return E_OBJ;
        }
        *dst_nod = node;
        node = node->child;
        
        if (oid_str >= str_end) {
            break;
        }
        if (*oid_str != '.') {
            return E_OBJ;
        }
        oid_str++;
    }

    return E_OK;
}

static ER snmp_mib_cre_nod_cnt(const VB* oid_str, UH oid_len, UH typ, UH* nod_cnt)
{
    ER ercd;
    T_SNMP_MIB_NOD* node;
    UW num;
    const VB* str_end;
    T_SNMP_MIB_NOD* pre;
    T_SNMP_MIB_NOD* tmp;
    INT flg_find;
    UH cnt_max;

    /* Number of nodes for new MIB */

    if ((oid_str == 0) || (oid_len == 0) || (nod_cnt == 0)) {
        return E_PAR;
    }

    /* Root */
    node = &mgr.nod_buf[0];
    str_end = oid_str + oid_len;
    num = ber_atoi(&oid_str, oid_len);
    if ((num != node->num) || (*oid_str != '.')) {
        return E_OBJ;    /* Root number incorrect */
    }
    oid_str++;

    *nod_cnt = 0;
    while (oid_str < str_end) {
        num = ber_atoi(&oid_str, str_end - oid_str);
        if (node->child == 0) {
            /* Create new child */
            *nod_cnt += 1;
            break;
        } else {
            tmp = node->child;
            if (tmp->num > num) {
                /* Create top child */
                *nod_cnt += 1;
                break;
            } else {
                /* Next */
                pre = 0;
                tmp = node->child;
                flg_find = 0;
                cnt_max = 0;
                while (tmp != 0) {
                    if (tmp->num == num) {
                        flg_find = 1;   /* Found */
                        node = tmp;
                        break;
                    } else if (tmp->num > num) {
                        if (pre == 0) {
                            return E_OBJ;    /* Tree incorrect */
                        }
                        break;
                    }
                    pre = tmp;
                    tmp = tmp->next;
                    cnt_max++;
                    if (cnt_max >= MAX_NOD_CNT) {
                        return E_OBJ;
                    }
                }
                if (flg_find == 0) {
                    /* Create next */
                    *nod_cnt += 1;
                    break;
                }
            }
        }
        if (oid_str >= str_end) {
            break;
        }
        if (*oid_str != '.') {
            return E_OBJ;    /* Dot not found */
        }
        oid_str++;
    }

    /* Count of new node */
    ercd = E_OK;
    while (oid_str < str_end) {
        if (*oid_str == '.') {
            oid_str++;
            num = ber_atoi(&oid_str, str_end - oid_str);
            *nod_cnt += 1;
        } else {
            ercd = E_OBJ;    /* Dot not found */
            break;
        }
    }

    return ercd;
}

static ER snmp_mib_cre_nod(const VB* oid_str, UH oid_len, UH typ, T_SNMP_MIB_NOD** dst_nod, UH* nod_cnt)
{
    T_SNMP_MIB_NOD* node;
    UW num;
    const VB* str_end;
    T_SNMP_MIB_NOD* pre;
    T_SNMP_MIB_NOD* tmp;
    T_SNMP_MIB_NOD* new_node;
    INT flg_find;
    UH typ_tbl_node;
    UH typ_tbl;
    UH cnt;
    UH flg_limit;

    /* Create MIB node */

    if (oid_str == 0x00 || oid_len == 0) {
        dbg_put_str("Error: E_PAR (MIB object is NULL)\r\n");
        return E_PAR;
    }

    /* Root */
    node = &mgr.nod_buf[0];
    str_end = oid_str + oid_len;
    num = ber_atoi(&oid_str, oid_len);
    if (num != node->num || *oid_str != '.') {
        dbg_put_str("Error: E_OBJ 1 (Root number is incorrect)\r\n");
        return E_OBJ;
    }
    oid_str++;

    typ_tbl_node = MIB_NOD_TBL;
    typ_tbl = 0x00;
    cnt = 0;
    while (oid_str < str_end) {
        num = ber_atoi(&oid_str, str_end - oid_str);
        if (num == 0 && oid_str >= str_end) {
            typ_tbl_node = 0x00;    /* Last number is zero. Not table object */
        }
        if (node->child == 0x00) {
            /* Create new child */
            new_node = snmp_mib_nod_buf_get();
            if (new_node == 0x00) {
                dbg_put_str("Error: E_NOMEM 1 (CFG_MIB_NOD_CNT is not enough)\r\n");
                return E_NOMEM;
            }
            new_node->parent = node;
            new_node->child = 0x00;
            new_node->next = 0x00;
            new_node->prev = 0x00;
            new_node->dat = 0x00;
            new_node->num = num;
            new_node->typ = typ | typ_tbl;
            node->child = new_node;
            node = new_node;
            cnt++;
        } else {
            tmp = node->child;
            if (tmp->num > num) {
                /* Create top child */
                new_node = snmp_mib_nod_buf_get();
                if (new_node == 0x00) {
                    dbg_put_str("Error: E_NOMEM 2 (CFG_MIB_NOD_CNT is not enough)\r\n");
                    return E_NOMEM;
                }
                new_node->parent = node;
                new_node->child = 0x00;
                new_node->next = node->child;
                new_node->prev = 0x00;
                new_node->dat = 0x00;
                new_node->num = num;
                new_node->typ = typ | typ_tbl;
                tmp->prev = new_node;
                node->child = new_node;
                node = new_node;
                cnt++;
            } else {
                /* Next */
                pre = 0;
                tmp = node->child;
                flg_find = 0;
                flg_limit = 0;
                while (tmp != 0x00) {
                    if (tmp->num == num) {
                        flg_find = 1;   /* Found */
                        node = tmp;
                        if ((node->typ & MIB_NOD_TBL) != 0x00) {
                            typ_tbl = MIB_NOD_TBL;
                        }
                        break;
                    } else if (tmp->num > num) {
                        if (pre == 0) {
                            dbg_put_str("Error: E_OBJ 2 (Node number is incorrect)\r\n");
                            return E_OBJ;
                        }
                        break;
                    }
                    pre = tmp;
                    tmp = tmp->next;
                    flg_limit++;
                    if (flg_limit >= MAX_NOD_CNT) {
                        return E_OBJ;
                    }
                }
                if (flg_find == 0) {
                    /* Create next */
                    new_node = snmp_mib_nod_buf_get();
                    if (new_node == 0) {
                        dbg_put_str("Error: E_NOMEM 3 (CFG_MIB_NOD_CNT is not enough)\r\n");
                        return E_NOMEM;
                    }
                    new_node->parent = node;
                    new_node->child = 0x00;
                    new_node->next = tmp;
                    new_node->prev = pre;
                    new_node->dat = 0x00;
                    new_node->num = num;
                    new_node->typ = typ | typ_tbl;
                    pre->next = new_node;
                    if (tmp != 0) {
                        tmp->prev = new_node;
                    }
                    node = new_node;
                    cnt++;
                }
            }
        }
        if (oid_str >= str_end) {
            break;
        }
        if (*oid_str != '.') {
            dbg_put_str("Error: E_OBJ 3 (Dot not found)\r\n");
            return E_OBJ;
        }
        oid_str++;
    }
    node->typ |= typ_tbl_node;

    if (dst_nod != 0x00) {
        *dst_nod = node;
    }
    if (nod_cnt != 0x00) {
        *nod_cnt = cnt;
    }

    return E_OK;
}

static ER snmp_mib_del_nod(const VB* oid_str, UH oid_len, VP* dat_ptr)
{
    ER ercd;
    UH flg_limit;
    T_SNMP_MIB_NOD* node;
    T_SNMP_MIB_NOD* node_tmp;

    /* Delete MIB node */

    if (oid_str == 0x00 || oid_len == 0) {
        return E_PAR;
    }

    /* Get a data node */
    ercd = snmp_mib_get_nod_oid_str(&node, oid_str, oid_len);
    if (ercd != E_OK) {
        return E_OBJ;
    }
    if ((node->child != 0) || (node->dat == 0)) {    /* Node has child */
        return E_OBJ;
    }
    if ((node->typ & MIB_NOD_VAL) == 0x00U) {
        return E_OBJ;
    }
    if (dat_ptr != 0x00) {
        *dat_ptr = node->dat;
    }

    /* Delete link */
    flg_limit = 0U;
    while ((node != 0) && ((node->typ & MIB_NOD_VAL) != 0x00U)) {
        if (node->child != 0) {
            break;
        } else if (node->next != 0) {
            /* Link to next chain */
            node_tmp = (T_SNMP_MIB_NOD*)node->next;
            node_tmp->prev = node->prev;
            if (node->prev != 0x00) {
                node_tmp = (T_SNMP_MIB_NOD*)node->prev;
                node_tmp->next = node->next;
            } else {
                node_tmp = (T_SNMP_MIB_NOD*)node->parent;
                node_tmp->child = node->next;
            }
            /* Release data node */
            ercd = snmp_mib_nod_buf_rel(node);
            break;
        } else {
            if (node->parent == 0) {
                /* Delete root */
                /* Release data node */
                ercd = snmp_mib_nod_buf_rel(node);
                break;
            } else if (node->prev == 0) {
                /* Delete and move to parent */
                node_tmp = (T_SNMP_MIB_NOD*)node->parent;
                ercd = snmp_mib_nod_buf_rel(node);
                if (ercd != E_OK) {
                    break;
                }
                node = node_tmp;
                node->child = 0;
            } else {
                /* Delete */
                node_tmp = (T_SNMP_MIB_NOD*)node->prev;
                ercd = snmp_mib_nod_buf_rel(node);
                if (ercd != E_OK) {
                    break;
                }
                node_tmp->next = 0;
                break;
            }
        }
        flg_limit++;
        if (flg_limit >= MAX_NOD_CNT) {
            ercd = E_OBJ;
            break;
        }
    }

    return ercd;
}

static ER snmp_mib_cre_tree(T_SNMP_MIB_TBL* mib_tbl, UH typ, VP oid_str_buf, UH oid_str_len, UH* nod_cnt)
{
    ER ercd;
    UH i;
    const VB* str;
    UH len;
    T_SNMP_MIB_NOD* node;
    INT flg_find;
    UH cnt;
    UH total_cnt;

    /* Create MIB tree */

    if (oid_str_buf == 0x00 || oid_str_len == 0) {
        return E_PAR;
    }

    flg_find = 0;
    total_cnt = 0;
    for (i = 0; i < MAX_NOD_CNT; i++) {
        str = mib_tbl->mib[i].str;
        if (str == 0x00) {
            flg_find = 1;
            break;
        }
        /* Check for maximum data length */
        if (mib_tbl->mib[i].len > CFG_MAX_MIB_DAT) {
            dbg_put_str("Error: E_OBJ 4 (CFG_SNMP_MIB_DAT_LEN is short)\r\n");
            return E_BOVR;
        }
        len = snmp_strlen(mib_tbl->pre);
        len += snmp_strlen(mib_tbl->mib[i].str);
        if (len + 1 > oid_str_len) {
            dbg_put_str("Error: E_NOMEM 0 (CFG_SNMP_MAX_MIB_DEP is not enough)\r\n");
            return E_NOMEM;
        }
        snmp_strcpy(oid_str_buf, mib_tbl->pre);
        snmp_strcat(oid_str_buf, mib_tbl->mib[i].str);
        str = oid_str_buf;
        /* Create node */
        len = snmp_strlen(str);
        ercd = snmp_mib_cre_nod(str, len, typ, &node, &cnt);
        if (ercd != E_OK) {
            return E_OBJ;
        }
        total_cnt += cnt;
        /* Data */
        node->dat = (VP)&mib_tbl->mib[i];    /* Pointer of MIB array (Not mib_dat) */
    }
    if (flg_find != 1) {
        dbg_put_str("Error: E_OBJ 5 (Null-terminate not found)\r\n");
        return E_OBJ;
    }

    if (nod_cnt != 0x00) {
        *nod_cnt = total_cnt;
    }

    return E_OK;
}

static ER snmp_mib_get_oid_nod(T_SNMP_BER_OID* oid, T_SNMP_MIB_NOD* node)
{
    INT len;
    INT i;
    UW tmp;
    UH id;

    /* Retrive an OID string from node */

    len = 0;
    while (node != 0x00) {
        oid->buf[len++] = node->num;
        if (len > CFG_MAX_MIB_DEP) {
            return E_OBJ;
        }
        node = node->parent;
    }

    /* Reverse order */
    for (i = 0; i < len / 2; i++) {
        tmp = oid->buf[i];
        id = len - 1 - i;
        oid->buf[i] = oid->buf[id];
        oid->buf[id] = tmp;
    }

    oid->len = len;

    return E_OK;
}

static ER snmp_mib_get_nod(T_SNMP_MIB_NOD** dst_node, INT* oid_idx, T_SNMP_BER_OID* oid, UH offset)
{
    ER ercd;
    T_SNMP_MIB_NOD* node;
    INT i;
    INT flg_oid_tbl;
    UH flg_nod;
    INT flg_nod_tbl;
    UH flg_limit;

    /* Retrive a node for OID */

    if (dst_node == 0x00) {
        return E_NOID;
    }
    *dst_node = 0x00;

    /* Root */
    node = &mgr.nod_buf[0];

    ercd = E_OBJ;
    flg_oid_tbl = 1;
    flg_nod_tbl = 0;
    for (i = 0; i < oid->len; i++) {
        if (flg_nod_tbl == 0 && oid->buf[i] == 0 && (i + 1) >= oid->len) {
            flg_oid_tbl = 0;    /* Last number is zero. OID is not table object */
        }
        ercd = E_OBJ;
        flg_limit = 0;
        while (node != 0x00) {
            if (oid->buf[i] == node->num) {
                ercd = E_OK;
                break;
            } else if (oid->buf[i] < node->num) {
                break;    /* Not found */
            }
            *dst_node = node;
            node = node->next;
            flg_limit++;
            if (flg_limit >= MAX_NOD_CNT) {
                break;
            }
        }
        if (ercd != E_OK) {
            if (offset != 0) {
                ercd = E_OK;    /* Find previous node */
            }
            break;
        }
        *dst_node = node;
        node = node->child;
        if (flg_nod_tbl == 0 && ((*dst_node)->typ & MIB_NOD_TBL) != 0x00) {
            flg_nod_tbl = 1;    /* Node data is table object */
        }
    }
    if (ercd == E_OK && *dst_node == 0x00) {
        ercd = E_OBJ;
    }
    if (oid_idx != 0x00) {
        *oid_idx = i;
    }

    if (ercd == E_OK && offset == 0) {
        flg_nod = (*dst_node)->typ & MIB_NOD_TBL;
        if (flg_oid_tbl == 0) {    /* OID is not table object */
            if (flg_nod != 0x00) {
                ercd = E_OBJ;
            }
        } else {
            if (flg_nod == 0x00) {
                ercd = E_OBJ;
            }
        }
    }

    if (ercd != E_OK) {
        ercd = (flg_nod_tbl == 0) ? E_NOID : E_NOINS;
    }

    return ercd;
}

static ER snmp_mib_get_nod_acs(T_SNMP_MIB_NOD* nod)
{
    ER ercd;
    T_SNMP_MIB_TCP_DAT* mib_tcp;
    T_SNMP_MIB* mib;

    /* Subroutine for retrive a next node */

    if (nod == 0x00 || nod->dat == 0x00) {
        return E_PAR;
    }

    ercd = E_OBJ;
    if (nod->typ == (MIB_NOD_VAL | MIB_NOD_STD)) {
        mib_tcp = (T_SNMP_MIB_TCP_DAT*)nod->dat;
        if ((mib_tcp->acs & SNMP_STS_NO) == 0x00) {
            ercd = E_OK;
        }
    } else {
        mib = (T_SNMP_MIB*)nod->dat;
        if ((mib->acs & SNMP_STS_NO) == 0x00) {
            ercd = E_OK;
        }
    }

    return ercd;
}

static ER snmp_mib_get_next_nod(T_SNMP_MIB_NOD** dst_nod, T_SNMP_BER_OID* oid, INT oid_idx)
{
    ER ercd;
    ER ercd_acs;
    T_SNMP_MIB_NOD* node;
    INT flg_child;
    UH flg_limit;

    /* Retrive a next node */

    ercd = E_OK;
    node = (T_SNMP_MIB_NOD*)*dst_nod;

    if (oid_idx == 0) {
        return E_QOVR;  /* EndOfMibView error */
    }
    if (oid_idx < oid->len && oid->buf[oid_idx] > node->num) {
        node = node->parent;
        flg_child = 0;
    } else {
        flg_child = 1;
    }

    flg_limit = 0;
    while (node != 0x00) {
        if (flg_child == 1 && node->child != 0x00) {
            node = node->child;
            if (node->dat != 0x00) {
                ercd_acs = snmp_mib_get_nod_acs(node);
                if (ercd_acs == E_OK) {
                    break;
                }
            }
        } else if (node->next != 0x00) {
            node = node->next;
            if (node->dat != 0x00) {
                ercd_acs = snmp_mib_get_nod_acs(node);
                if (ercd_acs == E_OK) {
                    break;
                }
            }
            flg_child = 1;
        } else {
            node = node->parent;
            flg_child = 0;
        }
        flg_limit++;
        if (flg_limit >= MAX_NOD_CNT) {
            ercd = E_OBJ;   /* General error */
            node = 0;
        }
    }
    *dst_nod = node;

    if (node == 0x00) {
        if (ercd != E_OK) {
            return ercd;
        }
        return E_QOVR;  /* EndOfMibView error */
    }

    return E_OK;
}

static ER snmp_mib_cnv_ber(T_SNMP_BER_BUF* buf, UB typ, VP_UW datp, UH dat_len)
{
    ER ercd;
    UH len;

    /* Convert to BER data */

    switch (typ) {
        case TYP_INT:
            ercd = ber_set_int((T_SNMP_BER_INT*)buf, (INT)(ADDR)datp);
            break;
        case TYP_OCT_STR:
            if (dat_len == 0) {
                len = snmp_strlen((const char*)datp);
            } else {
                len = dat_len;
            }
            ercd = ber_set_oct((T_SNMP_BER_OCT*)buf, datp, len);
            break;
        case TYP_OBJ_ID:
            len = snmp_strlen((const char*)datp);
            ercd = ber_set_oid((T_SNMP_BER_OID*)buf, datp, len);
            break;
        case TYP_CNT:
            ercd = ber_set_cnt((T_SNMP_BER_CNT*)buf, (UW)(ADDR)datp);
            break;
        case TYP_CNT64:
            ercd = ber_set_cnt64((T_SNMP_BER_CNT64*)buf, *(UD_SNMP*)datp);
            break;
        case TYP_GAUGE:
            ercd = ber_set_gau((T_SNMP_BER_GAU*)buf, (UW)(ADDR)datp);
            break;
        case TYP_TIM_TIC:
            ercd = ber_set_tim((T_SNMP_BER_TIM*)buf, (UW)(ADDR)datp);
            break;
        case TYP_IP_ADR:
            ercd = ber_set_ip((T_SNMP_BER_IP*)buf, (UW)(ADDR)datp);
            break;
        default:
            ercd = E_OBJ;
    }

    return ercd;
}

ER snmp_mib_ini(INT* id)
{
    /* Initialization */

    /* Manager */
    mgr.nod_buf = 0x00;
    mgr.tcp_dat = (T_SNMP_MIB_TCP_DAT*)&snmp_cfg_tcp_buf[0];

    mgr.dec_oid = (T_SNMP_BER_OID*)&snmp_cfg_buf[*id];
    mgr.dec_oid->typ = TYP_OBJ_ID;
    mgr.dec_oid->buf_len = sizeof(UW) * CFG_MAX_OID_DEP;
    *id += BUF_UW_LEN(sizeof(T_SNMP_BER_OID));
    mgr.dec_oid->buf = (UW*)&snmp_cfg_buf[*id];
    *id += BUF_UW_LEN(sizeof(UW) * (CFG_MAX_OID_DEP));
    mgr.dec_oid_str = (VB*)&snmp_cfg_buf[*id];
    *id += BUF_UW_LEN(6 * (CFG_MAX_OID_DEP));
    
    mgr.nod_top_id = 0;

    /* Status */
    mgr.sts = STS_ENA | STS_INI;

    return E_OK;
}

ER snmp_mib_ext(void)
{
    /* Termination */

    /* Status */
    mgr.sts = STS_INV;

    return E_OK;
}

ER snmp_mib_cre(VP buf, VP oid_str_buf, UH oid_str_len, UH* nod_cnt)
{
    ER ercd;
    const VB* str;
    UH len;
    UW num;
    T_SNMP_MIB_TBL mib_tbl;
    UH typ;
    UH cnt;
    UH i;

    /* Create MIB tree */

    if (buf == 0) {
        return E_PAR;
    }
    if (oid_str_buf == 0) {
        return E_PAR;
    }

    mgr.nod_buf = (T_SNMP_MIB_NOD*)buf;

    /* Root */
    if (CFG_MIB_NOD_CNT < 1) {
        return E_NOMEM;
    }
    str = &snmp_mib[0].str[0];
    len = (UH)snmp_strlen(str);
    num = ber_atoi(&str, len);
    mgr.nod_buf[0].parent = 0;
    mgr.nod_buf[0].child = 0;
    mgr.nod_buf[0].next = 0;
    mgr.nod_buf[0].prev = 0;
    mgr.nod_buf[0].dat = 0;
    mgr.nod_buf[0].num = num;
    mgr.nod_buf[0].typ = MIB_NOD_STD;

    /* MIB-II */
    mib_tbl.pre = snmp_mib_mib2_pre;
    mib_tbl.mib = snmp_mib;
    mgr.nod_top_id = 1;
    cnt = 0U;
    ercd = snmp_mib_cre_tree(&mib_tbl, MIB_NOD_STD, oid_str_buf, oid_str_len, &cnt);
    if (ercd != E_OK) {
        return ercd;
    }
    mgr.nod_top_id += cnt;

    /* Vendor */
    for (i = 0; i < 0xff; i++) {
        if (snmp_mib_ven[i].mib == 0x00) {
            break;
        }
        typ = MIB_NOD_VEN | (i & MIB_NOD_ID);
        ercd = snmp_mib_cre_tree(&snmp_mib_ven[i], typ, oid_str_buf, oid_str_len, &cnt);
        if (ercd != E_OK) {
            return ercd;
        }
        mgr.nod_top_id += cnt;
    }

    /* Clear MIB data */
    mgr.nod_buf_cnt = 0U;
    for (i = mgr.nod_top_id; i < CFG_MIB_NOD_CNT; i++) {
        mgr.nod_buf[i].typ = MIB_NOD_VAL;
        snmp_mib_nod_buf_rel(&mgr.nod_buf[i]);
    }

    if (nod_cnt != 0x00) {
        *nod_cnt = mgr.nod_top_id;
    }

    /* Status */
    mgr.sts = STS_ENA;

    return E_OK;
}

ER snmp_mib_has(T_SNMP_BER_OID* oid)
{
    ER ercd;
    T_SNMP_MIB_NOD* nod;

    /* Has node by OID */

    ercd = snmp_mib_get_nod(&nod, 0, oid, 0);

    return ercd;
}

ER snmp_mib_add(VB* oid_str, UH oid_len, UH typ, VP* dat_ptr, UH* mib_nod_cnt)
{
    ER ercd;
    UH nod_cnt;
    T_SNMP_MIB_NOD* node;

    /* Appends a node by OID string */

    if (oid_str == 0) {
        return E_PAR;
    }

    typ |= MIB_NOD_VAL;

    ercd = snmp_mib_cre_nod_cnt((const VB*)oid_str, oid_len, typ, &nod_cnt);
    if (ercd == E_OK) {
        if (mgr.nod_buf_cnt >= nod_cnt) {
            ercd = snmp_mib_cre_nod((const VB*)oid_str, oid_len, typ, &node, mib_nod_cnt);
            if (ercd == E_OK && dat_ptr != 0x00) {
                *dat_ptr = &node->dat;
            }
        } else {
            ercd = E_NOMEM;
        }
    }

    return ercd;
}

ER snmp_mib_del(VB* oid_str, UH oid_len, VP* dat_obj)
{
    ER ercd;
    VP dat;

    /* Delete a node by OID string */

    ercd = snmp_mib_del_nod(oid_str, oid_len, &dat);
    if (ercd == E_OK) {
        if (dat_obj != 0x00) {
            *dat_obj = dat;
        }
    } else {
        if (dat_obj != 0x00) {
            *dat_obj = 0x00;
        }
    }

    return ercd;
}

ER snmp_mib_get_ber_dat_oid(T_SNMP_BER_BUF* buf, T_SNMP_BER_OID* oid, UH offset)
{
    ER ercd;
    T_SNMP_MIB_NOD* node;
    INT oid_idx;
    UH nod_typ;
    VP datp;
    const T_SNMP_MIB* mib;
    UW id;
    UB mib_typ;
    UH len;

    if (buf == 0x00 || oid == 0x00) {
        return E_PAR;
    }

    /* Retrieved BER data on OID */

    len = 0;

    /* Node */
    ercd = snmp_mib_get_nod(&node, &oid_idx, oid, offset);
    if (ercd != E_OK) {
        return ercd;
    }
    if (offset == 1) {
        ercd = snmp_mib_get_next_nod(&node, oid, oid_idx);
        if (ercd == E_OK) {
            /* OID from node */
            ercd = snmp_mib_get_oid_nod(oid, node);
        }
        if (ercd != E_OK) {
            return ercd;
        }
    }

    /* Data */
    nod_typ = node->typ & MIB_NOD_TYP;
    if (nod_typ == (MIB_NOD_VAL | MIB_NOD_STD)) {
        /* Variable standard MIB */
        id = (UW)(((ADDR)node->dat - (ADDR)&mgr.tcp_dat[0]) / sizeof(T_SNMP_MIB_TCP_DAT));
        ercd = snmp_get_mib_datp(&datp, (UH)id, node->typ, CB_STS_DIS, 0x00);
        if (ercd == E_OK) {
            /* Object type */
            if ((mgr.tcp_dat[id].acs & SNMP_STS_NO) != 0x00) {
                return E_NOINS;
            }
            mib_typ = mgr.tcp_dat[id].typ;
        }
    } else {
        if (nod_typ == MIB_NOD_STD) {
            /* Standard MIB */
            mib = snmp_mib;
        } else if (nod_typ == MIB_NOD_VEN) {
            /* Vendor MIB */
            id = node->typ & MIB_NOD_ID;
            mib = snmp_mib_ven[id].mib;
        } else if (nod_typ == (MIB_NOD_VAL | MIB_NOD_VEN)) {
            /* Variable vendor MIB */
            id = node->typ & MIB_NOD_ID;
            mib = snmp_mib_ven_val[id].mib;
        } else {
            return E_OBJ;
        }
        
        id = (UW)(((ADDR)node->dat - (ADDR)&mib[0]) / sizeof(T_SNMP_MIB));
        ercd = snmp_get_mib_datp(&datp, (UH)id, node->typ, CB_STS_ENA, &len);
        if (ercd == E_OK) {
            /* Object type */
            mib = (T_SNMP_MIB*)node->dat;
            if ((mib->acs & SNMP_STS_NO) != 0x00) {
                if ((node->typ & MIB_NOD_TBL) == 0x00) {
                    return E_NOID;
                } else {
                    return E_NOINS;
                }
            }
            mib_typ = mib->typ;
        }
    }
    if (ercd != E_OK) {
        return E_OBJ;
    }

    /* Convert to BER data */
    ercd = snmp_mib_cnv_ber(buf, mib_typ, datp, len);

    return ercd;
}

ER snmp_mib_get_ber_dat_id(T_SNMP_BER_BUF* buf, UH id, UH nod_typ)
{
    ER ercd;
    VP datp;
    UH nod_id;
    UB mib_typ;
    UH len;

    if (buf == 0x00) {
        return E_PAR;
    }

    /* Retrieved BER data on macro ID */

    len = 0;

    /* Data */
    ercd = snmp_get_mib_datp(&datp, id, nod_typ, CB_STS_DIS, &len);
    if (ercd != E_OK) {
        return ercd;
    }

    /* Convert to BER data */
    nod_id = nod_typ & MIB_NOD_ID;
    nod_typ &= MIB_NOD_TYP;
    if (nod_typ == MIB_NOD_STD) {
        mib_typ = snmp_mib[id].typ;
    } else if (nod_typ == MIB_NOD_VEN) {
        mib_typ = snmp_mib_ven[nod_id].mib[id].typ;
    } else {
        return E_OBJ;
    }
    ercd = snmp_mib_cnv_ber(buf, mib_typ, datp, len);

    return ercd;
}

ER snmp_mib_set_ber_dat(T_SNMP_BER* ber, T_SNMP_BER_OID* oid, T_SNMP_MIB_NOD** node, VP buf_old)
{
    ER ercd;
    UH nod_typ;
    const T_SNMP_MIB* mib;
    UH id;
    T_SNMP_BER_ANY* any;
    T_SNMP_BER_BUF ber_buf;
    VP datp;
    ADDR dat;
    UH len;
    T_SNMP_BER_OCT* ber_oct;
    T_SNMP_BER_OID* ber_oid;

    /* Updating MIB data for BER */

    if (ber == 0x00 || node == 0x00) {
        return E_ENC;
    }
    if (oid == 0x00) {
        return E_NOID;
    }

    if (buf_old == 0x00) {    /* 1st phase */
        ercd = snmp_mib_get_nod(node, 0x00, oid, 0);
        if (ercd != E_OK) {
            return ercd;
        }
    }

    /* ID */
    nod_typ = (*node)->typ & MIB_NOD_TYP;
    if (nod_typ == MIB_NOD_STD) {
        mib = snmp_mib;
    } else if (nod_typ == MIB_NOD_VEN) {
        id = (*node)->typ & MIB_NOD_ID;
        mib = snmp_mib_ven[id].mib;
    } else if (nod_typ == (MIB_NOD_VAL | MIB_NOD_VEN)) {
        id = (*node)->typ & MIB_NOD_ID;
        mib = snmp_mib_ven_val[id].mib;
    } else {
        return E_OBJ;
    }
    id = (UH)((ADDR)(*node)->dat - (ADDR)mib);
    id /= sizeof(T_SNMP_MIB);

    /* Accsess */
    if ((mib[id].acs & SNMP_STS_WO) == 0x00) {
        return E_RO;
    }
    /* Type */
    if (mib[id].typ != ber->typ) {
        return E_TYP;
    }
    if (!(mib[id].typ == TYP_INT || 
          mib[id].typ == TYP_CNT ||
          mib[id].typ == TYP_CNT64 ||
          mib[id].typ == TYP_GAUGE ||
          mib[id].typ == TYP_TIM_TIC ||
          mib[id].typ == TYP_IP_ADR ||
          mib[id].typ == TYP_OBJ_ID ||
          mib[id].typ == TYP_OCT_STR)) {
        return E_TYP;
    }

    /* Decode */
    any = (T_SNMP_BER_ANY*)ber;
    switch(any->typ) {
        case TYP_INT:
            ercd = snmp_ber_dec_int((T_SNMP_BER_INT*)&ber_buf, ber);
            break;
        case TYP_CNT:
            ercd = snmp_ber_dec_cnt((T_SNMP_BER_CNT*)&ber_buf, ber);
            break;
        case TYP_CNT64:
            ercd = snmp_ber_dec_cnt64((T_SNMP_BER_CNT64*)&ber_buf, ber);
            break;
        case TYP_GAUGE:
            ercd = snmp_ber_dec_gau((T_SNMP_BER_GAU*)&ber_buf, ber);
            break;
        case TYP_TIM_TIC:
            ercd = snmp_ber_dec_tim((T_SNMP_BER_TIM*)&ber_buf, ber);
            break;
        case TYP_IP_ADR:
            ercd = snmp_ber_dec_ip((T_SNMP_BER_IP*)&ber_buf, ber);
            break;
        case TYP_OCT_STR:
            ber_oct = (T_SNMP_BER_OCT*)&ber_buf;
            ercd = snmp_ber_dec_oct(ber_oct, ber);
            if (ercd == E_OK) {
                if (ber_oct->len + 1 > mib[id].len) {
                    ercd = E_DAT;    /* Buffer not enough */
                }
            }
            break;
        case TYP_OBJ_ID:
            ber_oid = mgr.dec_oid;
            ercd = snmp_ber_dec_oid(ber_oid, ber);
            if (ercd == E_OK) {
                if (ber_oid->len > CFG_MAX_OID_DEP) {
                    ercd = E_DAT;    /* Buffer not enough */
                }
            }
            break;
        default:
            ercd = E_OBJ;
            break;
    }
    if (ercd != E_OK) {
        if (ercd < E_BASE) {
            return ercd;    /* Internal error */
        }
        return E_ENC;
    }

    if (buf_old == 0x00) {
        return E_OK;    /* 1st phase (Not updated) */
    }

    /* Update */
    len = 4;
    if (ercd == E_OK) {
        /* Old data */
        ercd = snmp_get_mib_datp(&datp, id, (*node)->typ, CB_STS_DIS, 0x00);
        if (ercd != E_OK) {
            return ercd;
        }
        if (any->typ == TYP_OCT_STR || any->typ == TYP_OBJ_ID) {
            snmp_strcpy((char*)buf_old, (const char*)datp);
        } else if (any->typ == TYP_CNT64) {
            *((UD_SNMP*)buf_old) = *((UD_SNMP*)datp);
        } else {
            *((UW*)buf_old) = (UW)(ADDR)datp;
        }
        
        switch(any->typ) {
            case TYP_INT:
                dat = (UW)((T_SNMP_BER_INT*)&ber_buf)->dat;
                break;
            case TYP_CNT:
                dat = ((T_SNMP_BER_CNT*)&ber_buf)->dat;
                break;
            case TYP_CNT64:
                len = 8;
                dat = (ADDR)&((T_SNMP_BER_CNT64*)&ber_buf)->dat;
                break;
            case TYP_GAUGE:
                dat = ((T_SNMP_BER_GAU*)&ber_buf)->dat;
                break;
            case TYP_TIM_TIC:
                dat = ((T_SNMP_BER_TIM*)&ber_buf)->dat;
                break;
            case TYP_IP_ADR:
                dat = ((T_SNMP_BER_IP*)&ber_buf)->dat;
                break;
            case TYP_OCT_STR:
                ber_oct = (T_SNMP_BER_OCT*)&ber_buf;
                dat = (ADDR)ber_oct->buf;
                len = ber_oct->len;
                break;
            case TYP_OBJ_ID:
                ber_oid = mgr.dec_oid;
                ercd = snmp_ber_str_oid(ber_oid, mgr.dec_oid_str, CFG_MAX_OID_DEP*6);
                if (ercd == E_OK) {
                    dat = (ADDR)mgr.dec_oid_str;
                    len = snmp_strlen(mgr.dec_oid_str);
                }
                break;
            default:
                ercd = E_OBJ;
                break;
        }
        if (ercd != E_OK) {
            return ercd;
        }
        ercd = snmp_set_mib_dat((VP_UW)dat, len, id, (*node)->typ, CB_STS_ENA);
        if (ercd != E_OK) {
            return ercd;
        }
    }

    return ercd;
}

ER snmp_mib_set_dat_oid(VP buf, T_SNMP_BER_OID* oid)
{
    ER ercd;
    T_SNMP_MIB_NOD* node;
    UH nod_typ;
    const T_SNMP_MIB* mib;
    UW id;
    UH len;
    UW* dat_buf;

    /* Updating MIB data for OID */

    if (oid == 0x00) {
        return E_NOID;
    }

    /* Node */
    ercd = snmp_mib_get_nod(&node, 0x00, oid, 0);
    if (ercd != E_OK) {
        return ercd;
    }

    /* ID */
    nod_typ = node->typ & MIB_NOD_TYP;
    if (nod_typ == MIB_NOD_STD) {
        mib = snmp_mib;
    } else if (nod_typ == MIB_NOD_VEN) {
        id = node->typ & MIB_NOD_ID;
        mib = snmp_mib_ven[id].mib;
    } else if (nod_typ == (MIB_NOD_VAL | MIB_NOD_VEN)) {
        id = node->typ & MIB_NOD_ID;
        mib = snmp_mib_ven_val[id].mib;
    } else {
        return E_OBJ;
    }
    id = (UW)((ADDR)node->dat - (ADDR)mib);
    id /= sizeof(T_SNMP_MIB);

    /* Data length */
    len = 4;
    if (mib[id].typ == TYP_OCT_STR || mib[id].typ == TYP_OBJ_ID) {
        len = snmp_strlen((const char*)buf);
    } else if (mib[id].typ == TYP_CNT64) {
        /* buf is 64bit data pointer. */
        len = 8;
    } else {
        dat_buf = (UW*)buf;
        buf = (VP_UW)dat_buf[0];
    }

    /* Update */
    ercd = snmp_set_mib_dat(buf, len, (UH)id, node->typ, CB_STS_DIS);
    if (ercd != E_OK) {
        return ercd;
    }

    return E_OK;
}

ER snmp_mib_get_oid_id(T_SNMP_BER_OID* ber, UH id, UH nod_typ, VP str_buf)
{
    ER ercd;
    UH nod_id;
    const T_SNMP_MIB* mib;
    UH len;

    /* Get OID BER data for macro ID */

    if (str_buf == 0x00) {
        return E_OBJ;
    }

    nod_id = nod_typ & MIB_NOD_ID;
    nod_typ &= MIB_NOD_TYP;
    if (nod_typ == MIB_NOD_STD) {
        mib = snmp_mib;
        snmp_strcpy(str_buf, snmp_mib_mib2_pre);
    } else if (nod_typ == MIB_NOD_VEN) {
        mib = snmp_mib_ven[nod_id].mib;
        snmp_strcpy(str_buf, snmp_mib_ven[nod_id].pre);
    } else {
        return E_OBJ;
    }

    snmp_strcat(str_buf, mib[id].str);
    len = snmp_strlen(str_buf);
    ercd = ber_set_oid(ber, str_buf, len);

    return ercd;
}

