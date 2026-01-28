/**
 * @file    uC3definh_if.c
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
 * Define Interrupt Handler                     *
 ************************************************/
ER _kernel_def_inh(INHNO inhno, T_DINH *pk_dinh) {
    return _kernel_systrace_p2_entry((FP)def_inh, ((INT)((UINT)TFN_DEF_INH << 8) + 2), (UD)inhno, (UD)pk_dinh);
}

