/**
 * @file    uC3creisr.c
 * @brief   Micro C Cube Standard, KERNEL
 *          AArch64 dependent function
 * @date    2016.09.21
 * @author  Copyright (c) 2016, eForce Co., Ltd. All rights reserved.
 *
 ******************************************************************************
 * @par     History
 *          - rev 1.0 (2016.09.21) y-kim
 *            Created based on uC3creisr.c of ARMv7-A.
 ******************************************************************************
 */
#include <string.h>
#include "uC3sys.h"

static ER _kernel_creisr_1(ID isrid, T_CISR *pk_cisr);
static ER _kernel_creisr(ID isrid, T_CISR *pk_cisr);

/***************************************
    Create Interrupt Service Routine
 ***************************************/

static ER _kernel_creisr_1(ID isrid, T_CISR *pk_cisr)
{
    ER ercd = E_OK;
    SIZE maxhdr = _kernel_vinftbl_length() / sizeof(T_VINFTBL);

    if ((maxhdr == 0ULL) || ((SIZE)pk_cisr->intno >= maxhdr)) {
        ercd = E_PAR;
    } else {
        T_VINFTBL *vinftbl = &_kernel_vinftbl_begin()[pk_cisr->intno];
        if ((!vinftbl) ||
            ((vinftbl->intfunc != (FP)&_kernel_entisr) &&
            (vinftbl->intfunc != (FP)&int_abort))) {
            ercd = E_PAR;

        } else {
            if (isrid == 0) {
                if (_kernel_systbl.qisr.inf->limit == _kernel_systbl.qisr.inf->usedc) {
                    ercd = E_NOID;
                } else {
                    isrid = (ID)_kernel_systbl.qisr.inf->limit;
                    while ((isrid >= TMIN_OBJ) && (_kernel_systbl.qisr.isr[isrid] != 0)) {
                        isrid--;
                    }
                    ercd = isrid;
                }
            } else {
                if (_kernel_systbl.qisr.isr[isrid] != 0) {
                    ercd = E_OBJ;
                } else {
                    ercd = E_OK;
                }
            }

            if (ercd >= E_OK) {
                T_ISR *isr = (T_ISR *)_kernel_getmem(&_kernel_systbl.free_sys, sizeof(T_ISR));
                if (isr == 0) {
                    ercd = E_NOMEM;
                } else {
                    _kernel_systbl.qisr.inf->usedc++;
                    _kernel_systbl.qisr.isr[isrid] = isr;
                    isr->next = 0;
                    isr->prev = 0;
                    isr->isr = pk_cisr->isr;
                    isr->isratr = (UH)pk_cisr->isratr;
                    isr->intno= (UH)pk_cisr->intno;
                    isr->exinf = pk_cisr->exinf;
                    if (_kernel_systbl.cssb == 0) {
                        _kernel_lock();
                    }
                    if (vinftbl->intfunc == (FP)&_kernel_entisr) {
                        isr->prev = vinftbl->prev;
                        vinftbl->prev->next = isr;
                        vinftbl->prev = isr;
                    } else {
                        vinftbl->next = isr;
                        vinftbl->prev = isr;
                        vinftbl->intinfo = (pk_cisr->intno << 16) |
                                           ((pk_cisr->isratr & 0xFFU) << 8) |
                                           (pk_cisr->imask & 0xFFU);
                        vinftbl->intfunc = (FP)&_kernel_entisr;
                    }
                    if (_kernel_systbl.cssb == 0) {
                        _kernel_unlock();
                    }
                }
            }
        }
    }
    return ercd;
}

static ER _kernel_creisr(ID isrid, T_CISR *pk_cisr)
{
    ER ercd;
    T_SSB *cssb = _kernel_systbl.cssb;

    if (cssb == 0) {
        _kernel_systbl.stat.s.dlock = 1U;
        ercd = _kernel_creisr_1(isrid, pk_cisr);
        _kernel_relrun();
    } else if (cssb->stat.catr == TA_INI) {
        ercd = _kernel_creisr_1(isrid, pk_cisr);
    } else {
        ercd = E_CTX;
    }
    return ercd;
}

ER cre_isr(ID isrid, T_CISR *pk_cisr)
{
    ER ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if (( _kernel_systbl.qisr.inf == 0) ||
               (isrid <= 0) ||
               (isrid > (ID)_kernel_systbl.qisr.inf->limit)) {
        ercd = E_ID;
    } else {
        ercd = _kernel_creisr(isrid, pk_cisr);
    }
    return ercd;
}

ER_ID acre_isr(T_CISR *pk_cisr)
{
    ER_ID ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if (_kernel_systbl.qisr.inf == 0) {
        ercd = E_NOID;
    } else {
        ercd = (ER_ID)_kernel_creisr(0, pk_cisr);
    }
    return ercd;
}
