/**
 * @file    uC3refmpf_if.c
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
 * Reference Fixed-Sized Memory Pool State      *
 ************************************************/
ER _kernel_ref_mpf(ID mpfid, T_RMPF *pk_rmpf) {
    return _kernel_systrace_p2_entry((FP)ref_mpf, ((INT)((UINT)TFN_REF_MPF << 8) + 2), (UD)mpfid, (UD)pk_rmpf);
}

