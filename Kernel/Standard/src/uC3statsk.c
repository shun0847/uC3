/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created.
            2010.06.22: Modified for the erro-code handler.
            2017.01.26: Fixed the IPA warnings.
 ***************************************************************************/

#include "uC3sys.h"


static ER _kernel_statsk(T_TCB *tcb, VP_INT stacd);
static BOOL _kernel_statsk_1(T_SSB *par, BOOL retcd);

/*******************************************
    Activate Task (with a Start Code)
 *******************************************/

static ER _kernel_statsk(T_TCB *tcb, VP_INT stacd)
{
    ER ercd;

    if (tcb->stat.msts != TTS_DMT) {
        ercd = E_OBJ;
    } else {
        _kernel_statsk_2(tcb, stacd);
        ercd = E_OK;
    }
    return ercd;
}

static BOOL _kernel_statsk_1(T_SSB *par, BOOL retcd)
{
    par->p1 = (VP_INT)_kernel_statsk((T_TCB *)par->p1, (VP_INT)par->p2);
    return retcd;
}

ER sta_tsk(ID tskid, VP_INT stacd)
{
    T_SSB *ssb;
    T_TCB *tcb;
    T_TCB *ttcb;
    ER ercd;

    if ((tskid > (ID)_kernel_systbl.qtcb.inf->limit) ||
        (tskid < TMIN_TSK)) {
        ercd = E_ID;

    } else if (_kernel_systbl.icnt != 0U) {
        ttcb = _kernel_systbl.qtcb.tcb[tskid];
        if (ttcb == 0) {
            ercd = E_NOEXS;
        } else {
            ssb = _kernel_getssb();
            if (ssb == 0) {
                ercd = E_NOMEM;
            } else {
                ssb->stat.catr = TA_SSB;
                ssb->p1 = (VP_INT)ttcb;
                ssb->p2 = stacd;
                ssb->p3 = (VP_INT)TFN_STA_TSK;
                ssb->p4 = (VP_INT)tskid;
                ssb->p5 = stacd;
                ssb->sysfunc = (FP)&_kernel_statsk_1;
                _kernel_enqssb(ssb);
                ercd = E_OK;
            }
        }
    } else if (_kernel_systbl.cssb == 0) {
        _kernel_systbl.stat.s.dlock = (UB)TRUE;
        ttcb = _kernel_systbl.qtcb.tcb[tskid];
        if (ttcb == 0) {
            _kernel_relrun();
            ercd = E_NOEXS;
        } else {
            tcb = _kernel_systbl.ctcb;
            tcb->p1 = (VP_INT)ttcb;
            tcb->p2 = stacd;
            tcb->sysfunc = (FP)&_kernel_statsk_1;
            _kernel_entsys(FALSE);
            ercd = (ER)tcb->p1;
        }
    } else {
        ttcb = _kernel_systbl.qtcb.tcb[tskid];
        if (ttcb == 0) {
            ercd = E_NOEXS;
        } else {
            ercd = _kernel_statsk(ttcb, stacd);
        }
    }
    return ercd;
}
