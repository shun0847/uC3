/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2018, eForce Co.,Ltd.  All rights reserved.

    Version Information
            2008.06.19: Created.
            2010.12.29: Added the trace function of memory-block.
            2018.05.29: change pc parameter type from UW to ADDR.
 ***************************************************************************/

#define _UC3CSIDE2_C_

#include "uC3sys.h"

extern void _cside_init(void);
extern void _cside_syscall(UW cur_ctxid, UW func, UW time, ADDR pc, UH count, UW *para);
extern void _cside_sysret(UW cur_ctxid, UW func, UW time, ADDR pc, UW ercd);

/***************************************************
    Trace system call for CSIDE
 ***************************************************/

void _kernel_cside_tarce_2(void)
{
    _kernel_systbl.ctxtrace0 = (FP)0;
    _kernel_systbl.ctxtrace1 = (FP)0;
    _kernel_systbl.ctxtrace2 = (FP)0;
    _kernel_systbl.ctxtrace3 = (FP)0;
    _kernel_systbl.ctxtrace4 = (FP)0;
    _kernel_systbl.systrace = (FP)_cside_syscall;
    _kernel_systbl.rettrace = (FP)_cside_sysret;
    _cside_init();
}
