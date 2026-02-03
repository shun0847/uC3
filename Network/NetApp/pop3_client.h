/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    POP3 Client header file
    Copyright (c)  2014-2018, eForce Co., Ltd. All rights reserved.

    Version Information
      2014.10.31: Created
      2015.12.14: The socket ID replaced SID types
      2016.02.10: Add include files for warning avoidance
      2016.08.24: Corrected the following problems.
        1. Execute static analysis tool to this source.
        2. Added the pop3_qpdec_body() to API functions.
      2018.03.20: Support SSL connection.
 ***************************************************************************/

#ifndef POP3_CLIENT_H
#define POP3_CLIENT_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "kernel.h"
#include "net_hdr.h"
#include "pop3_client_cfg.h"

#ifdef POP3C_SSL_SUP
#include "ssl_hdr.h"
#endif

#define POP3_DEF_PORT       110U
#define POP3S_DEF_PORT      995U

/* POP3 Authentication type (T_POP3_CLIENT::auth_type) */
#define POP3_AUTH_TYPE_MASK 0x0FU       /* mask */
#define POP3_AUTH_ENASSL    0x20U       /* ssl extension enable */
#define POP3_AUTH_EXPLSSL   0x40U       /* ssl extension (explicit) */
#define POP3_AUTH_IMPLSSL   0x80U       /* ssl extension (implicit) */
#define POP3_AUTH_PLAIN     1U
#define POP3_AUTH_APOP      2U
#define POP3_AUTH_SSL       (POP3_AUTH_PLAIN | POP3_AUTH_IMPLSSL)
#define POP3_AUTH_STARTTLS  (POP3_AUTH_PLAIN | POP3_AUTH_EXPLSSL)


/* Attach File type (T_POP3_FILE::type) */
#define POP3_FTYP_FS        0x01U
#define POP3_FTYP_MEM       0x02U

/* POP3 response */
#define POP3_RES_OK         "+OK"
#define POP3_RES_ERR        "-ERR"

/* Content Flag define */
        /* Content-Transfer-Encoding type */
#define POP3_CTE_7BIT       0x0001U
#define POP3_CTE_8BIT       0x0002U
#define POP3_CTE_BIN        0x0004U
#define POP3_CTE_BASE64     0x0008U
#define POP3_CTE_QTPRT      0x0010U      /* quoted-printable */
#define POP3_CTE            (POP3_CTE_7BIT | POP3_CTE_8BIT | \
                            POP3_CTE_BIN | POP3_CTE_BASE64 | POP3_CTE_QTPRT)

        /* Content-Type Parameter type */
#define POP3_CTP_BOUNDARY   0x0400U
#define POP3_CTP_CHARSET    0x0200U
#define POP3_CTP_NAME       0x0100U


typedef struct t_pop3_client {
    T_NODE svr;
    SID sid;
    UH len;
    VB *buf;
    VB *usr;
    VB *pw;
    UB auth_type;
    UB flag;
#ifdef POP3C_SSL_SUP
    ID ssn_id;          /* SSL session ID */
#endif
} T_POP3_CLIENT;

typedef struct t_pop3_file {
    VB *buf;            /* File buffer or file path */
    UW len;
    UB type;            /* Saved type file or memory */
} T_POP3_FILE;

typedef struct t_pop3_part {
    VB *body;
    VB *ctype;          /* Content-Type */
    VB *ctprm;          /* Content Parameter */
    UW ctflg;           /* Content Flag */
    VB *next;           /* Next Body Part */
} T_POP3_PART;

typedef struct t_pop3_mail {
    VB *from;
    VB *to;
    VB *cc;
    VB *bcc;
    VB *date;
    VB *subject;
    T_POP3_PART part;   /* MIME header, Body */
} T_POP3_MAIL;


/* POP3 API */
ER pop3_login(T_POP3_CLIENT *pop3);
ER pop3_quit(T_POP3_CLIENT *pop3);
ER pop3_rcv_msg(T_POP3_CLIENT *pop3, UW mid, const T_POP3_FILE *file);

ER pop3_snd_cmd(T_POP3_CLIENT *pop3, const VB *str);
ER pop3_snd_cmd_0(T_POP3_CLIENT *pop3, const VB *cmd);
ER pop3_snd_cmd_i(T_POP3_CLIENT *pop3, const VB *cmd, UW p1);
ER pop3_snd_cmd_a(T_POP3_CLIENT *pop3, const VB *cmd, const VB *p1);
ER pop3_snd_cmd_i2(T_POP3_CLIENT *pop3, const VB *cmd, UW p1, UW p2);
ER pop3_snd_cmd_a2(T_POP3_CLIENT *pop3, const VB *cmd, const VB *p1, const VB *p2);

ER pop3_mline_next(T_POP3_CLIENT *pop3);
ER pop3_mline_eod(T_POP3_CLIENT *pop3);


/* POP3 command macro (pop3_snd_cmd) */
#define pop3_cmd_user(_p3cl,_pm)        (pop3_snd_cmd_a((_p3cl), "USER", (_pm)))
#define pop3_cmd_pass(_p3cl,_pm)        (pop3_snd_cmd_a((_p3cl), "PASS", (_pm)))
#define pop3_cmd_apop(_p3cl,_p1,_p2)    (pop3_snd_cmd_a2((_p3cl), "APOP", (_p1), (_p2)))
#define pop3_cmd_quit(_p3cl)            (pop3_snd_cmd_0((_p3cl), "QUIT"))
#define pop3_cmd_noop(_p3cl)            (pop3_snd_cmd_0((_p3cl), "NOOP"))
#define pop3_cmd_stat(_p3cl)            (pop3_snd_cmd_0((_p3cl), "STAT"))
#define pop3_cmd_list(_p3cl,_pm)    ((0U == (_pm)) \
                                        ? pop3_snd_cmd_0((_p3cl), "LIST")             \
                                        : pop3_snd_cmd_i((_p3cl), "LIST", (_pm)))
#define pop3_cmd_retr(_p3cl,_pm)        (pop3_snd_cmd_i((_p3cl), "RETR", (_pm)))
#define pop3_cmd_dele(_p3cl,_pm)        (pop3_snd_cmd_i((_p3cl), "DELE", (_pm)))
#define pop3_cmd_rset(_p3cl)            (pop3_snd_cmd_0((_p3cl), "RSET"))
#define pop3_cmd_top(_p3cl,_p1,_p2)     (pop3_snd_cmd_i2((_p3cl), "TOP", (_p1), (_p2)))
#define pop3_cmd_uidl(_p3cl,_pm)    ((0U == (_pm)) \
                                        ? pop3_snd_cmd_0((_p3cl), "UIDL")             \
                                        : pop3_snd_cmd_i((_p3cl), "UIDL", (_pm)))

/* POP3 support API */
H pop3_parse_res_a(const VB *buf, UW *msg, VB *str);
H pop3_parse_res_i(const VB *buf, UW *msg, UW *num);
ER pop3_cnv_mail(T_POP3_MAIL *mail, VB *buf);
ER pop3_cnv_part(T_POP3_PART *part, VB *buf, const VB *boundary);
ER pop3_qpdec_body(VB *dst, const VB *src);
ER pop3_b64dec_hdr(VB *dst, const VB *src, VB *charset);
ER pop3_b64dec_body(VB *dst, const VB *src, UB chk);

#ifdef __cplusplus
}
#endif
#endif /* POP3_CLIENT_H */
