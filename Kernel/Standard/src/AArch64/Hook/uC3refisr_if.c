/**
 * @file    uC3refisr_if.c
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
 * Reference Interrupt Service Routine          *
 ************************************************/
ER _kernel_ref_isr(ID isrid, T_RISR *pk_risr) {
    return _kernel_systrace_p2_entry((FP)ref_isr, ((INT)((UINT)TFN_REF_ISR << 8) + 2), (UD)isrid, (UD)pk_risr);
}

