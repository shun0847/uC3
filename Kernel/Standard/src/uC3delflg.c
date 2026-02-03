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


static BOOL _kernel_delflg(T_SSB *par, BOOL retcd);

/***********************************
    Delete Eventflag
 ***********************************/

static BOOL _kernel_delflg(T_SSB *par, BOOL retcd)
{
    T_TCB *tcb;
    T_FLG *flg;
    T_WTCB *wtcb;
    PRI primax;

    flg = (T_FLG *)par->p1;

    if ((flg->flgatr & (TA_TPRI|TA_WMUL)) == (TA_TPRI|TA_WMUL)) {
        primax = (PRI)_kernel_systbl.qrdq.inf->limit;
        wtcb = flg->que.mwait;
    } else {
        primax = 0;
        wtcb = &flg->que.wait;
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

    _kernel_systbl.qflg.flg[flg->flgid] = 0;
    _kernel_systbl.qflg.inf->usedc--;

    par->p1 = (VP_INT)E_OK;
    return retcd;
}

ER del_flg(ID flgid)
{
    T_TCB *tcb;
    T_FLG *flg;
    ER ercd;

    if ((_kernel_systbl.icnt != 0U) ||
        (_kernel_systbl.cssb != 0)) {
        ercd = E_CTX;
    } else if (( _kernel_systbl.qflg.inf == 0) ||
               ((flgid < TMIN_OBJ) || (flgid > (ID)_kernel_systbl.qflg.inf->limit))) {
        ercd = E_ID;

    } else {
        _kernel_systbl.stat.s.dlock = (UB)TRUE;
        flg = _kernel_systbl.qflg.flg[flgid];
        if (flg == 0) {
            ercd = E_NOEXS;
        } else {
            tcb = _kernel_systbl.ctcb;
            tcb->p1 = (VP_INT)flg;
            tcb->sysfunc = (FP)&_kernel_delflg;
            _kernel_entsys(TRUE);
            ercd = (ER)tcb->p1;

            if (ercd == E_OK) { /* parasoft-suppress BD-PB-CC "2017/09/01 Reviewed" */
                if ((flg->flgatr & (TA_TPRI|TA_WMUL)) == (TA_TPRI|TA_WMUL)) {
                    (void)_kernel_relmem(&_kernel_systbl.free_sys, (T_MEM *)flg->que.mwait+1,
                                    _kernel_systbl.qrdq.inf->limit * sizeof(T_WTCB));
                }
                (void)_kernel_relmem(&_kernel_systbl.free_sys, (T_MEM *)flg, sizeof(T_FLG));
            }
        }
        _kernel_relrun();
    }
    return ercd;
}
