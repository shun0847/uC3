/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created.
            2009.03.31: Corrected the operation of timer-queue.
            2009.07.20: Re-corrected the operation of timer-queue.
            2017.01.26: Fixed the IPA warnings.
            2017.07.19: Deleted TMO cast when calling _kernel_enqtim.
 ***************************************************************************/

#include "uC3sys.h"


static ER _kernel_staalm(T_ALM *alm, RELTIM almtim);
static BOOL _kernel_staalm_1(T_SSB *par, BOOL retcd);

/***************************************
    Start Alarm Handler Operation
 ***************************************/

static ER _kernel_staalm(T_ALM *alm, RELTIM almtim)
{
    if ((alm->stat.msts & TALM_PND) == 0U) {
        if ((alm->stat.msts & TALM_STA) != 0U) {
            alm->sprev->snext = alm->snext;
            if (alm->snext != 0) {
                alm->snext->sprev = alm->sprev;
            }
        }
        alm->stat.msts = TALM_STA;
        _kernel_enqtim((T_TCB *)alm, &_kernel_systbl.systim, almtim);
    } else {
        alm->stat.msts = (TALM_PND|TALM_RST);
        alm->stime.ltime = almtim;
    }
    return E_OK;
}

static BOOL _kernel_staalm_1(T_SSB *par, BOOL retcd)
{
    par->p1 = (VP_INT)_kernel_staalm((T_ALM *)par->p1, (RELTIM)par->p2);
    return retcd;
}

ER sta_alm(ID almid, RELTIM almtim)
{
    T_TCB *tcb;
    T_ALM *alm;
    ER ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if (( _kernel_systbl.qalm.inf == 0) ||
               ((almid < TMIN_OBJ) || (almid > (ID)_kernel_systbl.qalm.inf->limit))) {
        ercd = E_ID;
    } else if (almtim == 0UL) {
        ercd = E_PAR;

    } else if (_kernel_systbl.cssb == 0) {
        _kernel_systbl.stat.s.dlock = (UB)TRUE;
        alm = _kernel_systbl.qalm.alm[almid];
        if (alm == 0) {
            _kernel_relrun();
            ercd = E_NOEXS;
        } else {
            tcb = _kernel_systbl.ctcb;
            tcb->p1 = (VP_INT)alm;
            tcb->p2 = (VP_INT)almtim;
            tcb->sysfunc = (FP)&_kernel_staalm_1;
            _kernel_entsys(FALSE);
            ercd = (ER)tcb->p1;
        }
    } else {
        alm = _kernel_systbl.qalm.alm[almid];
        if (alm == 0) {
            ercd = E_NOEXS;
        } else {
            ercd = _kernel_staalm(alm, almtim);
        }
    }
    return ercd;
}
