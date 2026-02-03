/**
 * @file    uC3waisem_if.c
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
 * Wait Semaphore Resource                      *
 ************************************************/
ER _kernel_twai_sem(ID semid, TMO tmout) {
    INT code = (tmout == TMO_FEVR)?((INT)((UINT)TFN_WAI_SEM << 8) + 1)
                                 :((tmout == TMO_POL)?((INT)((UINT)TFN_POL_SEM  << 8) + 1)
                                                     :((INT)((UINT)TFN_TWAI_SEM << 8) + 2));
    return _kernel_systrace_p2_entry((FP)twai_sem, code, (UD)semid, (UD)tmout);
}

