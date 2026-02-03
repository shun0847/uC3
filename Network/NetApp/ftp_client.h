/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    DHCP Client header file
    Copyright (c)  2008-2016, eForce Co., Ltd. All rights reserved.

    Version Information
      2008.10.05: Created
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
      2014.04.11: Corrected to "UH" a type of "dev_num".
      2014.05.20: Added the following features.
        1. Supports FTP over TLS/SSL. (uNet3/SSL required.)
        2. Added some API for FTP commands. (MKD, RMD, DELE, RNFR, RNTO, NOOP)
      2015.07.30: Adjustment of FTPC_DAT_BUF_MAX size (1024 -> 1000)
      2015.12.14: The socket ID replaced SID types
      2016.02.10: Add include files for warning avoidance
      2016.08.08: Execute static analysis tool to this source.
 ***************************************************************************/

#ifndef FTP_CLIENT_H
#define FTP_CLIENT_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "kernel.h"
#include "net_hdr.h"
#include "ftp_client_cfg.h"

#define FTPC_DAT_BUF_MAX        1000U
#define FTPC_CTL_BUF_MAX        256U

#define FTPC_CTL_PORT_DEFAULT   21U
#define FTPC_DATA_PORT_DEFAULT  42238U


typedef struct t_ftp_client {
    UW ipa;
    UH dev_num;
    SID ctl_sid;
    SID dat_sid;
    UH ctl_port;
    UH dat_port;
    UB mode;
    UB type;
    UW cmd_tmo;
    UH flag;
    UB Cmd[FTPC_CTL_BUF_MAX];
    UB Res[FTPC_CTL_BUF_MAX];
    UB Data[FTPC_DAT_BUF_MAX];
    UB rcv_buf[FTPC_DAT_BUF_MAX]; /* Temp */
    UH rcv_len;                  /* Temp */
    UH rcv_id;                /* Temp */

#ifdef FTPC_SSL_SUP
    SID ssl_sid;
#endif
}T_FTP_CLIENT;

typedef struct t_ftp_list {
    VP dat[2];
    VB *buf;                    
    UH len;
    UH next_pos;
    UB nameonly;                /* 0:LIST, 1:NLST */
} T_FTP_LIST;


#define FTPC_FLG_CTL         1U     /* Command socket connected */
#define FTPC_FLG_DAT         2U     /* Data socket connected    */
#define FTPC_FLG_LOG         4U     /* Login Successful         */
#define FTPC_FLG_SSL         8U     /* FTPS connected */
#define FTPC_FLG_SSL_DAT     0x10U  /* FTPS data connected */

#define FTPC_CFG_MODE        1
#define FTPC_CFG_TYPE        2

#define FTPC_MODE_ACTIVE     1U
#define FTPC_MODE_PASSIVE    0U      /* Default */
#define FTPC_MODE_APMASK     (FTPC_MODE_ACTIVE | FTPC_MODE_PASSIVE)
#define FTPC_TYPE_BINARY     ('I')  /* Default */
#define FTPC_TYPE_ASCII      ('A')

#define FTPC_MODE_SSL_USE    0x80   /* use FTPS */
#define FTPC_MODE_SSL_VFY    0x40   /* SSL vefity */
#define FTPC_MODE_SSL_IMP    0x20   /* FTPS Implicit mode */
#define FTPC_MODE_SSLMASK    (FTPC_MODE_SSL_USE | FTPC_MODE_SSL_VFY | FTPC_MODE_SSL_IMP)


/* FTP Client APIs */
ER ftp_login(T_FTP_CLIENT *ftp, const VB *user, const VB *pw);
ER ftp_quit(T_FTP_CLIENT *ftp);

ER ftp_get_file(T_FTP_CLIENT *ftp, const VB *lo_file, const VB *rmt_file);
ER ftp_put_file(T_FTP_CLIENT *ftp, const VB *lo_file, const VB *rmt_file);
ER ftp_del_file(T_FTP_CLIENT *ftp, const VB *rmt_file);
ER ftp_ren_file(T_FTP_CLIENT *ftp, const VB *org_name, const VB *ren_name);

ER ftp_cmd_cd(T_FTP_CLIENT *ftp, const VB *dir);
ER ftp_cmd_pwd(T_FTP_CLIENT *ftp, VB *dir);
ER ftp_cmd_list(T_FTP_CLIENT *ftp, const VB *dir, T_FTP_LIST *list);
ER ftp_cmd_list_next(T_FTP_LIST *list);
ER ftp_cmd_mkd(T_FTP_CLIENT *ftp, const VB *dir);
ER ftp_cmd_rmd(T_FTP_CLIENT *ftp, const VB *dir);
ER ftp_cmd_noop(T_FTP_CLIENT *ftp);


#ifdef __cplusplus
}
#endif
#endif /* FTP_CLIENT_H */

