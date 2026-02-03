/***************************************************************************
    FTP Sample (Operating as command in shell)
    Copyright (c)  2014, eForce Co., Ltd. All rights reserved.

    2014-04-02: Created.
 ***************************************************************************/
#include "kernel.h"
#include "sample_netapp_cfg.h"      /* include Sample Application Settings */
#if SAMPLE_ENA_FTPc

#include "ftp_client.h"

static T_FTP_CLIENT fc;
static UB exec_user = 0;
static VB ls_buf[SPL_FTP_LS_BUF_SZ];

/* Complement path name */
extern ER ftp_check_ffsysdrv(const VB *file_name, VB *fix_name, UB flen);

#if (0 == SAMPLE_USE_GENSRC)	/* no use configurator */
SID ID_SOC_FTPC_CMD;
SID ID_SOC_FTPC_DATA;

ER sample_ftpc_ini()
{
	ER ercd;
    T_NODE lo_host;

    lo_host.num = SAMPLE_SOCDEV_CLI;
    lo_host.ver = IP_VER4;
    lo_host.ipa = INADDR_ANY;
    lo_host.port = PORT_ANY;

    /* FTPc */
    ercd = cre_soc(IP_PROTO_TCP, &lo_host);
    if (0 >= ercd) {
        return ercd;
    }
    ID_SOC_FTPC_CMD = ercd;
    cfg_soc(ercd, SOC_TMO_RCV, (VP)25000);
    cfg_soc(ercd, SOC_TMO_SND, (VP)25000);
    cfg_soc(ercd, SOC_TMO_CON, (VP)25000);
    cfg_soc(ercd, SOC_TMO_CLS, (VP)25000);

    ercd = cre_soc(IP_PROTO_TCP, &lo_host);
    if (0 >= ercd) {
        return ercd;
    }
    ID_SOC_FTPC_DATA = ercd;
    cfg_soc(ercd, SOC_TMO_RCV, (VP)25000);
    cfg_soc(ercd, SOC_TMO_SND, (VP)25000);
    cfg_soc(ercd, SOC_TMO_CON, (VP)25000);
    cfg_soc(ercd, SOC_TMO_CLS, (VP)25000);

    return E_OK;
}
#endif



/* Execute LIST/NLST Command */
static ER ftp_subcmd_list(VP ctrl, T_FTP_CLIENT *ftp, const VB *dir, UB nameonly)
{
    T_FTP_LIST list;
    ER ercd;

    net_memset(&list, 0, sizeof(list));
    list.buf = ls_buf;
    list.len = sizeof(ls_buf) - 1;
    list.nameonly = nameonly;

    ercd = ftp_cmd_list(ftp, dir, &list);
    while (list.len == ercd) {
        ls_buf[ercd] = '\0';
        shell_puts(ctrl, ls_buf);
        ercd = ftp_cmd_list_next(&list);
    }

    if (0 > ercd) {
        /* Error LIST/NLST Command */
    }
    else {
        ls_buf[ercd] = '\0';
        shell_puts(ctrl, ls_buf);
        ercd = E_OK;
    }

    return ercd;
}

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

/* Command 'ftp' */
ER shell_usr_cmd_ftp(VP ctrl, INT argc, VB *argv[])
{
    ER ercd;
    UW ipa;
    VB tmp[64];
    VB fnbuf[32];
    INT sc_argc;
    VB *sc_argv[4];
    VB *prm1, *prm2;
    
    if (exec_user)  return E_OACV;
    exec_user = 1;

    /* hostname */
    ercd = dns_get_ipa_opt(SPL_DNS_SERVER, argv[1], &ipa);
    if (E_OK != ercd) {
        goto ftp_nocon_end;
    }

    /* FTP server connection settings */
    prm1 = (2 < argc) ? argv[2] : NULL;     /* username */
    prm2 = (3 < argc) ? argv[3] : NULL;     /* password */
    net_memset(&fc, 0, sizeof(fc));
    fc.ipa = ipa;
    fc.ctl_sid = ID_SOC_FTPC_CMD;
    fc.dat_sid = ID_SOC_FTPC_DATA;
    fc.ctl_port = SPL_FTP_CTL_PORT;
    fc.dat_port = SPL_FTP_DAT_PORT;
    ercd = ftp_login(&fc, prm1, prm2);
    if (E_OK != ercd) {
        goto ftp_nocon_end;
    }
    
    for (;;) {
        shell_puts(ctrl, SPL_LF"ftp> ");
        ercd = shell_gets(ctrl, tmp, sizeof(tmp));
        if (E_OK != ercd)   break;

        sc_argc = sizeof(sc_argv)/sizeof(sc_argv[0]);
        parse_cmd(tmp, &sc_argc, sc_argv);

        if (0 == net_strcmp(sc_argv[0], "?")) {         /* Command List */
            shell_puts(ctrl, SPL_LF);
            shell_puts(ctrl, " ?      Help (This command)"SPL_LF);
            shell_puts(ctrl, " cd     Change directory"SPL_LF);
            shell_puts(ctrl, " pwd    Print working directory"SPL_LF);
            shell_puts(ctrl, " rmd    Remove directory"SPL_LF);
            shell_puts(ctrl, " mkd    Create directory"SPL_LF);
            shell_puts(ctrl, " dir    List directory"SPL_LF);
            shell_puts(ctrl, " ls     List directory (Filename only)"SPL_LF);
            shell_puts(ctrl, " get    Get File"SPL_LF);
            shell_puts(ctrl, " put    Put File"SPL_LF);
            shell_puts(ctrl, " del    Remove File"SPL_LF);
            shell_puts(ctrl, " ren    Rename File"SPL_LF);
            shell_puts(ctrl, " act    Change ACTIVE mode"SPL_LF);
            shell_puts(ctrl, " psv    Change PASSIVE mode"SPL_LF);
            shell_puts(ctrl, " quit   Disconnect server"SPL_LF);
        }
        else if (0 == net_strcmp(sc_argv[0], "get")) {
            shell_puts(ctrl, SPL_LF);
            prm1 = (1 < sc_argc) ? sc_argv[1] : NULL;   /* local file */
            prm2 = (2 < sc_argc) ? sc_argv[2] : prm1;   /* remote file */
            if (prm1) {      /* fix file path */
                ercd = ftp_check_ffsysdrv(sc_argv[1], fnbuf, sizeof(fnbuf));
                prm1 = (0 < ercd) ? fnbuf : sc_argv[1];
            }
            ercd = ftp_get_file(&fc, prm1, prm2);
        }
        else if (0 == net_strcmp(sc_argv[0], "put")) {
            shell_puts(ctrl, SPL_LF);
            prm1 = (1 < sc_argc) ? sc_argv[1] : NULL;   /* local file */
            prm2 = (2 < sc_argc) ? sc_argv[2] : prm1;   /* remote file */
            if (prm1) {      /* fix file path */
                ercd = ftp_check_ffsysdrv(sc_argv[1], fnbuf, sizeof(fnbuf));
                prm1 = (0 < ercd) ? fnbuf : sc_argv[1];
            }
            ercd = ftp_put_file(&fc, prm1, prm2);
        }
        else if (0 == net_strcmp(sc_argv[0], "del")) {
            shell_puts(ctrl, SPL_LF);
            prm1 = (1 < sc_argc) ? sc_argv[1] : NULL;   /* remove file */
            ercd = ftp_del_file(&fc, prm1);
        }
        else if (0 == net_strcmp(sc_argv[0], "ren")) {
            shell_puts(ctrl, SPL_LF);
            prm1 = (1 < sc_argc) ? sc_argv[1] : NULL;   /* rename from */
            prm2 = (2 < sc_argc) ? sc_argv[2] : NULL;   /* rename to */
            ercd = ftp_ren_file(&fc, prm1, prm2);
        }
        else if (0 == net_strcmp(sc_argv[0], "cd")) {
            shell_puts(ctrl, SPL_LF);
            prm1 = (1 < sc_argc) ? sc_argv[1] : NULL;   /* change dir */
            ercd = ftp_cmd_cd(&fc, prm1);
        }
        else if (0 == net_strcmp(sc_argv[0], "rmd")) {
            shell_puts(ctrl, SPL_LF);
            prm1 = (1 < sc_argc) ? sc_argv[1] : NULL;   /* remove dir */
            ercd = ftp_cmd_rmd(&fc, prm1);
        }
        else if (0 == net_strcmp(sc_argv[0], "mkd")) {
            shell_puts(ctrl, SPL_LF);
            prm1 = (1 < sc_argc) ? sc_argv[1] : NULL;   /* make dir */
            ercd = ftp_cmd_mkd(&fc, prm1);
        }
        else if (0 == net_strcmp(sc_argv[0], "pwd")) {
            shell_puts(ctrl, SPL_LF);
            prm1 = tmp;                                 /* working dir */
            ercd = ftp_cmd_pwd(&fc, prm1);
            if (E_OK == ercd) {
                shell_puts(ctrl, prm1);
                shell_puts(ctrl, SPL_LF);
            }
        }
        else if (0 == net_strcmp(sc_argv[0], "ls")) {
            shell_puts(ctrl, SPL_LF);
            prm1 = (1 < sc_argc) ? sc_argv[1] : NULL;   /* reference dir */
            ercd = ftp_subcmd_list(ctrl, &fc, prm1, 1);
        }
        else if (0 == net_strcmp(sc_argv[0], "dir")) {
            shell_puts(ctrl, SPL_LF);
            prm1 = (1 < sc_argc) ? sc_argv[1] : NULL;   /* reference dir */
            ercd = ftp_subcmd_list(ctrl, &fc, prm1, 0);
        }
        else if (0 == net_strcmp(sc_argv[0], "act")) {
            shell_puts(ctrl, SPL_LF);
            fc.mode = (fc.mode & ~FTPC_MODE_APMASK) | FTPC_MODE_ACTIVE;
            ercd = E_OK;
        }
        else if (0 == net_strcmp(sc_argv[0], "psv")) {
            shell_puts(ctrl, SPL_LF);
            fc.mode = (fc.mode & ~FTPC_MODE_APMASK) | FTPC_MODE_PASSIVE;
            ercd = E_OK;
        }
        else if (0 == net_strcmp(sc_argv[0], "quit")) {
            shell_puts(ctrl, SPL_LF);
            goto ftp_quit_end;
        }

        if (E_OK != ercd) {
            net_strcpy(tmp, "Error(");
            net_strcat(tmp, net_itoa(ercd, fnbuf, 10));
            net_strcat(tmp, ")"SPL_LF);
            shell_puts(ctrl, tmp);
        }
    }

ftp_quit_end:
        ftp_quit(&fc);
    
ftp_nocon_end:
    exec_user = 0;
    return ercd;
}

#endif /* #if SAMPLE_ENA_FTPc */
