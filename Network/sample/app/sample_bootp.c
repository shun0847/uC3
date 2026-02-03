/***************************************************************************
    BOOTPc Sample (Operating as command in shell)
    Copyright (c)  2019, eForce Co., Ltd. All rights reserved.

    2019-06-25: Created.
 ***************************************************************************/
#include "kernel.h"
#include "sample_netapp_cfg.h"      /* include Sample Application Settings */

#ifndef SAMPLE_ENA_BOOTPc
#define SAMPLE_ENA_BOOTPc		0
#endif

#if SAMPLE_ENA_BOOTPc

#include "bootp_client.h"

#define ID_SOC_BOOTPC		ID_SOC_DHCP		/* use DHCPc socket */

void print_bootp_info(VP ctrl, const T_BOOTP_CLIENT* btpc)
{
    UW nx;
    VB ipbuf[16];
	VB pbuf[128];
    
	shell_puts(ctrl, SPL_LF"/*------ BOOTP Information ----------*/"SPL_LF);
    
	pbuf[0] = 0;
    ip_ntoa(ipbuf, btpc->ipa);
	net_strcat(pbuf, "ip-addr     : ");
	net_strcat(pbuf, ipbuf);
	net_strcat(pbuf, SPL_LF);
	
    ip_ntoa(ipbuf, btpc->ipa);
	net_strcat(pbuf, "subnetmask  : ");
	net_strcat(pbuf, ipbuf);
	net_strcat(pbuf, SPL_LF);
	
	shell_puts(ctrl, pbuf);
    
	pbuf[0] = 0;
    for (nx = 0; nx < BOOTPC_GW_NUM; ++nx) {
        ip_ntoa(ipbuf, btpc->gateway[nx]);
		net_strcat(pbuf, "gateway ip-addr     : ");
		net_strcat(pbuf, ipbuf);
		net_strcat(pbuf, SPL_LF);
    }
	shell_puts(ctrl, pbuf);
	
	pbuf[0] = 0;
    for (nx = 0; nx < BOOTPC_DNS_NUM; ++nx) {
        ip_ntoa(ipbuf, btpc->dns[nx]);
		net_strcat(pbuf, "dns server ip-addr  : ");
		net_strcat(pbuf, ipbuf);
		net_strcat(pbuf, SPL_LF);
    }
	shell_puts(ctrl, pbuf);
    
	pbuf[0] = 0;
	net_strcat(pbuf, "server host name    : ");
	net_strcat(pbuf, btpc->sname);
	net_strcat(pbuf, SPL_LF);
	shell_puts(ctrl, pbuf);
	
    
	pbuf[0] = 0;
	net_strcat(pbuf, "boot file name      : ");
	net_strcat(pbuf, btpc->file);
	net_strcat(pbuf, SPL_LF);
	shell_puts(ctrl, pbuf);
}

/* Command 'bootp' */
ER shell_usr_cmd_bootp(VP ctrl, INT argc, VB *argv[])
{
    int i;
    VB btp_svrhost[BOOTPC_SNAME_MAX];
    VB btp_bootfile[BOOTPC_FILE_MAX];
    T_BOOTP_CLIENT btpc[1];
    ER ercd;
    
    net_memset(&btpc[0], 0, sizeof(T_BOOTP_CLIENT));
    btpc[0].sid = ID_SOC_BOOTPC;
    btpc[0].dev_num = (argc >= 2) ? net_atoi(argv[1]) : 1 ;
    net_memset(btp_svrhost, 0, sizeof(btp_svrhost));
    net_memset(btp_bootfile, 0, sizeof(btp_bootfile));
    btpc[0].sname = btp_svrhost;
    btpc[0].file = btp_bootfile;
    for (i = 0; i < 10; i++) {
        ercd = bootp_client(&btpc[0]);
        if (ercd == E_OK) {
            break;
        }
        tslp_tsk(1000);
    }    
    if (ercd == E_TMOUT) {
        ercd = E_OK;
    }
    if (ercd != E_OK) {
        return ercd;
    }
    /* Print BOOTP information */
    print_bootp_info(ctrl, &btpc[0]);
	
    return ercd;
}

#endif /* #if SAMPLE_ENA_BOOTPc */
