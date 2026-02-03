/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created
            2017.01.26: Fixed the IPA warnings.
            2017.07.19: Corrected the checking flgatr.
 ***************************************************************************/

#include "uC3sys.h"


static ER _kernel_refflg(T_FLG *flg, T_RFLG *pk_rflg);
static BOOL _kernel_refflg_1(T_SSB *par, BOOL retcd);

/***************************************
    Reference Eventflag State
 ***************************************/

static ER _kernel_refflg(T_FLG *flg, T_RFLG *pk_rflg)
{
    T_TCB *tcb;
    T_WTCB *wtcb;
    PRI primax;

    pk_rflg->flgptn = flg->flgptn;
    if ((flg->flgatr & (TA_TPRI|TA_WMUL)) == (TA_TPRI|TA_WMUL)) {
        primax = (PRI)_kernel_systbl.qrdq.inf->limit;
        wtcb = flg->que.mwait;
    } else {
        primax = 0;
        wtcb = &flg->que.wait;
    }
    tcb = _kernel_gettcb(wtcb, primax);
    if (tcb != 0) {
        pk_rflg->wtskid = (ID)tcb->tskid;
    } else {
        pk_rflg->wtskid = TSK_NONE;
    }
    return E_OK;
}

static BOOL _kernel_refflg_1(T_SSB *par, BOOL retcd)
{
    par->p1 = (VP_INT)_kernel_refflg((T_FLG *)par->p1, (T_RFLG *)par->p2);
    return retcd;
}

ER ref_flg(ID flgid, T_RFLG *pk_rflg)
{
    T_TCB *tcb;
    T_FLG *flg;
    ER ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if (( _kernel_systbl.qflg.inf == 0)||
               ((flgid < TMIN_OBJ) || (flgid > (ID)_kernel_systbl.qflg.inf->limit))) {
        ercd = E_ID;

    } else if (_kernel_systbl.cssb == 0) {
        _kernel_systbl.stat.s.dlock = (UB)TRUE;
        flg = _kernel_systbl.qflg.flg[flgid];
        if (flg == 0) {
            _kernel_relrun();
            ercd = E_NOEXS;
        } else {
            tcb = _kernel_systbl.ctcb;
            tcb->p1 = (VP_INT)flg;
            tcb->p2 = (VP_INT)pk_rflg;
            tcb->sysfunc = (FP)&_kernel_refflg_1;
            _kernel_entsys(FALSE);
            ercd = (ER)tcb->p1;
        }
    } else {
        flg = _kernel_systbl.qflg.flg[flgid];
        if (flg == 0) {
            ercd = E_NOEXS;
        } else {
            ercd = _kernel_refflg(flg, pk_rflg);
        }
    }
    return ercd;
}
