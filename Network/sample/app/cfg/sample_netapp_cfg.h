/***************************************************************************
    Network Sample Application Configuration
    Copyright (c)  2014, eForce Co., Ltd. All rights reserved.

    2014-08-14: Created.
 ***************************************************************************/
#ifndef _SAMPLE_NETAPP_CFG_H
#define _SAMPLE_NETAPP_CFG_H

#include "net_hdr.h"
#include "net_strlib.h"
#include "shell.h"

#ifndef NULL
#define NULL    ((void*)0)
#endif

/* load sample settings */
#include "sample_use.h"
#include "sample_def_conf.h"

extern ER dns_get_ipa_opt(UW dnsd_ipa, VB *str, UW *ipa);

/*****************************************************************************
    Socket ID & Kernel ID
*****************************************************************************/
#if SAMPLE_USE_GENSRC
#include "kernel_id.h"
#include "net_id.h"

#else
extern SID ID_SOC_DHCP;
extern SID ID_SOC_DNS;
#define ID_SOC_DNSC		ID_SOC_DNS
#endif

/* printf like function */
extern void shell_printf(VP ctrl, const char *fmt, ...) ;



/*****************************************************************************
    Sample Application Settings
*****************************************************************************/
#define SPL_LF              "\r\n"          /* Line feed code */

/* Default DNS server */
extern UW dns_svr_ip;
#define SPL_DNS_SERVER      dns_svr_ip

#if SAMPLE_ENA_FTPc
/*---- FTP  ----------------------------------------------------------------*/
#ifndef NET_C
extern SID ID_SOC_FTPC_CMD;
extern SID ID_SOC_FTPC_DATA;
#endif

/* FTP settings (ftp command) */
#define SPL_FTP_CTL_PORT    21
#define SPL_FTP_DAT_PORT    20
#define SPL_FTP_LS_BUF_SZ   1024            /* LIST/NLST result buffer size */
#endif

#if SAMPLE_ENA_TFTPc
/*---- TFTPc----------------------------------------------------------------*/
#ifndef NET_C
extern SID ID_SOC_TFTPC;
#endif

/* TFTPc settings (tftp command) */
#define SPL_TFTP_PORT       69
#endif

#if SAMPLE_ENA_HTTPc
/*---- HTTP ----------------------------------------------------------------*/
#ifndef NET_C
extern SID ID_SOC_HTTPC;
extern ID ID_HTTP_MSG_MPF;     /* fixed memory pool */
#endif

/* HTTP settings (http_xxx command) */
#define SPL_HTTP_USR_BUF    64
#define SPL_HTTP_PW_BUF     64
#define SPL_HTTP_B_USR      "demo_basic"    /* Basic Auth Username */
#define SPL_HTTP_B_PW       "password"      /* Basic Auth Password */
#define SPL_HTTP_D_USR      "demo_digest"   /* Digest Auth Username */
#define SPL_HTTP_D_PW       "password"      /* Digest Auth Password */
#endif

#if SAMPLE_ENA_SMTPua
/*---- SMTP ----------------------------------------------------------------*/
#ifndef NET_C
extern SID ID_SOC_SMTP;
#endif

/* SMTP settings (mail command) */
#define SPL_SMTP_SERVER     "smtp.demo.co.jp"
#define SPL_SMTP_PORT       25
#define SPL_SMTP_USER       "from@demo.co.jp"       /* SMTP Authentication username */
#define SPL_SMTP_PASS       "password"              /* SMTP Authentication password */
#define SPL_SMTP_AUTH       SMTP_AUTH_CMD5          /* [SMTP_NO_AUTH | SMTP_AUTH_PLAIN | SMTP_AUTH_CMD5] */

/* send mail settings */
#define SPL_ML_FROM         "from_user <from@demo.co.jp>"
#define SPL_ML_TO           "to_user <to@demo.co.jp>"
#define SPL_ML_CC           "cc_user <cc@demo.co.jp>"
#define SPL_ML_BCC          0
#define SPL_ML_SUBJECT      "SMTPクライアント 送信テスト"
#define SPL_ML_BODY         "メール本文1行目\r\nメール本文2行目"
#define SPL_ML_CHARSET      "shift-jis"
#define SPL_ML_CHARSET8BIT  1       /* 1: Charset is 8bit. / 0: Charset is 7bit. */

#define SPL_ML_ATTACH_FILE
#ifdef SPL_ML_ATTACH_FILE
  #define SPL_MLAF_USE_MEM  1                       /* Use memory, or file(filesystem) */
  #define SPL_MLAF_NAME     "test.dat"              /* Attach Filename */
  #define SPL_MLAF_BUFPATH  (VP)shell_usr_cmd_mail  /* memory: Base Addr / file: Path */
  #define SPL_MLAF_BUFLEN   0xF0                    /* memory: length / file: not use */
#endif
#endif

#if SAMPLE_ENA_POP3c
/*---- POP3 ----------------------------------------------------------------*/
#ifndef NET_C
extern SID ID_SOC_POP3;
#endif

/* POP3 settings (mail command) */
#define SPL_POP3_SERVER     "pop.demo.co.jp"
#define SPL_POP3_PORT       110
#define SPL_POP3_USER       "from@demo.co.jp"      /* POP3 Authentication username */
#define SPL_POP3_PASS       "password"             /* POP3 Authentication password */
#define SPL_POP3_AUTH       POP3_AUTH_PLAIN        /* [POP3_AUTH_PLAIN | POP3_AUTH_APOP] */
#endif

#if SAMPLE_ENA_DHCPd
#if (0 == SAMPLE_USE_GENSRC)    /* no use configurator */
/*---- DHCP server ---------------------------------------------------------*/

#define SPL_DHCPD_STA_ADDR      ip_aton("172.16.1.100")
#define SPL_DHCPD_LEASE_NUM     10
#define SPL_DHCPD_LEASE_PERIOD  86400
#define SPL_DHCPD_SUBNETMASK    ip_aton("255.255.255.0")
#define SPL_DHCPD_GATEWAY       ip_aton("172.16.1.1")
#endif
#endif

#if SAMPLE_ENA_MQTTc
extern ER mqtt_usr_ini(void);
#endif

#endif /* _SAMPLE_NETAPP_CFG_H */
