/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created
            2017.01.26: Fixed the IPA warnings.
            2017.07.19: Deleted TMO cast when calling _kernel_enqtim.
 ***************************************************************************/

#include "uC3sys.h"
#include <string.h>

extern UW _cside_agent_init(void);
extern void _cside_agent(void);

static BOOL _kernel_cside_agent_1(void);

/***************************************************
    Trace contex for CSIDE
 ***************************************************/

static BOOL _kernel_cside_agent_1(void)
{
    _kernel_agentfunc(&_cside_agent);
    _kernel_enqtim((T_TCB *)_kernel_systbl.agentmng, &_kernel_systbl.systim, _kernel_systbl.agenttim);
    return FALSE;
}

void _kernel_cside_agent(void)
{
    T_CYC *cyc;

    cyc = (T_CYC *)_kernel_getmem(&_kernel_systbl.free_sys, sizeof(T_CYC));
    if (cyc == 0) {
        ;
    } else {
        memset((void *)cyc, 0x00, sizeof(T_CYC));
        cyc->stat.catr = TA_CYC;
        cyc->sysfunc = (FP)&_kernel_cside_agent_1;
        _kernel_systbl.agentmng = cyc;
        _kernel_systbl.agenttim = _cside_agent_init();

        if (_kernel_systbl.agenttim != 0UL) {
            _kernel_enqtim((T_TCB *)cyc, &_kernel_systbl.systim, _kernel_systbl.agenttim);
        }
    }
}
