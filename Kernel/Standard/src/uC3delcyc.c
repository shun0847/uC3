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


static BOOL _kernel_delcyc(T_SSB *par, BOOL retcd);

/***********************************
    Delete Cyclic Handler
 ***********************************/

static BOOL _kernel_delcyc(T_SSB *par, BOOL retcd)
{
    T_CYC *cyc;

    cyc = (T_CYC *)par->p1;
    if (((cyc->stat.oatr & TA_PHS) != 0U) ||
        (cyc->stat.msts == TCYC_STA)) {
        cyc->sprev->snext = cyc->snext;
        if (cyc->snext != 0) {
            cyc->snext->sprev = cyc->sprev;
        }
    }
    _kernel_systbl.qcyc.cyc[cyc->cycid] = 0;
    _kernel_systbl.qcyc.inf->usedc--;
    par->p1 = (VP_INT)E_OK;
    return retcd;
}

ER del_cyc(ID cycid)
{
    T_TCB *tcb;
    T_CYC *cyc;
    ER ercd;

    if ((_kernel_systbl.icnt != 0U) ||
        (_kernel_systbl.cssb != 0)) {
        ercd = E_CTX;
    } else if (( _kernel_systbl.qcyc.inf == 0) ||
               ((cycid < TMIN_OBJ) || (cycid > (ID)_kernel_systbl.qcyc.inf->limit))) {
        ercd = E_ID;

    } else {
        _kernel_systbl.stat.s.dlock = (UB)TRUE;
        cyc = _kernel_systbl.qcyc.cyc[cycid];
        if (cyc == 0) {
            ercd = E_NOEXS;
        } else {
            tcb = _kernel_systbl.ctcb;
            tcb->p1 = (VP_INT)cyc;
            tcb->sysfunc = (FP)&_kernel_delcyc;
            _kernel_entsys(TRUE);
            ercd = (ER)tcb->p1;

            if (ercd == E_OK) { /* parasoft-suppress BD-PB-CC "2017/09/01 Reviewed" */
                (void)_kernel_relmem(&_kernel_systbl.free_sys, (T_MEM *)cyc, sizeof(T_CYC));
            }
        }
        _kernel_relrun();
    }
    return ercd;
}
