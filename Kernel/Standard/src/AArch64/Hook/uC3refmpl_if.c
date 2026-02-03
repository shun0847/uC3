/**
 * @file    uC3refmpl_if.c
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
 * Reference Variable-Sized Memory Pool State   *
 ************************************************/
ER _kernel_ref_mpl(ID mplid, T_RMPL *pk_rmpl) {
    return _kernel_systrace_p2_entry((FP)ref_mpl, ((INT)((UINT)TFN_REF_MPL << 8) + 2), (UD)mplid, (UD)pk_rmpl);
}

