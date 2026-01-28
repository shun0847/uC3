/**
 * @file    uC3chgpri_if.src
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
 * Change Task Priority                         *
 ************************************************/
ER _kernel_chg_pri(ID tskid, PRI tskpri) {
    return _kernel_systrace_p2_entry((FP)chg_pri, ((INT)((UINT)TFN_CHG_PRI << 8) + 1), (UD)tskid, (UD)tskpri);
}

