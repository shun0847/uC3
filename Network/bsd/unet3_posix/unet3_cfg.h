/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK
    BSD socket wrapper/User configuration
    Copyright (c) 2014, eForce Co., Ltd. All rights reserved.

    Version Information
            2014.04.28: Created
            2014.07.25: Change definition of NUM_OF_TASK_ERRNO
            2015.08.25: Resource parameter is possible configuration
 ***************************************************************************/

#ifndef     _UNET_CFG_H
#define     _UNET_CFG_H

#include "net_cfg.h"

/* Build environment */
#if defined (__CC_ARM)     /* for ARM Compiler */
#define     BUILD_ARM
#elif defined (__ICCARM__)  /* for IAR Compiler */
#define     BUILD_IAR
#elif defined (__TMS470__)  /* for CCS Compiler */
#define     BUILD_CCS
#elif defined (__GNUC__)    /* for GCC Compiler */
#define     BUILD_GCC
#endif

#ifndef NULL
#define NULL    ((void*)0)
#endif

/* network stands on platform */
#ifndef SYS_PLATFORM
#define     SYS_PLATFORM        0 /* 0:stand alone 1:stand on specific system */
#endif

/* sockopt parameter variable length max */
#define     SOC_OPT_MAXLEN      32

/* Socket num */
#define     BSD_SOCKET_MAX      CFG_NET_SOC_MAX

/* Use Loopback Interface */
#define     LOOPBACK_IF_ENA     0  /* 0:Disable loopback I/F 1:Enable loopback I/F */

/* Number of tasks that refer to the errno */
#define     NUM_OF_TASK_ERRNO   (CFG_TASK_MAX+1)  /* must be 1 or more */

/* BSD task priority      */
#define     BSD_TASK_PRIORITY   4

#endif
