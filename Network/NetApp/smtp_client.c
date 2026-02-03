/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    SMTP Client
    Copyright (c)  2013-2025, eForce Co., Ltd. All rights reserved.

    Version Information
      2013.10.01: Created
      2014.04.16: Corresponding to transmission of memory area.
                  2 bytes character support of attached file name.
      2014.07.10: Suppressed warning of the GCC compiler.
      2014.10.27: Supported a POP before SMTP authentication.
      2014.11.18: Corrected a following problem:
                    Failure of file transmission function. (smtp_snd_mem)
                    BASE64 conversion failure of of MIME header.
      2014.12.24: Suppressed warning of Zynq GCC compiler.
      2015.08.07: Support the operation without file system. (SMTP_FSYS_NONE)
      2015.12.14: The socket ID replaced SID types
      2016.02.10: Corrected the buffer overflow problem of SMTP login.
      2016.03.07: Corrected a POP before SMTP authentication.
      2016.09.06: Execute static analysis tool to this source.
      2018.03.20: Support SSL connection.
      2018.10.31: Disabled a POP before SMTP authentication.
      2020.02.04: Supported LOGIN authentication. 
      2022.04.22: Supported HTTPS connection using uNet3-TLS.
	  2025.08.04: Added con_ssoc() timeout definition SMTPS_SSOC_TMO.
 ***************************************************************************/

#include "kernel.h"
#include "net_hdr.h"
#include "net_strlib.h"
#include "smtp_client.h"

#ifdef ENA_AUTH_PBS
#include "pop3_client.h"
#endif

#include "md5calc.h"
#include "base64calc.h"

#ifndef SMTPS_SSOC_TMO
#define SMTPS_SSOC_TMO      (10*1000)
#endif

#define SMTP_EXS_CODE       "250"
#define SMTP_EXS_8BIT       "8BITMIME"
#define SMTP_EXS_AUTH       "AUTH"
#define SMTP_EXS_AUTH_PLN   "PLAIN"
#define SMTP_EXS_AUTH_CMD5  "CRAM-MD5"
#define SMTP_EXS_AUTH_DMD5  "DIGEST-MD5"
#define SMTP_EXS_AUTH_LOGIN "LOGIN"
#define SMTP_EXS_STARTTLS   "STARTTLS"

#define SMTP_EX_AUTH_PLN    SMTP_AUTH_PLAIN
#define SMTP_EX_AUTH_CMD5   SMTP_AUTH_CMD5
#define SMTP_EX_AUTH_DMD5   SMTP_AUTH_DMD5
#define SMTP_EX_AUTH_LOGIN  SMTP_AUTH_LOGIN
#define SMTP_EX_8BIT        0x10U
#define SMTP_EX_STARTTLS    0x40U   /* Note: STARTTLS is not supported */

#define SMTP_TMP_BUFSIZE    128     /* The size of the temporary work buffer */

#define SMTP_B64SRC_LEN     57      /* base64 line max characters (raw) */
#define SMTP_B64DST_LEN     76      /* base64 line max characters (encode) */

#define SMTP_LEN_CRLF       2

#define PBS_WAIT_SVR        300     /* POP before SMTP server wait time (ms) */

/* macros */
#define CSTR_LEN(_cstr_)        (sizeof((_cstr_)) - 1U)
#define BASE64_LEN_CALC(_x_)    ((((W)(_x_) * 134) + ((W)(_x_) / 57)) / 100)

#ifndef SMTP_BUF_NOCHK
#define SMTP_BUF_CHK
#endif

#ifndef SMTPC_DBG_ON
#define SMTPC_DBG_PUTS(d,s) (void)0;
#else
extern void SMTPC_DBG_PUTS(const VB *div, const VB *str);
#endif

#ifdef SMTPC_SSL_SUP
#ifdef SSL_SERVER_NODE  /* using uNet3-TLS */
static ER smtpc_con_ssoc(SID sid, T_NODE *host, ID ssnid, TMO tmo)
{
    T_SSL_NODE sn = {0};
    sn.node_type = SSL_CLIENT_NODE;
    sn.host = *host;
    return con_ssoc(sid, &sn, ssnid, tmo);
}
#else
#define smtpc_con_ssoc  con_ssoc
#endif
#endif

static ER smtpc_con_soc(T_SMTP_CLIENT *smtp)
{
    ER ercd;

    /* Set SMTP server connect */
    if (0U == smtp->svr.port) {
        smtp->svr.port = SMTP_DEF_PORT;
        if (0U != (smtp->auth_type & (SMTP_AUTH_EXPLSSL | SMTP_AUTH_IMPLSSL))) {
            smtp->svr.port = (0U != (smtp->auth_type & SMTP_AUTH_EXPLSSL)) ? (UH)SMTP_ALT_PORT : (UH)SMTPS_DEF_PORT ;
        }
    }
    smtp->svr.ver = IP_VER4;    /* Support only IPv4 */

    ercd = E_PAR;
    if (0U == (smtp->auth_type & SMTP_AUTH_ENASSL)) {
        ercd = con_soc(smtp->sid, &smtp->svr, SOC_CLI);
    }
#ifdef SMTPC_SSL_SUP
    else {
        smtp->ssn_id = 0;
        ercd = get_ssid_soc(smtp->sid);
        if (0 == ercd) {
            ercd = smtpc_con_ssoc(smtp->sid, &smtp->svr, SMTPS_SSOC_TMO);
        }
        if (0 < ercd) {
            smtp->ssn_id = ercd;
            ercd = E_OK;
        }
    }
#endif

    return ercd;
}

static ER smtpc_snd_soc(const T_SMTP_CLIENT *smtp, VP data, UH len)
{
    ER ercd;
    ER (*snd)(SID, VP, UH);
    UB *dat;
    UH rlen;

    snd = NULL;
    if (0U == (smtp->auth_type & SMTP_AUTH_ENASSL)) {
        snd = snd_soc;
    }
#ifdef SMTPC_SSL_SUP
    else {
        snd = snd_ssoc;
    }
#endif
    if (NULL == snd) {
        return E_PAR;
    }

    /* Until buffer size is zero, snd data */
    dat = data;
    rlen = len;
    do {
        ercd = snd(smtp->sid, dat, len);
        if (0 > ercd) {
            break;
        }
        else {
            dat += (UH)ercd;
            len -= (UH)ercd;
        }
        if (0U == len) {
            ercd = (ER)rlen;
            break;
        }
    } while (1);

    return ercd;
}

static ER smtpc_rcv_soc(const T_SMTP_CLIENT *smtp, VP data, UH len)
{
    ER ercd;

    ercd = E_PAR;
    if (0U == (smtp->auth_type & SMTP_AUTH_ENASSL)) {
        ercd = rcv_soc(smtp->sid, data, len);
    }
#ifdef SMTPC_SSL_SUP
    else {
        ercd = rcv_ssoc(smtp->sid, data, len);
    }
#endif

    return ercd;
}

static ER smtpc_cls_soc(const T_SMTP_CLIENT *smtp)
{
    ER ercd;

    ercd = E_PAR;
    if (0U == (smtp->auth_type & SMTP_AUTH_ENASSL)) {
        ercd = cls_soc(smtp->sid, SOC_TCP_CLS);
    }
#ifdef SMTPC_SSL_SUP
    else {
        ercd = cls_ssoc(smtp->sid);
    }
#endif

    return ercd;
}


/* Read string number of current line */
static H readline(VB *str, UB *newline)
{
    VB *s;
    UB nl;

    nl = 0U;
    for (s = str; *s != '\0'; ++s) {
        if ((s[-1] == '\r') && (s[0] == '\n')) {
            nl = (UB)SMTP_LEN_CRLF;
            --s;
            break;
        }
    }
    if (NULL != newline) {
        *newline = nl;
    }

    return s - str;
}

/* Target string split by ",", and return first string. */
static VB *get_split_str(VB *str, VB *item)
{
    VB *tmp1;
    VB *tmp2;

    if (NULL == str) {
        tmp2 = NULL;
    }
    else if ('\0' == *str) {
        tmp2 = NULL;
    }
    else {
        /* Find beginning of valid data */
        for (tmp1 = str; ((*tmp1 == ' ') || (*tmp1 == ',')); ++tmp1) {
            ;
        }

        /* Find end of valid data (tmp2 - tmp1) */
        for (tmp2 = tmp1; ((*tmp2 != '\0') && (*tmp2 != ',')); ++tmp2) {
            ;
        }
        net_memcpy(item, tmp1, tmp2 - tmp1);
        item[tmp2 - tmp1] = '\0';
    }

    return tmp2;
}

/* Extract name or address from "name <mail@address>". */
static ER ext_mail_adr(VB **pos, VB *str, B name)
{
    ER ercd;
    VB *tmp1;
    VB *tmp2;
    UB blk;

    ercd = E_OK;

    /* check <...> */
    tmp2 = NULL;
    blk = 0U;
    for (tmp1 = str; *tmp1 != '\0'; ++tmp1) {
        if ('<' == *tmp1) {
            if (0U == (blk & 0x01U)) {
                tmp2 = tmp1;
                blk |= 0x01U;
            }
            else {              /* format error */
                ercd = E_OBJ;
            }
        }
        else if ('>' == *tmp1) {
            if (0U != (blk & 0x01U)) {
                blk <<= 1U;
            }
            else {              /* format error */
                ercd = E_OBJ;
            }
        }
        else {
            ;   /* do nothing */
        }
    }
    if (!((0U == blk) || (0x02U == blk))) {
        ercd = E_OBJ;
    }

    if (E_OK == ercd) {
        if (0 != name) {     /* get name part */
            if (0U == blk) {
                ercd = E_NOEXS; /* invalid address (Not E-mail) */
            }
            else {
                for (tmp1 = str; *tmp1 == ' '; ++tmp1) {
                    ;
                }
                for (tmp2 -= 1; *tmp2 == ' '; --tmp2) {
                    ;
                }
                *pos = tmp1;
                ercd = (tmp2 - tmp1) + 1;
            }

        }
        else {          /* get address part */
            if (0U == blk) {
                *pos = str;
                ercd = (ER)net_strlen(str);
            }
            else {
                tmp1 = tmp2 + 1;
                for (; *tmp2 != '>'; ++tmp2) {
                    ;
                }
                *pos = tmp1;
                ercd = tmp2 - tmp1;
            }
        }
    }

    return ercd;
}

/*------------------------------------------------------------*/

/* Convert DATA-body part */
static VB* cnv_body(VB *dst, VB *src, UH *dlen)
{
    VB *d;
    VB *s;
    UB nl;
    H len;

    d = dst;
    s = src;

    while (1) {
        len = readline(s, &nl);
        len = (H)(len + (H)nl);
        if (0 == len) {
            break;
        }
        if (*dlen < (UH)(len + (H)(d - dst))) {
            break;
        }

        if ('.' == *s) {
            *d++ = '.';
        }
        net_memcpy(d, s, (SIZE)len);
        d += len;
        s += len;
    }

    *dlen = (UH)(d - dst);

    return s;
}

/* DATA header part encode MIME format. */
static ER mime_enc_header(VB *dst, VB *src, VB *charset, H dlen)
{
    ER ercd;
    VB buf[SMTP_TMP_BUFSIZE];
    T_BASE64_INFO b64i;

    *dst = '\0';
    INI_B64I(&b64i);
    SET_B64I(&b64i, buf, src, sizeof(buf), net_strlen(src));
    ercd = base64enc(&b64i);
    if (E_OK == ercd) {
        ercd = b64i.rdlen;
        ercd += (ER)CSTR_LEN("=?" "?B?" "?=");
        ercd += (ER)net_strlen(charset);
        ercd += (ER)net_strlen(buf);
        if (ercd > (ER)dlen) {
            ercd = E_NOMEM;
        }
        else {
            buf[b64i.rdlen] = '\0';
            net_strcat(dst, "=?");
            net_strcat(dst, charset);
            net_strcat(dst, "?B?");
            net_strcat(dst, buf);
            net_strcat(dst, "?=");
            ercd = E_OK;
        }
    }

    return ercd;
}

/* Mail address encode MIME format. */
static ER mime_enc_mladr(VB *dst, VB *src, VB *charset, H dlen)
{
    ER ercd;
    VB *pos;
    VB buf[SMTP_TMP_BUFSIZE];
    T_BASE64_INFO b64i;

    *dst = '\0';
    ercd = ext_mail_adr(&pos, src, 1);
    if (0 < ercd) {
        INI_B64I(&b64i);
        SET_B64I(&b64i, buf, pos, sizeof(buf), ercd);
        ercd = base64enc(&b64i);
        if (E_OK == ercd) {
            ercd = b64i.rdlen;
            ercd += (ER)CSTR_LEN("=?" "?B?" "?=");
            ercd += (ER)net_strlen(charset);
            ercd += (ER)net_strlen(buf);
            if (ercd > (ER)dlen) {
                ercd = E_NOMEM;
            }
            else {
                buf[b64i.rdlen] = '\0';
                net_strcat(dst, "=?");
                net_strcat(dst, charset);
                net_strcat(dst, "?B?");
                net_strcat(dst, buf);
                net_strcat(dst, "?= ");
                ercd = E_OK;
            }
        }
        if (E_OK != ercd) {
            return ercd;
        }
    }

    ercd = ext_mail_adr(&pos, src, 0);
    if (0 < ercd) {
        net_strcat(dst, "<");
        net_strncat(dst, pos, (SIZE)ercd);
        net_strcat(dst, ">");
        ercd = E_OK;
    }

    return ercd;
}

/* Target file encode BASE64, and snd */
static ER smtp_snd_file(const T_SMTP_CLIENT *smtp, const VB *path)
{
#ifdef SMTP_FSYS_NONE
    return E_NOSPT;
#else
    FILE *fp;
    T_BASE64_INFO b64i;
    VB buf[SMTP_B64SRC_LEN];
    UB pos;
    ER ercd;

    fp = fopen(path, "rb");
    if (NULL == fp) {
        return E_OBJ;
    }

    pos = 0U;
    ercd = E_OK;
    do {
        INI_B64I(&b64i);
        SET_B64I(&b64i, smtp->buf, buf, smtp->len, 0);

        /* Read file 57byte length -> encode base64 */
        do {
            b64i.slen = (H)fread(buf, 1U, (SIZE)SMTP_B64SRC_LEN, fp);

            ercd = mime_base64enc(&b64i, &pos);
            b64i.dst += b64i.rdlen;
            b64i.dlen -= b64i.rdlen;
        } while ((b64i.slen == SMTP_B64SRC_LEN) && (b64i.dlen >= SMTP_B64DST_LEN));

        /* Send base64 data */
        ercd = smtpc_snd_soc(smtp, smtp->buf, (b64i.dst - smtp->buf));
        if (0 > ercd) {
            break;
        }

    } while (b64i.slen == SMTP_B64SRC_LEN);

    (void)fclose(fp);

    return ercd;
#endif
}


/* Target memory encode BASE64, and snd */
static ER smtp_snd_mem(const T_SMTP_CLIENT *smtp, VB *mem, UW len)
{
    T_BASE64_INFO b64i;
    UB pos;
    ER ercd;

    pos = 0U;
    ercd = E_OK;
    do {
        INI_B64I(&b64i);
        SET_B64I(&b64i, smtp->buf, mem, smtp->len, 0);

        /* Read memory 57byte length -> encode base64 */
        do {
            b64i.src  = mem;
            if ((UW)SMTP_B64SRC_LEN < len) {
                b64i.slen = SMTP_B64SRC_LEN;
            }
            else {
                b64i.slen = (H)len;
            }
            ercd = mime_base64enc(&b64i, &pos);
            b64i.dst += b64i.rdlen;
            b64i.dlen -= b64i.rdlen;
            mem += b64i.slen;
            len -= (UW)b64i.slen;
        } while ((b64i.slen == SMTP_B64SRC_LEN) && (b64i.dlen >= SMTP_B64DST_LEN));

        /* Send base64 data */
        ercd = smtpc_snd_soc(smtp, smtp->buf, (b64i.dst - smtp->buf));
        if (0 > ercd) {
            break;
        }

    } while (b64i.slen == SMTP_B64SRC_LEN);

    return ercd;
}



/* AUTH PLAIN function */
static ER smtp_auth_plain(T_SMTP_CLIENT *smtp)
{
    ER ercd;
    VB buf[SMTP_TMP_BUFSIZE];
    VB *tmp;
    T_BASE64_INFO b64i;

    do {
        /* check buffer size */
        ercd = (ER)(net_strlen(smtp->usr) + net_strlen(smtp->pw) + net_strlen("\0\0") + 1U);
        if ((ER)sizeof(buf) < ercd) {
            ercd = E_NOMEM;
            break;
        }

        net_strcpy(smtp->buf, "AUTH PLAIN\r\n");
        ercd = smtp_snd_cmd(smtp, smtp->buf, "334");
        if (0 > ercd) {
            break;
        }

        /* "\0userid\0password" encode base64 */
        tmp = buf;
        *tmp++ = '\0';
        net_strcpy(tmp, smtp->usr);
        tmp += (W)net_strlen(tmp) + 1;
        net_strcpy(tmp, smtp->pw);
        tmp += net_strlen(tmp);
        INI_B64I(&b64i);
        SET_B64I(&b64i, smtp->buf, buf, smtp->len, (tmp - buf));
        ercd = base64enc(&b64i);
        if (E_OK != ercd) {
            break;
        }

        smtp->buf[b64i.rdlen] = '\0';
        net_strcat(smtp->buf, "\r\n");
        ercd = smtp_snd_cmd(smtp, smtp->buf, "235");
        if (0 < ercd) {
            ercd = E_OK;
        }
    } while (0);

    return ercd;
}

/* AUTH LOGIN function */
static ER smtp_auth_login(T_SMTP_CLIENT *smtp)
{
    ER ercd;
    VB buf[SMTP_TMP_BUFSIZE];
    VB *tmp;
    T_BASE64_INFO b64i;

    do {
        /* check buffer size */
        ercd = (ER)(net_strlen(smtp->usr) + 1U);
        if ((ER)sizeof(buf) < ercd) {
            ercd = E_NOMEM;
            break;
        }
        ercd = (ER)(net_strlen(smtp->pw) + 1U);
        if ((ER)sizeof(buf) < ercd) {
            ercd = E_NOMEM;
            break;
        }

        net_strcpy(smtp->buf, "AUTH LOGIN\r\n");
        ercd = smtp_snd_cmd(smtp, smtp->buf, "334");
        if (0 > ercd) {
            break;
        }

        /* "userid" encode base64 */
        tmp = buf;
        net_strcpy(tmp, smtp->usr);
        tmp += net_strlen(tmp);
        INI_B64I(&b64i);
        SET_B64I(&b64i, smtp->buf, buf, smtp->len, (tmp - buf));
        ercd = base64enc(&b64i);
        if (E_OK != ercd) {
            break;
        }
        smtp->buf[b64i.rdlen] = '\0';
        net_strcat(smtp->buf, "\r\n");
        ercd = smtp_snd_cmd(smtp, smtp->buf, "334");
        if (0 > ercd) {
            break;
        }
        
        /* "password" encode base64 */
        tmp = buf;
        net_strcpy(tmp, smtp->pw);
        tmp += net_strlen(tmp);
        INI_B64I(&b64i);
        SET_B64I(&b64i, smtp->buf, buf, smtp->len, (tmp - buf));
        ercd = base64enc(&b64i);
        if (E_OK != ercd) {
            break;
        }
        smtp->buf[b64i.rdlen] = '\0';
        net_strcat(smtp->buf, "\r\n");
        ercd = smtp_snd_cmd(smtp, smtp->buf, "235");
        if (0 < ercd) {
            ercd = E_OK;
        }
    } while (0);

    return ercd;
}

/* AUTH CRAM-MD5 function */
static ER smtp_auth_cmd5(T_SMTP_CLIENT *smtp)
{
    ER ercd;
    VB buf[SMTP_TMP_BUFSIZE];
    VB *tmp;
    HASH HA;
    T_BASE64_INFO b64i;

    do {
        /* check buffer size */
        ercd = (ER)(net_strlen(smtp->usr) + HASHLEN + 1U);
        if ((ER)sizeof(buf) < ercd) {
            ercd = E_NOMEM;
            break;
        }

        net_strcpy(smtp->buf, "AUTH CRAM-MD5\r\n");
        ercd = smtp_snd_cmd(smtp, smtp->buf, "334");
        if (0 > ercd) {
            break;
        }
        tmp = smtp->buf + CSTR_LEN("334 "); /* Cued challenge code (BASE64) */

        /* Recv data decode BASE64. */
        INI_B64I(&b64i);
        SET_B64I(&b64i, buf, tmp, sizeof(buf), readline(tmp, 0));
        ercd = base64dec(&b64i);
        if (E_OK != ercd) {
            break;
        }

        /* Decode data and SMTP password encode HMAC-MD5. */
        hmac_md5((UB *)buf, (W)b64i.rdlen, (UB *)smtp->pw, (W)net_strlen(smtp->pw), &HA);

        /* SMTP username and HMAC-MD5 data encode BASE64. */
        tmp = smtp->buf;
        net_strcpy(tmp, smtp->usr);
        tmp += net_strlen(smtp->usr);
        *tmp++ = ' ';
        cnv_hex(HA, tmp);
        tmp += (H)sizeof(HASHHEX) - 1;
        net_memcpy(buf, smtp->buf, tmp - smtp->buf);

        /* "username {HMAC-MD5 data}" encode BASE64. */
        SET_B64I(&b64i, smtp->buf, buf, smtp->len, tmp - smtp->buf);
        ercd = base64enc(&b64i);
        if (E_OK != ercd) {
            break;
        }
        smtp->buf[b64i.rdlen] = '\0';

        /* Send BASE64 data */
        net_strcat(smtp->buf, "\r\n");
        ercd = smtp_snd_cmd(smtp, smtp->buf, "235");
        if (0 < ercd) {
            ercd = E_OK;
        }
    } while (0);

    return ercd;
}

#ifdef ENA_AUTH_PBS
/* AUTH POP before SMTP function */
static ER smtp_auth_pbs(T_SMTP_CLIENT *smtp)
{
    ER ercd;
    T_POP3_CLIENT p3c;

    net_memset(&p3c, 0, sizeof(p3c));

    p3c.svr.ipa     = smtp->svr.ipa;
    p3c.svr.num     = smtp->svr.num;
    p3c.sid     = smtp->sid;
    p3c.svr.port    = 0;
    p3c.buf     = smtp->buf;
    p3c.len     = smtp->len;
    p3c.auth_type =
        (SMTP_AUTH_PBS_APOP  & smtp->auth_type) ? POP3_AUTH_APOP :
        (SMTP_AUTH_PBS_PLAIN & smtp->auth_type) ? POP3_AUTH_PLAIN :
        0
    ;
    p3c.usr     = smtp->usr;
    p3c.pw      = smtp->pw;

    cfg_soc(smtp->sid, SOC_TMO_SND, (VP)SMTP_TMO_INIT);
    cfg_soc(smtp->sid, SOC_TMO_RCV, (VP)SMTP_TMO_INIT);
    ercd = pop3_login(&p3c);
    pop3_quit(&p3c);

    return ercd;
}
#endif

/* EHLO/HELO communication to server */
static ER smtp_ehlo_comm(T_SMTP_CLIENT *smtp)
{
    T_NODE host;
    ER ercd;
    VB *tmp;
    B next_ext;

    (void)cfg_soc(smtp->sid, SOC_TMO_SND, (VP)SMTP_TMO_CMD);
    (void)cfg_soc(smtp->sid, SOC_TMO_RCV, (VP)SMTP_TMO_CMD);

    net_memset(&host, 0, sizeof(host));
    (void)ref_soc(smtp->sid, SOC_IP_LOCAL, (VP)&host);

    smtp->flag = 0U;
    net_strcpy(smtp->buf, "EHLO [");
    ercd = (ER)net_strlen(smtp->buf);
    ip_ntoa(&smtp->buf[ercd], host.ipa);
    net_strcat(smtp->buf, "]\r\n");
    ercd = smtp_snd_cmd(smtp, smtp->buf, "250");
    if (0 > ercd) {
        /* 50x is EHLO no support? retry HELO command. */
        if ((0 == net_memcmp(smtp->buf, "50", 2U)) && (SMTP_NO_AUTH == smtp->auth_type)) {
            net_strcpy(smtp->buf, "HELO [");
            ercd = (ER)net_strlen(smtp->buf);
            ip_ntoa(&smtp->buf[ercd], host.ipa);
            net_strcat(smtp->buf, "]\r\n");
            ercd = smtp_snd_cmd(smtp, smtp->buf, "250");
        }
    }
    else {
        /* Get support SMTP Service Extension */
        tmp = smtp->buf;
        next_ext = 0;
        do {
            if ('\0' == *tmp) {
                ercd = smtpc_rcv_soc(smtp, (VP)smtp->buf, smtp->len);
                if (0 >= ercd) {
                    ercd = E_CLS;
                    break;
                }
                smtp->buf[ercd] = '\0';
                tmp = smtp->buf;
            }
            if (0 == net_strncmp(tmp, SMTP_EXS_CODE, CSTR_LEN(SMTP_EXS_CODE))) {
                tmp += CSTR_LEN(SMTP_EXS_CODE);
                next_ext = (B)((*tmp == '-') ? 1 : 0);     /* Exist next extension ? */
                tmp++;

                if (0 == net_strncmp(tmp, SMTP_EXS_8BIT, CSTR_LEN(SMTP_EXS_8BIT))) {
                    smtp->flag |= SMTP_EX_8BIT;
                }
                else if (0 == net_strncmp(tmp, SMTP_EXS_AUTH, CSTR_LEN(SMTP_EXS_AUTH))) {
                    for (tmp += CSTR_LEN(SMTP_EXS_AUTH); *tmp++ == ' '; ) {
                        if (0 == net_strncmp(tmp, SMTP_EXS_AUTH_PLN, CSTR_LEN(SMTP_EXS_AUTH_PLN))) {
                            smtp->flag |= SMTP_EX_AUTH_PLN;
                        }
                        else if (0 == net_strncmp(tmp, SMTP_EXS_AUTH_LOGIN, CSTR_LEN(SMTP_EXS_AUTH_LOGIN))) {
                            smtp->flag |= SMTP_EX_AUTH_LOGIN;
                        }
                        else if (0 == net_strncmp(tmp, SMTP_EXS_AUTH_CMD5, CSTR_LEN(SMTP_EXS_AUTH_CMD5))) {
                            smtp->flag |= SMTP_EX_AUTH_CMD5;
                        }
                        else if (0 == net_strncmp(tmp, SMTP_EXS_AUTH_DMD5, CSTR_LEN(SMTP_EXS_AUTH_DMD5))) {
                            smtp->flag |= SMTP_EX_AUTH_DMD5;
                        }
                        else {  /* other authenticate type */
                            ;   /* no support */
                        }

                        while ((*tmp != ' ') && (*tmp != '\r')) {
                            ++tmp;
                        }
                    }
                }
                else if (0 == net_strncmp(tmp, SMTP_EXS_STARTTLS, CSTR_LEN(SMTP_EXS_STARTTLS))) {
                    smtp->flag |= SMTP_EX_STARTTLS;
                }
                else {  /* other extension */
                    ;   /* do nothing */
                }
            }
            if (0 != next_ext) {
                while (*tmp++ != '\n') {
                    ;   /* Cued Next */
                }
            }
        } while (0 != next_ext);
    }
    if (0 < ercd) {
        ercd = E_OK;
    }

    return ercd;
}

/* Connect SMTP server */
ER smtp_login(T_SMTP_CLIENT *smtp)
{
    ER ercd;
    UB atype;

    if (NULL == smtp) {
        ercd = E_OBJ;
    }
    else if ((0U == smtp->sid) || (NULL == smtp->buf) || (0U == smtp->len)) {
        ercd = E_PAR;
    }
    else if ((0U == smtp->svr.ipa) || ((UH)NET_DEV_MAX < smtp->svr.num)) {
        ercd = E_PAR;
    }
#ifndef SMTPC_SSL_SUP
    if (0U != (smtp->auth_type & (SMTP_AUTH_EXPLSSL | SMTP_AUTH_IMPLSSL))) {
        ercd = E_PAR;
    }
#endif
    else {
        ercd = E_OK;
    }
    if (E_OK != ercd) {
        return ercd;
    }

#ifdef ENA_AUTH_PBS
    if ( (SMTP_AUTH_PBS_PLAIN & smtp->auth_type)
      || (SMTP_AUTH_PBS_APOP  & smtp->auth_type) ) {
        ercd = smtp_auth_pbs(smtp);

        if (E_OK != ercd) {
            return ercd;
        }
        dly_tsk(PBS_WAIT_SVR);   /* wait SMTP server connect */
    }
#endif

    smtp->auth_type &= ~SMTP_AUTH_ENASSL;
    if (0U != (smtp->auth_type & SMTP_AUTH_IMPLSSL)) {
        smtp->auth_type |= SMTP_AUTH_ENASSL;
    }

    ercd = smtpc_con_soc(smtp);
    if (E_OK == ercd) {
        (void)cfg_soc(smtp->sid, SOC_TMO_RCV, (VP)SMTP_TMO_INIT);
        ercd = smtpc_rcv_soc(smtp, (VP)smtp->buf, smtp->len);
    }
    if (0 < ercd) {
        if (0 != net_memcmp(smtp->buf, "220", 3U)) {
            ercd = E_SYS;   /* response error */
        }
    }
    else {
        ercd = E_CLS;       /* socket operation error */
    }

    /* Login SMTP server (SMTP Authenticate) */
    if (0 < ercd) {
        ercd = smtp_ehlo_comm(smtp);    /* send EHLO (or HELO). */

#ifdef SMTPC_SSL_SUP
        if (E_OK == ercd) {
            if (0U != (smtp->auth_type & SMTP_AUTH_EXPLSSL)) {
                if (0U != (smtp->flag & SMTP_EX_STARTTLS)) {
                    net_strcpy(smtp->buf, "STARTTLS\r\n");
                    ercd = smtp_snd_cmd(smtp, smtp->buf, "220");
                    if (0 <= ercd) {
                        smtp->auth_type |= SMTP_AUTH_ENASSL;
                        ercd = smtpc_con_soc(smtp);
                        if (E_OK != ercd) {
                            cls_ssoc(smtp->sid);
                        }
                        else {
                            ercd = smtp_ehlo_comm(smtp);    /* resend EHLO (or HELO). */
                        }
                    }
                }
                else {
                    ercd = E_NOSPT;     /* SSL connection no support (server specs) */
                }
            }
        }
#endif

        if (E_OK == ercd) {
            atype = (smtp->auth_type & SMTP_AUTH_TYPE_MASK);
            atype &= smtp->flag;
            
            /* Check processing possible types of authentication */
            if (SMTP_NO_AUTH == atype) {
                if (0U != (smtp->auth_type & SMTP_AUTH_TYPE_MASK)) {
                    ercd = E_NOSPT;     /* no matching authentication type */
                }
                else {
                    ercd = E_OK;        /* specify no authentication */
                }
            }
            else if (0U != (atype & SMTP_EX_AUTH_CMD5)) {
                ercd = smtp_auth_cmd5(smtp);
            }
            else if (0U != (atype & SMTP_EX_AUTH_PLN)) {
                ercd = smtp_auth_plain(smtp);
            }
            else if (0U != (atype & SMTP_EX_AUTH_LOGIN)) {
                ercd = smtp_auth_login(smtp);
            }
            else {
                ercd = E_NOSPT;         /* unknown branch */
            }
        }
    }

    return ercd;
}

/* Disconnect SMTP server */
ER smtp_quit(const T_SMTP_CLIENT *smtp)
{
    ER ercd;
    if (NULL == smtp) {
        ercd = E_OBJ;
    }
    else {
        (void)smtp_snd_cmd(smtp, "QUIT\r\n", "221");
        (void)smtpc_cls_soc(smtp);
        ercd = E_OK;
    }

    return ercd;
}

/* Send SMTP command, and Receive response code */
ER smtp_snd_cmd(const T_SMTP_CLIENT *smtp, const VB *cmd, const VB *res)
{
    ER ercd;

    if (NULL == smtp) {
        ercd = E_OBJ;
    }
    else if ((NULL == cmd) || (NULL == res)) {
        ercd = E_PAR;
    }
    else if (0U == smtp->sid) {
        ercd = E_PAR;
    }
    else {
        SMTPC_DBG_PUTS("send", cmd);
        ercd = smtpc_snd_soc(smtp, (VP)cmd, (UH)net_strlen(cmd));
        if (0 < ercd) {
            ercd = smtpc_rcv_soc(smtp, (VP)smtp->buf, smtp->len);
        }
        if (0 < ercd) {
            smtp->buf[ercd] = '\0';
            SMTPC_DBG_PUTS("recv", smtp->buf);
            if (0 != net_memcmp(smtp->buf, (VP)res, net_strlen(res))) {
                ercd = E_SYS;
            }
            else {
                ercd = E_OK;
            }
        }
        else {
            ercd = E_CLS;
        }
    }

    return ercd;
}


static void smtp_snd_abort(const T_SMTP_CLIENT *smtp)
{
    (void)smtp_snd_cmd(smtp, "RSET\r\n", "250");    /* aborted */
}

static UB smtp_none_addr(const VB *addr)
{
    UB ret = 1U;

    if (NULL != addr) {
        while (*addr == ' ') {
            addr++;
        }
        if (*addr != '\0') {
            ret = 0U;
        }
    }

    return ret;
}


/* Send Mail */
ER smtp_snd_mail(const T_SMTP_CLIENT *smtp, const T_SMTP_MAIL *mail)
{
    ER ercd;
    VB *rcpt[3];
    VB *rphdr[] = { "To:", "Cc:", "Bcc:" };
    VB *pos;
    VB *spos;
    VB buf[SMTP_TMP_BUFSIZE];
    UH len;
    UH tlen;
    UB nx;
    B ascii;
    T_BASE64_INFO b64i;
    T_SMTP_FILE *smtp_file;

    /* check parameter */
    ercd = E_OK;
    if (NULL == smtp) {
        ercd = E_OBJ;
    }
    else if ((0U == smtp->sid) || (NULL == smtp->buf) || (0U == smtp->len)) {
        ercd = E_PAR;
    }
    else if (NULL == mail) {
        ercd = E_PAR;
    }
    else if ((NULL == mail->from) || (NULL == mail->to)) {
        ercd = E_PAR;
    }
    else if ((0U != mail->cs8bit) && (NULL == mail->charset)) {
        ercd = E_PAR;
    }
    else {
        for (smtp_file = mail->file; NULL != smtp_file; smtp_file = smtp_file->next) {
            if (0U == (smtp_file->type & (SMTP_FTYP_FS | SMTP_FTYP_MEM))) {
                ercd = E_PAR;
                break;
            }
            if ((!smtp_file->buf) || (!smtp_file->name)) {
                ercd = E_PAR;
                break;
            }
        }
    }
    if (E_OK != ercd) {
        return ercd;
    }

    rcpt[0] = mail->to;
    rcpt[1] = mail->cc;
    rcpt[2] = mail->bcc;
    ascii = (B)(((NULL == mail->charset) || (0 == net_strcmp(mail->charset, "US-ASCII"))) ? 1 : 0 );

#ifdef SMTP_BUF_CHK     /* check buffer length */
    /*----  DATA    ----*/
    len = 0U;

    /* DATA-Header - From */
    len += CSTR_LEN("From: ");
    tlen = (UH)net_strlen(mail->from);
    tlen += (UH)net_strlen(mail->charset);
    if (0 != ascii) {
        len += tlen;
    }
    else {
        len += (UH)BASE64_LEN_CALC(tlen);
    }
    len += CSTR_LEN("\r\n");

    /* DATA-Header - To, Cc, Bcc */
    for (nx = 0U; nx < (UB)(sizeof(rcpt)/sizeof(rcpt[0])); ++nx) {
        if (0U != smtp_none_addr(rcpt[nx])) {
            continue;
        }
        len += (UH)net_strlen(rphdr[nx]);
        tlen = (UH)net_strlen(rcpt[nx]);
        tlen += (UH)net_strlen(mail->charset);
        if (0 != ascii) {
            len += tlen;
        }
        else {
            len += (UH)BASE64_LEN_CALC(tlen);
        }
        len += CSTR_LEN("\r\n");
    }

    /* DATA-Header - Date */
    if (NULL != mail->date) {
        len += CSTR_LEN("Date: ");
        len += (UH)net_strlen(mail->date);
        len += CSTR_LEN("\r\n");
    }

    /* DATA-Header - Subject */
    if (NULL != mail->subject) {
        len += CSTR_LEN("Subject: ");
        net_strcat(smtp->buf, "Subject: ");
        tlen = (UH)net_strlen(mail->subject);
        if (0 != ascii) {
            len += tlen;
        }
        else {
            len += (UH)BASE64_LEN_CALC(tlen);
        }
        len += CSTR_LEN("\r\n");
    }

    /* DATA-MIME use? */
    if ((0 == ascii) || (NULL != mail->file) || (0U != mail->cs8bit)) {
        len += CSTR_LEN("MIME-Version: 1.0\r\n");
        if (NULL != mail->file) {
            len += CSTR_LEN("Content-Type: multipart/mixed; boundary=\"");
            len += CSTR_LEN(SMTP_MIME_BND_HEAD "\"\r\n");
            len += CSTR_LEN("Content-Transfer-Encoding: 7bit\r\n");
            len += CSTR_LEN("\r\n");
        }
    }
    if (NULL != mail->body) {
        if (NULL != mail->file) {    /* start of multi-part */
            len += CSTR_LEN("\r\n");
            len += CSTR_LEN("--" SMTP_MIME_BND_HEAD "\r\n");
        }
        if (0 != ascii) {
            len += CSTR_LEN("Content-Type: text/plain; charset=");
            len += (UH)net_strlen(mail->charset);
            len += CSTR_LEN("\r\n");
        }
        if (0U != mail->cs8bit) {
            len += CSTR_LEN("Content-Transfer-Encoding: base64\r\n");
        }
    }
    len += CSTR_LEN("\r\n");

    if (len > smtp->len) {
        return E_NOMEM;
    }
#endif

    ercd = cfg_soc(smtp->sid, SOC_TMO_SND, (VP)SMTP_TMO_CMD);
    if (E_OK == ercd) {
        ercd = smtp_snd_cmd(smtp, "RSET\r\n", "250");
    }
    if (E_OK != ercd) {
        return ercd;
    }

    /*----  MAIL FROM    ----*/
    (void)cfg_soc(smtp->sid, SOC_TMO_RCV, (VP)SMTP_TMO_MAIL);
    spos = get_split_str(mail->from, buf);
    ercd = ext_mail_adr(&pos, buf, 0);
    if (0 <= ercd) {
        net_strcpy(smtp->buf, "MAIL FROM:<");
        net_strncat(smtp->buf, pos, (SIZE)ercd);
        net_strcat(smtp->buf, ">\r\n");
        ercd = smtp_snd_cmd(smtp, smtp->buf, "250");
    }
    if (E_OK != ercd) {
        goto _smtp_cmd_abort;
    }

    /*----  RCPT TO (To, Cc, Bcc)    ----*/
    (void)cfg_soc(smtp->sid,SOC_TMO_RCV, (VP)SMTP_TMO_RCPT);
    for (nx = 0U; nx < (UB)(sizeof(rcpt)/sizeof(rcpt[0])); ++nx) {
        if (0U != smtp_none_addr(rcpt[nx])) {
            continue;
        }
        len = 0U;
        spos = rcpt[nx];
        while (1) {
            spos = get_split_str(spos, buf);
            if (NULL == spos) {
                if (0U < len) {
                    break;
                }
                else {  /* not found valid address */
                    ercd = E_SYS;
                    goto _smtp_cmd_abort;
                }
            }
            ercd = ext_mail_adr(&pos, buf, 0);
            if (0 <= ercd) {
                net_strcpy(smtp->buf, "RCPT TO:<");
                net_strncat(smtp->buf, pos, (SIZE)ercd);
                net_strcat(smtp->buf, ">\r\n");
                ercd = smtp_snd_cmd(smtp, smtp->buf, "250");
            }
            if (E_OK != ercd) {
                goto _smtp_cmd_abort;
            }
            len++;
        }
    }

    /*----  DATA    ----*/
    (void)cfg_soc(smtp->sid, SOC_TMO_RCV, (VP)SMTP_TMO_DATA1);
    ercd = smtp_snd_cmd(smtp, "DATA\r\n", "354");
    if (E_OK != ercd) {
        goto _smtp_cmd_abort;
    }
    (void)cfg_soc(smtp->sid, SOC_TMO_SND, (VP)SMTP_TMO_DATA2);

    /* DATA-Header - From */
    net_strcpy(smtp->buf, "From: ");
    get_split_str(mail->from, buf);
    if (0 != ascii) {
        net_strcat(smtp->buf, buf);
    }
    else {
        len = (UH)net_strlen(smtp->buf);
        ercd = mime_enc_mladr(&smtp->buf[len], buf, mail->charset, smtp->len - len);
        if (E_OK != ercd) {
            goto _smtp_cmd_abort;
        }
    }
    net_strcat(smtp->buf, "\r\n");


    /* DATA-Header - To, Cc, Bcc */
    for (nx = 0U; nx < (UB)(sizeof(rcpt)/sizeof(rcpt[0])); ++nx) {
        if (0U != smtp_none_addr(rcpt[nx])) {
            continue;
        }
        net_strcat(smtp->buf, rphdr[nx]);

        spos = rcpt[nx];
        while (1) {
            spos = get_split_str(spos, buf);
            if (NULL == spos) {
                break;
            }
            net_strcat(smtp->buf, " ");
            if (0 != ascii) {
                net_strcat(smtp->buf, buf);
            }
            else {
                len = (UH)net_strlen(smtp->buf);
                ercd = mime_enc_mladr(&smtp->buf[len], buf, mail->charset, smtp->len - len);
                if (E_OK != ercd) {
                    goto _smtp_cmd_abort;
                }
            }

            if ('\0' != *spos) {
                net_strcat(smtp->buf, ",");
            }
            net_strcat(smtp->buf, "\r\n");
        }
    }

    /* DATA-Header - Date */
    if (NULL != mail->date) {
        net_strcat(smtp->buf, "Date: ");
        net_strcat(smtp->buf, mail->date);
        net_strcat(smtp->buf, "\r\n");
    }

    /* DATA-Header - Subject */
    if (NULL != mail->subject) {
        net_strcat(smtp->buf, "Subject: ");
        if (0 != ascii) {
            net_strcat(smtp->buf, mail->subject);
        }
        else {
            len = (UH)net_strlen(smtp->buf);
            ercd = mime_enc_header(&smtp->buf[len], mail->subject, mail->charset, smtp->len - len);
            if (E_OK != ercd) {
                goto _smtp_cmd_abort;
            }
        }
        net_strcat(smtp->buf, "\r\n");
    }

    /* DATA-MIME use? */
    if ((0 == ascii) || (NULL != mail->file) || (0U != mail->cs8bit)) {
        net_strcat(smtp->buf, "MIME-Version: 1.0\r\n");
        if (NULL != mail->file) {
            net_strcat(smtp->buf, "Content-Type: multipart/mixed; boundary=\"");
            net_strcat(smtp->buf, SMTP_MIME_BND_HEAD "\"\r\n");
            net_strcat(smtp->buf, "Content-Transfer-Encoding: 7bit\r\n");
            net_strcat(smtp->buf, "\r\n");
        }
    }
    if (NULL != mail->body) {
        if (NULL != mail->file) {    /* start of multi-part */
            net_strcat(smtp->buf, "\r\n");
            net_strcat(smtp->buf, "--" SMTP_MIME_BND_HEAD "\r\n");
        }
        if (0 == ascii) {
            net_strcat(smtp->buf, "Content-Type: text/plain; charset=");
            net_strcat(smtp->buf, mail->charset);
            net_strcat(smtp->buf, "\r\n");
        }
        if (0U != mail->cs8bit) {
            net_strcat(smtp->buf, "Content-Transfer-Encoding: base64\r\n");
        }
    }
    net_strcat(smtp->buf, "\r\n");          /* Border of header and data */
    ercd = smtpc_snd_soc(smtp, (VP)smtp->buf, (UH)net_strlen(smtp->buf));
    if (0 > ercd) {
        return ercd;
    }

    /* DATA-Body */
    if (NULL != mail->body) {
        if (0U != mail->cs8bit) {     /* BASE64 */
            nx = 0U;
            pos = mail->body;
            while ('\0' != *pos) {
                INI_B64I(&b64i);
                SET_B64I(&b64i, smtp->buf, pos, smtp->len, net_strlen(pos));
                ercd = mime_base64enc(&b64i, &nx);
                if (E_OK == ercd) {
                    pos += b64i.rslen;
                    ercd = smtpc_snd_soc(smtp, (VP)smtp->buf, (UH)b64i.rdlen);
                }
                if (0 > ercd) {
                    return ercd;
                }
            }
        }
        else {                  /* 7bit */
            pos = mail->body;
            while ('\0' != *pos) {
                len = smtp->len;
                pos = cnv_body(smtp->buf, pos, &len);
                ercd = smtpc_snd_soc(smtp, (VP)smtp->buf, len);
                if (0 > ercd) {
                    return ercd;
                }
            }
        }
    }
    net_strcpy(smtp->buf, "");

    /* multipart/mixed */
    if (NULL != mail->file) {
        for (smtp_file = mail->file; NULL != smtp_file; smtp_file = smtp_file->next) {
            /* HEADER */
            net_strcpy(smtp->buf, "\r\n");
            net_strcat(smtp->buf, "--" SMTP_MIME_BND_HEAD "\r\n");
            net_strcat(smtp->buf, "Content-Type: application/octet-stream;\r\n name=\"");
            if (0 != ascii) {
                net_strcat(smtp->buf, smtp_file->name);
            }
            else {
                len = (UH)net_strlen(smtp->buf);
                ercd = mime_enc_header(&smtp->buf[len], smtp_file->name, mail->charset, smtp->len - len);
            }
            net_strcat(smtp->buf, "\"\r\n");
            net_strcat(smtp->buf, "Content-Disposition: attachment;\r\n filename=\"");
            if (0 != ascii) {
                net_strcat(smtp->buf, smtp_file->name);
            }
            else {
                len = (UH)net_strlen(smtp->buf);
                ercd = mime_enc_header(&smtp->buf[len], smtp_file->name, mail->charset, smtp->len - len);
            }
            net_strcat(smtp->buf, "\"\r\n");
            net_strcat(smtp->buf, "Content-Transfer-Encoding: base64\r\n");

            net_strcat(smtp->buf, "\r\n");          /* Border of header and data */
            ercd = smtpc_snd_soc(smtp, (VP)smtp->buf, (UH)net_strlen(smtp->buf));
            if (0 > ercd) {
                return ercd;
            }

            /* DATA */
            if (0U != (smtp_file->type & SMTP_FTYP_FS)) {
                ercd = smtp_snd_file(smtp, smtp_file->buf);
            }
            else {
                ercd = smtp_snd_mem(smtp, smtp_file->buf, smtp_file->len);
            }
            if (0 > ercd) {
                return ercd;
            }
        }

        net_strcpy(smtp->buf, "\r\n--" SMTP_MIME_BND_HEAD "--\r\n");
    }

    /*----  DATA (EOF)    ----*/
    net_strcat(smtp->buf, "\r\n.\r\n");     /* End of the data */
    (void)cfg_soc(smtp->sid, SOC_TMO_RCV, (VP)SMTP_TMO_DATA3);
    ercd = smtp_snd_cmd(smtp, smtp->buf, "250");

_smtp_cmd_abort:
    if (E_OK != ercd) {
        smtp_snd_abort(smtp);
    }
    return ercd;
}
