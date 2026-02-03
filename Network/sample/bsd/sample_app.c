/***************************************************************************
    NetApp Sample (Operating as command in shell)
    Copyright (c)  2025, eForce Co., Ltd. All rights reserved.

    2014-04-02: Created.
    2022-09-29: Updated IPv6 ping process.
    2023-12-11: Incorporate modifications to app/sample_app.c
             -. Removed extra mqtt_usr_ini() runs from net_ini_soc().
             -. Enabled DHCP for multiple devices.
             -. IP address of the ping destination is no longer resolved by DNS.
    2023-12-12: Modified ping6 to allow device numbers to be specified.
    2025-03-13: Set the default device number for the ping command to DEV_ANY.
                Added option to disable UART (SAMPLE_ID_UART=0).
    2025-04-01: Changed to continue net_setup() regardless of DHCP result.
 ***************************************************************************/
#include "kernel.h"
#include "sample_netapp_cfg.h"      /* include Sample Application Settings */
#include "dns_client.h"
#include "ping_client.h"     

UW dns_svr_ip = 0x08080808;     /* Google DNS (8.8.8.8) */

/* Configuration */
#ifndef SAMPLE_ENA_DHCPc
#define SAMPLE_ENA_DHCPc        0xFF
#endif
#ifndef SAMPLE_ID_UART
#define SAMPLE_ID_UART          1
#endif

#define DHCP_ENB        SAMPLE_ENA_DHCPc    /* Enable DHCP */
#define ID_UART         SAMPLE_ID_UART      /* UART device ID */

#if DHCP_ENB
#include "dhcp_client.h"
#endif

extern void puts_com_opt(VB* msg); /* from main.c */
extern ER unet3_bsd_init(void);

#if SAMPLE_ID_UART
/* Control Block */
static T_SHELL_CTL net_ctl_console;

void net_console_tsk(VP_INT exinf)
{
    net_memset(&net_ctl_console, 0, sizeof(T_SHELL_CTL));

    net_ctl_console.typ    = TYP_COM;
    net_ctl_console.com_id = ID_UART;

    (void)shell_sta(&net_ctl_console);
}
#endif

#if (0 == SAMPLE_USE_GENSRC)
#ifndef SAMPLE_SHELL_TSKPRI
#define SAMPLE_SHELL_TSKPRI     6U
#endif

/*********************************************
    Shell Task for UART Terminal Console
 *********************************************/
#if SAMPLE_ID_UART
#if EXEC_CPU_64BIT
static const T_CTSK net_ctsk_console = {
    TA_HLNG|TA_FPU, (VP_INT)0, (FP)net_console_tsk, SAMPLE_SHELL_TSKPRI, 0x1800U, 0U, "Shell(UART)"};
#else
static const T_CTSK net_ctsk_console = {
    TA_HLNG|TA_FPU, (VP_INT)0, (FP)net_console_tsk, SAMPLE_SHELL_TSKPRI, 0x800U, 0U, "Shell(UART)"};
#endif

ER net_sta_console(void)
{
    ER ercd;

    /* Create Shell Task */
    ercd = acre_tsk((T_CTSK*)&net_ctsk_console);
    if (ercd <= E_OK) {
        return ercd;
    }

    /* Start for UART terminal console */
    ercd = sta_tsk((ID)ercd, 0x00U);

    return ercd;
}
#endif

/*******************************
    Socket ID Initialization (Generate Configurator)
 *******************************/
SID ID_SOC_DHCP;
SID ID_ICMP;
#ifdef IPV6_SUP
SID ID_ICMP6;
#endif
SID ID_SOC_DNS;

ER net_ini_soc(void)
{
    ER ercd;
    T_NODE lo_host = {0};

    lo_host.num = DEV_ANY;
    lo_host.ver = IP_VER4;
    lo_host.ipa = INADDR_ANY;

    /* DHCPc */
    lo_host.port = 68U; /* BOOTP PORT */
    ercd         = cre_soc(IP_PROTO_UDP, &lo_host);
    if (0 >= ercd) {
        return ercd;
    }
    ID_SOC_DHCP = (SID)ercd;
    (void)cfg_soc((SID)ercd, SOC_TMO_SND, (VP)5000U);
    (void)cfg_soc((SID)ercd, SOC_TMO_RCV, (VP)5000U);

    /* ---- */
    lo_host.port = 0U; /* port num set is application */

    /* PING */
    ercd = cre_soc(IP_PROTO_ICMP, &lo_host);
    if (0 >= ercd) {
        return ercd;
    }
    ID_ICMP = (SID)ercd;

#ifdef IPV6_SUP
    /* PING6 */
    lo_host.ver = IP_VER6;
    ercd = cre_soc(IP_PROTO_ICMPV6, &lo_host);
    if (0U >= ercd) {
        return ercd;
    }
    ID_ICMP6 = ercd;
    lo_host.ver = IP_VER4;
#endif

    /* DNSc */
    ercd = cre_soc(IP_PROTO_UDP, &lo_host);
    if (0 >= ercd) {
        return ercd;
    }
    ID_SOC_DNS = ercd;
    cfg_soc(ercd, SOC_TMO_SND, (VP)5000);
    cfg_soc(ercd, SOC_TMO_RCV, (VP)5000);

    return E_OK;
}

extern UW NET_DEV_MAX;

ER net_app_ini()
{
    ER ercd;
#if (0 < DHCP_ENB)
    VB ebuf[32];
    T_HOST_ADDR dhcp_addr;
    INT         i, k;
#endif

    /* Initialize Socket ID */
    ercd = net_ini_soc();
    if (ercd != E_OK) {
        return ercd;
    }

    (void)dly_tsk(4000U);

/* DHCP Client */
#if (0 < DHCP_ENB)
    for (k = 0; k < NET_DEV_MAX; k++) {
        /* Automatically set IP address only for DHCP enabled devices */
        if (0 == (DHCP_ENB & (1 << k)))     continue;
        
        net_strcpy(ebuf, "\r\n");
        if (1 < NET_DEV_MAX) {
            net_strcat(ebuf, "devID=");
            ercd = net_strlen(ebuf);
            net_itoa(k + 1, &ebuf[ercd], 10);
            net_strcat(ebuf, " ");
        }
        net_strcat(ebuf, "DHCP:");
        puts_com_opt(ebuf);

        net_memset(&dhcp_addr, 0, sizeof(dhcp_addr));
        dhcp_addr.dev_num = (k+1);
        dhcp_addr.socid   = ID_SOC_DHCP;
        for (i = 0; i < 5; i++) {
            ercd = dhcp_client(&dhcp_addr);
            if (ercd == E_OK) {
                break;
            }
            puts_com_opt("Retry ");
        }
        if (ercd == E_OK) {
            puts_com_opt("Success");
        } else {
            puts_com_opt("Not Found");
        }
    }
    puts_com_opt("\r\n\r\n");
    ercd = E_OK;    /* Success regardless of DHCP result. */
#endif

    /* BSD wrapper */
    ercd = unet3_bsd_init();
    if (ercd != E_OK) {
        return ercd;
    }
    
#if SAMPLE_ID_UART
    ercd = net_sta_console();
    if (ercd != E_OK) {
        return ercd;
    }
#endif
    
    return ercd;
}
#else
ER net_app_ini()
{
    ER ercd;
    
    /* BSD wrapper */
    ercd = unet3_bsd_init();
    if (ercd != E_OK) {
        return ercd;
    }   
    
    return act_tsk(ID_CONSOLE_TSK);
}
#endif  // (0 == SAMPLE_USE_GENSRC)

/* Extend of dns_get_ipaddr (include ip_aton) */
ER dns_get_ipa_opt(UW dnsd_ipa, VB *str, UW *ipa)
{
    ER ercd = E_OK;

    *ipa = ip_aton(str);
    if (*ipa == 0) {
        ercd = dns_get_ipaddr(ID_SOC_DNS, dnsd_ipa, str, ipa);
    }
    return ercd;
}

/* Command 'ipcfg' */
ER shell_usr_cmd_ipcfg(VP ctrl, INT argc, VB *argv[])
{
    T_NET_ADR adr;
    ER ercd;
    UW tmp;
    VB buf[32];

    ercd = net_ref(0, NET_IP4_CFG, (VP)&adr);
    if (ercd != E_OK) {
        return ercd;
    }

    shell_puts(ctrl, SPL_LF" Enter new values..");

    shell_puts(ctrl, SPL_LF" IP Address  : ");
    shell_gets(ctrl, (char *)buf, sizeof(buf));
    tmp = ip_aton((char *)buf);
    if (tmp != 0) adr.ipaddr = tmp;

    shell_puts(ctrl, SPL_LF" Subnet Mask : ");
    shell_gets(ctrl, (char *)buf, sizeof(buf));
    tmp = ip_aton((char *)buf);
    if (tmp != 0) adr.mask = tmp;

    shell_puts(ctrl, SPL_LF" Gateway     : ");
    shell_gets(ctrl, (char *)buf, sizeof(buf));
    tmp = ip_aton((char *)buf);
    if (tmp != 0) adr.gateway = tmp;

    net_cfg(0, NET_IP4_CFG, (VP)&adr);

    return ercd;
}



/* Command 'ping' */
ER shell_usr_cmd_ping(VP ctrl, INT argc, VB *argv[])
{
    UB cnt;
    ER ercd;
    UW ipa;
    T_PING_CLIENT pingc;

    net_memset(&pingc, 0, sizeof(pingc));
    pingc.sid = ID_ICMP;

    if(argc >= 3)
        pingc.len = net_atoi(argv[2]);
    else
        pingc.len = 1000;

    if(argc >= 4)
        pingc.devnum = net_atoi(argv[3]);
    else
        pingc.devnum = DEV_ANY;

    /* hostname */
    ercd = dns_get_ipa_opt(SPL_DNS_SERVER, argv[1], &ipa);
    if (E_OK != ercd) {
        return ercd;
    }

    pingc.ipa = ipa;
    if (pingc.ipa == 0) {
        shell_puts(ctrl, SPL_LF" invalid ip address");
        return E_OBJ;
    }

    for(cnt=0; cnt<1; cnt++) {
        ercd = ping_client(&pingc);
        if(ercd != E_OK) {
            shell_puts(ctrl, SPL_LF" ping request timed out");
            break;
        }
        shell_puts(ctrl, SPL_LF" ping request successful ");
    }

    return ercd;
}


#ifdef IPV6_SUP
/* Command 'ping6' */
ER shell_usr_cmd_ping6(VP ctrl, INT argc, VB *argv[])
{
    UB cnt;
    ER ercd;
    T_PING_CLIENT_V6 pingc;
    UW ip6addr[4];

    net_memset(&pingc, 0, sizeof(pingc));
    pingc.sid = ID_ICMP6;

    if(argc >= 3)
        pingc.len = net_atoi(argv[2]);
    else
        pingc.len = 1000;

    if(argc >= 4)
        pingc.devnum = net_atoi(argv[3]);
    else
        pingc.devnum = DEV_ANY;

    pingc.ip6addr = ip6addr;
    ip6_aton(argv[1], pingc.ip6addr);
    for(cnt=0; cnt<1; cnt++) {
        ercd = ping6_client(&pingc);
        if(ercd != E_OK) {
            shell_puts(ctrl, SPL_LF" ping request timed out");
            break;
        }
        shell_puts(ctrl, SPL_LF" ping request successful ");
    }

    return ercd;
}
#endif

