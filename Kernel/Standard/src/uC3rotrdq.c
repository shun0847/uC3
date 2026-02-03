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


static ER _kernel_rotrdq(PRI tskpri);
static BOOL _kernel_rotrdq_1(T_SSB *par, BOOL retcd);

/***********************************
    Rotate Task Precedence
 ***********************************/

static ER _kernel_rotrdq(PRI tskpri)
{
    T_TCB *tcb;

    tcb = (T_TCB *)_kernel_systbl.qrdq.rdq[tskpri].next;
    if (tcb != (T_TCB *)&_kernel_systbl.qrdq.rdq[tskpri].next) {
        _kernel_deqtcb(&tcb->wtcb);
        _kernel_enqtcb(&_kernel_systbl.qrdq.rdq[tskpri], &tcb->wtcb);
        if ((PRI)_kernel_systbl.pri >= tskpri) {
            if (_kernel_systbl.stat.dspint == 0UL) {
                _kernel_systbl.ctcb = 0;
            } else {
                _kernel_systbl.rdsp = 1U;
            }
        }
    }
    return E_OK;
}

static BOOL _kernel_rotrdq_1(T_SSB *par, BOOL retcd)
{
    par->p1 = (VP_INT)_kernel_rotrdq((PRI)par->p1);
    return retcd;
}

ER rot_rdq(PRI tskpri)
{
    T_SSB *ssb;
    T_TCB *tcb;
    ER ercd;

    if ((tskpri < TPRI_SELF) || (tskpri > (PRI)_kernel_systbl.qrdq.inf->limit)) {
        ercd = E_PAR;

    } else if (_kernel_systbl.icnt != 0U) {
        if (tskpri < TMIN_TPRI) {
            ercd = E_PAR;
        } else {
            ssb = _kernel_getssb();
            if (ssb == 0) {
                ercd = E_NOMEM;
            } else {
                ssb->stat.catr = TA_SSB;
                ssb->p1 = (VP_INT)tskpri;
                ssb->p3 = (VP_INT)TFN_ROT_RDQ;
                ssb->p4 = (VP_INT)tskpri;
                ssb->sysfunc = (FP)&_kernel_rotrdq_1;
                _kernel_enqssb(ssb);
                ercd = E_OK;
            }
        }
    } else if (_kernel_systbl.cssb == 0) {
        tcb = _kernel_systbl.ctcb;
        if (tskpri == TPRI_SELF) {
            tskpri = (PRI)tcb->bpri;
        }
        tcb->p1 = (VP_INT)tskpri;
        tcb->sysfunc = (FP)&_kernel_rotrdq_1;
        _kernel_entsys(FALSE);
        ercd = (ER)tcb->p1;
    } else {
        if (tskpri < TMIN_TPRI) {
            ercd = E_PAR;
        } else {
            ercd = _kernel_rotrdq(tskpri);
        }
    }
    return ercd;
}
