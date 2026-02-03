/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    Sample SNTP client Task header file
    Copyright (c)  2016, eForce Co., Ltd. All rights reserved.

    Version Information
      2016.02.02: Created.
 ***************************************************************************/

#ifndef SAMPLE_TSK_SNTPC_H
#define SAMPLE_TSK_SNTPC_H
#ifdef __cplusplus
extern "C" {
#endif

#include "kernel.h"
#include "sntp_client.h"
#include "net_cfg.h"
#include "net_id.h"

typedef struct t_sntpc_tsk_info {
    T_SNTP_CLIENT   sc;
    
    UW      interval;
    VB      *domain;
    UW      dns_svr_ipa;
    void (*set_cbk)(ER,UW,UW);
    ID      tid;
    ID      flgid;
    UB      retry;
} T_SNTPC_TSK_INFO;

ER sntpc_tsk(T_SNTPC_TSK_INFO *sc_inf);
ER sntpc_tsk_sendnow(T_SNTPC_TSK_INFO *sc_inf);
ER sntpc_tsk_stop(T_SNTPC_TSK_INFO *sc_inf);


#ifdef __cplusplus
}
#endif
#endif

