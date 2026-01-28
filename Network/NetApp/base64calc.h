/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    BASE64 Algorithm Implementation header file
    Copyright (c)  2013, eForce Co., Ltd. All rights reserved.

    Version Information
      2013.09.27: Created
      2016.02.10: Add include files for warning avoidance
 ***************************************************************************/

#ifndef BASE64CALC_H
#define BASE64CALC_H
#ifdef __cplusplus
extern "C" {
#endif

#include "kernel.h"

typedef struct t_base64_info {
    VB *dst;    /* [io] Destination Buffer */
    VB *src;    /* [i ] Source Buffer */
    H dlen;     /* [i ] DestBuf Length */
    H slen;     /* [i ] SrcBuf Length */
    H rdlen;    /* [ o] Read SrcBuf Count */
    H rslen;    /* [ o] Write DestBuf Count */
    UB end;     /* [ o] Convert end */
} T_BASE64_INFO;

ER base64dec(T_BASE64_INFO *b64i);
ER base64enc(T_BASE64_INFO *b64i);
ER mime_base64enc(T_BASE64_INFO *b64i, UB *pos);

#define SET_B64I(b64i, d, s, dl, sl)    \
    (b64i)->dst = (d); (b64i)->src = (s); (b64i)->dlen = (dl); (b64i)->slen = (sl);

#define INI_B64I(b64i)  net_memset((b64i), 0, sizeof(*(b64i)));
    
#ifdef __cplusplus
}
#endif
#endif /* BASE64CALC_H */
