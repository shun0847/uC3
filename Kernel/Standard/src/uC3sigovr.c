/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created.
            2009.07.08: Suppressed warning of the RVDS compiler.
            2009.09.12: Modified for the erro-code handler.
            2017.01.26: Fixed the IPA warnings.
            2017.07.19: Corrected initial value of ercd.
 ***************************************************************************/

#include "uC3sys.h"


static BOOL _kernel_sigovr(T_SSB *par, BOOL retcd);

/******************************************
    Supply Time Tick for Overrun Handler
 ******************************************/

static BOOL _kernel_sigovr(T_SSB *par, BOOL retcd)
{
    T_OVR *ovr;
    T_TCB *tcb;
    ID tskid;

    tskid = (ID)par->p1;
    par->p1 = (VP_INT)E_OK;
    ovr = _kernel_systbl.ovr;
    tcb = _kernel_systbl.qtcb.tcb[tskid];
    if ((ovr != 0) && (tcb != 0)) {
        if (tcb->ovrsts == TOVR_STA) {
            tcb->ovrsts = TOVR_STP;
            _kernel_ovrfunc((ATR)ovr->ovratr, ovr->ovrhdr, tcb);
        }
    }
    return retcd;
}

ER ivsig_ovr(void)
{
    T_TCB *tcb;
    T_SSB *ssb;
    ER ercd = E_OK;

    if (_kernel_systbl.icnt == 0U) {
        ercd = E_CTX;
    } else if (_kernel_systbl.ovr == 0) {
        ercd = E_OBJ;

    } else {
        tcb = _kernel_systbl.ctcb;
        ssb = _kernel_systbl.cssb;
        if (ssb != 0) {
            if ((ssb->stat.catr & (TA_SSB|TA_CYC|TA_ALM)) == 0U) {
                tcb = (T_TCB *)ssb;
            }
        }
        if (tcb != 0) {
            if (tcb->ovrsts == TOVR_STA) {
                if (tcb->ovrtim == 0U) {
                    ssb = _kernel_getssb();
                    if (ssb == 0) {
                        ercd = E_NOMEM;
                    } else {
                        ssb->stat.catr = TA_SSB;
                        ssb->p1 = (VP_INT)(UW)tcb->tskid;
                        ssb->p3 = (VP_INT)TFN_IVSIG_OVR;
                        ssb->sysfunc = (FP)&_kernel_sigovr;
                        _kernel_enqssb(ssb);
                    }
                } else {
                    tcb->ovrtim--;
                }
            }
        }
    }
    return ercd;
}
