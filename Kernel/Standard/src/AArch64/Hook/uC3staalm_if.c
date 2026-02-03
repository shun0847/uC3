/**
 * @file    uC3staalm_if.c
 * @brief   Micro C Cube Standard, KERNEL
 *          API Hook routine header (ARMv8A AArch64)
 * @date    2021.01.25
 * @author  Copyright (c) 2018-2021, eForce Co., Ltd. All rights reserved.
 *
 ******************************************************************************
 * @par     History
 *          - rev 1.0 (2018.05.25) yokota
 *            Initial version.
 *          - rev 1.1 (2020.03.13) yokota
 *            fix worng target function.
 *          - rev 1.2 (2021.01.25) Imada
 *            Fixed C++test warnings.
 ******************************************************************************/
#include "uC3hook.h"

/************************************************
 * Start Alarm Handler                          *
 ************************************************/
ER _kernel_sta_alm(ID almid, RELTIM almtim) {
    return _kernel_systrace_p2_entry((FP)sta_alm, ((INT)((UINT)TFN_STA_ALM << 8) + 2), (UD)almid, (UD)almtim);
}

