/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created
            2017.01.26: Fixed the IPA warnings.
 ***************************************************************************/

#include "uC3sys.h"


static ER _kernel_refmpf(T_MPF *mpf, T_RMPF *pk_rmpf);
static BOOL _kernel_refmpf_1(T_SSB *par, BOOL retcd);

/***********************************************
    Reference Fixed-Sized Memory Pool State
 ***********************************************/

static ER _kernel_refmpf(T_MPF *mpf, T_RMPF *pk_rmpf)
{
    T_TCB *tcb;
    T_WTCB *wtcb;
    PRI primax;

    if ((mpf->mpfatr & TA_TPRI) != 0U) {
        primax = (PRI)_kernel_systbl.qrdq.inf->limit;
        wtcb = mpf->que.mwait;
    } else {
        primax = 0;
        wtcb = &mpf->que.wait;
    }
    tcb = _kernel_gettcb(wtcb, primax);
    if (tcb != 0) {
        pk_rmpf->wtskid = (ID)tcb->tskid;
    } else {
        pk_rmpf->wtskid = TSK_NONE;
    }
    pk_rmpf->fblkcnt = (UINT)mpf->blkcnt;
    return E_OK;
}

static BOOL _kernel_refmpf_1(T_SSB *par, BOOL retcd)
{
    par->p1 = (VP_INT)_kernel_refmpf((T_MPF *)par->p1, (T_RMPF *)par->p2);
    return retcd;
}

ER ref_mpf(ID mpfid, T_RMPF *pk_rmpf)
{
    T_TCB *tcb;
    T_MPF *mpf;
    ER ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if (( _kernel_systbl.qmpf.inf == 0)||
               ((mpfid < TMIN_OBJ) || (mpfid > (ID)_kernel_systbl.qmpf.inf->limit))) {
        ercd = E_ID;

    } else if (_kernel_systbl.cssb == 0) {
        _kernel_systbl.stat.s.dlock = (UB)TRUE;
        mpf = _kernel_systbl.qmpf.mpf[mpfid];
        if (mpf == 0) {
            _kernel_relrun();
            ercd = E_NOEXS;
        } else {
            tcb = _kernel_systbl.ctcb;
            tcb->p1 = (VP_INT)mpf;
            tcb->p2 = (VP_INT)pk_rmpf;
            tcb->sysfunc = (FP)&_kernel_refmpf_1;
            _kernel_entsys(FALSE);
            ercd = (ER)tcb->p1;
        }
    } else {
        mpf = _kernel_systbl.qmpf.mpf[mpfid];
        if (mpf == 0) {
            ercd = E_NOEXS;
        } else {
            ercd = _kernel_refmpf(mpf, pk_rmpf);
        }
    }
    return ercd;
}
