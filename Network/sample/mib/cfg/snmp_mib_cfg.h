/*
    SNMP
    MIB definition 
    Copyright (c) 2023, eForce Co., Ltd. All rights reserved.
    
    2016-01-06 Created
    2023-05-31 Changed character encoding to UTF8.
*/

#ifndef SNMP_MIB_CFG_H
#define SNMP_MIB_CFG_H

/* MIB strings */

/* System sysDescr (1.3.6.1.2.1.1.1) */
/* 機器のハードウェア、ソフトウェアの名前やバージョン */
#define CFG_SNMP_MIB_SYS_DESCR_LEN    (32 + 1)    /* 最大長 (末端のNULL文字を含む) */
#define CFG_SNMP_MIB_SYS_DESCR        "HW:Ver.1.0.0 SW:Ver.1.0.0"

/* System sysObjectID (1.3.6.1.2.1.1.2) */
/* ベンダーのオブジェクトID */
/* MIBのSystemのsysObjectIDとトラップ(v1)のenterpriseフィールド */
#define CFG_SNMP_MIB_SYS_OBJECTID_LEN    (32 + 1)
#define CFG_SNMP_MIB_SYS_OBJECTID        "1.3.6.1.4.1.1234"

/* System sysContact (1.3.6.1.2.1.1.4) */
/* 機器の管理者の連絡先(メールアドレス) */
#define CFG_SNMP_MIB_SYS_CONTACT_LEN    (32 + 1)
#define CFG_SNMP_MIB_SYS_CONTACT        "Email address"

/* System sysName (1.3.6.1.2.1.1.5) */
/* 機器のドメインネーム */
#define CFG_SNMP_MIB_SYS_NAME_LEN    (32 + 1)
#define CFG_SNMP_MIB_SYS_NAME        "Evalution board"

/* System sysLocation (1.3.6.1.2.1.1.6) */
/* 機器の物理的な位置 */
#define CFG_SNMP_MIB_SYS_LOCATION_LEN    (32 + 1)
#define CFG_SNMP_MIB_SYS_LOCATION        "First floor"

/* System sysServices (1.3.6.1.2.1.1.7) */
/* 機器が提供するサービスの値 */
#define CFG_SNMP_MIB_SYS_SERVICES   64    /* アプリケーション層 */

#endif

