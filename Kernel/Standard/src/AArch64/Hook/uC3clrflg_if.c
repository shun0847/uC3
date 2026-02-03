/**
 * @file    uC3clrflg_if.c
 * @brief   Micro C Cube Standard, KERNEL
 *          API Hook routine header (ARMv8 AArch64)
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
* Clear Eventflag                              *
************************************************/
ER _kernel_clr_flg(ID flgid, FLGPTN clrptn) {
    return _kernel_systrace_p2_entry((FP)clr_flg, ((INT)((UINT)TFN_CLR_FLG << 8) + 1), (UD)flgid, (UD)clrptn);
}

