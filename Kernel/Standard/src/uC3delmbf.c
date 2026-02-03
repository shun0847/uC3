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


static BOOL _kernel_delmbf(T_SSB *par, BOOL retcd);

/***********************************
    Delete Message Buffer
 ***********************************/

static BOOL _kernel_delmbf(T_SSB *par, BOOL retcd)
{
    T_TCB *tcb;
    T_MBF *mbf;
    T_WTCB *wtcb;
    PRI primax;

    mbf = (T_MBF *)par->p1;
    if ((mbf->mbfatr & TA_TPRI) != 0U) {
        primax = (PRI)_kernel_systbl.qrdq.inf->limit;
        wtcb = mbf->sque.mwait;
    } else {
        primax = 0;
        wtcb = &mbf->sque.wait;
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

    for(tcb = _kernel_gettcb(&mbf->wque, 0); tcb != 0; tcb = _kernel_gettcb(&mbf->wque, 0)) {
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

    _kernel_systbl.qmbf.mbf[mbf->mbfid] = 0;
    _kernel_systbl.qmbf.inf->usedc--;

    par->p1 = (VP_INT)E_OK;
    return retcd;
}

ER del_mbf(ID mbfid)
{
    T_TCB *tcb;
    T_MBF *mbf;
    T_MEM **mem_base;
    ER ercd;

    if ((_kernel_systbl.icnt != 0U) ||
        (_kernel_systbl.cssb != 0)) {
        ercd = E_CTX;
    } else if (( _kernel_systbl.qmbf.inf == 0) ||
               ((mbfid < TMIN_OBJ) || (mbfid > (ID)_kernel_systbl.qmbf.inf->limit))) {
        ercd = E_ID;

    } else {
        _kernel_systbl.stat.s.dlock = (UB)TRUE;
        mbf = _kernel_systbl.qmbf.mbf[mbfid];
        if (mbf == 0) {
            ercd = E_NOEXS;
        } else {
            tcb = _kernel_systbl.ctcb;
            tcb->p1 = (VP_INT)mbf;
            tcb->sysfunc = (FP)&_kernel_delmbf;
            _kernel_entsys(TRUE);
            ercd = (ER)tcb->p1;

            if (ercd == E_OK) { /* parasoft-suppress BD-PB-CC "2017/09/01 Reviewed" */
                if ((mbf->mbfatr & TA_TPRI) != 0U) {
                    (void)_kernel_relmem(&_kernel_systbl.free_sys, (T_MEM *)mbf->sque.mwait+1,
                                    _kernel_systbl.qrdq.inf->limit * sizeof(T_WTCB));
                }
                if (((mbf->mbfatr & TA_UBUF) == 0U) && (mbf->mbfsz != 0U)) {
                    if (_kernel_systbl.free_mpl != (T_MEM *)1) {
                        mem_base = &_kernel_systbl.free_mpl;
                    } else if (_kernel_systbl.free_stk != (T_MEM *)1) {
                        mem_base = &_kernel_systbl.free_stk;
                    } else {
                        mem_base = &_kernel_systbl.free_sys;
                    }
                    (void)_kernel_relmem(mem_base, (T_MEM *)mbf->mbf, mbf->mbfsz);
                }
                (void)_kernel_relmem(&_kernel_systbl.free_sys, (T_MEM *)mbf, sizeof(T_MBF));
            }
        }
        _kernel_relrun();
    }
    return ercd;
}
