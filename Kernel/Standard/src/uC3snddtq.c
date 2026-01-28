/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created.
            2009.09.12: Modified for the erro-code handler.
            2010.02.05: Corrected the checking of error code.
            2017.03.02: Fixed the IPA warnings.
            2017.07.19: Added the following checking code.
                         - If the tmout is -2 or less, an E_PAR error.
 ***************************************************************************/

#include "uC3sys.h"


static ER _kernel_snddtq_2(T_DTQ *dtq, VP_INT data, TMO tmout, BOOL fmode, T_TCB *tcb);
static BOOL _kernel_snddtq_1(T_SSB *par, BOOL retcd);
static ER _kernel_snddtq(ID dtqid, VP_INT data, TMO tmout, BOOL fmode);
static void _kernel_snddtq_3(VP_INT data, T_TCB *tcb);

/***********************************
    Send to Data Queue
 ***********************************/

static void _kernel_snddtq_3(VP_INT data, T_TCB *tcb)
{
    VP_INT *p_data;

    p_data = (VP_INT *)tcb->p2;
    *p_data = data;
    _kernel_deqtcb(&tcb->wtcb);
    if ((tcb->stat.msts & TTS_TMR) != 0U) {
        tcb->sprev->snext = (T_SSB *)tcb->snext;
        if (tcb->snext != 0) {
            tcb->snext->sprev = (T_SSB *)tcb->sprev;
        }
        tcb->stat.msts &= (UB)~TTS_TMR;
    }
    _kernel_enqrdq(tcb);
}

static ER _kernel_snddtq_2(T_DTQ *dtq, VP_INT data, TMO tmout, BOOL fmode, T_TCB *tcb)
{
    T_TCB *ttcb;
    PRI tskpri;
    ER ercd;

    if (tmout == TMO_POL) {
        ttcb = _kernel_gettcb(&dtq->que, 0);
        if (ttcb != 0) {
            _kernel_snddtq_3(data, ttcb);
            ercd = E_OK;
        } else if ((dtq->dtqcnt == dtq->cnt) && (fmode == FALSE)) {
            ercd = E_TMOUT;
        } else {
            if ((dtq->dtqcnt != dtq->cnt) || (fmode == FALSE)) {
                dtq->cnt++;
            }
            dtq->dtq[dtq->put++] = data;
            if (dtq->put >= dtq->dtqcnt) {
                dtq->put = 0U;
            }
            ercd = E_OK;
        }
    } else if (_kernel_systbl.stat.dspint == 0UL) {
        ttcb = _kernel_gettcb(&dtq->que, 0);
        if (ttcb != 0) {
            _kernel_snddtq_3(data, ttcb);
        } else if ((dtq->dtqcnt == dtq->cnt) && (fmode == FALSE)) {
            _kernel_deqtcb(&tcb->wtcb);
            tcb->stat.msts = TTS_WAI;
            tcb->stat.wsts = TS_SDTQ;
            tcb->wobjid = dtq->dtqid;
            if ((dtq->dtqatr & TA_TPRI) != 0U) {
                tskpri = (PRI)tcb->cpri;
                tcb->wque = dtq->sque.mwait;
            } else {
                tskpri = 0;
                tcb->stat.msts |= TTS_FIFO;
                tcb->wque = &dtq->sque.wait;
            }
            _kernel_enqtcb(&tcb->wque[tskpri], &tcb->wtcb);
            if (tmout != TMO_FEVR) {
                _kernel_enqtim(tcb, &_kernel_systbl.systim, (RELTIM)tmout);
                tcb->stat.msts |= TTS_TMR;
            }
            _kernel_systbl.ctcb = 0;
        } else {
            if ((dtq->dtqcnt != dtq->cnt) || (fmode == FALSE)) {
                dtq->cnt++;
            }
            dtq->dtq[dtq->put++] = data;
            if (dtq->put >= dtq->dtqcnt) {
                dtq->put = 0U;
            }
        }
        ercd = E_OK;
    } else {
        ercd = E_CTX;
    }
    return ercd;
}

static BOOL _kernel_snddtq_1(T_SSB *par, BOOL retcd)
{
    par->p1 = (VP_INT)_kernel_snddtq_2((T_DTQ *)par->p1, (VP_INT)par->p5,
                             (TMO)par->p6, (BOOL)par->p2, (T_TCB *)par->p4);
    return retcd;
}

static ER _kernel_snddtq(ID dtqid, VP_INT data, TMO tmout, BOOL fmode)
{
    T_SSB *ssb;
    T_TCB *tcb;
    T_DTQ *dtq;
    ER ercd;

    if (( _kernel_systbl.qdtq.inf == 0) ||
        ((dtqid < TMIN_OBJ) || (dtqid > (ID)_kernel_systbl.qdtq.inf->limit))) {
        ercd = E_ID;
    } else if ((tmout != TMO_FEVR) && (tmout < 0)) {
        ercd = E_PAR;

    } else if (_kernel_systbl.icnt != 0U) {
        if (tmout != TMO_POL) {
            ercd = E_CTX;
        } else {
            dtq = _kernel_systbl.qdtq.dtq[dtqid];
            if (dtq == 0) {
                ercd = E_NOEXS;
            } else if ((dtq->dtqcnt == 0U) && (fmode != FALSE)) {
                ercd = E_ILUSE;
            } else {
                ssb = _kernel_getssb();
                if (ssb == 0) {
                    ercd = E_NOMEM;
                } else {
                    ssb->stat.catr = TA_SSB;
                    ssb->p1 = (VP_INT)dtq;
                    ssb->p2 = (VP_INT)fmode;
                    ssb->p3 = (fmode==FALSE)?(VP_INT)TFN_PSND_DTQ:(VP_INT)TFN_FSND_DTQ;
                    ssb->p4 = (VP_INT)dtqid;
                    ssb->p5 = (VP_INT)data;
                    ssb->p6 = (VP_INT)tmout;
                    ssb->sysfunc = (FP)&_kernel_snddtq_1;
                    _kernel_enqssb(ssb);
                    ercd = E_OK;
                }
            }
        }
    } else if (_kernel_systbl.cssb == 0) {
        _kernel_systbl.stat.s.dlock = (UB)TRUE;
        dtq = _kernel_systbl.qdtq.dtq[dtqid];
        if (dtq == 0) {
            _kernel_relrun();
            ercd = E_NOEXS;
        } else if ((dtq->dtqcnt == 0U) && (fmode != FALSE)) {
            _kernel_relrun();
            ercd = E_ILUSE;
        } else {
            tcb = _kernel_systbl.ctcb;
            tcb->p1 = (VP_INT)dtq;
            tcb->p2 = (VP_INT)fmode;
            tcb->p4 = (VP_INT)tcb;
            tcb->p5 = (VP_INT)data;
            tcb->p6 = (VP_INT)tmout;
            tcb->sysfunc = (FP)&_kernel_snddtq_1;
            _kernel_entsys(FALSE);
            ercd = (ER)tcb->p1;
        }
    } else {
        if (tmout != TMO_POL) {
            ercd = E_CTX;
        } else {
            dtq = _kernel_systbl.qdtq.dtq[dtqid];
            if (dtq == 0) {
                ercd = E_NOEXS;
            } else if ((dtq->dtqcnt == 0U) && (fmode != FALSE)) {
                ercd = E_ILUSE;
            } else {
                ercd = _kernel_snddtq_2(dtq, data, tmout, fmode, 0);
            }
        }
    }
    return ercd;
}

ER tsnd_dtq(ID dtqid, VP_INT data, TMO tmout)
{
    return _kernel_snddtq(dtqid, data, tmout, FALSE);
}


ER fsnd_dtq(ID dtqid, VP_INT data)
{
    return _kernel_snddtq(dtqid, data, (TMO)TMO_POL, TRUE);
}
