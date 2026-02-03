/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2022, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created.
            2010.02.05: Corrected the checking of error code.
            2010.12.21: Added the hook routine.
            2017.01.26: Fixed the IPA warnings.
            2017.07.19: Added the following checking code.
                         - If the tmout is -2 or less, an E_PAR error.
                         - If p_blk is NULL, an E_PAR error.
                        Corrected the tget_mpl post-processing, in case of E_TMOUT.
            2022.02.15: Added integer overflow check.
***************************************************************************/

#include "uC3sys.h"


static ER _kernel_getmpl_q(T_MPL *mpl, TMO tmout, T_TCB *tcb);
static ER _kernel_getmpl(T_MPL *mpl, UINT blksz, VP *p_blk, TMO tmout, T_TCB *tcb);
static BOOL _kernel_getmpl_1(T_SSB *par, BOOL retcd);

/*******************************************
    Acquire Variable-Sized Memory Block
 *******************************************/

static ER _kernel_getmpl_q(T_MPL *mpl, TMO tmout, T_TCB *tcb)
{
    PRI tskpri;
    ER ercd;

    if (tmout == TMO_POL) {
        ercd = E_TMOUT;
    } else {
        _kernel_deqtcb(&tcb->wtcb);
        tcb->stat.msts = TTS_WAI;
        tcb->stat.wsts = TS_MPL;
        tcb->wobjid = mpl->mplid;
        if ((mpl->mplatr & TA_TPRI) != 0U) {
            tskpri = (PRI)tcb->cpri;
            tcb->wque = mpl->que.mwait;
        } else {
            tskpri = 0;
            tcb->stat.msts |= TTS_FIFO;
            tcb->wque = &mpl->que.wait;
        }
        _kernel_enqtcb(&tcb->wque[tskpri], &tcb->wtcb);
        if (tmout != TMO_FEVR) {
            _kernel_enqtim(tcb, &_kernel_systbl.systim, (RELTIM)tmout);
            tcb->stat.msts |= TTS_TMR;
        }
        _kernel_systbl.ctcb = 0;
        ercd = E_OK;
    }
    return ercd;
}


static ER _kernel_getmpl(T_MPL *mpl, UINT blksz, VP *p_blk, TMO tmout, T_TCB *tcb)
{
    T_WTCB *wtcb;
    PRI primax;
    SIZE size;
    SIZE *blk;
    FN fncode;
    ER ercd;

    if ((tmout == TMO_POL) ||
        (_kernel_systbl.stat.dspint == 0UL)) {
        size = (SIZE)blksz + _kernel_ALIGN_SIZE;
        if ((SIZE)blksz > (_kernel_SIZE_MAX - _kernel_ALIGN_SIZE)) {
            /* check the integer overflow */
            ercd = E_PAR;
        } else if (size > mpl->allsz) {
            ercd = E_PAR;
        } else {
            if ((mpl->mplatr & TA_TPRI) != 0U) {
                primax = (PRI)_kernel_systbl.qrdq.inf->limit;
                wtcb = mpl->que.mwait;
            } else {
                primax = 0;
                wtcb = &mpl->que.wait;
            }
            if (_kernel_gettcb(wtcb, primax) != 0) {
                ercd = _kernel_getmpl_q(mpl, tmout, tcb);
            } else {
                blk = (SIZE *)_kernel_getmem(&mpl->top, size);
                if (blk != 0) {
                    *blk = size;
                    *(blk+1) = blksz;
                    *p_blk = (VP)((SIZE)blk + _kernel_ALIGN_SIZE);
// Hook routine (get_mpl)
                    if (_kernel_systbl.ctxtrace4 != 0) {
                        fncode = (tmout == TMO_POL)?(TFN_PGET_MPL):((tmout == TMO_FEVR)?(TFN_GET_MPL):(TFN_TGET_MPL));
                        _kernel_blktrace(_kernel_systbl.ctxid_c, fncode, (ID)mpl->mplid, (VP)((SIZE)blk + _kernel_ALIGN_SIZE), (SIZE)blksz);
                    }
                    ercd = E_OK;
                } else {
                    ercd = _kernel_getmpl_q(mpl, tmout, tcb);
                }
            }
        }
    } else {
        ercd = E_CTX;
    }
    return ercd;
}

static BOOL _kernel_getmpl_1(T_SSB *par, BOOL retcd)
{
    par->p1 = (VP_INT)_kernel_getmpl((T_MPL *)par->p1, (UINT)par->p2, (VP *)par->p3, (TMO)par->p4, (T_TCB *)par->p5);
    return retcd;
}

ER tget_mpl(ID mplid, UINT blksz, VP *p_blk, TMO tmout)
{
    void (*mplfunc)(ID);
    T_TCB *tcb;
    T_MPL *mpl;
    ER ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if (( _kernel_systbl.qmpl.inf == 0) ||
               ((mplid < TMIN_OBJ) || (mplid > (ID)_kernel_systbl.qmpl.inf->limit))) {
        ercd = E_ID;
    } else if (blksz == 0U) {
        ercd = E_PAR;
    } else if ((tmout != TMO_FEVR) && (tmout < 0)) {
        ercd = E_PAR;
    } else if (p_blk == 0) {
        ercd = E_PAR;

    } else if (_kernel_systbl.cssb == 0) {
        _kernel_systbl.stat.s.dlock = (UB)TRUE;
        mpl = _kernel_systbl.qmpl.mpl[mplid];
        if (mpl == 0) {
            _kernel_relrun();
            ercd = E_NOEXS;
        } else {
            tcb = _kernel_systbl.ctcb;
            tcb->p1 = (VP_INT)mpl;
            tcb->p2 = (VP_INT)blksz;
            tcb->p3 = (VP_INT)p_blk;
            tcb->p4 = (VP_INT)tmout;
            tcb->p5 = (VP_INT)tcb;
            tcb->sysfunc = (FP)&_kernel_getmpl_1;
            _kernel_entsys(FALSE);
            ercd = (ER)tcb->p1;
            if ((tmout != TMO_POL) && (ercd == E_TMOUT)) {
                mplfunc = (void (*)(ID))_kernel_systbl.mplfunc;
                if (mplfunc != 0) {
                    mplfunc(mplid);
                }
            }
        }
    } else {
        if (tmout != TMO_POL) {
            ercd = E_CTX;
        } else {
            mpl = _kernel_systbl.qmpl.mpl[mplid];
            if (mpl == 0) {
                ercd = E_NOEXS;
            } else {
                ercd = _kernel_getmpl(mpl, blksz, p_blk, tmout, 0);
            }
        }
    }
    return ercd;
}
