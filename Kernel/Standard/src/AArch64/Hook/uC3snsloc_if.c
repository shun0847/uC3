/**
 * @file    uC3snsloc_if.c
 * @brief   Micro C Cube Standard, KERNEL
 *          API Hook routine header (ARMv8A AArch64)
 * @date    2021.01.25
 * @author  Copyright (c) 2018-2021, eForce Co., Ltd. All rights reserved.
 *
 ******************************************************************************
 * @par     History
 *          - rev 1.0 (2018.05.25) yokota
 *            Initial version.
 *          - rev 1.1 (2021.01.25) Imada
 *            Fixed C++test warnings.
 ******************************************************************************/
#include "uC3hook.h"

/************************************************
 * Reference CPU State                          *
 ************************************************/
BOOL _kernel_sns_loc(void) {
    return _kernel_systrace_p0_entry((FP)sns_loc, ((INT)((UINT)TFN_SNS_LOC << 8) + 0));
}

