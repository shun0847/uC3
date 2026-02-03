/**
 * @file    uC3exttsk_if.c
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
 * Enable Interrupt                             *
 ************************************************/
void _kernel_ext_tsk(void) {
    (void)_kernel_systrace_p0_entry((FP)ext_tsk, ((INT)((UINT)TFN_EXT_TSK << 8) + 0));
}
