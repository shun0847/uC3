/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created
            2009.10.22: Corrected the error check.
            2017.01.26: Fixed the IPA warnings.
            2017.07.21: Deleted TMO cast when calling _kernel_enqtim.
 ***************************************************************************/

#include "uC3sys.h"


static BOOL _kernel_dlytsk_1(T_SSB *par, BOOL retcd);

/***********************************
    Delay Task
 ***********************************/

static BOOL _kernel_dlytsk_1(T_SSB *par, BOOL retcd)
{
    RELTIM dlytim;
    T_TCB *tcb;

    if (_kernel_systbl.stat.dspint != 0UL) {
        par->p1 = (VP_INT)E_CTX;
    } else {
        dlytim = (RELTIM)par->p1 + (RELTIM)_kernel_systbl.tick;
        tcb = (T_TCB *)par->p2;
        par->p1 = (VP_INT)E_OK;
        _kernel_deqtcb(&tcb->wtcb);
        tcb->stat.msts = TTS_WAI | TTS_TMR;
        tcb->stat.wsts = TS_DLY;
        _kernel_enqtim(tcb, &_kernel_systbl.systim, dlytim);
        _kernel_systbl.ctcb = 0;
    }
    return retcd;
}

ER dly_tsk(RELTIM dlytim)
{
    T_TCB *tcb;
    ER ercd;

    if ((_kernel_systbl.icnt != 0U) ||
        (_kernel_systbl.cssb != 0)) {
        ercd = E_CTX;
    } else {
        tcb = _kernel_systbl.ctcb;
        tcb->p1 = (VP_INT)dlytim;
        tcb->p2 = (VP_INT)tcb;
        tcb->sysfunc = (FP)&_kernel_dlytsk_1;
        _kernel_entsys(FALSE);
        ercd =  (ER)tcb->p1;
    }
    return ercd;
}
