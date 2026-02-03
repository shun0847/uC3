/***************************************************************************
    SMTP Sample (Operating as command in shell)
    Copyright (c)  2014, eForce Co., Ltd. All rights reserved.

    2014-04-02: Created.
 ***************************************************************************/
#include "kernel.h"
#include "sample_netapp_cfg.h"      /* include Sample Application Settings */
#if SAMPLE_ENA_SMTPua

#include "smtp_client.h"

static UB exec_user = 0;
static VB smtp_buf[1024];

#if (0 == SAMPLE_USE_GENSRC)	/* no use configurator */
SID ID_SOC_SMTP;

ER sample_smtpua_ini()
{
	ER ercd;
    T_NODE lo_host;

    lo_host.num = SAMPLE_SOCDEV_CLI;
    lo_host.ver = IP_VER4;
    lo_host.ipa = INADDR_ANY;
    lo_host.port = PORT_ANY;

    /* SMTPua */
    ercd = cre_soc(IP_PROTO_TCP, &lo_host);
    if (0 >= ercd) {
        return ercd;
    }
    ID_SOC_SMTP = ercd;

    return E_OK;
}
#endif

/* Command 'mail' */
ER shell_usr_cmd_mail(VP ctrl, INT argc, VB *argv[])
{
    T_SMTP_CLIENT   sc;
    T_SMTP_MAIL     ml;
    T_SMTP_FILE     sf;
    ER ercd;
    UW ipa;
    
    if (exec_user)  return E_OACV;
    exec_user = 1;

    net_memset(&sc, 0, sizeof(sc));
    net_memset(&ml, 0, sizeof(ml));
    net_memset(&sf, 0, sizeof(sf));
    
    ercd = dns_get_ipa_opt(SPL_DNS_SERVER, SPL_SMTP_SERVER, &ipa);
    if (E_OK != ercd) {
        exec_user = 0;
        return ercd;
    }

    /* set SMTP settings */
    sc.svr.num = 0;
    sc.svr.ipa  = ipa;
    sc.svr.ver = IP_VER4;
    sc.sid  = ID_SOC_SMTP;
    sc.svr.port = SPL_SMTP_PORT;
    sc.buf  = smtp_buf;
    sc.len  = sizeof(smtp_buf);
    sc.usr  = SPL_SMTP_USER;
    sc.pw   = SPL_SMTP_PASS;
    sc.auth_type = SPL_SMTP_AUTH;
    
    ercd = smtp_login(&sc);
    if (E_OK != ercd) {
        smtp_quit(&sc);
        exec_user = 0;
        return ercd;
    }
    
    /* set Send Mail settings */
    ml.from = SPL_ML_FROM;
    ml.to   = SPL_ML_TO;
    ml.cc   = SPL_ML_CC;
    ml.bcc  = SPL_ML_BCC;
    ml.subject  = SPL_ML_SUBJECT;
    ml.body     = SPL_ML_BODY;
    ml.charset  = SPL_ML_CHARSET;
    ml.cs8bit   = SPL_ML_CHARSET8BIT;
    
#ifdef SPL_ML_ATTACH_FILE   /* Attach File */
    ml.file = &sf;
    sf.type = (SPL_MLAF_USE_MEM) ? SMTP_FTYP_MEM : SMTP_FTYP_FS ;
    sf.name = SPL_MLAF_NAME;
    sf.buf  = SPL_MLAF_BUFPATH;
    sf.len  = SPL_MLAF_BUFLEN;
#endif

    ercd = smtp_snd_mail(&sc, &ml);
    if (E_OK != ercd) {
        smtp_quit(&sc);
        exec_user = 0;
        return ercd;
    }
    
    ercd = smtp_quit(&sc);
    if (E_OK != ercd) {
        exec_user = 0;
        return ercd;
    }
    
    exec_user = 0;
    return E_OK;
}

#endif  /* #if SAMPLE_ENA_SMTPua */
