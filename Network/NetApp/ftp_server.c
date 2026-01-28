/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    FTP Server
    Copyright (c) 2014-2025, eForce Co., Ltd. All rights reserved.

    Version Information
      2014.03.18: Created
      2014.07.10: Suppressed warning of the GCC compiler.
      2014.12.24: Stop a reference to global variable gNET_ADR.
      2015.01.22: Fixed. Lose a part of big file while downloading.
      2015.01.22: Unnecessary closedir call was removed.
      2015.01.22: CD command was fixed.
      2015.01.23: LIST command was fixed.
      2015.02.03: Add an API of ftp_server_stop
      2015.03.10: Bugfix. The overflow for ftp->cmd.
                  Bugfix. LIST command.
                  Add ftp response "500 The specified path is too long."
      2015.03.31: Add security policy for PORT command.
      2015.04.02: File system ver.2.5 was supported.
      2015.06.19: Bugfix. LIST command. (when the simple file system use)
      2015.12.14: The socket ID replaced SID types.
                  Variable initialization for build warning.
      2016.01.13: Changes to output information of the file transfer error.
      2016.02.29: Modified to use to listen port number of target socket.
      2016.03.08: Changes to Explicitly specify the open mode of fopen.
                  Modified to tightened the error handling.
      2016.03.18: Update for H/W OS
      2016.04.06: Support of rename operation. (RNFR, RNTO command)
                  Change the user authentication process to callback.
      2016.06.27: Change the buffer size for ftps_res_buf and ftps_path_buf.
      2016.07.07: Suppressed warning of Zynq GCC compiler.
      2016.07.25: Corrected the following problems.
        1. Execute static analysis tool to this source.
        2. Improve the analysis accuracy of the FTP command.
        3. Improve the problem that path for buffer overflows.
        4. Improve the operation of the some commands.
        5. Improve the problem of command to fail by the file name.
      2017.05.11: Remove definition CFG_FTPS_NET_SOC_MAX.
      2017.05.23: Improved the number of elements of array ftps_svr_ctl[].
      2017.07.27: Support 64bit processor
      2018.02.28: Support SYST command.
      2019.01.22: Changed the port number of PASV command to be configurable.
      2022.02.14: Improve the operation of commands. (LIST,NLST,STOR,RETR)
      2023.08.25: Suppressed warning of e2studio compiler.
      2025.04.16: Fixed for LIST error when readdir() returns DT_VOL_ID attribute.
 ***************************************************************************/

#include "kernel.h"
#include "net_hdr.h"
#include "net_cfg.h"
#include "net_strlib.h"
#include "ftp_server.h"

extern const T_FTP_USR_TBL ftp_usr_tbl[];

/* ftp server process node list */
static T_FTP_SERVER *gFTP_SERVER_STACK = NULL;

/* Default configuration */
#define FTPS_DRV_NAME        CFG_FTPS_DRV_NAME
#define FTPS_CMD_TMO         CFG_FTPS_CMD_TMO
#define FTPS_DAT_TMO         CFG_FTPS_DAT_TMO
#define FTPS_IDLE_TMO        CFG_FTPS_IDLE_TMO
#define FTPS_DAT_BUF_MAX     CFG_FTPS_DAT_BUF_MAX
#define FTPS_CTL_BUF_MAX     CFG_FTPS_CTL_BUF_MAX
#define FTPS_NET_SOC_MAX     (CFG_NET_SOC_MAX + 1)
#define FTPS_PATH_MAX        CFG_FTPS_PATH_MAX
#define FTPS_SES_NUM         CFG_FTPS_SES_NUM

/* Maximum number of users */
#if !defined(CFG_FTPS_MAX_USR_CNT)
#define FTPS_MAX_USR_CNT     256
#else
#define FTPS_MAX_USR_CNT     CFG_FTPS_MAX_USR_CNT
#endif

/* Mode and Type */
#define FTP_MOD_NONE        0U
#define FTP_MOD_ACTIVE      1U
#define FTP_MOD_PASSIVE     2U
#define FTP_TYP_BINARY      'I'
#define FTP_TYP_ASCII       'A'

/* Port number */
#define FTP_ACV_PORT    20U
#define FTP_PSV_PORT    42236U

/* FTP Commands */
#define FTP_CMD_NONE    0U
#define FTP_CMD_USER    1U
#define FTP_CMD_PASS    2U
#define FTP_CMD_CWD     3U      /* include XCWD, XCUP, CDUP */
#define FTP_CMD_QUIT    4U
#define FTP_CMD_PORT    5U
#define FTP_CMD_PASV    6U
#define FTP_CMD_TYPE    7U
#define FTP_CMD_RETR    8U
#define FTP_CMD_STOR    9U
#define FTP_CMD_RNFR    10U
#define FTP_CMD_RNTO    11U
#define FTP_CMD_DELE    12U
#define FTP_CMD_RMD     13U     /* include XRMD */
#define FTP_CMD_MKD     14U     /* include XMKD */
#define FTP_CMD_PWD     15U     /* include XPWD */
#define FTP_CMD_LIST    16U
#define FTP_CMD_NLST    17U
#define FTP_CMD_EPSV    18U
#define FTP_CMD_SIZE    19U
#define FTP_CMD_SYST    20U
#define FTP_CMD_REST    40U     /* not implemented */
#define FTP_CMD_ABOR    41U     /* not implemented */
#define FTP_CMD_ERR     50U     /* command error */


/* Status flag */
#define STS_CLI_CON     0x01U   /* Client Connected      */
#define STS_USER        0x02U   /* USER Command received */
#define STS_LOGIN       0x04U   /* Loggin Successful     */
#define STS_DAT_CON     0x08U   /* Data Socket Connected */
#define STS_DAT_DONE    0x10U   /* Data Socket Callback  */
#define STS_CMD_RNFR    0x80U   /* execute RNFR command */

/* Response Code */
#define RCD_CRLF    "\r\n"

#define RCD_150_1   "150 File Status okay. Opening data connection." RCD_CRLF
#define RCD_150_2   "150 Opening data connection." RCD_CRLF
#define RCD_200_1   "200 Command Successful." RCD_CRLF
#define RCD_200_2   "200 TYPE set." RCD_CRLF
#define RCD_213A    "213 "
#define RCD_215_1A  "215 "
#define RCD_215_2   "215 UNIX Type: L8" RCD_CRLF
#define RCD_220     "220 FTP Server Ready." RCD_CRLF
#define RCD_221     "221 Logged out." RCD_CRLF
#define RCD_226_1   "226 Closing data connection." RCD_CRLF
#define RCD_226_2   "226 File reception completed." RCD_CRLF
#define RCD_226_3A  "226 File transmission completed. ["
#define RCD_227A    "227 Entering Passive Mode ("
#define RCD_229A    "229 Entering Extended Passive Mode (|||"
#define RCD_230     "230 Login Successful." RCD_CRLF
#define RCD_250_1A  "250 "
#define RCD_250_2   "250 Directory removed." RCD_CRLF
#define RCD_250_3   "250 File removed." RCD_CRLF
#define RCD_250_4   "250 Requested file rename okay, completed." RCD_CRLF
#define RCD_257_1A  "257 "
#define RCD_257_2   "257 Directory created." RCD_CRLF
#define RCD_331     "331 Please specify the password." RCD_CRLF
#define RCD_350     "350 Requested file rename pending further information." RCD_CRLF
#define RCD_421     "421 Service not available, closing control connection." RCD_CRLF
#define RCD_425_1   "425 Can't open data connection." RCD_CRLF
#define RCD_425_2   "425 Use PORT or PASV first." RCD_CRLF
#define RCD_426A    "426 Connection closed; transfer aborted"
#define RCD_450_1   "450 File not found." RCD_CRLF
#define RCD_450_2A  "450 Requested file action not taken"
#define RCD_451A    "451 Requested action aborted. Local error in processing"
#define RCD_500_1   "500 Command unrecognized." RCD_CRLF
#define RCD_500_2   "500 Could not enter Extended Passive mode." RCD_CRLF
#define RCD_500_3   "500 Could not enter Passive mode." RCD_CRLF
#define RCD_500_4   "500 Syntax error." RCD_CRLF
#define RCD_500_5   "500 Syntax error. Invalid IP or Port." RCD_CRLF
#define RCD_500_6   "500 The specified path is too long." RCD_CRLF
#define RCD_501_1   "501 Directory name not specified." RCD_CRLF
#define RCD_501_2   "501 File name not specified." RCD_CRLF
#define RCD_501_3   "501 Command requires a parameter." RCD_CRLF
#define RCD_503     "503 Bad sequence of commands." RCD_CRLF
#define RCD_504     "504 Command not implemented for that parameter." RCD_CRLF
#define RCD_530_1   "530 User already logged in." RCD_CRLF
#define RCD_530_2   "530 User not logged in." RCD_CRLF
#define RCD_530_3   "530 User or password not correct." RCD_CRLF
#define RCD_550_1   "550 Could not get file size." RCD_CRLF
#define RCD_550_2   "550 Create directory action not taken." RCD_CRLF
#define RCD_550_3   "550 Failed to change directory." RCD_CRLF
#define RCD_550_4   "550 Permission denied." RCD_CRLF
#define RCD_550_5   "550 Remove directory action not taken." RCD_CRLF
#define RCD_550_6   "550 Remove file action not taken." RCD_CRLF
#define RCD_550_7   "550 File not found." RCD_CRLF
#define RCD_550_8A  "550 Requested file action not taken"
#define RCD_553     "553 Requested rename not taken." RCD_CRLF

/* Note: Changed the response code when the file API fails from 450 to 550. */
#if defined(CFG_FTPS_FILEIO_ERR_550)
#define RCD_FILEIO_ERR_1    RCD_550_7
#define RCD_FILEIO_ERR_2A   RCD_550_8A
#else
#define RCD_FILEIO_ERR_1    RCD_450_1
#define RCD_FILEIO_ERR_2A   RCD_450_2A
#endif

/* Other defines */
#define LEN_DRV_LTR         3U
#define LEN_RESPONSE_BUF    512U

#define DECIMAL_MAX         10
#define DECIMAL_DIGITS      10

#define WELLKNOWN_PORT_END  1023U

#define FTP_CMDLEN_MIN      3U
#define FTP_CMDLEN_MAX      4U

#define UH_MAX_VAL      0xFFFFU
#define UB_MAX_VAL      0xFFU

/* command compare macro */
#define CMD_STRCMP(buf, cmd)   (0 == net_strcmp((buf), (cmd)))

#define IS_LOWER(c)   (('a' <= (c)) && ((c) <= 'z'))
#define TO_UPPER(c)   (IS_LOWER(c) ? ((c) - (VB)0x20) : (c))

/* Variables */
T_FTP_SERVER* ftps_svr_ctl[FTPS_SES_NUM] = {0};   /* number of session */
static VB ftps_res_buf[LEN_RESPONSE_BUF];
static VB ftps_path_buf[FTPS_PATH_MAX];

/* Get ftp_server control */
static T_FTP_SERVER *get_svrctl(SID ctl_sid, SID dat_sid)
{
    T_FTP_SERVER *ftp = NULL;
    UB cnt;

    for (cnt = 0U; cnt < FTPS_SES_NUM; ++cnt) {
        if (ftps_svr_ctl[cnt] == NULL) {
            continue;
        }
        if (ftps_svr_ctl[cnt]->ctl_sid == ctl_sid) {
            if ((0 == dat_sid) || (ftps_svr_ctl[cnt]->dat_sid == dat_sid)) {
                ftp = ftps_svr_ctl[cnt];
            break;
        }
            else {
                continue;
            }
        }
        else if ((0 == ctl_sid) && (ftps_svr_ctl[cnt]->dat_sid == dat_sid)) {
            ftp = ftps_svr_ctl[cnt];
            break;
        }
    }
    return ftp;
}

/* Associate ftp_server control */
static ER set_svrctl(T_FTP_SERVER *ftp)
{
    T_FTP_SERVER *reg;
    UB cnt;

    reg = get_svrctl(ftp->ctl_sid, ftp->dat_sid);
    if (NULL == reg) {
        for (cnt = 0U; cnt < FTPS_SES_NUM; ++cnt) {
            if (ftps_svr_ctl[cnt] == NULL) {
                reg = ftp;
                ftps_svr_ctl[cnt] = reg;
                break;
            }
        }
    }

    return (NULL != reg) ? E_OK : E_NOMEM ;
}

/* Clear ftp_server control */
static void clr_svrctl(T_FTP_SERVER *ftp)
{
    UB cnt;

    for (cnt = 0U; cnt < FTPS_SES_NUM; ++cnt) {
        if (ftps_svr_ctl[cnt] == ftp) {
            ftps_svr_ctl[cnt] = NULL;
            break;
        }
    }
}

static VB* ftps_cat_dig(VB* buf, UW num, W width, VB fill)
{
    VB* buf_ptr;
    W i;
    W len;
    VB c[DECIMAL_DIGITS];

    /* Put digits of a number */

    buf_ptr = buf;

    for (i = 0; i < (W)(sizeof(c) / sizeof(c[0])); i++) {
        c[i] = (VB)('0' + (num % (UW)DECIMAL_MAX));
        num = num / (UW)DECIMAL_MAX;
        if (num == 0U) {
            break;
        }
    }
    len = (W)(i + 1);

    /* Destination pointer */
    for (i = 0; i < (W)FTPS_DAT_BUF_MAX; i++) {
        if (*buf == '\0') {
            break;
        }
        buf++;
    }
    if (i == (W)FTPS_DAT_BUF_MAX) {
        return NULL;
    }

    /* Filled space or zero */
    if (len < width) {
        width -= len;
        for (i = 0; i < width; i++) {
            *buf++ = fill;
        }
    }

    for (i = 0; i < len; i++) {
        *buf++ = c[len - i - 1];
    }
    *buf = '\0';

    return buf_ptr;
}

static UW ftps_svr_dat_done(SID sid, UH fncd, ER ercd)
{
    T_FTP_SERVER *ftp;

    if ((UH)EV_SOC_CON == (fncd & (UH)EV_SOC_CON)) {
        ftp = get_svrctl(0, sid);
        if (ftp != NULL) {
            ftp->sts_flg |= STS_DAT_DONE;
            ftp->wai_err = ercd;
            if (ercd == E_OK) {
                ftp->sts_flg |= STS_DAT_CON;
            }
            if (0U < ftp->wai_flg) {
                ftp->wai_flg = 0U;
                wup_tsk(ftp->wai_tsk_id);
            }
        }
    }

    return (UW)E_OK;
}

static ER ftps_svr_dat_cre(T_FTP_SERVER *ftp, UH port)
{
    ER ercd;
    VP dat;

    do {
        /* Prepare Socket */

        /* Local Port */
        ercd = cfg_soc(ftp->dat_sid, SOC_PRT_LOCAL, (VP)(ADDR)port);
        if (ercd != E_OK) {
            break;
        }
        /* Set Connection Timeout */
        ercd = cfg_soc(ftp->dat_sid, SOC_TMO_CON, (VP)FTPS_DAT_TMO);
        if (ercd != E_OK) {
            break;
        }
        /* Set Transmission Timeout */
        ercd = cfg_soc(ftp->dat_sid, SOC_TMO_SND, (VP)FTPS_DAT_TMO);
        if (ercd != E_OK) {
            break;
        }
        /* Set Reception Timeout */
        ercd = cfg_soc(ftp->dat_sid, SOC_TMO_RCV, (VP)FTPS_DAT_TMO);
        if (ercd != E_OK) {
            break;
        }
        /* Set Close Timeout */
        ercd = cfg_soc(ftp->dat_sid, SOC_TMO_CLS, (VP)FTPS_DAT_TMO);
        if (ercd != E_OK) {
            break;
        }
        /* Set socket connection to non-blocking */
        dat = (ftp->mod != FTP_MOD_PASSIVE) ? (VP)0 : (VP)&ftps_svr_dat_done;
        ercd = cfg_soc(ftp->dat_sid, SOC_CBK_HND, dat);
        if (ercd != E_OK) {
            break;
        }
        dat = (ftp->mod != FTP_MOD_PASSIVE) ? (VP)0 : (VP)EV_SOC_CON;
        ercd = cfg_soc(ftp->dat_sid, SOC_CBK_FLG, dat);
        if (ercd != E_OK) {
            break;
        }

        ftp->wai_flg = 0U;
        ercd = E_OK;
    } while (0);

    return ercd;
}

static ER ftps_calc_pasv_port(T_FTP_SERVER *ftp)
{
    ER erpt;
    ER ercd;
    ER port;

    /* Selecting PASV port number */
    ercd = E_OBJ;
    erpt = 0;
    if ((0U != ftp->psv_port_min) && (ftp->psv_port_min <= ftp->psv_port_max)) {
        erpt = (ER)(ftp->psv_port_max - ftp->psv_port_min + 1U);
        erpt = (net_rand() % erpt) + ftp->psv_port_min;
    }

    if (0 == erpt) {
        /* If the value of psv_port_min(max) is invalid, use FTP_PSV_PORT value. */
        ercd = E_OK;
        port = FTP_PSV_PORT;
    }
    else {
        /* Local Port */
        port = erpt;
        do {
            ercd = cfg_soc(ftp->dat_sid, SOC_PRT_LOCAL, (VP)(ADDR)port);
            if (ercd == E_OBJ) {
                /* In the case of E_OBJ, the specified port may be in use. */
                ++port;
                if (port > (ER)ftp->psv_port_max) {
                    port = (ER)ftp->psv_port_min;
                }
                if (port == erpt) {
                    break;    /* no available ports */
                }
            }
        } while (ercd == E_OBJ);
    }

    return (ercd == E_OK) ? port : ercd;
}

static ER ftps_svr_dat_listen(T_FTP_SERVER *ftp)
{
    T_NODE host;
    ER ercd;

    /* Selecting PASV port number */
    ercd = ftps_calc_pasv_port(ftp);
    if (0 > ercd) {
        return ercd;
    }
    ftp->cli_port = (UH)ercd;

    /* Prepare Socket */
    ercd = ftps_svr_dat_cre(ftp, ftp->cli_port);
    if (ercd != E_OK) {
        return ercd;
    }

    can_wup(TSK_SELF);
    ftp->wai_flg = 0U;
    ftp->sts_flg &= ~STS_DAT_DONE;

    net_memset((char *)&host, 0, sizeof(host));
    host.num  = (UB)ftp->cli_dev_num;
    host.ipa  = ftp->cli_adr;
    host.port = ftp->cli_port;
    host.ver  = IP_VER4;
    ercd = con_soc(ftp->dat_sid, &host, SOC_SER);
    if (ercd != E_WBLK) {
        return ercd;
    }

    return E_OK;
}

static ER ftps_svr_dat_con(T_FTP_SERVER *ftp)
{
    T_NODE host;
    ER ercd;

    if (STS_DAT_CON == (ftp->sts_flg & STS_DAT_CON)) {
        return E_OK;
    }

    if (ftp->mod == FTP_MOD_PASSIVE) {
        ercd = E_OK;
        if (!(ftp->sts_flg & STS_DAT_DONE)) {
            /* Wait for connection done */
            ftp->wai_flg = 1U;
            ercd = tslp_tsk(FTPS_DAT_TMO);
            ftp->wai_flg = 0U;

            /* ftp server task flag stopped state? */
            if ( ftp->sts_tsk_flg == FTP_SERVER_STOP ) {
                return E_RLWAI;
            }
        }
        if (STS_DAT_DONE == (ftp->sts_flg & STS_DAT_DONE)) {
            ftp->sts_flg &= ~STS_DAT_DONE;
            ercd = ftp->wai_err;
        }
        return ercd;
    }

    /* Prepare Socket */
    ercd = ftps_svr_dat_cre(ftp, ftp->dat_port);
    if (ercd != E_OK) {
        return ercd;
    }

    /* Establish connection */
    net_memset((char *)&host, 0, sizeof(host));
    host.num  = (UB)ftp->cli_dev_num;
    host.ipa  = ftp->cli_adr;
    host.port = ftp->cli_port;
    host.ver  = IP_VER4;
    ercd = con_soc(ftp->dat_sid, &host, SOC_CLI);
    if (ercd != E_OK) {
        return E_CLS;
    }

    ftp->sts_flg |= STS_DAT_CON;

    return E_OK;
}

static void ftps_svr_dat_cls(T_FTP_SERVER *ftp)
{
    (void)cls_soc(ftp->dat_sid, SOC_TCP_CLS);
    ftp->wai_flg = 0U;
    ftp->sts_flg &= ~STS_DAT_CON;
}

static ER ftps_svr_cre(T_FTP_SERVER *ftp)
{
    ER ercd;

    /* Set Transmission Timeout */
    ercd = cfg_soc(ftp->ctl_sid, SOC_TMO_SND, (VP)FTPS_CMD_TMO);
    if (ercd != E_OK) {
        return ercd;
    }

    /* Set Reception Timeout */
    ercd = cfg_soc(ftp->ctl_sid, SOC_TMO_RCV, (VP)FTPS_IDLE_TMO);
    if (ercd != E_OK) {
        return ercd;
    }

    /* Set Close Timeout */
    ercd = cfg_soc(ftp->ctl_sid, SOC_TMO_CLS, (VP)FTPS_CMD_TMO);
    if (ercd != E_OK) {
        return ercd;
    }

    return E_OK;
}

static ER ftps_svr_listen(T_FTP_SERVER *ftp)
{
    T_NODE host;
    ER ercd;

    while (1) {
        net_memset((char *)&host, 0, sizeof(host));
        host.num  = (UB)ftp->dev_num;
        host.ver  = IP_VER4;
        ercd = con_soc(ftp->ctl_sid, &host, SOC_SER);
        if (ercd != E_TMOUT) {
            break;
        }
        tslp_tsk(1);
        (void)cls_soc(ftp->ctl_sid, SOC_TCP_CLS);
    }
    if (E_OK == ercd) {
        (void)ref_soc(ftp->ctl_sid, SOC_IP_REMOTE, (VP)&host);
        ftp->cli_dev_num = host.num;
        ftp->sts_flg = STS_CLI_CON;
        ftp->pwd[0] = FTPS_DRV_NAME;
        ftp->pwd[1] = ':';
        ftp->pwd[2] = '\\';
        ftp->pwd[3] = '\0';
    }

    return ercd;
}

static void ftps_svr_cls(T_FTP_SERVER *ftp)
{
    ftps_svr_dat_cls(ftp);
    (void)cls_soc(ftp->ctl_sid, SOC_TCP_CLS);
    ftp->sts_flg = 0U;
}

static ER ftps_str_to_num(char *str, UW *ip, UH *port)
{
    UW tmp;

    *ip = 0U;
    *port = 0U;

    tmp = (UW)net_atol((const char *)str);
    *ip += tmp << 24;
    str = net_strchr(str, ',');
    if (str == NULL) {
        return E_PAR;
    }
    str++;

    tmp = (UW)net_atol((const char *)str);
    *ip += tmp << 16;
    str = net_strchr(str, ',');
    if (str == NULL) {
        return E_PAR;
    }
    str++;

    tmp = (UW)net_atol((const char *)str);
    *ip += tmp << 8;
    str = net_strchr(str, ',');
    if (str == NULL) {
        return E_PAR;
    }
    str++;

    tmp = (UW)net_atol((const char *)str);
    *ip += tmp;
    str = net_strchr(str, ',');
    if (str == NULL) {
        return E_PAR;
    }
    str++;

    tmp = (UW)net_atol((const char *)str);
    *port += (UH)(tmp << 8);
    str = net_strchr(str, ',');
    if (str == NULL) {
        return E_PAR;
    }
    str++;

    tmp = (UW)net_atol((const char *)str);
    *port += (UH)tmp;

    return E_OK;
}


static ER ftps_svr_parse(T_FTP_SERVER *ftp)
{
    ER ercd;
    VB cbuf[FTP_CMDLEN_MAX + 1U];
    UB n;

    /* extract & uppercase command string  */
    for (n = 0U; n < (UB)(sizeof(cbuf)/sizeof(cbuf[0])); ++n) {
        switch (ftp->cmd[n]) {
        case ' ':
        case '\r':
        case '\n':
            cbuf[n] = '\0';
            break;

        default:
            cbuf[n] = (VB)TO_UPPER(ftp->cmd[n]);
            break;
        }

        if ('\0' == cbuf[n]) {
            break;
        }
    }

    /* check command length */
    if ((FTP_CMDLEN_MIN > n) || (n > FTP_CMDLEN_MAX)) {
        ercd = E_PAR;
    }
    else {
        /* cue argument parameter */
        for (; ftp->cmd[n] == ' '; ++n) {
            ;
        }
        if ((ftp->cmd[n - 1U] == ' ') && (ftp->cmd[n] != '\0')) {
            ftp->arg = &ftp->cmd[n];
        }
        else {
            ftp->arg = NULL;
        }

        if (CMD_STRCMP(cbuf, "USER")) {
            ftp->cmd_id = FTP_CMD_USER;
        }
        else if (CMD_STRCMP(cbuf, "PASS")) {
            ftp->cmd_id = FTP_CMD_PASS;
        }
        else if (CMD_STRCMP(cbuf, "QUIT")) {
            ftp->cmd_id = FTP_CMD_QUIT;
        }
        else if (CMD_STRCMP(cbuf, "TYPE")) {
            ftp->cmd_id = FTP_CMD_TYPE;
        }
        else if (CMD_STRCMP(cbuf, "REST")) {
            ftp->cmd_id = FTP_CMD_REST;
            if (ftp->arg != NULL) {
                ftp->file_offset = (UW)net_atol((const char*)ftp->arg);
            }
        }
        else if (CMD_STRCMP(cbuf, "RNFR")) {
            ftp->cmd_id = FTP_CMD_RNFR;
        }
        else if (CMD_STRCMP(cbuf, "RNTO")) {
            ftp->cmd_id = FTP_CMD_RNTO;
        }
        else if (CMD_STRCMP(cbuf, "XPWD") || CMD_STRCMP(cbuf, "PWD")) {
            ftp->cmd_id = FTP_CMD_PWD;
        }
        else if (CMD_STRCMP(cbuf, "XCWD") || CMD_STRCMP(cbuf, "CWD")) {
            ftp->cmd_id = FTP_CMD_CWD;
        }
        else if (CMD_STRCMP(cbuf, "XCUP") || CMD_STRCMP(cbuf, "CDUP")) {
            ftp->cmd_id = FTP_CMD_CWD;
            ftp->arg = &ftp->cmd[5];
            net_strcpy(ftp->arg, "..");
        }
        else if (CMD_STRCMP(cbuf, "XMKD") || CMD_STRCMP(cbuf, "MKD")) {
            ftp->cmd_id = FTP_CMD_MKD;
        }
        else if (CMD_STRCMP(cbuf, "XRMD") || CMD_STRCMP(cbuf, "RMD")) {
            ftp->cmd_id = FTP_CMD_RMD;
        }
        else if (CMD_STRCMP(cbuf, "LIST")) {
            ftp->cmd_id = FTP_CMD_LIST;
        }
        else if (CMD_STRCMP(cbuf, "NLST")) {
            ftp->cmd_id = FTP_CMD_NLST;
        }
        else if (CMD_STRCMP(cbuf, "PORT")) {
            ftp->cmd_id = FTP_CMD_PORT;
        }
        else if (CMD_STRCMP(cbuf, "PASV")) {
            ftp->cmd_id = FTP_CMD_PASV;
        }
        else if (CMD_STRCMP(cbuf, "EPSV")) {
            ftp->cmd_id = FTP_CMD_EPSV;
        }
        else if (CMD_STRCMP(cbuf, "SIZE")) {
            ftp->cmd_id = FTP_CMD_SIZE;
        }
        else if (CMD_STRCMP(cbuf, "SYST")) {
            ftp->cmd_id = FTP_CMD_SYST;
        }
        else if (CMD_STRCMP(cbuf, "STOR")) {
            ftp->cmd_id = FTP_CMD_STOR;
        }
        else if (CMD_STRCMP(cbuf, "RETR")) {
            ftp->cmd_id = FTP_CMD_RETR;
        }
        else if (CMD_STRCMP(cbuf, "DELE")) {
            ftp->cmd_id = FTP_CMD_DELE;
        }
        else if (CMD_STRCMP(cbuf, "ABOR")) {
            ftp->cmd_id = FTP_CMD_ABOR;
        }
        else {
            ftp->cmd_id = FTP_CMD_NONE;
        }

        ercd = E_OK;
    }

    return ercd;
}

static ER ftps_svr_rcv_byte(T_FTP_SERVER *ftp, UB *c)
{
    ER ercd;

    if (ftp->rcv_len == 0U) {
        ercd = rcv_soc(ftp->ctl_sid, ftp->rcv_buf, (UH)sizeof(ftp->rcv_buf));
        if (ercd <= 0) {
            return ercd;
        }
        ftp->rcv_len = (UH)ercd;
        ftp->rcv_id = 0U;
    }

    *c = ftp->rcv_buf[ftp->rcv_id];
    ftp->rcv_id++;
    ftp->rcv_len--;

    return 1;
}

static ER ftps_svr_rcv(T_FTP_SERVER *ftp)
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
        ercd = ftps_svr_rcv_byte(ftp, (UB*)&c);
        if (ercd <= 0) {
            return -1;
        }
        if (c == '\r') {
            /* Skip */
            continue;
        }
        if (ignore == 0) {
            ftp->cmd[len] = c;
        }

        if (c == '\n') {

            ftp->cmd[len] = '\0';

            if (len < (H)LEN_DRV_LTR) {  /* FTP Minimum command size */
                more = 1;   /* Line is too short, Ignore and read next line */
            }

            break;
        }
        else {
            if ((INT)len == ((H)sizeof(ftp->cmd) - 1)) {
                ignore = 1;  /* Line is too big, trunc extra bytes */
            }
        }

        if ( ignore == 0 ) {
            len++;
        }
    }

    if (0 != more) {
        goto _ftp_res_start;
    }

    return (len + 1);
}

static ER ftps_svr_snd(SID sid, UB *data, UW len)
{
    ER ercd;
    UW i;
    UW slen;

    i = 0U;
    while (len > 0U) {
        slen = (UH)((len > UH_MAX_VAL) ? UH_MAX_VAL : len);
        ercd = snd_soc(sid, data + i, (UH)slen);
        if (ercd <= 0) {
            return ercd;
        }
        i += (UW)ercd;
        len -= (UW)ercd;
    }

    return (ER)i;
}

static ER ftps_svr_snd_res(T_FTP_SERVER *ftp, const char *res)
{
    /* Send response */
    return ftps_svr_snd(ftp->ctl_sid, (VP)res, (UW)net_strlen(res));
}

static void ftps_path_fix(char *path)
{
    char *dst;
    char *src;
    char *cur;
    UW   len;
    UW   i;

    src = path + LEN_DRV_LTR;
    dst = path + LEN_DRV_LTR;

    while (1) {
        len = 0U;
        cur = src;
        /* search \ */
        while (1) {
            if (*src == '\0') {
                break;
            }
            src++;
            len++;
            if (src[-1] == '\\') {
                break;
            }
        }
        /* convert path */
        if (len > 0U) {
            i = 0U;
            if (cur[i] == '.') {
                i++;
                if (cur[i] == '.') {
                    i++;
                }

                switch (cur[i]) {
                case '\\':
                case '\0':
                    i |= 0x80U;
                    break;

                default:
                    /* dot file/directory */
                    break;
                }
            }

            if (0U != (i & 0x80U)) {
                if (((i & 0x0FU) == 2U) && (dst > (path + LEN_DRV_LTR))) {
                    for (dst -= 2; dst >= (path + LEN_DRV_LTR); dst--) {
                        if (dst[-1] == '\\') {
                            break;
                        }
                    }
                }
            }
            else {
                for (i = 0U; i < len; i++) {
                    *dst++  = cur[i];
                }
            }
        }
        if (*src == '\0') {
            *dst = '\0';
            break;
        }
    }

    len = net_strlen(path);
    if (len < LEN_DRV_LTR) {
        path[2] = '\\';
        path[3] = '\0';
    }
    else if (len > LEN_DRV_LTR) {
        if (path[len - 1U] == '\\') {
            path[len - 1U] = '\0';
        }
    }
    else {
        /* do nothing */
    }

    return;
}

static ER chk_path_get_abs(char *pwd, char *path, UW nlen)
{
    ER ercd;
    UW len;

    /* Check the absolute path length */

    /* Path not specified, use existing path */
    if ((path == 0) || (*path == '\0')) {
        ercd = (nlen > net_strlen(pwd)) ? E_OK : E_NOMEM;
    }
    /* Root path is specified, use as it is */
    else if ((*path == '\\') || (*path == '/')) {
        len = net_strlen(path) + LEN_DRV_LTR;
        ercd = (nlen >= len) ? E_OK : E_NOMEM;
    }
    /* Append specified path with current path   */
    /* For this, current path must end with '\\' */
    else {
        len = net_strlen(pwd);
        if (pwd[len - 1U] != '\\') {
            len++;
        }
        len += net_strlen(path) + 1U;
        ercd = (nlen >= len) ? E_OK : E_NOMEM;
    }

    return ercd;
}



static void ftps_path_get_abs(char *pwd, char *path, char *np)
{
    UW len;
    UW i;

    /* Get the absolute path */

    /* Path not specified, use existing path */
    if ((path == 0) || (*path == '\0')) {
        net_strcpy(np, pwd);
        return;
    }

    /* Root path is specified, use as it is */
    if ((*path == '\\') || (*path == '/')) {
        np[0] = FTPS_DRV_NAME;
        np[1] = ':';
        np[2] = '\0';
        net_strcat(np, path);
    }
    else {
        /* Append specified path with current path   */
        /* For this, current path must end with '\\' */
        net_strcpy(np, pwd);
        len = net_strlen(np);
        if (np[len - 1U] != '\\') {
            net_strcat(np, "\\");
        }
        net_strcat(np, path);
    }

    /* Replace '/' with '\' */
    len = net_strlen(np);
    for (i = 0U; i < len; i++) {
        if (np[i] == '/') {
            np[i] = '\\';
        }
    }

    /* Traverse path */
    ftps_path_fix(np);
}

static ER ftps_path_get_relative(char *abs_path, char *path)
{
    UW len;
    UW i;

    /* Get the relative path */

    len = net_strlen(abs_path);
    if (len < LEN_DRV_LTR) {
        return -1;
    }

    net_strcpy(path, &abs_path[2]);

    len = net_strlen(path);
    for (i = 0U; i < len; i++) {
        if (path[i] == '\\') {
            path[i] = '/';
        }
    }

    return E_OK;
}

static char* ftps_path_split_sub(char *path)
{
    char* p;
    char* s;

    if ((path == 0) || (*path == '\0')) {
        return NULL;
    }
    p = path;
    s = path;
    while (*p != '\0') {
        if (*p++ == '\\') {
            s = p;
        }
    }
    if (s != path) {
        s[-1] = '\0';
    }

    return s;
}

static BOOL ftps_path_ls_match(char *str, char *ptn)
{
    char *s;
    char *p;
    B match;

    if (net_strcmp(str, ptn) == 0) {
        return TRUE;
    }

    s = net_strchr(ptn, '*');
    p = net_strchr(ptn, '?');
    if ((s == NULL) && (p == NULL)) {
        return FALSE;
    }

    s = str;
    p = ptn;
    match = 0;

    while (1) {
        if (*s == '\0') {
            /* File shorter than pattern */
            /* Return True, if remaining pattern is only '*' */
            while (*p == '*') {
                p++;
            }
            return (*p == '\0');
        }

        if (*s == *p) {
            s++;
            p++;
        }
        else if (*p == '?') {
            s++;
            p++;
        }
        else if (*p == '*') {
            while (*p == '*') {
                p++;      /* skip '*' */
            }
            if (*p == '\0') {
                return TRUE;
            }
            str = s; ptn = p;
            match = 1;
        }
        else {
            if (match == 0) {
                return FALSE;
            }
            s = str++;
            p = ptn;
        }
    }
}

static ER ftps_auth_login(UH dev_num, const char* usr, const char* pwd)
{
    ER ercd;
    W i;

    ercd = E_OBJ;
    for (i = 0; i < FTPS_MAX_USR_CNT; i++) {
        if (ftp_usr_tbl[i].usr == 0x00) {
            break;
        }
        if (ftp_usr_tbl[i].dev_num == DEV_ANY) {
            if ((net_strcmp(ftp_usr_tbl[i].usr, usr) == 0)
             && (net_strcmp(ftp_usr_tbl[i].pwd, pwd) == 0)) {
                ercd = E_OK;
                break;
            }
        } else if (ftp_usr_tbl[i].dev_num == dev_num) {
            if ((net_strcmp(ftp_usr_tbl[i].usr, usr) == 0)
             && (net_strcmp(ftp_usr_tbl[i].pwd, pwd) == 0)) {
                ercd = E_OK;
                break;
            }
        }
        else {
            /* do nothing */
        }
    }

    return ercd;
}

#define CHKPM_ALR_LOGIN     0x01U
#define CHKPM_NOT_LOGIN     0x02U
#define CHKPM_ARG_NULL      0x04U
#define CHKPM_CHK_FULLPATH  0x10U
#define CHKPM_MAK_FULLPATH  0x20U
#define CHKPM_GET_FULLPATH  (CHKPM_CHK_FULLPATH | CHKPM_MAK_FULLPATH)

static UH ftps_chk_param(T_FTP_SERVER *ftp, UH chk_flg, ER *ercd)
{
    UH ret;
    do {
        ret = 0U;

        if (0U != (chk_flg & CHKPM_ALR_LOGIN)) {
            if (STS_LOGIN == (ftp->sts_flg & STS_LOGIN)) {
                *ercd = ftps_svr_snd_res(ftp, RCD_530_1);
                ret = CHKPM_ALR_LOGIN;
                break;
            }
        }

        if (0U != (chk_flg & CHKPM_NOT_LOGIN)) {
            if (STS_LOGIN != (ftp->sts_flg & STS_LOGIN)) {
                *ercd = ftps_svr_snd_res(ftp, RCD_530_2);
                ret = CHKPM_NOT_LOGIN;
                break;
            }
        }

        if (0U != (chk_flg & CHKPM_ARG_NULL)) {
            if (ftp->arg == NULL) {
                switch (ftp->cmd_id) {
                case FTP_CMD_MKD:
                case FTP_CMD_RMD:
                case FTP_CMD_CWD:
                    *ercd = ftps_svr_snd_res(ftp, RCD_501_1);
                    break;

                case FTP_CMD_DELE:
                case FTP_CMD_RNFR:
                case FTP_CMD_RNTO:
                case FTP_CMD_SIZE:
                    *ercd = ftps_svr_snd_res(ftp, RCD_501_2);
                    break;

                default:
                    *ercd = ftps_svr_snd_res(ftp, RCD_501_3);
                    break;
                }
                ret = CHKPM_ARG_NULL;
                break;
            }
        }

        if (0U != (chk_flg & CHKPM_CHK_FULLPATH)) {
            *ercd = chk_path_get_abs(ftp->pwd, ftp->arg, sizeof(ftps_path_buf));
            if (*ercd != E_OK) {
                *ercd = ftps_svr_snd_res(ftp, RCD_500_6);
                ret = CHKPM_CHK_FULLPATH;
                break;
            }
        }
        if (0U != (chk_flg & CHKPM_MAK_FULLPATH)) {
            ftps_path_get_abs(ftp->pwd, ftp->arg, ftps_path_buf);
            *ercd = E_OK;
        }
    } while (0);

    return ret;
}

static ER ftps_cmd_usr(T_FTP_SERVER *ftp)
{
    ER ercd;
    UH chk_prm;

    do {
        chk_prm = CHKPM_ALR_LOGIN;
        chk_prm = ftps_chk_param(ftp, chk_prm, &ercd);
        if (0U != chk_prm) {
            break;
        }

        if (NULL != ftp->arg) {
            net_strcpy((char *)ftp->usr, (const char *)ftp->arg);
        }
        else {
            net_strcpy((char *)ftp->usr, "");
        }

        ftp->sts_flg |= STS_USER;

        ercd = ftps_svr_snd_res(ftp, RCD_331);
    } while (0);

    return ercd;
}

static ER ftps_cmd_pass(T_FTP_SERVER *ftp)
{
    ER ercd;
    VB *pass;
    UH chk_prm;

    do {
        chk_prm = CHKPM_ALR_LOGIN;
        chk_prm = ftps_chk_param(ftp, chk_prm, &ercd);
        if (0U != chk_prm) {
            break;
        }
        if (STS_USER != (ftp->sts_flg & STS_USER)) {
            ercd = ftps_svr_snd_res(ftp, RCD_503);
            break;
        }

        if (NULL != ftp->arg) {
            pass = ftp->arg;
        }
        else {
            pass = "";
        }

        ercd = ftp->auth_cbk(ftp->dev_num, (const char*)ftp->usr, pass);
        if (ercd != E_OK) {
            ercd = ftps_svr_snd_res(ftp, RCD_530_3);
            break;
        }

        ftp->sts_flg |= STS_LOGIN;

        ercd = ftps_svr_snd_res(ftp, RCD_230);
    } while (0);

    return ercd;
}

static ER ftps_cmd_quit(T_FTP_SERVER *ftp)
{
    /* 221 Service Closing connection */
    (void)ftps_svr_snd_res(ftp, RCD_221);

    if (STS_CLI_CON == (ftp->sts_flg & STS_CLI_CON)) {
        (void)cls_soc(ftp->ctl_sid, SOC_TCP_CLS);
    }
    ftp->sts_flg = 0U;

    return E_OK;
}

static ER ftps_cmd_pasv(T_FTP_SERVER *ftp)
{
    T_NODE host;
    ER ercd;
    UB ip[4];
    UB p[2];
    UH chk_prm;

    do {
        chk_prm = CHKPM_NOT_LOGIN;
        chk_prm = ftps_chk_param(ftp, chk_prm, &ercd);
        if (0U != chk_prm) {
            break;
        }

        ftps_svr_dat_cls(ftp);        /* Clean data socket */
        ftp->mod = FTP_MOD_PASSIVE;
        ercd = ftps_svr_dat_listen(ftp);
        if (ercd != E_OK) {
            ftps_svr_dat_cls(ftp);
            ercd = ftps_svr_snd_res(ftp, RCD_500_3);
            break;
        }

        ercd = ref_soc(ftp->dat_sid, SOC_IP_LOCAL, (VP)&host);
        if (ercd != E_OK) {
            ftps_svr_dat_cls(ftp);
            ercd = ftps_svr_snd_res(ftp, RCD_500_3);
            break;
        }

        ftp->cli_adr = host.ipa;
        ftp->cli_port = host.port;

        ip[3] = (host.ipa      ) & UB_MAX_VAL;
        ip[2] = (host.ipa >>  8) & UB_MAX_VAL;
        ip[1] = (host.ipa >> 16) & UB_MAX_VAL;
        ip[0] = (host.ipa >> 24) & UB_MAX_VAL;

        p[1]  = (host.port     ) & UB_MAX_VAL;
        p[0]  = (host.port >> 8) & UB_MAX_VAL;

        net_strcpy(ftp->cmd, RCD_227A);
        ftps_cat_dig(ftp->cmd, (UW)ip[0], 0, (VB)0);
        net_strcat(ftp->cmd, ",");
        ftps_cat_dig(ftp->cmd, (UW)ip[1], 0, (VB)0);
        net_strcat(ftp->cmd, ",");
        ftps_cat_dig(ftp->cmd, (UW)ip[2], 0, (VB)0);
        net_strcat(ftp->cmd, ",");
        ftps_cat_dig(ftp->cmd, (UW)ip[3], 0, (VB)0);
        net_strcat(ftp->cmd, ",");
        ftps_cat_dig(ftp->cmd, (UW)p[0],  0, (VB)0);
        net_strcat(ftp->cmd, ",");
        ftps_cat_dig(ftp->cmd, (UW)p[1],  0, (VB)0);
        net_strcat(ftp->cmd, ")" RCD_CRLF);

        ercd = ftps_svr_snd_res(ftp, (const char *)ftp->cmd);
    } while (0);

    return ercd;
}

static ER ftps_cmd_epsv(T_FTP_SERVER *ftp)
{
    T_NODE host;
    ER ercd;
    UH chk_prm;

    do {
        chk_prm = CHKPM_NOT_LOGIN;
        chk_prm = ftps_chk_param(ftp, chk_prm, &ercd);
        if (0U != chk_prm) {
            break;
        }

        ftps_svr_dat_cls(ftp);        /* Clean data socket */
        ftp->mod = FTP_MOD_PASSIVE;
        ercd = ftps_svr_dat_listen(ftp);
        if (ercd != E_OK) {
            ftps_svr_dat_cls(ftp);
            ercd = ftps_svr_snd_res(ftp, RCD_500_2);
            break;
        }

        ercd = ref_soc(ftp->dat_sid, SOC_IP_LOCAL, (VP)&host);
        if (ercd != E_OK) {
            ftps_svr_dat_cls(ftp);
            ercd = ftps_svr_snd_res(ftp, RCD_500_2);
            break;
        }

        ftp->cli_adr = host.ipa;
        ftp->cli_port = host.port;

        net_strcpy(ftp->cmd, RCD_229A);
        ftps_cat_dig(ftp->cmd, (UW)host.port, 0, (VB)0x00);
        net_strcat(ftp->cmd, "|)" RCD_CRLF);

        ercd = ftps_svr_snd_res(ftp, (const char *)ftp->cmd);
    } while (0);

    return ercd;
}

static ER ftps_cmd_port(T_FTP_SERVER *ftp)
{
    ER ercd;
    UH chk_prm;

    do {
        chk_prm = CHKPM_NOT_LOGIN | CHKPM_ARG_NULL;
        chk_prm = ftps_chk_param(ftp, chk_prm, &ercd);
        if (0U != chk_prm) {
            break;
        }

        if (ENA_DENY_PORTCOMMAND == (ftp->sec & ENA_DENY_PORTCOMMAND)) {
            ercd = ftps_svr_snd_res(ftp, RCD_550_4);
            break;
        }

        ftps_svr_dat_cls(ftp);        /* Clean data socket */

        ercd = ftps_str_to_num(ftp->arg, &ftp->cli_adr, &ftp->cli_port);
        if (ercd != E_OK) {
            ercd = ftps_svr_snd_res(ftp, RCD_500_5);
            break;
        }

        if (ENA_NOTCON_WELL_KNOWNPORT == (ftp->sec & ENA_NOTCON_WELL_KNOWNPORT)) {
            if (ftp->cli_port <= WELLKNOWN_PORT_END) {
                ercd = ftps_svr_snd_res(ftp, RCD_504);
                break;
            }
        }

        ftp->mod = FTP_MOD_ACTIVE;

        /* 200 Command okay */
        ercd = ftps_svr_snd_res(ftp, RCD_200_1);
    } while (0);

    return ercd;
}

static ER ftps_cmd_type(T_FTP_SERVER *ftp)
{
    ER ercd;
    char type;
    UH chk_prm;

    do {
        chk_prm = CHKPM_NOT_LOGIN | CHKPM_ARG_NULL;
        chk_prm = ftps_chk_param(ftp, chk_prm, &ercd);
        if (0U != chk_prm) {
            break;
        }

        type = *ftp->arg;
        if (type == FTP_TYP_ASCII) {
            ftp->typ = FTP_TYP_ASCII;
        }
        else if (type == FTP_TYP_BINARY) {
            ftp->typ = FTP_TYP_BINARY;
        }
        else {
            ercd = ftps_svr_snd_res(ftp, RCD_504);
            break;
        }

        /* 200 Command okay */
        ercd = ftps_svr_snd_res(ftp, RCD_200_2);
    } while (0);

    return ercd;
}

static ER ftps_cmd_pwd(T_FTP_SERVER *ftp)
{
    ER ercd;
    UH chk_prm;

    do {
        chk_prm = CHKPM_NOT_LOGIN;
        chk_prm = ftps_chk_param(ftp, chk_prm, &ercd);
        if (0U != chk_prm) {
            break;
        }

        /* Get full path */
        (void)ftps_path_get_relative(ftp->pwd, ftps_path_buf);

        /* 257 Command okay */
        net_strcpy(ftps_res_buf, RCD_257_1A "\"");
        net_strcat(ftps_res_buf, (const char*)ftps_path_buf);
        net_strcat(ftps_res_buf, "\"" RCD_CRLF);

        ercd = ftps_svr_snd_res(ftp, (const char *)ftps_res_buf);
    } while (0);

    return ercd;
}

static ER ftps_cmd_syst(T_FTP_SERVER *ftp)
{
    ER ercd;
    UH chk_prm;

    do {
        chk_prm = CHKPM_NOT_LOGIN;
        chk_prm = ftps_chk_param(ftp, chk_prm, &ercd);
        if (0U != chk_prm) {
            break;
        }

        /* 215 or User-callback */
        if (ftp->syst_name != NULL) {
            net_strcpy(ftps_res_buf, RCD_215_1A);
            net_strcat(ftps_res_buf, ftp->syst_name);
            net_strcat(ftps_res_buf, RCD_CRLF);
        }
        else {
            net_strcpy(ftps_res_buf, RCD_215_2);
        }

        ercd = ftps_svr_snd_res(ftp, (const char *)ftps_res_buf);
    } while (0);

    return ercd;
}

static ER ftps_cmd_cwd(T_FTP_SERVER *ftp)
{
    DIR *dir;
    ER ercd;
    UH chk_prm;

    do {
        chk_prm = CHKPM_NOT_LOGIN | CHKPM_ARG_NULL | CHKPM_GET_FULLPATH;
        chk_prm = ftps_chk_param(ftp, chk_prm, &ercd);
        if (0U != chk_prm) {
            break;
        }

        /* Check directory exists or not */
        dir = opendir((const char *)ftps_path_buf);
        (void)closedir(dir);
        if (dir == NULL) {
            /* Directory not exists */
            net_strcpy(ftps_res_buf, RCD_550_3);
        }
        else {
            /* Directory exists. Make it as current directory */
            net_strcpy(ftp->pwd, (const char *)ftps_path_buf);
            (void)ftps_path_get_relative(ftp->pwd, ftps_path_buf);
            net_strcpy(ftps_res_buf, RCD_250_1A "\"");
            net_strcat(ftps_res_buf, (const char*)ftps_path_buf);
            net_strcat(ftps_res_buf, "\"" RCD_CRLF);
        }

        ercd = ftps_svr_snd_res(ftp, (const char *)ftps_res_buf);
    } while (0);

    return ercd;
}

static ER ftps_cmd_mkdir(T_FTP_SERVER *ftp)
{
    ER ercd;
    UH chk_prm;

    do {
        chk_prm = CHKPM_NOT_LOGIN | CHKPM_ARG_NULL | CHKPM_GET_FULLPATH;
        chk_prm = ftps_chk_param(ftp, chk_prm, &ercd);
        if (0U != chk_prm) {
            break;
        }

        /* Create Directory */
        ercd = mkdir((const char *)ftps_path_buf, (UW)S_IARC);
        if (ercd == -1) {
            /* Directory not created */
            net_strcpy(ftps_res_buf, RCD_550_2);
        }
        else {
            /* Directory created */
            net_strcpy(ftps_res_buf, RCD_257_2);
        }

        ercd = ftps_svr_snd_res(ftp, (const char *)ftps_res_buf);
    } while (0);

    return ercd;
}

static ER ftps_cmd_rmdir(T_FTP_SERVER *ftp)
{
    ER ercd;
    UH chk_prm;

    do {
        chk_prm = CHKPM_NOT_LOGIN | CHKPM_ARG_NULL | CHKPM_GET_FULLPATH;
        chk_prm = ftps_chk_param(ftp, chk_prm, &ercd);
        if (0U != chk_prm) {
            break;
        }

        if (net_strcmp((const char *)ftp->pwd, (const char *)ftps_path_buf) == 0) {
            /* Cannot delete current working directory itself */
            ercd = ftps_svr_snd_res(ftp, RCD_550_5);
            break;
        }

        /* Remove Directory */
        ercd = rmdir((const char *)ftps_path_buf);
        if (ercd == -1) {
            /* Directory not exists */
            net_strcpy(ftps_res_buf, RCD_550_5);
        }
        else {
            /* Directory removed */
            net_strcpy(ftps_res_buf, RCD_250_2);
        }

        ercd = ftps_svr_snd_res(ftp, (const char *)ftps_res_buf);
    } while (0);

    return ercd;
}

static ER ftps_cmd_del(T_FTP_SERVER *ftp)
{
    ER ercd;
    UH chk_prm;

    do {
        chk_prm = CHKPM_NOT_LOGIN | CHKPM_ARG_NULL | CHKPM_GET_FULLPATH;
        chk_prm = ftps_chk_param(ftp, chk_prm, &ercd);
        if (0U != chk_prm) {
            break;
        }

        /* Delete file */
        ercd = remove((const char *)ftps_path_buf);
        if (ercd == -1) {
            /* Directory not exists */
            net_strcpy(ftps_res_buf, RCD_550_6);
        }
        else {
            /* Directory removed */
            net_strcpy(ftps_res_buf, RCD_250_3);
        }

        ercd = ftps_svr_snd_res(ftp, (const char *)ftps_res_buf);
    } while (0);

    return ercd;
}

static ER ftps_cmd_ren(T_FTP_SERVER *ftp)
{
    ER ercd;
    UH chk_prm;

    do {
        chk_prm = CHKPM_NOT_LOGIN | CHKPM_ARG_NULL | CHKPM_GET_FULLPATH;
        chk_prm = ftps_chk_param(ftp, chk_prm, &ercd);
        if (0U != chk_prm) {
            break;
        }

        if (ftp->cmd_id == FTP_CMD_RNFR) {
            if (net_strlen(ftps_path_buf) > (UW)sizeof(ftp->dat)) {
                ercd = ftps_svr_snd_res(ftp, RCD_500_6);
                break;
            }
            ftp->sts_flg |= STS_CMD_RNFR;
            net_strcpy((VB*)ftp->dat, ftps_path_buf);   /* rename source path (temporary use) */

            net_strcpy(ftps_res_buf, RCD_350);
        }
        if (ftp->cmd_id == FTP_CMD_RNTO) {
            ercd = (STS_CMD_RNFR == (ftp->sts_flg & STS_CMD_RNFR)) ? E_OK : -1 ;
            if (ercd == -1) {
                ercd = ftps_svr_snd_res(ftp, RCD_503);
                break;
            }

            /* Rename file */
            ercd = rename((VB*)ftp->dat, ftps_path_buf);
            if (ercd == -1) {
                /* rename failed */
                net_strcpy(ftps_res_buf, RCD_553);
            }
            else {
                /* rename successed */
                net_strcpy(ftps_res_buf, RCD_250_4);
            }
        }

        ercd = ftps_svr_snd_res(ftp, (const char *)ftps_res_buf);
    } while (0);

    return ercd;
}

static ER ftps_cmd_size(T_FTP_SERVER *ftp)
{
    struct stat fsts;
    W res;
    ER ercd;
    UH chk_prm;

    do {
        chk_prm = CHKPM_NOT_LOGIN | CHKPM_ARG_NULL | CHKPM_GET_FULLPATH;
        chk_prm = ftps_chk_param(ftp, chk_prm, &ercd);
        if (0U != chk_prm) {
            break;
        }

        /* Get file */
        res = stat((const char *)ftps_path_buf, &fsts);
        if (res == 0) {
            /* Directory removed */
            net_strcpy(ftps_res_buf, RCD_213A);
            ftps_cat_dig(ftps_res_buf, fsts.st_size, 0, (VB)0);
            net_strcat(ftps_res_buf, RCD_CRLF);
        } else {
            /* Directory not exists */
            net_strcpy(ftps_res_buf, RCD_550_1);
        }

        ercd = ftps_svr_snd_res(ftp, (const char *)ftps_res_buf);
    } while (0);

    return ercd;
}

static ER ftps_svr_snd_list_sub(T_FTP_SERVER *ftp, DIR *dir, char *name)
{
    static const char* ftps_month[12] = {
        "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"
    };

    struct stat fsts;
    struct dirent* dir_ent;
    ER ercd;
    W res;
    UW size;
    UW path_len;
    UH file_dt;
    UH file_tm;

    ercd = 1;   /* For socket error */
    path_len = net_strlen(ftps_path_buf);

    if (dir != NULL) {
        ercd = ftps_svr_snd_res(ftp, RCD_150_1);
        if (ercd <= 0) {
            (void)closedir(dir);
            (void)ftps_svr_snd_res(ftp, RCD_421);
            //ercd = 421;   // error ctl-sock (disconnect)
            return ercd;
        }

        ercd = ftps_svr_dat_con(ftp);
        if (ercd != E_OK) {
            (void)closedir(dir);
            /* Note: windows-ftp does not cancel the wait. */
            (void)ftps_svr_snd_res(ftp, RCD_425_1);
            ercd = 425;
            return ercd;
        }

        ftps_res_buf[0] = '\0';
        ercd = 1;   /* For socket error */
        while (1) {
            dir_ent = readdir(dir);
            if (dir_ent == 0x00) {
                break;
            }

#if defined(DT_VOL_ID)  /* support volume label */
            if (dir_ent->d_type & DT_VOL_ID) {
                continue;       /* skip volume label */
            }
#endif

            if (name && (!(ftps_path_ls_match(dir_ent->d_name, name)))) {
                continue;
            }

            if (ftp->cmd_id == FTP_CMD_LIST) {
                ftps_path_buf[path_len] = '\0';
                if ('\\' != ftps_path_buf[path_len - 1U]) {
                    net_strcat(ftps_path_buf, "\\");
                }
                net_strcat(ftps_path_buf, dir_ent->d_name);
                res = stat((const char *)ftps_path_buf, &fsts);
                if (res == 0) {
                    file_dt = (fsts.st_mtime >> 16) & UH_MAX_VAL;
                    file_tm = fsts.st_mtime & UH_MAX_VAL;
                    size = fsts.st_size;
                    if ((UW)S_IFDIR == (fsts.st_mode & (UW)S_IFDIR)) {
                        net_strcpy((char*)ftp->dat, "drw-r--r-- 1 ftp ftp ");
                    } else {
                        net_strcpy((char*)ftp->dat, "-rw-r--r-- 1 ftp ftp ");
                    }
                    ftps_cat_dig((char*)ftp->dat, size, 13, ' ');
                    net_strcat((char*)ftp->dat, " ");
                    net_strcat((char*)ftp->dat, ftps_month[((file_dt & 0x01E0U) >> 5)-1]);
                    ftps_cat_dig((char*)ftp->dat, file_dt & 0x1FU, 3, ' ');
                    net_strcat((char*)ftp->dat, " ");
                    ftps_cat_dig((char*)ftp->dat, file_tm >> 11, 2, '0');
                    net_strcat((char*)ftp->dat, ":");
                    ftps_cat_dig((char*)ftp->dat, (file_tm & 0x07E0U) >> 5, 2, '0');
                    net_strcat((char*)ftp->dat, " ");
                    net_strcat((char*)ftp->dat, dir_ent->d_name);
                    net_strcat((char*)ftp->dat, RCD_CRLF);
                    ercd = ftps_svr_snd(ftp->dat_sid, (VP)ftp->dat, net_strlen((char*)ftp->dat));
                    if (ercd <= 0) {
                        /* error snd_soc */
                        net_strcpy( ftps_res_buf, RCD_426A );
                        net_strcat( ftps_res_buf, " for ftps_svr_snd. [" );
                        net_itoa( (INT)ercd, (char *)&ftps_res_buf[net_strlen( ftps_res_buf )], 10 );
                        net_strcat( ftps_res_buf, "]" RCD_CRLF );
                        ercd = 426;     // error dat-sock (not disconnect)
                        break;
                    }
                } else {
                    /* error stat */
                    net_strcpy( ftps_res_buf, RCD_451A );
                    net_strcat( ftps_res_buf, " for stat." RCD_CRLF );
                    ercd = 451;     /* error stat() */
                    break;
                }
            }
            else {
                net_strcpy((char*)ftp->dat, dir_ent->d_name);
                net_strcat((char*)ftp->dat, RCD_CRLF);
                ercd = ftps_svr_snd(ftp->dat_sid, (VP)ftp->dat, net_strlen((char*)ftp->dat));
                if (ercd <= 0) {
                    /* error snd_soc */
                    net_strcpy( ftps_res_buf, RCD_426A );
                    net_strcat( ftps_res_buf, " for ftps_svr_snd. [" );
                    net_itoa( (INT)ercd, (char *)&ftps_res_buf[net_strlen( ftps_res_buf )], 10 );
                    net_strcat( ftps_res_buf, "]" RCD_CRLF );
                    ercd = 426;     // error dat-sock (not disconnect)
                    break;
                }
            }
        }
        res = closedir(dir);
        if (-1 == res) {
            if ('\0' == ftps_res_buf[0]) {
                net_strcpy( ftps_res_buf, RCD_FILEIO_ERR_2A );
                net_strcat( ftps_res_buf, " for closedir." RCD_CRLF );
                ercd = 450;     /* error closedir() */
            }
        }
    }

    if ('\0' == ftps_res_buf[0]) {  /* No Error */
        /* Set success message. */
        net_strcpy(ftps_res_buf, RCD_226_1);
        ercd = 226;
    }
    (void)ftps_svr_snd_res(ftp, (const char *)ftps_res_buf);
    return ercd;
}

static ER ftps_svr_snd_list(T_FTP_SERVER *ftp)
{
    DIR *dir;
    char *subpath;
    ER ercd;

    do {
        (void)ftps_chk_param(ftp, CHKPM_MAK_FULLPATH, &ercd);
        ercd = 1;   /* For socket error */

        /* Path is ending with a directory, display all contents */
        dir = opendir((const char *)ftps_path_buf);
        if (dir != NULL) {
            ercd = ftps_svr_snd_list_sub(ftp, dir, NULL);
            break;
        }

        /* Path is ending with a directory which does not exists */
        /* May be end with wild card or file */
        subpath = ftps_path_split_sub(ftps_path_buf);
        if (subpath == NULL) {
            net_strcpy( ftps_res_buf, RCD_FILEIO_ERR_2A );
            net_strcat( ftps_res_buf, " for local_func." RCD_CRLF);
            (void)ftps_svr_snd_res(ftp, ftps_res_buf);
            break;
        }

        dir = opendir((const char *)ftps_path_buf);
        if (dir == NULL) {
            /* Cannot open root path */
            net_strcpy( ftps_res_buf, RCD_FILEIO_ERR_2A );
            net_strcat( ftps_res_buf, " for opendir." RCD_CRLF);
            (void)ftps_svr_snd_res(ftp, ftps_res_buf);
            break;
        }

        /* closedir */
        if (net_strcmp(subpath, "*.*") == 0) {
            ercd = ftps_svr_snd_list_sub(ftp, dir, NULL);
            break;
        }

        /* Display matching contents */
        ercd = ftps_svr_snd_list_sub(ftp, dir, subpath);
    } while (0);

    return ercd;
}




static ER ftps_svr_rcv_file(T_FTP_SERVER *ftp)
{
    FILE *fp;
    UW flen;
    ER ercd;
    W res;

    do {
        ftps_res_buf[0] = '\0';
        (void)ftps_chk_param(ftp, CHKPM_MAK_FULLPATH, &ercd);


        fp = fopen((const char *)ftps_path_buf, "wb");
        if (fp == NULL) {
            net_strcpy( ftps_res_buf, RCD_FILEIO_ERR_2A );
            net_strcat( ftps_res_buf, " for fopen." RCD_CRLF);
            ercd = 450;
            break;
        }

        ercd = ftps_svr_snd_res(ftp, RCD_150_1);
        if (ercd <= 0) {
            net_strcpy( ftps_res_buf, RCD_421 );
            //ercd = 421;   // error ctl-sock (disconnect)
            break;
        }

        ercd = ftps_svr_dat_con(ftp);
        if (ercd != E_OK) {
            net_strcpy( ftps_res_buf, RCD_425_1 );
            ercd = 425;
            break;
        }



        /* Receive File */
        while (1) {
            ercd = rcv_soc(ftp->dat_sid, ftp->dat, FTPS_DAT_BUF_MAX);
            if ( (ercd == 0) || (ercd == E_CLS) ) {
                /* Success */
                ercd = 226;
                break;
            } else if ( ercd < 0 ) {
                net_strcpy( ftps_res_buf, RCD_426A );
                net_strcat( ftps_res_buf, " for rcv_soc. [" );
                net_itoa( (INT)ercd, (char *)&ftps_res_buf[net_strlen( ftps_res_buf )], 10 );
                net_strcat( ftps_res_buf, "]" RCD_CRLF );
                ercd = 426;   // error dat-sock (not disconnect)
                break;
            }
            else {
                /* do nothing */
            }

            flen = fwrite(ftp->dat, 1U, (UINT)ercd, fp);
            if (flen != (UW)ercd) {
                net_strcpy( ftps_res_buf, RCD_FILEIO_ERR_2A );
                net_strcat( ftps_res_buf, " for fwrite. [" );
                net_itoa( (INT)flen, (char *)&ftps_res_buf[net_strlen( ftps_res_buf )], 10 );
                net_strcat( ftps_res_buf, "]" RCD_CRLF );
                ercd = 450;     /* error fwrite() */
                break;
            }
        }
    } while (0);

    if (NULL != fp) {
        res = fclose(fp);
        if (EOF == res) {
            if ('\0' == ftps_res_buf[0]) {
                net_strcpy( ftps_res_buf, RCD_FILEIO_ERR_2A );
                net_strcat( ftps_res_buf, " for fclose." RCD_CRLF );
                ercd = 450;     /* error fclose() */
            }
        }
    #if !defined(CFG_FTPS_FSYNC_DIS) && defined(FFSYS_H) && defined(FFS_VER)
        res = fsync((W)(FTPS_DRV_NAME));
        if (-1 == res) {
            if ('\0' == ftps_res_buf[0]) {
                net_strcpy( ftps_res_buf, RCD_FILEIO_ERR_2A );
                net_strcat( ftps_res_buf, " for fsync." RCD_CRLF );
                ercd = 450;     /* error fsync() */
            }
        }
    #endif
    }

    if ('\0' == ftps_res_buf[0]) {  /* No Error */
        /* Set success message. */
        net_strcpy(ftps_res_buf, RCD_226_2);
        ercd = 226;
    }
    (void)ftps_svr_snd_res(ftp, (const char *)ftps_res_buf);

    return ercd;
}

static ER ftps_svr_snd_file(T_FTP_SERVER *ftp)
{
    FILE *fp;
    UW flen, fsize;
    ER ercd;
    W res;

    do {
        fsize = 0;  /* warning suppress */
        ftps_res_buf[0] = '\0';
        (void)ftps_chk_param(ftp, CHKPM_MAK_FULLPATH, &ercd);

        fp = fopen((const char *)ftps_path_buf, "rb");
        if (fp == NULL) {
            net_strcpy( ftps_res_buf, RCD_FILEIO_ERR_1 );
            ercd = 450;     /* error fopen() */
            break;
        }

        ercd = ftps_svr_snd_res(ftp, RCD_150_1);
        if (ercd <= 0) {
            net_strcpy( ftps_res_buf, RCD_421 );
            //ercd = 421;   // error ctl-sock (disconnect)
            break;
        }

        ercd = ftps_svr_dat_con(ftp);
        if (ercd != E_OK) {
            net_strcpy( ftps_res_buf, RCD_425_1 );
            ercd = 425;
            break;
        }

        /* Transmit File */
        fsize = 0;
        while (1) {
            flen = fread(ftp->dat, 1U, sizeof(ftp->dat), fp);
            if (flen > 0U) {
                ercd = ftps_svr_snd(ftp->dat_sid, ftp->dat, flen);
                if ( ercd <= 0 ) {
                    /* error snd_soc */
                    net_strcpy(ftps_res_buf, RCD_426A);
                    net_strcat(ftps_res_buf, " for ftps_svr_snd. [");
                    net_itoa((INT)ercd, (char *)&ftps_res_buf[net_strlen( ftps_res_buf )], 10);
                    net_strcat(ftps_res_buf, "]" RCD_CRLF);
                    ercd = 426;   // error dat-sock (not disconnect)
                    break;
                }
                fsize += flen;
            }
            if ( flen != (UW)sizeof(ftp->dat) ) {
                ercd = feof(fp);
                if (0 == ercd) {
                    net_strcpy(ftps_res_buf, RCD_FILEIO_ERR_2A);
                    net_strcat(ftps_res_buf, " for fread." RCD_CRLF);
                    ercd = 450;     /* error feof(), not last data. */
                } else {
                    /* success */
                    ercd = 226;
                }
                break;
            }
        }
    } while (0);

    if (NULL != fp) {
        res = fclose(fp);
        if (EOF == res) {
            if ('\0' == ftps_res_buf[0]) {
                net_strcpy( ftps_res_buf, RCD_FILEIO_ERR_2A );
                net_strcat( ftps_res_buf, " for fclose." RCD_CRLF );
                ercd = 450;     /* error fclose() */
            }
        }
    }

    if ('\0' == ftps_res_buf[0]) {  /* No Error */
        /* Set success message. */
        net_strcpy(ftps_res_buf, RCD_226_3A);
        ftps_cat_dig((char*)ftps_res_buf, fsize, 0, (VB)0x00);
        net_strcat(ftps_res_buf, "]" RCD_CRLF);
        ercd = 226;
    }
    (void)ftps_svr_snd_res(ftp, (const char *)ftps_res_buf);

    return ercd;
}

static ER ftps_svr_cmd(T_FTP_SERVER *ftp)
{
    ER ercd;
    ER (*func_dat_cmd)(T_FTP_SERVER *ftp);
    UH chk_prm;

    /* Note: If the return value is less than 0, FTP disconnection.
         Set the return value to 0 or more except for communication errors.    */

    ercd = E_OK;
    do {
        /* Note: Dont use "CHKPM_MAK_FULLPATH" parameter here.          */
        /* Because ftps_path_buf is likely to change by task switching. */
        switch (ftp->cmd_id) {
            case FTP_CMD_LIST:
            case FTP_CMD_NLST:
                chk_prm = CHKPM_NOT_LOGIN | CHKPM_CHK_FULLPATH;
                func_dat_cmd = &ftps_svr_snd_list;
                break;
            case FTP_CMD_STOR:
                chk_prm = CHKPM_NOT_LOGIN | CHKPM_ARG_NULL | CHKPM_CHK_FULLPATH;
                func_dat_cmd = &ftps_svr_rcv_file;
                break;
            case FTP_CMD_RETR:
                chk_prm = CHKPM_NOT_LOGIN | CHKPM_ARG_NULL | CHKPM_CHK_FULLPATH;
                func_dat_cmd = &ftps_svr_snd_file;
                break;
            default:
                /* abnormal branch */
                chk_prm = 0U;
                func_dat_cmd = NULL;
                break;
        }
        if (0U == chk_prm) {
            ercd = ftps_svr_snd_res(ftp, RCD_500_1);
            break;
        }
        chk_prm = ftps_chk_param(ftp, chk_prm, &ercd);
        if (0U != chk_prm) {
            break;
        }

        if (ftp->mod == FTP_MOD_NONE) {
            ercd = ftps_svr_snd_res(ftp, RCD_425_2);
            break;
        }

        ercd = func_dat_cmd(ftp);   /* include ftps_svr_dat_con() */
    } while (0);

    ftps_svr_dat_cls(ftp);
    return ercd;
}

static void enq_ftp_server( T_FTP_SERVER *node )
{
    if ( gFTP_SERVER_STACK == NULL ) {
        /* set first stack(FILO)                                             */
        node->next = NULL;
        gFTP_SERVER_STACK = node;
    }
    else {
        /* set stack(FILO)                                                   */
        node->next = gFTP_SERVER_STACK;
        gFTP_SERVER_STACK = node;
    }

    return;
}

#ifndef NET_HW_OS
static T_FTP_SERVER *deq_ftp_server( void )
{
    T_FTP_SERVER *node;                /* ftp node session handle            */

    /* get node(FILO)                                                        */
    node = gFTP_SERVER_STACK;
    if ( node != NULL ) {
        /* set next node                                                     */
        gFTP_SERVER_STACK = node->next;
    }

    return node;
}
#endif

static ER ftps_wait_ipset(T_FTP_SERVER *ftp)
{
    ER ercd;
    T_NET_ADR adr;

    while (1) {
        ercd = net_ref(ftp->dev_num, NET_IP4_CFG, (VP)&adr);
        if (ercd != E_OK) {
            break;
        }
        if (adr.ipaddr != 0U) {
            break;
        }
        tslp_tsk(500);

        /* ftp server task flag stopped state? */
        if ( ftp->sts_tsk_flg == FTP_SERVER_STOP ) {
            return E_RLWAI;
        }
    }

    return ercd;
}

ER ftp_server(T_FTP_SERVER *ftp)
{
    ER ercd;

    ercd = E_OK;
    if (ftp == NULL) {
        ercd = E_PAR;
    }
    else if ((ftp->ctl_sid == 0U) || (ftp->dat_sid == 0U)) {
        ercd = E_PAR;
    }
    else {
        (void)ref_soc(ftp->ctl_sid, SOC_PRT_LOCAL, (VP)&ftp->ctl_port);
        (void)ref_soc(ftp->dat_sid, SOC_PRT_LOCAL, (VP)&ftp->dat_port);
        if ((ftp->ctl_port == 0U) || (ftp->dat_port == 0U)) {
            ercd = E_PAR;
        }
    }
    if (ercd != E_OK) {
        return ercd;
    }

    /* ftp server authenticate (default set) */
    if (!ftp->auth_cbk) {
        ftp->auth_cbk = &ftps_auth_login;
    }

    /* get own task id */
    ercd = get_tid( &ftp->wai_tsk_id );
    if ( ercd != E_OK ) {
        return ercd;
    }

    /* regist ftp_server */
    ercd = set_svrctl(ftp);
    if ( ercd != E_OK ) {
        return ercd;
    }

    /* set ftp server boot status */
    ftp->sts_tsk_flg = FTP_SERVER_BOOT;

    /* enqueue ftp server handle */
    enq_ftp_server( ftp );

    if (ftp->dev_num != DEV_ANY) {
        ercd = ftps_wait_ipset(ftp);
        if ( ercd != E_OK ) {
            return ercd;
        }
    }

_ftp_start:

    ercd = ftps_svr_cre(ftp);
    if (ercd != E_OK) {
        return ercd;
    }

    ercd = ftps_svr_listen(ftp);
    if (ercd != E_OK) {
        goto _ftp_server_cls;
    }

    /* Send Welcome Message to Client */
    /* 220 Service ready for new user */
    ercd = ftps_svr_snd_res(ftp, RCD_220);
    if (ercd <= 0) {
        goto _ftp_server_cls;
    }

    ftp->mod = FTP_MOD_NONE;

_ftp_cmd:

    ercd = ftps_svr_rcv(ftp);
    if (ercd <= 0) {
        goto _ftp_server_cls;
    }

    ercd = ftps_svr_parse(ftp);
    if (ercd != E_OK) {
        ftp->cmd_id = FTP_CMD_ERR;  /* Command Syntax Error */
        ercd = E_OK;
    }

    switch (ftp->cmd_id) {
        case FTP_CMD_USER:
            ercd = ftps_cmd_usr(ftp);
            break;
        case FTP_CMD_PASS:
            ercd = ftps_cmd_pass(ftp);
            break;
        case FTP_CMD_QUIT:
            ercd = ftps_cmd_quit(ftp);
            break;
        case FTP_CMD_PASV:
            ercd = ftps_cmd_pasv(ftp);
            break;
        case FTP_CMD_PORT:
            ercd = ftps_cmd_port(ftp);
            break;
        case FTP_CMD_EPSV:
            ercd = ftps_cmd_epsv(ftp);
            break;
        case FTP_CMD_LIST:
        case FTP_CMD_NLST:
        case FTP_CMD_STOR:
        case FTP_CMD_RETR:
            ercd = ftps_svr_cmd(ftp);
            break;
        case FTP_CMD_TYPE:
            ercd = ftps_cmd_type(ftp);
            break;
        case FTP_CMD_SIZE:
            ercd = ftps_cmd_size(ftp);
            break;
        case FTP_CMD_PWD:
            ercd = ftps_cmd_pwd(ftp);
            break;
        case FTP_CMD_SYST:
            if (ENA_ALLOW_SYSTCOMMAND == (ftp->sec & ENA_ALLOW_SYSTCOMMAND)) {
                ercd = ftps_cmd_syst(ftp);
            }
            else {
                ercd = ftps_svr_snd_res(ftp, RCD_500_1);    /* same default */
            }
            break;
        case FTP_CMD_CWD:
            ercd = ftps_cmd_cwd(ftp);
            break;
        case FTP_CMD_MKD:
            ercd = ftps_cmd_mkdir(ftp);
            break;
        case FTP_CMD_RMD:
            ercd = ftps_cmd_rmdir(ftp);
            break;
        case FTP_CMD_DELE:
            ercd = ftps_cmd_del(ftp);
            break;
        case FTP_CMD_RNFR:
        case FTP_CMD_RNTO:
            ercd = ftps_cmd_ren(ftp);
            break;
        case FTP_CMD_ERR:
            ercd = ftps_svr_snd_res(ftp, RCD_500_4);
            break;
        default:
            ercd = ftps_svr_snd_res(ftp, RCD_500_1);
            break;
    }

    /* clear command status */
    if (ftp->cmd_id != FTP_CMD_RNFR) {
        ftp->sts_flg &= ~STS_CMD_RNFR;
    }

    if (ercd > 0) {
        goto _ftp_cmd;  /* No error, process next command */
    }

_ftp_server_cls:

    ftps_svr_cls(ftp);

    /* ftp server task flag stopped state? */
    if ( ftp->sts_tsk_flg == FTP_SERVER_STOP ) {
        /* unregist ftp_server */
        clr_svrctl(ftp);
        return E_RLWAI;
    }

    goto _ftp_start;
}

#ifndef NET_HW_OS
ER ftp_server_stop( UW retry )
{
    T_FTP_SERVER    *node;
    T_RTST          tskinfo;            /* task status info                  */
    UW              i;                  /* loop counter                      */
    ER              ercd;               /* function result                   */

    /*-----------------------------------------------------------------------*/
    /* initialize variable                                                   */
    /*-----------------------------------------------------------------------*/
    ercd = E_OK;


    /*-----------------------------------------------------------------------*/
    /* ftp server task stop proc                                             */
    /* all session dequeue                                                   */
    /*-----------------------------------------------------------------------*/
    while (1) {
        node = deq_ftp_server();
        if (node == NULL) {
            break;
        }

        /* set ftp server stop flag                                          */
        node->sts_tsk_flg = FTP_SERVER_STOP;

        for (i = 0U; i <= retry; i++) {
            /* control socket all cancel socket proc                         */
            (void)abt_soc( node->ctl_sid, SOC_ABT_ALL );

            /* data socket all cancel socket proc                            */
            (void)abt_soc( node->dat_sid, SOC_ABT_ALL );

            /* wait retrying                                                 */
            (void)dly_tsk( FTP_SERVER_RETRY_WAIT );

            /* get task status                                               */
            ercd = ref_tst(node->wai_tsk_id, &tskinfo);
            if ( ercd != E_OK ) {
                break;
            }
            /* stopped?                                                      */
            if ( tskinfo.tskstat == TTS_DMT ) {
                break;
            }
        }

        if (ercd != E_OK) {
            break;
        }
        /* retry over check                                                  */
        if (i > retry) {
            ercd = E_TMOUT;
            break;
        }
    }

    return ercd;
}
#endif
