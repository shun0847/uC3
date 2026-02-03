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
 * Recieve Data Qeueue                          *
 ************************************************/
ER _kernel_vprcv_dtq(ID coreid, ID dtqid, VP_INT *p_data) {
    return _kernel_systrace_p3_entry((FP)vprcv_dtq,  ((INT)(TFN_VPRCV_DTQ << 8) + 3), (UD)coreid, (UD)dtqid, (UD)p_data);
}

