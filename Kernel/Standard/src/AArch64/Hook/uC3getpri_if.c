/**
 * @file    uC3getpri_if.c
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
 * Reference Task Priority                      *
 ************************************************/
ER _kernel_get_pri(ID tskid, PRI *p_tskpri) {
    return _kernel_systrace_p2_entry((FP)get_pri, ((INT)((UINT)TFN_GET_PRI << 8) + 2), (UD)tskid, (UD)p_tskpri);
}

