/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created
            2017.01.26: Fixed the IPA warnings.
 ***************************************************************************/

#include "uC3sys.h"


static BOOL _kernel_tertsk_1(T_SSB *par, BOOL retcd);

/***********************************
    Terminate Task
 ***********************************/

static BOOL _kernel_tertsk_1(T_SSB *par, BOOL retcd)
{
    void (*umtxfunc)(T_TCB *);
    void (*mplfunc)(ID);
    void (*mbffunc)(ID);
    T_TCB *tcb;
    T_TCB *ttcb;
    T_MTX *mtx;
    PRI (*mtxfunc)(T_TCB *, BOOL);

    tcb = (T_TCB *)par->p1;
    if (tcb->stat.msts == TTS_DMT) {
        par->p1 = (VP_INT)E_OBJ;
    } else {
        if ((tcb->stat.msts & TTS_RDY) != 0U) {
            _kernel_deqtcb(&tcb->wtcb);
        } else if ((tcb->stat.msts & TTS_WAI) != 0U) {
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
        } else {
            /* Do Nothing */
        }
        umtxfunc = (void (*)(T_TCB *))_kernel_systbl.umtxfunc;
        if (umtxfunc != 0) {
            umtxfunc(tcb);
        }
        tcb->stat.msts = TTS_DMT;

        if (tcb->act > 0U) {
            tcb->act--;
            _kernel_statsk_2(tcb, tcb->exinf);
        }
        par->p1 = (VP_INT)E_OK;
    }
    return retcd;
}

ER ter_tsk(ID tskid)
{
    T_TCB *tcb;
    T_TCB *ttcb;
    ER ercd;

    if ((_kernel_systbl.icnt != 0U) || (_kernel_systbl.cssb != 0)) {
        ercd = E_CTX;
    } else if ((tskid > (ID)_kernel_systbl.qtcb.inf->limit) || (tskid < TMIN_TSK)) {
        ercd = E_ID;
    } else if (tskid == (ID)_kernel_systbl.tid) {
        ercd = E_ILUSE;

    } else {
        _kernel_systbl.stat.s.dlock = (UB)TRUE;
        ttcb = _kernel_systbl.qtcb.tcb[tskid];
        if (ttcb == 0) {
            _kernel_relrun();
            ercd = E_NOEXS;
        } else {
            tcb = _kernel_systbl.ctcb;
            tcb->p1 = (VP_INT)ttcb;
            tcb->sysfunc = (FP)&_kernel_tertsk_1;
            _kernel_entsys(FALSE);
            ercd = (ER)tcb->p1;
        }
    }
    return ercd;
}
