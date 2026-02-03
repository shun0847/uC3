/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2021, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created
            2009.03.31: Corrected the operation of timer-queue.
            2017.03.02: Fixed the IPA warnings.
            2017.07.19: Corrected the cycstat value.
            2021.01.27: Fixed C++test warnings.
 ***************************************************************************/

#include "uC3sys.h"


static ER _kernel_refcyc(ID cycid, T_RCYC *pk_rcyc);

/***********************************
    Reference Cyclic Handler State
 ***********************************/

static ER _kernel_refcyc(ID cycid, T_RCYC *pk_rcyc)
{
    T_CYC *cyc;
    RELTIM lefttim;
    ER ercd;

    cyc = _kernel_systbl.qcyc.cyc[cycid];
    if (cyc == 0) {
        ercd = E_NOEXS;
    } else {
        pk_rcyc->cycstat = cyc->stat.msts & (TCYC_STA|TCYC_STP);
        if ((cyc->stat.msts & TCYC_STA) != 0U) {
            lefttim = cyc->stime.ltime - _kernel_systbl.systim.ltime - 1U;
            pk_rcyc->lefttim = lefttim - (lefttim % (RELTIM)_kernel_systbl.tick);
        } else {
            pk_rcyc->lefttim = (RELTIM)TMO_FEVR;
        }
        ercd = E_OK;
    }
    return ercd;
}

ER ref_cyc(ID cycid, T_RCYC *pk_rcyc)
{
    ER ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if (( _kernel_systbl.qcyc.inf == 0) ||
               ((cycid < TMIN_OBJ) || (cycid > (ID)_kernel_systbl.qcyc.inf->limit))) {
        ercd = E_ID;

    } else if (_kernel_systbl.cssb == 0) {
        _kernel_lock();
        ercd = _kernel_refcyc(cycid, pk_rcyc);
        _kernel_unlock();
    } else {
        ercd = _kernel_refcyc(cycid, pk_rcyc);
    }
    return ercd;
}
