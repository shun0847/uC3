/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK
    Status
    Copyright (c) 2014-2023, eForce Co., Ltd. All rights reserved.

    Version Information
      2014.03.06: Created
      2014.05.15: Device number for UDP was fixed
      2014.06.18  SNMP compact was supported
      2014.07.10: Suppressed warning of the GCC compiler.
      2014.08.20: Add net_sts_ext function.
      2016.04.20: Check ref function to set in updating
      2017.01.17: Clarified the type for constant
      2017.01.18: Support 64bit processor
      2023.05.25: Fixed interface MIB is invalid in 64bit environment.
 ***************************************************************************/

#include <stdio.h>
#include "kernel.h"
#include "net_hdr.h"
#include "net_def.h"
#include "net_sts.h"
#include "net_sts_id.h"

#ifdef STS_SUP

extern T_NET_STS_CFG gNET_STS_CFG;    /* Configuration */

/* Configuration */
#define NET_STS_SEM_TMO    2000    /* Timeout for semaphore */

/* Undefine macros */
#undef PATH_MTU
#undef IP4_TTL
#undef IP4_IPR_TMO
#undef TCP_RTO_MIN
#undef TCP_RTO_MAX

/* Macros */
#define CMP_MAC(x, y)    (x[0] != y[0] || x[1] != y[1] || x[2] != y[2] || \
                          x[3] != y[3] || x[4] != y[4] || x[5] != y[5])

/* Status */
#define STS_CLR       0x0000U    /* Clear */
#define STS_ENA       0x0001U    /* Enabled */
#define STS_CHG_IP    0x0010U    /* IP address change */

/* Module status */
#define STS_MOD_INI   0x0001U    /* Initialize */
#define STS_SNMP_INI  0x0010U    /* Initialize SNMP */

/* Variables */
#ifdef NET_S_OS
ID ID_NET_SEM_STS = 0;         /* Semaphore */
#else
extern ID ID_NET_SEM_STS;
#endif
static T_NET_STS_DEV* gNET_STS_DEV;     /* Device status */
static T_NET_STS_ARP* gNET_STS_ARP;     /* ARP status */
static T_NET_STS_SOC* gNET_STS_SOC;     /* Socket status */
static VP* gNET_STS_PTR;                /* Status pointer */
static VP* gNET_STS_PTR_TMP;            /* Status pointer (Temporary) */
static T_NET_STS gNET_STS;              /* Status */
static T_NET_STS gNET_STS_TMP;          /* Status (Temporary) */
static UH gNET_STS_FLG[4];              /* Status flag pointer */
static UH gNET_STS_MOD_STS = 0x00;      /* Status of this module */

static ER net_sts_loc(void)
{
    /* Wait semaphore */

    return twai_sem(ID_NET_SEM_STS, NET_STS_SEM_TMO);
}

static void net_sts_ulc(void)
{
    /* Release semaphore */

    sig_sem(ID_NET_SEM_STS);

    return;
}

static ER net_sts_set(UH grp_id, UH id, UW dat)
{
    ER ercd;
    UW* buf;

    /* Write status data */

    if (dat == 0) {
        return E_PAR;
    }

    buf = (UW*)gNET_STS_PTR[grp_id];

    ercd = net_sts_loc();
    if (ercd != E_OK) {
        return ercd;
    }

    buf[id] = dat;

    if (grp_id < NET_STS_IFS) {
        gNET_STS_FLG[grp_id] = STS_ENA;
    }

    net_sts_ulc();

    return E_OK;
}

void net_sts_eth_cbk(UH dev_num, UH evt, VP sts)
{
    UW spd;

    /* Callback from network driver task */

    if ((gNET_STS_MOD_STS & STS_MOD_INI) == 0x00) {
        return;
    }
    if (dev_num == 0 || dev_num > NET_DEV_MAX) {
        return;
    }
    dev_num = dev_num - 1;

    /* Link status */
    if (evt == EV_CBK_DEV_LINK) {
        switch ((ADDR)sts) {
            case PHY_STS_LINK_DOWN:  /* Link down */
                spd = 0;
                break;
            case PHY_STS_10HD:       /* 10M Half-Duplex */
            case PHY_STS_10FD:       /* 10M Full-Duplex */
                spd = 10000000;
                break;
            case PHY_STS_100HD:      /* 100M Half-Duplex */
            case PHY_STS_100FD:      /* 100M Full-Duplex */
                spd = 100000000;
                break;
            case PHY_STS_1000FD:     /* 1000M Full-Duplex */
                spd = 1000000000;
                break;
            default:
                return;
        }
        net_sts_set(NET_STS_IFS + dev_num, NET_STS_IF_SPEED, spd);
        spd = (spd == 0) ? NET_STS_LINK_DOWN : NET_STS_LINK_UP;
        net_sts_set(NET_STS_IFS + dev_num, NET_STS_IF_OPER_STS, spd);
        
        /* SNMP callback */
        if (gNET_STS_CFG.cbk_snmp != NULL) {
            gNET_STS_CFG.cbk_snmp(dev_num + 1, evt, sts);
        }
    }

    return;
}

void net_sts_cbk(UH dev_num, UH evt, VP sts)
{
    UH* sts_cbk;

    /* Status callback */

    sts_cbk = (UH*)sts;

    /* Device IP address was changed */
    if (evt == EV_CBK_DEV_CHG_IP && sts_cbk == NET_STS_CBK_DIS) {
        gNET_STS_CFG.dev[dev_num - 1].sts |= STS_CHG_IP;
        return;
    }

    /* SNMP callback */
    if (gNET_STS_CFG.cbk_snmp != 0) {
        gNET_STS_CFG.cbk_snmp(dev_num, evt, sts);
    }

    return;
}

ER net_sts_ini(void)
{
#ifdef NET_S_OS
    ER ercd;
#endif
    INT i;
    T_NET_STS_IFS* ifs;

    /* Initialization */

    /* Create semaphore */
    #ifdef NET_S_OS
    ercd = acre_sem((T_CSEM *)&c_net_sem);
    if (ercd <= 0) {
        return E_OBJ;
    }
    ID_NET_SEM_STS = ercd;
    #endif

    /* ARP status */
    gNET_STS_ARP = gNET_STS_CFG.arp;
    net_memset(gNET_STS_ARP, 0, sizeof(T_NET_STS_ARP) * NET_ARP_MAX);

    /* Socket status */
    gNET_STS_SOC = gNET_STS_CFG.soc;
    net_memset(gNET_STS_SOC, 0, sizeof(T_NET_STS_SOC) * NET_SOC_MAX);

    /* Status flag */
    net_memset(&gNET_STS_FLG, 0, sizeof(gNET_STS_FLG));

    /* Status */
    net_memset(&gNET_STS, 0, sizeof(T_NET_STS));
    gNET_STS.ifs = gNET_STS_CFG.ifs;
    gNET_STS_PTR = gNET_STS_CFG.sts_ptr;
    gNET_STS_PTR[NET_STS_IP] = &gNET_STS.ip;
    gNET_STS_PTR[NET_STS_ICMP] = &gNET_STS.icmp;
    gNET_STS_PTR[NET_STS_TCP] = &gNET_STS.tcp;
    gNET_STS_PTR[NET_STS_UDP] = &gNET_STS.udp;

    /* Status temporary */
    net_memset(&gNET_STS_TMP, 0, sizeof(T_NET_STS));
    gNET_STS_TMP.ifs = gNET_STS_CFG.ifs_tmp;
    gNET_STS_PTR_TMP = gNET_STS_CFG.sts_ptr_tmp;
    gNET_STS_PTR_TMP[NET_STS_IP] = &gNET_STS_TMP.ip;
    gNET_STS_PTR_TMP[NET_STS_ICMP] = &gNET_STS_TMP.icmp;
    gNET_STS_PTR_TMP[NET_STS_TCP] = &gNET_STS_TMP.tcp;
    gNET_STS_PTR_TMP[NET_STS_UDP] = &gNET_STS_TMP.udp;

    /* Device and interface */
    gNET_STS_DEV = gNET_STS_CFG.dev;
    for (i = 0; i < NET_DEV_MAX; i++) {
        /* Device status */
        gNET_STS_DEV[i].sts = 0x00;
        
        /* Interface */
        gNET_STS_PTR[NET_STS_IFS + i] = &gNET_STS_CFG.ifs[i];
        /* Initial data */
        ifs = &gNET_STS_CFG.ifs[i];
        net_memset(ifs, 0, sizeof(T_NET_STS_IFS));
        ifs->index = gNET_DEV[i].num;               /* Index [1..4] */
        ifs->descr   = (UW)((ADDR)gNET_DEV[i].name & 0xFFFFFFFF);   /* lowwer - Descripter */
#if (_kernel_SIZE_SIZE==8)
        ifs->descr_h = (UW)((ADDR)gNET_DEV[i].name >> 32);          /* upper  - Descripter */
#endif        
        if (gNET_DEV[i].type == NET_DEV_TYPE_ETH) {
            ifs->type = NET_STS_TYPE_ETH;           /* Type (Ethernet CSMACD(6)) */
        } else if (gNET_DEV[i].type == NET_DEV_TYPE_PPP || gNET_DEV[i].type == NET_DEV_TYPE_PPPOE) {
            ifs->type = NET_STS_TYPE_PPP;           /* Type (PPP(23)) */
        } else if (gNET_DEV[i].type ==  NET_DEV_TYPE_LOOP) {
            ifs->type = NET_STS_TYPE_SOFT_LOOP;     /* Type (Software loop back(24)) */
        } else {
            ifs->type = NET_STS_TYPE_OTHER;         /* Type (Other(1)) */
        }
        ifs->mtu = gNET[i].cfg->PATH_MTU;            /* Maximum transmission unit(MTU) */
        ifs->phy_addr = (UW)((ADDR)gNET_DEV[i].cfg.eth.mac & 0xFFFFFFFF);   /* MAC address */
#if (_kernel_SIZE_SIZE==8)
        ifs->phy_addr_h = (UW)((ADDR)gNET_DEV[i].cfg.eth.mac >> 32);        /* upper  - MAC address */
#endif        
        ifs->admin_sts = NET_STS_LINK_UP;           /* User request for link of the interface */
        ifs->oper_sts = NET_STS_LINK_DOWN;          /* Current link status (Link down(2)) */
        
        /* Temporary */
        gNET_STS_PTR_TMP[NET_STS_IFS + i] = &gNET_STS_CFG.ifs_tmp[i];
        ifs = &gNET_STS_CFG.ifs_tmp[i];
        net_memset(ifs, 0, sizeof(T_NET_STS_IFS));
    }

    /* IP */
    gNET_STS_TMP.ip.forward = 2;                        /* Forwarding (Not forwarding(2))*/
    gNET_STS_TMP.ip.def_ttl = gNET[0].cfg->IP4_TTL;     /* Default TTL(Time-To-Live) */
    gNET_STS_TMP.ip.reasm_timeout = gNET[0].cfg->IP4_IPR_TMO / 1000;
                                                    /* Maximum seconds held reassemble fragments */
    gNET_STS_FLG[NET_STS_IP] = STS_ENA;

    /* TCP */
    gNET_STS_TMP.tcp.rto_algo = 5;                          /* RTO algorithm (RFC2988(5)) */
    gNET_STS_TMP.tcp.rto_min = gNET[0].cfg->TCP_RTO_MIN;    /* Minimum value of RTO */
    gNET_STS_TMP.tcp.rto_max = gNET[0].cfg->TCP_RTO_MAX;    /* Maximum value of RTO */
    gNET_STS_TMP.tcp.max_conn = NET_TCP_MAX;                /* Maximum number of connections */
    gNET_STS_FLG[NET_STS_TCP] = STS_ENA;

    /* Update status */
    net_sts_upd();

    gNET_STS_MOD_STS = STS_MOD_INI;

    return E_OK;
}

ER net_sts_ext(void)
{
    #ifdef NET_S_OS
    del_sem(ID_NET_SEM_STS);
    #endif
    
    return E_OK;
}

void net_sts_inc(UH grp_id, UH sts_id)
{
    UW* dat;

    /* Increment of status data */

    if (grp_id >= NET_STS_IFS) {
        return;
    }

    dat = (UW*)gNET_STS_PTR_TMP[grp_id];
    dat[sts_id] = dat[sts_id] + 1;

    gNET_STS_FLG[grp_id] = STS_ENA;

    return;
}

void net_sts_dec(UH grp_id, UH sts_id)
{
    UW* dat;

    /* Decrement of status data */

    if (grp_id >= NET_STS_IFS) {
        return;
    }

    dat = (UW*)gNET_STS_PTR_TMP[grp_id];
    dat[sts_id] = dat[sts_id] - 1;

    gNET_STS_FLG[grp_id] = STS_ENA;

    return;
}

void net_sts_add(UH grp_id, UH sts_id, UW dat_add)
{
    UW* dat;

    /* Addition status data */

    if (grp_id >= NET_STS_IFS) {
        return;
    }

    dat = (UW*)gNET_STS_PTR_TMP[grp_id];
    dat[sts_id] = dat[sts_id] + dat_add;

    gNET_STS_FLG[grp_id] = STS_ENA;

    return;
}

ER net_sts_get(UH grp_id, UH sts_id, UW* dat)
{
    ER ercd;
    UW* buf;

    /* Retrieve status */

    if (dat == 0) {
        return E_PAR;
    }

    buf = (UW*)gNET_STS_PTR[grp_id];

    ercd = net_sts_loc();
    if (ercd != E_OK) {
        return ercd;
    }

    *dat = buf[sts_id];

    net_sts_ulc();

    return E_OK;
}

static ER net_sts_upd_arp(void)
{
    ER ercd;
    UH i;
    T_NET_ARP *arp;
    UH dev_num;

    /* Update ARP status */

    for (i = 0; i < NET_ARP_MAX; i++) {
        arp = &gNET_ARP[i];
        if (arp->chg_flg != NET_STS_CLR) {
            if (arp->net != NULL && arp->net->dev != NULL) {
                dev_num = arp->net->dev->num;
            } else {
                dev_num = 1;
            }
            ercd = E_OBJ;
            if (arp->sts != gNET_STS_ARP[i].sts) {
                /* Update status */
                if (arp->sts == ARP_VALID) {
                    net_sts_cbk(dev_num, EV_CBK_ARP_CRE, (VP)arp);
                } else if (gNET_STS_ARP[i].sts == ARP_VALID) {
                    net_sts_cbk(dev_num, EV_CBK_ARP_DEL, (VP)arp);
                }
                ercd = E_OK;
            } else if (arp->sts == ARP_VALID) {
                /* Update variables */
                if (arp->ipaddr != gNET_STS_ARP[i].ipaddr ||
                    dev_num != gNET_STS_ARP[i].dev_num) {
                    /* Changed IP address or device number */
                    net_sts_cbk(dev_num, EV_CBK_ARP_CHG_IP, (VP)arp);
                    ercd = E_OK;
                } else if (CMP_MAC(arp->mac, gNET_STS_ARP[i].mac)) {
                    /* Only changed MAC address */
                    net_sts_cbk(dev_num, EV_CBK_ARP_CHG_MAC, (VP)arp);
                    ercd = E_OK;
                }
            }
            if (ercd == E_OK) {
                gNET_STS_ARP[i].ipaddr = arp->ipaddr;
                gNET_STS_ARP[i].dev_num = dev_num;
                net_memcpy(gNET_STS_ARP[i].mac, arp->mac, 6);
                gNET_STS_ARP[i].sts = arp->sts;
            }
            arp->chg_flg = NET_STS_CLR;
        }
    }

    return E_OK;
}

static ER net_sts_upd_soc(UH sts)
{
    ER ercd;
    UH i;
    T_NET_SOC *soc;
    UH dev_num;

    /* Update socket status */

    gNET_STS.tcp.curr_estab = 0;

    for (i = 0; i < NET_SOC_MAX; i++) {
        soc = &pNET_SOC[i];
        if (soc->tcp != NULL && soc->tcp->bsd >= TCP_ESTABLISHED) {
            gNET_STS.tcp.curr_estab++;
        }
        if ((sts & STS_SNMP_INI) != 0x00) {
            if (soc->chg_flg != NET_STS_CLR || (soc->tcp != NULL && soc->tcp->chg_flg != NET_STS_CLR)) {
                if (soc->usr_net != NULL && soc->usr_net->dev != NULL) {
                    dev_num = soc->usr_net->dev->num;
                } else {
                    dev_num = 1;
                }
                ercd = E_OBJ;
                if (soc->state != gNET_STS_SOC[i].state) {
                    /* Update status */
                    if (gNET_STS_SOC[i].state == SOC_UNUSED) {
                        /* Create socket */
                        net_sts_cbk(dev_num, EV_CBK_SOC_CRE, (VP)soc);
                        ercd = E_OK;
                    } else if (soc->state == SOC_UNUSED) {
                        /* Delete socket */
                        net_sts_cbk(dev_num, EV_CBK_SOC_DEL, (VP)soc);
                        ercd = E_OK;
                    }
                }
                if (ercd != E_OK && soc->state != SOC_UNUSED) {
                    if (soc->proto != gNET_STS_SOC[i].proto
                        || dev_num != gNET_STS_SOC[i].dev_num
                        || soc->lport != gNET_STS_SOC[i].lport
                        || ((gNET_STS_SOC[i].proto == IP_PROTO_TCP) &&
                            (soc->raddr != gNET_STS_SOC[i].raddr
                            || soc->rport != gNET_STS_SOC[i].rport))) {
                        /* Status changed */
                        net_sts_cbk(dev_num, EV_CBK_SOC_CHG, (VP)soc);
                        ercd = E_OK;
                    }
                    if (ercd != E_OK && soc->tcp != NULL) {
                        if (soc->tcp->bsd != gNET_STS_SOC[i].bsd) {
                            /* Only TCP status changed */
                            net_sts_cbk(dev_num, EV_CBK_SOC_CHG_TCP, (VP)soc);
                            ercd = E_OK;
                        }
                    }
                }
                /* Update status */
                if (ercd == E_OK) {
                    gNET_STS_SOC[i].proto = soc->proto;
                    gNET_STS_SOC[i].dev_num = dev_num;
                    gNET_STS_SOC[i].lport = soc->lport;
                    gNET_STS_SOC[i].raddr = soc->raddr;
                    gNET_STS_SOC[i].rport = soc->rport;
                    gNET_STS_SOC[i].state = soc->state;
                    if (soc->tcp != NULL) {
                        gNET_STS_SOC[i].bsd = soc->tcp->bsd;
                    } else {
                        gNET_STS_SOC[i].bsd = 0x00;
                    }
                }
                if (soc->tcp != NULL) {
                    soc->tcp->chg_flg = NET_STS_CLR;
                }
                soc->chg_flg = NET_STS_CLR;
            }
        }
    }

    return E_OK;
}

ER net_sts_snmp_ena(void)
{
    UH i;
    T_NET_SOC *soc;
    T_NET_ARP *arp;

    /* SNMP enabled */

    loc_tcp();

    net_memset(gNET_STS_SOC, 0, sizeof(T_NET_STS_SOC) * NET_SOC_MAX);
    net_memset(gNET_STS_ARP, 0, sizeof(T_NET_STS_ARP) * NET_ARP_MAX);

    for (i = 0; i < NET_SOC_MAX; i++) {
        soc = &pNET_SOC[i];
        soc->chg_flg = NET_STS_CHG;
    }
    for (i = 0; i < NET_ARP_MAX; i++) {
        arp = &gNET_ARP[i];
        arp->chg_flg = NET_STS_CHG;
    }

    gNET_STS_MOD_STS |= STS_SNMP_INI;

    ulc_tcp();

    return E_OK;
}

ER net_sts_snmp_dis(void)
{
    /* SNMP disabled */

    loc_tcp();

    gNET_STS_MOD_STS &= ~STS_SNMP_INI;

    ulc_tcp();

    return E_OK;
}

ER net_sts_upd(void)
{
    ER ercd;
    UH i;

    /* Update status */

    ercd = net_sts_loc();
    if (ercd != E_OK) {
        return ercd;
    }

    /* Update status */
    if (gNET_STS_FLG[NET_STS_IP] != 0x00) {
        gNET_STS.ip = gNET_STS_TMP.ip;
        gNET_STS_FLG[NET_STS_IP] = STS_CLR;
    }
    if (gNET_STS_FLG[NET_STS_ICMP] != 0x00) {
        gNET_STS.icmp = gNET_STS_TMP.icmp;
        gNET_STS_FLG[NET_STS_ICMP] = STS_CLR;
    }
    if (gNET_STS_FLG[NET_STS_TCP] != 0x00) {
        gNET_STS.tcp = gNET_STS_TMP.tcp;
        gNET_STS_FLG[NET_STS_TCP] = STS_CLR;
    }
    if (gNET_STS_FLG[NET_STS_UDP] != 0x00) {
        gNET_STS.udp = gNET_STS_TMP.udp;
        gNET_STS_FLG[NET_STS_UDP] = STS_CLR;
    }
    for (i = 0; i < NET_DEV_MAX; i++) {
        if (gNET_DEV[i].ref) {
            gNET_DEV[i].ref(i + 1, REF_ETH_UPD_STS, &gNET_STS.ifs[i]);
        }
        if ((gNET_STS_MOD_STS & STS_SNMP_INI) != 0x00) {
            if ((gNET_STS_CFG.dev[i].sts & STS_CHG_IP) != 0x00) {
                net_sts_cbk(i + 1, EV_CBK_DEV_CHG_IP, (VP)NET_STS_CBK_ENA);
                gNET_STS_CFG.dev[i].sts &= ~STS_CHG_IP;
            }
        }
    }

    if ((gNET_STS_MOD_STS & STS_SNMP_INI) != 0x00) {
        /* Update socket status */
        net_sts_upd_soc(STS_SNMP_INI);
        /* Update ARP status */
        net_sts_upd_arp();
    } else {
        net_sts_upd_soc(0x00);
    }

    net_sts_ulc();

    return E_OK;
}

#endif      /* #ifdef STS_SUP */
