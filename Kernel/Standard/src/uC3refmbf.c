/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created
            2017.01.26: Fixed the IPA warnings.
 ***************************************************************************/

#include "uC3sys.h"


static ER _kernel_refmbf(T_MBF *mbf, T_RMBF *pk_rmbf);
static BOOL _kernel_refmbf_1(T_SSB *par, BOOL retcd);

/***************************************
    Reference Message Buffer State
 ***************************************/

static ER _kernel_refmbf(T_MBF *mbf, T_RMBF *pk_rmbf)
{
    T_TCB *tcb;
    T_WTCB *wtcb;
    PRI primax;

    pk_rmbf->fmbfsz = (UINT)mbf->frsz;
    pk_rmbf->smsgcnt = (UINT)mbf->cnt;
    if ((mbf->mbfatr & TA_TPRI) != 0U) {
        primax = (PRI)_kernel_systbl.qrdq.inf->limit;
        wtcb = mbf->sque.mwait;
    } else {
        primax = 0;
        wtcb = &mbf->sque.wait;
    }
    tcb = _kernel_gettcb(wtcb, primax);
    if (tcb != 0) {
        pk_rmbf->stskid = (ID)tcb->tskid;
    } else {
        pk_rmbf->stskid = TSK_NONE;
    }
    tcb = _kernel_gettcb(&mbf->wque, 0);
    if (tcb != 0) {
        pk_rmbf->rtskid = (ID)tcb->tskid;
    } else {
        pk_rmbf->rtskid = TSK_NONE;
    }
    return E_OK;
}

static BOOL _kernel_refmbf_1(T_SSB *par, BOOL retcd)
{
    par->p1 = (VP_INT)_kernel_refmbf((T_MBF *)par->p1, (T_RMBF *)par->p2);
    return retcd;
}

ER ref_mbf(ID mbfid, T_RMBF *pk_rmbf)
{
    T_TCB *tcb;
    T_MBF *mbf;
    ER ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if (( _kernel_systbl.qmbf.inf == 0) ||
               ((mbfid < TMIN_OBJ) || (mbfid > (ID)_kernel_systbl.qmbf.inf->limit))) {
        ercd = E_ID;

    } else if (_kernel_systbl.cssb == 0) {
        _kernel_systbl.stat.s.dlock = (UB)TRUE;
        mbf = _kernel_systbl.qmbf.mbf[mbfid];
        if (mbf == 0) {
            _kernel_relrun();
            ercd = E_NOEXS;
        } else {
            tcb = _kernel_systbl.ctcb;
            tcb->p1 = (VP_INT)mbf;
            tcb->p2 = (VP_INT)pk_rmbf;
            tcb->sysfunc = (FP)&_kernel_refmbf_1;
            _kernel_entsys(FALSE);
            ercd = (ER)tcb->p1;
        }
    } else {
        mbf = _kernel_systbl.qmbf.mbf[mbfid];
        if (mbf == 0) {
            ercd = E_NOEXS;
        } else {
            ercd = _kernel_refmbf(mbf, pk_rmbf);
        }
    }
    return ercd;
}
