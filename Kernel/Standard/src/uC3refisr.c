/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created
            2017.01.26: Fixed the IPA warnings.
 ***************************************************************************/

#include "uC3sys.h"


static ER _kernel_refisr(ID isrid, T_RISR *pk_risr);

/***********************************************
    Reference Interrupt Service Routine State
 ***********************************************/

static ER _kernel_refisr(ID isrid, T_RISR *pk_risr)
{
    T_ISR *isr;
    ER ercd;

    isr = _kernel_systbl.qisr.isr[isrid];
    if (isr == 0) {
        ercd = E_NOEXS;
    } else {
        pk_risr->intno = isr->intno;
        pk_risr->isr = isr->isr;
        ercd = E_OK;
    }
    return ercd;
}

ER ref_isr(ID isrid, T_RISR *pk_risr)
{
    ER ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if (( _kernel_systbl.qisr.inf == 0) ||
               ((isrid < TMIN_OBJ) || (isrid > (ID)_kernel_systbl.qisr.inf->limit))) {
        ercd = E_ID;

    } else {
        if (_kernel_systbl.cssb == 0) {
            _kernel_systbl.stat.s.dlock = (UB)TRUE;
            ercd = _kernel_refisr(isrid, pk_risr);
            _kernel_relrun();
        } else {
            ercd = _kernel_refisr(isrid, pk_risr);
        }
    }
    return ercd;
}
