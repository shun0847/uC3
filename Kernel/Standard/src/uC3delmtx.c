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


static BOOL _kernel_delmtx(T_SSB *par, BOOL retcd);

/***********************************
    Delete Eventflag
 ***********************************/

static BOOL _kernel_delmtx(T_SSB *par, BOOL retcd)
{
    T_WTCB *wtcb;
    T_TCB *tcb;
    T_MTX *mtx;
    T_MTX *lmtx;
    void (*mtxfunc)(T_TCB *, BOOL);
    PRI primax;

    mtx = (T_MTX *)par->p1;

    if ((mtx->mtxatr & (TA_TPRI|TA_INHERIT|TA_CEILING)) != 0U) {
        primax = (PRI)_kernel_systbl.qrdq.inf->limit;
        wtcb = mtx->que.mwait;
    } else {
        primax = 0;
        wtcb = &mtx->que.wait;
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

    mtxfunc = (void (*)(T_TCB *, BOOL))_kernel_systbl.mtxfunc;
    if ((mtx->tskid != 0U) &&
        (mtxfunc != 0)) {
        tcb = _kernel_systbl.qtcb.tcb[mtx->tskid];
        if (tcb->lockmtx == mtx) {
            tcb->lockmtx = mtx->locked;
            mtxfunc(tcb, TRUE);
        } else {
            for(lmtx = tcb->lockmtx; lmtx != 0; lmtx = lmtx->locked) {
                if (lmtx->locked == mtx) {
                    lmtx->locked = mtx->locked;
                    mtxfunc(tcb, TRUE);
                }
            }
        }
    }

    _kernel_systbl.qmtx.mtx[mtx->mtxid] = 0;
    _kernel_systbl.qmtx.inf->usedc--;

    par->p1 = (VP_INT)E_OK;
    return retcd;
}

ER del_mtx(ID mtxid)
{
    T_TCB *tcb;
    T_MTX *mtx;
    ER ercd;

    if ((_kernel_systbl.icnt != 0U) ||
        (_kernel_systbl.cssb != 0)) {
        ercd = E_CTX;
    } else if (( _kernel_systbl.qmtx.inf == 0) ||
               ((mtxid < TMIN_OBJ) || (mtxid > (ID)_kernel_systbl.qmtx.inf->limit))) {
        ercd = E_ID;

    } else {
        _kernel_systbl.stat.s.dlock = (UB)TRUE;
        mtx = _kernel_systbl.qmtx.mtx[mtxid];
        if (mtx == 0) {
            ercd = E_NOEXS;
        } else {
            tcb = _kernel_systbl.ctcb;
            tcb->p1 = (VP_INT)mtx;
            tcb->sysfunc = (FP)&_kernel_delmtx;
            _kernel_entsys(TRUE);
            ercd = (ER)tcb->p1;

            if (ercd == E_OK) { /* parasoft-suppress BD-PB-CC "2017/09/01 Reviewed" */
                if ((mtx->mtxatr & (TA_TPRI|TA_INHERIT|TA_CEILING)) != 0U) {
                    (void)_kernel_relmem(&_kernel_systbl.free_sys, (T_MEM *)mtx->que.mwait+1,
                                    _kernel_systbl.qrdq.inf->limit * sizeof(T_WTCB));
                }
                (void)_kernel_relmem(&_kernel_systbl.free_sys, (T_MEM *)mtx, sizeof(T_MTX));
            }
        }
        _kernel_relrun();
    }
    return ercd;
}
