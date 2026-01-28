/**
 * @file    uC3dummy.c
 * @brief   Micro C Cube Standard, KERNEL
 *          AArch64 dependent function
 * @date    2021.01.25
 * @author  Copyright (c) 2016, eForce Co., Ltd. All rights reserved.
 *
 ******************************************************************************
 * @par     History
 *          - rev 1.0 (2016.09.21) y-kim
 *            Created based on uC3dummy.c of ARMv7-A.
 *          - rev 1.1 (2021.01.25) Imada
 *            Fixed C++test warnings.
 ******************************************************************************
 */
#include "uC3sys.h"

/***********************************
    Dummy function
 ***********************************/

void _kernel_start_multi_task(void)
{
    ;
}

void _kernel_spin_lock(UW *flag)
{
    (void)flag;
    ;
}

void _kernel_spin_unlock(UW *flag)
{
    (void)flag;
    ;
}

ID _kernel_spin_lock_test(UW *flag)
{
    (void)flag;
    return (ID)0;
}

ID _kernel_spin_locked_test(UW *flag)
{
    (void)flag;
    return (ID)0;
}
