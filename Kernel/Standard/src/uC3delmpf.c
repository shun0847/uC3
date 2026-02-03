/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created.
            2012.08.24: Corrected the dequeuing of timer queue.
            2017.01.26: Fixed the IPA warnings.
            2017.09.01: Fixed the C++test StaticAnalysis warnings.
 ***************************************************************************/

#include "uC3sys.h"


static BOOL _kernel_delmpf(T_SSB *par, BOOL retcd);

/***********************************
    Delete Fixed-Sized Memory Pool
 ***********************************/

static BOOL _kernel_delmpf(T_SSB *par, BOOL retcd)
{
    T_TCB *tcb;
    T_MPF *mpf;
    T_WTCB *wtcb;
    PRI primax;

    mpf = (T_MPF *)par->p1;
    if ((mpf->mpfatr & TA_TPRI) != 0U) {
        primax = (PRI)_kernel_systbl.qrdq.inf->limit;
        wtcb = mpf->que.mwait;
    } else {
        primax = 0;
        wtcb = &mpf->que.wait;
    }
    for(tcb = _kernel_gettcb(wtcb, primax); tcb != 0; tcb = _kernel_gettcb(wtcb, primax)) {
        _kernel_deqtcb(&tcb->wtcb);
        if ((tcb->stat.msts & TTS_TMR) != 0U) {
            tcb->sprev->snext = (T_SSB *)tcb->snext;
            if (tcb->snext != 0) {
                tcb->snext->sprev = (T_SSB *)tcb->sprev;
            }
        }
        tcb->p1 = (VP_INT)E_DLT;
        _kernel_enqrdq(tcb);
    }

    _kernel_systbl.qmpf.mpf[mpf->mpfid] = 0;
    _kernel_systbl.qmpf.inf->usedc--;

    par->p1 = (VP_INT)E_OK;
    return retcd;
}

ER del_mpf(ID mpfid)
{
    T_TCB *tcb;
    T_MPF *mpf;
    T_MEM **mem_base;
    ER ercd;

    if ((_kernel_systbl.icnt != 0U) ||
        (_kernel_systbl.cssb != 0)) {
        ercd = E_CTX;
    } else if (( _kernel_systbl.qmpf.inf == 0) ||
               ((mpfid < TMIN_OBJ) || (mpfid > (ID)_kernel_systbl.qmpf.inf->limit))) {
        ercd = E_ID;

    } else {
        _kernel_systbl.stat.s.dlock = (UB)TRUE;
        mpf = _kernel_systbl.qmpf.mpf[mpfid];
        if (mpf == 0) {
            ercd = E_NOEXS;
        } else {
            tcb = _kernel_systbl.ctcb;
            tcb->p1 = (VP_INT)mpf;
            tcb->sysfunc = (FP)&_kernel_delmpf;
            _kernel_entsys(TRUE);
            ercd = (ER)tcb->p1;

            if (ercd == E_OK) { /* parasoft-suppress BD-PB-CC "2017/09/01 Reviewed" */
                if ((mpf->mpfatr & TA_TPRI) != 0U) {
                    (void)_kernel_relmem(&_kernel_systbl.free_sys, (T_MEM *)mpf->que.mwait+1,
                                    _kernel_systbl.qrdq.inf->limit * sizeof(T_WTCB));
                }
                if ((mpf->mpfatr & TA_UBUF) == 0U) {
                    if (_kernel_systbl.free_mpl != (T_MEM *)1) {
                        mem_base = &_kernel_systbl.free_mpl;
                    } else if (_kernel_systbl.free_stk != (T_MEM *)1) {
                        mem_base = &_kernel_systbl.free_stk;
                    } else {
                        mem_base = &_kernel_systbl.free_sys;
                    }
                    (void)_kernel_relmem(mem_base, (T_MEM *)mpf->allad, mpf->allsz);
                }
                (void)_kernel_relmem(&_kernel_systbl.free_sys, (T_MEM *)mpf, sizeof(T_MPF));
            }
        }
        _kernel_relrun();
    }
    return ercd;
}
