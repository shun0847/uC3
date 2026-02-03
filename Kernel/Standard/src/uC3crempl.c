/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2022, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created.
            2009.07.08: Suppressed warning of the RVDS compiler.
            2010.12.29: Added the hook routine.
            2017.03.02: Fixed the IPA warnings.
            2021.01.27: Fixed C++test warnings.
            2022.02.15: Added integer overflow check.
 ***************************************************************************/

#include "uC3sys.h"
#include "string.h"

static void _kernel_chkmpl(ID mplid);
static ER _kernel_crempl_1(ID mplid, T_CMPL *pk_cmpl);
static ER _kernel_crempl(ID mplid, T_CMPL *pk_cmpl);

/***************************************
    Create Variable-Sized Memory Pool
 ***************************************/

static void _kernel_chkmpl(ID mplid)
{
    T_TCB *tcb;
    T_MPL *mpl;
    T_WTCB *wtcb;
    PRI primax;
    SIZE *blk;
    SIZE blksz;
    FN fncode;
    VP *p_blk;

    mpl = _kernel_systbl.qmpl.mpl[mplid];
    if ((mpl->mplatr & TA_TPRI) != 0U) {
        primax = (PRI)_kernel_systbl.qrdq.inf->limit;
        wtcb = mpl->que.mwait;
    } else {
        primax = 0;
        wtcb = &mpl->que.wait;
    }
    for(tcb = _kernel_gettcb(wtcb, primax); tcb != 0; tcb = _kernel_gettcb(wtcb, primax)) {
        blksz = (SIZE)tcb->p2 + _kernel_ALIGN_SIZE;
        p_blk = (VP *)tcb->p3;
        blk = (SIZE *)_kernel_getmem(&mpl->top, blksz);
        if (blk != 0) {
            *blk = blksz;
            *(blk+1) = (SIZE)tcb->p2;
            *p_blk = (VP)((SIZE)blk + _kernel_ALIGN_SIZE);
            _kernel_deqtcb(&tcb->wtcb);
            if ((tcb->stat.msts & TTS_TMR) != 0U) {
                tcb->sprev->snext = (T_SSB *)tcb->snext;
                if (tcb->snext != 0) {
                    tcb->snext->sprev = (T_SSB *)tcb->sprev;
                }
                tcb->stat.msts &= (UB)~TTS_TMR;
            }
// Hook routine (get_mpl)
            if (_kernel_systbl.ctxtrace4 != 0) {
                fncode = ((TMO)tcb->p4 == TMO_FEVR)?(TFN_GET_MPL):(TFN_TGET_MPL);
                _kernel_blktrace((UW)tcb->tskid, fncode, (ID)mpl->mplid,
                                 (VP)((SIZE)blk + _kernel_ALIGN_SIZE), (SIZE)tcb->p2);
            }
            _kernel_enqrdq(tcb);
        } else {
            break;
        }
    }
}

static ER _kernel_crempl_1(ID mplid, T_CMPL *pk_cmpl)
{
    T_MPL *mpl;
    T_WTCB *que;
    T_MEM **mem_base = 0;
    SIZE allsz;
    ER ercd;

    if (mplid == 0) {
        if (_kernel_systbl.qmpl.inf->limit == _kernel_systbl.qmpl.inf->usedc) {
            ercd = E_NOID;
        } else {
            for(mplid = (ID)_kernel_systbl.qmpl.inf->limit; mplid >= TMIN_OBJ; mplid--) {
                if (_kernel_systbl.qmpl.mpl[mplid] == 0) {
                    break;
                }
            }
            ercd = mplid;
        }
    } else {
        if (_kernel_systbl.qmpl.mpl[mplid] != 0) {
            ercd = E_OBJ;
        } else {
            ercd = E_OK;
        }
    }

    /* check the integer overflow */
    if (ercd >= E_OK) {
        if (pk_cmpl->mplsz > (_kernel_SIZE_MAX - (_kernel_ALIGN_SIZE - 1U))) {
            ercd = E_PAR;
        }
    }

    if (ercd >= E_OK) {
        mpl = (T_MPL *)_kernel_getmem(&_kernel_systbl.free_sys, sizeof(T_MPL));
        if (mpl == 0) {
            ercd = E_NOMEM;
        } else {
            memset((void *)mpl, 0x00, sizeof(T_MPL));
            mpl->name = pk_cmpl->name;
            mpl->mplatr = (UH)(pk_cmpl->mplatr & TA_TPRI);
            mpl->mplid = (UH)mplid;
            mpl->mplsz = pk_cmpl->mplsz;
            mpl->mpl = pk_cmpl->mpl;
            if (pk_cmpl->mpl == 0) {
                allsz = (mpl->mplsz+(_kernel_ALIGN_SIZE-1UL)) & (UW)(0x00000000L-(W)_kernel_ALIGN_SIZE);
                mpl->allsz = allsz;
                if (_kernel_systbl.free_mpl != (T_MEM *)1) {
                    mem_base = &_kernel_systbl.free_mpl;
                } else if (_kernel_systbl.free_stk != (T_MEM *)1) {
                    mem_base = &_kernel_systbl.free_stk;
                } else {
                    mem_base = &_kernel_systbl.free_sys;
                }
                mpl->allad = (VP)_kernel_getmem(mem_base, mpl->allsz);
                if (mpl->allad == 0) {
                    (void)_kernel_relmem(&_kernel_systbl.free_sys, (T_MEM *)mpl, sizeof(T_MPL));
                    ercd = E_NOMEM;
                }
            } else {
                allsz = mpl->mplsz & (UW)(0x00000000L-(W)_kernel_ALIGN_SIZE);
                if (allsz == 0U) {
                    ercd = E_PAR;
                } else {
                    mpl->allad = pk_cmpl->mpl;
                    mpl->allsz = allsz;
                    mpl->mplatr |= TA_UBUF;
                }
            }

            if (ercd >= E_OK) {
                mpl->top = (T_MEM *)mpl->allad;
                mpl->top->next = 0;
                mpl->top->size = allsz;

                if ((mpl->mplatr & TA_TPRI) != 0U) {
                    que = (T_WTCB *)_kernel_getmem(&_kernel_systbl.free_sys,
                               _kernel_systbl.qrdq.inf->limit * sizeof(T_WTCB));
                    if (que == 0) {
                        if ((mpl->mplatr & TA_UBUF) == 0U) {
                            (void)_kernel_relmem(mem_base, (T_MEM *)mpl->allad, mpl->allsz);
                        }
                        (void)_kernel_relmem(&_kernel_systbl.free_sys, (T_MEM *)mpl, sizeof(T_MPL));
                        ercd = E_NOMEM;
                    } else {
                        _kernel_inique(que, (ID)_kernel_systbl.qrdq.inf->limit);
                        mpl->que.mwait = que-1;
                    }
                } else {
                    mpl->que.wait.next = &mpl->que.wait;
                    mpl->que.wait.prev = &mpl->que.wait;
                }
            }

            if (ercd >= E_OK) {
                _kernel_systbl.qmpl.inf->usedc++;
                _kernel_systbl.qmpl.mpl[mplid] = mpl;
                _kernel_systbl.mplfunc = (FP)&_kernel_chkmpl;
            }
        }
    }
    return ercd;
}

static ER _kernel_crempl(ID mplid, T_CMPL *pk_cmpl)
{
    T_SSB *cssb;
    ER ercd;

    if ((pk_cmpl->mplatr & ~(TA_TPRI|TA_TFIFO)) != 0U) {
        ercd = E_RSATR;
    } else if (pk_cmpl->mplsz == 0U) {
        ercd = E_PAR;

    } else {
        cssb = _kernel_systbl.cssb;
        if (cssb == 0) {
            _kernel_systbl.stat.s.dlock = (UB)TRUE;
            ercd = _kernel_crempl_1(mplid, pk_cmpl);
            _kernel_relrun();
        } else if (cssb->stat.catr == TA_INI) {
            ercd = _kernel_crempl_1(mplid, pk_cmpl);
        } else {
            ercd = E_CTX;
        }
    }
    return ercd;
}

ER cre_mpl(ID mplid, T_CMPL *pk_cmpl)
{
    ER ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if (( _kernel_systbl.qmpl.inf == 0) ||
               ((mplid < TMIN_OBJ) || (mplid > (ID)_kernel_systbl.qmpl.inf->limit))) {
        ercd = E_ID;
    } else {
        ercd = _kernel_crempl(mplid, pk_cmpl);
    }
    return ercd;
}

ER_ID acre_mpl(T_CMPL *pk_cmpl)
{
    ER ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if (_kernel_systbl.qmpl.inf == 0) {
        ercd = E_NOID;
    } else {
        ercd = _kernel_crempl(0, pk_cmpl);
    }
    return ercd;
}

