/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    Sample DHCP client Task header file
    Copyright (c)  2016, eForce Co., Ltd. All rights reserved.

    Version Information
      2016.02.20: Created.
 ***************************************************************************/

#include "sample_tsk_dhcpc.h"


/* DHCPc task event */
#define SCINFO_FLG_STOP     0x0001
#define SCINFO_FLG_RESET    0x0002      /* reserve */
#define SCINFO_FLG_DO       0x0004
#define SCINFO_FLG_ALL      (SCINFO_FLG_STOP | SCINFO_FLG_RESET | SCINFO_FLG_DO)

static T_DHCPC_TSK_INFO *gDHCPC_TSK_INFO[CFG_NET_DEV_MAX] = {0};


/* DHCPc task initialize */
static ER dhcpc_tsk_ini(T_DHCPC_TSK_INFO *dc_inf)
{
    ER ercd = E_OK;
    
    /* Check parameter */
    if (!(dc_inf->flgid && dc_inf->interval && dc_inf->set_cbk)) {
        ercd = E_PAR;
    }
    else if (dc_inf->dc.dev_num > CFG_NET_DEV_MAX) {
        ercd = E_PAR;
    }
    
    if (E_OK == ercd) {
        /* Get this task ID */
        ercd = get_tid(&dc_inf->tid);
        if (0 == dc_inf->dc.dev_num) {
            dc_inf->dc.dev_num = 1;
        }
        gDHCPC_TSK_INFO[dc_inf->dc.dev_num - 1] = dc_inf;
    }
    
    return ercd;
}

static UH check_lan_stat(T_DHCPC_TSK_INFO *dc_inf)
{
    ER ercd;
    UH lan_stat;
    
    ercd = net_dev_sts(dc_inf->dc.dev_num, REF_ETH_LINK_STS, (VP)&lan_stat);
    
    /* If REF_ETH_LINK_STS is unsupported driver, impossible to link monitoring */
    return (E_NOSPT == ercd) ? 1 : (lan_stat & PHY_STS_MSK);
}

static ER wait_evt(T_DHCPC_TSK_INFO *dc_inf, UW sec, FLGPTN *flg)
{
    ER ercd;
    UH retry, retry_max;
    
    retry_max = (TMO_POL == sec) ? 1 : 1000;
    for (retry = 0; retry < retry_max; ++retry) {
        ercd = twai_flg(dc_inf->flgid, SCINFO_FLG_ALL, TWF_ORW, flg, sec);
        if (E_TMOUT != ercd)    break;
        
        if (0 == check_lan_stat(dc_inf)) {
            ercd = E_CLS;
            break;
        }
    }
    
    return ercd;
}

static void cfg_netaddr(T_DHCP_CLIENT *dhcp)
{
    T_NET_ADR adr;

    adr.ipaddr  = dhcp->ipaddr;
    adr.mask    = dhcp->subnet;
    adr.gateway = dhcp->gateway;

    net_cfg(dhcp->dev_num, NET_IP4_CFG, (VP)&adr);
}

/* sample DHCP client task */
ER dhcpc_tsk(T_DHCPC_TSK_INFO *dc_inf)
{
    ER ercd;
    UW sec, sec1, sec2;
    FLGPTN flg;
    ER (*dhcp_func)(T_DHCP_CLIENT*);
    UB cnt;
    
   
    ercd = dhcpc_tsk_ini(dc_inf);
    if (E_OK != ercd) {
        return ercd;
    }
    
    if (0 == dc_inf->dc.ipaddr) {
        /* Asynchronous of DHCP start (between 1 and 10sec) */
        sec = net_rand();
        sec = (3 & sec) + (7 & (sec >> 2));
        dly_tsk(sec * 1000);
    }
    
    for (;;) {
        /* Checking Link state */
        if (0 == check_lan_stat(dc_inf)) {
            ercd = E_CLS;               /* Linkdown */
            dc_inf->set_cbk(ercd, &dc_inf->dc);
            do {
                ercd = dly_tsk(100);
                if (E_RLWAI == ercd)    break;
            } while (0 == check_lan_stat(dc_inf));  /* until Link-Up */
            dly_tsk(250);   /* wait link use ready */
        }
        
        /* DHCP start(INIT) or DHCP restart(INIT-REBOOT) */
        dhcp_func = (0 == dc_inf->dc.ipaddr) ? dhcp_bind : dhcp_reboot;
        for (cnt = 0; ; ) {
            sec1 = get_net_sec();       /* start time */
            
            ercd = dhcp_func(&dc_inf->dc);
            dc_inf->set_cbk(ercd, &dc_inf->dc);
            if (E_OK == ercd)   break;
            
            dhcp_func = dhcp_bind;
            sec2 = get_net_sec();       /* end time */
            /* Calculate the send waiting time */
            if (cnt < 5) {
                sec = (4 << cnt);
                cnt++;
            }
            else {
                sec = dc_inf->interval;
                cnt = 0;
            }
            sec = (sec > (sec2 - sec1)) ? (sec - (sec2 - sec1)) : 0;
            
            ercd = wait_evt(dc_inf, sec, &flg);
            if (E_CLS == ercd)              break;      /* Linkdown */
            /* Check notify events */
            if (E_OK == ercd) {
                clr_flg(dc_inf->flgid, 0);
                if (flg & SCINFO_FLG_STOP)  goto _exit_dhcp_client_tsk;
                if (flg & SCINFO_FLG_RESET) continue;
                if (flg & SCINFO_FLG_DO)    ;
            }
        }
        if (E_CLS == ercd)  continue;   /* checking link status */
        
        /* Unlimited lease time */
        if (dc_inf->dc.lease == 0xFFFFFFFF) {
            do {
                ercd = dly_tsk(100);
                if (E_RLWAI == ercd)    break;
            } while (0 < check_lan_stat(dc_inf));   /* until Link-Down */
            continue;
        }
        
        /* RENEWING or REBINDING */
        dhcp_func = dhcp_renew;
        do {
          sec = 0;
          if (dhcp_func == dhcp_renew) {                /* RENEWING */
            sec1 = get_net_sec();       /* T1 start time */
            sec = dc_inf->dc.t1;
          }
          else if (dhcp_func == dhcp_rebind) {          /* REBINDING */
            sec2 = get_net_sec();       /* T1 end time */
            if (dc_inf->dc.t2 > (sec2 - sec1)) {
              sec = dc_inf->dc.t2 - (sec2 - sec1);
            }
          }
          else {                                /* until Lease time */
            sec2 = get_net_sec();       /* T2 end time */
            if (dc_inf->dc.lease > (sec2 - sec1)) {
              sec = dc_inf->dc.lease - (sec2 - sec1);
            }
          }
          ercd = wait_evt(dc_inf, sec, &flg);
          if (E_CLS == ercd)              break;      /* Linkdown */
          /* Check notify events */
          if (E_OK == ercd) {
              clr_flg(dc_inf->flgid, 0);
              if (flg & SCINFO_FLG_STOP)  goto _exit_dhcp_client_tsk;
              if (flg & SCINFO_FLG_RESET) continue;
              if (flg & SCINFO_FLG_DO)    ;
          }
          if (!dhcp_func)       break;          /* expire Lease time */
            
          ercd = dhcp_func(&dc_inf->dc);
          dc_inf->set_cbk(ercd, &dc_inf->dc);
          if (E_OK == ercd) {
            dhcp_func = dhcp_renew;
            continue;
          }
          
          /* DHCP failed. next step is ... REBINDING or expire Lease time */
          dhcp_func = (dhcp_func == dhcp_rebind) ? 0 : dhcp_rebind ;
        } while (1);
        if (E_CLS == ercd)  continue;   /* checking link status */
        
        /* expire Lease time */
        dc_inf->dc.ipaddr = 0;
        dc_inf->dc.subnet = 0;
        dc_inf->dc.gateway = 0;
        cfg_netaddr(&dc_inf->dc);
    }
_exit_dhcp_client_tsk:
    
    return E_OK;
}


/* Send frame now of DHCPc task */
ER dhcpc_tsk_renew(T_DHCPC_TSK_INFO *dc_inf)
{
    ER ercd;
    T_RTSK k_rtsk;
    
    /* Check parameter */
    for (ercd = 0; ercd < CFG_NET_DEV_MAX; ++ercd) {
        if (gDHCPC_TSK_INFO[ercd] == dc_inf)    break;
    }
    if (CFG_NET_DEV_MAX == ercd) {
        return E_PAR;
    }
    
    ercd = ref_tsk(dc_inf->tid, &k_rtsk);
    if (E_OK != ercd)   return ercd;
    
    if (k_rtsk.tskstat == TTS_WAI) {
        if (k_rtsk.wobjid == dc_inf->flgid) {
            ercd = set_flg(dc_inf->flgid, SCINFO_FLG_DO);
        }
    }
    
    return ercd;
}

/* Stop DHCPc task */
ER dhcpc_tsk_stop(T_DHCPC_TSK_INFO *dc_inf)
{
    ER ercd;
    T_RTSK k_rtsk;
    
    /* Check parameter */
    for (ercd = 0; ercd < CFG_NET_DEV_MAX; ++ercd) {
        if (gDHCPC_TSK_INFO[ercd] == dc_inf)    break;
    }
    if (CFG_NET_DEV_MAX == ercd) {
        return E_PAR;
    }
    
    ercd = ref_tsk(dc_inf->tid, &k_rtsk);
    if (E_OK != ercd)   return ercd;
    
    if (k_rtsk.tskstat != TTS_DMT) {
        ercd = set_flg(dc_inf->flgid, SCINFO_FLG_STOP);
        if (k_rtsk.wobjid != dc_inf->flgid) {
            rel_wai(dc_inf->tid);
        }
    }
    
    return ercd;
}
