/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created
            2017.01.26: Fixed the IPA warnings.
 ***************************************************************************/

#include "uC3sys.h"


static ER _kernel_refdtq(T_DTQ *dtq, T_RDTQ *pk_rdtq);
static BOOL _kernel_refdtq_1(T_SSB *par, BOOL retcd);

/***************************************
    Reference Data Queue State
 ***************************************/

static ER _kernel_refdtq(T_DTQ *dtq, T_RDTQ *pk_rdtq)
{
    T_TCB *tcb;
    T_WTCB *wtcb;
    PRI primax;

    pk_rdtq->sdtqcnt = (UINT)dtq->cnt;
    if ((dtq->dtqatr & TA_TPRI) != 0U) {
        primax = (PRI)_kernel_systbl.qrdq.inf->limit;
        wtcb = dtq->sque.mwait;
    } else {
        primax = 0;
        wtcb = &dtq->sque.wait;
    }
    tcb = _kernel_gettcb(wtcb, primax);
    if (tcb != 0) {
        pk_rdtq->stskid = (ID)tcb->tskid;
    } else {
        pk_rdtq->stskid = TSK_NONE;
    }
    tcb = _kernel_gettcb(&dtq->que, 0);
    if (tcb != 0) {
        pk_rdtq->rtskid = (ID)tcb->tskid;
    } else {
        pk_rdtq->rtskid = TSK_NONE;
    }
    return E_OK;
}

static BOOL _kernel_refdtq_1(T_SSB *par, BOOL retcd)
{
    par->p1 = (VP_INT)_kernel_refdtq((T_DTQ *)par->p1, (T_RDTQ *)par->p2);
    return retcd;
}

ER ref_dtq(ID dtqid, T_RDTQ *pk_rdtq)
{
    T_TCB *tcb;
    T_DTQ *dtq;
    ER ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if (( _kernel_systbl.qdtq.inf == 0) ||
               ((dtqid < TMIN_OBJ) || (dtqid > (ID)_kernel_systbl.qdtq.inf->limit))) {
        ercd = E_ID;

    } else if (_kernel_systbl.cssb == 0) {
        _kernel_systbl.stat.s.dlock = (UB)TRUE;
        dtq = _kernel_systbl.qdtq.dtq[dtqid];
        if (dtq == 0) {
            _kernel_relrun();
            ercd = E_NOEXS;
        } else {
            tcb = _kernel_systbl.ctcb;
            tcb->p1 = (VP_INT)dtq;
            tcb->p2 = (VP_INT)pk_rdtq;
            tcb->sysfunc = (FP)&_kernel_refdtq_1;
            _kernel_entsys(FALSE);
            ercd = (ER)tcb->p1;
        }
    } else {
        dtq = _kernel_systbl.qdtq.dtq[dtqid];
        if (dtq == 0) {
            ercd = E_NOEXS;
        } else {
            ercd = _kernel_refdtq(dtq, pk_rdtq);
        }
    }
    return ercd;
}
