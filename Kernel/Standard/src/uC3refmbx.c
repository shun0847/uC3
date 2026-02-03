/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created
            2017.01.26: Fixed the IPA warnings.
 ***************************************************************************/

#include "uC3sys.h"


static ER _kernel_refmbx(T_MBX *mbx, T_RMBX *pk_rmbx);
static BOOL _kernel_refmbx_1(T_SSB *par, BOOL retcd);

/***********************************
    Reference Mailbox State
 ***********************************/

static ER _kernel_refmbx(T_MBX *mbx, T_RMBX *pk_rmbx)
{
    T_TCB *tcb;
    T_MHD *mhd;
    T_WTCB *wtcb;
    PRI primax;

    pk_rmbx->pk_msg = NULL;
    if ((mbx->mbxatr & TA_TPRI) != 0U) {
        primax = (PRI)_kernel_systbl.qrdq.inf->limit;
        wtcb = mbx->que.mwait;
    } else {
        primax = 0;
        wtcb = &mbx->que.wait;
    }
    tcb = _kernel_gettcb(wtcb, primax);
    if (tcb != 0) {
        pk_rmbx->wtskid = (ID)tcb->tskid;
    } else {
        pk_rmbx->wtskid = TSK_NONE;
    }

    if ((mbx->mbxatr & TA_MPRI) != 0U) {
        mhd = mbx->mprihd.mult.mque+1;
        for(primax = 0; primax < mbx->mprihd.mult.maxmpri; primax++) {
            if (mhd->top != 0) {
                pk_rmbx->pk_msg = mhd->top;
                break;
            }
            mhd++;
        }
    } else {
        pk_rmbx->pk_msg = mbx->mprihd.sngl.top;
    }
    return E_OK;
}

static BOOL _kernel_refmbx_1(T_SSB *par, BOOL retcd)
{
    par->p1 = (VP_INT)_kernel_refmbx((T_MBX *)par->p1, (T_RMBX *)par->p2);
    return retcd;
}

ER ref_mbx(ID mbxid, T_RMBX *pk_rmbx)
{
    T_TCB *tcb;
    T_MBX *mbx;
    ER ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if (( _kernel_systbl.qmbx.inf == 0) ||
               ((mbxid < TMIN_OBJ) || (mbxid > (ID)_kernel_systbl.qmbx.inf->limit))) {
        ercd = E_ID;

    } else if (_kernel_systbl.cssb == 0) {
        _kernel_systbl.stat.s.dlock = (UB)TRUE;
        mbx = _kernel_systbl.qmbx.mbx[mbxid];
        if (mbx == 0) {
            _kernel_relrun();
            ercd = E_NOEXS;
        } else {
            tcb = _kernel_systbl.ctcb;
            tcb->p1 = (VP_INT)mbx;
            tcb->p2 = (VP_INT)pk_rmbx;
            tcb->sysfunc = (FP)&_kernel_refmbx_1;
            _kernel_entsys(FALSE);
            ercd = (ER)tcb->p1;
        }
    } else {
        mbx = _kernel_systbl.qmbx.mbx[mbxid];
        if (mbx == 0) {
            ercd = E_NOEXS;
        } else {
            ercd = _kernel_refmbx(mbx, pk_rmbx);
        }
    }
    return ercd;
}
