/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created
            2017.01.26: Fixed the IPA warnings.
 ***************************************************************************/

#include "uC3sys.h"


static ER _kernel_refpor(T_POR *por, T_RPOR *pk_rpor);
static BOOL _kernel_refpor_1(T_SSB *par, BOOL retcd);

/***************************************
    Reference Rendezvous Port State
 ***************************************/

static ER _kernel_refpor(T_POR *por, T_RPOR *pk_rpor)
{
    T_TCB *tcb;
    T_WTCB *wtcb;
    PRI primax;

    if ((por->poratr & TA_TPRI) != 0U) {
        primax = (PRI)_kernel_systbl.qrdq.inf->limit;
        wtcb = por->cque.mwait;
    } else {
        primax = 0;
        wtcb = &por->cque.wait;
    }
    tcb = _kernel_gettcb(wtcb, primax);
    if (tcb != 0) {
        pk_rpor->ctskid = (ID)tcb->tskid;
    } else {
        pk_rpor->ctskid = TSK_NONE;
    }
    tcb = _kernel_gettcb(&por->aque, 0);
    if (tcb != 0) {
        pk_rpor->atskid = (ID)tcb->tskid;
    } else {
        pk_rpor->atskid = TSK_NONE;
    }
    return E_OK;
}

static BOOL _kernel_refpor_1(T_SSB *par, BOOL retcd)
{
    par->p1 = (VP_INT)_kernel_refpor((T_POR *)par->p1, (T_RPOR *)par->p2);
    return retcd;
}

ER ref_por(ID porid, T_RPOR *pk_rpor)
{
    T_TCB *tcb;
    T_POR *por;
    ER ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if (( _kernel_systbl.qpor.inf == 0) ||
               ((porid < TMIN_OBJ) || (porid > (ID)_kernel_systbl.qpor.inf->limit))) {
        ercd = E_ID;

    } else if (_kernel_systbl.cssb == 0) {
        _kernel_systbl.stat.s.dlock = (UB)TRUE;
        por = _kernel_systbl.qpor.por[porid];
        if (por == 0) {
            _kernel_relrun();
            ercd = E_NOEXS;
        } else {
            tcb = _kernel_systbl.ctcb;
            tcb->p1 = (VP_INT)por;
            tcb->p2 = (VP_INT)pk_rpor;
            tcb->sysfunc = (FP)&_kernel_refpor_1;
            _kernel_entsys(FALSE);
            ercd = (ER)tcb->p1;
        }
    } else {
        por = _kernel_systbl.qpor.por[porid];
        if (por == 0) {
            ercd = E_NOEXS;
        } else {
            ercd = _kernel_refpor(por, pk_rpor);
        }
    }
    return ercd;
}
