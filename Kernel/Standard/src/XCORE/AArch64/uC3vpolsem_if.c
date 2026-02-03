/**
 * @file    uC3vpolsem_if.c
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
 * Acquire Semaphore Resource                   *
 ************************************************/
ER _kernel_vpol_sem(ID coreid, ID semid) {
    return _kernel_systrace_p2_entry((FP)vpol_sem,  ((INT)(TFN_VPOL_SEM << 8) + 2), (UD)coreid, (UD)semid);
}

