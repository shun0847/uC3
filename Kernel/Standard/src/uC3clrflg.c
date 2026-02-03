/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created
            2017.01.26: Fixed the IPA warnings.
 ***************************************************************************/

#include "uC3sys.h"


static ER _kernel_clrflg(ID flgid, FLGPTN clrptn);

/***********************************
    Clear Eventflag
 ***********************************/

static ER _kernel_clrflg(ID flgid, FLGPTN clrptn)
{
    T_FLG *flg;
    ER ercd;

    flg = _kernel_systbl.qflg.flg[flgid];
    if (flg == 0) {
        ercd = E_NOEXS;
    } else {
        flg->flgptn &= clrptn;
        ercd = E_OK;
    }
    return ercd;
}

ER clr_flg(ID flgid, FLGPTN clrptn)
{
    ER ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if ((_kernel_systbl.qflg.inf == 0) ||
               ((flgid > (ID)_kernel_systbl.qflg.inf->limit) || (flgid < TMIN_OBJ))) {
        ercd = E_ID;

    } else if (_kernel_systbl.cssb == 0) {
        _kernel_lock();
        ercd = _kernel_clrflg(flgid, clrptn);
        _kernel_unlock();
    } else {
        ercd = _kernel_clrflg(flgid, clrptn);
    }
    return ercd;
}
