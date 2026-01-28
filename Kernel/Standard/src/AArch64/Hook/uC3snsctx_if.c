/**
 * @file    uC3snsctx_if.c
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
 * Reference Contexts                           *
 ************************************************/
BOOL _kernel_sns_ctx(void) {
    return _kernel_systrace_p0_entry((FP)sns_ctx, ((INT)((UINT)TFN_SNS_CTX << 8) + 0));
}

