/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2021, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created
            2009.03.31: Corrected the operation of timer-queue.
            2017.03.02: Fixed the IPA warnings.
            2017.08.08: Corrected the almstat value.
            2021.01.27: Fixed C++test warnings.
***************************************************************************/

#include "uC3sys.h"


static ER _kernel_refalm(ID almid, T_RALM *pk_ralm);

/***************************************
    Reference Alarm Handler State
 ***************************************/

static ER _kernel_refalm(ID almid, T_RALM *pk_ralm)
{
    T_ALM *alm;
    RELTIM lefttim;
    ER ercd;

    alm = _kernel_systbl.qalm.alm[almid];
    if (alm == 0) {
        ercd = E_NOEXS;
    } else {
        pk_ralm->almstat = alm->stat.msts & (TALM_STA|TALM_STP);
        if ((alm->stat.msts & TALM_STA) != 0U) {
            lefttim = alm->stime.ltime - _kernel_systbl.systim.ltime - 1U;
            pk_ralm->lefttim = lefttim - (lefttim % (RELTIM)_kernel_systbl.tick);
        } else if ((alm->stat.msts & TALM_RST) != 0U) {
            pk_ralm->lefttim = alm->stime.ltime;
        } else {
            pk_ralm->lefttim = (RELTIM)TMO_FEVR;
        }
        ercd = E_OK;
    }
    return ercd;
}

ER ref_alm(ID almid, T_RALM *pk_ralm)
{
    ER ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if (( _kernel_systbl.qalm.inf == 0) ||
               ((almid < TMIN_OBJ) || (almid > (ID)_kernel_systbl.qalm.inf->limit))) {
        ercd = E_ID;

    } else if (_kernel_systbl.cssb == 0) {
        _kernel_lock();
        ercd = _kernel_refalm(almid, pk_ralm);
        _kernel_unlock();
    } else {
        ercd = _kernel_refalm(almid, pk_ralm);
    }
    return ercd;
}
