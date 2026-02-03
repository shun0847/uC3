/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    FTP Server
    Copyright (c) 2014-2019, eForce Co., Ltd. All rights reserved.

    Version Information
      2014.03.18: Created
      2015.03.31: Add security policy for PORT command.
      2015.12.14: The socket ID replaced SID types
      2016.02.10: Add include files for warning avoidance
      2016.04.06: Add auth_cbk member of T_FTP_SERVER structure.
      2016.07.25: Execute static analysis tool to this source.
      2017.05.11: Remove definition CFG_FTPS_NET_SOC_MAX.
      2018.02.28: Support SYST command. 
      2019.01.22: Add psv_port_min/max member of T_FTP_SERVER structure.
 ***************************************************************************/

#ifndef FTP_SERVER_H
#define FTP_SERVER_H
#ifdef __cplusplus
extern "C" {
#endif

#include "kernel.h"
#include "net_hdr.h"
/* Configuration */
#include "ftp_server_cfg.h"

#if !defined(CFG_FTPS_DAT_BUF_MAX)
#define CFG_FTPS_DAT_BUF_MAX     1024U    /* Data buffer size */
#endif
#if !defined(CFG_FTPS_CTL_BUF_MAX)
#define CFG_FTPS_CTL_BUF_MAX     272U     /* Control buffer size */
#endif
#if !defined(CFG_FTPS_USER_BUF_MAX)
#define CFG_FTPS_USER_BUF_MAX    32U      /* Username buffer size */
#endif

#define FTP_SERVER_RETRY_WAIT    100U     /* wait retrying(ms) */
#define FTP_SERVER_BOOT          1U       /* ftp server task state boot flag */
#define FTP_SERVER_STOP          0U       /* ftp server task state stop flag */

/* Security policy */
#define ENA_DENY_PORTCOMMAND        0x00000001U
#define ENA_NOTCON_WELL_KNOWNPORT   0x00000002U
#define ENA_ALLOW_SYSTCOMMAND       0x00000004U

/* Login user table */
typedef struct t_ftp_usr_tbl {
    UH dev_num;     /* Network device number (DEV_ANY(0): All device is allowed) */
    VB* usr;        /* User name */
    VB* pwd;        /* Password */
} T_FTP_USR_TBL;

/* Control block */
typedef struct t_ftp_server {
    VB *arg;                /* Command arguments */
    UW sec;                 /* Security policy   */
    UW file_offset;         /* File start position */
    UW cli_adr;             /* Client IP address */
    ID wai_tsk_id;          /* Wait Task ID */
    ER wai_err;             /* Wait Error */
    SID ctl_sid;            /* Command Socket ID */
    SID dat_sid;            /* Data Socket ID */
    UH sts_flg;             /* Status flag */
    UH ctl_port;            /* Control port */
    UH dat_port;            /* Data port */
    UH cli_port;            /* Client port */
    UH cmd_id;              /* Parsed command index */
    UH rcv_len;             /* Received data size */
    UH rcv_id;              /* Received buffer index */
    UH dev_num;             /* Device number of server listen channel */
    UH cli_dev_num;         /* Device number of client channel */
    UB mod;                 /* Mode (Passive or active) */
    UB typ;                 /* Type (Binary or ascii) */
    VB cmd[CFG_FTPS_CTL_BUF_MAX];           /* Client command */
    VB pwd[CFG_FTPS_PATH_MAX];              /* Print working directory */
    UB dat[CFG_FTPS_DAT_BUF_MAX];           /* Data transfer buffer */
    UB rcv_buf[CFG_FTPS_CTL_BUF_MAX];       /* Receive buffer */
    UB usr[CFG_FTPS_USER_BUF_MAX];          /* User name buffer */
    UB wai_flg;                             /* Wait flag */
    UB sts_tsk_flg;                         /* Task status flag */
    struct t_ftp_server *next;              /* ftp server next node */
    ER (*auth_cbk)(UH, const char*, const char*);   /* Authenticate callback */
    VB *syst_name;          /* SYST command answer (user specification) */
    UH psv_port_min;        /* Minimum port of PASV command */
    UH psv_port_max;        /* Maximum port of PASV command */
} T_FTP_SERVER;

/* FTP Server API */
ER ftp_server(T_FTP_SERVER *ftp);
ER ftp_server_stop( UW retry );

#ifdef __cplusplus
}
#endif
#endif

