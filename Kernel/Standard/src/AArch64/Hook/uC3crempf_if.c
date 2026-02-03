/**
 * @file    uC3crembf_if.c
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
 * Create Fixed-Sized Memory Pool               *
 ************************************************/
ER _kernel_cre_mpf(ID mpfid, T_CMBF *pk_cmpf) {
    return _kernel_systrace_p2_entry((FP)cre_mpf, ((INT)((UINT)TFN_CRE_MPF << 8) + 2), (UD)mpfid, (UD)pk_cmpf);
}

ER_ID _kernel_acre_mpf(T_CMBF *pk_cmpf) {
    return _kernel_systrace_p1_entry((FP)acre_mpf, ((INT)((UINT)TFN_ACRE_MPF << 8) + 1), (UD)pk_cmpf);
}

