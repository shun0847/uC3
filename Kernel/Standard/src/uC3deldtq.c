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


static BOOL _kernel_deldtq(T_SSB *par, BOOL retcd);

/***********************************
    Delete Data Queue
 ***********************************/

static BOOL _kernel_deldtq(T_SSB *par, BOOL retcd)
{
    T_TCB *tcb;
    T_DTQ *dtq;
    T_WTCB *wtcb;
    PRI primax;

    dtq = (T_DTQ *)par->p1;
    if ((dtq->dtqatr & TA_TPRI) != 0U) {
        primax = (PRI)_kernel_systbl.qrdq.inf->limit;
        wtcb = dtq->sque.mwait;
    } else {
        primax = 0;
        wtcb = &dtq->sque.wait;
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

    for(tcb = _kernel_gettcb(&dtq->que, 0); tcb != 0; tcb = _kernel_gettcb(&dtq->que, 0)) {
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

    _kernel_systbl.qdtq.dtq[dtq->dtqid] = 0;
    _kernel_systbl.qdtq.inf->usedc--;

    par->p1 = (VP_INT)E_OK;
    return retcd;
}

ER del_dtq(ID dtqid)
{
    T_TCB *tcb;
    T_DTQ *dtq;
    T_MEM **mem_base;
    ER ercd;

    if ((_kernel_systbl.icnt != 0U) ||
        (_kernel_systbl.cssb != 0)) {
        ercd = E_CTX;
    } else if (( _kernel_systbl.qdtq.inf == 0) ||
               ((dtqid < TMIN_OBJ) || (dtqid > (ID)_kernel_systbl.qdtq.inf->limit))) {
        ercd = E_ID;

    } else {
        _kernel_systbl.stat.s.dlock = (UB)TRUE;
        dtq = _kernel_systbl.qdtq.dtq[dtqid];
        if (dtq == 0) {
            ercd = E_NOEXS;
        } else {
            tcb = _kernel_systbl.ctcb;
            tcb->p1 = (VP_INT)dtq;
            tcb->sysfunc = (FP)&_kernel_deldtq;
            _kernel_entsys(TRUE);
            ercd = (ER)tcb->p1;

            if (ercd == E_OK) { /* parasoft-suppress BD-PB-CC "2017/09/01 Reviewed" */
                if ((dtq->dtqatr & TA_TPRI) != 0U) {
                    (void)_kernel_relmem(&_kernel_systbl.free_sys, (T_MEM *)dtq->sque.mwait+1,
                                    _kernel_systbl.qrdq.inf->limit * sizeof(T_WTCB));
                }
                if (((dtq->dtqatr & TA_UBUF) == 0U) && (dtq->dtqcnt != 0U)) {
                    if (_kernel_systbl.free_mpl != (T_MEM *)1) {
                        mem_base = &_kernel_systbl.free_mpl;
                    } else if (_kernel_systbl.free_stk != (T_MEM *)1) {
                        mem_base = &_kernel_systbl.free_stk;
                    } else {
                        mem_base = &_kernel_systbl.free_sys;
                    }
                    (void)_kernel_relmem(mem_base, (T_MEM *)dtq->dtq, dtq->dtqcnt*sizeof(VP_INT));
                }
                (void)_kernel_relmem(&_kernel_systbl.free_sys, (T_MEM *)dtq, sizeof(T_DTQ));
            }
        }
        _kernel_relrun();
    }
    return ercd;
}
