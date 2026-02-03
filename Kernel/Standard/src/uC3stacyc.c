/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created
            2009.03.31: Corrected the operation of timer-queue.
            2009.07.20: Re-corrected the operation of timer-queue.
            2017.01.26: Fixed the IPA warnings.
            2017.07.19: Deleted TMO cast when calling _kernel_enqtim.
 ***************************************************************************/

#include "uC3sys.h"


static ER _kernel_stacyc(T_CYC *cyc);
static BOOL _kernel_stacyc_1(T_SSB *par, BOOL retcd);

/*******************************************
    Start Cyclic Handler Operation
 *******************************************/

static ER _kernel_stacyc(T_CYC *cyc)
{
    if (((cyc->stat.oatr & TA_PHS)   == 0U) &&
        ((cyc->stat.msts & TCYC_PND) == 0U)) {
        if ((cyc->stat.msts & TCYC_STA) == 0U) {
            cyc->stat.msts |= TCYC_STA;
        } else {
            cyc->sprev->snext = cyc->snext;
            if (cyc->snext != 0) {
                cyc->snext->sprev = cyc->sprev;
            }
        }
        _kernel_enqtim((T_TCB *)cyc, &_kernel_systbl.systim, cyc->cyctim);
    } else {
        if ((cyc->stat.msts & TCYC_STA) == 0U) {
            cyc->stat.msts |= TCYC_STA;
        }
    }
    return E_OK;
}

static BOOL _kernel_stacyc_1(T_SSB *par, BOOL retcd)
{
    par->p1 = (VP_INT)_kernel_stacyc((T_CYC *)par->p1);
    return retcd;
}

ER sta_cyc(ID cycid)
{
    T_TCB *tcb;
    T_CYC *cyc;
    ER ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if (( _kernel_systbl.qcyc.inf == 0) ||
               ((cycid < TMIN_OBJ) || (cycid > (ID)_kernel_systbl.qcyc.inf->limit))) {
        ercd = E_ID;

    } else if (_kernel_systbl.cssb == 0) {
        _kernel_systbl.stat.s.dlock = (UB)TRUE;
        cyc = _kernel_systbl.qcyc.cyc[cycid];
        if (cyc == 0) {
            _kernel_relrun();
            ercd = E_NOEXS;
        } else {
            tcb = _kernel_systbl.ctcb;
            tcb->p1 = (VP_INT)cyc;
            tcb->sysfunc = (FP)&_kernel_stacyc_1;
            _kernel_entsys(FALSE);
            ercd = (ER)tcb->p1;
        }
    } else {
        cyc = _kernel_systbl.qcyc.cyc[cycid];
        if (cyc == 0) {
            ercd = E_NOEXS;
        } else {
            ercd = _kernel_stacyc(cyc);
        }
    }
    return ercd;
}
