/***************************************************************************
    MICRO C CUBE / NETWORK
    Static configuration header
    Copyright (c)  2014-2021, eForce Co., Ltd. All rights reserved.

    Version Information
      2014.03.31  Created.
      2014.07.31  Defined NET3_VER
      2015.03.20  BSD socket to support uNet3/Compact.
      2015.11.26  Removed EXT_ALOC_SUP for POSIX_API_SUP
      2016.03.09  Add version definition for H/W OS
      2016.03.19  IGMP Version 3 support
      2016.05.12  Add include file for H/W OS
      2016.06.08: Add SOC_RCV_PKT_SIZE option support
      2016.06.09: Add SOC_MSG_PEEK option support
      2016.06.16: Add SOC_REUSE_ADDR option support
      2020.10.02: Add uC3/SMP TKERNEL_PRID
      2021.03.17: Add ARP_API_SUP option support
 **************************************************************************/

#ifndef NETSUP_H
#define NETSUP_H
#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************/
/* CPU Architecture dependent Macros                                      */
/**************************************************************************/

#ifdef TKERNEL_PRID     /* Kernel éØï î‘çÜ */
#if ((TKERNEL_PRID & 0x0F00) == 0x0100)
#define NET_C_OS        /* Using Compact Kernel */
#elif (((TKERNEL_PRID & 0x0F00) == 0x0200)  || \
       ((TKERNEL_PRID & 0x0F00) == 0x0300))
#define NET_S_OS        /* Using Standard Kernel */
#endif
#else
#include "net_hwos.h"
#endif

/* Define endian, if not already defined else where */
#if !defined(_UC3_ENDIAN_BIG) && !defined(_UC3_ENDIAN_LITTLE)
#if defined (__CC_ARM)     /* for ARM Compiler */
#if !defined (__LITTLE_ENDIAN)
#define _UC3_ENDIAN_LITTLE
#endif
#endif
#if defined (__ICCARM__)  /* for IAR Compiler */
#if (__LITTLE_ENDIAN__ == 1)
#define _UC3_ENDIAN_LITTLE
#endif
#endif
#if defined (__GNUC__)    /* for GCC Compiler */
#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#define _UC3_ENDIAN_LITTLE
#endif
#endif
#if defined (__TMS470__)  /* for CCS Compiler */
#if defined (__little_endian__)
#define _UC3_ENDIAN_LITTLE
#endif
#endif
#ifdef _SH                /* for SuperH */
#ifdef _LIT
#define _UC3_ENDIAN_LITTLE
#endif
#endif
#if defined(__GCC_NDS32)  /* for ANDES GCC */
#define _UC3_ENDIAN_LITTLE
#endif
#endif

/**************************************************************************/
/* Network Architecture dependent Macros                                  */
/**************************************************************************/

#define NET3_VER        3

#ifdef NET_C_OS
#define NET_C           /* Using Network Configurator */
#endif

#ifdef NET_C
#define MAC_RESOLVE     /* MAC Resolver */
#ifdef MAC_RESOLVE
#define UDP_MAC_PORT    5000
#endif
#endif

#ifndef UNDEF_TCP
#define TCP_SUP         /* TCP Enabled */
#endif
#ifndef UNDEF_UDP
#define UDP_SUP         /* UDP Enabled */
#endif
#ifndef UNDEF_IPR
#define IPR_SUP         /* IP Reassembly Enabled */
#endif
#ifndef UNDEF_PING
#define PING_SUP        /* ICMP API Enabled */
#endif

#ifdef UDP_SUP
#ifndef UNDEF_MCAST
#define MCAST_SUP       /* IGMP Enabled */
/* #define IGMPv3_SUP */
#endif
#endif

#ifdef TCP_SUP
#ifndef UNDEF_KEEPALIVE
#define KEEPALIVE_SUP
#endif
#endif

#ifndef UNDEF_ACD
#define ACD_SUP
#endif

#ifndef UNDEF_STS       /* Network status Enabled */
#define STS_SUP
#endif

#ifndef UNDEF_ARP_API
#define ARP_API_SUP
#endif

#ifdef POSIX_API_SUP
#define IO_READY_SUP
#define LO_IF_SUP
#ifdef  MCAST_SUP
    #define MCAST_SOC_SUP   /* spec for multicast per socket */
#endif
#define MSG_PEEK_SUP
#define RCV_PKT_SIZE_SUP
#define REUSE_ADDR_SUP
#endif


#ifdef __cplusplus
}
#endif
#endif /* NETSUP_H */
