/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created.
            2010.12.21: Added the hook routine.
            2017.01.26: Fixed the IPA warnings.
            2017.07.19: Added the following checking code.
                         - If p_blk is NULL, an E_PAR error.
 ***************************************************************************/

#include "uC3sys.h"


static ER _kernel_relmpf(T_MPF *mpf, VP p_blk);
static BOOL _kernel_relmpf_1(T_SSB *par, BOOL retcd);

/***************************************
    Release Fixed-Sized Memory Block
 ***************************************/

static ER _kernel_relmpf(T_MPF *mpf, VP p_blk)
{
    T_TCB *tcb;
    T_WTCB *wtcb;
    PRI primax;
    FN fncode;
    VP *pp_blk;

    if ((mpf->mpfatr & TA_TPRI) != 0U) {
        primax = (PRI)_kernel_systbl.qrdq.inf->limit;
        wtcb = mpf->que.mwait;
    } else {
        primax = 0;
        wtcb = &mpf->que.wait;
    }
// Hook routine (rel_mpf)
    if (_kernel_systbl.ctxtrace4 != 0) {
        _kernel_blktrace(_kernel_systbl.ctxid_c, TFN_REL_MPF, (ID)mpf->mpfid, (VP)p_blk, (SIZE)mpf->blksz);
    }
    tcb = _kernel_gettcb(wtcb, primax);
    if (tcb != 0) {
        pp_blk = (VP *)tcb->p2;
        *pp_blk = p_blk;
        _kernel_deqtcb(&tcb->wtcb);
        if ((tcb->stat.msts & TTS_TMR) != 0U) {
            tcb->sprev->snext = (T_SSB *)tcb->snext;
            if (tcb->snext != 0) {
                tcb->snext->sprev = (T_SSB *)tcb->sprev;
            }
            tcb->stat.msts &= (UB)~TTS_TMR;
        }
// Hook routine (get_mpf)
        if (_kernel_systbl.ctxtrace4 != 0) {
            fncode = ((TMO)tcb->p3 == TMO_FEVR)?(TFN_GET_MPF):(TFN_TGET_MPF);
            _kernel_blktrace((UW)tcb->tskid, fncode, (ID)mpf->mpfid, (VP)p_blk, (SIZE)mpf->blksz);
        }
        _kernel_enqrdq(tcb);
    } else {
        ((T_MEM *)p_blk)->next = mpf->top;
        mpf->top = (T_MEM *)p_blk;
        mpf->blkcnt++;
    }
    return E_OK;
}

static BOOL _kernel_relmpf_1(T_SSB *par, BOOL retcd)
{
    par->p1 = (VP_INT)_kernel_relmpf((T_MPF *)par->p1, (VP)par->p2);
    return retcd;
}

ER rel_mpf(ID mpfid, VP p_blk)
{
    T_TCB *tcb;
    T_MPF *mpf;
    ER ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if ((_kernel_systbl.qmpf.inf == 0) ||
               ((mpfid < TMIN_OBJ) || (mpfid > (ID)_kernel_systbl.qmpf.inf->limit))) {
        ercd = E_ID;
    } else if (p_blk == 0) {
        ercd = E_PAR;

    } else if (_kernel_systbl.cssb == 0) {
        _kernel_systbl.stat.s.dlock = (UB)TRUE;
        mpf = _kernel_systbl.qmpf.mpf[mpfid];
        if (mpf == 0) {
            _kernel_relrun();
            ercd = E_NOEXS;
        } else {
            tcb = _kernel_systbl.ctcb;
            tcb->p1 = (VP_INT)mpf;
            tcb->p2 = (VP_INT)p_blk;
            tcb->sysfunc = (FP)&_kernel_relmpf_1;
            _kernel_entsys(FALSE);
            ercd = (ER)tcb->p1;
        }
    } else {
        mpf = _kernel_systbl.qmpf.mpf[mpfid];
        if (mpf == 0) {
            ercd = E_NOEXS;
        } else {
            ercd = _kernel_relmpf(mpf, p_blk);
        }
    }
    return ercd;
}
