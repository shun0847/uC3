/**
 * @file    uC3crepor_if.c
 * @brief   Micro C Cube Standard, KERNEL
 *          API Hook routine header (ARMv8A AArch64)
 * @date    2021.01.25
 * @author  Copyright (c) 2018-2021, eForce Co., Ltd. All rights reserved.
 *
 ******************************************************************************
 * @par     History
 *          - rev 1.0 (2018.05.23) yokota
 *            Initial version.
 *          - rev 1.1 (2020.03.13) yokota
 *            fix wrong target function.
 *          - rev 1.2 (2021.01.25) Imada
 *            Fixed C++test warnings.
 ******************************************************************************/
#include "uC3hook.h"

/************************************************
 * Create Rendezvous Port                       *
 ************************************************/
ER _kernel_cre_por(ID porid, T_CPOR *pk_cpor) {
    return _kernel_systrace_p2_entry((FP)cre_por, ((INT)((UINT)TFN_CRE_POR << 8) + 2), (UD)porid, (UD)pk_cpor);
}

ER_ID _kernel_acre_por(T_CPOR *pk_cpor) {
    return _kernel_systrace_p1_entry((FP)acre_por, ((INT)((UINT)TFN_ACRE_POR << 8) + 1), (UD)pk_cpor);
}

