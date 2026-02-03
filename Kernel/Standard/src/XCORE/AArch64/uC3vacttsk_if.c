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
 * Send Message Buffer                          *
 ************************************************/
ER _kernel_vact_tsk(ID coreid, ID tskid) {
    return _kernel_systrace_p2_entry((FP)vact_tsk,  ((INT)(TFN_VACT_TSK << 8) + 2), (UD)coreid, (UD)tskid);
}

