/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.14: Created
            2017.01.26: Fixed the IPA warnings.
 ***************************************************************************/

#include "uC3sys.h"
#include <string.h>


static BOOL _kernel_exttsk_1(T_SSB *par, BOOL retcd);
static ER _kernel_cretsk_1(ID tskid, T_CTSK *pk_ctsk);
static ER _kernel_cretsk(ID tskid, T_CTSK *pk_ctsk);

/***********************************
    Activate Task Sub
 ***********************************/

void _kernel_statsk_2(T_TCB *tcb, VP_INT stacd)
{
    tcb->sp = (T_REG *)((SIZE)tcb->stk + tcb->stksz);
    tcb->cpri = tcb->ipri;
    tcb->bpri = tcb->ipri;
    tcb->stat.msts = TTS_RDY;
    tcb->stat.wsts = 0U;
    tcb->wque = _kernel_systbl.qrdq.rdq;
    _kernel_setctx(tcb, stacd);
    _kernel_enqrdq(tcb);
}

ER _kernel_acttsk(T_TCB *tcb)
{
    ER ercd = E_OK;

    if (tcb->stat.msts == TTS_DMT) {
        _kernel_statsk_2(tcb, tcb->exinf);
    } else if (tcb->act < TMAX_ACTCNT) {
        tcb->act++;
    } else {
        ercd = E_QOVR;
    }
    return ercd;
}

BOOL _kernel_acttsk_1(T_SSB *par, BOOL retcd)
{
    par->p1 = (VP_INT)_kernel_acttsk((T_TCB *)par->p1);
    return retcd;
}

/***********************************
    Terminate Invoking Task Sub
 ***********************************/

static BOOL _kernel_exttsk_1(T_SSB *par, BOOL retcd)
{
    void (*umtxfunc)(T_TCB *);
    T_TCB *tcb;

    tcb = (T_TCB *)par->p1;

    umtxfunc = (void (*)(T_TCB *))_kernel_systbl.umtxfunc;
    if (umtxfunc != 0) {
        umtxfunc(tcb);
    }

    tcb->stat.msts = TTS_DMT;
    _kernel_deqtcb(&tcb->wtcb);
    _kernel_systbl.ctcb = 0;
    _kernel_systbl.stat.dspint = 0UL;

    if (tcb->act > 0U) {
        tcb->act--;
        _kernel_statsk_2(tcb, tcb->exinf);
    }
    (void)_kernel_setims((IMASK)0);
    return retcd;
}

void ext_tsk(void)
{
    T_TCB *tcb;

    if ((_kernel_systbl.icnt == 0U) && (_kernel_systbl.cssb == 0))
    {
        tcb = _kernel_systbl.ctcb;
        tcb->p1 = (VP_INT)tcb;
        tcb->sysfunc = (FP)&_kernel_exttsk_1;
        _kernel_entsys(FALSE);
    }
}

/***********************************
    Create Task
 ***********************************/

static ER _kernel_cretsk_1(ID tskid, T_CTSK *pk_ctsk)
{
    T_TCB *tcb;
    T_TCB *tcb1;
    T_MEM **mem_base;
    ER ercd;
    void (*func)(INT *, SIZE, ID);

    if (tskid == 0) {
        if (_kernel_systbl.qtcb.inf->limit == _kernel_systbl.qtcb.inf->usedc) {
            ercd = E_NOID;
        } else {
            for(tskid = (ID)_kernel_systbl.qtcb.inf->limit; tskid >= TMIN_TSK; tskid--) {
                if (_kernel_systbl.qtcb.tcb[tskid] == 0) {
                    break;
                }
            }
            ercd = (ER)tskid;
        }
    } else {
        if (_kernel_systbl.qtcb.tcb[tskid] != 0) {
            ercd = E_OBJ;
        } else {
            ercd = E_OK;
        }
    }

    if (ercd >= E_OK) {
        tcb1 = (T_TCB *)_kernel_getmem(&_kernel_systbl.free_sys, sizeof(T_TCB));
        if (tcb1 == 0) {
            ercd = E_NOMEM;
        } else {
            memset((void *)tcb1, 0x00, sizeof(T_TCB));
            if (pk_ctsk->stk == 0) {
                if (_kernel_systbl.free_stk != (T_MEM *)1) {
                    mem_base = &_kernel_systbl.free_stk;
                } else {
                    mem_base = &_kernel_systbl.free_sys;
                }
                tcb1->stk = (T_REG *)_kernel_getmem(mem_base, pk_ctsk->stksz);
                if (tcb1->stk == 0) {
                    (void)_kernel_relmem(&_kernel_systbl.free_sys, (T_MEM *)tcb1, sizeof(T_TCB));
                    ercd = E_NOMEM;
                } else {
                    tcb1->stat.oatr = (UB)pk_ctsk->tskatr;
                }
            } else {
                tcb1->stk = (T_REG *)pk_ctsk->stk;
                tcb1->stat.oatr = (UB)(pk_ctsk->tskatr | TA_USTK);
            }
        }
        if (ercd >= E_OK) {
            func = (void (*)(INT *, SIZE, ID))_kernel_systbl.inistk;
            if (func != 0) {
                func((INT *)tcb1->stk, pk_ctsk->stksz, tskid);
            }
            tcb1->name = pk_ctsk->name;
            tcb1->stksz = pk_ctsk->stksz;
            tcb1->tskid = (UH)tskid;
            tcb1->exinf = pk_ctsk->exinf;
            tcb1->task = pk_ctsk->task;
            tcb1->ipri = (UB)pk_ctsk->itskpri;
            tcb1->stat.msts = TTS_DMT;
            _kernel_systbl.qtcb.inf->usedc++;
            _kernel_systbl.qtcb.tcb[tskid] = tcb1;
            if ((tcb1->stat.oatr & TA_ACT) != 0U) {
                if (_kernel_systbl.cssb == 0) {
                    tcb = _kernel_systbl.ctcb;
                    tcb->p1 = (VP_INT)tcb1;
                    tcb->sysfunc = (FP)&_kernel_acttsk_1;
                    _kernel_entsys(FALSE);
                    if ((ER)tcb->p1 < E_OK) {
                        ercd = (ER)tcb->p1;
                    }
                } else {
                    _kernel_statsk_2(tcb1, tcb1->exinf);
                }
            }
        }
    }
    return ercd;
}

static ER _kernel_cretsk(ID tskid, T_CTSK *pk_ctsk)
{
    T_SSB *cssb;
    ER ercd;

    if ((pk_ctsk->itskpri < TMIN_TPRI) ||
        (pk_ctsk->itskpri > (PRI)_kernel_systbl.qrdq.inf->limit)) {
        ercd = E_PAR;
    } else {
        cssb = _kernel_systbl.cssb;
        if (cssb == 0) {
            _kernel_systbl.stat.s.dlock = (UB)TRUE;
            ercd = _kernel_cretsk_1(tskid, pk_ctsk);
            _kernel_relrun();
        } else if (cssb->stat.catr == TA_INI) {
            ercd = _kernel_cretsk_1(tskid, pk_ctsk);
        } else {
            ercd = E_CTX;
        }
    }
    return ercd;
}

ER cre_tsk(ID tskid, T_CTSK *pk_ctsk)
{
    ER ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if ((tskid < TMIN_TSK) || (tskid > (ID)_kernel_systbl.qtcb.inf->limit)) {
        ercd = E_ID;
    } else {
        ercd = _kernel_cretsk(tskid, pk_ctsk);
    }
    return ercd;
}

ER_ID acre_tsk(T_CTSK *pk_ctsk)
{
    ER_ID ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else {
        ercd = (ER_ID)_kernel_cretsk(0, pk_ctsk);
    }
    return ercd;
}
