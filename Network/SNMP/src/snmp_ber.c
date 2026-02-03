/*
    SNMP
    Basic encoding rules
    Copyright (c) 2014-2019, eForce Co., Ltd. All rights reserved.
    
    2014-03-06 Created
    2017-04-20 Support OID type for private MIB data
    2019-02-25 Support Counter64(SNMP_TYP_CNT64) type for private MIB data.
               Support accessible-for-notify(SNMP_STS_AN) status for private MIB data.
*/

#include "kernel.h"
#include "snmp.h"
#include "snmp_ber.h"

/* Debug configuration */
#if defined(CFG_SNMP_DBG_ENA)
#define CFG_DBG_ENA    1
#else
#define CFG_DBG_ENA    0
#endif
#if (CFG_DBG_ENA == 1)
#if !defined(CFG_SNMP_DBG_PTF)
extern void apl_put_str(VB const*);
extern void apl_put_dig(UW);
extern void apl_put_hex(UW);
#define dbg_put_str(x)    apl_put_str(x)
#define dbg_put_dig(x)    apl_put_dig(x)
#define dbg_put_hex(x)    apl_put_hex(x)
#else
#include <stdio.h>
#define dbg_put_str(x)    printf(x);
#define dbg_put_dig(x)    printf("%d", x);
#define dbg_put_hex(x)    printf("0x%x", x);
#endif
#else
#define dbg_put_str(x)
#define dbg_put_dig(x)
#define dbg_put_hex(x)
#endif

/* Length */
#define LEN_LONG    0x80    /* Length is 2 or more */

static ER snmp_ber_dec_val(UW* dat, T_SNMP_BER* ber, UH typ)
{
    UB* buf_ptr;
    INT i;

    /* Decode integer or coounter */

    if (ber->len == 0) {
        return E_PAR;
    }

    /* Data */
    buf_ptr = (UB*)ber->buf;
    *dat = 0;
    if (typ == TYP_INT) {
        if ((buf_ptr[0] & 0x80) != 0x00) {
            /* Negative */
            *dat = ~0;
        }
    }
    for (i = 0; i < ber->len; i++) {
        *dat <<= 8;
        *dat |= buf_ptr[i];
    }

    return E_OK;
}

static ER snmp_ber_dec_val64(UD_SNMP* dat, T_SNMP_BER* ber, UH typ)
{
    UB* buf_ptr;
    INT i;

    /* Decode integer or coounter */

    if (ber->len == 0) {
        return E_PAR;
    }

    /* Data */
    buf_ptr = (UB*)ber->buf;
    *dat = 0;
    if (typ == TYP_INT) {
        if ((buf_ptr[0] & 0x80) != 0x00) {
            /* Negative */
            *dat = ~0;
        }
    }
    for (i = 0; i < ber->len; i++) {
        *dat <<= 8;
        *dat |= buf_ptr[i];
    }

    return E_OK;
}


static INT snmp_ber_enc_len(UB** buf, UH* len, UH dat_len)
{
    UB* buf_ptr;
    INT enc_len;

    /* Encode length */

    buf_ptr = (len != 0) ? *buf : 0;

    if (dat_len < 128) {
        enc_len = 1;
        if (buf_ptr != 0x00) {
            if (*len < (UH)enc_len) {
                return E_BOVR;
            }
            *buf_ptr++ = (UB)dat_len;
        }
    } else {
        /* 128-byte or more */
        if (dat_len <= 0x00ff) {
            enc_len = 2;
            if (buf_ptr != 0x00) {
                if (*len < (UH)enc_len) {
                    return E_BOVR;
                }
                *buf_ptr++ = LEN_LONG | 1;
                *buf_ptr++ = (UB)dat_len;
            }
        } else {
            enc_len = 3;
            if (buf_ptr != 0x00) {
                if (*len < (UH)enc_len) {
                    return E_BOVR;
                }
                *buf_ptr++ = LEN_LONG | 2;
                *buf_ptr++ = (UB)(dat_len >> 8);
                *buf_ptr++ = (UB)dat_len;
            }
        }
    }

    if (buf_ptr != 0x00) {
        *buf = buf_ptr;
        *len -= (UH)enc_len;
    }

    return enc_len;
}

static INT snmp_ber_enc_val(VP* buf, UH* len, UH typ, UW dat)
{
    UB* buf_ptr;
    INT enc_len;
    UH dat_len;
    UW dat_buf;
    INT enc_l;
    UH i;

    /* Encode integer or counter */

    if (buf != 0x00 && len == 0) {
        return E_PAR;
    }
    buf_ptr = (buf != 0) ? (UB*)*buf : 0;

    /* Tag */
    if (buf_ptr != 0x00) {
        if (*len < 1) {
            return E_BOVR;
        }
        *buf_ptr++ = (UB)typ;
        (*len)--;
    }
    enc_len = 1;

    /* Length */
    dat_buf = dat;
    if (typ == TYP_INT && (INT)dat < 0) {
        dat_buf = ~dat_buf;    /* Negative */
    }
    for (dat_len = 1; dat_len <= 4; dat_len++) {
        if ((dat_buf >> 8) == 0) {
            break;
        }
        dat_buf >>= 8;
    }
    if (typ != TYP_IP_ADR) {
        if ((dat_buf & 0x80) != 0x00) {
            dat_len++;
        }
    }
    enc_l = snmp_ber_enc_len(&buf_ptr, len, dat_len);
    if (enc_l <= 0) {
        return enc_l;
    }
    enc_len += enc_l;

    /* Data */
    if (buf_ptr != 0x00) {
        if (*len < dat_len) {
            return E_BOVR;
        }
        for (i = 0; i < dat_len; i++) {
            dat_buf = dat >> (8 * (dat_len - 1 - i));
            *buf_ptr++ = (UB)dat_buf;
            (*len)--;
        }
    }
    enc_len += dat_len;

    if (buf_ptr != 0x00) {
        *buf = buf_ptr;
    }

    return enc_len;
}
    

static INT snmp_ber_enc_val64(VP* buf, UH* len, UH typ, UD_SNMP dat)
{
    UB* buf_ptr;
    INT enc_len;
    UD_SNMP dat_buf;
    INT enc_l;
    UH dat_len;
    UH i;

    /* Encode counter64 */

    if (buf != 0x00 && len == 0) {
        return E_PAR;
    }
    buf_ptr = (buf != 0) ? (UB*)*buf : 0;

    /* Tag */
    if (buf_ptr != 0x00) {
        if (*len < 1) {
            return E_BOVR;
        }
        *buf_ptr++ = (UB)typ;
        (*len)--;
    }
    enc_len = 1;

    /* Length */
    dat_buf = dat;
    for (dat_len = 1; dat_len <= 8; dat_len++) {
        if ((dat_buf >> 8) == 0) {
            break;
        }
        dat_buf >>= 8;
    }
    if ((dat_buf & 0x80) != 0x00) {
        dat_len++;
    }
    enc_l = snmp_ber_enc_len(&buf_ptr, len, dat_len);
    if (enc_l <= 0) {
        return enc_l;
    }
    enc_len += enc_l;

    /* Data */
    if (buf_ptr != 0x00) {
        if (*len < dat_len) {
            return E_BOVR;
        }
        for (i = 0; i < dat_len; i++) {
            dat_buf = dat >> (8 * (dat_len - 1 - i));
            *buf_ptr++ = (UB)dat_buf;
            (*len)--;
        }
    }
    enc_len += dat_len;

    if (buf_ptr != 0x00) {
        *buf = buf_ptr;
    }

    return enc_len;
}

static INT snmp_ber_enc_buf(VP* buf, UH* len, T_SNMP_BER* ber)
{
    UB* buf_ptr;
    INT enc_len;
    INT enc_l;
    UH i;
    UB* dat;

    /* Encode BER buffer */

    if (buf != 0x00 && len == 0) {
        return E_PAR;
    }
    buf_ptr = (buf != 0x00) ? (UB*)*buf : 0;

    /* Tag */
    if (buf_ptr != 0x00) {
        if (*len < 1) {
            return E_BOVR;
        }
        *buf_ptr++ = (UB)ber->typ;
        (*len)--;
    }
    enc_len = 1;

    /* Length */
    enc_l = snmp_ber_enc_len(&buf_ptr, len, ber->len);
    if (enc_l <= 0) {
        return enc_l;
    }
    enc_len += enc_l;

    /* Data */
    if (buf_ptr != 0x00) {
        if (*len < ber->len) {
            return E_BOVR;
        }
        dat = (UB*)ber->buf;
        for (i = 0; i < ber->len; i++) {
            *buf_ptr++ = dat[i];
            (*len)--;
        }
    }
    enc_len += ber->len;

    if (buf_ptr != 0x00) {
        *buf = buf_ptr;
    }

    return enc_len;
}

UW snmp_ber_atoi(const VB** buf, UH len)
{
    UW num;
    const VB* dat;
    UH i;

    /* ASCII to integer */

    if (buf == 0x00) {
        return 0;
    }
    dat = *buf;

    num = 0;
    for (i = 0; i < len; i++) {
        if (!(dat[i] >= '0' && dat[i] <= '9')) {
            break;
        }
        num = num * 10 + (dat[i] - '0');
    }

    *buf = dat + i;

    return num;
}

ER snmp_ber_set_any(T_SNMP_BER_ANY* ber, UH typ)
{
    /* Substitute any */

    ber->typ = typ;
    ber->len = 0;

    return E_OK;
}

ER snmp_ber_set_int(T_SNMP_BER_INT* ber, INT dat)
{
    /* Substitute integer */

    ber->typ = TYP_INT;
    ber->dat = dat;

    return E_OK;
}

ER snmp_ber_set_oct(T_SNMP_BER_OCT* ber, VP buf, UH len)
{
    /* Substitute octet string */

    ber->typ = TYP_OCT_STR;
    ber->buf = buf; 
    ber->len = len; 

    return E_OK;
}

ER snmp_ber_set_oid(T_SNMP_BER_OID* ber, VP buf, UH len)
{
    const VB* str;
    const VB* str_end;
    INT i;
    UW num;

    /* Substitute OID */

    /* Type */
    ber->typ = TYP_OBJ_ID;

    /* Data */
    str = (const VB*)buf;
    str_end = str + len;
    i = 0;
    while (str < str_end) {
        num = snmp_ber_atoi(&str, str_end - str);
        if (ber->buf_len <= i) {
            return E_OBJ;
        }
        ber->buf[i++] = num;
        
        if (str >= str_end) {
            break;
        }
        if (*str != '.') {
            return E_OBJ;
        }
        str++;
    }
    ber->len = i; 

    return E_OK;
}

ER snmp_ber_set_ip(T_SNMP_BER_IP* ber, UW adr)
{
    /* Substitute IP address */

    ber->typ = TYP_IP_ADR;
    ber->dat = adr;

    return E_OK;
}

ER snmp_ber_set_cnt(T_SNMP_BER_CNT* ber, UW dat)
{
    /* Substitute counter */

    ber->typ = TYP_CNT;
    ber->dat = dat;

    return E_OK;
}

ER snmp_ber_set_cnt64(T_SNMP_BER_CNT64* ber, UD_SNMP dat)
{
    ber->typ = TYP_CNT64;
    ber->dat = dat;

    return E_OK;
}

ER snmp_ber_set_gau(T_SNMP_BER_GAU* ber, UW dat)
{
    /* Substitute gauge */

    ber->typ = TYP_GAUGE;
    ber->dat = dat;

    return E_OK;
}

ER snmp_ber_set_tim(T_SNMP_BER_TIM* ber, UW dat)
{
    /* Substitute timetick */

    ber->typ = TYP_TIM_TIC;
    ber->dat = dat;

    return E_OK;
}

ER snmp_ber_cpy_oid(T_SNMP_BER_OID* dst_ber, T_SNMP_BER_OID* src_ber)
{
    INT i;

    /* Copy OID */

    if (dst_ber->buf_len < src_ber->len) {
        return E_OBJ;
    }

    /* Type */
    dst_ber->typ = TYP_OBJ_ID;

    /* Data */
    for (i = 0; i < src_ber->len; i++) {
        dst_ber->buf[i] = src_ber->buf[i]; 
    }
    dst_ber->len = src_ber->len; 

    return E_OK;
}

ER snmp_ber_cpy_buf(T_SNMP_BER* dst_ber, T_SNMP_BER* src_ber)
{
    /* Copy BER buffer */

    dst_ber->typ = src_ber->typ | TYP_BER_BUF;
    dst_ber->len = src_ber->len;
    dst_ber->buf = src_ber->buf;

    return E_OK;
}

ER snmp_ber_dec_any(T_SNMP_BER_ANY* ber, T_SNMP_BER* buf)
{
    /* Decode any */
    
    ber->typ = buf->typ;
    ber->len = buf->len;

    return E_OK;
}

ER snmp_ber_dec_int(T_SNMP_BER_INT* ber, T_SNMP_BER* buf)
{
    ER ercd;

    /* Decode integer */
    
    if (buf->typ != TYP_INT) {
        return E_PAR;
    }

    /* Type */
    ber->typ = TYP_INT;

    /* Data */
    ercd = snmp_ber_dec_val((UW*)&ber->dat, buf, TYP_INT);
    if (ercd != E_OK) {
        return E_PAR;
    }

    return E_OK;
}

ER snmp_ber_dec_oct(T_SNMP_BER_OCT* ber, T_SNMP_BER* buf)
{
    /* Decode octet string */

    if (buf->typ != TYP_OCT_STR) {
        return E_PAR;
    }

    ber->typ = TYP_OCT_STR;
    ber->buf = (UB*)buf->buf;
    ber->len = buf->len;

    return E_OK;
}

ER snmp_ber_dec_oid(T_SNMP_BER_OID* ber, T_SNMP_BER* buf)
{
    UB* buf_ptr;
    INT i;

    /* Decode OID */

    if (buf->typ != TYP_OBJ_ID) {
        return E_PAR;
    }
    if (buf->len < 1) {
        return E_PAR;
    }
    if (ber->buf_len < 3) {
        return E_PAR;
    }

    /* Type */
    ber->typ = TYP_OBJ_ID;

    /* Data */
    buf_ptr = (UB*)buf->buf;
    ber->buf[0] = buf_ptr[0] / 40;
    ber->buf[1] = buf_ptr[0] % 40;
    ber->buf[2] = 0;
    ber->len = 2;

    for (i = 1; i < buf->len; i++) {
        if ((buf_ptr[i] & 0x80) != 0x00) {
            ber->buf[ber->len] += buf_ptr[i] & 0x7f;
            ber->buf[ber->len] <<= 7;
        } else {
            ber->buf[ber->len] += buf_ptr[i];
            ber->len++;
            if (ber->len > ber->buf_len) {
                return E_OBJ;
            }
            ber->buf[ber->len] = 0;
        }
    }

    return E_OK;
}

ER snmp_ber_dec_ip(T_SNMP_BER_IP* ber, T_SNMP_BER* buf)
{
    UB* buf_ptr;

    /* Decode IP address */
    
    if (buf->typ != TYP_IP_ADR) {
        return E_PAR;
    }

    /* Type */
    ber->typ = TYP_IP_ADR;

    /* Data */
    buf_ptr = (UB*)buf->buf;
    ber->dat = (UW)buf_ptr[0] << 24;
    ber->dat |= (UW)buf_ptr[1] << 16;
    ber->dat |= (UW)buf_ptr[2] << 8;
    ber->dat |= (UW)buf_ptr[3];

    return E_OK;
}

ER snmp_ber_dec_cnt(T_SNMP_BER_CNT* ber, T_SNMP_BER* buf)
{
    ER ercd;

    /* Decode counter */
    
    if (buf->typ != TYP_CNT) {
        return E_PAR;
    }

    /* Type */
    ber->typ = TYP_CNT;

    /* Data */
    ercd = snmp_ber_dec_val(&ber->dat, buf, TYP_CNT);
    if (ercd != E_OK) {
        return E_PAR;
    }

    return E_OK;
}

ER snmp_ber_dec_cnt64(T_SNMP_BER_CNT64* ber, T_SNMP_BER* buf)
{
    ER ercd;

    /* Decode counter */
    
    if (buf->typ != TYP_CNT64) {
        return E_PAR;
    }

    /* Type */
    ber->typ = TYP_CNT64;

    /* Data */
    ercd = snmp_ber_dec_val64((UD_SNMP*)&ber->dat, buf, TYP_CNT64);
    if (ercd != E_OK) {
        return E_PAR;
    }

    return E_OK;
}

ER snmp_ber_dec_gau(T_SNMP_BER_GAU* ber, T_SNMP_BER* buf)
{
    ER ercd;

    /* Decode gauge */
    
    if (buf->typ != TYP_GAUGE) {
        return E_PAR;
    }

    /* Type */
    ber->typ = TYP_GAUGE;

    /* Data */
    ercd = snmp_ber_dec_val(&ber->dat, buf, TYP_GAUGE);
    if (ercd != E_OK) {
        return E_PAR;
    }

    return E_OK;
}

ER snmp_ber_dec_tim(T_SNMP_BER_TIM* ber, T_SNMP_BER* buf)
{
    ER ercd;

    /* Decode timetick */
    
    if (buf->typ != TYP_TIM_TIC) {
        return E_PAR;
    }

    /* Type */
    ber->typ = TYP_TIM_TIC;

    /* Data */
    ercd = snmp_ber_dec_val(&ber->dat, buf, TYP_TIM_TIC);
    if (ercd != E_OK) {
        return E_PAR;
    }

    return E_OK;
}

ER snmp_ber_dec(T_SNMP_BER* ber, VP* buf, UH* len, UB flg)
{
    UB* dat;
    UH dec_len;
    UB ber_len;

    /* Parsing BER data */

    if (*len == 0 || *buf == 0x00 || ber == 0x00) {
        return E_PAR;
    }
    if (*len < 2) {
        return E_PAR;    /* Buffer too short */
    }
    dat = (UB*)(*buf);

    /* Type */
    ber->typ = dat[0];
    dec_len = 1;

    /* Length */
    if ((dat[1] & LEN_LONG) == 0x00) {
        ber->len = dat[1];
        dec_len++;
    } else {
        /* 128-byte or more */
        ber_len = dat[1] & ~LEN_LONG;
        if (ber_len == 1) {
            if (*len < 3) {
                return E_PAR;    /* Buffer too short */
            }
            ber->len = dat[2];
            dec_len += 2;
        } else if (ber_len == 2) {
            if (*len < 4) {
                return E_PAR;    /* Buffer too short */
            }
            ber->len = dat[2] << 8;
            ber->len |= dat[3];
            dec_len += 3;
        } else {
            return E_OBJ;    /* Overflow (16-bit) */
        }
    }
    if (*len < dec_len + ber->len) {
        return E_OBJ;
    }

    /* Data */
    if ((flg & BER_TAG) != 0x00) {
        /* Tag */
        ber->buf = 0x00;
    } else {
        /* Data */
        ber->buf = (ber->len == 0) ? 0x00 : &dat[dec_len];
        dec_len += ber->len;    /* Data size */
    }

    *buf = (UB*)(*buf) + dec_len;
    *len = (*len >= dec_len) ? *len - dec_len : 0;

    return E_OK;
}

INT snmp_ber_enc_any(VP* buf, UH* len, T_SNMP_BER_ANY* ber)
{
    UB* buf_ptr;
    INT enc_len;
    INT enc_l;

    /* Encode any */

    if (buf != 0x00 && len == 0) {
        return E_PAR;
    }
    buf_ptr = (buf != 0x00) ? (UB*)*buf : 0;

    /* Tag */
    if (buf_ptr != 0x00) {
        if (*len < 1) {
            return E_BOVR;
        }
        *buf_ptr++ = (UB)ber->typ;
        (*len)--;
    }
    enc_len = 1;

    /* Length */
    enc_l = snmp_ber_enc_len(&buf_ptr, len, ber->len);
    if (enc_l <= 0) {
        return enc_l;
    }
    enc_len += enc_l;

    if (buf_ptr != 0x00) {
        *buf = buf_ptr;
    }

    return enc_len;
}

INT snmp_ber_enc_int(VP* buf, UH* len, T_SNMP_BER_INT* ber)
{
    INT enc_len;

    /* Encode integer */

    enc_len = snmp_ber_enc_val(buf, len, TYP_INT, (UW)ber->dat);

    return enc_len;
}

INT snmp_ber_enc_oct(VP* buf, UH* len, T_SNMP_BER_OCT* ber)
{
    UB* buf_ptr;
    INT enc_len;
    INT enc_l;
    UH i;

    /* Encode octet string */

    if (buf != 0x00 && len == 0) {
        return E_PAR;
    }
    buf_ptr = (buf != 0x00) ? (UB*)*buf : 0;

    /* Tag */
    if (buf_ptr != 0x00) {
        if (*len < 1) {
            return E_BOVR;
        }
        *buf_ptr++ = TYP_OCT_STR;
        (*len)--;
    }
    enc_len = 1;

    /* Length */
    enc_l = snmp_ber_enc_len(&buf_ptr, len, ber->len);
    if (enc_l <= 0) {
        return enc_l;
    }
    enc_len += enc_l;

    /* Data */
    if (buf_ptr != 0x00) {
        if (*len < ber->len) {
            return E_BOVR;
        }
        for (i = 0; i < ber->len; i++) {
            *buf_ptr++ = ber->buf[i];
            (*len)--;
        }
    }
    enc_len += ber->len;

    if (buf_ptr != 0x00) {
        *buf = buf_ptr;
    }

    return enc_len;
}

INT snmp_ber_enc_oid(VP* buf, UH* len, T_SNMP_BER_OID* ber)
{
    UB* buf_ptr;
    INT enc_len;
    UH dat_len;
    UH i;
    INT enc_l;
    INT sft;

    /* Encode OID */

    if (buf != 0x00 && len == 0) {
        return E_PAR;
    }
    buf_ptr = (buf != 0x00) ? (UB*)*buf : 0;

    /* Tag */
    if (buf_ptr != 0x00) {
        if (*len < 1) {
            return E_BOVR;
        }
        *buf_ptr++ = TYP_OBJ_ID;
        (*len)--;
    }
    enc_len = 1;

    /* Length */
    dat_len = 1;
    for (i = 2; i < ber->len; i++) {
        if (ber->buf[i] < 0x80) {
            dat_len += 1;
        } else if (ber->buf[i] < 0x4000) {
            dat_len += 2;
        } else if (ber->buf[i] < 0x200000) {
            dat_len += 3;
        } else if (ber->buf[i] < 0x10000000) {
            dat_len += 4;
        } else {
            dat_len += 5;
        }
    }
    enc_l = snmp_ber_enc_len(&buf_ptr, len, dat_len);
    if (enc_l <= 0) {
        return enc_l;
    }
    enc_len += enc_l;

    /* Data */
    if (buf_ptr != 0x00) {
        if (*len < dat_len) {
            return E_BOVR;
        }
        if (ber->len < 2) {
            *buf_ptr++ = 0;    /* Null OID */
        } else {
            *buf_ptr++ = ber->buf[0] * 40 + ber->buf[1];
        }
        (*len)--;
        
        for (i = 2; i < ber->len; i++) {
            if (ber->buf[i] < 0x80) {
                enc_l = 1;
            } else if (ber->buf[i] < 0x4000) {
                enc_l = 2;
            } else if (ber->buf[i] < 0x200000) {
                enc_l = 3;
            } else if (ber->buf[i] < 0x10000000) {
                enc_l = 4;
            } else {
                enc_l = 5;
            }
            *len -= enc_l;
            sft = 7 * (enc_l - 1);
            for (; enc_l > 1; enc_l--) {
                *buf_ptr++ = 0x80 | (UB)((ber->buf[i] >> sft) & 0x007f);
                sft -= 7;
            }
            *buf_ptr++ = ber->buf[i] & 0x7f;
        }
    }
    enc_len += dat_len;

    if (buf_ptr != 0x00) {
        *buf = buf_ptr;
    }

    return enc_len;
}

INT snmp_ber_enc_ip(VP* buf, UH* len, T_SNMP_BER_IP* ber)
{
    UB* buf_ptr;
    INT enc_len;
    INT enc_l;

    /* Encode IP address */

    if (buf != 0x00 && len == 0) {
        return E_PAR;
    }
    buf_ptr = (buf != 0x00) ? (UB*)*buf : 0x00;

    /* Tag */
    if (buf_ptr != 0x00) {
        if (*len < 1) {
            return E_BOVR;
        }
        *buf_ptr++ = TYP_IP_ADR;
        (*len)--;
    }
    enc_len = 1;

    /* Length */
    enc_l = snmp_ber_enc_len(&buf_ptr, len, 4);
    if (enc_l <= 0) {
        return enc_l;
    }
    enc_len += enc_l;

    /* Data */
    if (buf_ptr != 0x00) {
        if (*len < 4) {
            return E_BOVR;
        }
        *buf_ptr++ = (UB)(ber->dat >> 24);
        *buf_ptr++ = (UB)(ber->dat >> 16);
        *buf_ptr++ = (UB)(ber->dat >> 8);
        *buf_ptr++ = (UB)ber->dat;
        (*len) -= 4;
    }
    enc_len += 4;

    if (buf_ptr != 0x00) {
        *buf = buf_ptr;
    }

    return enc_len;
}

INT snmp_ber_enc_cnt(VP* buf, UH* len, T_SNMP_BER_CNT* ber)
{
    INT enc_len;

    /* Encode counter */

    enc_len = snmp_ber_enc_val(buf, len, TYP_CNT, ber->dat);

    return enc_len;
}

INT snmp_ber_enc_cnt64(VP* buf, UH* len, T_SNMP_BER_CNT64* ber)
{
    INT enc_len;

    /* Encode counter */

    enc_len = snmp_ber_enc_val64(buf, len, TYP_CNT64, ber->dat);

    return enc_len;
}

INT snmp_ber_enc_gau(VP* buf, UH* len, T_SNMP_BER_GAU* ber)
{
    INT enc_len;

    /* Encode gauge */

    enc_len = snmp_ber_enc_val(buf, len, TYP_GAUGE, ber->dat);

    return enc_len;
}

INT snmp_ber_enc_tim(VP* buf, UH* len, T_SNMP_BER_TIM* ber)
{
    INT enc_len;

    /* Encode timetick */

    enc_len = snmp_ber_enc_val(buf, len, TYP_TIM_TIC, ber->dat);

    return enc_len;
}

INT snmp_ber_enc(VP* buf, UH* len, T_SNMP_BER_BUF* ber)
{
    UH enc_len;
    T_SNMP_BER_ANY* ber_any;

    /* Encode BER buffer */

    if (buf != 0x00 && len == 0) {
        return E_PAR;
    }

    ber_any = (T_SNMP_BER_ANY*)ber;

    if ((ber_any->typ & TYP_BER_BUF) != 0x00) {
        enc_len = snmp_ber_enc_buf(buf, len, (T_SNMP_BER*)ber);
        return enc_len;
    } else if ((ber_any->typ & TYP_EXC) != 0x00) {
        enc_len = snmp_ber_enc_any(buf, len, (T_SNMP_BER_ANY*)ber);
        return enc_len;
    }

    switch(ber_any->typ) {
        case TYP_INT:
            enc_len = snmp_ber_enc_int(buf, len, (T_SNMP_BER_INT*)ber);
            break;
        case TYP_OCT_STR:
            enc_len = snmp_ber_enc_oct(buf, len, (T_SNMP_BER_OCT*)ber);
            break;
        case TYP_OBJ_ID:
            enc_len = snmp_ber_enc_oid(buf, len, (T_SNMP_BER_OID*)ber);
            break;
        case TYP_CNT:
            enc_len = snmp_ber_enc_cnt(buf, len, (T_SNMP_BER_CNT*)ber);
            break;
        case TYP_CNT64:
            enc_len = snmp_ber_enc_cnt64(buf, len, (T_SNMP_BER_CNT64*)ber);
            break;
        case TYP_GAUGE:
            enc_len = snmp_ber_enc_gau(buf, len, (T_SNMP_BER_GAU*)ber);
            break;
        case TYP_TIM_TIC:
            enc_len = snmp_ber_enc_tim(buf, len, (T_SNMP_BER_TIM*)ber);
            break;
        case TYP_IP_ADR:
            enc_len = snmp_ber_enc_ip(buf, len, (T_SNMP_BER_IP*)ber);
            break;
        case TYP_NULL:
            enc_len = snmp_ber_enc_any(buf, len, (T_SNMP_BER_ANY*)ber);
            break;
        default:
            return E_OBJ;
    }

    return enc_len;
}

ER snmp_ber_str_oid(T_SNMP_BER_OID* ber, VB* buf, UH len)
{
    VB d[5];
    UW w;
    UH i;
    INT j;

    if (ber == 0x00 || ber->typ != TYP_OBJ_ID) {
        return E_PAR;
    }
    for (i = 0; i < ber->len; i++) {

        w = *(ber->buf + i);
        if (w > 0x10000) {
            return E_PAR;
        }

        for (j = 0; j < 5; j++) {
            d[j] = '0' + (w % 10);
            w = w / 10;
            if (w == 0) {
                break;
            }
        }
        if (j+2 > len) {   /* include '.' or null */
            return E_PAR;
        }
        len -= (j+2);
        for (; j >= 0; j--) {
            *buf++ = d[j];
        }
        *buf++ = '.';
    }
    *(buf-1) = 0x00;
    return E_OK;
}


