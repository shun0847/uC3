/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created
            2017.03.02: Fixed the IPA warnings.
 ***************************************************************************/

#include "uC3sys.h"


static void _kernel_refrdv(ID tskid, T_RRDV *pk_rrdv);

/***************************************
    Reference Rendezvous State
 ***************************************/

static void _kernel_refrdv(ID tskid, T_RRDV *pk_rrdv)
{
    T_TCB *tcb;

    tcb = _kernel_systbl.qtcb.tcb[tskid];
    if (tcb != 0) {
        if (((tcb->stat.msts & TTS_WAI) == TTS_WAI) &&
            ( tcb->stat.wsts == TS_RDV)) {
            pk_rrdv->wtskid = tskid;
        }
    }
}

ER ref_rdv(RDVNO rdvno, T_RRDV *pk_rrdv)
{
    ER ercd;
    ID tskid;

    tskid = (ID)rdvno >> 16;
    pk_rrdv->wtskid = TSK_NONE;
    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if ((tskid > (ID)_kernel_systbl.qtcb.inf->limit) ||
               (tskid < TMIN_TSK)) {
        ercd = E_OK;

    } else if (_kernel_systbl.cssb == 0) {
        _kernel_lock();
        _kernel_refrdv(tskid, pk_rrdv);
        _kernel_unlock();
        ercd = E_OK;
    } else {
        _kernel_refrdv(tskid, pk_rrdv);
        ercd = E_OK;
    }
    return ercd;
}
