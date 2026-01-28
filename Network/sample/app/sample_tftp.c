/***************************************************************************
    TFTP Sample (Operating as command in shell)
    Copyright (c)  2014, eForce Co., Ltd. All rights reserved.

    2014-04-02: Created.
 ***************************************************************************/
#include "kernel.h"
#include "sample_netapp_cfg.h"      /* include Sample Application Settings */

#if SAMPLE_ENA_TFTPc
#include "tftp_client.h"

/* Complement path name */
extern ER ftp_check_ffsysdrv(const VB *file_name, VB *fix_name, UB flen);

#if (0 == SAMPLE_USE_GENSRC)	/* no use configurator */
SID ID_SOC_TFTPC;

ER sample_tftpc_ini()
{
	ER ercd;
    T_NODE lo_host;

    lo_host.num = 1;
    lo_host.ver = IP_VER4;
    lo_host.ipa = INADDR_ANY;
    lo_host.port = PORT_ANY;

    /* TFTPc */
    ercd = cre_soc(IP_PROTO_UDP, &lo_host);
    if (0 >= ercd) {
        return ercd;
    }
    ID_SOC_TFTPC = ercd;
    cfg_soc(ercd, SOC_TMO_RCV, (VP)5000);
    cfg_soc(ercd, SOC_TMO_SND, (VP)5000);

    return E_OK;
}
#endif

/* Command 'tftp' */
ER shell_usr_cmd_tftp(VP ctrl, INT argc, VB *argv[])
{
    T_TFTP_CLIENT   tfc;
    ER ercd;
    UW ipa;
    UB idx;
    VB *lo_file, *rmt_file;
    VB fnbuf[32];
    
    net_memset(&tfc, 0, sizeof(tfc));
    
    /*---- check argument parameter ----*/
    /* binary set */
    idx = 1;
    if (0 == net_strcmp(argv[idx], "-i")) {
        ++idx;
        tfc.asc = 0;
    }
    else {
        tfc.asc = 1;
    }
    
    /* hostname */
    ercd = dns_get_ipa_opt(SPL_DNS_SERVER, argv[idx], &ipa);
    if (E_OK != ercd) {
        return ercd;
    }
    ++idx;
    
    /* localfile, remotefile */
    lo_file  = argv[idx + 1];
    rmt_file = ((idx + 2) >= argc) ? argv[idx + 1] : argv[idx + 2];
    
    /* fix file path */
    ercd = ftp_check_ffsysdrv(lo_file, fnbuf, sizeof(fnbuf));
    if (0 < ercd) lo_file =  fnbuf;
    
    /* Remote settings */
    tfc.rmt.ver = IP_VER4;
    tfc.rmt.ipa  = ipa;
    tfc.rmt.num  = 0;
    tfc.rmt.port = SPL_TFTP_PORT;
    tfc.dat_sid  = ID_SOC_TFTPC;

    /* get or put, otherwise ... */
    if (0 == net_strcmp(argv[idx], "get")) {
        ercd = tftp_get_file(&tfc, lo_file, rmt_file);
    }
    else if (0 == net_strcmp(argv[idx], "put")) {
        ercd = tftp_put_file(&tfc, lo_file, rmt_file);
    }
    else {
        ercd = E_PAR;
    }
    
    return ercd;
}

#endif /* SAMPLE_ENA_TFTPc */
