/**
 * @file    uC3slptsk_if.c
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
 * Put Task to Sleep                            *
 ************************************************/
ER _kernel_tslp_tsk(TMO timout) {
    INT code = (timout == TMO_FEVR)?((INT)((UINT)TFN_SLP_TSK  << 8) + 0)
                                   :((INT)((UINT)TFN_TSLP_TSK << 8) + 1);
    return _kernel_systrace_p1_entry((FP)tslp_tsk, code, (UD)timout);
}

