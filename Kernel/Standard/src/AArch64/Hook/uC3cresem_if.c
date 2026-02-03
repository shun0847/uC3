/**
 * @file    uC3cresem_if.c
 * @brief   Micro C Cube Standard, KERNEL
 *          API Hook routine header (ARMv8A AArch64)
 * @date    2021.01.25
 * @author  Copyright (c) 2018-2021, eForce Co., Ltd. All rights reserved.
 *
 ******************************************************************************
 * @par     History
 *          - rev 1.0 (2018.05.23) yokota
 *            Initial version.
 *          - rev 1.1 (2021.01.25) Imada
 *            Fixed C++test warnings.
 ******************************************************************************/
#include "uC3hook.h"

/************************************************
 * Create Semaphore                             *
 ************************************************/
ER _kernel_cre_sem(ID semid, T_CSEM *pk_csem) {
    return _kernel_systrace_p2_entry((FP)cre_sem, ((INT)((UINT)TFN_CRE_SEM << 8) + 2), (UD)semid, (UD)pk_csem);
}

ER_ID _kernel_acre_sem(T_CSEM *pk_csem) {
    return _kernel_systrace_p1_entry((FP)acre_sem, ((INT)((UINT)TFN_ACRE_SEM << 8) + 1), (UD)pk_csem);
}

