/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    Sample SNTP client Task header file
    Copyright (c)  2016, eForce Co., Ltd. All rights reserved.

    Version Information
      2016.02.02: Created.
 ***************************************************************************/

#include "sample_tsk_sntpc.h"

/* SNTPc task event */
#define SCINFO_FLG_STOP     0x0001
#define SCINFO_FLG_RESET    0x0002
#define SCINFO_FLG_DO       0x0004
#define SCINFO_FLG_ALL      (SCINFO_FLG_STOP | SCINFO_FLG_RESET | SCINFO_FLG_DO)

static T_SNTPC_TSK_INFO *gSNTPC_TSK_INFO[CFG_NET_DEV_MAX] = {0};


#ifdef ID_SOC_DNS       /* enable DNSc socket */
#include "dns_client.h"
static ER get_sntp_svr_ipa(T_SNTPC_TSK_INFO *sc_inf)
{
    UW ipa;
    ER ercd = E_CLS;

    ercd = dns_get_ipaddr(ID_SOC_DNS, sc_inf->dns_svr_ipa, sc_inf->domain, &ipa);
    if (E_OK != ercd) {
        ipa = ip_aton(sc_inf->domain);
        if (ipa) {
            ercd = E_OK;
        }
        else if (sc_inf->sc.ipa) {
            ipa = sc_inf->sc.ipa;
            ercd = E_OK;
        }
    }
    if (E_OK == ercd) {
        sc_inf->sc.ipa = ipa;
    }
    return ercd;
}
#else
static ER get_sntp_svr_ipa(T_SNTPC_TSK_INFO *sc_inf)
{
    UW ipa;
    ER ercd = E_CLS;

    ipa = ip_aton(sc_inf->domain);
    if (ipa) {
        ercd = E_OK;
    }
    else if (sc_inf->sc.ipa) {
        ipa = sc_inf->sc.ipa;
        ercd = E_OK;
    }
    if (E_OK == ercd) {
        sc_inf->sc.ipa = ipa;
    }
    return ercd;
}
#endif

/* SNTPc task initialize */
static ER sntpc_tsk_ini(T_SNTPC_TSK_INFO *sc_inf)
{
    T_NET_ADR adr;
    ER ercd = E_OK;
    
    /* Check parameter */
    if (!(sc_inf->flgid && sc_inf->interval && sc_inf->set_cbk)) {
        ercd = E_PAR;
    }
    if (!(sc_inf->sc.ipa || sc_inf->domain)) {
        ercd = E_PAR;
    }

    if (E_OK == ercd) {
        /* Get this task ID */
        ercd = get_tid(&sc_inf->tid);
        if (0 == sc_inf->sc.devnum) {
            sc_inf->sc.devnum = 1;
        }
        gSNTPC_TSK_INFO[sc_inf->sc.devnum - 1] = sc_inf;
        
        /* Set DNS server IP (equal Gateway Address) */
        if (0 == sc_inf->dns_svr_ipa) {
            net_ref(sc_inf->sc.devnum, NET_IP4_CFG, (VP)&adr);
            sc_inf->dns_svr_ipa = adr.gateway;
        }
    }
    
    return ercd;
}

/* sample SNTP client task */
ER sntpc_tsk(T_SNTPC_TSK_INFO *sc_inf)
{
    ER ercd;
    UW sec, fra;
    FLGPTN flg;
    UH retry;
    
    ercd = sntpc_tsk_ini(sc_inf);
    if (E_OK != ercd) {
        return ercd;
    }
    
    set_flg(sc_inf->flgid, SCINFO_FLG_DO);  /* first send is no wait */
    for (;;) {
        /* Waiting send ... */
        for (retry = 0; retry < 1000; ++retry) {
            ercd = twai_flg(sc_inf->flgid, SCINFO_FLG_ALL, TWF_ORW, &flg, sc_inf->interval);
            if (E_TMOUT != ercd)    break;
        }

        /* Check notify events */
        if (E_OK == ercd) {
            clr_flg(sc_inf->flgid, 0);
            if (flg & SCINFO_FLG_STOP)  break;      /* exit sntp_client_tsk */;
            if (flg & SCINFO_FLG_RESET) continue;
            if (flg & SCINFO_FLG_DO)    ;
        }
        
        /* Get SNTP server address */
        ercd = get_sntp_svr_ipa(sc_inf);
        if (E_OK != ercd) {
            break;          /* get failed. */
        }
        
        /* Connect and retry to SNTP server */
        retry = sc_inf->retry;
        do {
            ercd = sntp_client(&sc_inf->sc, &sec, &fra);
            if ((E_OK == ercd) || (E_RLWAI == ercd))    break;
        } while (retry--);
        sc_inf->set_cbk(ercd, sec, fra);    /* User function callback */
    }
    
    return E_OK;
}


/* Send frame now of SNTPc task */
ER sntpc_tsk_sendnow(T_SNTPC_TSK_INFO *sc_inf)
{
    ER ercd;
    T_RTSK k_rtsk;
    
    /* Check parameter */
    for (ercd = 0; ercd < CFG_NET_DEV_MAX; ++ercd) {
        if (gSNTPC_TSK_INFO[ercd] == sc_inf)    break;
    }
    if (CFG_NET_DEV_MAX == ercd) {
        return E_PAR;
    }
    
    ercd = ref_tsk(sc_inf->tid, &k_rtsk);
    if (E_OK != ercd)   return ercd;
    
    if (k_rtsk.tskstat == TTS_WAI) {
        if (k_rtsk.wobjid == sc_inf->flgid) {
            ercd = set_flg(sc_inf->flgid, SCINFO_FLG_DO);
        }
    }
    
    return ercd;
}

/* Stop SNTPc task */
ER sntpc_tsk_stop(T_SNTPC_TSK_INFO *sc_inf)
{
    ER ercd;
    T_RTSK k_rtsk;
    
    /* Check parameter */
    for (ercd = 0; ercd < CFG_NET_DEV_MAX; ++ercd) {
        if (gSNTPC_TSK_INFO[ercd] == sc_inf)    break;
    }
    if (CFG_NET_DEV_MAX == ercd) {
        return E_PAR;
    }
    
    ercd = ref_tsk(sc_inf->tid, &k_rtsk);
    if (E_OK != ercd)   return ercd;
    
    if (k_rtsk.tskstat != TTS_DMT) {
        ercd = set_flg(sc_inf->flgid, SCINFO_FLG_STOP);
        if (k_rtsk.wobjid != sc_inf->flgid) {
            rel_wai(sc_inf->tid);
        }
    }
    
    return ercd;
}
