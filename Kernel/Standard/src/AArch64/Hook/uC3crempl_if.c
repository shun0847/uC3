/**
 * @file    uC3crempl_if.c
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
 * Create Variable-Sized Memory Pool            *
 ************************************************/
ER _kernel_cre_mpl(ID mplid, T_CMPL *pk_cmpl) {
    return _kernel_systrace_p2_entry((FP)cre_mpl, ((INT)((UINT)TFN_CRE_MPL << 8) + 2), (UD)mplid, (UD)pk_cmpl);
}

ER_ID _kernel_acre_mpl(T_CMPL *pk_cmpl) {
    return _kernel_systrace_p1_entry((FP)acre_mpl, ((INT)((UINT)TFN_ACRE_MPL << 8) + 1), (UD)pk_cmpl);
}
