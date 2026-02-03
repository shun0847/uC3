/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    SMTP Client header file
    Copyright (c)  2013-2020, eForce Co., Ltd. All rights reserved.

    Version Information
      2013.10.01: Created
      2014.04.04: Moved user setting to configure file.
      2014.04.11: Corrected to "UH" a type of "dev_num".
      2014.04.16: Corresponding to transmission of memory area.
      2014.10.27: Supported a POP before SMTP authentication.
      2015.12.14: The socket ID replaced SID types
      2016.02.10: Add include files for warning avoidance
      2016.09.06: Execute static analysis tool to this source.
      2018.03.20: Support SSL connection.
      2018.10.31: Disabled a POP before SMTP authentication.
      2020.02.04: Supported LOGIN authentication. 
 ***************************************************************************/

#ifndef SMTP_CLIENT_H
#define SMTP_CLIENT_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "kernel.h"
#include "net_hdr.h"
#include "smtp_client_cfg.h"

#ifdef SMTPC_SSL_SUP
#include "ssl_hdr.h"
#endif

#ifndef FORCE_ENA_AUTH_PBS
#undef ENA_AUTH_PBS
#else
#define ENA_AUTH_PBS
#endif

#define SMTP_DEF_PORT       25U
#define SMTP_ALT_PORT       587U
#define SMTPS_DEF_PORT      465U

/* SMTP Authentication type (T_SMTP_CLIENT::auth_type) */
#define SMTP_AUTH_TYPE_MASK 0x0FU       /* mask */
#define SMTP_AUTH_ENASSL    0x20U       /* ssl extension enable */
#define SMTP_AUTH_EXPLSSL   0x40U       /* ssl extension (explicit) */
#define SMTP_AUTH_IMPLSSL   0x80U       /* ssl extension (implicit) */
#define SMTP_NO_AUTH        0x00U
#define SMTP_AUTH_PLAIN     0x01U
#define SMTP_AUTH_CMD5      0x02U
#define SMTP_AUTH_DMD5      0x04U       /* not supported */
#define SMTP_AUTH_LOGIN     0x08U    
#define SMTP_AUTH_SSL       ((SMTP_AUTH_PLAIN | SMTP_AUTH_LOGIN) | SMTP_AUTH_IMPLSSL)
#define SMTP_AUTH_STARTTLS  ((SMTP_AUTH_PLAIN | SMTP_AUTH_LOGIN) | SMTP_AUTH_EXPLSSL)

#ifdef ENA_AUTH_PBS
#define SMTP_AUTH_PBS_PLAIN 0x0100U
#define SMTP_AUTH_PBS_APOP  0x0200U
#endif
    
/* Attach File type (T_SMTP_FILE::type) */
#define SMTP_FTYP_FS        0x01U
#define SMTP_FTYP_MEM       0x02U

typedef struct t_smtp_client {
    T_NODE svr;
    SID sid;
    UB flag;
    VB *buf;
    UH len;
    UH auth_type;
    VB *usr;
    VB *pw;
#ifdef SMTPC_SSL_SUP
    ID ssn_id;          /* SSL session ID */
#endif
} T_SMTP_CLIENT;

typedef struct t_smtp_file {
    struct t_smtp_file *next;
    VB *name;           /* Attach filename */
    VB *buf;            /* File buffer or file path */
    UW len;
    UB type;
} T_SMTP_FILE;

typedef struct t_smtp_mail {
    VB *from;
    VB *to;
    VB *cc;
    VB *bcc;
    VB *date;
    VB *subject;
    VB *body;
    VB *charset;
    T_SMTP_FILE *file;
    UB cs8bit;
} T_SMTP_MAIL;


ER smtp_login(T_SMTP_CLIENT *smtp);
ER smtp_quit(const T_SMTP_CLIENT *smtp);
ER smtp_snd_cmd(const T_SMTP_CLIENT *smtp, const VB *cmd, const VB *res);
ER smtp_snd_mail(const T_SMTP_CLIENT *smtp, const T_SMTP_MAIL *mail);

#ifdef __cplusplus
}
#endif
#endif /* SMTP_CLIENT_H */
