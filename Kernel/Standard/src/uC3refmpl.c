/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created
            2017.01.26: Fixed the IPA warnings.
 ***************************************************************************/

#include "uC3sys.h"


static ER _kernel_refmpl(T_MPL *mpl, T_RMPL *pk_rmpl);
static BOOL _kernel_refmpl_1(T_SSB *par, BOOL retcd);

/***********************************************
    Reference Variable-Sized Memory Pool State
 ***********************************************/

static ER _kernel_refmpl(T_MPL *mpl, T_RMPL *pk_rmpl)
{
    T_TCB *tcb;
    T_WTCB *wtcb;
    PRI primax;
    SIZE size;
    SIZE maxsz;

    if ((mpl->mplatr & TA_TPRI) != 0U) {
        primax = (PRI)_kernel_systbl.qrdq.inf->limit;
        wtcb = mpl->que.mwait;
    } else {
        primax = 0;
        wtcb = &mpl->que.wait;
    }
    tcb = _kernel_gettcb(wtcb, primax);
    if (tcb != 0) {
        pk_rmpl->wtskid = (ID)tcb->tskid;
    } else {
        pk_rmpl->wtskid = TSK_NONE;
    }
    _kernel_getsize(&mpl->top, &size, &maxsz);
    pk_rmpl->fmplsz = size;
    if (maxsz >= _kernel_ALIGN_SIZE) {
        maxsz -= _kernel_ALIGN_SIZE;
    }
  #if (_kernel_INT_SIZE != _kernel_SIZE_SIZE)
    if (maxsz > (UINT)(-1)) {
        pk_rmpl->fblksz = (UINT)(-1);
    } else {
        pk_rmpl->fblksz = (UINT)maxsz;
    }
  #else
    pk_rmpl->fblksz = (UINT)maxsz;
  #endif
    return E_OK;
}

static BOOL _kernel_refmpl_1(T_SSB *par, BOOL retcd)
{
    par->p1 = (VP_INT)_kernel_refmpl((T_MPL *)par->p1, (T_RMPL *)par->p2);
    return retcd;
}

ER ref_mpl(ID mplid, T_RMPL *pk_rmpl)
{
    T_TCB *tcb;
    T_MPL *mpl;
    ER ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if (( _kernel_systbl.qmpl.inf == 0)||
               ((mplid < TMIN_OBJ) || (mplid > (ID)_kernel_systbl.qmpl.inf->limit))) {
        ercd = E_ID;

    } else if (_kernel_systbl.cssb == 0) {
        _kernel_systbl.stat.s.dlock = (UB)TRUE;
        mpl = _kernel_systbl.qmpl.mpl[mplid];
        if (mpl == 0) {
            _kernel_relrun();
            ercd = E_NOEXS;
        } else {
            tcb = _kernel_systbl.ctcb;
            tcb->p1 = (VP_INT)mpl;
            tcb->p2 = (VP_INT)pk_rmpl;
            tcb->sysfunc = (FP)&_kernel_refmpl_1;
            _kernel_entsys(FALSE);
            ercd = (ER)tcb->p1;
        }
    } else {
        mpl = _kernel_systbl.qmpl.mpl[mplid];
        if (mpl == 0) {
            ercd = E_NOEXS;
        } else {
            ercd = _kernel_refmpl(mpl, pk_rmpl);
        }
    }
    return ercd;
}
