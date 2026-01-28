/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created
            2017.01.26: Fixed the IPA warnings.
 ***************************************************************************/

#include "uC3sys.h"


static ER _kernel_refmtx(T_MTX *mtx, T_RMTX *pk_rmtx);
static BOOL _kernel_refmtx_1(T_SSB *par, BOOL retcd);

/***********************************
    Reference Mutex State
 ***********************************/

static ER _kernel_refmtx(T_MTX *mtx, T_RMTX *pk_rmtx)
{
    T_TCB *tcb;
    T_WTCB *wtcb;
    PRI primax;

    if ((mtx->mtxatr & (TA_TPRI|TA_INHERIT|TA_CEILING)) != 0U) {
        primax = (PRI)_kernel_systbl.qrdq.inf->limit;
        wtcb = mtx->que.mwait;
    } else {
        primax = 0;
        wtcb = &mtx->que.wait;
    }
    tcb = _kernel_gettcb(wtcb, primax);
    if (tcb != 0) {
        pk_rmtx->wtskid = (ID)tcb->tskid;
    } else {
        pk_rmtx->wtskid = TSK_NONE;
    }
    pk_rmtx->htskid = (ID)mtx->tskid;

    return E_OK;
}

static BOOL _kernel_refmtx_1(T_SSB *par, BOOL retcd)
{
    par->p1 = (VP_INT)_kernel_refmtx((T_MTX *)par->p1, (T_RMTX *)par->p2);
    return retcd;
}

ER ref_mtx(ID mtxid, T_RMTX *pk_rmtx)
{
    T_TCB *tcb;
    T_MTX *mtx;
    ER ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if (( _kernel_systbl.qmtx.inf == 0) ||
               ((mtxid < TMIN_OBJ) || (mtxid > (ID)_kernel_systbl.qmtx.inf->limit))) {
        ercd = E_ID;

    } else if (_kernel_systbl.cssb == 0) {
        _kernel_systbl.stat.s.dlock = (UB)TRUE;
        mtx = _kernel_systbl.qmtx.mtx[mtxid];
        if (mtx == 0) {
            _kernel_relrun();
            ercd = E_NOEXS;
        } else {
            tcb = _kernel_systbl.ctcb;
            tcb->p1 = (VP_INT)mtx;
            tcb->p2 = (VP_INT)pk_rmtx;
            tcb->sysfunc = (FP)&_kernel_refmtx_1;
            _kernel_entsys(FALSE);
            ercd = (ER)tcb->p1;
        }
    } else {
        mtx = _kernel_systbl.qmtx.mtx[mtxid];
        if (mtx == 0) {
            ercd = E_NOEXS;
        } else {
            ercd = _kernel_refmtx(mtx, pk_rmtx);
        }
    }
    return ercd;
}
