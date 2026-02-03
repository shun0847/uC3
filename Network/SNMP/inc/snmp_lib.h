/*
    SNMP
    Library configurations
    Copyright (c) 2014-2017, eForce Co., Ltd. All rights reserved.
    
    2014-03-06 Created
    2016-01-06 Memory and string libraries were added
    2016-03-22 Add external declarations for net_string API
    2016-03-23 Support for the R-IN32
    2016-04-29 Macro NET_HW_OS was added
    2017-04-20 Support OID type for private MIB data
*/

#ifndef SNMP_LIB_CFG_H
#define SNMP_LIB_CFG_H

/* Conversion macros for memory (net_hdr.h) */
#define snmp_memset    net_memset
#define snmp_memcpy    net_memcpy
#define snmp_memcmp    net_memcmp

/* Conversion macros for string */
#if !defined(NET_HW_OS)
#include "net_strlib.h"
#else
extern char* net_strcpy(char*, const char*);
extern SIZE net_strlen(const char*);
extern char* net_strcat(char*, const char*);
extern char* net_strncpy(char*, const char*, SIZE);
#endif
#define snmp_strcpy    net_strcpy
#define snmp_strlen    net_strlen
#define snmp_strcat    net_strcat
#define snmp_strncpy   net_strncpy

extern char *unet3snmp_get_version(unsigned char mode);
#define UNET3SNMP_VERSION  "1.2.7" /* for HW/OS*/

#endif

