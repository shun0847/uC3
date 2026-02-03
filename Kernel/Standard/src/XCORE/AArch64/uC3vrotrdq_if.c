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
 * Rotate Ready Queue                           *
 ************************************************/
ER _kernel_vrot_rdq(ID coreid, PRI tskpri) {
    return _kernel_systrace_p2_entry((FP)vrot_rdq,  ((INT)(TFN_VROT_RDQ << 8) + 2), (UD)coreid, (UD)tskpri);
}

