/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    Sample DHCP client Task header file
    Copyright (c)  2016, eForce Co., Ltd. All rights reserved.

    Version Information
      2016.02.20: Created.
 ***************************************************************************/

#ifndef SAMPLE_TSK_DHCPC_H
#define SAMPLE_TSK_DHCPC_H
#ifdef __cplusplus
extern "C" {
#endif

#include "kernel.h"
#include "dhcp_client.h"
#include "net_cfg.h"

typedef struct t_dhcpc_tsk_info {
    T_DHCP_CLIENT   dc;
    
    UW      interval;
    void (*set_cbk)(ER,T_DHCP_CLIENT*);
    ID      tid;
    ID      flgid;
} T_DHCPC_TSK_INFO;

ER dhcpc_tsk(T_DHCPC_TSK_INFO *sc_inf);
ER dhcpc_tsk_renew(T_DHCPC_TSK_INFO *dc_inf);
ER dhcpc_tsk_stop(T_DHCPC_TSK_INFO *dc_inf);


#ifdef __cplusplus
}
#endif
#endif

