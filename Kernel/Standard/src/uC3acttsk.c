/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.14: Created.
            2009.09.12: Modified for the erro-code handler.
            2017.01.26: Fixed the IPA warnings.
            2017.07.21: Fixed to be able to specify TSK_SELF from task context.
 ***************************************************************************/

#include "uC3sys.h"
#include <string.h>


/***********************************
    Activate Task
 ***********************************/

ER act_tsk(ID tskid)
{
    T_SSB *ssb;
    T_TCB *tcb;
    T_TCB *ttcb;
    ER ercd;

    if ((tskid > (ID)_kernel_systbl.qtcb.inf->limit) ||
        (tskid < TSK_SELF)) {
        ercd = E_ID;
    } else if (_kernel_systbl.icnt != 0U) {
        if (tskid == TSK_SELF) {
            ercd = E_ID;
        } else {
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
                    ssb->p3 = (VP_INT)TFN_ACT_TSK;
                    ssb->p4 = (VP_INT)tskid;
                    ssb->sysfunc = (FP)&_kernel_acttsk_1;
                    _kernel_enqssb(ssb);
                    ercd = E_OK;
                }
            }
        }
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
            tcb = _kernel_systbl.ctcb;
            tcb->p1 = (VP_INT)ttcb;
            tcb->sysfunc = (FP)&_kernel_acttsk_1;
            _kernel_entsys(FALSE);
            ercd = (ER)tcb->p1;
        }
    } else {
        if (tskid == TSK_SELF) {
            ercd = E_ID;
        } else {
            ttcb = _kernel_systbl.qtcb.tcb[tskid];
            if (ttcb == 0) {
                ercd = E_NOEXS;
            } else {
                ercd = _kernel_acttsk(ttcb);
            }
        }
    }
    return ercd;
}
