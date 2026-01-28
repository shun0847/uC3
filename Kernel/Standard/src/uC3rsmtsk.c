/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created
            2017.03.02: Fixed the IPA warnings.
 ***************************************************************************/

#include "uC3sys.h"


static ER _kernel_rsmtsk(T_TCB *tcb, BOOL flag);
static BOOL _kernel_rsmtsk_2(T_SSB *par, BOOL retcd);
static ER _kernel_rsmtsk_1(ID tskid, BOOL flag);

/***********************************
    Resume Suspended Task
 ***********************************/

static ER _kernel_rsmtsk(T_TCB *tcb, BOOL flag)
{
    ER ercd;

    if ((tcb->stat.msts & TTS_SUS) == 0U) {
        ercd = E_OBJ;

    } else {
        if (flag != FALSE) {
            tcb->sus = 0U;
        } else {
            tcb->sus--;
        }
        if (tcb->sus == 0U) {
            tcb->stat.msts &= (UB)~TTS_SUS;
            if ((tcb->stat.msts & TTS_WAI) == 0U) {
                _kernel_enqrdq(tcb);
            }
        }
        ercd = E_OK;
    }
    return ercd;
}

static BOOL _kernel_rsmtsk_2(T_SSB *par, BOOL retcd)
{
    par->p1 = (VP_INT)_kernel_rsmtsk((T_TCB *)par->p1, (BOOL)par->p2);
    return retcd;
}

static ER _kernel_rsmtsk_1(ID tskid, BOOL flag)
{
    T_TCB *tcb;
    T_TCB *ttcb;
    ER ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if ((tskid < TMIN_TSK) || (tskid > (ID)_kernel_systbl.qtcb.inf->limit)) {
        ercd = E_ID;

    } else if (_kernel_systbl.cssb == 0) {
        _kernel_systbl.stat.s.dlock = (UB)TRUE;
        ttcb = _kernel_systbl.qtcb.tcb[tskid];
        if (ttcb == 0) {
            _kernel_relrun();
            ercd = E_NOEXS;
        } else {
            tcb = _kernel_systbl.ctcb;
            tcb->p1 = (VP_INT)ttcb;
            tcb->p2 = (VP_INT)flag;
            tcb->sysfunc = (FP)&_kernel_rsmtsk_2;
            _kernel_entsys(FALSE);
            ercd = (ER)tcb->p1;
        }
    } else {
        ttcb = _kernel_systbl.qtcb.tcb[tskid];
        if (ttcb == 0) {
            ercd = E_NOEXS;
        } else {
            ercd = _kernel_rsmtsk(ttcb, flag);
        }
    }
    return ercd;
}

ER rsm_tsk(ID tskid)
{
    return _kernel_rsmtsk_1(tskid, FALSE);
}

ER frsm_tsk(ID tskid)
{
    return _kernel_rsmtsk_1(tskid, TRUE);
}
