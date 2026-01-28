/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2021, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created
            2009.03.31: Corrected TimeEvent Handler.
            2009.08.03: Modified for the multi-core system.
            2011.12.11: Corrected the timer enqueuing.
            2017.03.02: Fixed the IPA warnings.
            2017.07.19: Changed timout type to RELTIM in _kernel_enqtim
            2017.09.01: Modified _kernel_sigtim() to global function.
            2021.04.07: Add mutex restore process.
 ***************************************************************************/

#include "uC3sys.h"


static BOOL _kernel_sigtim_1(T_SSB *par, BOOL retcd);

/******************************************
     Connect Control Block to Timer Queue
 ******************************************/

void _kernel_enqtim(T_TCB *tcb, SYSTIM *systim, RELTIM timout)
{
    T_TCB *tim_top, *tcb1;
    BOOL loopend;
    UW ltime;

    ltime = systim->ltime;
    tcb->stime.utime = systim->utime;
    tcb->stime.ltime = systim->ltime + timout;
    if (tcb->stime.ltime < ltime) {
        tcb->stime.utime++;
    }

    tim_top = (T_TCB *)&_kernel_systbl;
    for(loopend = FALSE; loopend == FALSE; ) {
        tcb1 = (T_TCB *)tim_top->snext;
        if (tcb1 != 0) {
            if (((tcb1->stime.utime == tcb->stime.utime) && (tcb1->stime.ltime > tcb->stime.ltime)) ||
                 (tcb1->stime.utime >  tcb->stime.utime)) {
                tcb1->sprev = (T_SSB *)tcb;
                tcb->sprev = (T_SSB *)tim_top;
                tcb->snext = (T_SSB *)tcb1;
                tim_top->snext = (T_SSB *)tcb;
                loopend = TRUE;
            } else {
                tim_top = tcb1;
            }
        } else {
            tim_top->snext = (T_SSB *)tcb;
            tcb->snext = 0;
            tcb->sprev = (T_SSB *)tim_top;
            loopend = TRUE;
        }
    }
}

/**********************************
     isig_tim()
 **********************************/

ER _kernel_sigtim(void)
{
    UW utime, ltime;
    T_TCB *tim_top, *tcb;
    T_TCB *ttcb;
    T_MTX *mtx;
    void (*mtxfunc)(T_TCB *, BOOL);
    UB wsts;

    ltime = _kernel_systbl.systim.ltime + (UW)_kernel_systbl.tick;
    utime = _kernel_systbl.systim.utime;
    if (ltime < _kernel_systbl.systim.ltime) {
        utime++;
        _kernel_systbl.systim.utime = utime;
    }
    _kernel_systbl.systim.ltime = ltime;

    for(tim_top = (T_TCB *)&_kernel_systbl; tim_top->snext != 0; ) {
        tcb = (T_TCB *)tim_top->snext;
        if (((tcb->stime.utime == utime) && (tcb->stime.ltime <= ltime)) ||
             (tcb->stime.utime <  utime)) {
            tim_top->snext = (T_SSB *)tcb->snext;
            if (tcb->snext != 0) {
                tcb->snext->sprev = (T_SSB *)tim_top;
            }
            if ((tcb->stat.catr & TA_ALM) != 0U) {
                ((T_ALM*)tcb)->stat.msts |= TALM_PND;
                _kernel_enqssb((T_SSB *)tcb);
            } else if ((tcb->stat.catr & TA_CYC) != 0U) {
                ((T_CYC*)tcb)->stat.msts |= TCYC_PND;
                _kernel_enqssb((T_SSB *)tcb);
            } else if ((tcb->stat.catr & TA_SSB) != 0U) {
                _kernel_enqssb((T_SSB *)tcb);
            } else {
                if (((tcb->stat.msts & TTS_WAI) != 0U) &&
                    ( tcb->stat.wsts != TS_DLY)) {
                    tcb->p1 = (VP_INT)E_TMOUT;
                }
                if (((tcb->stat.msts & TTS_WAI) != 0U) &&
                    ( tcb->stat.wsts >= TS_SEM)) {
                    _kernel_deqtcb(&tcb->wtcb);
                    wsts = tcb->stat.wsts;
                    _kernel_enqrdq(tcb);
                    if (wsts == TS_MTX) {
                        mtx = _kernel_systbl.qmtx.mtx[tcb->wobjid];
                        if (mtx->mtxatr == TA_INHERIT) {
                            ttcb = _kernel_gettcb((T_WTCB *)mtx->que.mwait, (PRI)_kernel_systbl.qrdq.inf->limit);
                            if (ttcb != 0) {
                                mtx->pri = ttcb->cpri;
                            } else {
                                mtx->pri = (UH)TMAX_TPRI;
                            }
                            ttcb = _kernel_systbl.qtcb.tcb[mtx->tskid];
                            mtxfunc = (void (*)(T_TCB *, BOOL))_kernel_systbl.mtxfunc;
                            mtxfunc(ttcb, TRUE);
                        }
                    }
                } else {
                    _kernel_enqrdq(tcb);
                }
            }
        } else {
            break;
        }
    }
    return E_OK;
}

static BOOL _kernel_sigtim_1(T_SSB *par, BOOL retcd)
{
    par->p1 = (VP_INT)_kernel_sigtim();
    return retcd;
}

ER isig_tim(void)
{
    T_SSB *ssb;
    ER ercd;

    if (_kernel_systbl.icnt == 0U) {
        ercd = E_CTX;

    } else {
        ssb = _kernel_getssb();
        if (ssb == 0) {
            ercd = E_NOMEM;
        } else {
            ssb->stat.catr = TA_SSB;
            ssb->sysfunc = (FP)&_kernel_sigtim_1;
            ssb->p3 = (VP_INT)TFN_ISIG_TIM;
            _kernel_enqssb(ssb);
            ercd = E_OK;
        }
    }
    return ercd;
}
