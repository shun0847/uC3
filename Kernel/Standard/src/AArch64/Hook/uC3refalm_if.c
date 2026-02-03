/**
 * @file    uC3refalm_if.c
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
 * Reference Alarm Handler                      *
 ************************************************/
ER _kernel_ref_alm(ID almid, T_RALM *pk_ralm) {
    return _kernel_systrace_p2_entry((FP)ref_alm, ((INT)((UINT)TFN_REF_ALM << 8) + 2), (UD)almid, (UD)pk_ralm);
}

