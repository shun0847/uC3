/*
    SNMP
    User configuration 
    Copyright (c) 2023, eForce Co., Ltd. All rights reserved.
    
    2016-01-06 Created
    2023-05-31 Changed character encoding to UTF8.
*/

#ifndef SNMP_CFG_H
#define SNMP_CFG_H

#include "net_cfg.h"

/* Configurable definition */

/* ネットワークデバイス（LANポート）の個数 (1?16) */
#define CFG_SNMP_NET_DEV_CNT    (CFG_NET_DEV_MAX)    /* Number of network devices (1..16) */
/* SNMPで使用するネットワークデバイスの個数 (1?16) */
#define CFG_SNMP_NET_USE_CNT    1           /* Number of network devices for SNMP (1..16) */
/* ネットワークのソケットとTCPのソケットの最大個数 (net_cfg.hと同じ値) */
#define CFG_SNMP_MAX_SOC_CNT    (CFG_NET_SOC_MAX)
#define CFG_SNMP_MAX_TCP_CNT    (CFG_NET_TCP_MAX)
#define CFG_SNMP_MAX_ARP_CNT    (CFG_NET_ARP_MAX)

/* 標準トラップとインフォームの応答用資源の最大個数 (0の場合は標準トラップ/インフォームを使用しない) */
#define CFG_SNMP_MAX_TRP_CNT    32          /* Number of traps at any time (0 or 1..) */
/* snd_trpで同時に送信するトラップ/インフォームの最大個数 (0の場合はsnd_trpを使用しない) */
#define CFG_SNMP_VEN_TRP_CNT    8           /* Number of vendor traps at any time (0 or 1..32) */
/* SNMPのパケットに追加するvariable bindingsデータの最大個数 */
#define CFG_SNMP_MSG_VAR_CNT    32          /* Maximum number of variable bindings (4 or greater) */
/* MIBのツリーのノードの最大個数 */
#define CFG_SNMP_MIB_NOD_CNT    1900        /* Number of nodes in the MIB tree */
/* MIBのツリーのノードの最大の深さ（オブジェクトIDの数字の最大個数) */
#define CFG_SNMP_MAX_MIB_DEP    32          /* Maximum depth of the MIB tree */
/* MIBのオブジェクトのデータの最大サイズ
   snmp_mib_cfg.cのDESCR_LENなどで設定したOctet String(文字列)のデータの
   最大長（末端のNULL文字を含む） */
#define CFG_SNMP_MIB_DAT_LEN    (64 + 1)    /* Maximum size of the MIB data */

/* OID型MIBオブジェクトの最大階層(ドット数) */
#define CFG_SNMP_MAX_OID_DEP    10

/* Generic trap enabled */
/* 送信する一般トラップの登録 */
/* TRP_ALL_BITは全てのトラップ (ただし、リンクダウン時はトラップを送信しない） */
#define CFG_SNMP_GEN_TRP_ENA    TRP_ALL_BIT

/* MIB2 group selector */
/* MIB2のグループの有効(1), 無効(0) */
#define CFG_SNMP_MIB2_IF_ENA      1    /* Interfaces    (1.3.6.1.2.1.2) */
#define CFG_SNMP_MIB2_AT_ENA      1    /* Address trans (1.3.6.1.2.1.3) */
#define CFG_SNMP_MIB2_IP_ENA      1    /* IP            (1.3.6.1.2.1.4) */
#define CFG_SNMP_MIB2_ICMP_ENA    1    /* ICMP          (1.3.6.1.2.1.5) */
#define CFG_SNMP_MIB2_TCP_ENA     1    /* TCP           (1.3.6.1.2.1.6) */
#define CFG_SNMP_MIB2_UDP_ENA     1    /* UDP           (1.3.6.1.2.1.7) */
#define CFG_SNMP_MIB2_SNMP_ENA    1    /* SNMP          (1.3.6.1.2.1.11) */

/* Task priority */
/* SNMPのタスクの優先度 (Standard版OSの場合のみ) */
/* Compact版OSの場合、タスクの優先度はコンフィギュレータで設定 */
#define TSK_RCV_PRI     6       /* Receive task  */
#define TSK_TIM_PRI     6       /* Timer task */
#define TSK_TRP_PRI     6       /* Trap task */

/* Task stack size */
/* コールバック関数の内部処理でスタックが不足する場合はReceive taskの
   スタックサイズ(TSK_RCV_STK)を増やしてください */
/* TimerとTrapのタスクのスタックサイズは変更する必要はありません */
#if (_kernel_SIZE_SIZE==8)
#define TSK_RCV_STK     (1536*3)	/* Receive task (byte) */
#define TSK_TIM_STK     (512*3)     /* Timer task (byte) */
#define TSK_TRP_STK     (1024*3)    /* Trap task (byte) */
#else
#define TSK_RCV_STK     1536    /* Receive task (byte) */
#define TSK_TIM_STK     512     /* Timer task (byte) */
#define TSK_TRP_STK     1024    /* Trap task (byte) */
#endif

/* Maximum size of an SNMP message  (4-byte aligned) */
/* 受信,送信可能なSNMPメッセージの最大サイズ */
#define CFG_SNMP_RCV_MSG_LEN    2048                    /* Message can receive */
#define CFG_SNMP_SND_MSG_LEN    CFG_SNMP_RCV_MSG_LEN    /* Message can send*/

/* Do not change the following items */
/* 以下の項目は変更不要 */
#define CFG_SNMP_FNC_FLG        SNMP_AGT    /* SNMP agent */
#define CFG_SNMP_UDP_SND_TMO    2000        /* UDP socket send timeout */
#define CFG_SNMP_UDP_RCV_TMO    2000        /* UDP socket receive timeout */
#define CFG_SNMP_SEM_TMO        1000        /* Semaphore timeput period for internal */

#endif

