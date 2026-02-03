/***************************************************************************
    Network Sample Application Configuration
    Copyright (c)  2014, eForce Co., Ltd. All rights reserved.

    2014-08-14: Created.
 ***************************************************************************/
#ifndef _SAMPLE_NETAPP_CFG_H
#define _SAMPLE_NETAPP_CFG_H

#include "net_hdr.h"
#include "net_strlib.h"
#include "shell.h"

#ifndef NULL
#define NULL    ((void*)0)
#endif

/* load sample settings */
#include "sample_use.h"

#ifndef SAMPLE_USE_GENSRC
#ifdef NET_C
#define SAMPLE_USE_GENSRC       1
#else
#define SAMPLE_USE_GENSRC       0
#endif
#endif

#ifndef SAMPLE_SOCDEV_CLI
#define SAMPLE_SOCDEV_CLI       1       /* Device number used for client socket */
#endif

#ifndef SAMPLE_SOCDEV_SER
#define SAMPLE_SOCDEV_SER       1       /* Device number used for server socket */
#endif

#if (_kernel_SIZE_SIZE==8)
#define EXEC_CPU_64BIT      1
#else
#define EXEC_CPU_64BIT      0
#endif

extern ER dns_get_ipa_opt(UW dnsd_ipa, VB *str, UW *ipa);

/*****************************************************************************
    Socket ID & Kernel ID
*****************************************************************************/
#if SAMPLE_USE_GENSRC
#include "kernel_id.h"
#include "net_id.h"

#else
extern SID ID_SOC_DNS;
#define ID_SOC_DNSC     ID_SOC_DNS
#endif


/*****************************************************************************
    Sample Application Settings
*****************************************************************************/
#define SPL_LF              "\r\n"          /* Line feed code */

/* Default DNS server */
extern UW dns_svr_ip;
#define SPL_DNS_SERVER      dns_svr_ip


#endif /* _SAMPLE_NETAPP_CFG_H */
