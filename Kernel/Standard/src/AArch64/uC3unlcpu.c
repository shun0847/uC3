/**
 * @file    uC3unlcpu.c
 * @brief   Micro C Cube Standard, KERNEL
 *          AArch64 dependent function
 * @date    2016.09.21
 * @author  Copyright (c) 2016, eForce Co., Ltd. All rights reserved.
 *
 ******************************************************************************
 * @par     History
 *          - rev 1.0 (2016.09.21) y-kim
 *            Created based on uC3unlcpu.c of ARMv7-A.
 ******************************************************************************
 */
#include "uC3sys.h"

/***********************************
    Unlock CPU
 ***********************************/

ER unl_cpu(void)
{
    IMASK iims = _kernel_set_imask(0U);
    if (iims == 1U) {
        if ((_kernel_systbl.icnt == 0U) &&
            (_kernel_systbl.cssb == 0)) {
            _kernel_relrun();
        }
    }
    return E_OK;
}
