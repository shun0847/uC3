/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    Ping Client header file
    Copyright (c)  2012-2016, eForce Co., Ltd. All rights reserved.

    Version Information
      2012.06.07: Created
      2015.12.14: The socket ID replaced SID types
      2016.07.06: Execute static analysis tool to this source.
 ***************************************************************************/

#ifndef PING_CLIENT_H
#define PING_CLIENT_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "kernel.h"
#include "net_hdr.h"

#define PING_LEN_MAX    1472U
#define PING_LEN_MIN      32U

#define PING6_LEN_MAX   1452U
#define PING6_LEN_MIN     32U
    
#define PING_TIMEOUT    3000

typedef struct {
    SID sid;
    UW ipa;
    TMO tmo;
    UH devnum;
    UH len;

}T_PING_CLIENT;

#ifdef IPV6_SUP
typedef struct {
    SID sid;
    UW *ip6addr;
    TMO tmo;
    UH devnum;
    UH len;

}T_PING_CLIENT_V6;
#endif

ER ping_client(T_PING_CLIENT *pc);
#ifdef IPV6_SUP
ER ping6_client(T_PING_CLIENT_V6 *pc);
#endif

#ifdef __cplusplus
}
#endif
#endif
