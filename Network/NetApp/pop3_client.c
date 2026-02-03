/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    POP3 Client
    Copyright (c)  2014-2025, eForce Co., Ltd. All rights reserved.

    Version Information
      2014.10.31: Created
      2015.03.16: Suppressed warning of the GCC compiler.
      2015.08.07: Support the operation without file system. (POP3_FSYS_NONE)
      2015.09.04: Fixed an exception when auth is different of a server and client.
      2016.08.24: Corrected the following problems.
        1. Execute static analysis tool to this source.
        2. Fixed the BASE64 decoding process of the MIME header.
        3. Supported the decoding for quoted-printable.
      2017.07.27: Support 64bit processor
      2018.03.20: Support SSL connection.
      2019.11.06: Suppressed warning in setterm_value().
      2022.04.22: Supported HTTPS connection using uNet3-TLS.
	  2025.08.04: Added con_ssoc() timeout definition POP3S_SSOC_TMO.
 ***************************************************************************/

#include "kernel.h"
#include "net_hdr.h"
#include "net_strlib.h"
#include "pop3_client.h"

#include "md5calc.h"
#include "base64calc.h"

#ifndef POP3S_SSOC_TMO
#define POP3S_SSOC_TMO      (10*1000)
#endif

/* String defines */
#define S_CRLF      "\r\n"
#define S_HDR_DIV   ": "

/* Status flag */
#define POP3F_MLCMD     0x01U       /* execute multiline response command */

/* Other defines */
#define B64_WKBUF_SIZE      64U     /* must be a multiple of 4 */
#define EOD_CRLF_LEN        3U      /* \r\n. */

/* macros */
#define CSTR_LEN(_cstr_)        (sizeof((_cstr_)) - 1U)

/* search string and pointer increment */
#define STRSTR_INC(s1, s2)      (strstr_inc((s1), (s2)))

/* set terminate for mailhdr value */
#define SETTERM_VALUE(s)        (setterm_value((s)))

/* set terminate for mail header */
#define SETTERM_CRLF(s)  \
    if (NULL != (s)) {                                              \
        tmp = net_strstr((s), S_CRLF);                              \
        if (NULL != tmp) {                                          \
            while ((' ' == tmp[CSTR_LEN(S_CRLF)]) || ('\t' == tmp[CSTR_LEN(S_CRLF)])) {  \
                tmp = net_strstr(tmp + sizeof(S_CRLF), S_CRLF);     \
            }                                                       \
            *tmp = '\0';                                            \
        }                                                           \
    }

/* set terminate for body part */
#define SETTERM_BODY(s)                                 \
    if (NULL != (s)) {                                  \
        tmp = net_strstr((s), (S_CRLF "." S_CRLF));     \
        if (NULL != tmp) {                              \
            *tmp = '\0';                                \
        }                                               \
    }


/* command compare macro */
#define CMD_STRNCMP(buf, cmd)   (0 == net_strncmp((buf), (cmd), CSTR_LEN((cmd))))

/* debug macro */
#ifndef POP3C_DBG_ON
#define POP3C_DBG_PUTS(d,s) (void)0;
#else
extern void POP3C_DBG_PUTS(const VB *div, const VB *str);
#endif

#ifdef POP3C_SSL_SUP
#ifdef SSL_SERVER_NODE  /* using uNet3-TLS */
static ER pop3c_con_ssoc(SID sid, T_NODE *host, ID ssnid, TMO tmo)
{
    T_SSL_NODE sn = {0};
    sn.node_type = SSL_CLIENT_NODE;
    sn.host = *host;
    return con_ssoc(sid, &sn, ssnid, tmo);
}
#else
#define pop3c_con_ssoc  con_ssoc
#endif
#endif

static ER pop3c_con_soc(T_POP3_CLIENT *pop3)
{
    ER ercd;

    /* Set POP3 server connect */
    if (0U == pop3->svr.port) {
        pop3->svr.port = POP3_DEF_PORT;
        if (0U != (pop3->auth_type & POP3_AUTH_IMPLSSL)) {
            pop3->svr.port = POP3S_DEF_PORT;
        }
    }
    pop3->svr.ver = IP_VER4;    /* Support only IPv4 */

    ercd = E_PAR;
    if (0U == (pop3->auth_type & POP3_AUTH_ENASSL)) {
        ercd = con_soc(pop3->sid, &pop3->svr, SOC_CLI);
    }
#ifdef POP3C_SSL_SUP
    else {
        pop3->ssn_id = 0;
        ercd = get_ssid_soc(pop3->sid);
        if (0 == ercd) {
            ercd = pop3c_con_ssoc(pop3->sid, &pop3->svr, 0, POP3S_SSOC_TMO);
        }
        if (0 < ercd) {
            pop3->ssn_id = ercd;
            ercd = E_OK;
        }
    }
#endif

    return ercd;
}

static ER pop3c_snd_soc(const T_POP3_CLIENT *pop3, VP data, UH len)
{
    ER ercd;

    ercd = E_PAR;
    if (0U == (pop3->auth_type & POP3_AUTH_ENASSL)) {
        ercd = snd_soc(pop3->sid, data, len);
    }
#ifdef POP3C_SSL_SUP
    else {
        ercd = snd_ssoc(pop3->sid, data, len);
    }
#endif

    return ercd;
}

static ER pop3c_rcv_soc(const T_POP3_CLIENT *pop3, VP data, UH len)
{
    ER ercd;

    ercd = E_PAR;
    if (0U == (pop3->auth_type & POP3_AUTH_ENASSL)) {
        ercd = rcv_soc(pop3->sid, data, len);
    }
#ifdef POP3C_SSL_SUP
    else {
        ercd = rcv_ssoc(pop3->sid, data, len);
    }
#endif

    return ercd;
}

static ER pop3c_cls_soc(const T_POP3_CLIENT *pop3)
{
    ER ercd;

    ercd = E_PAR;
    if (0U == (pop3->auth_type & POP3_AUTH_ENASSL)) {
        ercd = cls_soc(pop3->sid, SOC_TCP_CLS);
    }
#ifdef POP3C_SSL_SUP
    else {
        ercd = cls_ssoc(pop3->sid);
    }
#endif

    return ercd;
}


static void setterm_value(VB **str)
{
    VB term[] = { '\"', ';', '\r', '\0' };
    VB *tm;
    VB *wk;

    if (NULL != *str) {
        if ('\"' == **str) {
            tm = &term[0];
            *str += 1;
        }
        else {
            tm = &term[1];
        }

        do {
            wk = net_strchr(*str, *tm);
            if (NULL != wk) {
                *wk = '\0';
                break;
            }
        } while (*(tm++) != '\0');
    }
}

static VB *strstr_inc(VB *s1, VB *s2)
{
    VB *tmp;

    tmp = net_strstr(s1, s2);

    if (NULL != tmp) {
        tmp += net_strlen(s2);
    }

    return tmp;
}

static UB pop3_is_mlcmd(const VB *str)
{
    UB ret = 0U;

    if (CMD_STRNCMP(str, "TOP") || CMD_STRNCMP(str, "RETR")) {
        ret = 1U;
    }
    else if (CMD_STRNCMP(str, "LIST") || CMD_STRNCMP(str, "UIDL")) {
        ret = 1U;
        /* If argument specified, response is single line. */
        str += CSTR_LEN("LIST");
        for (; (*str != '\r') && (*str != '\0'); str++) {
            if (('0' <= *str) && (*str <= '9')) {
                ret = 0U;
                break;
            }
        }
    }
    else {  /* other command */
        ret = 0U;
    }

    return ret;
}


/* Parse response message (message-id, string) */
H pop3_parse_res_a(const VB *buf, UW *msg, VB *str)
{
    const VB *tmp;

    if (NULL == buf) {
        return 0;
    }
    tmp = buf;

    /* support format ## regular expression: [^0-9] ([0-9]+) ([0-9]+) */
    /* +OK number-of-messages maildrop-size */
    /* +OK message-number message-size */

    for (; '\0' != *tmp; ++tmp) {
        if (('0' <= *tmp) && (*tmp <= '9')) {
            break;
        }
    }
    if ('\0' == *tmp) {
        return 0;
    }
    if (NULL != msg) {
        *msg = (UW)net_atoi(tmp);
    }

    for (; '\0' != *tmp; ++tmp) {
        if (('0' > *tmp) || (*tmp > '9')) {
            break;
        }
    }
    if (' ' != *tmp) {
        return 0;
    }
    ++tmp;

    if (NULL != str) {
        for (; ('\0' != *tmp) && (*tmp != '\r'); tmp++) {
            *str = *tmp;
            str++;
        }
        *str = '\0';
    }
    else {
        for (; ('\0' != *tmp) && (*tmp != '\r'); ++tmp) {
            ;
        }
    }

    return (tmp - buf);
}

/* Parse response message (message-id, integer) */
H pop3_parse_res_i(const VB *buf, UW *msg, UW *num)
{
    ER ercd;
    VB wk[41];

    ercd = pop3_parse_res_a(buf, msg, wk);
    if (0 < ercd) {
        *num = (UW)net_atoi(wk);
    }

    return (H)ercd;
}

/* check BASE64 encode string */
static UB chk_b64enc_str(const VB *s, UW len)
{
    UB ret;

    ret = 1U;
    for (; 0U < len; len--, s++) {
        /* If have a character that is not included in BASE64, it is not encode */
        if ( (('A' <= *s) && (*s <= 'Z')) || (('a' <= *s) && (*s <= 'z'))
          || (('0' <= *s) && (*s <= '9')) || ('+' == *s) || ('/' == *s) || ('=' == *s)
          || ('\r' == *s) || ('\n' == *s) ) {   /* MIME support */
            continue;
        }

        ret = 0U;
        break;
    }
    return ret;
}

/* Replace string sl to ss */
static void replace_str(const VB *src, VB *dst, const VB *sl, const VB *ss)
{
    const VB *t1;
    const VB *t2;
    VB *d;
    UW len;
    UB len_sl;
    UB len_ss;

    len_sl = (UB)net_strlen(sl);
    len_ss = (UB)net_strlen(ss);
    d  = dst;
    t1 = src;

    while (1) {
        t2 = net_strstr(t1, sl);    /* find sl */
        if (NULL == t2) {
            break;
        }
        len = (UW)(t2 - t1);

        /* copy Area(area from t1 to t2) to d.  */
        net_memcpy(d, t1, len);
        d += len;
        /* copy ss to d */
        net_memcpy(d, ss, (SIZE)len_ss);
        d += len_ss;

        /* skip sl */
        t1 = t2 + len_sl;
    }

    net_strcpy(d, t1);
}

static UB pop3_atoh(VB *hex)
{
    UB ret;

    ret = 0U;

    for (; '\0' != *hex; hex++) {
        ret = (UB)(ret << 4U);
        if (('0' <= *hex) && (*hex <= '9')) {
            ret += (UB)(*hex - '0');
        }
        else if (('A' <= *hex) && (*hex <= 'F')) {
            ret += (UB)(*hex - 'A' + 10);
        }
        else {
            ret += (UB)(*hex - 'a' + 10);
        }
    }

    return ret;
}


/* decode quoted-printable mail-body */
ER pop3_qpdec_body(VB *dst, const VB *src)
{
    ER ercd;
    const VB *tmp1;
    const VB *tmp2;
    VB hex[3];
    INT n;

    if ((NULL == dst) || (NULL == src)) {
        ercd = E_PAR;
    }
    else {
        n = 0;
        hex[2] = '\0';
        tmp1 = tmp2 = src;

        while (NULL != tmp2) {
            tmp2 = net_strstr(tmp1, "=");
            if (NULL != tmp2) {
                ercd = tmp2 - tmp1;
            }
            else {
                ercd = (ER)net_strlen(tmp1);
            }
            net_strncpy(&dst[n], tmp1, (SIZE)ercd);
            n += ercd;
            dst[n] = '\0';

            if (NULL != tmp2) {
                tmp2++;
                hex[0] = *tmp2++;
                hex[1] = *tmp2++;
                if (hex[0] != '\r') {
                    dst[n++] = pop3_atoh(hex);
                    dst[n] = '\0';
                }
                tmp1 = tmp2;
            }
        }
        ercd = (ER)n;
    }

    return ercd;
}

/* decode BASE64 mail-body */
ER pop3_b64dec_body(VB *dst, const VB *src, UB chk)
{
    ER ercd;
    T_BASE64_INFO b64i;
    const VB *tmp1;
    const VB *tmp2;
    VB wk[B64_WKBUF_SIZE];
    INT n;

    if ((NULL == dst) || (NULL == src)) {
        ercd = E_PAR;
    }
    /* check BASE64 encoding */
    else if ((0U != chk) && (0U == chk_b64enc_str(src, net_strlen(src)))) {
        ercd = E_SYS;
    }
    else {
        n = 0;
        tmp1 = src;

        while ('\0' != *tmp1) {
            /* decode BASE64 part - start */
            tmp2 = net_strstr(tmp1, S_CRLF);
            if (NULL != tmp2) {
                ercd = tmp2 - tmp1;
            }
            else {
                ercd = (ER)net_strlen(tmp1);
            }

            INI_B64I(&b64i);
            SET_B64I(&b64i, wk, (VB*)tmp1, sizeof(wk), ercd);
            (void)base64dec(&b64i);
            net_memcpy(&dst[n], wk, (SIZE)b64i.rdlen);
            n += (INT)b64i.rdlen;
            /* decode BASE64 part - end */

            if (!tmp2) {
                break;
            }
            tmp1 = tmp2 + CSTR_LEN(S_CRLF);
        }

        dst[n] = '\0';
        ercd = (ER)n;
    }

    return ercd;
}


/* decode BASE64 mail-header */
#define MIME_EW_START   "=?"    /* 'encoded-word' start */
#define MIME_EW_END     "?="    /* 'encoded-word' end */
#define MIME_EW_B64     "?B?"   /* 'encoded-word' BASE64 encode */

#define MIME_EWS_START  (CSTR_LEN(MIME_EW_START))
#define MIME_EWS_END    (CSTR_LEN(MIME_EW_END))
#define MIME_EWS_B64    (CSTR_LEN(MIME_EW_B64))

ER pop3_b64dec_hdr(VB *dst, const VB *src, VB *charset)
{
    ER ercd;
    T_BASE64_INFO b64i;
    const VB *tmp1;
    const VB *tmp2;
    VB wk[B64_WKBUF_SIZE];
    INT n;

    if ((NULL == dst) || (NULL == src)) {
        return E_PAR;
    }

    /* check format ... =?{charset}?B?{BASE64decode data}?= */
    ercd = E_SYS;
    tmp1 = src;
    while (1) {
        tmp2 = net_strstr(tmp1, MIME_EW_START);
        if (NULL == tmp2) {
            break;
        }
        ercd = E_SYS;
        tmp1 = net_strstr(tmp2 + MIME_EWS_START, MIME_EW_B64);
        if (NULL == tmp1) {
            break;
        }
        tmp2 = net_strstr(tmp1 + MIME_EWS_B64, MIME_EW_END);
        if (NULL == tmp2) {
            break;
        }
        tmp1 = tmp2 + MIME_EWS_END;
        ercd = E_OK;
    }
    if (E_OK != ercd) {
        return ercd;    /* format error? OR not BASE64 encoding */
    }

    /* get charset (only first element) */
    if (NULL != charset) {
        tmp1 = net_strstr(src, MIME_EW_START) + MIME_EWS_START;
        tmp2 = net_strstr(tmp1, MIME_EW_B64);
        ercd = tmp2 - tmp1;
        net_memcpy(charset, tmp1, (SIZE)ercd);
        charset[ercd] = '\0';
    }

    /* RFC2047 (6.2. Display of 'encoded-words') */
    replace_str(src, dst, S_CRLF, "");          /* removal of CRLF */
    /* removal of 'linear-white-space' */
    replace_str(dst, dst, (MIME_EW_END " " MIME_EW_START), (MIME_EW_END MIME_EW_START));
    replace_str(dst, dst, (MIME_EW_END "\t" MIME_EW_START), (MIME_EW_END MIME_EW_START));

    n = 0;
    tmp1 = dst;
    while (1) {
        tmp2 = net_strstr(tmp1, MIME_EW_START);
        if (NULL == tmp2) {
            break;
        }
        net_memcpy(&dst[n], tmp1, (tmp2 - tmp1));  // copy string before "=?"
        n += (INT)(tmp2 - tmp1);

        /* decode BASE64 part - start */
        tmp1 = net_strstr(tmp2 + MIME_EWS_START, MIME_EW_B64);
        tmp1 += MIME_EWS_B64;   // skip "?B?"
        tmp2 = net_strstr(tmp1, MIME_EW_END);
        ercd = tmp2 - tmp1;

        INI_B64I(&b64i);
        SET_B64I(&b64i, wk, (VB*)tmp1, sizeof(wk), ercd);
        (void)base64dec(&b64i);
        net_memcpy(&dst[n], wk, (SIZE)b64i.rdlen);
        n += (INT)b64i.rdlen;
        /* decode BASE64 part - end */

        tmp1 = tmp2 + MIME_EWS_END;     // skip "?="
    }
    net_strcpy(&dst[n], tmp1);          // copy string after "?="
    n = (INT)net_strlen(dst);

    return n;
}


/* Convert Multi-Part (buf to part) */
ER pop3_cnv_part(T_POP3_PART *part, VB *buf, const VB *boundary)
{
    VB *tmp;

    if ((NULL == buf) || (NULL == part)) {
        return E_PAR;
    }

    net_memset(part, 0, sizeof(*part));

    if (NULL != boundary) {
        tmp = buf;
        /* check MIME boundary */
        tmp = net_strstr(tmp, boundary);
        if (!tmp) {
            return E_OBJ;   /* not found MIME boundary */
        }
        tmp += net_strlen(boundary);
        part->next  = net_strstr(tmp, boundary);
        if (NULL != part->next) {
            if (('-' != part->next[-1]) || ('-' != part->next[-2])) {
                return E_OBJ;       /* boundary format error */
            }
            part->next[-2] = '\0';  /* prev part terminate */
        }
        else if (0 == net_strncmp(tmp, ("--" S_CRLF), CSTR_LEN("--" S_CRLF))) {   /* terminate */
            net_memset(part, 0, sizeof(*part));
            return E_OK;
        }
        else {
            /* do nothing */
        }
    }

    /* search MIME header */
    part->body  = STRSTR_INC(buf, S_CRLF S_CRLF);
    if (NULL != part->body) {
        /* not search body part */
        part->body[-2] = '\0';
    }

    /* search Content-Type header */
    part->ctype = STRSTR_INC(buf, S_CRLF "Content-Type" S_HDR_DIV);
    if (NULL == part->ctype) {
        part->ctype = STRSTR_INC(buf, "Content-Type" S_HDR_DIV);
    }

    if (NULL == part->ctprm) {
        part->ctprm = STRSTR_INC(part->ctype, "boundary=");
        if (NULL != part->ctprm) {
            part->ctflg |= POP3_CTP_BOUNDARY;
        }
    }
    if (NULL == part->ctprm) {
        part->ctprm = STRSTR_INC(part->ctype, "charset=");
        if (NULL != part->ctprm) {
            part->ctflg |= POP3_CTP_CHARSET;
        }
    }
    if (NULL == part->ctprm) {
        part->ctprm = STRSTR_INC(part->ctype, "name=");
        if (NULL != part->ctprm) {
            part->ctflg |= POP3_CTP_NAME;
        }
    }

    tmp = STRSTR_INC(buf, "Content-Transfer-Encoding" S_HDR_DIV);
    if (NULL != tmp) {
        part->ctflg |=  (*tmp == 'b') ?
                            (tmp[1] == 'a') ? (UW)POP3_CTE_BASE64 : (UW)POP3_CTE_BIN :
                        (*tmp == 'q') ? (UW)POP3_CTE_QTPRT :
                        (*tmp == '8') ? (UW)POP3_CTE_8BIT : (UW)POP3_CTE_7BIT ;
    }

    /* convert MIME header */
    SETTERM_VALUE(&part->ctprm);
    SETTERM_VALUE(&part->ctype);
    return E_OK;
}

/* Convert MailHeader (buf to mail) */
ER pop3_cnv_mail(T_POP3_MAIL *mail, VB *buf)
{
    ER ercd;
    VB *tmp;
    VB *body;
    VB bch;

    if ((NULL == buf) || (NULL == mail)) {
        return E_PAR;
    }

    /* not search body part */
    body  = STRSTR_INC(buf, S_CRLF S_CRLF);
    if (NULL != body) {
        bch = body[-2];
        body[-2] = '\0';
    }

    /* search mail header */
    /* Note: the order of the headers is not fixed. */
    mail->date  = STRSTR_INC(buf, "Date" S_HDR_DIV);
    mail->from  = STRSTR_INC(buf, "From" S_HDR_DIV);
    mail->to    = STRSTR_INC(buf, "To" S_HDR_DIV);
    mail->cc    = STRSTR_INC(buf, "Cc" S_HDR_DIV);
    mail->bcc   = STRSTR_INC(buf, "Bcc" S_HDR_DIV);
    mail->subject = STRSTR_INC(buf, "Subject: ");

    /* restore body part */
    if (NULL != body) {
        body[-2] = bch;
    }

    /* check required fields */
    if ((NULL == mail->from) || (!(mail->to || mail->cc || mail->bcc))) {
        ercd = E_OBJ;
    }
    else {
        /* search mail header (MIME), Body */
        ercd = pop3_cnv_part(&mail->part, buf, NULL);
    }

    /* convert mail header */
    if (E_OK == ercd) {
        SETTERM_CRLF(mail->date);
        SETTERM_CRLF(mail->from);
        SETTERM_CRLF(mail->to);
        SETTERM_CRLF(mail->cc);
        SETTERM_CRLF(mail->bcc);
        SETTERM_CRLF(mail->subject);
        /* body terminate (not multipart) */
        if (0U == (POP3_CTP_BOUNDARY & mail->part.ctflg)) {
            SETTERM_BODY(mail->part.body);
        }
    }
    return ercd;
}

/*------------------------------------------------------------*/

/* AUTH PLAIN function */
static ER pop3_auth_plain(T_POP3_CLIENT *pop3)
{
    ER ercd;

    ercd = pop3_cmd_user(pop3, pop3->usr);
    if (E_OK == ercd) {
        ercd = pop3_cmd_pass(pop3, pop3->pw);
    }

    return ercd;
}


/* AUTH APOP function */
static ER pop3_auth_apop(T_POP3_CLIENT *pop3)
{
    HASH HA;
    VB tmstamp[128];
    VB *tmp;
    VB *start;
    ER ercd;
    UB chk_flg;

    /* Look for timestamp from POP server response message. */
    chk_flg = 0U;
    start = NULL;
    for (tmp = pop3->buf; ('\0' != *tmp) && (chk_flg != 0x07U) ; ++tmp) {
        switch (*tmp) {
        case '<':
            start = tmp;
            chk_flg = 0x01U;
            break;
        case '@':
            if (chk_flg == 0x01U) {
                chk_flg |= 0x02U;
            }
            break;
        case '>':
            if (chk_flg == 0x03U) {
                chk_flg |= 0x04U;
            }
            break;
        default:
            /* do nothing */
            break;
        }
    }
    if (chk_flg == 0x07U) {
        ercd = tmp - start;
        tmstamp[ercd] = '\0';
        net_memcpy(tmstamp, start, (SIZE)ercd);
        ercd = E_OK;
    }
    else {
        ercd = E_OBJ;   /* not found digest */
    }

    /* APOP Authenticate */
    if (E_OK == ercd) {
        net_strcpy(pop3->buf, "APOP ");
        net_strcat(pop3->buf, pop3->usr);
        net_strcat(pop3->buf, " ");

        tmp = &pop3->buf[net_strlen(pop3->buf)];
        net_strcat(tmstamp, pop3->pw);
        md5_hash((UB*)tmstamp, net_strlen(tmstamp), HA);
        cnv_hex(HA, tmp);

        net_strcat(pop3->buf, S_CRLF);
        ercd = pop3_snd_cmd(pop3, pop3->buf);
    }

    return ercd;
}

/* Connect POP3 server */
ER pop3_login(T_POP3_CLIENT *pop3)
{
    ER ercd;

    /* check parameter */
    if (NULL == pop3) {
        ercd = E_OBJ;
    }
    else if ((0U == pop3->sid) || (NULL == pop3->buf) || (0U == pop3->len)) {
        ercd = E_PAR;
    }
    else if ((0U == pop3->svr.ipa) || ((UB)NET_DEV_MAX < pop3->svr.num)) {
        ercd = E_PAR;
    }
    else {
        ercd = E_OK;
    }
#ifndef POP3C_SSL_SUP
    if (0U != (pop3->auth_type & (POP3_AUTH_EXPLSSL | POP3_AUTH_IMPLSSL))) {
        ercd = E_PAR;
    }
#endif
    if (E_OK != ercd) {
        return ercd;
    }

    pop3->auth_type &= ~POP3_AUTH_ENASSL;
    if (0U != (pop3->auth_type & POP3_AUTH_IMPLSSL)) {
        pop3->auth_type |= POP3_AUTH_ENASSL;
    }

    ercd = pop3c_con_soc(pop3);
    if (E_OK == ercd) {
        ercd = pop3c_rcv_soc(pop3, pop3->buf, pop3->len);
    }
    if (0 < ercd) {
        if (0 == net_strncmp(pop3->buf, POP3_RES_OK, CSTR_LEN(POP3_RES_OK))) {
            ercd = E_OK;    /* response ok */
        }
        else {
            ercd = E_SYS;   /* response error */
        }
    }
    else {
        ercd = E_CLS;       /* socket operation error */
    }

#ifdef POP3C_SSL_SUP
    if (E_OK == ercd) {
        if (0U != (pop3->auth_type & POP3_AUTH_EXPLSSL)) {
            net_strcpy(pop3->buf, "STLS" S_CRLF);
            ercd = pop3_snd_cmd(pop3, pop3->buf);
            if (0 <= ercd) {
                pop3->auth_type |= POP3_AUTH_ENASSL;
                ercd = pop3c_con_soc(pop3);
            }
            if (E_OK != ercd) {
                pop3->auth_type &= ~POP3_AUTH_ENASSL;
                pop3c_cls_soc(pop3);
            }
        }
    }
#endif


    /* Login SMTP server (SMTP Authenticate) */
    if (E_OK == ercd) {
        switch (pop3->auth_type & POP3_AUTH_TYPE_MASK) {
        case POP3_AUTH_PLAIN:
            ercd = pop3_auth_plain(pop3);
            break;

        case POP3_AUTH_APOP:
            ercd = pop3_auth_apop(pop3);
            break;

        default:
            ercd = E_NOSPT;
            break;
        }
    }

    return ercd;
}


/* Disconnect POP3 server */
ER pop3_quit(T_POP3_CLIENT *pop3)
{
    ER ercd;

    if (NULL == pop3) {
        ercd = E_OBJ;
    }
    else if ((0U == pop3->sid) || (NULL == pop3->buf) || (0U == pop3->len)) {
        ercd = E_PAR;
    }
    else {
        (void)pop3_cmd_quit(pop3);
        (void)pop3c_rcv_soc(pop3, pop3->buf, pop3->len - 1U);   /* wait disconnect */
        ercd = pop3c_cls_soc(pop3);
        ercd = E_OK;
    }
    return ercd;
}


/* Judge the end of the data for multiline response. */
ER pop3_mline_eod(T_POP3_CLIENT *pop3)
{
    ER ercd;
    UW len;

    if (NULL == pop3) {
        ercd = E_OBJ;
    }
    else if (0U != (pop3->flag & POP3F_MLCMD)) {
        /* Termination judgment of response */
        len = net_strlen(pop3->buf);

        ercd = ((len >= EOD_CRLF_LEN)
            && (0 == net_strncmp(&pop3->buf[len - EOD_CRLF_LEN], ("." S_CRLF), EOD_CRLF_LEN))) ? 1 : 0 ;
    }
    else {  /* singleline response command */
        ercd = 1;
    }
    return ercd;
}


/* Receiving a response multiline response command */
ER pop3_mline_next(T_POP3_CLIENT *pop3)
{
    ER ercd;
    if (NULL == pop3) {
        ercd = E_OBJ;
    }
    else if (0U == pop3->sid) {
        ercd = E_PAR;
    }
    else if (0U != (pop3->flag & POP3F_MLCMD)) {
        ercd = pop3c_rcv_soc(pop3, pop3->buf, pop3->len - 1U);
        if (0 < ercd) {
            pop3->buf[ercd] = '\0';
        }
        else {  /* error or connection close */
            if (E_TMOUT != ercd) {
                ercd = E_CLS;
            }
        }
    }
    else {  /* singleline response command */
        ercd = 0;
    }
    return ercd;
}


/* Save file a received message. */
static ER pop3_rcv_msg_file(T_POP3_CLIENT *pop3, const VB *path)
{
#ifdef POP3_FSYS_NONE
    return E_NOSPT;
#else
    FILE *fp;
    ER ercd;
    UW size;

    fp = fopen(path, "wb");
    if (!fp) {
        ercd = E_OBJ;
    }
    else {
        do {
            ercd = pop3_mline_next(pop3);
            if (0 > ercd) {
                break;
            }
            size = (UW)fwrite(pop3->buf, sizeof(UB), (SIZE)ercd, fp);
            if (size < (UW)ercd) {  /* write error (Memory capacity over?) */
                /* continue to receive data until the end */
                while (0 == pop3_mline_eod(pop3)) {
                    ercd = pop3_mline_next(pop3);
                    if (0 >= ercd) {
                        break;
                    }
                }
                ercd = E_MACV;
                break;
            }
        } while (0 == pop3_mline_eod(pop3));

        (void)fclose(fp);
    }

    return (0 < ercd) ? E_OK : ercd ;
#endif
}


/* Save buffer a received message. */
static ER pop3_rcv_msg_mem(T_POP3_CLIENT *pop3, VB *mem, UW len)
{
    VB *tmp;
    ER ercd;

    tmp = mem;
    do {
        ercd = pop3_mline_next(pop3);
        if (0 > ercd) {
            break;
        }
        else if (len > (UW)ercd) {
            net_memcpy(tmp, pop3->buf, (SIZE)ercd);
            len -= (UW)ercd;
            tmp += ercd;
        }
        else {  /* Memory capacity over */
            /* continue to receive data until the end */
            while (0 == pop3_mline_eod(pop3)) {
                ercd = pop3_mline_next(pop3);
                if (0 >= ercd) {
                    break;
                }
            }
            ercd = E_MACV;
            break;
        }
    } while (0 == pop3_mline_eod(pop3));

    if (0 < ercd) {
        ercd = E_OK;
        *tmp = '\0';
    }

    return ercd;
}


/* Save received message. */
ER pop3_rcv_msg(T_POP3_CLIENT *pop3, UW mid, const T_POP3_FILE *file)
{
    ER ercd;
    UW size = 0U;

    /* check parameter */
    if (NULL == pop3) {
        ercd = E_OBJ;
    }
    else if (NULL == file) {
        ercd = E_PAR;
    }
    else if ((NULL == file->buf) || (0U == (file->type & (POP3_FTYP_FS | POP3_FTYP_MEM)))) {
        ercd = E_PAR;
    }
    else if (0U == pop3->sid) {
        ercd = E_PAR;
    }
    else {
        /* check validate mid file */
        ercd = pop3_cmd_list(pop3, mid);
    }

    /* receive mid file */
    if (E_OK == ercd) {
        /* check buffer memory (use memory buffer only) */
        if (0U != (file->type & POP3_FTYP_MEM)) {
            ercd = pop3_parse_res_i(pop3->buf, NULL, &size);
            if (0 != ercd) {
                if (size >= file->len) {
                    ercd = E_NOMEM;
                }
            }
            else {
                ercd = E_SYS;   /* response message parse error */
            }
        }

        /* request to receive mid file */
        if (0 <= ercd) {
            ercd = pop3_cmd_retr(pop3, mid);
            if (E_OK == ercd) {
                if (0U != (file->type & POP3_FTYP_MEM)) {
                    ercd = pop3_rcv_msg_mem(pop3, file->buf, file->len);
                }
                else {  /* POP3_FTYP_FS */
                    ercd = pop3_rcv_msg_file(pop3, file->buf);
                }
            }
        }

        /* delete mid file on success receive */
        if (E_OK == ercd) {
            ercd = pop3_cmd_dele(pop3, mid);
        }
    }

    return ercd;
}


/* Send POP3 command, and Receive response code */
ER pop3_snd_cmd(T_POP3_CLIENT *pop3, const VB *str)
{
    ER ercd;
    UB mlcmd;

    if (NULL == pop3) {
        ercd = E_OBJ;
    }
    else if (NULL == str) {
        ercd = E_PAR;
    }
    else if (0U == pop3->sid) {
        ercd = E_PAR;
    }
    else {
        POP3C_DBG_PUTS("send", str);
        ercd = pop3c_snd_soc(pop3, (VP)str, (UH)net_strlen(str));
        if (0 < ercd) {
            mlcmd = pop3_is_mlcmd(str);     /* multiline command ? */
            ercd = pop3c_rcv_soc(pop3, (VP)pop3->buf, pop3->len - 1U);
        }

        if (0 < ercd) {
            pop3->buf[ercd] = '\0';
            POP3C_DBG_PUTS("recv", pop3->buf);
            /* check +OK or -ERR */
            if (0 != net_strncmp(pop3->buf, (VP)POP3_RES_OK, CSTR_LEN(POP3_RES_OK))) {
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

    pop3->flag &= ~POP3F_MLCMD;
    if (E_OK == ercd) {
        if (0U != mlcmd) {
            pop3->flag |= POP3F_MLCMD;
        }
    }

    return ercd;
}

/* pop3_snd_cmd (command only) */
ER pop3_snd_cmd_0(T_POP3_CLIENT *pop3, const VB *cmd)
{
    ER ercd;
    VB *buf = pop3->buf;

    if (NULL == cmd) {
        ercd = E_PAR;
    }
    else {
        net_strcpy(buf, cmd);
        net_strcat(buf, S_CRLF);
        ercd = pop3_snd_cmd(pop3, buf);
    }
    return ercd;
}

/* pop3_snd_cmd (command and one Integer parameter) */
ER pop3_snd_cmd_i(T_POP3_CLIENT *pop3, const VB *cmd, UW p1)
{
    ER ercd;
    VB *buf = pop3->buf;

    if (NULL == cmd) {
        ercd = E_PAR;
    }
    else {
        net_strcpy(buf, cmd);
        net_strcat(buf, " ");
        net_utoa((UINT)p1, &buf[net_strlen(buf)], 10);
        net_strcat(buf, S_CRLF);
        ercd = pop3_snd_cmd(pop3, pop3->buf);
    }
    return ercd;
}

/* pop3_snd_cmd (command and one String parameter) */
ER pop3_snd_cmd_a(T_POP3_CLIENT *pop3, const VB *cmd, const VB *p1)
{
    ER ercd;
    VB *buf = pop3->buf;

    if ((NULL == cmd) || (NULL == p1)) {
        ercd = E_PAR;
    }
    else {
        net_strcpy(buf, cmd);
        net_strcat(buf, " ");
        net_strcat(buf, p1);
        net_strcat(buf, S_CRLF);
        ercd = pop3_snd_cmd(pop3, buf);
    }
    return ercd;
}

/* pop3_snd_cmd (command and two Integer parameter) */
ER pop3_snd_cmd_i2(T_POP3_CLIENT *pop3, const VB *cmd, UW p1, UW p2)
{
    ER ercd;
    VB *buf = pop3->buf;

    if (NULL == cmd) {
        ercd = E_PAR;
    }
    else {
        net_strcpy(pop3->buf, cmd);
        net_strcat(buf, " ");
        net_utoa((UINT)p1, &buf[net_strlen(buf)], 10);
        net_strcat(buf, " ");
        net_utoa((UINT)p2, &buf[net_strlen(buf)], 10);
        net_strcat(buf, S_CRLF);
        ercd =  pop3_snd_cmd(pop3, buf);
    }
    return ercd;
}

/* pop3_snd_cmd (command and two String parameter) */
ER pop3_snd_cmd_a2(T_POP3_CLIENT *pop3, const VB *cmd, const VB *p1, const VB *p2)
{
    ER ercd;
    VB *buf = pop3->buf;

    if ((NULL == cmd) || (NULL == p1) || (NULL == p2)) {
        ercd = E_PAR;
    }
    else {
        net_strcpy(buf, cmd);
        net_strcat(buf, " ");
        net_strcat(buf, p1);
        net_strcat(buf, " ");
        net_strcat(buf, p2);
        net_strcat(buf, S_CRLF);
        ercd = pop3_snd_cmd(pop3, buf);
    }

    return ercd;
}
