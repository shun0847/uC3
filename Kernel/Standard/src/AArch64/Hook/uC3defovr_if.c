/**
 * @file    uC3defovr_if.c
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
 * Define Overrun Handler                       *
 ************************************************/
ER _kernel_def_ovr(T_DOVR *pk_dovr) {
    return _kernel_systrace_p1_entry((FP)def_ovr, ((INT)((UINT)TFN_DEF_OVR << 8) + 1), (UD)pk_dovr);
}
