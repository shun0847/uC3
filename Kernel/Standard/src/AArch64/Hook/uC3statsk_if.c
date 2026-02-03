/**
 * @file    uC3statsk_if.c
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
 * Activate Task (with a Start Code)            *
 ************************************************/
ER _kernel_sta_tsk(ID tskid, VP_INT stacd) {
    return _kernel_systrace_p2_entry((FP)sta_tsk, ((INT)((UINT)TFN_STA_TSK << 8) + 2), (UD)tskid, (UD)stacd);
}

