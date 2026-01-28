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


static BOOL _kernel_delmpl(T_SSB *par, BOOL retcd);

/***************************************
    Delete Variable-Sized Memory Pool
 ***************************************/

static BOOL _kernel_delmpl(T_SSB *par, BOOL retcd)
{
    T_TCB *tcb;
    T_MPL *mpl;
    T_WTCB *wtcb;
    PRI primax;

    mpl = (T_MPL *)par->p1;
    if ((mpl->mplatr & TA_TPRI) != 0U) {
        primax = (PRI)_kernel_systbl.qrdq.inf->limit;
        wtcb = mpl->que.mwait;
    } else {
        primax = 0;
        wtcb = &mpl->que.wait;
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

    _kernel_systbl.qmpl.mpl[mpl->mplid] = 0;
    _kernel_systbl.qmpl.inf->usedc--;

    par->p1 = (VP_INT)E_OK;
    return retcd;
}

ER del_mpl(ID mplid)
{
    T_TCB *tcb;
    T_MPL *mpl;
    T_MEM **mem_base;
    ER ercd;

    if ((_kernel_systbl.icnt != 0U) ||
        (_kernel_systbl.cssb != 0)) {
        ercd = E_CTX;
    } else if (( _kernel_systbl.qmpl.inf == 0) ||
               ((mplid < TMIN_OBJ) || (mplid > (PRI)_kernel_systbl.qmpl.inf->limit))) {
        ercd = E_ID;

    } else {
        _kernel_systbl.stat.s.dlock = (UB)TRUE;
        mpl = _kernel_systbl.qmpl.mpl[mplid];
        if (mpl == 0) {
            ercd = E_NOEXS;
        } else {
            tcb = _kernel_systbl.ctcb;
            tcb->p1 = (VP_INT)mpl;
            tcb->sysfunc = (FP)&_kernel_delmpl;
            _kernel_entsys(TRUE);
            ercd = (ER)tcb->p1;

            if (ercd == E_OK) { /* parasoft-suppress BD-PB-CC "2017/09/01 Reviewed" */
                if ((mpl->mplatr & TA_TPRI) != 0U) {
                    (void)_kernel_relmem(&_kernel_systbl.free_sys, (T_MEM *)mpl->que.mwait+1,
                                    _kernel_systbl.qrdq.inf->limit * sizeof(T_WTCB));
                }
                if ((mpl->mplatr & TA_UBUF) == 0U) {
                    if (_kernel_systbl.free_mpl != (T_MEM *)1) {
                        mem_base = &_kernel_systbl.free_mpl;
                    } else if (_kernel_systbl.free_stk != (T_MEM *)1) {
                        mem_base = &_kernel_systbl.free_stk;
                    } else {
                        mem_base = &_kernel_systbl.free_sys;
                    }
                    (void)_kernel_relmem(mem_base, (T_MEM *)mpl->allad, mpl->allsz);
                }
                (void)_kernel_relmem(&_kernel_systbl.free_sys, (T_MEM *)mpl, sizeof(T_MPL));
            }
        }
        _kernel_relrun();
    }
    return ercd;
}
