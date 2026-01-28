/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created
            2017.01.26: Fixed the IPA warnings.
 ***************************************************************************/

#include "uC3sys.h"


static ER_UINT _kernel_canact(ID tskid);

/***********************************
    Cancel Task Activation Requests
 ***********************************/

static ER_UINT _kernel_canact(ID tskid)
{
    T_TCB *tcb;
    ER_UINT ercd;

    tcb = _kernel_systbl.qtcb.tcb[tskid];
    if (tcb == 0) {
        ercd = E_NOEXS;
    } else {
        ercd = (ER_UINT)tcb->act;
        tcb->act = 0U;
    }
    return ercd;
}

ER_UINT can_act(ID tskid)
{
    ER_UINT ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if ((tskid > (ID)_kernel_systbl.qtcb.inf->limit) ||
               (tskid < TSK_SELF)) {
        ercd = E_ID;

    } else if (_kernel_systbl.cssb == 0) {
        if (tskid == TSK_SELF) {
            tskid = (ID)_kernel_systbl.ctcb->tskid;
        }
        _kernel_lock();
        ercd = _kernel_canact(tskid);
        _kernel_unlock();
    } else {
        if (tskid < TMIN_TSK) {
            ercd = E_ID;
        } else {
            ercd = _kernel_canact(tskid);
        }
    }
    return ercd;
}
