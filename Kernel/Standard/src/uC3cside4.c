/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2018, eForce Co.,Ltd.  All rights reserved.

    Version Information
            2008.06.19: Created.
            2010.12.29: Added the trace function of memory-block.
            2018.05.29: change pc parameter type from UW to ADDR.
 ***************************************************************************/

#define _UC3CSIDE4_C_

#include "uC3sys.h"

extern void _cside_init(void);
extern void _cside_swctx0(UW pre_ctxid, UW next_ctxid, UW time, ADDR pc);
extern void _cside_swctx1(UW pre_ctxid, UW next_ctxid, UW time, ADDR pc);
extern void _cside_swctx2(UW pre_ctxid, UW next_ctxid, UW time, ADDR pc);
extern void _cside_swctx3(UW pre_ctxid, UW next_ctxid, UW time, ADDR pc);

/***************************************************
    Trace contex and system call for CSIDE
 ***************************************************/

void _kernel_cside_tarce_4(void)
{
    _kernel_systbl.ctxtrace0 = (FP)_cside_swctx0;
    _kernel_systbl.ctxtrace1 = (FP)_cside_swctx1;
    _kernel_systbl.ctxtrace2 = (FP)_cside_swctx2;
    _kernel_systbl.ctxtrace3 = (FP)_cside_swctx3;
    _kernel_systbl.ctxtrace4 = (FP)0;
    _kernel_systbl.systrace = (FP)0;
    _kernel_systbl.rettrace = (FP)0;
    _cside_init();
}
