/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created.
            2010.02.05: Corrected the checking of error code.
            2017.01.26: Fixed the IPA warnings.
            2017.07.19: Added the following checking code.
                         - If the tmout is -2 or less, an E_PAR error.
                         - In dispatch pending state, if TMO_POL is specified, an E_CTX error.
 ***************************************************************************/

#include "uC3sys.h"


static BOOL _kernel_slptsk_1(T_SSB *par, BOOL retcd);

/***********************************
    Put Task to Sleep
 ***********************************/

static BOOL _kernel_slptsk_1(T_SSB *par, BOOL retcd)
{
    TMO timout;
    T_TCB *tcb;

    timout = (TMO)par->p1;
    tcb = (T_TCB *)par->p2;
    par->p1 = (VP_INT)E_OK;

    if (_kernel_systbl.stat.dspint == 0UL) {
        if (timout == TMO_POL) {
            if (tcb->wup == 0U) {
                tcb->p1 = (VP_INT)E_TMOUT;
            } else {
                tcb->wup--;
            }
        } else {
            if (tcb->wup == 0U) {
                _kernel_deqtcb(&tcb->wtcb);
                tcb->stat.msts = TTS_WAI;
                tcb->stat.wsts = TS_SLP;
                if (timout != TMO_FEVR) {
                    _kernel_enqtim(tcb, &_kernel_systbl.systim, (RELTIM)timout);
                    tcb->stat.msts |= TTS_TMR;
                }
                _kernel_systbl.ctcb = 0;
            } else {
                tcb->wup--;
            }
        }
    } else {
        tcb->p1 = (VP_INT)E_CTX;
    }
    return retcd;
}

ER tslp_tsk(TMO timout)
{
    T_TCB *tcb;
    ER ercd;

    if ((_kernel_systbl.icnt != 0U) ||
        (_kernel_systbl.cssb != 0)) {
        ercd = E_CTX;
    } else if ((timout != TMO_FEVR) && (timout < 0)) {
        ercd = E_PAR;
    } else {
        tcb = _kernel_systbl.ctcb;
        tcb->p1 = (VP_INT)timout;
        tcb->p2 = tcb;
        tcb->sysfunc = (FP)&_kernel_slptsk_1;
        _kernel_entsys(FALSE);
        ercd = (ER)tcb->p1;
    }
    return ercd;
}
