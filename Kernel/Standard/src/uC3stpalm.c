/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created
            2009.03.31: Corrected the operation of timer-queue.
            2017.01.26: Fixed the IPA warnings.
 ***************************************************************************/

#include "uC3sys.h"


static ER _kernel_stpalm(T_ALM *alm);
static BOOL _kernel_stpalm_1(T_SSB *par, BOOL retcd);

/***************************************
    Stop Alarm Handler Operation
 ***************************************/

static ER _kernel_stpalm(T_ALM *alm)
{
    if ((alm->stat.msts & TALM_STA) != 0U) {
        if ((alm->stat.msts & TALM_PND) == 0U) {
            alm->sprev->snext = alm->snext;
            if (alm->snext != 0) {
                alm->snext->sprev = alm->sprev;
            }
        }
        alm->stat.msts &= ~(TALM_STA|TALM_RST);
    }
    return E_OK;
}

static BOOL _kernel_stpalm_1(T_SSB *par, BOOL retcd)
{
    par->p1 = (VP_INT)_kernel_stpalm((T_ALM *)par->p1);
    return retcd;
}

ER stp_alm(ID almid)
{
    T_TCB *tcb;
    T_ALM *alm;
    ER ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if (( _kernel_systbl.qalm.inf == 0) ||
               ((almid < TMIN_OBJ) || (almid > (ID)_kernel_systbl.qalm.inf->limit))) {
        ercd = E_ID;

    } else if (_kernel_systbl.cssb == 0) {
        _kernel_systbl.stat.s.dlock = (UB)TRUE;
        alm = _kernel_systbl.qalm.alm[almid];
        if (alm == 0) {
            _kernel_relrun();
            ercd = E_NOEXS;
        } else {
            tcb = _kernel_systbl.ctcb;
            tcb->p1 = (VP_INT)alm;
            tcb->sysfunc = (FP)&_kernel_stpalm_1;
            _kernel_entsys(FALSE);
            ercd = (ER)tcb->p1;
        }
    } else {
        alm = _kernel_systbl.qalm.alm[almid];
        if (alm == 0) {
            ercd = E_NOEXS;
        } else {
            ercd = _kernel_stpalm(alm);
        }
    }
    return ercd;
}
