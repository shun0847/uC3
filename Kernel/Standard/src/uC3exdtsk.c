/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created
            2017.01.26: Fixed the IPA warnings.
 ***************************************************************************/

#include "uC3sys.h"


static BOOL _kernel_exdtsk_1(T_SSB *par, BOOL retcd);

/***************************************
    Terminate and Delete Invoking Task
 ***************************************/

static BOOL _kernel_exdtsk_1(T_SSB *par, BOOL retcd)
{
    void (*umtxfunc)(T_TCB *);
    T_TCB *tcb;
    T_MEM **mem_base;

    tcb = (T_TCB *)par->p1;
    umtxfunc = (void (*)(T_TCB *))_kernel_systbl.umtxfunc;
    if (umtxfunc != 0) {
        umtxfunc(tcb);
    }

    _kernel_deqtcb(&tcb->wtcb);
    _kernel_systbl.qtcb.tcb[tcb->tskid] = 0;
    _kernel_systbl.qtcb.inf->usedc--;

    if ((tcb->stat.oatr & TA_USTK) == 0U) {
        if (_kernel_systbl.free_stk != (T_MEM *)1) {
            mem_base = &_kernel_systbl.free_stk;
        } else {
            mem_base = &_kernel_systbl.free_sys;
        }
        (void)_kernel_relmem(mem_base, (T_MEM *)tcb->stk, tcb->stksz);
    }
    (void)_kernel_relmem(&_kernel_systbl.free_sys, (T_MEM *)tcb, sizeof(T_TCB));
    _kernel_systbl.stat.dspint = 0UL;
    _kernel_systbl.ctcb = 0;
    (void)_kernel_setims((IMASK)0);
    return retcd;
}

void exd_tsk(void)
{
    T_TCB *tcb;

    if ((_kernel_systbl.icnt == 0U) && (_kernel_systbl.cssb == 0)) {
        tcb = _kernel_systbl.ctcb;
        tcb->p1 = (VP_INT)tcb;
        tcb->sysfunc = (FP)&_kernel_exdtsk_1;
        _kernel_entsys(FALSE);
    }
}
