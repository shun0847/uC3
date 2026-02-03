/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created
            2017.01.26: Fixed the IPA warnings.
 ***************************************************************************/

#include "uC3sys.h"


static ER _kernel_gettim(SYSTIM *p_systim);

/***********************************
    Refernce System Time
 ***********************************/

static ER _kernel_gettim(SYSTIM *p_systim)
{
    p_systim->ltime = _kernel_systbl.systim.ltime + _kernel_systbl.diftim.ltime;
    p_systim->utime = _kernel_systbl.systim.utime + _kernel_systbl.diftim.utime;
    if (_kernel_systbl.systim.ltime > p_systim->ltime) {
        p_systim->utime++;
    }
    return E_OK;
}

ER get_tim(SYSTIM *p_systim)
{
    ER ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;

    } else if (_kernel_systbl.cssb == 0) {
        _kernel_lock();
        ercd = _kernel_gettim(p_systim);
        _kernel_unlock();
    } else {
        ercd = _kernel_gettim(p_systim);
    }
    return ercd;
}
