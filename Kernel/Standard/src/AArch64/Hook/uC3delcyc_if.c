/**
 * @file    uC3delcyc_if.c
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
 * Delete Cyclic Handler                        *
 ************************************************/
ER _kernel_del_cyc(ID cycid) {
    return _kernel_systrace_p1_entry((FP)del_cyc, ((INT)((UINT)TFN_DEL_CYC << 8) + 1), (UD)cycid);
}

