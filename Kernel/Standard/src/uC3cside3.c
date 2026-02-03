/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2018, eForce Co.,Ltd.  All rights reserved.

    Version Information
            2008.06.19: Created.
            2010.12.29: Added the trace function of memory-block.
            2018.05.29: change pc parameter type from UW to ADDR.
 ***************************************************************************/

#define _UC3CSIDE3_C_

#include "uC3sys.h"

extern void _cside_init(void);
extern void _cside_swctx10(UW pre_ctxid, UW next_ctxid, UW time, ADDR pc);
extern void _cside_swctx11(UW pre_ctxid, UW next_ctxid, UW time, ADDR pc);
extern void _cside_swctx12(UW pre_ctxid, UW next_ctxid, UW time, ADDR pc);
extern void _cside_swctx13(UW pre_ctxid, UW next_ctxid, UW time, ADDR pc);
extern void _cside_syscall(UW cur_ctxid, UW func, UW time, ADDR pc, UH count, UW *para);
extern void _cside_sysret(UW cur_ctxid, UW func, UW time, ADDR pc, UW ercd);

/***************************************************
    Trace contex and system call for CSIDE
 ***************************************************/

void _kernel_cside_tarce_3(void)
{
    _kernel_systbl.ctxtrace0 = (FP)_cside_swctx10;
    _kernel_systbl.ctxtrace1 = (FP)_cside_swctx11;
    _kernel_systbl.ctxtrace2 = (FP)_cside_swctx12;
    _kernel_systbl.ctxtrace3 = (FP)_cside_swctx13;
    _kernel_systbl.ctxtrace4 = (FP)0;
    _kernel_systbl.systrace = (FP)_cside_syscall;
    _kernel_systbl.rettrace = (FP)_cside_sysret;
    _cside_init();
}
