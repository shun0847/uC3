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
 * Send Data Queue                              *
 ************************************************/
ER _kernel_vpsnd_dtq(ID coreid, ID dtqid, VP_INT data) {
    return _kernel_systrace_p3_entry((FP)vpsnd_dtq,  ((INT)(TFN_VPSND_DTQ << 8) + 3), (UD)coreid, (UD)dtqid, (UD)data);
}

ER _kernel_vfsnd_dtq(ID coreid, ID dtqid, VP_INT data) {
    return _kernel_systrace_p3_entry((FP)vfsnd_dtq,  ((INT)(TFN_VFSND_DTQ << 8) + 3), (UD)coreid, (UD)dtqid, (UD)data);
}

