/**
 * @file    uC3vstatsk_if.c
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
 * Activate Task                                *
 ************************************************/
ER _kernel_vsta_tsk(ID coreid, ID tskid, VP_INT stacd) {
    return _kernel_systrace_p3_entry((FP)vsta_tsk,  ((INT)(TFN_VSTA_TSK << 8) + 3), (UD)coreid, (UD)tskid, (UD)stacd);
}

