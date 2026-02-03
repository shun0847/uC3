/***************************************************************************
    SNTP Sample (Operating as command in shell)
    Copyright (c)  2014, eForce Co., Ltd. All rights reserved.

    2014-04-05: Created.
 ***************************************************************************/
#include "kernel.h"
#include "sample_netapp_cfg.h"      /* include Sample Application Settings */
#if SAMPLE_ENA_SNTP

#include "sntp_client.h"
#include "sntp_server.h"            /* use converter api */

#define ID_NETIF_DEV   1

#if (0 == SAMPLE_USE_GENSRC)    /* no use configurator */

/*******************************
    SNTP Server Task
 *******************************/
T_SNTP_SERVER net_ctl_sntps;
#if !SAMPLE_ENA_HTTPd
static SID ID_SOC_SNTPC;
#else
extern SID ID_SOC_SNTPC;
#endif
static SID ID_SOC_SNTPD;
static ID ID_SNTP_FLG;
#ifdef SNTP_MULTICAST_VALID
static ID ID_SNTP_MC_TSK;
#endif

void sntpd_tsk(VP_INT exinf)
{
    net_memset((char* )&net_ctl_sntps, 0, sizeof(net_ctl_sntps));
    net_ctl_sntps.dev_num = ID_NETIF_DEV;
    net_ctl_sntps.socid   = ID_SOC_SNTPD;
    net_ctl_sntps.flgid   = ID_SNTP_FLG;
#ifdef SNTP_MULTICAST_VALID
    net_ctl_sntps.mc_tid  = ID_SNTP_MC_TSK;
#endif
    sntp_server(&net_ctl_sntps);
}
#if EXEC_CPU_64BIT
static const T_CTSK net_ctsk_sntps    = {TA_HLNG | TA_FPU, (VP_INT)0, (FP)sntpd_tsk, 4, 0x480, 0, "SntpServerTask"};
#else
static const T_CTSK net_ctsk_sntps    = {TA_HLNG | TA_FPU, (VP_INT)0, (FP)sntpd_tsk, 4, 0x180, 0, "SntpServerTask"};
#endif
static const T_CFLG net_cflg_sntps_flg = {TA_TFIFO|TA_WMUL, 0, "SntpServerTask"};

#ifdef SNTP_MULTICAST_VALID
#if EXEC_CPU_64BIT
static const T_CTSK net_ctsk_sntps_mc = {TA_HLNG | TA_FPU, (VP)ID_NETIF_DEV, (FP)sntp_mc_tsk , 4, 0x480, 0, "SntpMcSendTask"};
#else
static const T_CTSK net_ctsk_sntps_mc = {TA_HLNG | TA_FPU, (VP)ID_NETIF_DEV, (FP)sntp_mc_tsk , 4, 0x180, 0, "SntpMcSendTask"};
#endif
#endif

#ifndef INT_SNTP_IMASK
#define INT_SNTP_IMASK	240
#endif

static const T_CISR net_cisr_sntps_tick =
  {TA_NULL, (VP_INT)0, INT_SNTP_TICK, (FP)isr_sntp_tick, INT_SNTP_IMASK};


ER sample_sntp_ini()
{
    ER ercd;
    T_NODE lo_host;

    lo_host.num = SAMPLE_SOCDEV_CLI;
    lo_host.ver = IP_VER4;
    lo_host.ipa = INADDR_ANY;
    lo_host.port = PORT_ANY;

    /*---- init resource --------------------------------*/
#if !SAMPLE_ENA_HTTPd
    /* SNTPc */
    ercd = cre_soc(IP_PROTO_UDP, &lo_host);
    if (0 >= ercd) {
        return ercd;
    }
    ID_SOC_SNTPC = ercd;
#endif
    
    /* SNTPs */
    lo_host.num = SAMPLE_SOCDEV_SER;
    lo_host.port = 123;
    ercd = cre_soc(IP_PROTO_UDP, &lo_host);
    if (0 >= ercd) {
        return ercd;
    }
    cfg_soc(ercd, SOC_TMO_SND, (VP)1000);
    cfg_soc(ercd, SOC_TMO_RCV, (VP)1000);
    ID_SOC_SNTPD = ercd;
    ercd = acre_isr((T_CISR *)&net_cisr_sntps_tick);
    if (ercd <= E_OK) {
        return ercd;
    }
    ercd = acre_flg((T_CFLG*)&net_cflg_sntps_flg);
    if (ercd <= E_OK) {
        return ercd;
    }
    ID_SNTP_FLG = ercd;

    /*---- start task -----------------------------------*/
#ifdef SNTP_MULTICAST_VALID
    ercd = acre_tsk((T_CTSK*)&net_ctsk_sntps_mc);
    if (ercd <= E_OK) {
        return ercd;
    }
    ID_SNTP_MC_TSK = ercd;
#endif
#if 1
    ercd = acre_tsk((T_CTSK*)&net_ctsk_sntps);
    if (ercd <= E_OK) {
        return ercd;
    }
    act_tsk((ID)ercd);
#endif
    return E_OK;
}

#endif


/* Parse Command */
static void parse_cmd(VB *s, INT *argc, VB *argv[])
{
    INT max_argc = *argc;

    /* Parse command */
    argv[0] = "";
    for (*argc = 0; *argc < max_argc; (*argc)++)
    {   while (' ' == *s)
            s++;
        if ('\0' == *s)
            break;
        argv[*argc] = s;
        s = net_strchr(s, ' ');
        if (s == NULL)
            break;
        *s++ = '\0';
    }
    if(0 == net_strcmp(argv[0], "")) {
        return;
    }
    (*argc)++;
}

ER shell_usr_cmd_sntp(VP ctrl, INT argc, VB *argv[])
{
    T_SNTP_CLIENT sc;
    T_NTP_TIM   ntp;
    T_TM_SIMPLE tm;
    VB tmstr[20];
    ER ercd;
    UW ipa;

    /* SNTP server address */
    ercd = dns_get_ipa_opt(SPL_DNS_SERVER, argv[1], &ipa);
    if (E_OK != ercd) {
        return ercd;
    }

    net_memset(&sc, 0, sizeof(sc));
    sc.ipa = ipa;
    sc.sid = ID_SOC_SNTPC;

    ercd = sntp_client(&sc, &ntp.sec, &ntp.fra);
    if (E_OK != ercd) {
        return ercd;
    }

    shell_puts(ctrl, SPL_LF"NTP Timestamp(sec,fra): ");
    net_itoa(ntp.sec, tmstr, 16);
    shell_puts(ctrl, "0x");
    shell_puts(ctrl, tmstr);
    shell_puts(ctrl, ", 0x");
    net_itoa(ntp.fra, tmstr, 16);
    shell_puts(ctrl, tmstr);

    net_memset(&tm, 0, sizeof(tm));
    shell_puts(ctrl, SPL_LF"UTC Time: ");
    cnv_ntp_to_tm(&tm, &ntp);
    ercd = cnv_tm_to_str(tmstr, &tm, SNTP_TMFMT_DATE);
    if (E_OK != ercd) {
        shell_puts(ctrl, "Convert Error."SPL_LF);
    }
    else {
        shell_puts(ctrl, tmstr);
        shell_puts(ctrl, SPL_LF);
    }

    return ercd;
}

ER shell_usr_cmd_sntp_server(VP ctrl, INT argc, VB *argv[])
{
    ER ercd;
    VB tmp[32];
    INT sc_argc;
    VB *sc_argv[4];
    VB *prm1, *prm2;
    T_SNTP_CLIENT sc;
    T_NTP_TIM   ntp;

    for (;;) {
        shell_puts(ctrl, SPL_LF"sntp_server> ");
        ercd = shell_gets(ctrl, tmp, sizeof(tmp));
        if (E_OK != ercd)   break;

        sc_argc = sizeof(sc_argv)/sizeof(sc_argv[0]);
        parse_cmd(tmp, &sc_argc, sc_argv);

        if (0 == net_strcmp(sc_argv[0], "?")) {         /* Command List */
            shell_puts(ctrl, SPL_LF" ?      Help (This command)");
            shell_puts(ctrl, SPL_LF" auto   Auto time adjust (SNTP client)");
            shell_puts(ctrl, SPL_LF" get    Get SNTP server time (UTC)");
#ifdef SNTP_MULTICAST_VALID
            shell_puts(ctrl, SPL_LF" mc_set Set Multicast SNTP packet event");
            shell_puts(ctrl, SPL_LF" mc_stp Stop Multicast SNTP packet event");
#endif
            shell_puts(ctrl, SPL_LF" quit   Exit Sub command");
        }
        else if (0 == net_strcmp(sc_argv[0], "auto")) {
            net_memset(&sc, 0, sizeof(sc));
            ercd = dns_get_ipa_opt(SPL_DNS_SERVER, "ntp.nict.jp", &sc.ipa);
            if (E_OK != ercd) {
                return ercd;
            }
            sc.sid = ID_SOC_SNTPC;

            ercd = sntp_client(&sc, &ntp.sec, &ntp.fra);
            if (E_OK == ercd) {
                ercd = sntp_set_tim(&ntp);
            }
        }
        else if (0 == net_strcmp(sc_argv[0], "get")) {
            prm1 = tmp;
            ercd = sntp_get_tim_Str(prm1, SNTP_TMFMT_DATE);
            if (E_OK == ercd) {
                shell_puts(ctrl, SPL_LF" ");
                shell_puts(ctrl, prm1);
            }
        }
#ifdef SNTP_MULTICAST_VALID
        else if (0 == net_strcmp(sc_argv[0], "mc_set")) {
            prm1 = (VB*)(ADDR)net_atoi(sc_argv[1]); /* interval */
            prm2 = (VB*)(ADDR)net_atoi(sc_argv[2]); /* send_cnt */
            ercd = sntp_mc_send(ID_NETIF_DEV, (UB)((ADDR)prm1), (UW)((ADDR)prm2));
            if (E_OK != ercd) {
                shell_puts(ctrl, SPL_LF" Usage: mc_set <interval> <send_cnt>");
            }
        }
        else if (0 == net_strcmp(sc_argv[0], "mc_stp")) {
            ercd = sntp_mc_stop(ID_NETIF_DEV);
        }
#endif
        else if (0 == net_strcmp(sc_argv[0], "quit")) {
            break;
        }

        if (E_OK != ercd) {
            shell_puts(ctrl, SPL_LF"Error");
        }
    }

    return ercd;
}

#endif /* #if SAMPLE_ENA_SNTP */
