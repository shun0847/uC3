/**
 * @file    uC3chgims.c
 * @brief   Micro C Cube Standard, KERNEL
 *          AArch64 dependent function
 * @date    2025.12.01
 * @author  Copyright (c) 2016-2025, eForce Co., Ltd. All rights reserved.
 *
 ******************************************************************************
 * @par     History
 *          - rev 1.0 (2016.09.21) y-kim
 *            Created based on uC3chgims.c of ARMv7-A.
 *          - rev 1.1 (2017.04.25) yokota
 *            Fixed the IPA warnings.
 *          - rev 1.2 (2025.12.01)
 *            Clear the interrupt mask status if iims is equal to 0x0U.
 ******************************************************************************
 */
#include "uC3sys.h"

/***********************************
    Change Interrupt Mask Level
 ***********************************/

ER chg_ims(IMASK imask)
{
    ER ercd;

    if ((imask != 0U) && (imask != 1U)) {
        ercd = E_PAR;
    } else {
        IMASK iims = _kernel_set_imask(imask);
        if ((iims == 0U) && (imask == 1U)) {
            _kernel_systbl.iims = 0U;
        } else if (iims == 1U) {
            _kernel_systbl.stat.s.ims = 0;
            _kernel_relrun();
        } else {
            /* Do Nothing */
        }
        ercd = E_OK;
    }
    return ercd;
}
