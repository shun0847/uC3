/**
 * @file    uC3calpor_if.c
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
 * Call Rendezvous                              *
 ************************************************/
ER _kernel_tcal_por(ID porid, RDVPTN calptn, VP msg, UINT cmsgsz, TMO tmout) {
    INT code = (tmout == TMO_FEVR)?((INT)((UINT)TFN_CAL_POR  << 8) + 4)
                                 :((INT)((UINT)TFN_TCAL_POR << 8) + 5);
    return  _kernel_systrace_p5_entry((FP)tcal_por, code, (UD)porid, (UD)calptn, (UD)msg, (UD)cmsgsz, (UD)tmout);
}

