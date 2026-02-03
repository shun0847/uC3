/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    BASE64 Algorithm Implementation
    Copyright (c)  2013, eForce Co., Ltd. All rights reserved.

    Version Information
      2013.09.27: Created
      2014.07.10: Suppressed warning of the GCC compiler.
 ***************************************************************************/

#include "kernel.h"
#include "base64calc.h"

static const UB cnvtbl[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* decode base64 core process */
UB dec64_core(VB *d, VB *s)
{
    VB t[4] = {0};
    UB nx, ny, pad;
    
    /* convtbl -> 6bit*4 -> 8bit*3 */
    pad = 0;
    for (nx = 0; nx < 4; ++nx) {
        for (ny = 0; ny < sizeof(cnvtbl)/sizeof(cnvtbl[0]); ++ny) {
            if (cnvtbl[ny] == s[nx]) {
                t[nx] = ny;
                break;
            }
        }
        if (ny == sizeof(cnvtbl)/sizeof(cnvtbl[0])) {
            pad = 4 - nx;
            break;
        }
    }
    
    d[0] = t[0] << 2 | t[1] >> 4;
    d[1] = t[1] << 4 | t[2] >> 2;
    d[2] = t[2] << 6 | t[3] ;
    
    return pad;
}

/* decode base64 */
ER base64dec(T_BASE64_INFO *b64i)
{
    VB *d, *s;
    H nx, ny;
    
    /* check parameter */
    if ((!b64i) || (!b64i->dst) || (!b64i->src)) {
        return E_PAR;
    }
    
    /* decode base64 */
    nx = b64i->slen;
    ny = b64i->dlen;
    
    for (d = b64i->dst, s = b64i->src; (4 <= nx && 3 <= ny); d += 3, s += 4, nx -= 4, ny -= 3) {
        d -= dec64_core(d, s);
    }
    if (nx && 3 <= ny) {     /* Src length is not a multiple of 4 */
        dec64_core(d, s);
        d += nx;
    }    
    
    b64i->rdlen = d - b64i->dst;
    b64i->rslen = s - b64i->src;
    
    return E_OK;
}

/* encode base64 core process */
void enc64_core(VB *d, VB *s)
{
    /* 8bit*3 -> 6bit*4 -> convtbl */
    d[0] = cnvtbl[ 0x3F & (s[0] >> 2)               ];
    d[1] = cnvtbl[ 0x3F & ((s[0] << 4) | (s[1] >> 4)) ];
    d[2] = cnvtbl[ 0x3F & ((s[1] << 2) | (s[2] >> 6)) ];
    d[3] = cnvtbl[ 0x3F & (s[2])                    ];
}


ER check_base64mem(H dlen, H slen, UB mime)
{
    if (mime) {
        /* Size increases about 34%-37% as the base64 */
        if ((dlen * 134 + (slen / 57)) < (slen * 100)) {
            return E_NOMEM;
        }
    }
    else {
        /* Size increases about 34% as the base64 */
        if ((dlen * 134) < (slen * 100)) {
            return E_NOMEM;
        }
    }
    
    return E_OK;
}

/* encode base64 */
ER base64enc(T_BASE64_INFO *b64i)
{
    VB *d, *s, t[3];
    H nx, ny;
    
    /* check parameter */
    if ((!b64i) || (!b64i->dst) || (!b64i->src)) {
        return E_PAR;
    }
    
    /* encode base64 */
    nx = b64i->slen;
    ny = b64i->dlen;
    
    for (d = b64i->dst, s = b64i->src; (3 <= nx && 4 <= ny); d += 4, s += 3, nx -= 3, ny -= 4) {
        enc64_core(d, s);
    }
    if (nx && 4 <= ny) {   /* Src length is not a multiple of 3 */
        t[0] = *s++;
        t[1] = (1 == nx) ? 0 : *s++ ;
        t[2] = 0;
        enc64_core(d, t);
        d += (1 == nx) ? 2 : 3;
        
        /* padding */
        for (nx = 3 - nx; nx; --nx)    *(d++) = '=';
    }
    b64i->rdlen = d - b64i->dst;
    b64i->rslen = s - b64i->src;
    b64i->end = (b64i->rslen == b64i->slen) ? 1 : 0 ;
    
    return E_OK;
}

ER mime_base64enc(T_BASE64_INFO *b64i, UB *pos)
{
    ER ercd;
    T_BASE64_INFO t64i;
    
    ercd = E_OK;
    t64i = *b64i;
    t64i.rslen = 0;
    t64i.rdlen = 0;
    
    for (; 4 <= t64i.dlen;) {
        if (57 <= *pos) {
            *pos = 0;
            *t64i.dst++ = '\r';
            *t64i.dst++ = '\n';
            t64i.dlen -= 2;
        }

        t64i.slen = b64i->slen - (t64i.src - b64i->src);
        if (0 >= t64i.slen)  break;
        
        ercd = 57 - *pos;
        if (t64i.slen > ercd)   t64i.slen = ercd;
        ercd = base64enc(&t64i);
        if (E_OK != ercd) {
            break;
        }
        t64i.src += t64i.rslen;
        t64i.dst += t64i.rdlen;
        t64i.dlen -= t64i.rdlen;
        *pos += t64i.rslen;
    }
    
    b64i->rslen = t64i.src - b64i->src;
    b64i->rdlen = t64i.dst - b64i->dst;
    b64i->end = (b64i->rslen == b64i->slen) ? 1 : 0 ;
    
    return ercd;
}

