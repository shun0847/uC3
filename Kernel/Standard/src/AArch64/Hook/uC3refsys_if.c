/**
 * @file    uC3refsys_if.c
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
 * Reference System State                       *
 ************************************************/
ER _kernel_ref_sys(T_RSYS *pk_rsys) {
    return _kernel_systrace_p1_entry((FP)ref_sys, ((INT)((UINT)TFN_REF_SYS << 8) + 1), (UD)pk_rsys);
}

