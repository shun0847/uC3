/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created
            2017.01.26: Fixed the IPA warnings.
 ***************************************************************************/

#include "uC3sys.h"


static ER _kernel_stpovr(ID tskid);

/***************************************
    Stop Overrun Handler Operation
 ***************************************/

static ER _kernel_stpovr(ID tskid)
{
    T_TCB *tcb;
    ER ercd;

    tcb = _kernel_systbl.qtcb.tcb[tskid];
    if (tcb == 0) {
        ercd = E_NOEXS;
    } else if (_kernel_systbl.ovr == 0) {
        ercd = E_OBJ;
    } else {
        tcb->ovrsts = TOVR_STP;
        tcb->ovrtim = 0U;
        ercd = E_OK;
    }
    return ercd;
}

ER stp_ovr(ID tskid)
{
    ER_UINT ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if ((tskid < TSK_SELF) || (tskid > (ID)_kernel_systbl.qtcb.inf->limit)) {
        ercd = E_ID;

    } else if (_kernel_systbl.cssb == 0) {
        if (tskid == TSK_SELF) {
            tskid = (ID)_kernel_systbl.ctcb->tskid;
        }
        _kernel_lock();
        ercd = _kernel_stpovr(tskid);
        _kernel_unlock();
    } else {
        if (tskid < TMIN_TSK) {
            ercd = E_ID;
        } else {
            ercd = _kernel_stpovr(tskid);
        }
    }
    return ercd;
}
