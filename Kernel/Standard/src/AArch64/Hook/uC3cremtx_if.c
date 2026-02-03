/**
 * @file    uC3cremtx_if.c
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
 * Create Mutex                                 *
 ************************************************/
ER _kernel_cre_mtx(ID mtxid, T_CMTX *pk_cmtx) {
    return _kernel_systrace_p2_entry((FP)cre_mtx, ((INT)((UINT)TFN_CRE_MTX << 8) + 2), (UD)mtxid, (UD)pk_cmtx);
}

ER_ID _kernel_acre_mtx(T_CMTX *pk_cmtx) {
    return _kernel_systrace_p1_entry((FP)acre_mtx, ((INT)((UINT)TFN_ACRE_MTX << 8) + 1), (UD)pk_cmtx);
}
