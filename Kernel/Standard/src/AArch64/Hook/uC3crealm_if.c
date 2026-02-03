/**
 * @file    uC3crealm_if.c
 * @brief   Micro C Cube Standard, KERNEL
 *          API Hook routine header (ARMv8A AArch64)
 * @date    2021.01.25
 * @author  Copyright (c) 2018-2021, eForce Co., Ltd. All rights reserved.
 *
 ******************************************************************************
 * @par     History
 *          - rev 1.0 (2018.05.22) yokota
 *            Initial version.
 *          - rev 1.1 (2021.01.25) Imada
 *            Fixed C++test warnings.
 ******************************************************************************/
#include "uC3hook.h"

/************************************************
 * Create Alarm Handler                         *
 ************************************************/

ER _kernel_cre_alm(ID almid, T_CALM *pk_calm) {
    return _kernel_systrace_p2_entry((FP)cre_alm, ((INT)((UINT)TFN_CRE_ALM << 8) + 2), (UD)almid, (UD)pk_calm);
}

ER_ID _kernel_acre_alm(T_CALM *pk_calm) {
    return _kernel_systrace_p1_entry((FP)acre_alm, ((INT)((UINT)TFN_ACRE_ALM << 8) + 1), (UD)pk_calm);
}
