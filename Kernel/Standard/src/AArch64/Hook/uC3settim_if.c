/**
 * @file    uC3settim_if.c
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
 * Set System Time                              *
 ************************************************/
ER _kernel_set_tim(SYSTIM *p_systim) {
    return _kernel_systrace_p1_entry((FP)set_tim, ((INT)((UINT)TFN_SET_TIM << 8) + 1), (UD)p_systim);
}

