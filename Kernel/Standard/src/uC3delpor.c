/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created.
            2012.08.24: Corrected the dequeuing of timer queue.
            2017.01.26: Fixed the IPA warnings.
            2017.09.01: Fixed the C++test StaticAnalysis warnings.
 ***************************************************************************/

#include "uC3sys.h"


static BOOL _kernel_delpor(T_SSB *par, BOOL retcd);

/***********************************
    Delete Rendezvous Port
 ***********************************/

static BOOL _kernel_delpor(T_SSB *par, BOOL retcd)
{
    T_TCB *tcb;
    T_POR *por;
    T_WTCB *wtcb;
    PRI primax;

    por = (T_POR *)par->p1;
    if ((por->poratr & TA_TPRI) != 0U) {
        primax = (PRI)_kernel_systbl.qrdq.inf->limit;
        wtcb = por->cque.mwait;
    } else {
        primax = 0;
        wtcb = &por->cque.wait;
    }
    for(tcb = _kernel_gettcb(wtcb, primax); tcb != 0; tcb = _kernel_gettcb(wtcb, primax)) {
        _kernel_deqtcb(&tcb->wtcb);
        if ((tcb->stat.msts & TTS_TMR) != 0U) {
            tcb->sprev->snext = (T_SSB *)tcb->snext;
            if (tcb->snext != 0) {
                tcb->snext->sprev = (T_SSB *)tcb->sprev;
            }
        }
        tcb->p1 = (VP_INT)E_DLT;
        _kernel_enqrdq(tcb);
    }

    for(tcb = _kernel_gettcb(&por->aque, 0); tcb != 0; tcb = _kernel_gettcb(&por->aque, 0)) {
        _kernel_deqtcb(&tcb->wtcb);
        if ((tcb->stat.msts & TTS_TMR) != 0U) {
            tcb->sprev->snext = (T_SSB *)tcb->snext;
            if (tcb->snext != 0) {
                tcb->snext->sprev = (T_SSB *)tcb->sprev;
            }
        }
        tcb->p1 = (VP_INT)E_DLT;
        _kernel_enqrdq(tcb);
    }

    _kernel_systbl.qpor.por[por->porid] = 0;
    _kernel_systbl.qpor.inf->usedc--;

    par->p1 = (VP_INT)E_OK;
    return retcd;
}

ER del_por(ID porid)
{
    T_TCB *tcb;
    T_POR *por;
    ER ercd;

    if ((_kernel_systbl.icnt != 0U) ||
        (_kernel_systbl.cssb != 0)) {
        ercd = E_CTX;
    } else if (( _kernel_systbl.qpor.inf == 0) ||
               ((porid < TMIN_OBJ) || (porid > (ID)_kernel_systbl.qpor.inf->limit))) {
        ercd = E_ID;

    } else {
        _kernel_systbl.stat.s.dlock = (UB)TRUE;
        por = _kernel_systbl.qpor.por[porid];
        if (por == 0) {
            ercd = E_NOEXS;
        } else {
            tcb = _kernel_systbl.ctcb;
            tcb->p1 = (VP_INT)por;
            tcb->sysfunc = (FP)&_kernel_delpor;
            _kernel_entsys(TRUE);
            ercd = (ER)tcb->p1;

            if (ercd == E_OK) { /* parasoft-suppress BD-PB-CC "2017/09/01 Reviewed" */
                if ((por->poratr & TA_TPRI) != 0U) {
                    (void)_kernel_relmem(&_kernel_systbl.free_sys, (T_MEM *)por->cque.mwait+1,
                                    _kernel_systbl.qrdq.inf->limit * sizeof(T_WTCB));
                }
                (void)_kernel_relmem(&_kernel_systbl.free_sys, (T_MEM *)por, sizeof(T_POR));
            }
        }
        _kernel_relrun();
    }
    return ercd;
}
