/**
 * @file    uC3fwdpor_if.c
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
 * Forward Rendezvous                           *
 ************************************************/
ER _kernel_fwd_por(ID porid, RDVPTN calptn, RDVNO rdvno, VP msg, UINT cmsgsz) {
    return _kernel_systrace_p5_entry((FP)fwd_por, ((INT)((UINT)TFN_FWD_POR << 8) + 5), (UD)porid, (UD)calptn, (UD)rdvno, (UD)msg, (UD)cmsgsz);
}

