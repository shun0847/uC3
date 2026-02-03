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
                         - If p_data is NULL, an E_PAR error.
 ***************************************************************************/

#include "uC3sys.h"


static void _kernel_rcvdtq_2(T_TCB *tcb);
static void _kernel_rcvdtq_3(T_WTCB *wtcb, PRI primax, T_DTQ *dtq, VP_INT *p_data);
static ER _kernel_rcvdtq(T_DTQ *dtq, VP_INT *p_data, TMO tmout, T_TCB *tcb);
static BOOL _kernel_rcvdtq_1(T_SSB *par, BOOL retcd);

/***********************************
    Receive from Data Queue
 ***********************************/

static void _kernel_rcvdtq_3(T_WTCB *wtcb, PRI primax, T_DTQ *dtq, VP_INT *p_data)
{
    T_TCB *ttcb;
    UH get;

    if (dtq->put >= dtq->cnt) {
        get = (UH)(dtq->put - dtq->cnt);
    } else {
        get = (UH)((UH)(dtq->put - dtq->cnt) + dtq->dtqcnt);
    }
    *p_data = dtq->dtq[get];
    dtq->cnt--;
    ttcb = _kernel_gettcb(wtcb, primax);
    if (ttcb != 0) {
        dtq->cnt++;
        dtq->dtq[dtq->put++] = (VP_INT)ttcb->p5;
        if (dtq->put >= dtq->dtqcnt) {
            dtq->put = 0U;
        }
        _kernel_rcvdtq_2(ttcb);
    }
}

static void _kernel_rcvdtq_2(T_TCB *tcb)
{
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

static ER _kernel_rcvdtq(T_DTQ *dtq, VP_INT *p_data, TMO tmout, T_TCB *tcb)
{
    T_TCB *ttcb;
    T_WTCB *wtcb;
    PRI primax;
    ER ercd;

    if ((dtq->dtqatr & TA_TPRI) != 0U) {
        primax = (PRI)_kernel_systbl.qrdq.inf->limit;
        wtcb = dtq->sque.mwait;
    } else {
        primax = 0;
        wtcb = &dtq->sque.wait;
    }

    if (tmout == TMO_POL) {
        if (dtq->cnt > 0U) {
            _kernel_rcvdtq_3(wtcb, primax, dtq, p_data);
            ercd = E_OK;
        } else {
            ttcb = _kernel_gettcb(wtcb, primax);
            if (ttcb != 0) {
                *p_data = (VP_INT)ttcb->p5;
                _kernel_rcvdtq_2(ttcb);
                ercd = E_OK;
            } else {
                ercd = E_TMOUT;
            }
        }
    } else if (_kernel_systbl.stat.dspint == 0UL) {
        if (dtq->cnt > 0U) {
            _kernel_rcvdtq_3(wtcb, primax, dtq, p_data);
        } else {
            ttcb = _kernel_gettcb(wtcb, primax);
            if (ttcb != 0) {
                *p_data = (VP_INT)ttcb->p5;
                _kernel_rcvdtq_2(ttcb);
            } else {
                _kernel_deqtcb(&tcb->wtcb);
                tcb->stat.msts = TTS_WAI | TTS_FIFO;
                tcb->stat.wsts = TS_RDTQ;
                tcb->wobjid = dtq->dtqid;
                tcb->wque = &dtq->que;
                _kernel_enqtcb(&tcb->wque[0], &tcb->wtcb);
                if (tmout != TMO_FEVR) {
                    _kernel_enqtim(tcb, &_kernel_systbl.systim, (RELTIM)tmout);
                    tcb->stat.msts |= TTS_TMR;
                }
                _kernel_systbl.ctcb = 0;
            }
        }
        ercd = E_OK;
    } else {
        ercd = E_CTX;
    }
    return ercd;
}

static BOOL _kernel_rcvdtq_1(T_SSB *par, BOOL retcd)
{
    par->p1 = (VP_INT)_kernel_rcvdtq((T_DTQ *)par->p1, (VP_INT *)par->p2,
                             (TMO)par->p3, (T_TCB *)par->p4);
    return retcd;
}

ER trcv_dtq(ID dtqid, VP_INT *p_data, TMO tmout)
{
    T_TCB *tcb;
    T_DTQ *dtq;
    ER ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if (( _kernel_systbl.qdtq.inf == 0) ||
               ((dtqid < TMIN_OBJ) || (dtqid > (ID)_kernel_systbl.qdtq.inf->limit))) {
        ercd = E_ID;
    } else if ((tmout != TMO_FEVR) && (tmout < 0)) {
        ercd = E_PAR;
    } else if (p_data == 0) {
        ercd = E_PAR;

    } else if (_kernel_systbl.cssb == 0) {
        _kernel_systbl.stat.s.dlock = (UB)TRUE;
        dtq = _kernel_systbl.qdtq.dtq[dtqid];
        if (dtq == 0) {
            _kernel_relrun();
            ercd = E_NOEXS;
        } else {
            tcb = _kernel_systbl.ctcb;
            tcb->p1 = (VP_INT)dtq;
            tcb->p2 = (VP_INT)p_data;
            tcb->p3 = (VP_INT)tmout;
            tcb->p4 = (VP_INT)tcb;
            tcb->sysfunc = (FP)&_kernel_rcvdtq_1;
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
            } else {
                ercd = _kernel_rcvdtq(dtq, p_data, tmout, 0);
            }
        }
    }
    return ercd;
}
