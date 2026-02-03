/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    FTP Client
    Copyright (c)  2008-2025, eForce Co., Ltd. All rights reserved.

    Version Information
      2008.12.28: Created
      2010.12.22: Modified to use socket ID of parameter
      2011.06.20: Added dly_tsk() in FtpTcpReadByte() for big
                  welcome message
      2011.07.07: Corrected the case which data port already close
                 at calling soc_rcv()
      2013.07.03: Stop the use of C standard library
                 (Use net_xxxxxx function instead.)
      2013.07.04: Corrected the sequence of file downloads
      2013.08.06: 1. Change the names of Following API.
                   FtpClient_Login     -> ftp_login
                   FtpClient_Quit      -> ftp_quit
                   FtpClient_PutFile   -> ftp_put_file
                   FtpClient_GetFile   -> ftp_get_file
                 2. Supports some commands, add an API.
                 3. Corrected modify the processing of active mode.
      2014.04.02: Change the names of some internal function.
      2014.07.10: Suppressed warning of the GCC compiler.
      2015.04.22: Corrected the following problems.
        1. Allow the timeout dynamic setting from user.
        2. Corrected better readability of some function.
        3. Reviewed the execution condition of ftp_quit().
        4. File successfully received had returned an errory by the timing.
        5. In some cases, the response of the command previously issued to retrieve.
      2015.05.20: Added the following features.
        1. Supports FTP over TLS/SSL. (uNet3/SSL required.)
        2. Added some API for FTP commands. (MKD, RMD, DELE, RNFR, RNTO, NOOP)
      2015.09.28: Fixed the problem of data transmission process.
      2015.12.14: The socket ID replaced SID types
      2016.08.08: Corrected the following problems.
        1. Execute static analysis tool to this source.
        2. Improved connectivity with some FTP servers.
      2017.08.07: Deleted the judged as network address at PASV response.
      2017.08.21: Support 64bit processor
      2018.04.23: Fixed data start processing in Active mode.
      2020.03.23: Suppressed warning of the 64bit GCC compiler.
      2020.12.06: Fixed problem that an error occurs in send size when using HTTPS.
      2022.04.22: Supported HTTPS connection using uNet3-TLS.
	  2025.08.04: Added con_ssoc() timeout definition FTPS_SSOC_TMO.
 ***************************************************************************/

#include "kernel.h"
#include "net_hdr.h"
#include "ftp_client.h"
#include "net_strlib.h"

#ifdef FTPC_SSL_SUP
#include "ssl_cfg.h"
#include "ssl_hdr.h"
#endif

#ifndef FTPS_SSOC_TMO
#define FTPS_SSOC_TMO       (2*1000)
#endif

#ifndef FTPC_USE_TSKS
#define FTPC_USE_TSKS   1
#endif

#ifndef FTPC_SSLPDU_LEN
#define FTPC_SSLPDU_LEN    1024U
#endif

/* Use Control Port Only */
#define SC_CWD      0U
#define SC_PWD      1U
#define SC_MKD      2U
#define SC_RMD      3U
#define SC_NOOP     4U
#define SC_DELE     5U
#define SC_RNFR     6U
#define SC_RNTO     7U
/* Use Control Port and Data Port */
#define SCD_RETR    8U
#define SCD_STOR    9U
#define SCD_LIST    10U     /* include "NLST" */

#define UH_MAX_VAL      0xFFFFU
#define UB_MAX_VAL      0xFFU

#define ACT_PNG_INTV    100U    /* ACTIVE mode ready monitoring interval */
#define ACTSET_CTL_RCV  0x01U
#define ACTSET_DAT_CON  0x02U
#define ACTSET_BOTH     (ACTSET_CTL_RCV | ACTSET_DAT_CON)


/* command compare macro */
#define CMD_STRNCMP(buf, cmd)   (0 == net_memcmp((buf), (cmd),sizeof((cmd))-1U))

/*------------------------------------------------------------*/
typedef struct t_asso_soc_tsk {
    ID tid;
    SID sid;
    ER ercd;
} T_ASSO_SOC_TSK;

static T_ASSO_SOC_TSK man_ast[FTPC_USE_TSKS * 2] = {0};

/* Get task ID that is associated with socket ID (Core) */
static T_ASSO_SOC_TSK *get_ast(SID sid)
{
    T_ASSO_SOC_TSK *ast = NULL;
    UW ret;
    
    for (ret = 0U; ret < (UW)(sizeof(man_ast)/sizeof(man_ast[0])); ++ret) {
        if (man_ast[ret].sid == sid) {
            ast = &man_ast[ret];
            break;
        }
    }
    return ast;
}

/* Get task ID that is associated with socket ID */
static ID ftp_get_tid(SID sid)
{
    T_ASSO_SOC_TSK *ast;
    
    ast = get_ast(sid);
    return (NULL != ast) ? ast->tid : -1 ;
}

/* Associate task ID into socket ID */
static ER set_ast(SID sid, ID tid)
{
    T_ASSO_SOC_TSK *ast;
    ER ercd;
    
    ast = get_ast(sid);
    if (NULL == ast) {
        ast = get_ast(0U);      /* New registration */
    }
    
    if (NULL != ast) {
        ast->sid = sid;
        ast->tid = tid;
        ast->ercd = E_WBLK;     /* initial value */
        ercd = E_OK;
    }
    else {
        /* Needs to be adjusted value of FTPC_USE_TSKS */
        ercd = E_NOMEM;
    }
    
    return ercd;
}

/* Get ErrorCode that is associated with socket ID */
static ID ftp_get_ercd(SID sid)
{
    T_ASSO_SOC_TSK *ast;
    
    ast = get_ast(sid);
    return (NULL != ast) ? ast->ercd : -1 ;
}

/* Associate ErrorCode into socket ID */
static ER set_ase(SID sid, ER ercd)
{
    T_ASSO_SOC_TSK *ast;
    
    ast = get_ast(sid);
    if (NULL == ast) {
        ast = get_ast(0U);      /* New registration */
    }
    
    if (NULL != ast) {
        ast->ercd = ercd;
        ercd = E_OK;
    }
    else {
        ercd = E_NOMEM;
    }
    
    return ercd;
}

/* Clear Associate task ID and socket ID */
static void clr_ast(SID sid)
{
    T_ASSO_SOC_TSK *ast;
    
    ast = get_ast(sid);
    if (NULL != ast) {
        net_memset(ast, 0, sizeof(*ast));
    }
}

/* FTP callback function */
static UW ftp_con_dat_done(SID sid, UH fncd, ER ercd)
{
    if (EV_SOC_CON == (fncd & EV_SOC_CON)) {
        (void)set_ase(sid, ercd);
    }
    return 0U;
}

static UW ftp_rcv_ctl_done(SID sid, UH fncd, ER ercd)
{
    if (EV_SOC_RCV == (fncd & EV_SOC_RCV)) {
        (void)set_ase(sid, ercd);
    }
    return 0U;
}

/* flush socket receive buffer */
static void ftp_rcv_flush(SID sid, UB *buf, UW len, UB ssl)
{
    TMO set_tmo;
    
    (void)ref_soc(sid, SOC_TMO_RCV, (VP)&set_tmo);
    (void)cfg_soc(sid, SOC_TMO_RCV, (VP)0);
    
    len = (UH)((len > UH_MAX_VAL) ? UH_MAX_VAL : len );

#ifdef FTPC_SSL_SUP
    if (0 < ssl) {
        while (0 < rcv_ssoc(sid, buf, (UH)len)) {
            ;
        }
    }
    else {
        while (0 < rcv_soc(sid, buf, (UH)len)) {
            ;
        }
    }
#else
    while (0 < rcv_soc(sid, buf, (UH)len)) {
        ;
    }
#endif
    
    (void)cfg_soc(sid, SOC_TMO_RCV, (VP)(ADDR)set_tmo);
}

/*------------------------------------------------------------*/

/* Receive data (LIST/NLST message) */
static ER rcv_list(T_FTP_CLIENT *ftp, T_FTP_LIST *list)
{
    ER ercd;
    UH cpos;    /* buffer current position */
    UH rpos;
    UH wpos;
    INT len;
    
    rpos = 0U;
    wpos = 0U;
    cpos = 0U;
    while (1) {
#ifdef FTPC_SSL_SUP
        if (ftp->flag & FTPC_FLG_SSL) {
            ercd = rcv_ssoc(ftp->dat_sid, ftp->Data, FTPC_DAT_BUF_MAX);
            if (E_OBJ == ercd)  ercd = 0;   /* data terminate */
        }
        else {
            ercd = rcv_soc(ftp->dat_sid, ftp->Data, FTPC_DAT_BUF_MAX);
        }
#else
        ercd = rcv_soc(ftp->dat_sid, ftp->Data, FTPC_DAT_BUF_MAX);
#endif
        if (ercd <= 0) {
            switch (ercd) {
            case E_OK:
            case E_CLS:
                ercd = (ER)wpos;    /* Number of bytes is read */
                break;
                
            default:
                break;
            }
            
            break;              /* Communication end */
        }
        
        /* Target buffer is empty */
        if (wpos >= list->len) {
            continue;
        }
        
        /* Copy of the target buffer */
        len = (INT)cpos + ercd;
        if ((cpos <= list->next_pos) && (list->next_pos < (UH)len)) {
            rpos = (UH)(list->next_pos - cpos);
            len = ercd - (INT)rpos;
            if (len > (INT)(list->len - wpos)) {
                len = (INT)(list->len - wpos);
            }
            net_memcpy(&list->buf[wpos], &ftp->Data[rpos], (UW)len);
            
            list->next_pos += (UH)len;
            wpos += (UH)len;
        }
        cpos += (UH)ercd;
    }
    
    return ercd;
}

static ER ftp_snd_dat(SID sid, UB *dat, UW len, UB ssl)
{
    ER ercd;
    UW i;
    UW slen;
    UW tot_len;

    tot_len = len;

    i = 0U;
    while (len > 0U) {
#ifdef FTPC_SSL_SUP
        if (ssl) {
            slen = (len > FTPC_SSLPDU_LEN) ? (UW)FTPC_SSLPDU_LEN : len;
            ercd = snd_ssoc(sid, dat + i, (UH)slen);
        }
        else {
            slen = (len > UH_MAX_VAL) ? (UW)UH_MAX_VAL : len;
            ercd = snd_soc(sid, dat + i, (UH)slen);
        }
#else
        slen = (len > UH_MAX_VAL) ? (UW)UH_MAX_VAL : len;
        ercd = snd_soc(sid, dat + i, (UH)slen);
#endif
        if (ercd <= 0) {
            break;
        }
        i   += (UW)ercd;
        len -= (UW)ercd;
    }


    return (i == tot_len) ? E_OK : ercd ;
}



/* Receive File */
static ER rcv_file_dat(T_FTP_CLIENT *ftp, FILE *fp)
{
    /* fopen, fclose is performed in the caller */
    UW flen;
    ER ercd;
    
    flen = 0U;
    ercd = E_OK;
    
    while (1) {
#ifdef FTPC_SSL_SUP
        if (ftp->flag & FTPC_FLG_SSL) {
            ercd = rcv_ssoc(ftp->dat_sid, ftp->Data, FTPC_DAT_BUF_MAX);
            if (E_OBJ == ercd)  ercd = 0;   /* data terminate */
        }
        else {
            ercd = rcv_soc(ftp->dat_sid, ftp->Data, FTPC_DAT_BUF_MAX);
        }
#else
        ercd = rcv_soc(ftp->dat_sid, ftp->Data, FTPC_DAT_BUF_MAX);
#endif
        if (ercd <= 0) {
            if (E_CLS == ercd) {
                ercd = E_OK;
            }
            break;
        }

        flen = (UW)fwrite(ftp->Data, 1U, (SIZE)ercd, fp);
        if (flen != (UW)ercd) {
            ercd = E_NOMEM;
            break;
        }
    }
    
    return ercd;
}

/* Send File */
static ER snd_file_dat(T_FTP_CLIENT *ftp, FILE *fp)
{
    /* fopen, fclose is performed in the caller */
    UW flen;
    ER ercd;
    
    while (1) {
        ercd = (ER)sizeof(ftp->Data);
        flen = (UW)fread(ftp->Data, 1U, (SIZE)ercd, fp);
        if (flen > 0U) {
            if (E_OK != ftp_snd_dat(ftp->dat_sid, ftp->Data, flen, (ftp->flag & FTPC_FLG_SSL))) {
                ercd = E_CLS;
                break;
            }
        }
        if (flen != (UW)ercd) {
            ercd = E_OK;
            break;
        }
    }
    
    return ercd;
}


static ER ftp_con(T_FTP_CLIENT *ftp)
{
    T_NODE host;
    SID sid;
    ER ercd;

    if (ftp->ctl_sid == 0U) {
        return E_PAR;
    }
    sid = ftp->ctl_sid;

    if (ftp->ctl_port == 0U) {
        ftp->ctl_port = FTPC_CTL_PORT_DEFAULT;
    }
    
    /* Set socket timeout */
    if (0U == ftp->cmd_tmo) {
        /* Get Timeout (Setting the value of Configurator) */
        ercd = ref_soc(sid, SOC_TMO_CON, (VP)&ftp->cmd_tmo);
        if (ercd != E_OK) {
            return ercd;
        }    
    }
    (void)cfg_soc(sid, SOC_TMO_CON, (VP)(ADDR)ftp->cmd_tmo);
    (void)cfg_soc(sid, SOC_TMO_CLS, (VP)(ADDR)ftp->cmd_tmo);
    (void)cfg_soc(sid, SOC_TMO_RCV, (VP)(ADDR)ftp->cmd_tmo);
    (void)cfg_soc(sid, SOC_TMO_SND, (VP)(ADDR)ftp->cmd_tmo);
    (void)cfg_soc(ftp->dat_sid, SOC_TMO_CON, (VP)(ADDR)ftp->cmd_tmo);
    (void)cfg_soc(ftp->dat_sid, SOC_TMO_CLS, (VP)(ADDR)ftp->cmd_tmo);
    (void)cfg_soc(ftp->dat_sid, SOC_TMO_RCV, (VP)(ADDR)ftp->cmd_tmo);
    (void)cfg_soc(ftp->dat_sid, SOC_TMO_SND, (VP)(ADDR)ftp->cmd_tmo);

    /* Connect */
    net_memset((VB *)&host, 0, sizeof(host));
    host.num  = (UB)ftp->dev_num;
    host.ipa  = ftp->ipa;
    host.port = ftp->ctl_port;
    host.ver  = IP_VER4;
    ercd = con_soc(sid, &host, SOC_CLI);
    if (ercd != E_OK) {
        return ercd;
    }

    ftp->flag |= FTPC_FLG_CTL;
    return ercd;
}

#ifdef FTPC_SSL_SUP
#ifdef SSL_SERVER_NODE  /* using uNet3-TLS */
static ER ftpc_con_ssoc(SID sid, T_NODE *host, ID ssnid, TMO tmo)
{
    T_SSL_NODE sn = {0};
    sn.node_type = SSL_CLIENT_NODE;
    sn.host = *host;
    return con_ssoc(sid, &sn, ssnid, tmo);
}
#else
#define ftpc_con_ssoc  con_ssoc
#endif

ER ftps_con(T_FTP_CLIENT *ftp)
{
    T_NODE host;
    SID sid;
    ER ercd;

    if (ftp->ctl_sid == 0) {
        return E_PAR;
    }
    sid = ftp->ctl_sid;

    if (ftp->ctl_port == 0) {
        ftp->ctl_port = FTPC_CTL_PORT_DEFAULT;
    }
    
    /* Get Remote host */
    ercd = ref_soc(sid, SOC_IP_REMOTE, (VP)&host);
    if (ercd != E_OK) {
        return ercd;
    }    
    
    if (host.ipa) {     /* connected */
    }
    else {
        /* Set socket timeout */
        if (0 == ftp->cmd_tmo) {
            /* Get Timeout (Setting the value of Configurator) */
            ercd = ref_soc(sid, SOC_TMO_CON, (VP)&ftp->cmd_tmo);
            if (ercd != E_OK) {
                return ercd;
            }    
        }
        (void)cfg_soc(sid, SOC_TMO_CON, (VP)(ADDR)ftp->cmd_tmo);
        (void)cfg_soc(sid, SOC_TMO_CLS, (VP)(ADDR)ftp->cmd_tmo);
        (void)cfg_soc(sid, SOC_TMO_RCV, (VP)(ADDR)ftp->cmd_tmo);
        (void)cfg_soc(sid, SOC_TMO_SND, (VP)(ADDR)ftp->cmd_tmo);
        (void)cfg_soc(ftp->dat_sid, SOC_TMO_CON, (VP)(ADDR)ftp->cmd_tmo);
        (void)cfg_soc(ftp->dat_sid, SOC_TMO_CLS, (VP)(ADDR)ftp->cmd_tmo);
        (void)cfg_soc(ftp->dat_sid, SOC_TMO_RCV, (VP)(ADDR)ftp->cmd_tmo);
        (void)cfg_soc(ftp->dat_sid, SOC_TMO_SND, (VP)(ADDR)ftp->cmd_tmo);

        /* Connect */
        net_memset((VB *)&host, 0, sizeof(host));
        host.num  = ftp->dev_num;
        host.ipa  = ftp->ipa;
        host.port = ftp->ctl_port;
        host.ver  = IP_VER4;
    }    
    
    ercd = ftpc_con_ssoc(sid, &host, 0, FTPS_SSOC_TMO);
    if (0 >= ercd) {
        return ercd;
    }
    ftp->ssl_sid = ercd;
    ftp->flag |= FTPC_FLG_CTL | FTPC_FLG_SSL;
    return E_OK;
}
#else
#define ftps_con(_fc_)  ((VP)0)
#endif

static void ftp_cls(T_FTP_CLIENT *ftp)
{
#ifdef FTPC_SSL_SUP
    if (ftp->flag & FTPC_FLG_SSL) {
        (void)cls_ssoc(ftp->ctl_sid);
    }
    else {
        (void)cls_soc(ftp->ctl_sid, SOC_TCP_CLS);
    }
#else
    (void)cls_soc(ftp->ctl_sid, SOC_TCP_CLS);
#endif
    ftp->flag = 0U;
}

/* Wait for response from server           */
/* Read the response to a temporary buffer */
/* Return one byte of data from the temporary buffer. */

static ER ftp_rd_ctl_byte(T_FTP_CLIENT *ftp, VB *c)
{
    ER ercd;

    if (ftp->rcv_len == 0U) {
#ifdef FTPC_SSL_SUP
        if (ftp->flag & FTPC_FLG_SSL) {
            ercd = rcv_ssoc(ftp->ctl_sid, ftp->rcv_buf, FTPC_DAT_BUF_MAX);
        }
        else {
            ercd = rcv_soc(ftp->ctl_sid, ftp->rcv_buf, FTPC_DAT_BUF_MAX);
        }
#else
        ercd = rcv_soc(ftp->ctl_sid, ftp->rcv_buf, FTPC_DAT_BUF_MAX);
#endif
        if (ercd <= 0) {
            return ercd;
        }
        ftp->rcv_len = (UH)ercd;
        ftp->rcv_id = 0U;
    }

    *c = (VB)ftp->rcv_buf[ftp->rcv_id];
    ftp->rcv_id++;
    ftp->rcv_len--;

    return 1;
}

/*
    Read data from TCP buffer until a complete line is read.
    A complete line is xxx text\r\n, where xxx are digits.
    Ignore the line with '-' next to xxx and read next line.
    Ignore extra bytes if the line is too big for the receive buffer.
*/

static ER ftp_rd_res(T_FTP_CLIENT *ftp)
{
    ER ercd;
    H len;
    B more;
    B ignore;
    VB c;

_ftp_res_start:

    len = 0;    /* Length of the line                   */
    more = 0;   /* Set, when need to read next line     */
    ignore = 0;  /* Set, when need to ignore extra bytes */
    c = '\0';

    while (1) {

        ercd = ftp_rd_ctl_byte(ftp, &c);
        if (ercd <= 0) {
            return ercd;
        }

        if (ignore == 0) {
            ftp->Res[len++] = (UB)c;
        }

        if (c == '\n') {

            if (0 != ignore) {
                ftp->Res[len] = '\n';
            }
            ftp->Res[len + 1] = '\0';

            if (len < 4) {
                more = 1;   /* Line is too short, Ignore and read next line */
            }

            if (ftp->Res[3] == '-') {
                more = 1;   /* Found '-', Ignore and read next line */
            }
            break;
        }
        else {
            if (len == (H)(FTPC_CTL_BUF_MAX - 1U)) { /* 1 byte for '\0' */
                ignore = 1;  /* Line is too big, trunc extra bytes */
            }
        }
    }

    if (0 != more) {
        goto _ftp_res_start;
    }

    return (len + 1);
}

static ER ftp_str_to_num(VB *str, UW *ip, UH *port)
{
    ER ercd;
    UW tmp;

    *ip = 0U;
    *port = 0U;
    ercd = E_PAR;
    do {
        tmp = (UW)net_atol((const char *)str);
        *ip += tmp << 24;
        str = net_strchr(str, ',');
        if (str == NULL) {
            break;
        }
        str++;

        tmp = (UW)net_atol((const char *)str);
        *ip += tmp << 16;
        str = net_strchr(str, ',');
        if (str == NULL) {
            break;
        }
        str++;

        tmp = (UW)net_atol((const char *)str);
        *ip += tmp << 8;
        str = net_strchr(str, ',');
        if (str == NULL) {
            break;
        }
        str++;

        tmp = (UW)net_atol((const char *)str);
        *ip += tmp;
        str = net_strchr(str, ',');
        if (str == NULL) {
            break;
        }
        str++;

        tmp = (UW)net_atol((const char *)str);
        *port += (UH)(tmp << 8);
        str = net_strchr(str, ',');
        if (str == NULL) {
            break;
        }
        str++;

        tmp = (UW)net_atol((const char *)str);
        *port += (UH)tmp;
        ercd = E_OK;
    } while (0);

    return ercd;
}


static ER ftp_con_dat(T_FTP_CLIENT *ftp)
{
    T_NODE host;
    SID sid;
    UW CmdLen;
    ER ercd;
    UH ip[4];
    UH p[2];
    UH remote_port;
    UW remote_ip;
    VB buf[8];
    VB *ptr;

    if (FTPC_FLG_DAT == (ftp->flag & FTPC_FLG_DAT)) {
        return E_OK;    /* Data connection already exists */
    }

    if (ftp->dat_sid == 0U) {
        return E_PAR;
    }
    sid = ftp->dat_sid;

    if ((ftp->mode & FTPC_MODE_APMASK) == FTPC_MODE_ACTIVE) {
        /* Set socket connection to non-blocking */
        ercd = cfg_soc(sid, SOC_CBK_HND, (VP)&ftp_con_dat_done);
        if (ercd != E_OK) {
            return ercd;
        }
        ercd = cfg_soc(sid, SOC_CBK_FLG, (VP)EV_SOC_CON);
        if (ercd != E_OK) {
            return ercd;
        }
        get_tid(&ercd);
        ercd = set_ast(sid, ercd);
        if (ercd != E_OK) {
            return ercd;
        }
    }
    else {
        ercd = cfg_soc(sid, SOC_CBK_HND, (VP)0);
        if (ercd != E_OK) {
            return ercd;
        }
        ercd = cfg_soc(sid, SOC_CBK_FLG, (VP)0);
        if (ercd != E_OK) {
            return ercd;
        }
    }

    /* 2. TYPE Command */
    /*    200 Command Okay */
    
    net_strcpy((VB *)ftp->Cmd, "TYPE ");
    CmdLen = net_strlen((const VB *)ftp->Cmd);
    ftp->Cmd[CmdLen++] = ftp->type;
    ftp->Cmd[CmdLen++] = '\r';
    ftp->Cmd[CmdLen++] = '\n';
    ercd = ftp_snd_dat(ftp->ctl_sid, ftp->Cmd, CmdLen, (ftp->flag & FTPC_FLG_SSL));
    if (ercd != E_OK) {
        return E_CLS;
    }

    ercd = ftp_rd_res(ftp);
    if ((ercd <= 0) || (!CMD_STRNCMP(ftp->Res,"200"))) {
        return E_CLS;
    }

    /* 3a. PORT */
    /*     200 Command Okay */
    if ((ftp->mode & FTPC_MODE_APMASK) == FTPC_MODE_ACTIVE) {
        /* Prepare to listen for connection : Non-Blocking */
        (void)cfg_soc(sid, SOC_PRT_LOCAL, (VP)PORT_ANY);  /* port change */

        net_memset((VB *)&host, 0, sizeof(host));
        host.num  = (UB)ftp->dev_num;
        host.ipa  = INADDR_ANY;
        host.port = ftp->dat_port;
        host.ver  = IP_VER4;
        ercd = con_soc(sid, &host, SOC_SER);
        if (ercd != E_WBLK) {
            return E_CLS;
        }

        /* Get IP address from the control socket ID */
        ercd = ref_soc(ftp->ctl_sid, SOC_IP_LOCAL, (VP)&host);
        if (ercd != E_OK) {
            return E_CLS;
        }
        /* Get port number from the data socket ID */
        ercd = ref_soc(sid, SOC_PRT_LOCAL, (VP)&host.port);
        if (ercd != E_OK) {
            return E_CLS;
        }

        ip[3]  = (host.ipa       ) & UB_MAX_VAL;
        ip[2]  = (host.ipa >>  8 ) & UB_MAX_VAL;
        ip[1]  = (host.ipa >> 16 ) & UB_MAX_VAL;
        ip[0]  = (host.ipa >> 24 ) & UB_MAX_VAL;

        p[1]  = (host.port       ) & UB_MAX_VAL;
        p[0]  = (host.port >>  8 ) & UB_MAX_VAL;

        /* Inform listening port to server */
        net_strcpy((VB *)ftp->Cmd, "PORT ");
        net_strcat((VB *)ftp->Cmd, net_itoa((INT)ip[0], buf, 10));
        net_strcat((VB *)ftp->Cmd, ",");
        net_strcat((VB *)ftp->Cmd, net_itoa((INT)ip[1], buf, 10));
        net_strcat((VB *)ftp->Cmd, ",");
        net_strcat((VB *)ftp->Cmd, net_itoa((INT)ip[2], buf, 10));
        net_strcat((VB *)ftp->Cmd, ",");
        net_strcat((VB *)ftp->Cmd, net_itoa((INT)ip[3], buf, 10));
        net_strcat((VB *)ftp->Cmd, ",");
        net_strcat((VB *)ftp->Cmd, net_itoa((INT)p[0], buf, 10));
        net_strcat((VB *)ftp->Cmd, ",");
        net_strcat((VB *)ftp->Cmd, net_itoa((INT)p[1], buf, 10));
        net_strcat((VB *)ftp->Cmd, "\r\n");
        CmdLen = net_strlen((const VB *)ftp->Cmd);
        ercd = ftp_snd_dat(ftp->ctl_sid, ftp->Cmd, CmdLen, (ftp->flag & FTPC_FLG_SSL));
        if (ercd != E_OK) {
            return E_CLS;
        }

        ercd = ftp_rd_res(ftp);
        if ((ercd <= 0) || (!CMD_STRNCMP(ftp->Res,"200"))) {
            return E_CLS;
        }

        /* Check for connection after issue command */
        return E_OK;
    }


    /* 2b. PASV */
    /*     227 Entering Passive Mode (h1,h2,h3,h4,p1,p2) */
    net_strcpy((VB *)ftp->Cmd, "PASV\r\n");
    CmdLen = net_strlen((const VB *)ftp->Cmd);
    ercd = ftp_snd_dat(ftp->ctl_sid, ftp->Cmd, CmdLen, (ftp->flag & FTPC_FLG_SSL));
    if (ercd != E_OK) {
        return E_CLS;
    }

    ercd = ftp_rd_res(ftp);
    if ((ercd <= 0) || (!CMD_STRNCMP(ftp->Res,"227"))) {
        return E_CLS;
    }

    /* Extract IP & Port info from response */
    ptr = net_strchr((VB *)ftp->Res, '(');
    if (ptr == NULL) {
        return E_CLS;
    }
    ptr++;
    ercd = ftp_str_to_num(ptr, &remote_ip, &remote_port);
    if (ercd != E_OK) {
        return E_CLS;
    }
    if ((remote_ip == 0U) || (remote_port == 0U)) {
        return E_CLS;
    }
    
    /* Establish connection */
    net_memset((VB *)&host, 0, sizeof(host));
    host.num  = (UB)ftp->dev_num;
    host.ipa  = remote_ip;
    host.port = remote_port;
    host.ver  = IP_VER4;
    ercd = con_soc(sid, &host, SOC_CLI);
    if (ercd != E_OK) {
        return E_CLS;
    }

    ftp->flag |= FTPC_FLG_DAT;

    return E_OK;
}

static ER ftp_ope_dat_cmd(T_FTP_CLIENT *ftp, INT argc, VP argv[])
{
    FILE *fp;
    ER ercd;
    ER recd;
    UW CmdLen;
    UW cmd;
    VB *prm;
#ifdef FTPC_SSL_SUP
    T_NODE remote;
#endif
    UB cbk_set;

    if (ftp == NULL) {
        return E_OBJ;
    }
    if (!(ftp->flag & FTPC_FLG_LOG)) {
        return E_CLS;
    }
    if ((ftp->ctl_sid == 0U) || (0 == argc)) {
        return E_PAR;
    }
    cmd = (UW)((ADDR)argv[0]);
    
    /* Open Local File */
    switch (cmd) {
    case SCD_STOR:
        fp = fopen(argv[1], (FTPC_TYPE_BINARY == ftp->type) ? "rb" : "r");
        if (fp == NULL) {
            return E_PAR;
        }
        break;
        
    case SCD_RETR:
        fp = fopen(argv[1], (FTPC_TYPE_BINARY == ftp->type) ? "wb" : "w");
        if (fp == NULL) {
            return E_PAR;
        }
        break;
        
    default:
        fp = NULL;
        break;
    }
    
    /* flush previous command and data response */
    ftp_rcv_flush(ftp->ctl_sid, ftp->Res, sizeof(ftp->Res), (ftp->flag & FTPC_FLG_SSL));
    ftp_rcv_flush(ftp->dat_sid, ftp->Res, sizeof(ftp->Res), (ftp->flag & FTPC_FLG_SSL));
    
    ercd = ftp_con_dat(ftp);
    if (ercd != E_OK) {
        goto _ftp_data_end;
    }

    /* Make Command Message */
    prm = NULL;
    switch (cmd) {
    case SCD_LIST:  /* argv ... [1]:VB*(param), [2]:T_FTP_LIST*(return) */
        prm = argv[1];
        net_strcpy((VB *)ftp->Cmd, 
            (0U == ((T_FTP_LIST *)argv[2])->nameonly) ? "LIST" : "NLST");
        break;
        
    case SCD_STOR:  /* argv ... [1]:VB*(lo_file), [2]:VB*(rmt_file) */
        prm = argv[2];
        net_strcpy((VB *)ftp->Cmd, "STOR");        
        break;
        
    case SCD_RETR:  /* argv ... [1]:VB*(lo_file), [2]:VB*(rmt_file) */
        prm = argv[2];
        net_strcpy((VB *)ftp->Cmd, "RETR");
        break;

    default:
        /* abnormal case (not enter) */
        break;
    }
    if (prm != NULL) {
        net_strcat((VB *)ftp->Cmd, " ");
        net_strcat((VB *)ftp->Cmd, prm);
    }
    net_strcat((VB *)ftp->Cmd, "\r\n");
    
    /* Send Command, and Receive Response */    
    CmdLen = net_strlen((const VB *)ftp->Cmd);
    ercd = ftp_snd_dat(ftp->ctl_sid, ftp->Cmd, CmdLen, (ftp->flag & FTPC_FLG_SSL));
    if (ercd != E_OK) {
        ercd = E_CLS;
        goto _ftp_data_end;
    }

    /* TCP connection might have established */
    /* else wait for connection              */
    if (!(ftp->flag & FTPC_FLG_DAT)) {
        ercd = E_OK;
        
        /* FTPC_MODE_ACTIVE */
        if (0 < ftp_get_tid(ftp->dat_sid)) {
            /* Set control socket receving to non-blocking */
            get_tid(&ercd);
            ercd = set_ast(ftp->ctl_sid, ercd);
            if (ercd != E_OK) {
                goto _ftp_data_end;
            }
            (void)cfg_soc(ftp->ctl_sid, SOC_CBK_HND, (VP)&ftp_rcv_ctl_done);
            (void)cfg_soc(ftp->ctl_sid, SOC_CBK_FLG, (VP)EV_SOC_RCV);
            
            ercd = ftp_rd_res(ftp);
            cbk_set = 0;
            if ((0 < ercd) && (ftp->Res[0] == '1')) {
                cbk_set = ACTSET_CTL_RCV;
            }
            else if (E_WBLK != ercd) {
                ercd = E_OBJ;       /* error response */
            }
            
            if (E_OBJ != ercd) {    /* E_WBLK or "150" */
                /* Wait for both events of receive control socket or connect data socket. */
                for (CmdLen = 0U; CmdLen <= ftp->cmd_tmo ; CmdLen += ACT_PNG_INTV) {
                    dly_tsk(ACT_PNG_INTV - 1U);
                    
                    if (0U == (cbk_set & ACTSET_CTL_RCV)) { /* control socket) */
                        ercd = ftp_get_ercd(ftp->ctl_sid);
                        if (E_WBLK != ercd) {
                            ercd = ftp_rd_res(ftp);
                            if ((0 < ercd) && (ftp->Res[0] == '1')) {
                                cbk_set |= ACTSET_CTL_RCV;
                            }
                            else {
                                break;
                            }
                        }
                    }
                    
                    if (0U == (cbk_set & ACTSET_DAT_CON)) { /* data socket */
                        recd = ftp_get_ercd(ftp->dat_sid);
                        if (E_WBLK != recd) {
                            if (E_OK == recd) {
                                cbk_set |= ACTSET_DAT_CON;
                            }
                            else {
                                ercd = recd;
                                break;
                            }
                        }
                    }
                    
                    if (cbk_set == ACTSET_BOTH) {           /* events complete */
                        break;
                    }
                }
            }
            
            if (cbk_set != ACTSET_BOTH) {
                ercd = E_TMOUT;
                (void)abt_soc(ftp->ctl_sid, SOC_ABT_RCV);       /* cancel ftp_rd_res */
            }
            /* Restore control socket receving to blocking */
            (void)cfg_soc(ftp->ctl_sid, SOC_CBK_HND, (VP)0);
            (void)cfg_soc(ftp->ctl_sid, SOC_CBK_FLG, (VP)0);
            clr_ast(ftp->ctl_sid);
        }
        clr_ast(ftp->dat_sid);
    }
    else {
        ercd = ftp_rd_res(ftp);
    }
    if ((ercd <= 0) || (ftp->Res[0] != '1')) {
        ercd = E_CLS;
        goto _ftp_data_end;
    }
    
/* _ftp_data_start: */
#ifdef FTPC_SSL_SUP
    if (ftp->flag & FTPC_FLG_SSL) {
        ercd = ref_soc(ftp->dat_sid, SOC_IP_REMOTE, (VP)&remote);
        if (E_OK != ercd) {
            ercd = E_CLS;
            goto _ftp_data_end;
        }
        
        ercd = ftpc_con_ssoc(ftp->dat_sid, &remote, ftp->ssl_sid, FTPS_SSOC_TMO);
        if (0 >= ercd) {
            ercd = E_CLS;
            goto _ftp_data_end;
        }
        ftp->flag |= FTPC_FLG_SSL_DAT;
    }
#endif    
    
    /* Receive Command Data */
    switch (cmd) {
    case SCD_LIST:
        ercd = rcv_list(ftp, (T_FTP_LIST *)argv[2]);
        break;
        
    case SCD_STOR:
        ercd = snd_file_dat(ftp, fp);
        break;
        
    case SCD_RETR:
        ercd = rcv_file_dat(ftp, fp);
        break;

    default:
        ercd = -1;  /* abnormal case (not enter) */
        break;
    }
    

_ftp_data_end:
    if (0 <= ercd) {
#ifdef FTPC_SSL_SUP
        if (ftp->flag & FTPC_FLG_SSL_DAT) {
            ftp->flag &= ~FTPC_FLG_SSL_DAT;
            (void)cls_ssoc(ftp->dat_sid);
        }
        else {
            (void)cls_soc(ftp->dat_sid, SOC_TCP_CLS);
        }
#else
        (void)cls_soc(ftp->dat_sid, SOC_TCP_CLS);
#endif
        
        /* 226,250 */
        recd = ftp_rd_res(ftp);
        if ((recd <= 0) || (ftp->Res[0] != '2')) {
            ercd = E_CLS;       /* FTP server not response or process error */
        }
    }
    else {
        /* Note: Some servers, does not return FINACK in the case of abnormal 
          response codes. Therefore, shorten temporarily timeout of cls_soc. */
        (void)cfg_soc(ftp->dat_sid, SOC_TMO_CLS, (VP)10);
#ifdef FTPC_SSL_SUP
        if (ftp->flag & FTPC_FLG_SSL) {
            (void)cls_ssoc(ftp->dat_sid);
        }
        else {
            (void)cls_soc(ftp->dat_sid, SOC_TCP_CLS);
        }
#else
        (void)cls_soc(ftp->dat_sid, SOC_TCP_CLS);
#endif
        (void)cfg_soc(ftp->dat_sid, SOC_TMO_CLS, (VP)(ADDR)ftp->cmd_tmo);
    }
    
    /* Close Local File */
    if (fp != NULL) {
        (void)fclose(fp);
    }

    ftp->flag &= ~FTPC_FLG_DAT;

    return ercd;
}

static ER ftp_ope_cmd(T_FTP_CLIENT *ftp, INT argc, VP argv[])
{
    ER ercd;
    UW CmdLen;
    UW cmd;
    VB *start;
    VB *end;
    VB *prm;
    VB rcd;
    
    if (ftp == NULL) {
        return E_OBJ;
    }
    if (!(ftp->flag & FTPC_FLG_LOG)) {
        return E_CLS;
    }
    if ((ftp->ctl_sid == 0U) || (0 == argc)) {
        return E_PAR;
    }
    
    /* Make Command Message */
    rcd = '2';
    prm = NULL;
    cmd = (UW)((ADDR)argv[0]);
    switch (cmd) {
    case SC_PWD:
        net_strcpy((VB *)ftp->Cmd, "PWD");
        break;
        
    case SC_CWD:    /* argv ... [1]:VB*(param) */
        prm = argv[1];
        net_strcpy((VB *)ftp->Cmd, "CWD");
        break;
        
    case SC_MKD:    /* argv ... [1]:VB*(param) */
        prm = argv[1];
        net_strcpy((VB *)ftp->Cmd, "MKD");
        break;
        
    case SC_RMD:    /* argv ... [1]:VB*(param) */
        prm = argv[1];
        net_strcpy((VB *)ftp->Cmd, "RMD");
        break;
        
    case SC_DELE:   /* argv ... [1]:VB*(param) */
        prm = argv[1];
        net_strcpy((VB *)ftp->Cmd, "DELE");
        break;
        
    case SC_RNFR:   /* argv ... [1]:VB*(param) */
        rcd = '3';
        prm = argv[1];
        net_strcpy((VB *)ftp->Cmd, "RNFR");
        break;
        
    case SC_RNTO:   /* argv ... [1]:VB*(param) */
        prm = argv[1];
        net_strcpy((VB *)ftp->Cmd, "RNTO");
        break;
        
    case SC_NOOP:
        net_strcpy((VB *)ftp->Cmd, "NOOP");
        break;
        
    default:
        return E_PAR;
    }
    if (prm != NULL) {
        net_strcat((VB *)ftp->Cmd, " ");
        net_strcat((VB *)ftp->Cmd, prm);
    }
    net_strcat((VB *)ftp->Cmd, "\r\n");
    
    /* flush previous command response */
    ftp_rcv_flush(ftp->ctl_sid, ftp->Res, sizeof(ftp->Res), (ftp->flag & FTPC_FLG_SSL));
    
    /* Send Command, and Receive Response */
    CmdLen = net_strlen((const VB *)ftp->Cmd);
    ercd = ftp_snd_dat(ftp->ctl_sid, ftp->Cmd, CmdLen, (ftp->flag & FTPC_FLG_SSL));
    if (ercd != E_OK) {
        ercd = E_CLS;
        goto _cmd_end;
    }
    ercd = ftp_rd_res(ftp);
    if ((ercd <= 0) || ((VB)ftp->Res[0] != rcd)) {
        ercd = E_CLS;
        goto _cmd_end;
    }
    
    /* Process Response Message */
    switch (cmd) {
    case SC_PWD:
        start = net_strchr((VB *)ftp->Res, '\"');
        ++start;
        end   = net_strchr(start, '\"');
        net_memcpy((VB *)argv[1], start, end - start);
        ((VB *)argv[1])[end - start] = '\0';
        break;
    
    default:
        break;
    }    

_cmd_end:
    return (0 < ercd) ? E_OK : ercd;
}

/* FTP server - login */
ER ftp_login(T_FTP_CLIENT *ftp, const VB *user, const VB *pw)
{
    const VB *anonymous = "anonymous";
    ER ercd;
    UW CmdLen;

    if (ftp == NULL) {
        return E_OBJ;
    }
    if (ftp->ipa == 0U) {
        return E_PAR;
    }
    if (ftp->type != FTPC_TYPE_ASCII) {
        ftp->type = FTPC_TYPE_BINARY;
    }
    if ((ftp->mode & FTPC_MODE_APMASK) != FTPC_MODE_ACTIVE) {
        ftp->mode = (ftp->mode & ~FTPC_MODE_APMASK) | FTPC_MODE_PASSIVE;
    }
    ftp->flag = 0U;

    /* 1. Establish TCP connection with Ftp Server */
#ifdef FTPC_SSL_SUP
    if ((ftp->mode & FTPC_MODE_SSLMASK) == (FTPC_MODE_SSL_USE | FTPC_MODE_SSL_IMP) ) {
        ercd = ftps_con(ftp);
    }
    else {
        ercd = ftp_con(ftp);
    }
#else
    ercd = ftp_con(ftp);
#endif
    if (ercd != E_OK) {
        return E_CLS;
    }

    /* 2. Wait for Server Ready Message  */
    /*    220 Service ready for new user */
    ercd = ftp_rd_res(ftp);
    if ((ercd <= 0) || (!CMD_STRNCMP(ftp->Res,"220"))) {
        ftp_cls(ftp);
        return E_CLS;
    }
    
#ifdef FTPC_SSL_SUP
    /* use FTP over SSL/TLS */
    if (ftp->mode & FTPC_MODE_SSL_USE) {
        /* FTPS Explicit Mode */
        if (0 == (ftp->mode & FTPC_MODE_SSL_IMP)) {
            /* AUTH Command */
            net_strcpy((VB *)ftp->Cmd, "AUTH TLS\r\n");
            CmdLen = net_strlen((const VB *)ftp->Cmd);
            ercd = ftp_snd_dat(ftp->ctl_sid, ftp->Cmd, CmdLen, (ftp->flag & FTPC_FLG_SSL));
            if (ercd != E_OK) {
                ftp_cls(ftp);
                return E_CLS;
            }
            ercd = ftp_rd_res(ftp);
            if ((ercd <= 0) || (!CMD_STRNCMP(ftp->Res,"234"))) {
                ftp_cls(ftp);
                return E_CLS;
            }
            
            ercd = ftps_con(ftp);   /* SSL connection */
            if (ercd != E_OK) {
                ftp_cls(ftp);
                return E_CLS;
            }
        }
            
        /* Verification of certificate */
        if (ftp->mode & FTPC_MODE_SSL_VFY) {
            ercd = get_sig_verify(ftp->ssl_sid);
            if (ercd != E_OK) {
                ftp_cls(ftp);
                return E_CLS;
            }
        }
    }
#endif
    
    /* 3. USER Command */
    /*    331 User name okay, need password */
    /*    230 User logged in, proceed       */
    net_strcpy((VB *)ftp->Cmd, "USER ");
    net_strcat((VB *)ftp->Cmd, (NULL != user) ? user : anonymous);
    net_strcat((VB *)ftp->Cmd, "\r\n");
    CmdLen = net_strlen((const VB *)ftp->Cmd);
    ercd = ftp_snd_dat(ftp->ctl_sid, ftp->Cmd, CmdLen, (ftp->flag & FTPC_FLG_SSL));
    if (ercd != E_OK) {
        ftp_cls(ftp);
        return E_CLS;
    }

    ercd = ftp_rd_res(ftp);
    if (ercd <= 0) {
        ftp_cls(ftp);
        return E_CLS;
    }

    if (CMD_STRNCMP(ftp->Res,"230")) {
        ftp->flag |= FTPC_FLG_LOG;
        return E_OK;
    }

    if (!CMD_STRNCMP(ftp->Res,"331")) {
        ftp_cls(ftp);
        return E_CLS;
    }

    /* 4. PASS Command */
    /*    230 User logged in, proceed       */
    
    net_strcpy((VB *)ftp->Cmd, "PASS ");
    net_strcat((VB *)ftp->Cmd, (pw != NULL) ? pw : anonymous);
    net_strcat((VB *)ftp->Cmd, "\r\n");
    CmdLen = net_strlen((const VB *)ftp->Cmd);
    ercd = ftp_snd_dat(ftp->ctl_sid, ftp->Cmd, CmdLen, (ftp->flag & FTPC_FLG_SSL));
    if (ercd != E_OK) {
        ftp_cls(ftp);
        return E_CLS;
    }

    ercd = ftp_rd_res(ftp);
    if ((ercd <= 0) || (!CMD_STRNCMP(ftp->Res,"230"))) {
        ftp_cls(ftp);
        return E_CLS;
    }
    
#ifdef FTPC_SSL_SUP
    /* use FTP over SSL/TLS */
    if (ftp->mode & FTPC_MODE_SSL_USE) {
        /* PBSZ Command */
        net_strcpy((VB *)ftp->Cmd, "PBSZ 0\r\n");
        CmdLen = net_strlen((const VB *)ftp->Cmd);
        ercd = ftp_snd_dat(ftp->ctl_sid, ftp->Cmd, CmdLen, (ftp->flag & FTPC_FLG_SSL));
        if (ercd != E_OK) {
            ftp_cls(ftp);
            return E_CLS;
        }
        ercd = ftp_rd_res(ftp);
        if ((ercd <= 0) || (!CMD_STRNCMP(ftp->Res,"200"))) {
            ftp_cls(ftp);
            return E_CLS;
        }
        
        /* PROT Command */
        net_strcpy((VB *)ftp->Cmd, "PROT P\r\n");
        CmdLen = net_strlen((const VB *)ftp->Cmd);
        ercd = ftp_snd_dat(ftp->ctl_sid, ftp->Cmd, CmdLen, (ftp->flag & FTPC_FLG_SSL));
        if (ercd != E_OK) {
            ftp_cls(ftp);
            return E_CLS;
        }
        ercd = ftp_rd_res(ftp);
        if ((ercd <= 0) || (!CMD_STRNCMP(ftp->Res,"200"))) {
            ftp_cls(ftp);
            return E_CLS;
        }
    }
#endif       

    ftp->flag |= FTPC_FLG_LOG;

    return E_OK;
}

/* FTP server - disconnect */
ER ftp_quit(T_FTP_CLIENT *ftp)
{
    ER ercd;
    UW CmdLen;

    if (ftp == NULL) {
        ercd = E_OBJ;
    }
    else {
        /* 1. QUIT Command */
        /*    221 Service closing control connection */

        net_strcpy((VB *)ftp->Cmd, "QUIT\r\n");
        CmdLen = net_strlen((const VB *)ftp->Cmd);
        ercd = ftp_snd_dat(ftp->ctl_sid, ftp->Cmd, CmdLen, (ftp->flag & FTPC_FLG_SSL));
        if (ercd == E_OK) {
            (void)ftp_rd_res(ftp);
        }
        ftp_cls(ftp);
        ftp->flag = 0U;

        ercd = E_OK;
    }

    return ercd;
}

/* RETR: Get File */
ER ftp_get_file(T_FTP_CLIENT *ftp, const VB *lo_file, const VB *rmt_file)
{
    ER ercd;
    VP argv[3];
    
    if ((lo_file == NULL) || (rmt_file == NULL)) {
        ercd = E_PAR;
    }
    else {
        argv[0] = (VP)SCD_RETR;
        argv[1] = (VP)lo_file;
        argv[2] = (VP)rmt_file;
        ercd = ftp_ope_dat_cmd(ftp, sizeof(argv)/sizeof(VB*), argv);
    }
    
    return ercd;
}

/* STOR: Put File */
ER ftp_put_file(T_FTP_CLIENT *ftp, const VB *lo_file, const VB *rmt_file)
{
    ER ercd;
    VP argv[3];
    
    if ((lo_file == NULL) || (rmt_file == NULL)) {
        ercd = E_PAR;
    }
    else {
        argv[0] = (VP)SCD_STOR;
        argv[1] = (VP)lo_file;
        argv[2] = (VP)rmt_file;
        ercd = ftp_ope_dat_cmd(ftp, sizeof(argv)/sizeof(VB*), argv);
    }
    
    return ercd;
}

/* DELE: Delete File */
ER ftp_del_file(T_FTP_CLIENT *ftp, const VB *rmt_file)
{
    ER ercd;
    VP argv[2];
    
    if (rmt_file == NULL) {
        ercd = E_PAR;
    }
    else {
        argv[0] = (VP)SC_DELE;
        argv[1] = (VP)rmt_file;
        ercd = ftp_ope_cmd(ftp, sizeof(argv)/sizeof(VB*), argv);
    }
    
    return ercd;
}

/* RNFR, RNTO: Rename File */
ER ftp_ren_file(T_FTP_CLIENT *ftp, const VB *org_name, const VB *ren_name)
{
    ER ercd;
    VP argv[2];
    
    if ((org_name == NULL) || (ren_name == NULL)) {
        ercd = E_PAR;
    }
    else {
        argv[0] = (VP)SC_RNFR;
        argv[1] = (VP)org_name;
        ercd = ftp_ope_cmd(ftp, sizeof(argv)/sizeof(VB*), argv);

        if (E_OK == ercd) {
            argv[0] = (VP)SC_RNTO;
            argv[1] = (VP)ren_name;
            ercd = ftp_ope_cmd(ftp, sizeof(argv)/sizeof(VB*), argv);
        }
    }
    
    return ercd;
}

/* CWD: Change Working Directory */
ER ftp_cmd_cd(T_FTP_CLIENT *ftp, const VB *dir)
{
    ER ercd;
    VP argv[2];
    
    if (dir == NULL) {
        ercd = E_PAR;
    }
    else {
        argv[0] = (VP)SC_CWD;
        argv[1] = (VP)dir;
        ercd = ftp_ope_cmd(ftp, sizeof(argv)/sizeof(VB*), argv);
    }

    return ercd;
}

/* PWD: Printt Working Directory */
ER ftp_cmd_pwd(T_FTP_CLIENT *ftp, VB *dir)
{
    ER ercd;
    VP argv[2];
    
    if (dir == NULL) {
        ercd = E_PAR;
    }
    else {
        argv[0] = (VP)SC_PWD;
        argv[1] = (VP)dir;
        ercd = ftp_ope_cmd(ftp, sizeof(argv)/sizeof(VB*), argv);
    }

    return ercd;
}

/* MKD: Make Directory */
ER ftp_cmd_mkd(T_FTP_CLIENT *ftp, const VB *dir)
{
    ER ercd;
    VP argv[2];
    
    if (dir == NULL) {
        ercd = E_PAR;
    }
    else {
        argv[0] = (VP)SC_MKD;
        argv[1] = (VP)dir;
        ercd = ftp_ope_cmd(ftp, sizeof(argv)/sizeof(VB*), argv);
    }
    
    return ercd;
}

/* RMD: Remove Directory */
ER ftp_cmd_rmd(T_FTP_CLIENT *ftp, const VB *dir)
{
    ER ercd;
    VP argv[2];
    
    if (dir == NULL) {
        ercd = E_PAR;
    }
    else {
        argv[0] = (VP)SC_RMD;
        argv[1] = (VP)dir;
        ercd = ftp_ope_cmd(ftp, sizeof(argv)/sizeof(VB*), argv);
    }
    
    return ercd;
}

/* NOOP: No Operation */
ER ftp_cmd_noop(T_FTP_CLIENT *ftp)
{
    VP argv[1];
    
    argv[0] = (VP)SC_NOOP;
    
    return ftp_ope_cmd(ftp, sizeof(argv)/sizeof(VB*), argv);
}

/* LIST,NLST: Show the file list */
ER ftp_cmd_list(T_FTP_CLIENT *ftp, const VB *dir, T_FTP_LIST *list)
{
    ER ercd;
    VP argv[3];
    
    if (list == NULL) {
        ercd = E_PAR;
    }
    else if ((list->len == 0U) || (list->buf == NULL)) {
        ercd = E_PAR;
    }
    else {
        list->next_pos = 0U;
        list->dat[0] = (VP)ftp;
        list->dat[1] = (VP)dir;
        argv[0] = (VP)SCD_LIST;
        argv[1] = (VP)dir;
        argv[2] = (VP)list;
        ercd = ftp_ope_dat_cmd(ftp, sizeof(argv)/sizeof(VB*), argv);
    }
    
    return ercd;
}

/* LIST,NLST: Show Next file list */
ER ftp_cmd_list_next(T_FTP_LIST *list)
{
    ER ercd;
    VP argv[3];
    
    if (list == NULL) {
        ercd = E_PAR;
    }
    else if ((list->len == 0U) || (list->buf == NULL)) {
        ercd = E_PAR;
    }
    else {
        argv[0] = (VP)SCD_LIST;
        argv[1] = (VP)list->dat[1];
        argv[2] = (VP)list;
        ercd = ftp_ope_dat_cmd((T_FTP_CLIENT *)list->dat[0], sizeof(argv)/sizeof(VB*), argv);
    }
    
    return ercd;
}

