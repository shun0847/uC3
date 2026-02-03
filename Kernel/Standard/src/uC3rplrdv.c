/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2021, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created
            2017.03.02: Fixed the IPA warnings.
            2017.07.19: Added the following checking code.
                         - If msg is NULL, an E_PAR error.
            2018.02.20: Fix msgsz and msg checking code.
            2021.01.27: Fixed C++test warnings.
 ***************************************************************************/

#include "uC3sys.h"


static BOOL _kernel_rplrdv_1(T_SSB *par, BOOL retcd);

/***********************************
    Terminate Rendezvous
 ***********************************/

static BOOL _kernel_rplrdv_1(T_SSB *par, BOOL retcd)
{
    UB *from_msg;
    UB *to_msg;
    T_TCB *ctcb;
    UINT msgsz;

    ctcb = (T_TCB *)par->p1;
    if (((ctcb->stat.msts & TTS_WAI) == TTS_WAI) &&
        ( ctcb->stat.wsts == TS_RDV)) {
        msgsz = (UINT)par->p3;
        if ((UINT)ctcb->p4 < msgsz) {
            par->p1 = (VP_INT)E_PAR;
        } else {
            ctcb->p1 = (VP_INT)msgsz;
            to_msg = (UB *)ctcb->p6;
            from_msg = (UB *)par->p2;
            for(; msgsz != 0U; msgsz--) {
                *to_msg++ = *from_msg++;
            }
            if ((ctcb->stat.msts & TTS_TMR) != 0U) {
                ctcb->sprev->snext = (T_SSB *)ctcb->snext;
                if (ctcb->snext != 0) {
                    ctcb->snext->sprev = (T_SSB *)ctcb->sprev;
                }
                ctcb->stat.msts &= (UB)~TTS_TMR;
            }
            _kernel_enqrdq(ctcb);
            par->p1 = (VP_INT)E_OK;
        }
    } else {
        par->p1 = (VP_INT)E_OBJ;
    }
    return retcd;
}

ER rpl_rdv(RDVNO rdvno, VP msg, UINT msgsz)
{
    T_TCB *ttcb;
    T_TCB *tcb;
    ID tskid;
    ER ercd;

    tskid = (ID)rdvno >> 16;
    if ((_kernel_systbl.icnt != 0U) ||
        (_kernel_systbl.cssb != 0)) {
        ercd = E_CTX;
    } else if ((tskid > (ID)_kernel_systbl.qtcb.inf->limit) ||
               (tskid < TMIN_TSK)) {
        ercd = E_OBJ;
    } else if ((msgsz > 0U) && (msg == 0)) {
        ercd = E_PAR;
    } else {
        _kernel_systbl.stat.s.dlock = (UB)TRUE;
        ttcb = _kernel_systbl.qtcb.tcb[tskid];
        if (ttcb == 0) {
            _kernel_relrun();
            ercd = E_OBJ;
        } else {
            tcb = _kernel_systbl.ctcb;
            tcb->p1 = (VP_INT)ttcb;
            tcb->p2 = (VP_INT)msg;
            tcb->p3 = (VP_INT)msgsz;
            tcb->sysfunc = (FP)&_kernel_rplrdv_1;
            _kernel_entsys(FALSE);
            ercd = (ER)tcb->p1;
        }
    }
    return ercd;
}
