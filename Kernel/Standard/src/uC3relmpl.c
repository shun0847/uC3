/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.14: Created.
            2010.12.21: Added the hook routine.
            2017.01.26: Fixed the IPA warnings.
            2017.07.19: Added the following checking code.
                         - If p_blk is NULL, an E_PAR error.
 ***************************************************************************/

#include "uC3sys.h"


static ER _kernel_relmpl(T_MPL *mpl, VP p_blk);
static BOOL _kernel_relmpl_1(T_SSB *par, BOOL retcd);

/*******************************************
    Release Variable-Sized Memory Block
 *******************************************/

static ER _kernel_relmpl(T_MPL *mpl, VP p_blk)
{
    void (*mplfunc)(ID);
    SIZE blksz;

    p_blk = (VP)((SIZE)p_blk - _kernel_ALIGN_SIZE);
    blksz = *(SIZE *)p_blk;
// Hook routine (rel_mpl)
    if (_kernel_systbl.ctxtrace4 != 0) {
        _kernel_blktrace(_kernel_systbl.ctxid_c, TFN_REL_MPL, (ID)mpl->mplid,
                         (VP)((SIZE)p_blk + _kernel_ALIGN_SIZE), *((SIZE *)p_blk+1));
    }
    (void)_kernel_relmem(&mpl->top, (T_MEM *)p_blk, blksz);
    mplfunc = (void (*)(ID))_kernel_systbl.mplfunc;
    mplfunc((ID)mpl->mplid);
    return E_OK;
}

static BOOL _kernel_relmpl_1(T_SSB *par, BOOL retcd)
{
    par->p1 = (VP_INT)_kernel_relmpl((T_MPL *)par->p1, (VP)par->p2);
    return retcd;
}

ER rel_mpl(ID mplid, VP p_blk)
{
    T_TCB *tcb;
    T_MPL *mpl;
    ER ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if (( _kernel_systbl.qmpl.inf == 0) ||
               ((mplid < TMIN_OBJ) || (mplid > (ID)_kernel_systbl.qmpl.inf->limit))) {
        ercd = E_ID;
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
            tcb->p2 = (VP_INT)p_blk;
            tcb->sysfunc = (FP)&_kernel_relmpl_1;
            _kernel_entsys(FALSE);
            ercd = (ER)tcb->p1;
        }
    } else {
        mpl = _kernel_systbl.qmpl.mpl[mplid];
        if (mpl == 0) {
            ercd = E_NOEXS;
        } else {
            ercd = _kernel_relmpl(mpl, p_blk);
        }
    }
    return ercd;
}
