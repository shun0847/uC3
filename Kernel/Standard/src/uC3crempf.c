/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2022, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created.
            2009.07.08: Suppressed warning of the RVDS compiler.
            2017.03.02: Fixed the IPA warnings.
            2021.01.27: Fixed C++test warnings.
            2022.02.18: Added integer overflow check.
****************************************************************************/

#include "uC3sys.h"
#include "string.h"


static ER _kernel_crempf_1(ID mpfid, T_CMPF *pk_cmpf);
static ER _kernel_crempf(ID mpfid, T_CMPF *pk_cmpf);

/***********************************
    Create Fixed-Sized Memory Pool
 ***********************************/

static ER _kernel_crempf_1(ID mpfid, T_CMPF *pk_cmpf)
{
    T_MPF *mpf;
    T_WTCB *que;
    T_MEM **mem_base = 0;
    SIZE *blk;
    ER ercd;
    SIZE blksz;
    UW blkcnt;

    if (mpfid == 0) {
        if (_kernel_systbl.qmpf.inf->limit == _kernel_systbl.qmpf.inf->usedc) {
            ercd = E_NOID;
        } else {
            for(mpfid = (ID)_kernel_systbl.qmpf.inf->limit; mpfid >= TMIN_OBJ; mpfid--) {
                if (_kernel_systbl.qmpf.mpf[mpfid] == 0) {
                    break;
                }
            }
            ercd = mpfid;
        }
    } else {
        if (_kernel_systbl.qmpf.mpf[mpfid] != 0) {
            ercd = E_OBJ;
        } else {
            ercd = E_OK;
        }
    }

    /* check the integer overflow */
    if (ercd >= E_OK) {
        if (pk_cmpf->blksz > (_kernel_SIZE_MAX - (_kernel_ALIGN_SIZE-1U))) {
            ercd = E_PAR;
        }
    }

    if (ercd >= E_OK) {
        mpf = (T_MPF *)_kernel_getmem(&_kernel_systbl.free_sys, sizeof(T_MPF));
        if (mpf == 0) {
            ercd = E_NOMEM;
        } else {
            memset((void *)mpf, 0x00, sizeof(T_MPF));
            mpf->name = pk_cmpf->name;
            mpf->mpfatr = (UH)(pk_cmpf->mpfatr & (TA_TFIFO|TA_TPRI));
            mpf->mpfid = (UH)mpfid;
            mpf->blkcnt = pk_cmpf->blkcnt;
            mpf->blksz = pk_cmpf->blksz;
            blksz = ((SIZE)mpf->blksz+(_kernel_ALIGN_SIZE-1U)) & (SIZE)(0x00000000L-(W)_kernel_ALIGN_SIZE);
            if (pk_cmpf->mpf == 0) {
                mpf->allsz = (SIZE)blksz * (SIZE)mpf->blkcnt;
                if (_kernel_systbl.free_mpl != (T_MEM *)1) {
                    mem_base = &_kernel_systbl.free_mpl;
                } else if (_kernel_systbl.free_stk != (T_MEM *)1) {
                    mem_base = &_kernel_systbl.free_stk;
                } else {
                    mem_base = &_kernel_systbl.free_sys;
                }
                mpf->allad = (VP)_kernel_getmem(mem_base, mpf->allsz);
                if (mpf->allad == 0) {
                    (void)_kernel_relmem(&_kernel_systbl.free_sys, (T_MEM *)mpf, sizeof(T_MPF));
                    ercd = E_NOMEM;
                }
            } else {
                mpf->mpfatr |= TA_UBUF;
                mpf->allad = pk_cmpf->mpf;
                mpf->allsz = 0U;
            }

            if (ercd >= E_OK) {
                mpf->top = (T_MEM *)mpf->allad;
                for(blkcnt=1UL, blk=(VP)mpf->allad; blkcnt<mpf->blkcnt; blkcnt++) {
                    *blk = (SIZE)((UB *)blk + blksz);
                    blk = (SIZE *)((UB *)blk + blksz);
                }
                *blk = 0U;

                if ((mpf->mpfatr & TA_TPRI) != 0U) {
                    que = (T_WTCB *)_kernel_getmem(&_kernel_systbl.free_sys,
                               _kernel_systbl.qrdq.inf->limit * sizeof(T_WTCB));
                    if (que == 0) {
                        if ((mpf->mpfatr & TA_UBUF) == 0U) {
                            (void)_kernel_relmem(mem_base, (T_MEM *)mpf->allad, mpf->allsz);
                        }
                        (void)_kernel_relmem(&_kernel_systbl.free_sys, (T_MEM *)mpf, sizeof(T_MPF));
                        ercd = E_NOMEM;
                    } else {
                        _kernel_inique(que, (ID)_kernel_systbl.qrdq.inf->limit);
                        mpf->que.mwait = que-1;
                    }
                } else {
                    mpf->que.wait.next = &mpf->que.wait;
                    mpf->que.wait.prev = &mpf->que.wait;
                }
            }

            if (ercd >= E_OK) {
                _kernel_systbl.qmpf.inf->usedc++;
                _kernel_systbl.qmpf.mpf[mpfid] = mpf;
            }
        }
    }
    return ercd;
}

static ER _kernel_crempf(ID mpfid, T_CMPF *pk_cmpf)
{
    T_SSB *cssb;
    ER ercd;

    if ((pk_cmpf->mpfatr & ~(TA_TPRI|TA_TFIFO)) != 0U) {
        ercd = E_RSATR;
    } else if ((pk_cmpf->blkcnt == 0U) || (pk_cmpf->blksz == 0U)) {
        ercd = E_PAR;

    } else {
        cssb = _kernel_systbl.cssb;
        if (cssb == 0) {
            _kernel_systbl.stat.s.dlock = (UB)TRUE;
            ercd = _kernel_crempf_1(mpfid, pk_cmpf);
            _kernel_relrun();
        } else if (cssb->stat.catr == TA_INI) {
            ercd = _kernel_crempf_1(mpfid, pk_cmpf);
        } else {
            ercd = E_CTX;
        }
    }
    return ercd;
}

ER cre_mpf(ID mpfid, T_CMPF *pk_cmpf)
{
    ER ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if (( _kernel_systbl.qmpf.inf == 0) ||
               ((mpfid < TMIN_OBJ) || (mpfid > (ID)_kernel_systbl.qmpf.inf->limit))) {
        ercd = E_ID;
    } else {
        ercd = _kernel_crempf(mpfid, pk_cmpf);
    }
    return ercd;
}

ER_ID acre_mpf(T_CMPF *pk_cmpf)
{
    ER_ID ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if (_kernel_systbl.qmpf.inf == 0) {
        ercd = E_NOID;
    } else {
        ercd = (ER_ID)_kernel_crempf(0, pk_cmpf);
    }
    return ercd;
}
