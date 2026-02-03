/**
 * @file    uC3chgims_if.c
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
* Change Interrupt Mask Level                  *
************************************************/
ER _kernel_chg_ims(IMASK imask) {
    return _kernel_systrace_p1_entry((FP)chg_ims, ((INT)((UINT)TFN_CHG_IMS << 8) + 1), (UD)imask);
}

