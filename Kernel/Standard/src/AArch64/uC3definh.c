/**
 * @file    uC3definh.c
 * @brief   Micro C Cube Standard, KERNEL
 *          AArch64 dependent function
 * @date    2017.04.25
 * @author  Copyright (c) 2016-2017, eForce Co., Ltd. All rights reserved.
 *
 ******************************************************************************
 * @par     History
 *          - rev 1.0 (2016.09.21) y-kim
 *            Created based on uC3definh.c of ARMv7-A.
 *          - rev 1.1 (2017.04.25) yokota
 *            Fixed the IPA warnings.
 ******************************************************************************
 */
#include "uC3sys.h"

/***********************************
    Define Interrupt Handler
 ***********************************/

ER def_inh(INHNO inhno, T_DINH *pk_dinh)
{
    ER ercd = E_OK;
    SIZE maxhdr = _kernel_vinftbl_length() / sizeof(T_VINFTBL);
    if ((maxhdr == 0ULL) || ((SIZE)inhno >= maxhdr)) {
            ercd = E_PAR;
    } else {
        T_VINFTBL * vinftbl = &_kernel_vinftbl_begin()[inhno];
        IMASK imask = (IMASK)_kernel_set_imask(1U);
        if ((!vinftbl) ||
        	(vinftbl->intfunc == (FP)&_kernel_entisr)) {
            ercd = E_PAR;
        } else if (pk_dinh != 0) {
            vinftbl->intinfo = (inhno << 16) |
                               ((pk_dinh->inhatr & 0xFFU) << 8) |
                               (pk_dinh->imask & 0xFFU);
            vinftbl->intfunc = pk_dinh->inthdr;
        } else {
            vinftbl->next = 0;
            vinftbl->prev = 0;
            vinftbl->intinfo = 0U;
            vinftbl->intfunc = &int_abort;
        }
        if (imask == 0U) {
            (void)_kernel_set_imask(imask);
        }
    }
    return ercd;
}
