/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created
            2017.01.26: Fixed the IPA warnings.
 ***************************************************************************/

#include "uC3sys.h"


static ER _kernel_getpri(ID tskid, PRI *p_tskpri);

/***********************************
    Reference Task Priority
 ***********************************/

static ER _kernel_getpri(ID tskid, PRI *p_tskpri)
{
    T_TCB *tcb;
    ER ercd;

    tcb = _kernel_systbl.qtcb.tcb[tskid];
    if (tcb == 0) {
        ercd = E_NOEXS;
    } else {
        if (tcb->stat.msts == TTS_DMT) {
            ercd = E_OBJ;
        } else {
            *p_tskpri = (PRI)tcb->cpri;
            ercd = E_OK;
        }
    }
    return ercd;
}

ER get_pri(ID tskid, PRI *p_tskpri)
{
    ER ercd;

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
        ercd = _kernel_getpri(tskid, p_tskpri);
        _kernel_unlock();
    } else {
        if (tskid < TMIN_TSK) {
            ercd = E_ID;
        } else {
            ercd = _kernel_getpri(tskid, p_tskpri);
        }
    }
    return ercd;
}
