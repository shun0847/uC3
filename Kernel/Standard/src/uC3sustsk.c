/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created
            2017.01.26: Fixed the IPA warnings.
 ***************************************************************************/

#include "uC3sys.h"


static ER _kernel_sustsk(T_TCB *tcb);
static BOOL _kernel_sustsk_1(T_SSB *par, BOOL retcd);

/***********************************
    Suspend Task
 ***********************************/

static ER _kernel_sustsk(T_TCB *tcb)
{
    ER ercd;

    if (tcb->stat.msts == TTS_DMT) {
        ercd = E_OBJ;
    } else if ((_kernel_systbl.tid == tcb->tskid) &&
               (_kernel_systbl.stat.dspint != 0UL)) {
        ercd = E_CTX;
    } else {
        if (tcb->sus < TMAX_SUSCNT) {
            tcb->sus++;
            if ((tcb->stat.msts & TTS_SUS) == 0U) {
                if ((tcb->stat.msts & TTS_RDY) != 0U) {
                    _kernel_deqtcb(&tcb->wtcb);
                    tcb->stat.msts = TTS_SUS;
                    if (_kernel_systbl.ctcb == tcb) {
                        _kernel_systbl.ctcb = 0;
                    }
                } else {
                    tcb->stat.msts |= TTS_SUS;
                }
            }
            ercd = E_OK;
        } else {
            ercd = E_QOVR;
        }
    }
    return ercd;
}

static BOOL _kernel_sustsk_1(T_SSB *par, BOOL retcd)
{
    par->p1 = (VP_INT)_kernel_sustsk((T_TCB *)par->p1);
    return retcd;
}

ER sus_tsk(ID tskid)
{
    T_TCB *tcb;
    T_TCB *ttcb;
    ER ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if ((tskid < TSK_SELF) || (tskid > (ID)_kernel_systbl.qtcb.inf->limit)) {
        ercd = E_ID;

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
            tcb->sysfunc = (FP)&_kernel_sustsk_1;
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
                ercd = _kernel_sustsk(ttcb);
            }
        }
    }
    return ercd;
}
