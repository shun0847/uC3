/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2021, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created
            2014.01.21: Corrected the judgment to call a scheduler.
            2017.03.02: Fixed the IPA warnings.
            2017.07.19: Added the following checking code.
                         - If the tmout is -2 or less, an E_PAR error.
                        Fixed the unlocking process when a task terminates.
                        Fixed that priority was not inherited at the second inheritance.
                        When unlocking, the priority is initialized for mutexes
                         whose tasks do not wait for acquisition.
                        Fixed to restore task current priority when E_TMOUT error.
            2017.08.09: Fixed the DS-5 compile error.
                        Fixed the C++test StaticAnalysis warnings.
            2017.09.01: Fixed the C++test StaticAnalysis warnings.
            2019.03.28: If you change the task's current priority in conjunction 
                        with the mutex operation, If there is a task with the same 
                        priority as the priority, it has lower priority than the task.
                        This behavior has been fixed so that it will not be implemented
                        if there is currently no change in priority.
            2021.01.27: Fixed C++test warnings.
            2021.04.07: tloc_mtx remove restore process on priority inherited protocol mutex.
                        remove null check in _kernel_inherit_chktcb for C++test FlowAnalysis warnings.
***************************************************************************/

#include "uC3sys.h"


static BOOL _kernel_locmtx_1(T_SSB *par, BOOL retcd);
static BOOL _kernel_unlmtx_1(T_SSB *par, BOOL retcd);
static void _kernel_unlmtx_2(T_MTX *mtx);
static void _kernel_unlmtx(T_TCB *tcb);
static PRI _kernel_chkmtx(T_TCB *tcb, BOOL flag);
static T_TCB* _kernel_inherit_chktcb(T_MTX* mtx);

/*******************************************
    Lock Mutex
 *******************************************/

static BOOL _kernel_locmtx_1(T_SSB *par, BOOL retcd)
{
    T_TCB *ttcb;
    T_TCB *tcb;
    T_MTX *mtx;
    TMO tmout;
    PRI tskpri;
    ID tskid;

    mtx = (T_MTX *)par->p1;
    tcb = (T_TCB *)par->p3;
    tmout = (TMO)par->p2;

    if ((mtx->mtxatr == TA_CEILING) &&
        (mtx->pri    >  tcb->bpri)) {
        par->p1 = (VP_INT)E_ILUSE;
    } else if (mtx->tskid == 0U) {
        if (mtx->mtxatr == TA_CEILING) {
            if (tcb->cpri > mtx->pri) {
                tcb->cpri = mtx->pri;
                _kernel_systbl.pri = tcb->cpri;
                _kernel_deqtcb(&tcb->wtcb);
                _kernel_enqtcb(&tcb->wque[tcb->cpri], &tcb->wtcb);
            }
        } else {
            ;
        }
        mtx->tskid = tcb->tskid;
        mtx->locked = tcb->lockmtx;
        tcb->lockmtx = mtx;
        _kernel_systbl.mtxfunc = (FP)&_kernel_chkmtx;
        _kernel_systbl.umtxfunc = (FP)&_kernel_unlmtx;
        par->p1 = E_OK;
    } else {
        if (tmout == TMO_POL) {
            par->p1 = (VP_INT)E_TMOUT;
        } else if (_kernel_systbl.stat.dspint == 0UL) {
            _kernel_deqtcb(&tcb->wtcb);
            tcb->stat.msts = TTS_WAI;
            tcb->stat.wsts = TS_MTX;
            tcb->wobjid = mtx->mtxid;
            if ((mtx->mtxatr & (TA_TPRI|TA_INHERIT|TA_CEILING)) != 0U) {
                tskpri = (PRI)tcb->cpri;
                tcb->wque = mtx->que.mwait;
            } else {
                tskpri = 0;
                tcb->stat.msts |= TTS_FIFO;
                tcb->wque = &mtx->que.wait;
            }
            _kernel_enqtcb(&tcb->wque[tskpri], &tcb->wtcb);
            if (tmout != TMO_FEVR) {
                _kernel_enqtim(tcb, &_kernel_systbl.systim, (RELTIM)tmout);
                tcb->stat.msts |= TTS_TMR;
            }
            _kernel_systbl.ctcb = 0;

            if (mtx->mtxatr == TA_INHERIT) {
                if (mtx->pri > tcb->cpri) {
                    mtx->pri = tcb->cpri;
                    tskid = (PRI)mtx->tskid;
                    ttcb = _kernel_systbl.qtcb.tcb[tskid];
                    if (ttcb->cpri > mtx->pri) {
                        _kernel_chgpri(ttcb, (PRI)mtx->pri);
                    }
                }
            }
            par->p1 = (VP_INT)E_OK;
        } else {
            par->p1 = (VP_INT)E_CTX;
        }
    }
    return retcd;
}

ER tloc_mtx(ID mtxid, TMO tmout)
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

    } else if ((tmout != TMO_FEVR) && (tmout < 0)) {
        ercd = E_PAR;

    } else {
        _kernel_systbl.stat.s.dlock = (UB)TRUE;
        mtx = _kernel_systbl.qmtx.mtx[mtxid];
        tcb = _kernel_systbl.ctcb;
        if (mtx == 0) {
            _kernel_relrun();
            ercd = E_NOEXS;
        } else if (mtx->tskid == tcb->tskid) {
            _kernel_relrun();
            ercd = E_ILUSE;
        } else {
            tcb->p1 = (VP_INT)mtx;
            tcb->p2 = (VP_INT)tmout;
            tcb->p3 = (VP_INT)tcb;
            tcb->sysfunc = (FP)&_kernel_locmtx_1;
            _kernel_entsys(FALSE);
            ercd = (ER)tcb->p1;
        }
    }
    return ercd;
}

/*******************************************
    Unlock Mutex
 *******************************************/

static T_TCB* _kernel_inherit_chktcb(T_MTX* mtx)
{
    T_TCB *tcb = 0;
    PRI pri;

    if (mtx->mtxatr == TA_INHERIT) {
        for (pri = 1; pri <= (PRI)_kernel_systbl.qrdq.inf->limit; pri++) {
            if (mtx->que.mwait[pri].next != &mtx->que.mwait[pri]) {
                tcb = (T_TCB*)mtx->que.mwait[pri].next;
            }
        }
    }
    return tcb;
}

static PRI _kernel_chkmtx(T_TCB *tcb, BOOL flag)
{
    T_MTX *mtx;
    BOOL loopend;
    PRI tskpri;

    mtx = tcb->lockmtx;
    tskpri = (PRI)tcb->bpri;
    for(loopend = FALSE; loopend == FALSE;) {
        if (mtx == 0) {
            loopend = TRUE;
        } else {
            if ((mtx->mtxatr == TA_INHERIT) ||
                (mtx->mtxatr == TA_CEILING)) {
                if (tskpri > (PRI)mtx->pri) {
                    tskpri = (PRI)mtx->pri;
                }
            }
            mtx = mtx->locked;
        }
    }
    if (flag != FALSE) {
        _kernel_chgpri(tcb, tskpri);
    }
    return tskpri;
}

static void _kernel_unlmtx(T_TCB *tcb)
{
    T_MTX *mtx;
    T_MTX *next;

    for (mtx = tcb->lockmtx; mtx != 0; ) {
        next = mtx->locked;
        _kernel_unlmtx_2(mtx);
        mtx = next;
    }
    tcb->lockmtx = 0;
}

static void _kernel_unlmtx_2(T_MTX *mtx)
{
    T_WTCB *wtcb;
    T_TCB *tcb;
    PRI primax;

    if ((mtx->mtxatr & (TA_TPRI|TA_INHERIT|TA_CEILING)) != 0U) {
        primax = (PRI)_kernel_systbl.qrdq.inf->limit;
        wtcb = mtx->que.mwait;
    } else {
        primax = 0;
        wtcb = &mtx->que.wait;
    }
    tcb = _kernel_gettcb(wtcb, primax);
    if (tcb != 0) {
        _kernel_deqtcb(&tcb->wtcb);
        if ((tcb->stat.msts & TTS_TMR) != 0U) {
            tcb->sprev->snext = (T_SSB *)tcb->snext;
            if (tcb->snext != 0) {
                tcb->snext->sprev = (T_SSB *)tcb->sprev;
            }
            tcb->stat.msts &= (UB)~TTS_TMR;
        }

        if (mtx->mtxatr == TA_INHERIT) {
            T_TCB* tcb2;
            mtx->pri = tcb->cpri;
            tcb2 = _kernel_inherit_chktcb(mtx);
            if (tcb2 != 0) {
                /* Do Nothing */
            } else {
                mtx->pri = (UH)TMAX_TPRI;
            }
        } else if (mtx->mtxatr == TA_CEILING) {
            if (tcb->cpri > mtx->pri) {
                tcb->cpri = mtx->pri;
            }
        } else {
            /* Do Nothing */
        }
        mtx->tskid = tcb->tskid;
        mtx->locked = tcb->lockmtx;
        tcb->lockmtx = mtx;
        _kernel_enqrdq(tcb);
    } else {
        mtx->tskid = 0U;
        if (mtx->mtxatr == TA_INHERIT) {
            mtx->pri = (UH)TMAX_TPRI;
        }
    }
}

static BOOL _kernel_unlmtx_1(T_SSB *par, BOOL retcd)
{
    T_TCB *tcb;
    T_MTX *mtx;
    T_MTX *lmtx;
    PRI pri;

    mtx = (T_MTX *)par->p1;
    tcb = (T_TCB *)par->p2;

    if (tcb->lockmtx == mtx) {
        tcb->lockmtx = mtx->locked;
    } else {
        for(lmtx = tcb->lockmtx; lmtx != 0; lmtx = lmtx->locked) {
            if (lmtx->locked == mtx) {
                lmtx->locked = mtx->locked;
            }
        }
    }
    _kernel_unlmtx_2(mtx);
    pri = _kernel_chkmtx(tcb, FALSE);
    if (tcb->cpri != (UH)pri){
        _kernel_deqtcb(&tcb->wtcb);
        _kernel_enqtcb(&tcb->wque[pri], &tcb->wtcb);
    }
    tcb->cpri = (UH)pri;
    if ((PRI)_kernel_systbl.pri < pri) {
        if (_kernel_systbl.stat.dspint != 0UL) {
            _kernel_systbl.rdsp = 1U;
        } else {
            _kernel_systbl.ctcb = 0;
        }
    }

    par->p1 = E_OK;
    return retcd;
}

ER unl_mtx(ID mtxid)
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
        tcb = _kernel_systbl.ctcb;
        if (mtx == 0) {
            _kernel_relrun();
            ercd = E_NOEXS;
        } else if (mtx->tskid != tcb->tskid) {
            _kernel_relrun();
            ercd = E_ILUSE;
        } else {
            tcb->p1 = (VP_INT)mtx;
            tcb->p2 = (VP_INT)tcb;
            tcb->sysfunc = (FP)&_kernel_unlmtx_1;
            _kernel_entsys(FALSE);
            ercd = (ER)tcb->p1;
        }
    }
    return ercd;
}
