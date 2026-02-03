/*
    SNMP
    Vendor MIB definition 
    Copyright (c) 2016-2023, eForce Co., Ltd. All rights reserved.
    
    2016-01-06 Created
    2017-11-30 Add OID type for private MIB data
    2023-05-31 Changed character encoding to UTF8.
*/

#include "kernel.h"
#include "snmp.h"
#include "snmp_cfg.h"

/* Vendor extensions and implementations */

/* Macros */
#define LEN_INT    4        /* Length of INT,CNT,GAUGE,IP_ADR */
#define CBK_NONE   0x00     /* Callback function not defined */

/* MIB strings */

/* Vendor Descr */
#define DESCR_LEN    (64 + 1)    /* Maximum string size (with null terminator) */
static VB snmp_mib_ven_descr[DESCR_LEN] = {
    "Vendor MIB"
};

/* Version Descr */
static VB snmp_mib_ven_ver[DESCR_LEN] = {
    "Version 0.0.0"
};

/* User name */
#define USER_LEN    (32 + 1)
static VB snmp_mib_ven_user_name[USER_LEN] = {
    "User name"
};

/* Obect identifier */
#define OIDSTR_LEN (60)
static VB snmp_mib_ven_obj_id[OIDSTR_LEN] = {
    "1.22.333.4444.55555.6.7.8.9.10"
};

/* Disk name */
static const VB snmp_mib_ven_disk_name_1[DESCR_LEN] = {
    "Disk 1 [Main]"
};
static const VB snmp_mib_ven_disk_name_2[DESCR_LEN] = {
    "Disk 2 [Sub]"
};
static const VB snmp_mib_ven_disk_name_200[DESCR_LEN] = {
    "Disk 200 [Ext]"
};
static const VB snmp_mib_ven_disk_name_551[DESCR_LEN] = {
    "Disk 551 [Ext]"
};

/* MIB OID prefix */
const VB snmp_mib_ven_pre_0[] = "1.3.6.1.4.1.1234.";    /* Prefix OID (MIB 0) */
const VB snmp_mib_ven_pre_1[] = "1.3.6.1.4.1.5678.";    /* Prefix OID (MIB 1) */

/* MIB 0 OID */
const VB snmp_mib_1234_1_1[] = "1.1.0";     /* Descr      (1.3.6.1.4.1.1234.1.1) */
const VB snmp_mib_1234_1_2[] = "1.2.0";     /* Version    (1.3.6.1.4.1.1234.1.2) */
const VB snmp_mib_1234_1_3[] = "1.3.0";     /* User name  (1.3.6.1.4.1.1234.1.3) */
const VB snmp_mib_1234_1_4[] = "1.4.0";     /* Time ticks (1.3.6.1.4.1.1234.1.4) */
const VB snmp_mib_1234_1_5[] = "1.5.0";     /* Status     (1.3.6.1.4.1.1234.1.5) */
const VB snmp_mib_1234_1_6[] = "1.6.0";     /* IP address (1.3.6.1.4.1.1234.1.6) */
const VB snmp_mib_1234_1_7[] = "1.7.0";     /* Counter    (1.3.6.1.4.1.1234.1.7) */
const VB snmp_mib_1234_1_8[] = "1.8.0";     /* Gauge      (1.3.6.1.4.1.1234.1.8) */
const VB snmp_mib_1234_1_9[] = "1.9.0";     /* Identifier (1.3.6.1.4.1.1234.1.9) */

/* MIB 1 OID */
const VB snmp_mib_5678_1_1[]            = "1.1.0";          /* Descr             (1.3.6.1.4.1.5678.1.1) */
const VB snmp_mib_5678_1_2[]            = "1.2.0";          /* Version           (1.3.6.1.4.1.5678.1.2) */
const VB snmp_mib_5678_1_3[]            = "1.3.0";          /* Status            (1.3.6.1.4.1.5678.1.3) */
const VB snmp_mib_5678_1_8[]            = "1.8.0";          /* Disk              (1.3.6.1.4.1.5678.1.8) */
const VB snmp_mib_5678_1_8_1[]          = "1.8.1.0";        /* Disk number       (1.3.6.1.4.1.5678.1.8.1) */
const VB snmp_mib_5678_1_8_2[]          = "1.8.2.0";        /* Disk table        (1.3.6.1.4.1.5678.1.8.2) */
                                                            /* Start entry */
const VB snmp_mib_5678_1_8_2_1[]        = "1.8.2.1";        /* Disk entry        (1.3.6.1.4.1.5678.1.8.2.1) */
const VB snmp_mib_5678_1_8_2_1_1_1[]    = "1.8.2.1.1.1";    /* Disk #1   ifIndex (1.3.6.1.4.1.5678.1.8.2.1.1.1) */
const VB snmp_mib_5678_1_8_2_1_1_2[]    = "1.8.2.1.1.2";    /* Disk #2   ifIndex (1.3.6.1.4.1.5678.1.8.2.1.1.2) */
const VB snmp_mib_5678_1_8_2_1_1_200[]  = "1.8.2.1.1.200";  /* Disk #200 ifIndex (1.3.6.1.4.1.5678.1.8.2.1.1.200) */
const VB snmp_mib_5678_1_8_2_1_1_551[]  = "1.8.2.1.1.551";  /* Disk #551 ifIndex (1.3.6.1.4.1.5678.1.8.2.1.1.551) */
const VB snmp_mib_5678_1_8_2_1_2_1[]    = "1.8.2.1.2.1";    /* Disk #1   descr   (1.3.6.1.4.1.5678.1.8.2.1.2.1) */
const VB snmp_mib_5678_1_8_2_1_2_2[]    = "1.8.2.1.2.2";    /* Disk #2   descr   (1.3.6.1.4.1.5678.1.8.2.1.2.2) */
const VB snmp_mib_5678_1_8_2_1_2_200[]  = "1.8.2.1.2.200";  /* Disk #200 descr   (1.3.6.1.4.1.5678.1.8.2.1.2.200) */
const VB snmp_mib_5678_1_8_2_1_2_551[]  = "1.8.2.1.2.551";  /* Disk #551 descr   (1.3.6.1.4.1.5678.1.8.2.1.2.551) */
const VB snmp_mib_5678_1_8_2_1_17_1[]   = "1.8.2.1.17.1";   /* Disk #1   size    (1.3.6.1.4.1.5678.1.8.2.1.17.1) */
const VB snmp_mib_5678_1_8_2_1_17_2[]   = "1.8.2.1.17.2";   /* Disk #2   size    (1.3.6.1.4.1.5678.1.8.2.1.17.2) */
const VB snmp_mib_5678_1_8_2_1_17_200[] = "1.8.2.1.17.200"; /* Disk #200 size    (1.3.6.1.4.1.5678.1.8.2.1.17.200) */
const VB snmp_mib_5678_1_8_2_1_17_551[] = "1.8.2.1.17.551"; /* Disk #551 size    (1.3.6.1.4.1.5678.1.8.2.1.17.551) */
const VB snmp_mib_5678_1_8_2_1_18_1[]   = "1.8.2.1.18.1";   /* Disk #1   free    (1.3.6.1.4.1.5678.1.8.2.1.18.1) */
const VB snmp_mib_5678_1_8_2_1_18_2[]   = "1.8.2.1.18.2";   /* Disk #2   free    (1.3.6.1.4.1.5678.1.8.2.1.18.2) */
const VB snmp_mib_5678_1_8_2_1_18_200[] = "1.8.2.1.18.200"; /* Disk #200 free    (1.3.6.1.4.1.5678.1.8.2.1.18.200) */
const VB snmp_mib_5678_1_8_2_1_18_551[] = "1.8.2.1.18.551"; /* Disk #551 free    (1.3.6.1.4.1.5678.1.8.2.1.18.551) */
                                                            /* End of entry */
const VB snmp_mib_5678_1_9[]            = "1.9.0";          /* Gauge 1           (1.3.6.1.4.1.5678.1.9) */
const VB snmp_mib_5678_2_32[]           = "2.32.0";         /* Memory            (1.3.6.1.4.1.5678.2.32) */
const VB snmp_mib_5678_2_32_1[]         = "2.32.1.0";       /* Memory number     (1.3.6.1.4.1.5678.2.32.1) */
const VB snmp_mib_5678_2_32_2[]         = "2.32.2.0";       /* Memory table      (1.3.6.1.4.1.5678.2.32.2) */
                                                            /* Start entry */
const VB snmp_mib_5678_2_32_2_1[]       = "2.32.2.1";       /* Memory entry      (1.3.6.1.4.1.5678.2.32.2.1) */
const VB snmp_mib_5678_2_32_2_1_1_0[]   = "2.32.2.1.1.0";   /* Memory #0 ifIndex (1.3.6.1.4.1.5678.2.32.2.1.1.0) */
const VB snmp_mib_5678_2_32_2_1_1_1[]   = "2.32.2.1.1.1";   /* Memory #1 ifIndex (1.3.6.1.4.1.5678.2.32.2.1.1.1) */
const VB snmp_mib_5678_2_32_2_1_2_0[]   = "2.32.2.1.2.0";   /* Memory #0 size    (1.3.6.1.4.1.5678.2.32.2.1.2.0) */
const VB snmp_mib_5678_2_32_2_1_2_1[]   = "2.32.2.1.2.1";   /* Memory #1 size    (1.3.6.1.4.1.5678.2.32.2.1.2.1) */
                                                            /* End of entry */
const VB snmp_mib_5678_2_39[]           = "1.39.0";         /* Gauge 2           (1.3.6.1.4.1.5678.2.39) */

/* MIB 0 */
const T_SNMP_MIB snmp_mib_ven_obj_0[] = {
    /* OID                 Length      Type          Access  */
    {snmp_mib_1234_1_1,    DESCR_LEN,  TYP_OCT_STR,  STS_RO},    /* Descr      (1.3.6.1.4.1.1234.1.1) */
    {snmp_mib_1234_1_2,    DESCR_LEN,  TYP_OCT_STR,  STS_RO},    /* Version    (1.3.6.1.4.1.1234.1.2) */
    {snmp_mib_1234_1_3,    USER_LEN,   TYP_OCT_STR,  STS_RW},    /* User name  (1.3.6.1.4.1.1234.1.3) */
    {snmp_mib_1234_1_4,    LEN_INT,    TYP_TIM_TIC,  STS_RW},    /* Time ticks (1.3.6.1.4.1.1234.1.4) */
    {snmp_mib_1234_1_5,    LEN_INT,    TYP_INT,      STS_RW},    /* Status     (1.3.6.1.4.1.1234.1.5) */
    {snmp_mib_1234_1_6,    LEN_INT,    TYP_IP_ADR,   STS_RW},    /* IP address (1.3.6.1.4.1.1234.1.6) */
    {snmp_mib_1234_1_7,    LEN_INT,    TYP_CNT,      STS_RO},    /* Counter    (1.3.6.1.4.1.1234.1.7) */
    {snmp_mib_1234_1_8,    LEN_INT,    TYP_GAUGE,    STS_RO},    /* Gauge      (1.3.6.1.4.1.1234.1.8) */
    {snmp_mib_1234_1_9,    OIDSTR_LEN, TYP_OBJ_ID,   STS_RW},    /* Identifier (1.3.6.1.4.1.1234.1.9) */
    {0, 0, 0, 0}    /* Terminator */
};

/* MIB 1 */
const T_SNMP_MIB snmp_mib_ven_obj_1[] = {
    /* OID                         Length     Type          Access  */
    {snmp_mib_5678_1_1,            DESCR_LEN, TYP_OCT_STR,  STS_RO},    /* Descr              (1.3.6.1.4.1.5678.1.1) */
    {snmp_mib_5678_1_2,            LEN_INT,   TYP_INT,      STS_RO},    /* Version            (1.3.6.1.4.1.5678.1.2) */
    {snmp_mib_5678_1_3,            LEN_INT,   TYP_INT,      STS_RW},    /* Status             (1.3.6.1.4.1.5678.1.3) */
    {snmp_mib_5678_1_8,            LEN_INT,   TYP_NONE,     STS_NO},    /* Disk               (1.3.6.1.4.1.5678.1.8) */
    {snmp_mib_5678_1_8_1,          LEN_INT,   TYP_INT,      STS_RO},    /* Disk number        (1.3.6.1.4.1.5678.1.8.1) */
    {snmp_mib_5678_1_8_2,          LEN_INT,   TYP_SEQ,      STS_NO},    /* Disk table         (1.3.6.1.4.1.5678.1.8.2) */
    {snmp_mib_5678_1_8_2_1,        LEN_INT,   TYP_NONE,     STS_NO},    /* Disk entry         (1.3.6.1.4.1.5678.1.8.2.1) */
    {snmp_mib_5678_1_8_2_1_1_1,    LEN_INT,   TYP_INT,      STS_RO},    /* Disk #1   ifIndex  (1.3.6.1.4.1.5678.1.8.2.1.1) */
    {snmp_mib_5678_1_8_2_1_1_2,    LEN_INT,   TYP_INT,      STS_RO},    /* Disk #2   ifIndex  (1.3.6.1.4.1.5678.1.8.2.1.2) */
    {snmp_mib_5678_1_8_2_1_1_200,  LEN_INT,   TYP_INT,      STS_RO},    /* Disk #200 ifIndex  (1.3.6.1.4.1.5678.1.8.2.1.200) */
    {snmp_mib_5678_1_8_2_1_1_551,  LEN_INT,   TYP_INT,      STS_RO},    /* Disk #551 ifIndex  (1.3.6.1.4.1.5678.1.8.2.1.551) */
    {snmp_mib_5678_1_8_2_1_2_1,    DESCR_LEN, TYP_OCT_STR,  STS_RO},    /* Disk #1   descr    (1.3.6.1.4.1.5678.1.8.2.2.1) */
    {snmp_mib_5678_1_8_2_1_2_2,    DESCR_LEN, TYP_OCT_STR,  STS_RO},    /* Disk #2   descr    (1.3.6.1.4.1.5678.1.8.2.2.2) */
    {snmp_mib_5678_1_8_2_1_2_200,  DESCR_LEN, TYP_OCT_STR,  STS_RO},    /* Disk #200 descr    (1.3.6.1.4.1.5678.1.8.2.2.200) */
    {snmp_mib_5678_1_8_2_1_2_551,  DESCR_LEN, TYP_OCT_STR,  STS_RO},    /* Disk #551 descr    (1.3.6.1.4.1.5678.1.8.2.2.551) */
    {snmp_mib_5678_1_8_2_1_17_1,   LEN_INT,   TYP_GAUGE,    STS_RO},    /* Disk #1   size     (1.3.6.1.4.1.5678.1.8.2.17.1) */
    {snmp_mib_5678_1_8_2_1_17_2,   LEN_INT,   TYP_GAUGE,    STS_RO},    /* Disk #2   size     (1.3.6.1.4.1.5678.1.8.2.17.2) */
    {snmp_mib_5678_1_8_2_1_17_200, LEN_INT,   TYP_GAUGE,    STS_RO},    /* Disk #200 size     (1.3.6.1.4.1.5678.1.8.2.17.200) */
    {snmp_mib_5678_1_8_2_1_17_551, LEN_INT,   TYP_GAUGE,    STS_RO},    /* Disk #551 size     (1.3.6.1.4.1.5678.1.8.2.17.551) */
    {snmp_mib_5678_1_8_2_1_18_1,   LEN_INT,   TYP_GAUGE,    STS_RO},    /* Disk #1   free     (1.3.6.1.4.1.5678.1.8.2.18.1) */
    {snmp_mib_5678_1_8_2_1_18_2,   LEN_INT,   TYP_GAUGE,    STS_RO},    /* Disk #2   free     (1.3.6.1.4.1.5678.1.8.2.18.2) */
    {snmp_mib_5678_1_8_2_1_18_200, LEN_INT,   TYP_GAUGE,    STS_RO},    /* Disk #200 free     (1.3.6.1.4.1.5678.1.8.2.18.200) */
    {snmp_mib_5678_1_8_2_1_18_551, LEN_INT,   TYP_GAUGE,    STS_RO},    /* Disk #551 free     (1.3.6.1.4.1.5678.1.8.2.18.551) */
    {snmp_mib_5678_1_9,            LEN_INT,   TYP_GAUGE,    STS_RW},    /* Gauge 1            (1.3.6.1.4.1.5678.1.9) */
    {snmp_mib_5678_2_32,           LEN_INT,   TYP_NONE,     STS_NO},    /* Memory             (1.3.6.1.4.1.5678.2.32) */
    {snmp_mib_5678_2_32_1,         LEN_INT,   TYP_INT,      STS_RO},    /* Memory number      (1.3.6.1.4.1.5678.2.32.1) */
    {snmp_mib_5678_2_32_2,         LEN_INT,   TYP_SEQ,      STS_NO},    /* Memory table       (1.3.6.1.4.1.5678.2.32.2) */
    {snmp_mib_5678_2_32_2_1,       LEN_INT,   TYP_NONE,     STS_NO},    /* Memory entry       (1.3.6.1.4.1.5678.2.32.2.1) */
    {snmp_mib_5678_2_32_2_1_1_0,   LEN_INT,   TYP_INT,      STS_RO},    /* Memory #0  ifIndex (1.3.6.1.4.1.5678.2.32.2.1.1.0) */
    {snmp_mib_5678_2_32_2_1_1_1,   LEN_INT,   TYP_INT,      STS_RO},    /* Memory #1  ifIndex (1.3.6.1.4.1.5678.2.32.2.1.1.1) */
    {snmp_mib_5678_2_32_2_1_2_0,   LEN_INT,   TYP_INT,      STS_RO},    /* Memory #0  size    (1.3.6.1.4.1.5678.2.32.2.1.2.0) */
    {snmp_mib_5678_2_32_2_1_2_1,   LEN_INT,   TYP_INT,      STS_RO},    /* Memory #1  size    (1.3.6.1.4.1.5678.2.32.2.1.2.1) */
    {snmp_mib_5678_2_39,           LEN_INT,   TYP_GAUGE,    STS_RW},    /* Gauge 2            (1.3.6.1.4.1.5678.2.39) */
    {0, 0, 0, 0}    /* Terminator */
};

/* MIB 0 data */
T_SNMP_MIB_DAT snmp_mib_ven_dat_0[] = {
    (VP)snmp_mib_ven_descr,         /* Descr                      (1.3.6.1.4.1.1234.1.1) */
    (VP)snmp_mib_ven_ver,           /* Version                    (1.3.6.1.4.1.1234.1.2) */
    (VP)snmp_mib_ven_user_name,     /* User name                  (1.3.6.1.4.1.1234.1.3) */
    (VP)2192481,                    /* Time ticks (6:05:24.81)    (1.3.6.1.4.1.1234.1.4) */
    (VP)1,                          /* Status                     (1.3.6.1.4.1.1234.1.5) */
    (VP)0xc0a80167,                 /* IP address (192.168.1.103) (1.3.6.1.4.1.1234.1.6) */
    (VP)0,                          /* Counter [0..4294967295]    (1.3.6.1.4.1.1234.1.7) */
    (VP)10,                         /* Gauge   [0..4294967295]    (1.3.6.1.4.1.1234.1.8) */
    (VP)snmp_mib_ven_obj_id,        /* Identifier                 (1.3.6.1.4.1.1234.1.9) */
};

/* MIB 1 data */
T_SNMP_MIB_DAT snmp_mib_ven_dat_1[] = {
    (VP)snmp_mib_ven_descr,         /* Descr                         (1.3.6.1.4.1.5678.1.1) */
    (VP)100,                        /* Version                       (1.3.6.1.4.1.5678.1.2) */
    (VP)-1,                         /* Status                        (1.3.6.1.4.1.5678.1.3) */
    (VP)0,                          /* Disk (Not accessible)         (1.3.6.1.4.1.5678.1.8) */
    (VP)4,                          /* Disk number                   (1.3.6.1.4.1.5678.1.8.1) */
    (VP)0,                          /* Disk table (Not accessible)   (1.3.6.1.4.1.5678.1.8.2) */
    (VP)0,                          /* Disk entry (Not accessible)   (1.3.6.1.4.1.5678.1.8.2.1) */
    (VP)1,                          /* Disk #1   index               (1.3.6.1.4.1.5678.1.8.2.1.1.1) */
    (VP)2,                          /* Disk #2   index               (1.3.6.1.4.1.5678.1.8.2.1.1.2) */
    (VP)200,                        /* Disk #200 index               (1.3.6.1.4.1.5678.1.8.2.1.1.200) */
    (VP)551,                        /* Disk #551 index               (1.3.6.1.4.1.5678.1.8.2.1.1.551) */
    (VP)snmp_mib_ven_disk_name_1,   /* Disk #1   descr               (1.3.6.1.4.1.5678.1.8.2.1.2.1) */
    (VP)snmp_mib_ven_disk_name_2,   /* Disk #2   descr               (1.3.6.1.4.1.5678.1.8.2.1.2.2) */
    (VP)snmp_mib_ven_disk_name_200, /* Disk #200 descr               (1.3.6.1.4.1.5678.1.8.2.1.2.200) */
    (VP)snmp_mib_ven_disk_name_551, /* Disk #551 descr               (1.3.6.1.4.1.5678.1.8.2.1.2.551) */
    (VP)512,                        /* Disk #1   size                (1.3.6.1.4.1.5678.1.8.2.1.17.1) */
    (VP)1024,                       /* Disk #2   size                (1.3.6.1.4.1.5678.1.8.2.1.17.2) */
    (VP)0x0000ffff,                 /* Disk #200 size                (1.3.6.1.4.1.5678.1.8.2.1.17.200) */
    (VP)0xffffffff,                 /* Disk #551 size                (1.3.6.1.4.1.5678.1.8.2.1.17.551) */
    (VP)128,                        /* Disk #0   free                (1.3.6.1.4.1.5678.1.8.2.1.18.1) */
    (VP)256,                        /* Disk #1   free                (1.3.6.1.4.1.5678.1.8.2.1.18.2) */
    (VP)0x00ff,                     /* Disk #200 free                (1.3.6.1.4.1.5678.1.8.2.1.18.200) */
    (VP)0xffff,                     /* Disk #551 free                (1.3.6.1.4.1.5678.1.8.2.1.18.551) */
    (VP)2048,                       /* Gauge 1                       (1.3.6.1.4.1.5678.1.9) */
    (VP)0,                          /* Memory (Not accessible)       (1.3.6.1.4.1.5678.2.32) */
    (VP)2,                          /* Memory number                 (1.3.6.1.4.1.5678.2.32.1) */
    (VP)0,                          /* Memory table (Not accessible) (1.3.6.1.4.1.5678.2.32.2) */
    (VP)0,                          /* Memory entry (Not accessible) (1.3.6.1.4.1.5678.2.32.2.1) */
    (VP)0,                          /* Memory #0  index              (1.3.6.1.4.1.5678.2.32.2.1.1.0) */
    (VP)1,                          /* Memory #1  index              (1.3.6.1.4.1.5678.2.32.2.1.1.1) */
    (VP)32,                         /* Memory #0  size               (1.3.6.1.4.1.5678.2.32.2.1.2.0) */
    (VP)64,                         /* Memory #1  size               (1.3.6.1.4.1.5678.2.32.2.1.2.1) */
    (VP)128,                        /* Gauge 2                       (1.3.6.1.4.1.5678.2.39) */
};

/* MIB 1 callback functions */
extern ER apl_snmp_cbk_0(T_SNMP_CFG_CBK_DAT*);
extern ER apl_snmp_cbk_1(T_SNMP_CFG_CBK_DAT*);
extern ER apl_snmp_cbk_2(T_SNMP_CFG_CBK_DAT*);

const T_SNMP_CFG_CBK snmp_mib_ven_cbk_1[] = {
    {CBK_NONE},         /* Descr                         (1.3.6.1.4.1.5678.1.1) */
    {CBK_NONE},         /* Version                       (1.3.6.1.4.1.5678.1.2) */
    {CBK_NONE},         /* Status                        (1.3.6.1.4.1.5678.1.3) */
    {CBK_NONE},         /* Disk (Not accessible)         (1.3.6.1.4.1.5678.1.8) */
    {CBK_NONE},         /* Disk number                   (1.3.6.1.4.1.5678.1.8.1) */
    {CBK_NONE},         /* Disk table (Not accessible)   (1.3.6.1.4.1.5678.1.8.2) */
    {CBK_NONE},         /* Disk entry (Not accessible)   (1.3.6.1.4.1.5678.1.8.2.1) */
    {CBK_NONE},         /* Disk #1   index               (1.3.6.1.4.1.5678.1.8.2.1.1.1) */
    {CBK_NONE},         /* Disk #2   index               (1.3.6.1.4.1.5678.1.8.2.1.1.2) */
    {CBK_NONE},         /* Disk #200 index               (1.3.6.1.4.1.5678.1.8.2.1.1.200) */
    {CBK_NONE},         /* Disk #551 index               (1.3.6.1.4.1.5678.1.8.2.1.1.551) */
    {CBK_NONE},         /* Disk #1   descr               (1.3.6.1.4.1.5678.1.8.2.1.2.1) */
    {CBK_NONE},         /* Disk #2   descr               (1.3.6.1.4.1.5678.1.8.2.1.2.2) */
    {CBK_NONE},         /* Disk #200 descr               (1.3.6.1.4.1.5678.1.8.2.1.2.200) */
    {CBK_NONE},         /* Disk #551 descr               (1.3.6.1.4.1.5678.1.8.2.1.2.551) */
    {CBK_NONE},         /* Disk #1   size                (1.3.6.1.4.1.5678.1.8.2.1.17.1) */
    {CBK_NONE},         /* Disk #2   size                (1.3.6.1.4.1.5678.1.8.2.1.17.2) */
    {CBK_NONE},         /* Disk #200 size                (1.3.6.1.4.1.5678.1.8.2.1.17.200) */
    {CBK_NONE},         /* Disk #551 size                (1.3.6.1.4.1.5678.1.8.2.1.17.551) */
    {apl_snmp_cbk_1},   /* Disk #1   free                (1.3.6.1.4.1.5678.1.8.2.1.18.1) */
    {apl_snmp_cbk_1},   /* Disk #2   free                (1.3.6.1.4.1.5678.1.8.2.1.18.2) */
    {apl_snmp_cbk_1},   /* Disk #200 free                (1.3.6.1.4.1.5678.1.8.2.1.18.200) */
    {apl_snmp_cbk_1},   /* Disk #551 free                (1.3.6.1.4.1.5678.1.8.2.1.18.551) */
    {apl_snmp_cbk_0},   /* Gauge 1                       (1.3.6.1.4.1.5678.1.9) */
    {CBK_NONE},         /* Memory (Not accessible)       (1.3.6.1.4.1.5678.1.2.32) */
    {CBK_NONE},         /* Memory number                 (1.3.6.1.4.1.5678.1.2.32.1) */
    {CBK_NONE},         /* Memory table (Not accessible) (1.3.6.1.4.1.5678.1.2.32.2) */
    {CBK_NONE},         /* Memory entry (Not accessible) (1.3.6.1.4.1.5678.1.2.32.2.1) */
    {CBK_NONE},         /* Memory #0 index               (1.3.6.1.4.1.5678.2.32.2.1.1.0) */
    {CBK_NONE},         /* Memory #1 index               (1.3.6.1.4.1.5678.2.32.2.1.1.1) */
    {apl_snmp_cbk_2},   /* Memory #0 size                (1.3.6.1.4.1.5678.2.32.2.1.2.0) */
    {apl_snmp_cbk_2},   /* Memory #1 size                (1.3.6.1.4.1.5678.2.32.2.1.2.1) */
    {apl_snmp_cbk_0},   /* Gauge 2                       (1.3.6.1.4.1.5678.2.39) */
};

/* Vendor MIB table */
T_SNMP_MIB_TBL snmp_mib_ven[] = {
    /* Prefix            OID                 Data                Callback            Reserve */
    {snmp_mib_ven_pre_0, snmp_mib_ven_obj_0, snmp_mib_ven_dat_0, 0x00,               0},  /* MIB 0 */
    {snmp_mib_ven_pre_1, snmp_mib_ven_obj_1, snmp_mib_ven_dat_1, snmp_mib_ven_cbk_1, 0},  /* MIB 1 */
    {0, 0, 0, 0, 0}    /* Terminator */
};

T_SNMP_MIB_TBL snmp_mib_ven_val[] = {  /* 追加変更するベンダーMIB無し */
    {0, 0, 0, 0, 0}
};

/* MIB data Initialized */
/* ベンダー固有のMIBを動的に初期化(main.c) (必要な場合のみ使用) */
void snmp_mib_ven_ini(INT sts)
{
    /* MIB 0 data */
    snmp_mib_ven_dat_0[4] = (VP)((ADDR)sts);    /* Status */

    return;
}

