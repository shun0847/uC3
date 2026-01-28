/**
 * @file    uC3cretsk_if.c
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
 * Create Task                                  *
 ************************************************/
ER _kernel_cre_tsk(ID tskid, T_CTSK *pk_ctsk) {
    return _kernel_systrace_p2_entry((FP)cre_tsk, ((INT)((UINT)TFN_CRE_TSK << 8) + 2), (UD)tskid, (UD)pk_ctsk);
}

ER_ID _kernel_acre_tsk(T_CTSK *pk_ctsk) {
    return _kernel_systrace_p1_entry((FP)acre_tsk, ((INT)((UINT)TFN_ACRE_TSK << 8) + 1), (UD)pk_ctsk);
}
