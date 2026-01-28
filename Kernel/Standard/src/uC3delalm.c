/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created
            2017.01.26: Fixed the IPA warnings.
            2017.09.01: Fixed the C++test StaticAnalysis warnings.
 ***************************************************************************/

#include "uC3sys.h"


static BOOL _kernel_delalm(T_SSB *par, BOOL retcd);

/***********************************
    Delete Alarm Handler
 ***********************************/

static BOOL _kernel_delalm(T_SSB *par, BOOL retcd)
{
    T_ALM *alm;

    alm = (T_ALM *)par->p1;
    if (alm->stat.msts == TALM_STA) {
        alm->sprev->snext = alm->snext;
        if (alm->snext != 0) {
            alm->snext->sprev = alm->sprev;
        }
    }
    _kernel_systbl.qalm.alm[alm->almid] = 0;
    _kernel_systbl.qalm.inf->usedc--;
    par->p1 = (VP_INT)E_OK;
    return retcd;
}

ER del_alm(ID almid)
{
    T_TCB *tcb;
    T_ALM *alm;
    ER ercd;

    if ((_kernel_systbl.icnt != 0U) ||
        (_kernel_systbl.cssb != 0)) {
        ercd = E_CTX;
    } else if (( _kernel_systbl.qalm.inf == 0) ||
               ((almid < TMIN_OBJ) || (almid > (ID)_kernel_systbl.qalm.inf->limit))) {
        ercd = E_ID;

    } else {
        _kernel_systbl.stat.s.dlock = (UB)TRUE;
        alm = _kernel_systbl.qalm.alm[almid];
        if (alm == 0) {
            ercd = E_NOEXS;
        } else {
            tcb = _kernel_systbl.ctcb;
            tcb->p1 = (VP_INT)alm;
            tcb->sysfunc = (FP)&_kernel_delalm;
            _kernel_entsys(TRUE);
            ercd = (ER)tcb->p1;

            if (ercd == E_OK) { /* parasoft-suppress BD-PB-CC "2017/09/01 Reviewed" */
                (void)_kernel_relmem(&_kernel_systbl.free_sys, (T_MEM *)alm, sizeof(T_ALM));
            }
        }
        _kernel_relrun();
    }
    return ercd;
}
