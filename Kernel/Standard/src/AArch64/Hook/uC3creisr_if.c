/**
 * @file    uC3creisr_if.c
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
 * Create Interrupt Service Routine             *
 ************************************************/
ER _kernel_cre_isr(ID isrid, T_CISR *pk_cisr) {
    return _kernel_systrace_p2_entry((FP)cre_isr, ((INT)((UINT)TFN_CRE_ISR << 8) + 2), (UD)isrid, (UD)pk_cisr);
}

ER_ID _kernel_acre_isr(T_CISR *pk_cisr) {
    return _kernel_systrace_p1_entry((FP)acre_isr, ((INT)((UINT)TFN_ACRE_ISR << 8) + 1), (UD)pk_cisr);
}

