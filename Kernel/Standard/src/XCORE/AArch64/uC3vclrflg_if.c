/**
 * @file    uC3vacttsk_if.c
 * @brief   Micro C Cube Standard, KERNEL
 *          API Hook routine header (ARMv8A AArch64)
 * @date    2018.05.25
 * @author  Copyright (c) 2018, eForce Co., Ltd. All rights reserved.
 *
 ******************************************************************************
 * @par     History
 *          - rev 1.0 (2018.05.25) yokota
 *            Initial version.
 ******************************************************************************/
#include "../../AArch64/Hook/uC3hook.h"

/************************************************
 * Clear Eventflag                              *
 ************************************************/
ER _kernel_vclr_flg(ID coreid, ID flgid, FLGPTN clrptn) {
    return _kernel_systrace_p3_entry((FP)vclr_flg,  ((INT)(TFN_VCLR_FLG << 8) + 3), (UD)coreid, (UD)flgid, (UD)clrptn);
}

