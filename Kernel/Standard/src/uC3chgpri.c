/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c) 2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created
            2017.03.02: Fixed the IPA warnings.
            2017.07.19: Fixed the range check of tskpri.
            2017.09.01: Fixed the C++test StaticAnalysis warnings.
 ***************************************************************************/

#include "uC3sys.h"


static BOOL _kernel_chgpri_1(T_SSB *par, BOOL retcd);
static PRI _kernel_chgpri_ceiling(T_TCB *tcb);
static ER _kernel_chgpri_2(T_TCB *tcb, PRI tskpri);

/***********************************
    Change Task Priority
 ***********************************/

void _kernel_chgpri(T_TCB *tcb, PRI tskpri)
{
    void (*mplfunc)(ID);
    void (*mbffunc)(ID);
    PRI (*mtxfunc)(T_TCB *, BOOL);
    T_TCB *ttcb;
    T_MTX *mtx;
    BOOL loopend;

    do {
        loopend = TRUE;
        ttcb = tcb;
        ttcb->cpri = (UH)tskpri;
        if ((ttcb->stat.msts & TTS_FIFO) == 0U) {
            if ((ttcb->stat.msts & (TTS_WAI | TTS_RDY)) != 0U) {
                if (((ttcb->stat.msts & TTS_RDY) != 0U) ||
                     (ttcb->stat.wsts >= TS_SEM)) {
                    _kernel_deqtcb(&ttcb->wtcb);
                    _kernel_enqtcb(&ttcb->wque[tskpri], &ttcb->wtcb);
                    if ((ttcb->stat.msts & TTS_WAI) != 0U) {
                        if (ttcb->stat.wsts == TS_MPL) {
                            mplfunc = (void (*)(ID))_kernel_systbl.mplfunc;
                            if (mplfunc != 0) {
                                mplfunc((ID)ttcb->wobjid);
                            }
                        } else  if (ttcb->stat.wsts == TS_SMBF) {
                            mbffunc = (void (*)(ID))_kernel_systbl.mbffunc;
                            if (mbffunc != 0) {
                                mbffunc((ID)ttcb->wobjid);
                            }
                        } else  if (ttcb->stat.wsts == TS_MTX) {
                            mtx = _kernel_systbl.qmtx.mtx[ttcb->wobjid];
                            if (mtx->mtxatr == TA_INHERIT) {
                                if ((PRI)mtx->pri > tskpri) {
                                    mtx->pri = (UH)tskpri;
                                    tcb = _kernel_systbl.qtcb.tcb[mtx->tskid];
                                    if ((PRI)tcb->cpri > tskpri) {
                                        loopend = FALSE;
                                    }
                                } else if ((PRI)mtx->pri < tskpri) {
                                    tcb = _kernel_gettcb((T_WTCB *)mtx->que.mwait, (PRI)_kernel_systbl.qrdq.inf->limit);
                                    mtx->pri = tcb->cpri;
                                    tcb = _kernel_systbl.qtcb.tcb[mtx->tskid];
                                    mtxfunc = (PRI (*)(T_TCB *, BOOL))_kernel_systbl.mtxfunc;
                                    tskpri = mtxfunc(tcb, FALSE);   /* parasoft-suppress BD-PB-NP "2017/09/01 Reviewed" */
                                    if ((PRI)tcb->cpri != tskpri) {
                                        loopend = FALSE;
                                    }
                                } else {
                                    /* Do Nothing */
                                }
                            }
                        } else {
                            /* Do Nothing */
                        }
                    }
                }

                if ((ttcb->stat.msts & TTS_RDY) != 0U) {
                    if (_kernel_systbl.ctcb == ttcb) {
                        if ((PRI)_kernel_systbl.pri > tskpri) {
                            _kernel_systbl.pri = (UB)tskpri;
                        } else if (_kernel_systbl.stat.dspint != 0UL) {
                            _kernel_systbl.rdsp = 1U;
                        } else {
                            _kernel_systbl.ctcb = 0;
                        }
                    } else {
                        if ((PRI)_kernel_systbl.pri > tskpri) {
                            if (_kernel_systbl.stat.dspint != 0UL) {
                                _kernel_systbl.rdsp = 1U;
                            } else {
                                _kernel_systbl.ctcb = 0;
                            }
                        }
                    }
                }
            }
        }
    } while(loopend == FALSE);
}

static PRI _kernel_chgpri_ceiling(T_TCB *tcb)
{
    T_MTX *mtx;
    PRI tskpri = 0;

    for(mtx = tcb->lockmtx; mtx != 0; mtx = mtx->locked) {
        if (mtx->mtxatr == TA_CEILING) {
            if (tskpri < (PRI)mtx->pri) {
                tskpri = (PRI)mtx->pri;
            }
        }
    }
    return tskpri;
}

static ER _kernel_chgpri_2(T_TCB *tcb, PRI tskpri)
{
    PRI (*mtxfunc)(T_TCB *, BOOL);
    T_MTX *mtx;
    PRI ceiling;
    ER ercd;

    if (tcb->stat.msts == TTS_DMT) {
        ercd = E_OBJ;
    } else {
        if (tskpri == TPRI_INI) {
            tskpri = (PRI)tcb->ipri;
        }
        ceiling = _kernel_chgpri_ceiling(tcb);
        if (tskpri < ceiling) {
            ercd = E_ILUSE;
        } else {
            if (((tcb->stat.msts & TTS_WAI) != 0U) &&
                 (tcb->stat.wsts == TS_MTX)) {
                mtx = _kernel_systbl.qmtx.mtx[tcb->wobjid];
                if ((mtx->mtxatr == TA_CEILING) &&
                    ((PRI)mtx->pri > tskpri)) {
                    ercd = E_ILUSE;
                } else {
                    tcb->bpri = (UB)tskpri;
                    mtxfunc = (PRI (*)(T_TCB *, BOOL))_kernel_systbl.mtxfunc;
                    if (mtxfunc == 0) {
                        _kernel_chgpri(tcb, tskpri);
                    } else {
                        mtxfunc(tcb, TRUE);
                    }
                    ercd = E_OK;
                }
            } else {
                tcb->bpri = (UB)tskpri;
                mtxfunc = (PRI (*)(T_TCB *, BOOL))_kernel_systbl.mtxfunc;
                if (mtxfunc == 0) {
                    _kernel_chgpri(tcb, tskpri);
                } else {
                    mtxfunc(tcb, TRUE);
                }
                ercd = E_OK;
            }
        }
    }
    return ercd;
}

static BOOL _kernel_chgpri_1(T_SSB *par, BOOL retcd)
{
    par->p1 = (VP_INT)_kernel_chgpri_2((T_TCB *)par->p1, (PRI)par->p2);
    return retcd;
}

ER chg_pri(ID tskid, PRI tskpri)
{
    T_TCB *tcb;
    T_TCB *ttcb;
    ER ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if ((tskpri > (PRI)_kernel_systbl.qrdq.inf->limit) ||
               (tskpri < TPRI_SELF)) {
        ercd = E_PAR;
    } else if ((tskid > (ID)_kernel_systbl.qtcb.inf->limit) ||
               (tskid < TSK_SELF)) {
        ercd = E_ID;

    } else if (_kernel_systbl.cssb == 0) {
        tcb = _kernel_systbl.ctcb;
        if (tskid == TSK_SELF) {
            tskid = (ID)tcb->tskid;
        }
        _kernel_systbl.stat.s.dlock = (UB)TRUE;
        ttcb = _kernel_systbl.qtcb.tcb[tskid];
        if (ttcb == 0) {
            _kernel_relrun();
            ercd = E_NOEXS;
        } else {
            tcb->p1 = (VP_INT)ttcb;
            tcb->p2 = (VP_INT)tskpri;
            tcb->sysfunc = (FP)&_kernel_chgpri_1;
            _kernel_entsys(FALSE);
            ercd = (ER)tcb->p1;
        }
    } else {
        if (tskpri < TMIN_TPRI) {
            ercd = E_PAR;
        } else if (tskid < TMIN_TSK) {
            ercd = E_ID;
        } else {
            ttcb = _kernel_systbl.qtcb.tcb[tskid];
            if (ttcb == 0) {
                ercd = E_NOEXS;
            } else {
                ercd = _kernel_chgpri_2(ttcb, tskpri);
            }
        }
    }
    return ercd;
}
