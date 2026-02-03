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
 * Create Message Buffer                        *
 ************************************************/
ER _kernel_cre_mbf(ID mbfid, T_CMBF *pk_cmbf) {
    return _kernel_systrace_p2_entry((FP)cre_mbf, ((INT)((UINT)TFN_CRE_MBF << 8) + 2), (UD)mbfid, (UD)pk_cmbf);
}

ER_ID _kernel_acre_mbf(T_CMBF *pk_cmbf) {
    return _kernel_systrace_p1_entry((FP)acre_mbf, ((INT)((UINT)TFN_ACRE_MBF << 8) + 1), (UD)pk_cmbf);
}

