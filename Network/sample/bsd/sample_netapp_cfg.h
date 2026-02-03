/***************************************************************************
    Network Sample Application Configuration
    Copyright (c)  2014, eForce Co., Ltd. All rights reserved.

    2014-08-14: Created.
    2023-12-11: Add DNS server setting.
 ***************************************************************************/
#ifndef _SAMPLE_NETAPP_CFG_H
#define _SAMPLE_NETAPP_CFG_H

#include "net_hdr.h"
#include "net_strlib.h"
#include "shell.h"

#ifndef NULL
#define NULL    ((void*)0)
#endif

#ifndef SAMPLE_USE_GENSRC
#ifdef NET_C
#define SAMPLE_USE_GENSRC		1
#else
#define SAMPLE_USE_GENSRC		0
#endif
#endif

#if (_kernel_SIZE_SIZE==8)
#define EXEC_CPU_64BIT      1
#else
#define EXEC_CPU_64BIT      0
#endif

/*****************************************************************************
    Socket ID & Kernel ID
*****************************************************************************/
#if SAMPLE_USE_GENSRC
#include "kernel_id.h"
#include "net_id.h"
#endif

/*****************************************************************************
    Sample Application Settings
*****************************************************************************/
#define SPL_LF              "\r\n"          /* Line feed code */

/* Default DNS server */
extern UW dns_svr_ip;
#define SPL_DNS_SERVER      dns_svr_ip

#endif /* _SAMPLE_NETAPP_CFG_H */
