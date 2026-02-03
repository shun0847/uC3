/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created
            2017.01.26: Fixed the IPA warnings.
            2017.09.01: Fixed the C++test StaticAnalysis warnings.
 ***************************************************************************/

#include "uC3sys.h"


static BOOL _kernel_deltsk(T_SSB *par, BOOL retcd);

/***********************************
    Delete Task
 ***********************************/

static BOOL _kernel_deltsk(T_SSB *par, BOOL retcd)
{
    T_TCB *tcb;

    tcb = (T_TCB *)par->p1;
    if (tcb->stat.msts != TTS_DMT) {
        par->p1 = (VP_INT)E_OBJ;
    } else {
        _kernel_systbl.qtcb.tcb[tcb->tskid] = 0;
        _kernel_systbl.qtcb.inf->usedc--;
        par->p1 = (VP_INT)E_OK;
    }
    return retcd;
}

ER del_tsk(ID tskid)
{
    T_TCB *tcb;
    T_TCB *ttcb;
    T_MEM **mem_base;
    ER ercd;

    if ((_kernel_systbl.icnt != 0U) ||
        (_kernel_systbl.cssb != 0)) {
        ercd = E_CTX;
    } else if ((tskid > (ID)_kernel_systbl.qtcb.inf->limit) ||
               (tskid < TMIN_TSK)) {
        ercd = E_ID;
    } else {
        _kernel_systbl.stat.s.dlock = (UB)TRUE;
        ttcb = _kernel_systbl.qtcb.tcb[tskid];
        if (ttcb == 0) {
            ercd = E_NOEXS;
        } else {
            tcb = _kernel_systbl.ctcb;
            tcb->p1 = (VP_INT)ttcb;
            tcb->sysfunc = (FP)&_kernel_deltsk;
            _kernel_entsys(TRUE);
            ercd = (ER)tcb->p1;

            if (ercd == E_OK) { /* parasoft-suppress BD-PB-CC "2017/09/01 Reviewed" */
                if ((ttcb->stat.oatr & TA_USTK) == 0U) {
                    if (_kernel_systbl.free_stk != (T_MEM *)1) {
                        mem_base = &_kernel_systbl.free_stk;
                    } else {
                        mem_base = &_kernel_systbl.free_sys;
                    }
                    (void)_kernel_relmem(mem_base, (T_MEM *)ttcb->stk, ttcb->stksz);
                }
                (void)_kernel_relmem(&_kernel_systbl.free_sys, (T_MEM *)ttcb, sizeof(T_TCB));
            }
        }
        _kernel_relrun();
    }
    return ercd;
}
