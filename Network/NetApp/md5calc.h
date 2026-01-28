/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    MD5 Message-Digest Algorithm Implementation header file
    Copyright (c)  2013, eForce Co., Ltd. All rights reserved.

    Version Information
      2013.06.24: Created
      2013.09.27: Add HMAC-MD5 processing function
      2016.02.10: Add include files for warning avoidance
 ***************************************************************************/

#ifndef MD5CALC_H
#define MD5CALC_H
#ifdef __cplusplus
extern "C" {
#endif

#include "kernel.h"

#define HASHLEN     16
typedef char HASH[HASHLEN];
#define HASHHEXLEN  32
typedef char HASHHEX[HASHHEXLEN + 1];
    
typedef struct md5_info {
    UW len;
    UW var[4];
    UB buf[64];
} MD5_INFO;

void md5i_init(MD5_INFO *md5i);
void md5i_append(MD5_INFO *md5i, const UB *msg, UW msg_len);
void md5i_finish(MD5_INFO *md5i, HASH hash);

void md5_hash(UB *bytes, UW len, HASH hash);
void cnv_hex(HASH bin, HASHHEX hex);
void cnv_n_hex(char *bin, char *hex, UW len);

void hmac_md5(UB *txt, W txt_len, UB *k, W k_len, HASH *ha);

#ifdef __cplusplus
}
#endif
#endif /* MD5CALC_H */
