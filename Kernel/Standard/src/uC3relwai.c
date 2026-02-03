/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created.
            2009.09.12: Modified for the erro-code handler.
            2017.01.26: Fixed the IPA warnings.
 ***************************************************************************/

#include "uC3sys.h"


static ER _kernel_relwai(T_TCB *tcb);
static BOOL _kernel_relwai_1(T_SSB *par, BOOL retcd);

/***********************************
    Release Task from Waiting
 ***********************************/

static ER _kernel_relwai(T_TCB *tcb)
{
    void (*mplfunc)(ID);
    void (*mbffunc)(ID);
    PRI (*mtxfunc)(T_TCB *, BOOL);
    T_TCB *ttcb;
    T_MTX *mtx;
    ER ercd;

    if ((tcb->stat.msts & TTS_WAI) != 0U) {
        if (tcb->stat.wsts >= TS_SEM) {
            _kernel_deqtcb(&tcb->wtcb);
            if (tcb->stat.wsts == TS_MPL) {
                mplfunc = (void (*)(ID))_kernel_systbl.mplfunc;
                if (mplfunc != 0) {
                    mplfunc((ID)tcb->wobjid);
                }
            }
            if (tcb->stat.wsts == TS_SMBF) {
                mbffunc = (void (*)(ID))_kernel_systbl.mbffunc;
                if (mbffunc != 0) {
                    mbffunc((ID)tcb->wobjid);
                }
            }
            if (tcb->stat.wsts == TS_MTX) {
                mtx = _kernel_systbl.qmtx.mtx[tcb->wobjid];
                if (mtx->mtxatr == TA_INHERIT) {
                    ttcb = _kernel_gettcb((T_WTCB *)mtx->que.mwait, (PRI)_kernel_systbl.qrdq.inf->limit);
                    if (ttcb != 0) {
                        mtx->pri = ttcb->cpri;
                    } else {
                        mtx->pri = (UH)TMAX_TPRI;
                    }
                    ttcb = _kernel_systbl.qtcb.tcb[mtx->tskid];
                    mtxfunc = (PRI (*)(T_TCB *, BOOL))_kernel_systbl.mtxfunc;
                    mtxfunc(ttcb, TRUE);
                }
            }
        }
        if ((tcb->stat.msts & TTS_TMR) != 0U) {
            tcb->sprev->snext = (T_SSB *)tcb->snext;
            if (tcb->snext != 0) {
                tcb->snext->sprev = (T_SSB *)tcb->sprev;
            }
        }
        _kernel_enqrdq(tcb);
        tcb->p1 = (VP_INT)E_RLWAI;
        ercd = E_OK;
    } else {
        ercd = E_OBJ;
    }
    return ercd;
}

static BOOL _kernel_relwai_1(T_SSB *par, BOOL retcd)
{
    par->p1 = (VP_INT)_kernel_relwai((T_TCB *)par->p1);
    return retcd;
}

ER rel_wai(ID tskid)
{
    T_SSB *ssb;
    T_TCB *tcb;
    T_TCB *ttcb;
    ER ercd;

    if ((tskid > (ID)_kernel_systbl.qtcb.inf->limit) ||
        (tskid < TMIN_TSK)) {
        ercd = E_ID;

    } else if (_kernel_systbl.icnt != 0U) {
        ttcb = _kernel_systbl.qtcb.tcb[tskid];
        if (ttcb == 0) {
            ercd = E_NOEXS;
        } else {
            ssb = _kernel_getssb();
            if (ssb == 0) {
                ercd = E_NOMEM;
            } else {
                ssb->stat.catr = TA_SSB;
                ssb->p1 = (VP_INT)ttcb;
                ssb->p3 = (VP_INT)TFN_REL_WAI;
                ssb->p4 = (VP_INT)tskid;
                ssb->sysfunc = (FP)&_kernel_relwai_1;
                _kernel_enqssb(ssb);
                ercd = E_OK;
            }
        }
    } else if (_kernel_systbl.cssb == 0) {
        _kernel_systbl.stat.s.dlock = (UB)TRUE;
        ttcb = _kernel_systbl.qtcb.tcb[tskid];
        if (ttcb == 0) {
            _kernel_relrun();
            ercd = E_NOEXS;
        } else {
            tcb = _kernel_systbl.ctcb;
            tcb->p1 = (VP_INT)ttcb;
            tcb->sysfunc = (FP)&_kernel_relwai_1;
            _kernel_entsys(FALSE);
            ercd = (ER)tcb->p1;
        }
    } else {
        ttcb = _kernel_systbl.qtcb.tcb[tskid];
        if (ttcb == 0) {
            ercd = E_NOEXS;
        } else {
            ercd = _kernel_relwai(ttcb);
        }
    }
    return ercd;
}
