/*
    SNMP
    Basic encoding rules
    Copyright (c) 2014-2019, eForce Co., Ltd. All rights reserved.
    
    2014-03-06 Created
    2017-04-20 Support OID type for private MIB data
    2019-02-25 Support Counter64(SNMP_TYP_CNT64) type for private MIB data.
*/

#ifndef SNMP_BER_H
#define SNMP_BER_H

/* Type */
/*      TYP_NONE                0x0000       None */
#define TYP_BOOL                0x0001    /* Boolean */
/*      TYP_INT                 0x0002       Integer */
#define TYP_BIT_STR             0x0003    /* Bit string */
/*      TYP_OCT_STR             0x0004       Octet string */
#define TYP_NULL                0x0005    /* Null */
/*      TYP_OBJ_ID              0x0006       Object identifier */
/*      TYP_SEQ                 0x0030       Sequence */
/*      TYP_IP_ADR              0x0040       IP address */
/*      TYP_CNT                 0x0041       Counter (32) */
/*      TYP_GAUGE               0x0042       Gauge (32) */
/*      TYP_TIM_TIC             0x0043       Time ticks */
#define TYP_OPAQUE              0x0044    /* Opaque */
#define TYP_NSAP_ADR            0x0045    /* NsapAddress */
/* #define TYP_CNT64               0x0046    Big Counter (64) */
#define TYP_UINT32              0x0047    /* Unsigned integer (32) */

/* Exception */
#define TYP_EXC                 0x0080    /* Exception bit */
#define TYP_EXC_NO_SUCH_OBJ     0x0080    /* NoSuchObject */
#define TYP_EXC_NO_SUCH_INS     0x0081    /* NoSuchInstance */
#define TYP_EXC_END_MIB_VIEW    0x0082    /* EndOfMibView */

/* Internal flgs */
#define TYP_BER_BUF             0x0800    /* Plain buffer */

/* Tag flag */
#define BER_TAG         0x01    /* Tag */
#define BER_DAT         0x02    /* Data */

/* Buffer pointer */
typedef struct t_snmp_ber {
    UH typ;     /* Type */
    VP buf;     /* Data buffer */
    UH len;     /* Length */
} T_SNMP_BER;

/* Any */
typedef struct t_snmp_ber_any {
    UH typ;     /* Type */
    UH len;     /* Length */
} T_SNMP_BER_ANY;

/* Integer */
typedef struct t_snmp_ber_int {
    UH typ;     /* Type */
    INT dat;    /* Data */
} T_SNMP_BER_INT;

/* Octet string */
typedef struct t_snmp_ber_oct {
    UH typ;     /* Type */
    UB* buf;    /* Data pointer */
    UH len;     /* Data size */
} T_SNMP_BER_OCT;

/* Object ID */
typedef struct t_snmp_ber_oid {
    UH typ;     /* Type */
    UW* buf;    /* Data pointer */
    UH buf_len; /* Buffer size */
    UH len;     /* Data size */
} T_SNMP_BER_OID;

/* IP address */
typedef struct t_snmp_ber_ip {
    UH typ;     /* Type */
    UW dat;     /* Data */
} T_SNMP_BER_IP;

/* Counter */
typedef struct t_snmp_ber_cnt {
    UH typ;     /* Type */
    UW dat;     /* Data */
} T_SNMP_BER_CNT;

/* Gauge */
typedef struct t_snmp_ber_gau {
    UH typ;     /* Type */
    UW dat;     /* Data */
} T_SNMP_BER_GAU;

/* Timetick */
typedef struct t_snmp_ber_tim {
    UH typ;     /* Type */
    UW dat;     /* Data */
} T_SNMP_BER_TIM;

/* Big counter(64) */
typedef struct t_snmp_ber_cnt64 {
    UH typ;     /* Type */
    #if 0
    UD dat;     /* Data */
    #else
    unsigned long long dat;
    #endif
} T_SNMP_BER_CNT64;

/* Memory buffer */
typedef struct t_snmp_ber_buf {
    union {
        T_SNMP_BER       dmy0;
        T_SNMP_BER_ANY   dmy1;
        T_SNMP_BER_INT   dmy2;
        T_SNMP_BER_OCT   dmy3;
        T_SNMP_BER_OID   dmy4;
        T_SNMP_BER_IP    dmy5;
        T_SNMP_BER_CNT   dmy6;
        T_SNMP_BER_GAU   dmy7;
        T_SNMP_BER_TIM   dmy8;
        T_SNMP_BER_CNT64 dmy9;
    } snmp_ber_buf_all;
} T_SNMP_BER_BUF;

/* Function Prototypes */
UW snmp_ber_atoi(const VB**, UH);
ER snmp_ber_set_any(T_SNMP_BER_ANY*, UH);
ER snmp_ber_set_int(T_SNMP_BER_INT*, INT);
ER snmp_ber_set_oct(T_SNMP_BER_OCT*, VP, UH);
ER snmp_ber_set_null(T_SNMP_BER_ANY*);
ER snmp_ber_set_oid(T_SNMP_BER_OID*, VP, UH);
ER snmp_ber_set_ip(T_SNMP_BER_IP*, UW);
ER snmp_ber_set_cnt(T_SNMP_BER_CNT*, UW);
ER snmp_ber_set_cnt64(T_SNMP_BER_CNT64* ber, UD_SNMP dat);
ER snmp_ber_set_gau(T_SNMP_BER_GAU*, UW);
ER snmp_ber_set_tim(T_SNMP_BER_TIM*, UW);
ER snmp_ber_cpy_oid(T_SNMP_BER_OID*, T_SNMP_BER_OID*);
ER snmp_ber_cpy_buf(T_SNMP_BER*, T_SNMP_BER*);
ER snmp_ber_str_oid(T_SNMP_BER_OID*, VB*, UH);
ER snmp_ber_dec_any(T_SNMP_BER_ANY*, T_SNMP_BER*);
ER snmp_ber_dec_int(T_SNMP_BER_INT*, T_SNMP_BER*);
ER snmp_ber_dec_oct(T_SNMP_BER_OCT*, T_SNMP_BER*);
ER snmp_ber_dec_null(T_SNMP_BER_ANY*, T_SNMP_BER*);
ER snmp_ber_dec_oid(T_SNMP_BER_OID*, T_SNMP_BER*);
ER snmp_ber_dec_ip(T_SNMP_BER_IP*, T_SNMP_BER*);
ER snmp_ber_dec_cnt(T_SNMP_BER_CNT*, T_SNMP_BER*);
ER snmp_ber_dec_cnt64(T_SNMP_BER_CNT64* ber, T_SNMP_BER* buf);
ER snmp_ber_dec_gau(T_SNMP_BER_GAU*, T_SNMP_BER*);
ER snmp_ber_dec_tim(T_SNMP_BER_TIM*, T_SNMP_BER*);
ER snmp_ber_dec(T_SNMP_BER*, VP*, UH*, UB);
INT snmp_ber_enc_any(VP*, UH*, T_SNMP_BER_ANY*);
INT snmp_ber_enc_int(VP*, UH*, T_SNMP_BER_INT*);
INT snmp_ber_enc_oct(VP*, UH*, T_SNMP_BER_OCT*);
INT snmp_ber_enc_nul(VP*, UH*, T_SNMP_BER_ANY*);
INT snmp_ber_enc_oid(VP*, UH*, T_SNMP_BER_OID*);
INT snmp_ber_enc_ip(VP*, UH*, T_SNMP_BER_IP*);
INT snmp_ber_enc_cnt(VP*, UH*, T_SNMP_BER_CNT*);
INT snmp_ber_enc_gau(VP*, UH*, T_SNMP_BER_GAU*);
INT snmp_ber_enc_tim(VP*, UH*, T_SNMP_BER_TIM*);
INT snmp_ber_enc(VP*, UH*, T_SNMP_BER_BUF*);

/* Conversion macros */
#define ber_atoi        snmp_ber_atoi
#define ber_set_any     snmp_ber_set_any
#define ber_set_int     snmp_ber_set_int
#define ber_set_oct     snmp_ber_set_oct
#define ber_set_nul     snmp_ber_set_nul
#define ber_set_oid     snmp_ber_set_oid
#define ber_set_ip      snmp_ber_set_ip
#define ber_set_cnt     snmp_ber_set_cnt
#define ber_set_cnt64   snmp_ber_set_cnt64
#define ber_set_gau     snmp_ber_set_gau
#define ber_set_tim     snmp_ber_set_tim
#define ber_cpy_oid     snmp_ber_cpy_oid
#define ber_cpy_buf     snmp_ber_cpy_buf
#define ber_dec_any     snmp_ber_dec_any
#define ber_dec_int     snmp_ber_dec_int
#define ber_dec_oct     snmp_ber_dec_oct
#define ber_dec_nul     snmp_ber_dec_nul
#define ber_dec_oid     snmp_ber_dec_oid
#define ber_dec_ip      snmp_ber_dec_ip
#define ber_dec_cnt     snmp_ber_dec_cnt
#define ber_dec_gau     snmp_ber_dec_gau
#define ber_dec_tim     snmp_ber_dec_tim
#define ber_dec         snmp_ber_dec
#define ber_enc_any     snmp_ber_enc_any
#define ber_enc_int     snmp_ber_enc_int
#define ber_enc_oct     snmp_ber_enc_oct
#define ber_enc_nul     snmp_ber_enc_nul
#define ber_enc_oid     snmp_ber_enc_oid
#define ber_enc_ip      snmp_ber_enc_ip
#define ber_enc_cnt     snmp_ber_enc_cnt
#define ber_enc_gau     snmp_ber_enc_gau
#define ber_enc_tim     snmp_ber_enc_tim
#define ber_enc         snmp_ber_enc

#endif

