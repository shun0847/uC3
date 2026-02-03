/**
 * @file    uC3sigsem_if.c
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
 * Release Semaphore Resource                   *
 ************************************************/
ER _kernel_sig_sem(ID semid) {
    return _kernel_systrace_p1_entry((FP)sig_sem, ((INT)((UINT)TFN_SIG_SEM << 8) + 1), (UD)semid);
}
