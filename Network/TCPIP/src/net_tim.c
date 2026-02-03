/***************************************************************************
    MICRO C CUBE / COMPACT, NETWORK
    Protocol stack timer task
    Copyright (c)  2008-2024, eForce Co., Ltd. All rights reserved.

    Version Information
      2008.11.19: Created
      2010.06.15: Updated for IPv6 support
      2010.07.30: Updated for IPv6 Timer support
      2010.11.16: Modified TCP time handling
      2012.10.02  Modify to avoid use of string libraries.
      2013.08.16: Avoid implementation-dependent for kernel
                  (dly_tsk() -> tslp_tsk())
      2014.03.06: SNMP option was added
      2014.03.25: Corrected the judgment of timer processing
      2014.11.04: Add get_net_sec() function
      2014.11.26: Add ip fowarding function
      2016.03.19  Add IGMPv3_SUP
      2023.02.08: Suppressed the warning for defined UNDEF_STS.
      2024.03.22: Disabled processing net_sts_upd() with CFG_STS_UPD_RES(0).
 ***************************************************************************/

#include <stdio.h>
#include "kernel.h"
#include "net_hdr.h"
#include "net_def.h"

extern UW NET_STS_RES;

UB IPR_TIMER_ON;
UB IGMP_TIMER_ON;
UB IGMPV1_TIMER_ON;
#ifdef IGMPv3_SUP
UB IGMPV3_TIMER_ON;
#endif
#ifdef IPV6_SUP
UB IPV6_TIMER_ON;
#endif

UW NET_TICK;
UW NET_SEC;

#define NET_TIM_RES     100     /*  100msec */

UW get_net_sec(void)
{
    UW sec;

    loc_tcp();
    sec = NET_SEC;
    ulc_tcp();
    return sec;
}

/*
    TCP/IP Timer (Task)
    This 100ms timer handles the various timer event of TCP/IP stack.
*/

void net_tim_tsk(VP_INT exinf)
{
    ER ercd;

    NET_TICK = NET_SEC = 0;

    for (;;) {

        ercd = tslp_tsk(NET_TIM_RES);
        if (ercd != E_TMOUT) {
            NET_ERR(printf("\r\n ERR: net_tsk tslp_tsk %x", ercd));
            tslp_tsk(1000);
            continue;
        }

        NET_TICK += NET_TIM_RES;

        loc_tcp();

        if (NET_TICK % 500 < NET_TIM_RES) {
            arp_timer(NET_TICK);
        }

#ifdef IPR_SUP
        if (IPR_TIMER_ON) {
            ipr_timer(NET_TICK);
        }
#endif

#ifdef MCAST_SUP
        if (IGMP_TIMER_ON) {
            igmp_timer(NET_TICK);
        }

        if (IGMPV1_TIMER_ON) {
            igmpv1_timer(NET_TICK);
        }
#ifdef IGMPv3_SUP
        if (IGMPV3_TIMER_ON) {
            igmpv3_timer(NET_TICK);
        }
#endif
#endif

#ifdef TCP_SUP
        if (NET_TICK % 200 < NET_TIM_RES) {
            tcp_ack_timer();
        }
        if (NET_TICK % 500 < NET_TIM_RES) {
            tcp_dat_timer();
        }
        if (NET_TICK % 1000 < NET_TIM_RES) {
            tcp_usr_timer();
        }
#endif

#ifdef IPV6_SUP
        if (IPV6_TIMER_ON) {
            /* IPv6 Timer 1 sec */
            if (NET_TICK % (1000/DEF_IPV6_TIM_RES) < NET_TIM_RES) {
                ip6_addr_timer();
                nd_timer();
            }
        }
#endif
#ifdef STS_SUP
        if (NET_STS_RES) {
            if (NET_TICK % NET_STS_RES < NET_TIM_RES) {
                net_sts_upd();
            }
        }
#endif
        if (NET_TICK % 1000 < NET_TIM_RES) {
            NET_SEC++;
            nat_timer(NET_SEC);
        }

        ulc_tcp();

    }
}

/*EOF*/
