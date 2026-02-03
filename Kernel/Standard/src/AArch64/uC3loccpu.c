/**
 * @file    uC3loccpu.c
 * @brief   Micro C Cube Standard, KERNEL
 *          AArch64 dependent function
 * @date    2016.09.21
 * @author  Copyright (c) 2016, eForce Co., Ltd. All rights reserved.
 *
 ******************************************************************************
 * @par     History
 *          - rev 1.0 (2016.09.21) y-kim
 *            Created based on uC3loccpu.c of ARMv7-A.
 ******************************************************************************
 */
#include "uC3sys.h"

/***********************************
    Lock CPU
 ***********************************/

ER loc_cpu(void)
{
    IMASK iims = _kernel_set_imask(1U);
    if (iims == 0U) {
        if (_kernel_systbl.icnt == 0U) {
            _kernel_systbl.iims = 0U;
        }
    }
    return E_OK;
}
