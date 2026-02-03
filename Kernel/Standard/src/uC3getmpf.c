/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created.
            2010.02.05: Corrected the checking of error code.
            2010.12.21: Added the hook routine.
            2017.01.26: Fixed the IPA warnings.
            2017.07.19: Added the following checking code.
                         - If the tmout is -2 or less, an E_PAR error.
                         - If p_blk is NULL, an E_PAR error.
 ***************************************************************************/

#include "uC3sys.h"


static ER _kernel_getmpf(T_MPF *mpf, VP *p_blk, TMO tmout, T_TCB *tcb);
static BOOL _kernel_getmpf_1(T_SSB *par, BOOL retcd);

/***************************************
    Acquire Fixed-Sized Memory Block
 ***************************************/

static ER _kernel_getmpf(T_MPF *mpf, VP *p_blk, TMO tmout, T_TCB *tcb)
{
    PRI tskpri;
    FN fncode;
    ER ercd;

    if (tmout == TMO_POL) {
        *p_blk = mpf->top;
        if (mpf->top == 0) {
            ercd = E_TMOUT;
        } else {
            mpf->top = mpf->top->next;
            mpf->blkcnt--;
// Hook routine (get_mpf)
            if (_kernel_systbl.ctxtrace4 != 0) {
                _kernel_blktrace(_kernel_systbl.ctxid_c, TFN_PGET_MPF, (ID)mpf->mpfid, (VP)*p_blk, (SIZE)mpf->blksz);
            }
            ercd = E_OK;
        }
    } else if (_kernel_systbl.stat.dspint == 0UL) {
        *p_blk = mpf->top;
        if (mpf->top == 0) {
            _kernel_deqtcb(&tcb->wtcb);
            tcb->stat.msts = TTS_WAI;
            tcb->stat.wsts = TS_MPF;
            tcb->wobjid = mpf->mpfid;
            if ((mpf->mpfatr & TA_TPRI) != 0U) {
                tskpri = (PRI)tcb->cpri;
                tcb->wque = mpf->que.mwait;
            } else {
                tskpri = 0;
                tcb->stat.msts |= TTS_FIFO;
                tcb->wque = &mpf->que.wait;
            }
            _kernel_enqtcb(&tcb->wque[tskpri], &tcb->wtcb);
            if (tmout != TMO_FEVR) {
                _kernel_enqtim(tcb, &_kernel_systbl.systim, (RELTIM)tmout);
                tcb->stat.msts |= TTS_TMR;
            }
            _kernel_systbl.ctcb = 0;
        } else {
            mpf->top = mpf->top->next;
            mpf->blkcnt--;
// Hook routine (get_mpf)
            if (_kernel_systbl.ctxtrace4 != 0) {
                fncode = ((tmout == TMO_FEVR)?(TFN_GET_MPF):(TFN_TGET_MPF));
                _kernel_blktrace(_kernel_systbl.ctxid_c, fncode, (ID)mpf->mpfid, (VP)*p_blk, (SIZE)mpf->blksz);
            }
        }
        ercd = E_OK;
    } else {
        ercd = E_CTX;
    }
    return ercd;
}

static BOOL _kernel_getmpf_1(T_SSB *par, BOOL retcd)
{
    par->p1 = (VP_INT)_kernel_getmpf((T_MPF *)par->p1, (VP *)par->p2, (TMO)par->p3, (T_TCB *)par->p4);
    return retcd;
}

ER tget_mpf(ID mpfid, VP *p_blk, TMO tmout)
{
    T_TCB *tcb;
    T_MPF *mpf;
    ER ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if (( _kernel_systbl.qmpf.inf == 0) ||
               ((mpfid < TMIN_OBJ) || (mpfid > (ID)_kernel_systbl.qmpf.inf->limit))) {
        ercd = E_ID;
    } else if ((tmout != TMO_FEVR) && (tmout < 0)) {
        ercd = E_PAR;
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
            tcb->p3 = (VP_INT)tmout;
            tcb->p4 = (VP_INT)tcb;
            tcb->sysfunc = (FP)&_kernel_getmpf_1;
            _kernel_entsys(FALSE);
            ercd = (ER)tcb->p1;
        }
    } else {
        if (tmout != TMO_POL) {
            ercd = E_CTX;
        } else {
            mpf = _kernel_systbl.qmpf.mpf[mpfid];
            if (mpf == 0) {
                ercd = E_NOEXS;
            } else {
                ercd = _kernel_getmpf(mpf, p_blk, tmout, 0);
            }
        }
    }
    return ercd;
}
