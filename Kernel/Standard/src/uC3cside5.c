/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2018, eForce Co.,Ltd.  All rights reserved.

    Version Information
            2008.06.19: Created.
            2010.12.29: Added the trace function of memory-block.
            2018.05.29: change pc parameter type from UW to ADDR.
            2018.06.04: change func type to FN, ercd type to ER.
 ***************************************************************************/

#define _UC3CSIDE5_C_

#include "uC3sys.h"

extern void _cside_init(void);
extern void _cside_swctx0(UW pre_ctxid, UW next_ctxid, UW time, ADDR pc);
extern void _cside_swctx1(UW pre_ctxid, UW next_ctxid, UW time, ADDR pc);
extern void _cside_swctx2(UW pre_ctxid, UW next_ctxid, UW time, ADDR pc);
extern void _cside_swctx3(UW pre_ctxid, UW next_ctxid, UW time, ADDR pc);
extern void _cside_memblk(UW cur_ctxid, FN func, UW time, ADDR pc, ID objid, ADDR addr, SIZE size);
extern void _cside_syscall(UW cur_ctxid, FN func, UW time, ADDR pc, UH count, UW *para);
extern void _cside_sysret(UW cur_ctxid, FN func, UW time, ADDR pc, ER ercd);

/***************************************************
    Trace contex and system call for CSIDE
 ***************************************************/

void _kernel_cside_tarce_5(void)
{
    _kernel_systbl.ctxtrace0 = (FP)_cside_swctx0;
    _kernel_systbl.ctxtrace1 = (FP)_cside_swctx1;
    _kernel_systbl.ctxtrace2 = (FP)_cside_swctx2;
    _kernel_systbl.ctxtrace3 = (FP)_cside_swctx3;
    _kernel_systbl.ctxtrace4 = (FP)_cside_memblk;
    _kernel_systbl.systrace = (FP)_cside_syscall;
    _kernel_systbl.rettrace = (FP)_cside_sysret;
    _cside_init();
}
