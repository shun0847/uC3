/*
    SNMP
    MIB definition 
    Copyright (c) 2014-2016, eForce Co., Ltd. All rights reserved.
    
    2014-03-06 Created
    2016-01-06 Multiple network devices were supported
*/

#include "kernel.h"
#include "snmp.h"
#include "snmp_ber.h"
#include "snmp_def.h"
#include "snmp_lib.h"
#include "snmp_mib.h"
#include "snmp_cfg.h"
#include "snmp_mib_cfg.h"

/* Configuration */
#define MIB_SYS_SERVICES    CFG_SNMP_MIB_SYS_SERVICES

/* Length */
#define LEN_INT    4    /* Length of INT,CNT,GAUGE,IP_ADR */
#define LEN_MAC    7    /* Length of MAC address with null */

/* MIB strings */

/* system sysDescr (1.3.6.1.2.1.1.1) */
static const VB snmp_mib_sys_desc[CFG_SNMP_MIB_SYS_DESCR_LEN] = {
    CFG_SNMP_MIB_SYS_DESCR
};

/* system sysObjectID (1.3.6.1.2.1.1.2) */
const VB snmp_mib_sys_obj_id[CFG_SNMP_MIB_SYS_OBJECTID_LEN] = {
    CFG_SNMP_MIB_SYS_OBJECTID
};

/* system sysContact (1.3.6.1.2.1.1.4) */
static VB snmp_mib_sys_contact[CFG_SNMP_MIB_SYS_CONTACT_LEN] = {
    CFG_SNMP_MIB_SYS_CONTACT
};

/* system sysName (1.3.6.1.2.1.1.5) */
static VB snmp_mib_sys_name[CFG_SNMP_MIB_SYS_NAME_LEN] = {
    CFG_SNMP_MIB_SYS_NAME
};

/* system sysLocation (1.3.6.1.2.1.1.6) */
static VB snmp_mib_sys_location[CFG_SNMP_MIB_SYS_LOCATION_LEN] = {
    CFG_SNMP_MIB_SYS_LOCATION
};

/* Interface ifSpecific (1.3.6.1.2.1.2.2.1.22) */
#if (CFG_SNMP_MIB2_IF_ENA == 1)
const VB snmp_mib_if_spec[] = "0.0";
#endif

/* Number of MIB objects */
#if (CFG_SNMP_MIB2_AT_ENA == 1)
#define SNMP_MIB2_OBJ_CNT_AT    (SNMP_MIB2_DEF_CNT_AT)
#else
#define SNMP_MIB2_OBJ_CNT_AT    0
#endif

#if (CFG_SNMP_MIB2_IP_ENA == 1)
#define SNMP_MIB2_OBJ_CNT_IP    (SNMP_MIB2_DEF_CNT_IP)
#else
#define SNMP_MIB2_OBJ_CNT_IP    0
#endif

#if (CFG_SNMP_MIB2_ICMP_ENA == 1)
#define SNMP_MIB2_OBJ_CNT_ICMP    (SNMP_MIB2_DEF_CNT_ICMP)
#else
#define SNMP_MIB2_OBJ_CNT_ICMP    0
#endif

#if (CFG_SNMP_MIB2_TCP_ENA == 1)
#define SNMP_MIB2_OBJ_CNT_TCP    (SNMP_MIB2_DEF_CNT_TCP)
#else
#define SNMP_MIB2_OBJ_CNT_TCP    0
#endif

#if (CFG_SNMP_MIB2_UDP_ENA == 1)
#define SNMP_MIB2_OBJ_CNT_UDP    (SNMP_MIB2_DEF_CNT_UDP)
#else
#define SNMP_MIB2_OBJ_CNT_UDP    0
#endif

#if (CFG_SNMP_MIB2_SNMP_ENA == 1)
#define SNMP_MIB2_OBJ_CNT_SNMP    (SNMP_MIB2_DEF_CNT_SNMP)
#else
#define SNMP_MIB2_OBJ_CNT_SNMP    0
#endif

#if (CFG_SNMP_MIB2_IF_ENA == 1)
#define SNMP_MIB2_OBJ_CNT_ITF    (SNMP_MIB2_DEF_CNT_ITF_IDX \
                                 + (SNMP_MIB2_DEF_CNT_ITF_TBL * CFG_SNMP_NET_DEV_CNT))
#else
#define SNMP_MIB2_OBJ_CNT_ITF    0
#endif

#define SNMP_MIB2_OBJ_CNT    (SNMP_MIB2_DEF_CNT_SYS + SNMP_MIB2_OBJ_CNT_AT \
                             + SNMP_MIB2_OBJ_CNT_IP + SNMP_MIB2_OBJ_CNT_ICMP \
                             + SNMP_MIB2_OBJ_CNT_TCP + SNMP_MIB2_OBJ_CNT_UDP \
                             + SNMP_MIB2_OBJ_CNT_SNMP + SNMP_MIB2_OBJ_CNT_ITF)

/* MIB OID */
const VB snmp_mib_mib2_pre[] = "1.3.6.1.2.1.";    /* Prefix OID */

/* System group */
static const VB snmp_mib_1_1[] = "1.1.0";    /* sysDescr */
static const VB snmp_mib_1_2[] = "1.2.0";    /* sysObjectID */
static const VB snmp_mib_1_3[] = "1.3.0";    /* sysUpTime */
static const VB snmp_mib_1_4[] = "1.4.0";    /* sysContact */
static const VB snmp_mib_1_5[] = "1.5.0";    /* sysName */
static const VB snmp_mib_1_6[] = "1.6.0";    /* sysLocation */
static const VB snmp_mib_1_7[] = "1.7.0";    /* sysServices */

/* Interfaces group */
#if (CFG_SNMP_MIB2_IF_ENA == 1)
static const VB snmp_mib_2_1[] = "2.1.0";                /* ifNumber */
static const VB snmp_mib_2_2[] = "2.2.0";                /* ifTable */
static const VB snmp_mib_2_2_1[] = "2.2.1";              /* ifEntry */
static const VB snmp_mib_2_2_1_1_1[] = "2.2.1.1.1";      /* ifIndex */
static const VB snmp_mib_2_2_1_2_1[] = "2.2.1.2.1";      /* ifDescr */
static const VB snmp_mib_2_2_1_3_1[] = "2.2.1.3.1";      /* ifType */
static const VB snmp_mib_2_2_1_4_1[] = "2.2.1.4.1";      /* ifMtu */
static const VB snmp_mib_2_2_1_5_1[] = "2.2.1.5.1";      /* ifSpeed */
static const VB snmp_mib_2_2_1_6_1[] = "2.2.1.6.1";      /* ifPhysAddress */
static const VB snmp_mib_2_2_1_7_1[] = "2.2.1.7.1";      /* ifAdminStatus */
static const VB snmp_mib_2_2_1_8_1[] = "2.2.1.8.1";      /* ifOperStatus */
static const VB snmp_mib_2_2_1_9_1[] = "2.2.1.9.1";      /* ifLastChanges */
static const VB snmp_mib_2_2_1_10_1[] = "2.2.1.10.1";    /* ifInOctets */
static const VB snmp_mib_2_2_1_11_1[] = "2.2.1.11.1";    /* ifInUcastPkts */
static const VB snmp_mib_2_2_1_12_1[] = "2.2.1.12.1";    /* ifInNUcastPkts */
static const VB snmp_mib_2_2_1_13_1[] = "2.2.1.13.1";    /* ifInDiscards */
static const VB snmp_mib_2_2_1_14_1[] = "2.2.1.14.1";    /* ifInErrors */
static const VB snmp_mib_2_2_1_15_1[] = "2.2.1.15.1";    /* ifInUnknownProtos */
static const VB snmp_mib_2_2_1_16_1[] = "2.2.1.16.1";    /* ifOutOctets */
static const VB snmp_mib_2_2_1_17_1[] = "2.2.1.17.1";    /* ifOutUcastPkts */
static const VB snmp_mib_2_2_1_18_1[] = "2.2.1.18.1";    /* ifOutNUcastPkts */
static const VB snmp_mib_2_2_1_19_1[] = "2.2.1.19.1";    /* ifOutDiscards */
static const VB snmp_mib_2_2_1_20_1[] = "2.2.1.20.1";    /* ifOutErrors */
static const VB snmp_mib_2_2_1_21_1[] = "2.2.1.21.1";    /* ifOutQLen */
static const VB snmp_mib_2_2_1_22_1[] = "2.2.1.22.1";    /* ifSpecific */
#if (CFG_SNMP_NET_DEV_CNT >= 2)
static const VB snmp_mib_2_2_1_1_2[] = "2.2.1.1.2";      /* ifIndex */
static const VB snmp_mib_2_2_1_2_2[] = "2.2.1.2.2";      /* ifDescr */
static const VB snmp_mib_2_2_1_3_2[] = "2.2.1.3.2";      /* ifType */
static const VB snmp_mib_2_2_1_4_2[] = "2.2.1.4.2";      /* ifMtu */
static const VB snmp_mib_2_2_1_5_2[] = "2.2.1.5.2";      /* ifSpeed */
static const VB snmp_mib_2_2_1_6_2[] = "2.2.1.6.2";      /* ifPhysAddress */
static const VB snmp_mib_2_2_1_7_2[] = "2.2.1.7.2";      /* ifAdminStatus */
static const VB snmp_mib_2_2_1_8_2[] = "2.2.1.8.2";      /* ifOperStatus */
static const VB snmp_mib_2_2_1_9_2[] = "2.2.1.9.2";      /* ifLastChanges */
static const VB snmp_mib_2_2_1_10_2[] = "2.2.1.10.2";    /* ifInOctets */
static const VB snmp_mib_2_2_1_11_2[] = "2.2.1.11.2";    /* ifInUcastPkts */
static const VB snmp_mib_2_2_1_12_2[] = "2.2.1.12.2";    /* ifInNUcastPkts */
static const VB snmp_mib_2_2_1_13_2[] = "2.2.1.13.2";    /* ifInDiscards */
static const VB snmp_mib_2_2_1_14_2[] = "2.2.1.14.2";    /* ifInErrors */
static const VB snmp_mib_2_2_1_15_2[] = "2.2.1.15.2";    /* ifInUnknownProtos */
static const VB snmp_mib_2_2_1_16_2[] = "2.2.1.16.2";    /* ifOutOctets */
static const VB snmp_mib_2_2_1_17_2[] = "2.2.1.17.2";    /* ifOutUcastPkts */
static const VB snmp_mib_2_2_1_18_2[] = "2.2.1.18.2";    /* ifOutNUcastPkts */
static const VB snmp_mib_2_2_1_19_2[] = "2.2.1.19.2";    /* ifOutDiscards */
static const VB snmp_mib_2_2_1_20_2[] = "2.2.1.20.2";    /* ifOutErrors */
static const VB snmp_mib_2_2_1_21_2[] = "2.2.1.21.2";    /* ifOutQLen */
static const VB snmp_mib_2_2_1_22_2[] = "2.2.1.22.2";    /* ifSpecific */
#endif
#if (CFG_SNMP_NET_DEV_CNT >= 3)
static const VB snmp_mib_2_2_1_1_3[] = "2.2.1.1.3";      /* ifIndex */
static const VB snmp_mib_2_2_1_2_3[] = "2.2.1.2.3";      /* ifDescr */
static const VB snmp_mib_2_2_1_3_3[] = "2.2.1.3.3";      /* ifType */
static const VB snmp_mib_2_2_1_4_3[] = "2.2.1.4.3";      /* ifMtu */
static const VB snmp_mib_2_2_1_5_3[] = "2.2.1.5.3";      /* ifSpeed */
static const VB snmp_mib_2_2_1_6_3[] = "2.2.1.6.3";      /* ifPhysAddress */
static const VB snmp_mib_2_2_1_7_3[] = "2.2.1.7.3";      /* ifAdminStatus */
static const VB snmp_mib_2_2_1_8_3[] = "2.2.1.8.3";      /* ifOperStatus */
static const VB snmp_mib_2_2_1_9_3[] = "2.2.1.9.3";      /* ifLastChanges */
static const VB snmp_mib_2_2_1_10_3[] = "2.2.1.10.3";    /* ifInOctets */
static const VB snmp_mib_2_2_1_11_3[] = "2.2.1.11.3";    /* ifInUcastPkts */
static const VB snmp_mib_2_2_1_12_3[] = "2.2.1.12.3";    /* ifInNUcastPkts */
static const VB snmp_mib_2_2_1_13_3[] = "2.2.1.13.3";    /* ifInDiscards */
static const VB snmp_mib_2_2_1_14_3[] = "2.2.1.14.3";    /* ifInErrors */
static const VB snmp_mib_2_2_1_15_3[] = "2.2.1.15.3";    /* ifInUnknownProtos */
static const VB snmp_mib_2_2_1_16_3[] = "2.2.1.16.3";    /* ifOutOctets */
static const VB snmp_mib_2_2_1_17_3[] = "2.2.1.17.3";    /* ifOutUcastPkts */
static const VB snmp_mib_2_2_1_18_3[] = "2.2.1.18.3";    /* ifOutNUcastPkts */
static const VB snmp_mib_2_2_1_19_3[] = "2.2.1.19.3";    /* ifOutDiscards */
static const VB snmp_mib_2_2_1_20_3[] = "2.2.1.20.3";    /* ifOutErrors */
static const VB snmp_mib_2_2_1_21_3[] = "2.2.1.21.3";    /* ifOutQLen */
static const VB snmp_mib_2_2_1_22_3[] = "2.2.1.22.3";    /* ifSpecific */
#endif
#if (CFG_SNMP_NET_DEV_CNT >= 4)
static const VB snmp_mib_2_2_1_1_4[] = "2.2.1.1.4";      /* ifIndex */
static const VB snmp_mib_2_2_1_2_4[] = "2.2.1.2.4";      /* ifDescr */
static const VB snmp_mib_2_2_1_3_4[] = "2.2.1.3.4";      /* ifType */
static const VB snmp_mib_2_2_1_4_4[] = "2.2.1.4.4";      /* ifMtu */
static const VB snmp_mib_2_2_1_5_4[] = "2.2.1.5.4";      /* ifSpeed */
static const VB snmp_mib_2_2_1_6_4[] = "2.2.1.6.4";      /* ifPhysAddress */
static const VB snmp_mib_2_2_1_7_4[] = "2.2.1.7.4";      /* ifAdminStatus */
static const VB snmp_mib_2_2_1_8_4[] = "2.2.1.8.4";      /* ifOperStatus */
static const VB snmp_mib_2_2_1_9_4[] = "2.2.1.9.4";      /* ifLastChanges */
static const VB snmp_mib_2_2_1_10_4[] = "2.2.1.10.4";    /* ifInOctets */
static const VB snmp_mib_2_2_1_11_4[] = "2.2.1.11.4";    /* ifInUcastPkts */
static const VB snmp_mib_2_2_1_12_4[] = "2.2.1.12.4";    /* ifInNUcastPkts */
static const VB snmp_mib_2_2_1_13_4[] = "2.2.1.13.4";    /* ifInDiscards */
static const VB snmp_mib_2_2_1_14_4[] = "2.2.1.14.4";    /* ifInErrors */
static const VB snmp_mib_2_2_1_15_4[] = "2.2.1.15.4";    /* ifInUnknownProtos */
static const VB snmp_mib_2_2_1_16_4[] = "2.2.1.16.4";    /* ifOutOctets */
static const VB snmp_mib_2_2_1_17_4[] = "2.2.1.17.4";    /* ifOutUcastPkts */
static const VB snmp_mib_2_2_1_18_4[] = "2.2.1.18.4";    /* ifOutNUcastPkts */
static const VB snmp_mib_2_2_1_19_4[] = "2.2.1.19.4";    /* ifOutDiscards */
static const VB snmp_mib_2_2_1_20_4[] = "2.2.1.20.4";    /* ifOutErrors */
static const VB snmp_mib_2_2_1_21_4[] = "2.2.1.21.4";    /* ifOutQLen */
static const VB snmp_mib_2_2_1_22_4[] = "2.2.1.22.4";    /* ifSpecific */
#endif
#if (CFG_SNMP_NET_DEV_CNT >= 5)
static const VB snmp_mib_2_2_1_1_5[] = "2.2.1.1.5";      /* ifIndex */
static const VB snmp_mib_2_2_1_2_5[] = "2.2.1.2.5";      /* ifDescr */
static const VB snmp_mib_2_2_1_3_5[] = "2.2.1.3.5";      /* ifType */
static const VB snmp_mib_2_2_1_4_5[] = "2.2.1.4.5";      /* ifMtu */
static const VB snmp_mib_2_2_1_5_5[] = "2.2.1.5.5";      /* ifSpeed */
static const VB snmp_mib_2_2_1_6_5[] = "2.2.1.6.5";      /* ifPhysAddress */
static const VB snmp_mib_2_2_1_7_5[] = "2.2.1.7.5";      /* ifAdminStatus */
static const VB snmp_mib_2_2_1_8_5[] = "2.2.1.8.5";      /* ifOperStatus */
static const VB snmp_mib_2_2_1_9_5[] = "2.2.1.9.5";      /* ifLastChanges */
static const VB snmp_mib_2_2_1_10_5[] = "2.2.1.10.5";    /* ifInOctets */
static const VB snmp_mib_2_2_1_11_5[] = "2.2.1.11.5";    /* ifInUcastPkts */
static const VB snmp_mib_2_2_1_12_5[] = "2.2.1.12.5";    /* ifInNUcastPkts */
static const VB snmp_mib_2_2_1_13_5[] = "2.2.1.13.5";    /* ifInDiscards */
static const VB snmp_mib_2_2_1_14_5[] = "2.2.1.14.5";    /* ifInErrors */
static const VB snmp_mib_2_2_1_15_5[] = "2.2.1.15.5";    /* ifInUnknownProtos */
static const VB snmp_mib_2_2_1_16_5[] = "2.2.1.16.5";    /* ifOutOctets */
static const VB snmp_mib_2_2_1_17_5[] = "2.2.1.17.5";    /* ifOutUcastPkts */
static const VB snmp_mib_2_2_1_18_5[] = "2.2.1.18.5";    /* ifOutNUcastPkts */
static const VB snmp_mib_2_2_1_19_5[] = "2.2.1.19.5";    /* ifOutDiscards */
static const VB snmp_mib_2_2_1_20_5[] = "2.2.1.20.5";    /* ifOutErrors */
static const VB snmp_mib_2_2_1_21_5[] = "2.2.1.21.5";    /* ifOutQLen */
static const VB snmp_mib_2_2_1_22_5[] = "2.2.1.22.5";    /* ifSpecific */
#endif
#if (CFG_SNMP_NET_DEV_CNT >= 6)
static const VB snmp_mib_2_2_1_1_6[] = "2.2.1.1.6";      /* ifIndex */
static const VB snmp_mib_2_2_1_2_6[] = "2.2.1.2.6";      /* ifDescr */
static const VB snmp_mib_2_2_1_3_6[] = "2.2.1.3.6";      /* ifType */
static const VB snmp_mib_2_2_1_4_6[] = "2.2.1.4.6";      /* ifMtu */
static const VB snmp_mib_2_2_1_5_6[] = "2.2.1.5.6";      /* ifSpeed */
static const VB snmp_mib_2_2_1_6_6[] = "2.2.1.6.6";      /* ifPhysAddress */
static const VB snmp_mib_2_2_1_7_6[] = "2.2.1.7.6";      /* ifAdminStatus */
static const VB snmp_mib_2_2_1_8_6[] = "2.2.1.8.6";      /* ifOperStatus */
static const VB snmp_mib_2_2_1_9_6[] = "2.2.1.9.6";      /* ifLastChanges */
static const VB snmp_mib_2_2_1_10_6[] = "2.2.1.10.6";    /* ifInOctets */
static const VB snmp_mib_2_2_1_11_6[] = "2.2.1.11.6";    /* ifInUcastPkts */
static const VB snmp_mib_2_2_1_12_6[] = "2.2.1.12.6";    /* ifInNUcastPkts */
static const VB snmp_mib_2_2_1_13_6[] = "2.2.1.13.6";    /* ifInDiscards */
static const VB snmp_mib_2_2_1_14_6[] = "2.2.1.14.6";    /* ifInErrors */
static const VB snmp_mib_2_2_1_15_6[] = "2.2.1.15.6";    /* ifInUnknownProtos */
static const VB snmp_mib_2_2_1_16_6[] = "2.2.1.16.6";    /* ifOutOctets */
static const VB snmp_mib_2_2_1_17_6[] = "2.2.1.17.6";    /* ifOutUcastPkts */
static const VB snmp_mib_2_2_1_18_6[] = "2.2.1.18.6";    /* ifOutNUcastPkts */
static const VB snmp_mib_2_2_1_19_6[] = "2.2.1.19.6";    /* ifOutDiscards */
static const VB snmp_mib_2_2_1_20_6[] = "2.2.1.20.6";    /* ifOutErrors */
static const VB snmp_mib_2_2_1_21_6[] = "2.2.1.21.6";    /* ifOutQLen */
static const VB snmp_mib_2_2_1_22_6[] = "2.2.1.22.6";    /* ifSpecific */
#endif
#if (CFG_SNMP_NET_DEV_CNT >= 7)
static const VB snmp_mib_2_2_1_1_7[] = "2.2.1.1.7";      /* ifIndex */
static const VB snmp_mib_2_2_1_2_7[] = "2.2.1.2.7";      /* ifDescr */
static const VB snmp_mib_2_2_1_3_7[] = "2.2.1.3.7";      /* ifType */
static const VB snmp_mib_2_2_1_4_7[] = "2.2.1.4.7";      /* ifMtu */
static const VB snmp_mib_2_2_1_5_7[] = "2.2.1.5.7";      /* ifSpeed */
static const VB snmp_mib_2_2_1_6_7[] = "2.2.1.6.7";      /* ifPhysAddress */
static const VB snmp_mib_2_2_1_7_7[] = "2.2.1.7.7";      /* ifAdminStatus */
static const VB snmp_mib_2_2_1_8_7[] = "2.2.1.8.7";      /* ifOperStatus */
static const VB snmp_mib_2_2_1_9_7[] = "2.2.1.9.7";      /* ifLastChanges */
static const VB snmp_mib_2_2_1_10_7[] = "2.2.1.10.7";    /* ifInOctets */
static const VB snmp_mib_2_2_1_11_7[] = "2.2.1.11.7";    /* ifInUcastPkts */
static const VB snmp_mib_2_2_1_12_7[] = "2.2.1.12.7";    /* ifInNUcastPkts */
static const VB snmp_mib_2_2_1_13_7[] = "2.2.1.13.7";    /* ifInDiscards */
static const VB snmp_mib_2_2_1_14_7[] = "2.2.1.14.7";    /* ifInErrors */
static const VB snmp_mib_2_2_1_15_7[] = "2.2.1.15.7";    /* ifInUnknownProtos */
static const VB snmp_mib_2_2_1_16_7[] = "2.2.1.16.7";    /* ifOutOctets */
static const VB snmp_mib_2_2_1_17_7[] = "2.2.1.17.7";    /* ifOutUcastPkts */
static const VB snmp_mib_2_2_1_18_7[] = "2.2.1.18.7";    /* ifOutNUcastPkts */
static const VB snmp_mib_2_2_1_19_7[] = "2.2.1.19.7";    /* ifOutDiscards */
static const VB snmp_mib_2_2_1_20_7[] = "2.2.1.20.7";    /* ifOutErrors */
static const VB snmp_mib_2_2_1_21_7[] = "2.2.1.21.7";    /* ifOutQLen */
static const VB snmp_mib_2_2_1_22_7[] = "2.2.1.22.7";    /* ifSpecific */
#endif
#if (CFG_SNMP_NET_DEV_CNT >= 8)
static const VB snmp_mib_2_2_1_1_8[] = "2.2.1.1.8";      /* ifIndex */
static const VB snmp_mib_2_2_1_2_8[] = "2.2.1.2.8";      /* ifDescr */
static const VB snmp_mib_2_2_1_3_8[] = "2.2.1.3.8";      /* ifType */
static const VB snmp_mib_2_2_1_4_8[] = "2.2.1.4.8";      /* ifMtu */
static const VB snmp_mib_2_2_1_5_8[] = "2.2.1.5.8";      /* ifSpeed */
static const VB snmp_mib_2_2_1_6_8[] = "2.2.1.6.8";      /* ifPhysAddress */
static const VB snmp_mib_2_2_1_7_8[] = "2.2.1.7.8";      /* ifAdminStatus */
static const VB snmp_mib_2_2_1_8_8[] = "2.2.1.8.8";      /* ifOperStatus */
static const VB snmp_mib_2_2_1_9_8[] = "2.2.1.9.8";      /* ifLastChanges */
static const VB snmp_mib_2_2_1_10_8[] = "2.2.1.10.8";    /* ifInOctets */
static const VB snmp_mib_2_2_1_11_8[] = "2.2.1.11.8";    /* ifInUcastPkts */
static const VB snmp_mib_2_2_1_12_8[] = "2.2.1.12.8";    /* ifInNUcastPkts */
static const VB snmp_mib_2_2_1_13_8[] = "2.2.1.13.8";    /* ifInDiscards */
static const VB snmp_mib_2_2_1_14_8[] = "2.2.1.14.8";    /* ifInErrors */
static const VB snmp_mib_2_2_1_15_8[] = "2.2.1.15.8";    /* ifInUnknownProtos */
static const VB snmp_mib_2_2_1_16_8[] = "2.2.1.16.8";    /* ifOutOctets */
static const VB snmp_mib_2_2_1_17_8[] = "2.2.1.17.8";    /* ifOutUcastPkts */
static const VB snmp_mib_2_2_1_18_8[] = "2.2.1.18.8";    /* ifOutNUcastPkts */
static const VB snmp_mib_2_2_1_19_8[] = "2.2.1.19.8";    /* ifOutDiscards */
static const VB snmp_mib_2_2_1_20_8[] = "2.2.1.20.8";    /* ifOutErrors */
static const VB snmp_mib_2_2_1_21_8[] = "2.2.1.21.8";    /* ifOutQLen */
static const VB snmp_mib_2_2_1_22_8[] = "2.2.1.22.8";    /* ifSpecific */
#endif
#if (CFG_SNMP_NET_DEV_CNT >= 9)
static const VB snmp_mib_2_2_1_1_9[] = "2.2.1.1.9";      /* ifIndex */
static const VB snmp_mib_2_2_1_2_9[] = "2.2.1.2.9";      /* ifDescr */
static const VB snmp_mib_2_2_1_3_9[] = "2.2.1.3.9";      /* ifType */
static const VB snmp_mib_2_2_1_4_9[] = "2.2.1.4.9";      /* ifMtu */
static const VB snmp_mib_2_2_1_5_9[] = "2.2.1.5.9";      /* ifSpeed */
static const VB snmp_mib_2_2_1_6_9[] = "2.2.1.6.9";      /* ifPhysAddress */
static const VB snmp_mib_2_2_1_7_9[] = "2.2.1.7.9";      /* ifAdminStatus */
static const VB snmp_mib_2_2_1_8_9[] = "2.2.1.8.9";      /* ifOperStatus */
static const VB snmp_mib_2_2_1_9_9[] = "2.2.1.9.9";      /* ifLastChanges */
static const VB snmp_mib_2_2_1_10_9[] = "2.2.1.10.9";    /* ifInOctets */
static const VB snmp_mib_2_2_1_11_9[] = "2.2.1.11.9";    /* ifInUcastPkts */
static const VB snmp_mib_2_2_1_12_9[] = "2.2.1.12.9";    /* ifInNUcastPkts */
static const VB snmp_mib_2_2_1_13_9[] = "2.2.1.13.9";    /* ifInDiscards */
static const VB snmp_mib_2_2_1_14_9[] = "2.2.1.14.9";    /* ifInErrors */
static const VB snmp_mib_2_2_1_15_9[] = "2.2.1.15.9";    /* ifInUnknownProtos */
static const VB snmp_mib_2_2_1_16_9[] = "2.2.1.16.9";    /* ifOutOctets */
static const VB snmp_mib_2_2_1_17_9[] = "2.2.1.17.9";    /* ifOutUcastPkts */
static const VB snmp_mib_2_2_1_18_9[] = "2.2.1.18.9";    /* ifOutNUcastPkts */
static const VB snmp_mib_2_2_1_19_9[] = "2.2.1.19.9";    /* ifOutDiscards */
static const VB snmp_mib_2_2_1_20_9[] = "2.2.1.20.9";    /* ifOutErrors */
static const VB snmp_mib_2_2_1_21_9[] = "2.2.1.21.9";    /* ifOutQLen */
static const VB snmp_mib_2_2_1_22_9[] = "2.2.1.22.9";    /* ifSpecific */
#endif
#if (CFG_SNMP_NET_DEV_CNT >= 10)
static const VB snmp_mib_2_2_1_1_10[] = "2.2.1.1.10";      /* ifIndex */
static const VB snmp_mib_2_2_1_2_10[] = "2.2.1.2.10";      /* ifDescr */
static const VB snmp_mib_2_2_1_3_10[] = "2.2.1.3.10";      /* ifType */
static const VB snmp_mib_2_2_1_4_10[] = "2.2.1.4.10";      /* ifMtu */
static const VB snmp_mib_2_2_1_5_10[] = "2.2.1.5.10";      /* ifSpeed */
static const VB snmp_mib_2_2_1_6_10[] = "2.2.1.6.10";      /* ifPhysAddress */
static const VB snmp_mib_2_2_1_7_10[] = "2.2.1.7.10";      /* ifAdminStatus */
static const VB snmp_mib_2_2_1_8_10[] = "2.2.1.8.10";      /* ifOperStatus */
static const VB snmp_mib_2_2_1_9_10[] = "2.2.1.9.10";      /* ifLastChanges */
static const VB snmp_mib_2_2_1_10_10[] = "2.2.1.10.10";    /* ifInOctets */
static const VB snmp_mib_2_2_1_11_10[] = "2.2.1.11.10";    /* ifInUcastPkts */
static const VB snmp_mib_2_2_1_12_10[] = "2.2.1.12.10";    /* ifInNUcastPkts */
static const VB snmp_mib_2_2_1_13_10[] = "2.2.1.13.10";    /* ifInDiscards */
static const VB snmp_mib_2_2_1_14_10[] = "2.2.1.14.10";    /* ifInErrors */
static const VB snmp_mib_2_2_1_15_10[] = "2.2.1.15.10";    /* ifInUnknownProtos */
static const VB snmp_mib_2_2_1_16_10[] = "2.2.1.16.10";    /* ifOutOctets */
static const VB snmp_mib_2_2_1_17_10[] = "2.2.1.17.10";    /* ifOutUcastPkts */
static const VB snmp_mib_2_2_1_18_10[] = "2.2.1.18.10";    /* ifOutNUcastPkts */
static const VB snmp_mib_2_2_1_19_10[] = "2.2.1.19.10";    /* ifOutDiscards */
static const VB snmp_mib_2_2_1_20_10[] = "2.2.1.20.10";    /* ifOutErrors */
static const VB snmp_mib_2_2_1_21_10[] = "2.2.1.21.10";    /* ifOutQLen */
static const VB snmp_mib_2_2_1_22_10[] = "2.2.1.22.10";    /* ifSpecific */
#endif
#if (CFG_SNMP_NET_DEV_CNT >= 11)
static const VB snmp_mib_2_2_1_1_11[] = "2.2.1.1.11";      /* ifIndex */
static const VB snmp_mib_2_2_1_2_11[] = "2.2.1.2.11";      /* ifDescr */
static const VB snmp_mib_2_2_1_3_11[] = "2.2.1.3.11";      /* ifType */
static const VB snmp_mib_2_2_1_4_11[] = "2.2.1.4.11";      /* ifMtu */
static const VB snmp_mib_2_2_1_5_11[] = "2.2.1.5.11";      /* ifSpeed */
static const VB snmp_mib_2_2_1_6_11[] = "2.2.1.6.11";      /* ifPhysAddress */
static const VB snmp_mib_2_2_1_7_11[] = "2.2.1.7.11";      /* ifAdminStatus */
static const VB snmp_mib_2_2_1_8_11[] = "2.2.1.8.11";      /* ifOperStatus */
static const VB snmp_mib_2_2_1_9_11[] = "2.2.1.9.11";      /* ifLastChanges */
static const VB snmp_mib_2_2_1_10_11[] = "2.2.1.10.11";    /* ifInOctets */
static const VB snmp_mib_2_2_1_11_11[] = "2.2.1.11.11";    /* ifInUcastPkts */
static const VB snmp_mib_2_2_1_12_11[] = "2.2.1.12.11";    /* ifInNUcastPkts */
static const VB snmp_mib_2_2_1_13_11[] = "2.2.1.13.11";    /* ifInDiscards */
static const VB snmp_mib_2_2_1_14_11[] = "2.2.1.14.11";    /* ifInErrors */
static const VB snmp_mib_2_2_1_15_11[] = "2.2.1.15.11";    /* ifInUnknownProtos */
static const VB snmp_mib_2_2_1_16_11[] = "2.2.1.16.11";    /* ifOutOctets */
static const VB snmp_mib_2_2_1_17_11[] = "2.2.1.17.11";    /* ifOutUcastPkts */
static const VB snmp_mib_2_2_1_18_11[] = "2.2.1.18.11";    /* ifOutNUcastPkts */
static const VB snmp_mib_2_2_1_19_11[] = "2.2.1.19.11";    /* ifOutDiscards */
static const VB snmp_mib_2_2_1_20_11[] = "2.2.1.20.11";    /* ifOutErrors */
static const VB snmp_mib_2_2_1_21_11[] = "2.2.1.21.11";    /* ifOutQLen */
static const VB snmp_mib_2_2_1_22_11[] = "2.2.1.22.11";    /* ifSpecific */
#endif
#if (CFG_SNMP_NET_DEV_CNT >= 12)
static const VB snmp_mib_2_2_1_1_12[] = "2.2.1.1.12";      /* ifIndex */
static const VB snmp_mib_2_2_1_2_12[] = "2.2.1.2.12";      /* ifDescr */
static const VB snmp_mib_2_2_1_3_12[] = "2.2.1.3.12";      /* ifType */
static const VB snmp_mib_2_2_1_4_12[] = "2.2.1.4.12";      /* ifMtu */
static const VB snmp_mib_2_2_1_5_12[] = "2.2.1.5.12";      /* ifSpeed */
static const VB snmp_mib_2_2_1_6_12[] = "2.2.1.6.12";      /* ifPhysAddress */
static const VB snmp_mib_2_2_1_7_12[] = "2.2.1.7.12";      /* ifAdminStatus */
static const VB snmp_mib_2_2_1_8_12[] = "2.2.1.8.12";      /* ifOperStatus */
static const VB snmp_mib_2_2_1_9_12[] = "2.2.1.9.12";      /* ifLastChanges */
static const VB snmp_mib_2_2_1_10_12[] = "2.2.1.10.12";    /* ifInOctets */
static const VB snmp_mib_2_2_1_11_12[] = "2.2.1.11.12";    /* ifInUcastPkts */
static const VB snmp_mib_2_2_1_12_12[] = "2.2.1.12.12";    /* ifInNUcastPkts */
static const VB snmp_mib_2_2_1_13_12[] = "2.2.1.13.12";    /* ifInDiscards */
static const VB snmp_mib_2_2_1_14_12[] = "2.2.1.14.12";    /* ifInErrors */
static const VB snmp_mib_2_2_1_15_12[] = "2.2.1.15.12";    /* ifInUnknownProtos */
static const VB snmp_mib_2_2_1_16_12[] = "2.2.1.16.12";    /* ifOutOctets */
static const VB snmp_mib_2_2_1_17_12[] = "2.2.1.17.12";    /* ifOutUcastPkts */
static const VB snmp_mib_2_2_1_18_12[] = "2.2.1.18.12";    /* ifOutNUcastPkts */
static const VB snmp_mib_2_2_1_19_12[] = "2.2.1.19.12";    /* ifOutDiscards */
static const VB snmp_mib_2_2_1_20_12[] = "2.2.1.20.12";    /* ifOutErrors */
static const VB snmp_mib_2_2_1_21_12[] = "2.2.1.21.12";    /* ifOutQLen */
static const VB snmp_mib_2_2_1_22_12[] = "2.2.1.22.12";    /* ifSpecific */
#endif
#if (CFG_SNMP_NET_DEV_CNT >= 13)
static const VB snmp_mib_2_2_1_1_13[] = "2.2.1.1.13";      /* ifIndex */
static const VB snmp_mib_2_2_1_2_13[] = "2.2.1.2.13";      /* ifDescr */
static const VB snmp_mib_2_2_1_3_13[] = "2.2.1.3.13";      /* ifType */
static const VB snmp_mib_2_2_1_4_13[] = "2.2.1.4.13";      /* ifMtu */
static const VB snmp_mib_2_2_1_5_13[] = "2.2.1.5.13";      /* ifSpeed */
static const VB snmp_mib_2_2_1_6_13[] = "2.2.1.6.13";      /* ifPhysAddress */
static const VB snmp_mib_2_2_1_7_13[] = "2.2.1.7.13";      /* ifAdminStatus */
static const VB snmp_mib_2_2_1_8_13[] = "2.2.1.8.13";      /* ifOperStatus */
static const VB snmp_mib_2_2_1_9_13[] = "2.2.1.9.13";      /* ifLastChanges */
static const VB snmp_mib_2_2_1_10_13[] = "2.2.1.10.13";    /* ifInOctets */
static const VB snmp_mib_2_2_1_11_13[] = "2.2.1.11.13";    /* ifInUcastPkts */
static const VB snmp_mib_2_2_1_12_13[] = "2.2.1.12.13";    /* ifInNUcastPkts */
static const VB snmp_mib_2_2_1_13_13[] = "2.2.1.13.13";    /* ifInDiscards */
static const VB snmp_mib_2_2_1_14_13[] = "2.2.1.14.13";    /* ifInErrors */
static const VB snmp_mib_2_2_1_15_13[] = "2.2.1.15.13";    /* ifInUnknownProtos */
static const VB snmp_mib_2_2_1_16_13[] = "2.2.1.16.13";    /* ifOutOctets */
static const VB snmp_mib_2_2_1_17_13[] = "2.2.1.17.13";    /* ifOutUcastPkts */
static const VB snmp_mib_2_2_1_18_13[] = "2.2.1.18.13";    /* ifOutNUcastPkts */
static const VB snmp_mib_2_2_1_19_13[] = "2.2.1.19.13";    /* ifOutDiscards */
static const VB snmp_mib_2_2_1_20_13[] = "2.2.1.20.13";    /* ifOutErrors */
static const VB snmp_mib_2_2_1_21_13[] = "2.2.1.21.13";    /* ifOutQLen */
static const VB snmp_mib_2_2_1_22_13[] = "2.2.1.22.13";    /* ifSpecific */
#endif
#if (CFG_SNMP_NET_DEV_CNT >= 14)
static const VB snmp_mib_2_2_1_1_14[] = "2.2.1.1.14";      /* ifIndex */
static const VB snmp_mib_2_2_1_2_14[] = "2.2.1.2.14";      /* ifDescr */
static const VB snmp_mib_2_2_1_3_14[] = "2.2.1.3.14";      /* ifType */
static const VB snmp_mib_2_2_1_4_14[] = "2.2.1.4.14";      /* ifMtu */
static const VB snmp_mib_2_2_1_5_14[] = "2.2.1.5.14";      /* ifSpeed */
static const VB snmp_mib_2_2_1_6_14[] = "2.2.1.6.14";      /* ifPhysAddress */
static const VB snmp_mib_2_2_1_7_14[] = "2.2.1.7.14";      /* ifAdminStatus */
static const VB snmp_mib_2_2_1_8_14[] = "2.2.1.8.14";      /* ifOperStatus */
static const VB snmp_mib_2_2_1_9_14[] = "2.2.1.9.14";      /* ifLastChanges */
static const VB snmp_mib_2_2_1_10_14[] = "2.2.1.10.14";    /* ifInOctets */
static const VB snmp_mib_2_2_1_11_14[] = "2.2.1.11.14";    /* ifInUcastPkts */
static const VB snmp_mib_2_2_1_12_14[] = "2.2.1.12.14";    /* ifInNUcastPkts */
static const VB snmp_mib_2_2_1_13_14[] = "2.2.1.13.14";    /* ifInDiscards */
static const VB snmp_mib_2_2_1_14_14[] = "2.2.1.14.14";    /* ifInErrors */
static const VB snmp_mib_2_2_1_15_14[] = "2.2.1.15.14";    /* ifInUnknownProtos */
static const VB snmp_mib_2_2_1_16_14[] = "2.2.1.16.14";    /* ifOutOctets */
static const VB snmp_mib_2_2_1_17_14[] = "2.2.1.17.14";    /* ifOutUcastPkts */
static const VB snmp_mib_2_2_1_18_14[] = "2.2.1.18.14";    /* ifOutNUcastPkts */
static const VB snmp_mib_2_2_1_19_14[] = "2.2.1.19.14";    /* ifOutDiscards */
static const VB snmp_mib_2_2_1_20_14[] = "2.2.1.20.14";    /* ifOutErrors */
static const VB snmp_mib_2_2_1_21_14[] = "2.2.1.21.14";    /* ifOutQLen */
static const VB snmp_mib_2_2_1_22_14[] = "2.2.1.22.14";    /* ifSpecific */
#endif
#if (CFG_SNMP_NET_DEV_CNT >= 15)
static const VB snmp_mib_2_2_1_1_15[] = "2.2.1.1.15";      /* ifIndex */
static const VB snmp_mib_2_2_1_2_15[] = "2.2.1.2.15";      /* ifDescr */
static const VB snmp_mib_2_2_1_3_15[] = "2.2.1.3.15";      /* ifType */
static const VB snmp_mib_2_2_1_4_15[] = "2.2.1.4.15";      /* ifMtu */
static const VB snmp_mib_2_2_1_5_15[] = "2.2.1.5.15";      /* ifSpeed */
static const VB snmp_mib_2_2_1_6_15[] = "2.2.1.6.15";      /* ifPhysAddress */
static const VB snmp_mib_2_2_1_7_15[] = "2.2.1.7.15";      /* ifAdminStatus */
static const VB snmp_mib_2_2_1_8_15[] = "2.2.1.8.15";      /* ifOperStatus */
static const VB snmp_mib_2_2_1_9_15[] = "2.2.1.9.15";      /* ifLastChanges */
static const VB snmp_mib_2_2_1_10_15[] = "2.2.1.10.15";    /* ifInOctets */
static const VB snmp_mib_2_2_1_11_15[] = "2.2.1.11.15";    /* ifInUcastPkts */
static const VB snmp_mib_2_2_1_12_15[] = "2.2.1.12.15";    /* ifInNUcastPkts */
static const VB snmp_mib_2_2_1_13_15[] = "2.2.1.13.15";    /* ifInDiscards */
static const VB snmp_mib_2_2_1_14_15[] = "2.2.1.14.15";    /* ifInErrors */
static const VB snmp_mib_2_2_1_15_15[] = "2.2.1.15.15";    /* ifInUnknownProtos */
static const VB snmp_mib_2_2_1_16_15[] = "2.2.1.16.15";    /* ifOutOctets */
static const VB snmp_mib_2_2_1_17_15[] = "2.2.1.17.15";    /* ifOutUcastPkts */
static const VB snmp_mib_2_2_1_18_15[] = "2.2.1.18.15";    /* ifOutNUcastPkts */
static const VB snmp_mib_2_2_1_19_15[] = "2.2.1.19.15";    /* ifOutDiscards */
static const VB snmp_mib_2_2_1_20_15[] = "2.2.1.20.15";    /* ifOutErrors */
static const VB snmp_mib_2_2_1_21_15[] = "2.2.1.21.15";    /* ifOutQLen */
static const VB snmp_mib_2_2_1_22_15[] = "2.2.1.22.15";    /* ifSpecific */
#endif
#if (CFG_SNMP_NET_DEV_CNT >= 16)
static const VB snmp_mib_2_2_1_1_16[] = "2.2.1.1.16";      /* ifIndex */
static const VB snmp_mib_2_2_1_2_16[] = "2.2.1.2.16";      /* ifDescr */
static const VB snmp_mib_2_2_1_3_16[] = "2.2.1.3.16";      /* ifType */
static const VB snmp_mib_2_2_1_4_16[] = "2.2.1.4.16";      /* ifMtu */
static const VB snmp_mib_2_2_1_5_16[] = "2.2.1.5.16";      /* ifSpeed */
static const VB snmp_mib_2_2_1_6_16[] = "2.2.1.6.16";      /* ifPhysAddress */
static const VB snmp_mib_2_2_1_7_16[] = "2.2.1.7.16";      /* ifAdminStatus */
static const VB snmp_mib_2_2_1_8_16[] = "2.2.1.8.16";      /* ifOperStatus */
static const VB snmp_mib_2_2_1_9_16[] = "2.2.1.9.16";      /* ifLastChanges */
static const VB snmp_mib_2_2_1_10_16[] = "2.2.1.10.16";    /* ifInOctets */
static const VB snmp_mib_2_2_1_11_16[] = "2.2.1.11.16";    /* ifInUcastPkts */
static const VB snmp_mib_2_2_1_12_16[] = "2.2.1.12.16";    /* ifInNUcastPkts */
static const VB snmp_mib_2_2_1_13_16[] = "2.2.1.13.16";    /* ifInDiscards */
static const VB snmp_mib_2_2_1_14_16[] = "2.2.1.14.16";    /* ifInErrors */
static const VB snmp_mib_2_2_1_15_16[] = "2.2.1.15.16";    /* ifInUnknownProtos */
static const VB snmp_mib_2_2_1_16_16[] = "2.2.1.16.16";    /* ifOutOctets */
static const VB snmp_mib_2_2_1_17_16[] = "2.2.1.17.16";    /* ifOutUcastPkts */
static const VB snmp_mib_2_2_1_18_16[] = "2.2.1.18.16";    /* ifOutNUcastPkts */
static const VB snmp_mib_2_2_1_19_16[] = "2.2.1.19.16";    /* ifOutDiscards */
static const VB snmp_mib_2_2_1_20_16[] = "2.2.1.20.16";    /* ifOutErrors */
static const VB snmp_mib_2_2_1_21_16[] = "2.2.1.21.16";    /* ifOutQLen */
static const VB snmp_mib_2_2_1_22_16[] = "2.2.1.22.16";    /* ifSpecific */
#endif
#endif

/* Address translation group */
#if (CFG_SNMP_MIB2_AT_ENA == 1)
static const VB snmp_mib_3_1[] = "3.1.0";            /* atTable */
static const VB snmp_mib_3_1_1[] = "3.1.1";          /* atEntry */
static const VB snmp_mib_3_1_1_1[] = "3.1.1.1";      /* atIfIndex */
static const VB snmp_mib_3_1_1_2[] = "3.1.1.2";      /* atPhysAddress */
static const VB snmp_mib_3_1_1_3[] = "3.1.1.3";      /* atNetAddress */
#endif

/* IP group */
#if (CFG_SNMP_MIB2_IP_ENA == 1)
static const VB snmp_mib_4_1[] = "4.1.0";            /* ipForwarding */
static const VB snmp_mib_4_2[] = "4.2.0";            /* ipDefaultTTL */
static const VB snmp_mib_4_3[] = "4.3.0";            /* ipInReceives */
static const VB snmp_mib_4_4[] = "4.4.0";            /* ipInHdrErrors */
static const VB snmp_mib_4_5[] = "4.5.0";            /* ipInAddrErrors */
static const VB snmp_mib_4_6[] = "4.6.0";            /* ipForwDatagrams */
static const VB snmp_mib_4_7[] = "4.7.0";            /* ipInUnknownProtos */
static const VB snmp_mib_4_8[] = "4.8.0";            /* ipInDiscards */
static const VB snmp_mib_4_9[] = "4.9.0";            /* ipInDelivers */
static const VB snmp_mib_4_10[] = "4.10.0";          /* ipOutRequests */
static const VB snmp_mib_4_11[] = "4.11.0";          /* ipOutDiscards */
static const VB snmp_mib_4_12[] = "4.12.0";          /* ipOutNoRoutes */
static const VB snmp_mib_4_13[] = "4.13.0";          /* ipReasmTimeout */
static const VB snmp_mib_4_14[] = "4.14.0";          /* ipReasmReqds */
static const VB snmp_mib_4_15[] = "4.15.0";          /* ipReasmOKs */
static const VB snmp_mib_4_16[] = "4.16.0";          /* ipReasmFails */
static const VB snmp_mib_4_17[] = "4.17.0";          /* ipFragOKs */
static const VB snmp_mib_4_18[] = "4.18.0";          /* ipFragFails */
static const VB snmp_mib_4_19[] = "4.19.0";          /* ipFragCreates */
static const VB snmp_mib_4_20[] = "4.20.0";          /* ipAddrTable */
static const VB snmp_mib_4_20_1[] = "4.20.1";        /* ipAddrEntry */
static const VB snmp_mib_4_20_1_1[] = "4.20.1.1";    /* ipAdEntAddr */
static const VB snmp_mib_4_20_1_2[] = "4.20.1.2";    /* ipAdEntIfIndex */
static const VB snmp_mib_4_20_1_3[] = "4.20.1.3";    /* ipAdEntNetMask */
static const VB snmp_mib_4_20_1_4[] = "4.20.1.4";    /* ipAdEntBcastAddr */
static const VB snmp_mib_4_20_1_5[] = "4.20.1.5";    /* ipAdEntReasmMaxSize */
static const VB snmp_mib_4_21[] = "4.21.0";          /* ipRouteTable */
static const VB snmp_mib_4_21_1[] = "4.21.1";        /* ipRouteEntry */
static const VB snmp_mib_4_22[] = "4.22.0";          /* ipNetToMediaTable */
static const VB snmp_mib_4_22_1[] = "4.22.1";        /* ipNetToMediaEntry */
static const VB snmp_mib_4_22_1_1[] = "4.22.1.1";    /* ipNetToMediaIfIndex  */
static const VB snmp_mib_4_22_1_2[] = "4.22.1.2";    /* ipNetToMediaPhysAddress */
static const VB snmp_mib_4_22_1_3[] = "4.22.1.3";    /* ipNetToMediaNetAddress */
static const VB snmp_mib_4_22_1_4[] = "4.22.1.4";    /* ipNetToMediaType */
static const VB snmp_mib_4_23[] = "4.23.0";          /* ipRoutingDiscards */
#endif

/* ICMP group */
#if (CFG_SNMP_MIB2_ICMP_ENA == 1)
static const VB snmp_mib_5_1[] = "5.1.0";      /* icmpInMsgs */
static const VB snmp_mib_5_2[] = "5.2.0";      /* icmpInErrors */
static const VB snmp_mib_5_3[] = "5.3.0";      /* icmpInDestUnreachs */
static const VB snmp_mib_5_4[] = "5.4.0";      /* icmpInTimeExcds */
static const VB snmp_mib_5_5[] = "5.5.0";      /* icmpInParmProbs */
static const VB snmp_mib_5_6[] = "5.6.0";      /* icmpInSrcQuenchs */
static const VB snmp_mib_5_7[] = "5.7.0";      /* icmpInRedirects */
static const VB snmp_mib_5_8[] = "5.8.0";      /* icmpInEchos */
static const VB snmp_mib_5_9[] = "5.9.0";      /* icmpInEchoReps */
static const VB snmp_mib_5_10[] = "5.10.0";    /* icmpInTimestamps */
static const VB snmp_mib_5_11[] = "5.11.0";    /* icmpInTimestampReps */
static const VB snmp_mib_5_12[] = "5.12.0";    /* icmpInAddrMasks */
static const VB snmp_mib_5_13[] = "5.13.0";    /* icmpInAddrMaskReps */
static const VB snmp_mib_5_14[] = "5.14.0";    /* icmpOutMsgs */
static const VB snmp_mib_5_15[] = "5.15.0";    /* icmpOutErrors */
static const VB snmp_mib_5_16[] = "5.16.0";    /* icmpOutDestUnreachs */
static const VB snmp_mib_5_17[] = "5.17.0";    /* icmpOutTimeExcds */
static const VB snmp_mib_5_18[] = "5.18.0";    /* icmpOutParmProbs */
static const VB snmp_mib_5_19[] = "5.19.0";    /* icmpOutSrcQuenchs */
static const VB snmp_mib_5_20[] = "5.20.0";    /* icmpOutRedirects */
static const VB snmp_mib_5_21[] = "5.21.0";    /* icmpOutEchos */
static const VB snmp_mib_5_22[] = "5.22.0";    /* icmpOutEchoReps */
static const VB snmp_mib_5_23[] = "5.23.0";    /* icmpOutTimestamps */
static const VB snmp_mib_5_24[] = "5.24.0";    /* icmpOutTimestampReps */
static const VB snmp_mib_5_25[] = "5.25.0";    /* icmpOutAddrMasks */
static const VB snmp_mib_5_26[] = "5.26.0";    /* icmpOutAddrMaskReps */
#endif

/* TCP group */
#if (CFG_SNMP_MIB2_TCP_ENA == 1)
static const VB snmp_mib_6_1[] = "6.1.0";               /* tcpRtoAlgorithm */
static const VB snmp_mib_6_2[] = "6.2.0";               /* tcpRtoMin */
static const VB snmp_mib_6_3[] = "6.3.0";               /* tcpRtoMax */
static const VB snmp_mib_6_4[] = "6.4.0";               /* tcpMaxConn */
static const VB snmp_mib_6_5[] = "6.5.0";               /* tcpActiveOpens */
static const VB snmp_mib_6_6[] = "6.6.0";               /* tcpPassiveOpens */
static const VB snmp_mib_6_7[] = "6.7.0";               /* tcpAttemptFails */
static const VB snmp_mib_6_8[] = "6.8.0";               /* tcpEstabResets */
static const VB snmp_mib_6_9[] = "6.9.0";               /* tcpCurrEstab */
static const VB snmp_mib_6_10[] = "6.10.0";             /* tcpInSegs */
static const VB snmp_mib_6_11[] = "6.11.0";             /* tcpOutSegs */
static const VB snmp_mib_6_12[] = "6.12.0";             /* tcpRetransSegs */
static const VB snmp_mib_6_13[] = "6.13.0";             /* tcpConnTable */
static const VB snmp_mib_6_13_1[] = "6.13.1";           /* tcpConnEntry */
static const VB snmp_mib_6_13_1_1[] = "6.13.1.1";       /* tcpConnState */
static const VB snmp_mib_6_13_1_2[] = "6.13.1.2";       /* tcpConnLocalAddress */
static const VB snmp_mib_6_13_1_3[] = "6.13.1.3";       /* tcpConnLocalPort */
static const VB snmp_mib_6_13_1_4[] = "6.13.1.4";       /* tcpConnRemAddress */
static const VB snmp_mib_6_13_1_5[] = "6.13.1.5";       /* tcpConnRemPort */
static const VB snmp_mib_6_14[] = "6.14.0";             /* tcpInErrs */
static const VB snmp_mib_6_15[] = "6.15.0";             /* tcpOutRsts */
#endif

/* UDP group */
#if (CFG_SNMP_MIB2_UDP_ENA == 1)
static const VB snmp_mib_7_1[] = "7.1.0";          /* udpInDatagrams */
static const VB snmp_mib_7_2[] = "7.2.0";          /* udpNoPorts */
static const VB snmp_mib_7_3[] = "7.3.0";          /* udpInErrors */
static const VB snmp_mib_7_4[] = "7.4.0";          /* udpOutDatagrams */
static const VB snmp_mib_7_5[] = "7.5.0";          /* udpTable */
static const VB snmp_mib_7_5_1[] = "7.5.1";        /* udpEntry */
static const VB snmp_mib_7_5_1_1[] = "7.5.1.1";    /* udpLocalAddress */
static const VB snmp_mib_7_5_1_2[] = "7.5.1.2";    /* udpLocalPort */
#endif

/* SNMP group */
#if (CFG_SNMP_MIB2_SNMP_ENA == 1)
static const VB snmp_mib_11_1[] = "11.1.0";      /* snmpInPkts */
static const VB snmp_mib_11_2[] = "11.2.0";      /* snmpOutPkts */
static const VB snmp_mib_11_3[] = "11.3.0";      /* snmpInBadVersions */
static const VB snmp_mib_11_4[] = "11.4.0";      /* snmpInBadCommunityNames */
static const VB snmp_mib_11_5[] = "11.5.0";      /* snmpInBadCommunityUses */
static const VB snmp_mib_11_6[] = "11.6.0";      /* snmpInASNParseErrs */
static const VB snmp_mib_11_8[] = "11.8.0";      /* snmpInTooBigs */
static const VB snmp_mib_11_9[] = "11.9.0";      /* snmpInNoSuchNames */
static const VB snmp_mib_11_10[] = "11.10.0";    /* snmpInBadValues */
static const VB snmp_mib_11_11[] = "11.11.0";    /* snmpInReadOnlys */
static const VB snmp_mib_11_12[] = "11.12.0";    /* snmpInGenErrs */
static const VB snmp_mib_11_13[] = "11.13.0";    /* snmpInTotalReqVars */
static const VB snmp_mib_11_14[] = "11.14.0";    /* snmpInTotalSetVars */
static const VB snmp_mib_11_15[] = "11.15.0";    /* snmpInGetRequests */
static const VB snmp_mib_11_16[] = "11.16.0";    /* snmpInGetNexts */
static const VB snmp_mib_11_17[] = "11.17.0";    /* snmpInSetRequests */
static const VB snmp_mib_11_18[] = "11.18.0";    /* snmpInGetResponses */
static const VB snmp_mib_11_19[] = "11.19.0";    /* snmpInTraps */
static const VB snmp_mib_11_20[] = "11.20.0";    /* snmpOutTooBigs */
static const VB snmp_mib_11_21[] = "11.21.0";    /* snmpOutNoSuchNames */
static const VB snmp_mib_11_22[] = "11.22.0";    /* snmpOutBadValues */
static const VB snmp_mib_11_24[] = "11.24.0";    /* snmpOutGetErrs */
static const VB snmp_mib_11_25[] = "11.25.0";    /* snmpOutGetRequests */
static const VB snmp_mib_11_26[] = "11.26.0";    /* snmpOutGetNexts */
static const VB snmp_mib_11_27[] = "11.27.0";    /* snmpOutSetRequests */
static const VB snmp_mib_11_28[] = "11.28.0";    /* snmpOutGetResponses */
static const VB snmp_mib_11_29[] = "11.29.0";    /* snmpOutTraps */
static const VB snmp_mib_11_30[] = "11.30.0";    /* snmpEnableAuthenTraps */
#endif

/* MIB */
const T_SNMP_MIB snmp_mib[SNMP_MIB2_OBJ_CNT + 1] = {
    /* System */
    {snmp_mib_1_1, CFG_SNMP_MIB_SYS_DESCR_LEN,    TYP_OCT_STR, STS_RO},    /* sysDescr    (Display String) */
    {snmp_mib_1_2, CFG_SNMP_MIB_SYS_OBJECTID_LEN, TYP_OBJ_ID,  STS_RO},    /* sysObjectID (Object Identifier) */
    {snmp_mib_1_3, LEN_INT,                       TYP_TIM_TIC, STS_RO},    /* sysUpTime   (TimeTicks) */
    {snmp_mib_1_4, CFG_SNMP_MIB_SYS_CONTACT_LEN,  TYP_OCT_STR, STS_RW},    /* sysContact  (Display String) */
    {snmp_mib_1_5, CFG_SNMP_MIB_SYS_NAME_LEN,     TYP_OCT_STR, STS_RW},    /* sysName     (Display String) */
    {snmp_mib_1_6, CFG_SNMP_MIB_SYS_LOCATION_LEN, TYP_OCT_STR, STS_RW},    /* sysLocation (Display String) */
    {snmp_mib_1_7, LEN_INT,                       TYP_INT,     STS_RO},    /* sysServices (Integer) */

    /* Address translation group */
    #if (CFG_SNMP_MIB2_AT_ENA == 1)
    {snmp_mib_3_1,      LEN_INT, TYP_SEQ,  STS_NO},    /* atTable       (Sequence) */
    {snmp_mib_3_1_1,    LEN_INT, TYP_NONE, STS_NO},    /* atEntry       (Entry) */
    {snmp_mib_3_1_1_1,  LEN_INT, TYP_NONE, STS_NO},    /* atIfIndex     (Entry) */
    {snmp_mib_3_1_1_2,  LEN_INT, TYP_NONE, STS_NO},    /* atPhysAddress (Entry) */
    {snmp_mib_3_1_1_3,  LEN_INT, TYP_NONE, STS_NO},    /* atNetAddress  (Entry) */
    #endif

    /* IP group */
    #if (CFG_SNMP_MIB2_IP_ENA == 1)
    {snmp_mib_4_1,      LEN_INT, TYP_INT,  STS_RO},    /* ipForwarding            (Integer [1..2]) */
    {snmp_mib_4_2,      LEN_INT, TYP_INT,  STS_RO},    /* ipDefaultTTL            (Integet32 [1..255]) */
    {snmp_mib_4_3,      LEN_INT, TYP_CNT,  STS_RO},    /* ipInReceives            (Counter32) */
    {snmp_mib_4_4,      LEN_INT, TYP_CNT,  STS_RO},    /* ipInHdrErrors           (Counter32) */
    {snmp_mib_4_5,      LEN_INT, TYP_CNT,  STS_RO},    /* ipInAddrErrors          (Counter32) */
    {snmp_mib_4_6,      LEN_INT, TYP_CNT,  STS_RO},    /* ipForwDatagrams         (Counter32) */
    {snmp_mib_4_7,      LEN_INT, TYP_CNT,  STS_RO},    /* ipInUnknownProtos       (Counter32) */
    {snmp_mib_4_8,      LEN_INT, TYP_CNT,  STS_RO},    /* ipInDiscards            (Counter32) */
    {snmp_mib_4_9,      LEN_INT, TYP_CNT,  STS_RO},    /* ipInDelivers            (Counter32) */
    {snmp_mib_4_10,     LEN_INT, TYP_CNT,  STS_RO},    /* ipOutRequests           (Counter32) */
    {snmp_mib_4_11,     LEN_INT, TYP_CNT,  STS_RO},    /* ipOutDiscards           (Counter32) */
    {snmp_mib_4_12,     LEN_INT, TYP_CNT,  STS_RO},    /* ipOutNoRoutes           (Counter32) */
    {snmp_mib_4_13,     LEN_INT, TYP_INT,  STS_RO},    /* ipReasmTimeout          (Integer32) */
    {snmp_mib_4_14,     LEN_INT, TYP_CNT,  STS_RO},    /* ipReasmReqds            (Counter32) */
    {snmp_mib_4_15,     LEN_INT, TYP_CNT,  STS_RO},    /* ipReasmOKs              (Counter32) */
    {snmp_mib_4_16,     LEN_INT, TYP_CNT,  STS_RO},    /* ipReasmFails            (Counter32) */
    {snmp_mib_4_17,     LEN_INT, TYP_CNT,  STS_RO},    /* ipFragOKs               (Counter32) */
    {snmp_mib_4_18,     LEN_INT, TYP_CNT,  STS_RO},    /* ipFragFails             (Counter32) */
    {snmp_mib_4_19,     LEN_INT, TYP_CNT,  STS_RO},    /* ipFragCreates           (Counter32) */
    {snmp_mib_4_20,     LEN_INT, TYP_SEQ,  STS_NO},    /* ipAddrTable             (Sequence) */
    {snmp_mib_4_20_1,   LEN_INT, TYP_NONE, STS_NO},    /* ipAddrEntry             (Entry) */
    {snmp_mib_4_20_1_1, LEN_INT, TYP_NONE, STS_NO},    /* ipAdEntAddr             (Entry) */
    {snmp_mib_4_20_1_2, LEN_INT, TYP_NONE, STS_NO},    /* ipAdEntIfIndex          (Entry) */
    {snmp_mib_4_20_1_3, LEN_INT, TYP_NONE, STS_NO},    /* ipAdEntNetMask          (Entry) */
    {snmp_mib_4_20_1_4, LEN_INT, TYP_NONE, STS_NO},    /* ipAdEntBcastAddr        (Entry) */
    {snmp_mib_4_20_1_5, LEN_INT, TYP_NONE, STS_NO},    /* ipAdEntReasmMaxSize     (Entry) */
    {snmp_mib_4_21,     LEN_INT, TYP_SEQ,  STS_NO},    /* ipRouteTable            (Sequence) */
    {snmp_mib_4_21_1,   LEN_INT, TYP_NONE, STS_NO},    /* ipRouteEntry            (Entry) */
    {snmp_mib_4_22,     LEN_INT, TYP_SEQ,  STS_NO},    /* ipNetToMediaTable       (Sequence) */
    {snmp_mib_4_22_1,   LEN_INT, TYP_NONE, STS_NO},    /* ipNetToMediaEntry       (Entry) */
    {snmp_mib_4_22_1_1, LEN_INT, TYP_NONE, STS_NO},    /* ipNetToMediaIfIndex     (Entry) */
    {snmp_mib_4_22_1_2, LEN_INT, TYP_NONE, STS_NO},    /* ipNetToMediaPhysAddress (Entry) */
    {snmp_mib_4_22_1_3, LEN_INT, TYP_NONE, STS_NO},    /* ipNetToMediaNetAddress  (Entry) */
    {snmp_mib_4_22_1_4, LEN_INT, TYP_NONE, STS_NO},    /* ipNetToMediaType        (Entry) */
    {snmp_mib_4_23,     LEN_INT, TYP_CNT,  STS_RO},    /* ipRoutingDiscards       (Counter32) */
    #endif

    /* ICMP group */
    #if (CFG_SNMP_MIB2_ICMP_ENA == 1)
    {snmp_mib_5_1,    LEN_INT, TYP_CNT,  STS_RO},    /* icmpInMsgs           (Counter32) */
    {snmp_mib_5_2,    LEN_INT, TYP_CNT,  STS_RO},    /* icmpInErrors         (Counter32) */
    {snmp_mib_5_3,    LEN_INT, TYP_CNT,  STS_RO},    /* icmpInDestUnreachs   (Counter32) */
    {snmp_mib_5_4,    LEN_INT, TYP_CNT,  STS_RO},    /* icmpInTimeExcds      (Counter32) */
    {snmp_mib_5_5,    LEN_INT, TYP_CNT,  STS_RO},    /* icmpInParmProbs      (Counter32) */
    {snmp_mib_5_6,    LEN_INT, TYP_CNT,  STS_RO},    /* icmpInSrcQuenchs     (Counter32) */
    {snmp_mib_5_7,    LEN_INT, TYP_CNT,  STS_RO},    /* icmpInRedirects      (Counter32) */
    {snmp_mib_5_8,    LEN_INT, TYP_CNT,  STS_RO},    /* icmpInEchos          (Counter32) */
    {snmp_mib_5_9,    LEN_INT, TYP_CNT,  STS_RO},    /* icmpInEchoReps       (Counter32) */
    {snmp_mib_5_10,   LEN_INT, TYP_CNT,  STS_RO},    /* icmpInTimestamps     (Counter32) */
    {snmp_mib_5_11,   LEN_INT, TYP_CNT,  STS_RO},    /* icmpInTimestampReps  (Counter32) */
    {snmp_mib_5_12,   LEN_INT, TYP_CNT,  STS_RO},    /* icmpInAddrMasks      (Counter32) */
    {snmp_mib_5_13,   LEN_INT, TYP_CNT,  STS_RO},    /* icmpInAddrMaskReps   (Counter32) */
    {snmp_mib_5_14,   LEN_INT, TYP_CNT,  STS_RO},    /* icmpOutMsgs          (Counter32) */
    {snmp_mib_5_15,   LEN_INT, TYP_CNT,  STS_RO},    /* icmpOutErrors        (Counter32) */
    {snmp_mib_5_16,   LEN_INT, TYP_CNT,  STS_RO},    /* icmpOutDestUnreachs  (Counter32) */
    {snmp_mib_5_17,   LEN_INT, TYP_CNT,  STS_RO},    /* icmpOutTimeExcds     (Counter32) */
    {snmp_mib_5_18,   LEN_INT, TYP_CNT,  STS_RO},    /* icmpOutParmProbs     (Counter32) */
    {snmp_mib_5_19,   LEN_INT, TYP_CNT,  STS_RO},    /* icmpOutSrcQuenchs    (Counter32) */
    {snmp_mib_5_20,   LEN_INT, TYP_CNT,  STS_RO},    /* icmpOutRedirects     (Counter32) */
    {snmp_mib_5_21,   LEN_INT, TYP_CNT,  STS_RO},    /* icmpOutEchos         (Counter32) */
    {snmp_mib_5_22,   LEN_INT, TYP_CNT,  STS_RO},    /* icmpOutEchoReps      (Counter32) */
    {snmp_mib_5_23,   LEN_INT, TYP_CNT,  STS_RO},    /* icmpOutTimestamps    (Counter32) */
    {snmp_mib_5_24,   LEN_INT, TYP_CNT,  STS_RO},    /* icmpOutTimestampReps (Counter32) */
    {snmp_mib_5_25,   LEN_INT, TYP_CNT,  STS_RO},    /* icmpOutAddrMasks     (Counter32) */
    {snmp_mib_5_26,   LEN_INT, TYP_CNT,  STS_RO},    /* icmpOutAddrMaskReps  (Counter32) */
    #endif

    /* TCP group */
    #if (CFG_SNMP_MIB2_TCP_ENA == 1)
    {snmp_mib_6_1,      LEN_INT, TYP_INT,     STS_RO},   /* tcpRtoAlgorithm     (Integer [1..5]) */
    {snmp_mib_6_2,      LEN_INT, TYP_INT,     STS_RO},   /* tcpRtoMin           (Integer32) */
    {snmp_mib_6_3,      LEN_INT, TYP_INT,     STS_RO},   /* tcpRtoMax           (Integer32) */
    {snmp_mib_6_4,      LEN_INT, TYP_INT,     STS_RO},   /* tcpMaxConn          (Integer32) */
    {snmp_mib_6_5,      LEN_INT, TYP_CNT,     STS_RO},   /* tcpActiveOpens      (Counter32) */
    {snmp_mib_6_6,      LEN_INT, TYP_CNT,     STS_RO},   /* tcpPassiveOpens     (Counter32) */
    {snmp_mib_6_7,      LEN_INT, TYP_CNT,     STS_RO},   /* tcpAttemptFails     (Counter32) */
    {snmp_mib_6_8,      LEN_INT, TYP_CNT,     STS_RO},   /* tcpEstabResets      (Counter32) */
    {snmp_mib_6_9,      LEN_INT, TYP_GAUGE,   STS_RO},   /* tcpCurrEstab        (Gauge32) */
    {snmp_mib_6_10,     LEN_INT, TYP_CNT,     STS_RO},   /* tcpInSegs           (Counter32) */
    {snmp_mib_6_11,     LEN_INT, TYP_CNT,     STS_RO},   /* tcpOutSegs          (Counter32) */
    {snmp_mib_6_12,     LEN_INT, TYP_CNT,     STS_RO},   /* tcpRetransSegs      (Counter32) */
    {snmp_mib_6_13,     LEN_INT, TYP_SEQ,     STS_NO},   /* tcpConnTable        (Sequence) */
    {snmp_mib_6_13_1,   LEN_INT, TYP_NONE,    STS_NO},   /* tcpConnEntry        (Entry) */
    {snmp_mib_6_13_1_1, LEN_INT, TYP_NONE,    STS_NO},   /* tcpConnState        (Entry) */
    {snmp_mib_6_13_1_2, LEN_INT, TYP_NONE,    STS_NO},   /* tcpConnLocalAddress (Entry) */
    {snmp_mib_6_13_1_3, LEN_INT, TYP_NONE,    STS_NO},   /* tcpConnLocalPort    (Entry) */
    {snmp_mib_6_13_1_4, LEN_INT, TYP_NONE,    STS_NO},   /* tcpConnRemAddress   (Entry) */
    {snmp_mib_6_13_1_5, LEN_INT, TYP_NONE,    STS_NO},   /* tcpConnRemPort      (Entry) */
    {snmp_mib_6_14,     LEN_INT, TYP_CNT,     STS_RO},   /* tcpInErrs           (Counter32) */
    {snmp_mib_6_15,     LEN_INT, TYP_CNT,     STS_RO},   /* tcpOutRsts          (Counter32) */
    #endif

    /* UDP group */
    #if (CFG_SNMP_MIB2_UDP_ENA == 1)
    {snmp_mib_7_1,     LEN_INT, TYP_CNT,     STS_RO},    /* udpInDatagrams  (Counter32) */
    {snmp_mib_7_2,     LEN_INT, TYP_CNT,     STS_RO},    /* udpNoPorts      (Counter32) */
    {snmp_mib_7_3,     LEN_INT, TYP_CNT,     STS_RO},    /* udpInErrors     (Counter32) */
    {snmp_mib_7_4,     LEN_INT, TYP_CNT,     STS_RO},    /* udpOutDatagrams (Counter32) */
    {snmp_mib_7_5,     LEN_INT, TYP_SEQ,     STS_NO},    /* udpTable        (Sequence) */
    {snmp_mib_7_5_1,   LEN_INT, TYP_NONE,    STS_NO},    /* udpEntry        (Entry) */
    {snmp_mib_7_5_1_1, LEN_INT, TYP_NONE,    STS_NO},    /* udpLocalAddress (Entry) */
    {snmp_mib_7_5_1_2, LEN_INT, TYP_NONE,    STS_NO},    /* udpLocalPort    (Entry) */
    #endif

    /* SNMP group */
    #if (CFG_SNMP_MIB2_SNMP_ENA == 1)
    {snmp_mib_11_1,    LEN_INT, TYP_CNT,     STS_RO},    /* snmpInPkts              (Counter32) */
    {snmp_mib_11_2,    LEN_INT, TYP_CNT,     STS_RO},    /* snmpOutPkts             (Counter32) */
    {snmp_mib_11_3,    LEN_INT, TYP_CNT,     STS_RO},    /* snmpInBadVersions       (Counter32) */
    {snmp_mib_11_4,    LEN_INT, TYP_CNT,     STS_RO},    /* snmpInBadCommunityNames (Counter32) */
    {snmp_mib_11_5,    LEN_INT, TYP_CNT,     STS_RO},    /* snmpInBadCommunityUses  (Counter32) */
    {snmp_mib_11_6,    LEN_INT, TYP_CNT,     STS_RO},    /* snmpInASNParseErrs      (Counter32) */
    {snmp_mib_11_8,    LEN_INT, TYP_CNT,     STS_RO},    /* snmpInTooBigs           (Counter32) */
    {snmp_mib_11_9,    LEN_INT, TYP_CNT,     STS_RO},    /* snmpInNoSuchNames       (Counter32) */
    {snmp_mib_11_10,   LEN_INT, TYP_CNT,     STS_RO},    /* snmpInBadValues         (Counter32) */
    {snmp_mib_11_11,   LEN_INT, TYP_CNT,     STS_RO},    /* snmpInReadOnlys         (Counter32) */
    {snmp_mib_11_12,   LEN_INT, TYP_CNT,     STS_RO},    /* snmpInGenErrs           (Counter32) */
    {snmp_mib_11_13,   LEN_INT, TYP_CNT,     STS_RO},    /* snmpInTotalReqVars      (Counter32) */
    {snmp_mib_11_14,   LEN_INT, TYP_CNT,     STS_RO},    /* snmpInTotalSetVars      (Counter32) */
    {snmp_mib_11_15,   LEN_INT, TYP_CNT,     STS_RO},    /* snmpInGetRequests       (Counter32) */
    {snmp_mib_11_16,   LEN_INT, TYP_CNT,     STS_RO},    /* snmpInGetNexts          (Counter32) */
    {snmp_mib_11_17,   LEN_INT, TYP_CNT,     STS_RO},    /* snmpInSetRequests       (Counter32) */
    {snmp_mib_11_18,   LEN_INT, TYP_CNT,     STS_RO},    /* snmpInGetResponses      (Counter32) */
    {snmp_mib_11_19,   LEN_INT, TYP_CNT,     STS_RO},    /* snmpInTraps             (Counter32) */
    {snmp_mib_11_20,   LEN_INT, TYP_CNT,     STS_RO},    /* snmpOutTooBigs          (Counter32) */
    {snmp_mib_11_21,   LEN_INT, TYP_CNT,     STS_RO},    /* snmpOutNoSuchNames      (Counter32) */
    {snmp_mib_11_22,   LEN_INT, TYP_CNT,     STS_RO},    /* snmpOutBadValues        (Counter32) */
    {snmp_mib_11_24,   LEN_INT, TYP_CNT,     STS_RO},    /* snmpOutGetErrs          (Counter32) */
    {snmp_mib_11_25,   LEN_INT, TYP_CNT,     STS_RO},    /* snmpOutGetRequests      (Counter32) */
    {snmp_mib_11_26,   LEN_INT, TYP_CNT,     STS_RO},    /* snmpOutGetNexts         (Counter32) */
    {snmp_mib_11_27,   LEN_INT, TYP_CNT,     STS_RO},    /* snmpOutSetRequests      (Counter32) */
    {snmp_mib_11_28,   LEN_INT, TYP_CNT,     STS_RO},    /* snmpOutGetResponses     (Counter32) */
    {snmp_mib_11_29,   LEN_INT, TYP_CNT,     STS_RO},    /* snmpOutTraps            (Counter32) */
    {snmp_mib_11_30,   LEN_INT, TYP_INT,     STS_RO},    /* snmpEnableAuthenTraps   (Counter32) */
    #endif

    /* Interfaces group */
    #if (CFG_SNMP_MIB2_IF_ENA == 1)
    {snmp_mib_2_1,         LEN_INT, TYP_INT,     STS_RO},     /* ifNumber          (Integer32) */
    {snmp_mib_2_2,         LEN_INT, TYP_SEQ,     STS_NO},     /* ifTable           (Sequence) */
    {snmp_mib_2_2_1,       LEN_INT, TYP_NONE,    STS_NO},     /* ifEntry           (Entry) */
    {snmp_mib_2_2_1_1_1,   LEN_INT, TYP_INT,     STS_RO},     /* ifIndex           (IfIndex) */
    {snmp_mib_2_2_1_2_1,   0,       TYP_OCT_STR, STS_RO},     /* ifDescr           (DisplayString) */
    {snmp_mib_2_2_1_3_1,   LEN_INT, TYP_INT,     STS_RO},     /* ifType            (IfType) */
    {snmp_mib_2_2_1_4_1,   LEN_INT, TYP_INT,     STS_RO},     /* ifMtu             (Integer32) */
    {snmp_mib_2_2_1_5_1,   LEN_INT, TYP_GAUGE,   STS_RO},     /* ifSpeed           (Gauge32) */
    {snmp_mib_2_2_1_6_1,   LEN_MAC, TYP_OCT_STR, STS_RO},     /* ifPhysAddress     (PhysAddress) */
    {snmp_mib_2_2_1_7_1,   LEN_INT, TYP_INT,     STS_RO},     /* ifAdminStatus     (Integer) */
    {snmp_mib_2_2_1_8_1,   LEN_INT, TYP_INT,     STS_RO},     /* ifOperStatus      (Integer) */
    {snmp_mib_2_2_1_9_1,   LEN_INT, TYP_TIM_TIC, STS_RO},     /* ifLastChanges     (TimeTicks) */
    {snmp_mib_2_2_1_10_1,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInOctets        (Counter32) */
    {snmp_mib_2_2_1_11_1,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInUcastPkts     (Counter32) */
    {snmp_mib_2_2_1_12_1,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInNUcastPkts    (Counter32) */
    {snmp_mib_2_2_1_13_1,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInDiscards      (Counter32) */
    {snmp_mib_2_2_1_14_1,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInErrors        (Counter32) */
    {snmp_mib_2_2_1_15_1,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInUnknownProtos (Counter32) */
    {snmp_mib_2_2_1_16_1,  LEN_INT, TYP_CNT,     STS_RO},     /* ifOutOctets       (Counter32) */
    {snmp_mib_2_2_1_17_1,  LEN_INT, TYP_CNT,     STS_RO},     /* ifOutUcastPkts    (Counter32) */
    {snmp_mib_2_2_1_18_1,  LEN_INT, TYP_CNT,     STS_RO},     /* ifOutNUcastPkts   (Counter32) */
    {snmp_mib_2_2_1_19_1,  LEN_INT, TYP_CNT,     STS_RO},     /* ifOutDiscards     (Counter32) */
    {snmp_mib_2_2_1_20_1,  LEN_INT, TYP_CNT,     STS_RO},     /* ifOutErrors       (Counter32) */
    {snmp_mib_2_2_1_21_1,  LEN_INT, TYP_GAUGE,   STS_RO},     /* ifOutQLen         (Gauge32) */
    {snmp_mib_2_2_1_22_1,  0,       TYP_OBJ_ID,  STS_RO},     /* ifSpecific        (Object Identifier) */
    #if (CFG_SNMP_NET_DEV_CNT >= 2)
    {snmp_mib_2_2_1_1_2,   LEN_INT, TYP_INT,     STS_RO},     /* ifIndex */
    {snmp_mib_2_2_1_2_2,   0,       TYP_OCT_STR, STS_RO},     /* ifDescr */
    {snmp_mib_2_2_1_3_2,   LEN_INT, TYP_INT,     STS_RO},     /* ifType */
    {snmp_mib_2_2_1_4_2,   LEN_INT, TYP_INT,     STS_RO},     /* ifMtu */
    {snmp_mib_2_2_1_5_2,   LEN_INT, TYP_GAUGE,   STS_RO},     /* ifSpeed */
    {snmp_mib_2_2_1_6_2,   LEN_MAC, TYP_OCT_STR, STS_RO},     /* ifPhysAddress */
    {snmp_mib_2_2_1_7_2,   LEN_INT, TYP_INT,     STS_RO},     /* ifAdminStatus */
    {snmp_mib_2_2_1_8_2,   LEN_INT, TYP_INT,     STS_RO},     /* ifOperStatus */
    {snmp_mib_2_2_1_9_2,   LEN_INT, TYP_TIM_TIC, STS_RO},     /* ifLastChanges */
    {snmp_mib_2_2_1_10_2,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInOctets */
    {snmp_mib_2_2_1_11_2,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInUcastPkts */
    {snmp_mib_2_2_1_12_2,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInNUcastPkts */
    {snmp_mib_2_2_1_13_2,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInDiscards */
    {snmp_mib_2_2_1_14_2,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInErrors */
    {snmp_mib_2_2_1_15_2,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInUnknownProtos */
    {snmp_mib_2_2_1_16_2,  LEN_INT, TYP_CNT,     STS_RO},     /* ifOutOctets */
    {snmp_mib_2_2_1_17_2,  LEN_INT, TYP_CNT,     STS_RO},     /* ifOutUcastPkts */
    {snmp_mib_2_2_1_18_2,  LEN_INT, TYP_CNT,     STS_RO},     /* ifOutNUcastPkts */
    {snmp_mib_2_2_1_19_2,  LEN_INT, TYP_CNT,     STS_RO},     /* ifOutDiscards */
    {snmp_mib_2_2_1_20_2,  LEN_INT, TYP_CNT,     STS_RO},     /* ifOutErrors */
    {snmp_mib_2_2_1_21_2,  LEN_INT, TYP_GAUGE,   STS_RO},     /* ifOutQLen */
    {snmp_mib_2_2_1_22_2,  0,       TYP_OBJ_ID,  STS_RO},     /* ifSpecific */
    #endif
    #if (CFG_SNMP_NET_DEV_CNT >= 3)
    {snmp_mib_2_2_1_1_3,   LEN_INT, TYP_INT,     STS_RO},     /* ifIndex */
    {snmp_mib_2_2_1_2_3,   0,       TYP_OCT_STR, STS_RO},     /* ifDescr */
    {snmp_mib_2_2_1_3_3,   LEN_INT, TYP_INT,     STS_RO},     /* ifType */
    {snmp_mib_2_2_1_4_3,   LEN_INT, TYP_INT,     STS_RO},     /* ifMtu */
    {snmp_mib_2_2_1_5_3,   LEN_INT, TYP_GAUGE,   STS_RO},     /* ifSpeed */
    {snmp_mib_2_2_1_6_3,   LEN_MAC, TYP_OCT_STR, STS_RO},     /* ifPhysAddress */
    {snmp_mib_2_2_1_7_3,   LEN_INT, TYP_INT,     STS_RO},     /* ifAdminStatus */
    {snmp_mib_2_2_1_8_3,   LEN_INT, TYP_INT,     STS_RO},     /* ifOperStatus */
    {snmp_mib_2_2_1_9_3,   LEN_INT, TYP_TIM_TIC, STS_RO},     /* ifLastChanges */
    {snmp_mib_2_2_1_10_3,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInOctets */
    {snmp_mib_2_2_1_11_3,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInUcastPkts */
    {snmp_mib_2_2_1_12_3,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInNUcastPkts */
    {snmp_mib_2_2_1_13_3,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInDiscards */
    {snmp_mib_2_2_1_14_3,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInErrors */
    {snmp_mib_2_2_1_15_3,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInUnknownProtos */
    {snmp_mib_2_2_1_16_3,  LEN_INT, TYP_CNT,     STS_RO},     /* ifOutOctets */
    {snmp_mib_2_2_1_17_3,  LEN_INT, TYP_CNT,     STS_RO},     /* ifOutUcastPkts */
    {snmp_mib_2_2_1_18_3,  LEN_INT, TYP_CNT,     STS_RO},     /* ifOutNUcastPkts */
    {snmp_mib_2_2_1_19_3,  LEN_INT, TYP_CNT,     STS_RO},     /* ifOutDiscards */
    {snmp_mib_2_2_1_20_3,  LEN_INT, TYP_CNT,     STS_RO},     /* ifOutErrors */
    {snmp_mib_2_2_1_21_3,  LEN_INT, TYP_GAUGE,   STS_RO},     /* ifOutQLen */
    {snmp_mib_2_2_1_22_3,  0,       TYP_OBJ_ID,  STS_RO},     /* ifSpecific */
    #endif
    #if (CFG_SNMP_NET_DEV_CNT >= 4)
    {snmp_mib_2_2_1_1_4,   LEN_INT, TYP_INT,     STS_RO},     /* ifIndex */
    {snmp_mib_2_2_1_2_4,   0,       TYP_OCT_STR, STS_RO},     /* ifDescr */
    {snmp_mib_2_2_1_3_4,   LEN_INT, TYP_INT,     STS_RO},     /* ifType */
    {snmp_mib_2_2_1_4_4,   LEN_INT, TYP_INT,     STS_RO},     /* ifMtu */
    {snmp_mib_2_2_1_5_4,   LEN_INT, TYP_GAUGE,   STS_RO},     /* ifSpeed */
    {snmp_mib_2_2_1_6_4,   LEN_MAC, TYP_OCT_STR, STS_RO},     /* ifPhysAddress */
    {snmp_mib_2_2_1_7_4,   LEN_INT, TYP_INT,     STS_RO},     /* ifAdminStatus */
    {snmp_mib_2_2_1_8_4,   LEN_INT, TYP_INT,     STS_RO},     /* ifOperStatus */
    {snmp_mib_2_2_1_9_4,   LEN_INT, TYP_TIM_TIC, STS_RO},     /* ifLastChanges */
    {snmp_mib_2_2_1_10_4,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInOctets */
    {snmp_mib_2_2_1_11_4,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInUcastPkts */
    {snmp_mib_2_2_1_12_4,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInNUcastPkts */
    {snmp_mib_2_2_1_13_4,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInDiscards */
    {snmp_mib_2_2_1_14_4,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInErrors */
    {snmp_mib_2_2_1_15_4,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInUnknownProtos */
    {snmp_mib_2_2_1_16_4,  LEN_INT, TYP_CNT,     STS_RO},     /* ifOutOctets */
    {snmp_mib_2_2_1_17_4,  LEN_INT, TYP_CNT,     STS_RO},     /* ifOutUcastPkts */
    {snmp_mib_2_2_1_18_4,  LEN_INT, TYP_CNT,     STS_RO},     /* ifOutNUcastPkts */
    {snmp_mib_2_2_1_19_4,  LEN_INT, TYP_CNT,     STS_RO},     /* ifOutDiscards */
    {snmp_mib_2_2_1_20_4,  LEN_INT, TYP_CNT,     STS_RO},     /* ifOutErrors */
    {snmp_mib_2_2_1_21_4,  LEN_INT, TYP_GAUGE,   STS_RO},     /* ifOutQLen */
    {snmp_mib_2_2_1_22_4,  0,       TYP_OBJ_ID,  STS_RO},     /* ifSpecific */
    #endif
    #if (CFG_SNMP_NET_DEV_CNT >= 5)
    {snmp_mib_2_2_1_1_5,   LEN_INT, TYP_INT,     STS_RO},     /* ifIndex */
    {snmp_mib_2_2_1_2_5,   0,       TYP_OCT_STR, STS_RO},     /* ifDescr */
    {snmp_mib_2_2_1_3_5,   LEN_INT, TYP_INT,     STS_RO},     /* ifType */
    {snmp_mib_2_2_1_4_5,   LEN_INT, TYP_INT,     STS_RO},     /* ifMtu */
    {snmp_mib_2_2_1_5_5,   LEN_INT, TYP_GAUGE,   STS_RO},     /* ifSpeed */
    {snmp_mib_2_2_1_6_5,   LEN_MAC, TYP_OCT_STR, STS_RO},     /* ifPhysAddress */
    {snmp_mib_2_2_1_7_5,   LEN_INT, TYP_INT,     STS_RO},     /* ifAdminStatus */
    {snmp_mib_2_2_1_8_5,   LEN_INT, TYP_INT,     STS_RO},     /* ifOperStatus */
    {snmp_mib_2_2_1_9_5,   LEN_INT, TYP_TIM_TIC, STS_RO},     /* ifLastChanges */
    {snmp_mib_2_2_1_10_5,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInOctets */
    {snmp_mib_2_2_1_11_5,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInUcastPkts */
    {snmp_mib_2_2_1_12_5,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInNUcastPkts */
    {snmp_mib_2_2_1_13_5,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInDiscards */
    {snmp_mib_2_2_1_14_5,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInErrors */
    {snmp_mib_2_2_1_15_5,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInUnknownProtos */
    {snmp_mib_2_2_1_16_5,  LEN_INT, TYP_CNT,     STS_RO},     /* ifOutOctets */
    {snmp_mib_2_2_1_17_5,  LEN_INT, TYP_CNT,     STS_RO},     /* ifOutUcastPkts */
    {snmp_mib_2_2_1_18_5,  LEN_INT, TYP_CNT,     STS_RO},     /* ifOutNUcastPkts */
    {snmp_mib_2_2_1_19_5,  LEN_INT, TYP_CNT,     STS_RO},     /* ifOutDiscards */
    {snmp_mib_2_2_1_20_5,  LEN_INT, TYP_CNT,     STS_RO},     /* ifOutErrors */
    {snmp_mib_2_2_1_21_5,  LEN_INT, TYP_GAUGE,   STS_RO},     /* ifOutQLen */
    {snmp_mib_2_2_1_22_5,  0,       TYP_OBJ_ID,  STS_RO},     /* ifSpecific */
    #endif
    #if (CFG_SNMP_NET_DEV_CNT >= 6)
    {snmp_mib_2_2_1_1_6,   LEN_INT, TYP_INT,     STS_RO},     /* ifIndex */
    {snmp_mib_2_2_1_2_6,   0,       TYP_OCT_STR, STS_RO},     /* ifDescr */
    {snmp_mib_2_2_1_3_6,   LEN_INT, TYP_INT,     STS_RO},     /* ifType */
    {snmp_mib_2_2_1_4_6,   LEN_INT, TYP_INT,     STS_RO},     /* ifMtu */
    {snmp_mib_2_2_1_5_6,   LEN_INT, TYP_GAUGE,   STS_RO},     /* ifSpeed */
    {snmp_mib_2_2_1_6_6,   LEN_MAC, TYP_OCT_STR, STS_RO},     /* ifPhysAddress */
    {snmp_mib_2_2_1_7_6,   LEN_INT, TYP_INT,     STS_RO},     /* ifAdminStatus */
    {snmp_mib_2_2_1_8_6,   LEN_INT, TYP_INT,     STS_RO},     /* ifOperStatus */
    {snmp_mib_2_2_1_9_6,   LEN_INT, TYP_TIM_TIC, STS_RO},     /* ifLastChanges */
    {snmp_mib_2_2_1_10_6,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInOctets */
    {snmp_mib_2_2_1_11_6,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInUcastPkts */
    {snmp_mib_2_2_1_12_6,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInNUcastPkts */
    {snmp_mib_2_2_1_13_6,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInDiscards */
    {snmp_mib_2_2_1_14_6,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInErrors */
    {snmp_mib_2_2_1_15_6,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInUnknownProtos */
    {snmp_mib_2_2_1_16_6,  LEN_INT, TYP_CNT,     STS_RO},     /* ifOutOctets */
    {snmp_mib_2_2_1_17_6,  LEN_INT, TYP_CNT,     STS_RO},     /* ifOutUcastPkts */
    {snmp_mib_2_2_1_18_6,  LEN_INT, TYP_CNT,     STS_RO},     /* ifOutNUcastPkts */
    {snmp_mib_2_2_1_19_6,  LEN_INT, TYP_CNT,     STS_RO},     /* ifOutDiscards */
    {snmp_mib_2_2_1_20_6,  LEN_INT, TYP_CNT,     STS_RO},     /* ifOutErrors */
    {snmp_mib_2_2_1_21_6,  LEN_INT, TYP_GAUGE,   STS_RO},     /* ifOutQLen */
    {snmp_mib_2_2_1_22_6,  0,       TYP_OBJ_ID,  STS_RO},     /* ifSpecific */
    #endif
    #if (CFG_SNMP_NET_DEV_CNT >= 7)
    {snmp_mib_2_2_1_1_7,   LEN_INT, TYP_INT,     STS_RO},     /* ifIndex */
    {snmp_mib_2_2_1_2_7,   0,       TYP_OCT_STR, STS_RO},     /* ifDescr */
    {snmp_mib_2_2_1_3_7,   LEN_INT, TYP_INT,     STS_RO},     /* ifType */
    {snmp_mib_2_2_1_4_7,   LEN_INT, TYP_INT,     STS_RO},     /* ifMtu */
    {snmp_mib_2_2_1_5_7,   LEN_INT, TYP_GAUGE,   STS_RO},     /* ifSpeed */
    {snmp_mib_2_2_1_6_7,   LEN_MAC, TYP_OCT_STR, STS_RO},     /* ifPhysAddress */
    {snmp_mib_2_2_1_7_7,   LEN_INT, TYP_INT,     STS_RO},     /* ifAdminStatus */
    {snmp_mib_2_2_1_8_7,   LEN_INT, TYP_INT,     STS_RO},     /* ifOperStatus */
    {snmp_mib_2_2_1_9_7,   LEN_INT, TYP_TIM_TIC, STS_RO},     /* ifLastChanges */
    {snmp_mib_2_2_1_10_7,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInOctets */
    {snmp_mib_2_2_1_11_7,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInUcastPkts */
    {snmp_mib_2_2_1_12_7,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInNUcastPkts */
    {snmp_mib_2_2_1_13_7,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInDiscards */
    {snmp_mib_2_2_1_14_7,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInErrors */
    {snmp_mib_2_2_1_15_7,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInUnknownProtos */
    {snmp_mib_2_2_1_16_7,  LEN_INT, TYP_CNT,     STS_RO},     /* ifOutOctets */
    {snmp_mib_2_2_1_17_7,  LEN_INT, TYP_CNT,     STS_RO},     /* ifOutUcastPkts */
    {snmp_mib_2_2_1_18_7,  LEN_INT, TYP_CNT,     STS_RO},     /* ifOutNUcastPkts */
    {snmp_mib_2_2_1_19_7,  LEN_INT, TYP_CNT,     STS_RO},     /* ifOutDiscards */
    {snmp_mib_2_2_1_20_7,  LEN_INT, TYP_CNT,     STS_RO},     /* ifOutErrors */
    {snmp_mib_2_2_1_21_7,  LEN_INT, TYP_GAUGE,   STS_RO},     /* ifOutQLen */
    {snmp_mib_2_2_1_22_7,  0,       TYP_OBJ_ID,  STS_RO},     /* ifSpecific */
    #endif
    #if (CFG_SNMP_NET_DEV_CNT >= 8)
    {snmp_mib_2_2_1_1_8,   LEN_INT, TYP_INT,     STS_RO},     /* ifIndex */
    {snmp_mib_2_2_1_2_8,   0,       TYP_OCT_STR, STS_RO},     /* ifDescr */
    {snmp_mib_2_2_1_3_8,   LEN_INT, TYP_INT,     STS_RO},     /* ifType */
    {snmp_mib_2_2_1_4_8,   LEN_INT, TYP_INT,     STS_RO},     /* ifMtu */
    {snmp_mib_2_2_1_5_8,   LEN_INT, TYP_GAUGE,   STS_RO},     /* ifSpeed */
    {snmp_mib_2_2_1_6_8,   LEN_MAC, TYP_OCT_STR, STS_RO},     /* ifPhysAddress */
    {snmp_mib_2_2_1_7_8,   LEN_INT, TYP_INT,     STS_RO},     /* ifAdminStatus */
    {snmp_mib_2_2_1_8_8,   LEN_INT, TYP_INT,     STS_RO},     /* ifOperStatus */
    {snmp_mib_2_2_1_9_8,   LEN_INT, TYP_TIM_TIC, STS_RO},     /* ifLastChanges */
    {snmp_mib_2_2_1_10_8,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInOctets */
    {snmp_mib_2_2_1_11_8,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInUcastPkts */
    {snmp_mib_2_2_1_12_8,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInNUcastPkts */
    {snmp_mib_2_2_1_13_8,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInDiscards */
    {snmp_mib_2_2_1_14_8,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInErrors */
    {snmp_mib_2_2_1_15_8,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInUnknownProtos */
    {snmp_mib_2_2_1_16_8,  LEN_INT, TYP_CNT,     STS_RO},     /* ifOutOctets */
    {snmp_mib_2_2_1_17_8,  LEN_INT, TYP_CNT,     STS_RO},     /* ifOutUcastPkts */
    {snmp_mib_2_2_1_18_8,  LEN_INT, TYP_CNT,     STS_RO},     /* ifOutNUcastPkts */
    {snmp_mib_2_2_1_19_8,  LEN_INT, TYP_CNT,     STS_RO},     /* ifOutDiscards */
    {snmp_mib_2_2_1_20_8,  LEN_INT, TYP_CNT,     STS_RO},     /* ifOutErrors */
    {snmp_mib_2_2_1_21_8,  LEN_INT, TYP_GAUGE,   STS_RO},     /* ifOutQLen */
    {snmp_mib_2_2_1_22_8,  0,       TYP_OBJ_ID,  STS_RO},     /* ifSpecific */
    #endif
    #if (CFG_SNMP_NET_DEV_CNT >= 9)
    {snmp_mib_2_2_1_1_9,   LEN_INT, TYP_INT,     STS_RO},     /* ifIndex */
    {snmp_mib_2_2_1_2_9,   0,       TYP_OCT_STR, STS_RO},     /* ifDescr */
    {snmp_mib_2_2_1_3_9,   LEN_INT, TYP_INT,     STS_RO},     /* ifType */
    {snmp_mib_2_2_1_4_9,   LEN_INT, TYP_INT,     STS_RO},     /* ifMtu */
    {snmp_mib_2_2_1_5_9,   LEN_INT, TYP_GAUGE,   STS_RO},     /* ifSpeed */
    {snmp_mib_2_2_1_6_9,   LEN_MAC, TYP_OCT_STR, STS_RO},     /* ifPhysAddress */
    {snmp_mib_2_2_1_7_9,   LEN_INT, TYP_INT,     STS_RO},     /* ifAdminStatus */
    {snmp_mib_2_2_1_8_9,   LEN_INT, TYP_INT,     STS_RO},     /* ifOperStatus */
    {snmp_mib_2_2_1_9_9,   LEN_INT, TYP_TIM_TIC, STS_RO},     /* ifLastChanges */
    {snmp_mib_2_2_1_10_9,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInOctets */
    {snmp_mib_2_2_1_11_9,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInUcastPkts */
    {snmp_mib_2_2_1_12_9,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInNUcastPkts */
    {snmp_mib_2_2_1_13_9,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInDiscards */
    {snmp_mib_2_2_1_14_9,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInErrors */
    {snmp_mib_2_2_1_15_9,  LEN_INT, TYP_CNT,     STS_RO},     /* ifInUnknownProtos */
    {snmp_mib_2_2_1_16_9,  LEN_INT, TYP_CNT,     STS_RO},     /* ifOutOctets */
    {snmp_mib_2_2_1_17_9,  LEN_INT, TYP_CNT,     STS_RO},     /* ifOutUcastPkts */
    {snmp_mib_2_2_1_18_9,  LEN_INT, TYP_CNT,     STS_RO},     /* ifOutNUcastPkts */
    {snmp_mib_2_2_1_19_9,  LEN_INT, TYP_CNT,     STS_RO},     /* ifOutDiscards */
    {snmp_mib_2_2_1_20_9,  LEN_INT, TYP_CNT,     STS_RO},     /* ifOutErrors */
    {snmp_mib_2_2_1_21_9,  LEN_INT, TYP_GAUGE,   STS_RO},     /* ifOutQLen */
    {snmp_mib_2_2_1_22_9,  0,       TYP_OBJ_ID,  STS_RO},     /* ifSpecific */
    #endif
    #if (CFG_SNMP_NET_DEV_CNT >= 10)
    {snmp_mib_2_2_1_1_10,  LEN_INT, TYP_INT,     STS_RO},     /* ifIndex */
    {snmp_mib_2_2_1_2_10,  0,       TYP_OCT_STR, STS_RO},     /* ifDescr */
    {snmp_mib_2_2_1_3_10,  LEN_INT, TYP_INT,     STS_RO},     /* ifType */
    {snmp_mib_2_2_1_4_10,  LEN_INT, TYP_INT,     STS_RO},     /* ifMtu */
    {snmp_mib_2_2_1_5_10,  LEN_INT, TYP_GAUGE,   STS_RO},     /* ifSpeed */
    {snmp_mib_2_2_1_6_10,  LEN_MAC, TYP_OCT_STR, STS_RO},     /* ifPhysAddress */
    {snmp_mib_2_2_1_7_10,  LEN_INT, TYP_INT,     STS_RO},     /* ifAdminStatus */
    {snmp_mib_2_2_1_8_10,  LEN_INT, TYP_INT,     STS_RO},     /* ifOperStatus */
    {snmp_mib_2_2_1_9_10,  LEN_INT, TYP_TIM_TIC, STS_RO},     /* ifLastChanges */
    {snmp_mib_2_2_1_10_10, LEN_INT, TYP_CNT,     STS_RO},     /* ifInOctets */
    {snmp_mib_2_2_1_11_10, LEN_INT, TYP_CNT,     STS_RO},     /* ifInUcastPkts */
    {snmp_mib_2_2_1_12_10, LEN_INT, TYP_CNT,     STS_RO},     /* ifInNUcastPkts */
    {snmp_mib_2_2_1_13_10, LEN_INT, TYP_CNT,     STS_RO},     /* ifInDiscards */
    {snmp_mib_2_2_1_14_10, LEN_INT, TYP_CNT,     STS_RO},     /* ifInErrors */
    {snmp_mib_2_2_1_15_10, LEN_INT, TYP_CNT,     STS_RO},     /* ifInUnknownProtos */
    {snmp_mib_2_2_1_16_10, LEN_INT, TYP_CNT,     STS_RO},     /* ifOutOctets */
    {snmp_mib_2_2_1_17_10, LEN_INT, TYP_CNT,     STS_RO},     /* ifOutUcastPkts */
    {snmp_mib_2_2_1_18_10, LEN_INT, TYP_CNT,     STS_RO},     /* ifOutNUcastPkts */
    {snmp_mib_2_2_1_19_10, LEN_INT, TYP_CNT,     STS_RO},     /* ifOutDiscards */
    {snmp_mib_2_2_1_20_10, LEN_INT, TYP_CNT,     STS_RO},     /* ifOutErrors */
    {snmp_mib_2_2_1_21_10, LEN_INT, TYP_GAUGE,   STS_RO},     /* ifOutQLen */
    {snmp_mib_2_2_1_22_10, 0,       TYP_OBJ_ID,  STS_RO},     /* ifSpecific */
    #endif
    #if (CFG_SNMP_NET_DEV_CNT >= 11)
    {snmp_mib_2_2_1_1_11,  LEN_INT, TYP_INT,     STS_RO},     /* ifIndex */
    {snmp_mib_2_2_1_2_11,  0,       TYP_OCT_STR, STS_RO},     /* ifDescr */
    {snmp_mib_2_2_1_3_11,  LEN_INT, TYP_INT,     STS_RO},     /* ifType */
    {snmp_mib_2_2_1_4_11,  LEN_INT, TYP_INT,     STS_RO},     /* ifMtu */
    {snmp_mib_2_2_1_5_11,  LEN_INT, TYP_GAUGE,   STS_RO},     /* ifSpeed */
    {snmp_mib_2_2_1_6_11,  LEN_MAC, TYP_OCT_STR, STS_RO},     /* ifPhysAddress */
    {snmp_mib_2_2_1_7_11,  LEN_INT, TYP_INT,     STS_RO},     /* ifAdminStatus */
    {snmp_mib_2_2_1_8_11,  LEN_INT, TYP_INT,     STS_RO},     /* ifOperStatus */
    {snmp_mib_2_2_1_9_11,  LEN_INT, TYP_TIM_TIC, STS_RO},     /* ifLastChanges */
    {snmp_mib_2_2_1_10_11, LEN_INT, TYP_CNT,     STS_RO},     /* ifInOctets */
    {snmp_mib_2_2_1_11_11, LEN_INT, TYP_CNT,     STS_RO},     /* ifInUcastPkts */
    {snmp_mib_2_2_1_12_11, LEN_INT, TYP_CNT,     STS_RO},     /* ifInNUcastPkts */
    {snmp_mib_2_2_1_13_11, LEN_INT, TYP_CNT,     STS_RO},     /* ifInDiscards */
    {snmp_mib_2_2_1_14_11, LEN_INT, TYP_CNT,     STS_RO},     /* ifInErrors */
    {snmp_mib_2_2_1_15_11, LEN_INT, TYP_CNT,     STS_RO},     /* ifInUnknownProtos */
    {snmp_mib_2_2_1_16_11, LEN_INT, TYP_CNT,     STS_RO},     /* ifOutOctets */
    {snmp_mib_2_2_1_17_11, LEN_INT, TYP_CNT,     STS_RO},     /* ifOutUcastPkts */
    {snmp_mib_2_2_1_18_11, LEN_INT, TYP_CNT,     STS_RO},     /* ifOutNUcastPkts */
    {snmp_mib_2_2_1_19_11, LEN_INT, TYP_CNT,     STS_RO},     /* ifOutDiscards */
    {snmp_mib_2_2_1_20_11, LEN_INT, TYP_CNT,     STS_RO},     /* ifOutErrors */
    {snmp_mib_2_2_1_21_11, LEN_INT, TYP_GAUGE,   STS_RO},     /* ifOutQLen */
    {snmp_mib_2_2_1_22_11, 0,       TYP_OBJ_ID,  STS_RO},     /* ifSpecific */
    #endif
    #if (CFG_SNMP_NET_DEV_CNT >= 12)
    {snmp_mib_2_2_1_1_12,  LEN_INT, TYP_INT,     STS_RO},     /* ifIndex */
    {snmp_mib_2_2_1_2_12,  0,       TYP_OCT_STR, STS_RO},     /* ifDescr */
    {snmp_mib_2_2_1_3_12,  LEN_INT, TYP_INT,     STS_RO},     /* ifType */
    {snmp_mib_2_2_1_4_12,  LEN_INT, TYP_INT,     STS_RO},     /* ifMtu */
    {snmp_mib_2_2_1_5_12,  LEN_INT, TYP_GAUGE,   STS_RO},     /* ifSpeed */
    {snmp_mib_2_2_1_6_12,  LEN_MAC, TYP_OCT_STR, STS_RO},     /* ifPhysAddress */
    {snmp_mib_2_2_1_7_12,  LEN_INT, TYP_INT,     STS_RO},     /* ifAdminStatus */
    {snmp_mib_2_2_1_8_12,  LEN_INT, TYP_INT,     STS_RO},     /* ifOperStatus */
    {snmp_mib_2_2_1_9_12,  LEN_INT, TYP_TIM_TIC, STS_RO},     /* ifLastChanges */
    {snmp_mib_2_2_1_10_12, LEN_INT, TYP_CNT,     STS_RO},     /* ifInOctets */
    {snmp_mib_2_2_1_11_12, LEN_INT, TYP_CNT,     STS_RO},     /* ifInUcastPkts */
    {snmp_mib_2_2_1_12_12, LEN_INT, TYP_CNT,     STS_RO},     /* ifInNUcastPkts */
    {snmp_mib_2_2_1_13_12, LEN_INT, TYP_CNT,     STS_RO},     /* ifInDiscards */
    {snmp_mib_2_2_1_14_12, LEN_INT, TYP_CNT,     STS_RO},     /* ifInErrors */
    {snmp_mib_2_2_1_15_12, LEN_INT, TYP_CNT,     STS_RO},     /* ifInUnknownProtos */
    {snmp_mib_2_2_1_16_12, LEN_INT, TYP_CNT,     STS_RO},     /* ifOutOctets */
    {snmp_mib_2_2_1_17_12, LEN_INT, TYP_CNT,     STS_RO},     /* ifOutUcastPkts */
    {snmp_mib_2_2_1_18_12, LEN_INT, TYP_CNT,     STS_RO},     /* ifOutNUcastPkts */
    {snmp_mib_2_2_1_19_12, LEN_INT, TYP_CNT,     STS_RO},     /* ifOutDiscards */
    {snmp_mib_2_2_1_20_12, LEN_INT, TYP_CNT,     STS_RO},     /* ifOutErrors */
    {snmp_mib_2_2_1_21_12, LEN_INT, TYP_GAUGE,   STS_RO},     /* ifOutQLen */
    {snmp_mib_2_2_1_22_12, 0,       TYP_OBJ_ID,  STS_RO},     /* ifSpecific */
    #endif
    #if (CFG_SNMP_NET_DEV_CNT >= 13)
    {snmp_mib_2_2_1_1_13,  LEN_INT, TYP_INT,     STS_RO},     /* ifIndex */
    {snmp_mib_2_2_1_2_13,  0,       TYP_OCT_STR, STS_RO},     /* ifDescr */
    {snmp_mib_2_2_1_3_13,  LEN_INT, TYP_INT,     STS_RO},     /* ifType */
    {snmp_mib_2_2_1_4_13,  LEN_INT, TYP_INT,     STS_RO},     /* ifMtu */
    {snmp_mib_2_2_1_5_13,  LEN_INT, TYP_GAUGE,   STS_RO},     /* ifSpeed */
    {snmp_mib_2_2_1_6_13,  LEN_MAC, TYP_OCT_STR, STS_RO},     /* ifPhysAddress */
    {snmp_mib_2_2_1_7_13,  LEN_INT, TYP_INT,     STS_RO},     /* ifAdminStatus */
    {snmp_mib_2_2_1_8_13,  LEN_INT, TYP_INT,     STS_RO},     /* ifOperStatus */
    {snmp_mib_2_2_1_9_13,  LEN_INT, TYP_TIM_TIC, STS_RO},     /* ifLastChanges */
    {snmp_mib_2_2_1_10_13, LEN_INT, TYP_CNT,     STS_RO},     /* ifInOctets */
    {snmp_mib_2_2_1_11_13, LEN_INT, TYP_CNT,     STS_RO},     /* ifInUcastPkts */
    {snmp_mib_2_2_1_12_13, LEN_INT, TYP_CNT,     STS_RO},     /* ifInNUcastPkts */
    {snmp_mib_2_2_1_13_13, LEN_INT, TYP_CNT,     STS_RO},     /* ifInDiscards */
    {snmp_mib_2_2_1_14_13, LEN_INT, TYP_CNT,     STS_RO},     /* ifInErrors */
    {snmp_mib_2_2_1_15_13, LEN_INT, TYP_CNT,     STS_RO},     /* ifInUnknownProtos */
    {snmp_mib_2_2_1_16_13, LEN_INT, TYP_CNT,     STS_RO},     /* ifOutOctets */
    {snmp_mib_2_2_1_17_13, LEN_INT, TYP_CNT,     STS_RO},     /* ifOutUcastPkts */
    {snmp_mib_2_2_1_18_13, LEN_INT, TYP_CNT,     STS_RO},     /* ifOutNUcastPkts */
    {snmp_mib_2_2_1_19_13, LEN_INT, TYP_CNT,     STS_RO},     /* ifOutDiscards */
    {snmp_mib_2_2_1_20_13, LEN_INT, TYP_CNT,     STS_RO},     /* ifOutErrors */
    {snmp_mib_2_2_1_21_13, LEN_INT, TYP_GAUGE,   STS_RO},     /* ifOutQLen */
    {snmp_mib_2_2_1_22_13, 0,       TYP_OBJ_ID,  STS_RO},     /* ifSpecific */
    #endif
    #if (CFG_SNMP_NET_DEV_CNT >= 14)
    {snmp_mib_2_2_1_1_14,  LEN_INT, TYP_INT,     STS_RO},     /* ifIndex */
    {snmp_mib_2_2_1_2_14,  0,       TYP_OCT_STR, STS_RO},     /* ifDescr */
    {snmp_mib_2_2_1_3_14,  LEN_INT, TYP_INT,     STS_RO},     /* ifType */
    {snmp_mib_2_2_1_4_14,  LEN_INT, TYP_INT,     STS_RO},     /* ifMtu */
    {snmp_mib_2_2_1_5_14,  LEN_INT, TYP_GAUGE,   STS_RO},     /* ifSpeed */
    {snmp_mib_2_2_1_6_14,  LEN_MAC, TYP_OCT_STR, STS_RO},     /* ifPhysAddress */
    {snmp_mib_2_2_1_7_14,  LEN_INT, TYP_INT,     STS_RO},     /* ifAdminStatus */
    {snmp_mib_2_2_1_8_14,  LEN_INT, TYP_INT,     STS_RO},     /* ifOperStatus */
    {snmp_mib_2_2_1_9_14,  LEN_INT, TYP_TIM_TIC, STS_RO},     /* ifLastChanges */
    {snmp_mib_2_2_1_10_14, LEN_INT, TYP_CNT,     STS_RO},     /* ifInOctets */
    {snmp_mib_2_2_1_11_14, LEN_INT, TYP_CNT,     STS_RO},     /* ifInUcastPkts */
    {snmp_mib_2_2_1_12_14, LEN_INT, TYP_CNT,     STS_RO},     /* ifInNUcastPkts */
    {snmp_mib_2_2_1_13_14, LEN_INT, TYP_CNT,     STS_RO},     /* ifInDiscards */
    {snmp_mib_2_2_1_14_14, LEN_INT, TYP_CNT,     STS_RO},     /* ifInErrors */
    {snmp_mib_2_2_1_15_14, LEN_INT, TYP_CNT,     STS_RO},     /* ifInUnknownProtos */
    {snmp_mib_2_2_1_16_14, LEN_INT, TYP_CNT,     STS_RO},     /* ifOutOctets */
    {snmp_mib_2_2_1_17_14, LEN_INT, TYP_CNT,     STS_RO},     /* ifOutUcastPkts */
    {snmp_mib_2_2_1_18_14, LEN_INT, TYP_CNT,     STS_RO},     /* ifOutNUcastPkts */
    {snmp_mib_2_2_1_19_14, LEN_INT, TYP_CNT,     STS_RO},     /* ifOutDiscards */
    {snmp_mib_2_2_1_20_14, LEN_INT, TYP_CNT,     STS_RO},     /* ifOutErrors */
    {snmp_mib_2_2_1_21_14, LEN_INT, TYP_GAUGE,   STS_RO},     /* ifOutQLen */
    {snmp_mib_2_2_1_22_14, 0,       TYP_OBJ_ID,  STS_RO},     /* ifSpecific */
    #endif
    #if (CFG_SNMP_NET_DEV_CNT >= 15)
    {snmp_mib_2_2_1_1_15,  LEN_INT, TYP_INT,     STS_RO},     /* ifIndex */
    {snmp_mib_2_2_1_2_15,  0,       TYP_OCT_STR, STS_RO},     /* ifDescr */
    {snmp_mib_2_2_1_3_15,  LEN_INT, TYP_INT,     STS_RO},     /* ifType */
    {snmp_mib_2_2_1_4_15,  LEN_INT, TYP_INT,     STS_RO},     /* ifMtu */
    {snmp_mib_2_2_1_5_15,  LEN_INT, TYP_GAUGE,   STS_RO},     /* ifSpeed */
    {snmp_mib_2_2_1_6_15,  LEN_MAC, TYP_OCT_STR, STS_RO},     /* ifPhysAddress */
    {snmp_mib_2_2_1_7_15,  LEN_INT, TYP_INT,     STS_RO},     /* ifAdminStatus */
    {snmp_mib_2_2_1_8_15,  LEN_INT, TYP_INT,     STS_RO},     /* ifOperStatus */
    {snmp_mib_2_2_1_9_15,  LEN_INT, TYP_TIM_TIC, STS_RO},     /* ifLastChanges */
    {snmp_mib_2_2_1_10_15, LEN_INT, TYP_CNT,     STS_RO},     /* ifInOctets */
    {snmp_mib_2_2_1_11_15, LEN_INT, TYP_CNT,     STS_RO},     /* ifInUcastPkts */
    {snmp_mib_2_2_1_12_15, LEN_INT, TYP_CNT,     STS_RO},     /* ifInNUcastPkts */
    {snmp_mib_2_2_1_13_15, LEN_INT, TYP_CNT,     STS_RO},     /* ifInDiscards */
    {snmp_mib_2_2_1_14_15, LEN_INT, TYP_CNT,     STS_RO},     /* ifInErrors */
    {snmp_mib_2_2_1_15_15, LEN_INT, TYP_CNT,     STS_RO},     /* ifInUnknownProtos */
    {snmp_mib_2_2_1_16_15, LEN_INT, TYP_CNT,     STS_RO},     /* ifOutOctets */
    {snmp_mib_2_2_1_17_15, LEN_INT, TYP_CNT,     STS_RO},     /* ifOutUcastPkts */
    {snmp_mib_2_2_1_18_15, LEN_INT, TYP_CNT,     STS_RO},     /* ifOutNUcastPkts */
    {snmp_mib_2_2_1_19_15, LEN_INT, TYP_CNT,     STS_RO},     /* ifOutDiscards */
    {snmp_mib_2_2_1_20_15, LEN_INT, TYP_CNT,     STS_RO},     /* ifOutErrors */
    {snmp_mib_2_2_1_21_15, LEN_INT, TYP_GAUGE,   STS_RO},     /* ifOutQLen */
    {snmp_mib_2_2_1_22_15, 0,       TYP_OBJ_ID,  STS_RO},     /* ifSpecific */
    #endif
    #if (CFG_SNMP_NET_DEV_CNT >= 16)
    {snmp_mib_2_2_1_1_16,  LEN_INT, TYP_INT,     STS_RO},     /* ifIndex */
    {snmp_mib_2_2_1_2_16,  0,       TYP_OCT_STR, STS_RO},     /* ifDescr */
    {snmp_mib_2_2_1_3_16,  LEN_INT, TYP_INT,     STS_RO},     /* ifType */
    {snmp_mib_2_2_1_4_16,  LEN_INT, TYP_INT,     STS_RO},     /* ifMtu */
    {snmp_mib_2_2_1_5_16,  LEN_INT, TYP_GAUGE,   STS_RO},     /* ifSpeed */
    {snmp_mib_2_2_1_6_16,  LEN_MAC, TYP_OCT_STR, STS_RO},     /* ifPhysAddress */
    {snmp_mib_2_2_1_7_16,  LEN_INT, TYP_INT,     STS_RO},     /* ifAdminStatus */
    {snmp_mib_2_2_1_8_16,  LEN_INT, TYP_INT,     STS_RO},     /* ifOperStatus */
    {snmp_mib_2_2_1_9_16,  LEN_INT, TYP_TIM_TIC, STS_RO},     /* ifLastChanges */
    {snmp_mib_2_2_1_10_16, LEN_INT, TYP_CNT,     STS_RO},     /* ifInOctets */
    {snmp_mib_2_2_1_11_16, LEN_INT, TYP_CNT,     STS_RO},     /* ifInUcastPkts */
    {snmp_mib_2_2_1_12_16, LEN_INT, TYP_CNT,     STS_RO},     /* ifInNUcastPkts */
    {snmp_mib_2_2_1_13_16, LEN_INT, TYP_CNT,     STS_RO},     /* ifInDiscards */
    {snmp_mib_2_2_1_14_16, LEN_INT, TYP_CNT,     STS_RO},     /* ifInErrors */
    {snmp_mib_2_2_1_15_16, LEN_INT, TYP_CNT,     STS_RO},     /* ifInUnknownProtos */
    {snmp_mib_2_2_1_16_16, LEN_INT, TYP_CNT,     STS_RO},     /* ifOutOctets */
    {snmp_mib_2_2_1_17_16, LEN_INT, TYP_CNT,     STS_RO},     /* ifOutUcastPkts */
    {snmp_mib_2_2_1_18_16, LEN_INT, TYP_CNT,     STS_RO},     /* ifOutNUcastPkts */
    {snmp_mib_2_2_1_19_16, LEN_INT, TYP_CNT,     STS_RO},     /* ifOutDiscards */
    {snmp_mib_2_2_1_20_16, LEN_INT, TYP_CNT,     STS_RO},     /* ifOutErrors */
    {snmp_mib_2_2_1_21_16, LEN_INT, TYP_GAUGE,   STS_RO},     /* ifOutQLen */
    {snmp_mib_2_2_1_22_16, 0,       TYP_OBJ_ID,  STS_RO},     /* ifSpecific */
    #endif
    #endif

    {0, 0, 0, 0}
};

/* MIB data */
/* System group */
T_SNMP_MIB_DAT snmp_mib_dat_sys[] = {
    (VP)snmp_mib_sys_desc,          /* sysDescr */
    (VP)snmp_mib_sys_obj_id,        /* sysObjectID */
    (VP)0,                          /* sysUpTime */
    (VP)snmp_mib_sys_contact,       /* sysContact */
    (VP)snmp_mib_sys_name,          /* sysName */
    (VP)snmp_mib_sys_location,      /* sysLocation */
    (VP)MIB_SYS_SERVICES            /* sysServices */
};

/* Interfaces group */
#if (CFG_SNMP_MIB2_IF_ENA == 1)
T_SNMP_MIB_DAT snmp_mib_dat_itf[1 + (3 * CFG_SNMP_NET_DEV_CNT)] = {
    (VP)CFG_SNMP_NET_DEV_CNT,       /* ifNumber */
    (VP)1,                          /* ifIndex */
    (VP)0,                          /* ifLastChanges */
    (VP)snmp_mib_if_spec,           /* ifSpecific */
    #if (CFG_SNMP_NET_DEV_CNT >= 2)
    (VP)2,                          /* ifIndex */
    (VP)0,                          /* ifLastChanges */
    (VP)snmp_mib_if_spec,           /* ifSpecific */
    #endif
    #if (CFG_SNMP_NET_DEV_CNT >= 3)
    (VP)3,                          /* ifIndex */
    (VP)0,                          /* ifLastChanges */
    (VP)snmp_mib_if_spec,           /* ifSpecific */
    #endif
    #if (CFG_SNMP_NET_DEV_CNT >= 4)
    (VP)4,                          /* ifIndex */
    (VP)0,                          /* ifLastChanges */
    (VP)snmp_mib_if_spec,           /* ifSpecific */
    #endif
    #if (CFG_SNMP_NET_DEV_CNT >= 5)
    (VP)5,                          /* ifIndex */
    (VP)0,                          /* ifLastChanges */
    (VP)snmp_mib_if_spec,           /* ifSpecific */
    #endif
    #if (CFG_SNMP_NET_DEV_CNT >= 6)
    (VP)6,                          /* ifIndex */
    (VP)0,                          /* ifLastChanges */
    (VP)snmp_mib_if_spec,           /* ifSpecific */
    #endif
    #if (CFG_SNMP_NET_DEV_CNT >= 7)
    (VP)7,                          /* ifIndex */
    (VP)0,                          /* ifLastChanges */
    (VP)snmp_mib_if_spec,           /* ifSpecific */
    #endif
    #if (CFG_SNMP_NET_DEV_CNT >= 8)
    (VP)8,                          /* ifIndex */
    (VP)0,                          /* ifLastChanges */
    (VP)snmp_mib_if_spec,           /* ifSpecific */
    #endif
    #if (CFG_SNMP_NET_DEV_CNT >= 9)
    (VP)9,                          /* ifIndex */
    (VP)0,                          /* ifLastChanges */
    (VP)snmp_mib_if_spec,           /* ifSpecific */
    #endif
    #if (CFG_SNMP_NET_DEV_CNT >= 10)
    (VP)10,                         /* ifIndex */
    (VP)0,                          /* ifLastChanges */
    (VP)snmp_mib_if_spec,           /* ifSpecific */
    #endif
    #if (CFG_SNMP_NET_DEV_CNT >= 11)
    (VP)11,                         /* ifIndex */
    (VP)0,                          /* ifLastChanges */
    (VP)snmp_mib_if_spec,           /* ifSpecific */
    #endif
    #if (CFG_SNMP_NET_DEV_CNT >= 12)
    (VP)12,                         /* ifIndex */
    (VP)0,                          /* ifLastChanges */
    (VP)snmp_mib_if_spec,           /* ifSpecific */
    #endif
    #if (CFG_SNMP_NET_DEV_CNT >= 13)
    (VP)13,                         /* ifIndex */
    (VP)0,                          /* ifLastChanges */
    (VP)snmp_mib_if_spec,           /* ifSpecific */
    #endif
    #if (CFG_SNMP_NET_DEV_CNT >= 14)
    (VP)14,                         /* ifIndex */
    (VP)0,                          /* ifLastChanges */
    (VP)snmp_mib_if_spec,           /* ifSpecific */
    #endif
    #if (CFG_SNMP_NET_DEV_CNT >= 15)
    (VP)15,                         /* ifIndex */
    (VP)0,                          /* ifLastChanges */
    (VP)snmp_mib_if_spec,           /* ifSpecific */
    #endif
    #if (CFG_SNMP_NET_DEV_CNT >= 16)
    (VP)16,                         /* ifIndex */
    (VP)0,                          /* ifLastChanges */
    (VP)snmp_mib_if_spec,           /* ifSpecific */
    #endif
};
#endif

/* SNMP group */
#if (CFG_SNMP_MIB2_SNMP_ENA == 1)
T_SNMP_MIB_DAT snmp_mib_dat_snmp[SNMP_MIB2_OBJ_CNT_SNMP] = {
    (VP)0,                          /* snmpInPkts */
    (VP)0,                          /* snmpOutPkts */
    (VP)0,                          /* snmpInBadVersions */
    (VP)0,                          /* snmpInBadCommunityNames */
    (VP)0,                          /* snmpInBadCommunityUses */
    (VP)0,                          /* snmpInASNParseErrs */
    (VP)0,                          /* snmpInTooBigs */
    (VP)0,                          /* snmpInNoSuchNames */
    (VP)0,                          /* snmpInBadValues */
    (VP)0,                          /* snmpInReadOnlys */
    (VP)0,                          /* snmpInGenErrs */
    (VP)0,                          /* snmpInTotalReqVars */
    (VP)0,                          /* snmpInTotalSetVars */
    (VP)0,                          /* snmpInGetRequests */
    (VP)0,                          /* snmpInGetNexts */
    (VP)0,                          /* snmpInSetRequests */
    (VP)0,                          /* snmpInGetResponses */
    (VP)0,                          /* snmpInTraps */
    (VP)0,                          /* snmpOutTooBigs */
    (VP)0,                          /* snmpOutNoSuchNames */
    (VP)0,                          /* snmpOutBadValues */
    (VP)0,                          /* snmpOutGetErrs */
    (VP)0,                          /* snmpOutGetRequests */
    (VP)0,                          /* snmpOutGetNexts */
    (VP)0,                          /* snmpOutSetRequests */
    (VP)0,                          /* snmpOutGetResponses */
    (VP)0,                          /* snmpOutTraps */
    (VP)0,                          /* snmpEnableAuthenTraps */
};
#endif

/* Number of MIB objects */
const UH snmp_mib_dat_cnt[SNMP_MIB2_CNT_GRP] = {
    SNMP_MIB2_DEF_CNT_SYS,
    SNMP_MIB2_OBJ_CNT_AT,
    SNMP_MIB2_OBJ_CNT_IP,
    SNMP_MIB2_OBJ_CNT_ICMP,
    SNMP_MIB2_OBJ_CNT_TCP,
    SNMP_MIB2_OBJ_CNT_UDP,
    SNMP_MIB2_OBJ_CNT_SNMP,
    SNMP_MIB2_OBJ_CNT_ITF
};

/* MIB data block */
T_SNMP_MIB_DAT* snmp_mib_dat[3] = {
    snmp_mib_dat_sys,    /* System */
    #if (CFG_SNMP_MIB2_IF_ENA == 1)
    snmp_mib_dat_itf,    /* Interfaces group */
    #else
    0x00,
    #endif
    #if (CFG_SNMP_MIB2_SNMP_ENA == 1)
    snmp_mib_dat_snmp    /* SNMP group */
    #else
    0x00
    #endif
};

