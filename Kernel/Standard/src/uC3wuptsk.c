/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created.
            2009.09.12: Modified for the erro-code handler.
            2017.01.26: Fixed the IPA warnings.
 ***************************************************************************/

#include "uC3sys.h"


static ER _kernel_wuptsk(T_TCB *tcb);
static BOOL _kernel_wuptsk_1(T_SSB *par, BOOL retcd);

/***********************************
    Wakeup Task
 ***********************************/

static ER _kernel_wuptsk(T_TCB *tcb)
{
    ER ercd;

    if (tcb->stat.msts == TTS_DMT) {
        ercd = E_OBJ;

    } else if (((tcb->stat.msts & TTS_WAI) != 0U) &&
               ( tcb->stat.wsts == TS_SLP)) {
        if ((tcb->stat.msts & TTS_TMR) != 0U) {
            tcb->sprev->snext = (T_SSB *)tcb->snext;
            if (tcb->snext != 0) {
                tcb->snext->sprev = (T_SSB *)tcb->sprev;
            }
        }
        _kernel_enqrdq(tcb);
        ercd = E_OK;
    } else {
        if (tcb->wup == TMAX_WUPCNT) {
            ercd = E_QOVR;
        } else {
            tcb->wup++;
            ercd = E_OK;
        }
    }
    return ercd;
}

static BOOL _kernel_wuptsk_1(T_SSB *par, BOOL retcd)
{
    par->p1 = (VP_INT)_kernel_wuptsk((T_TCB *)par->p1);
    return retcd;
}

ER wup_tsk(ID tskid)
{
    T_SSB *ssb;
    T_TCB *tcb;
    T_TCB *ttcb;
    ER ercd;

    if ((tskid < TSK_SELF) || (tskid > (ID)_kernel_systbl.qtcb.inf->limit)) {
        ercd = E_ID;

    } else if (_kernel_systbl.icnt != 0U) {
        if (tskid < TMIN_TSK) {
            ercd = E_ID;
        } else {
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
                    ssb->p3 = (VP_INT)TFN_WUP_TSK;
                    ssb->p4 = (VP_INT)tskid;
                    ssb->sysfunc = (FP)&_kernel_wuptsk_1;
                    _kernel_enqssb(ssb);
                    ercd = E_OK;
                }
            }
        }
    } else if (_kernel_systbl.cssb == 0) {
        tcb = _kernel_systbl.ctcb;
        if (tskid == TSK_SELF) {
            tskid = (ID)tcb->tskid;
        }
        _kernel_systbl.stat.s.dlock = (UB)TRUE;
        ttcb = _kernel_systbl.qtcb.tcb[tskid];
        if (ttcb == 0) {
            _kernel_relrun();
            ercd = E_NOEXS;
        } else {
            tcb->p1 = (VP_INT)ttcb;
            tcb->sysfunc = (FP)&_kernel_wuptsk_1;
            _kernel_entsys(FALSE);
            ercd = (ER)tcb->p1;
        }
    } else {
        if (tskid < TMIN_TSK) {
            ercd = E_ID;
        } else {
            ttcb = _kernel_systbl.qtcb.tcb[tskid];
            if (ttcb == 0) {
                ercd = E_NOEXS;
            } else {
                ercd = _kernel_wuptsk(ttcb);
            }
        }
    }
    return ercd;
}
