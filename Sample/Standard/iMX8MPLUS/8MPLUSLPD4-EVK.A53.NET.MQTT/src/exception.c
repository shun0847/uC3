/**
 * @file    exception.c
 * @brief   Sample program for 8MPLUSLPD4-EVK (i.MX8M Plus, Cortex-A53)
 *          Exception handler
 * @date    2020.12.08
 * @author  Copyright (c) 2020, eForce Co., Ltd. All rights reserved.
 *
 ****************************************************************************
 * @par     History
 *          - rev 1.0 (2020.12.08) Imada
 *            Initial version.
 ****************************************************************************
 */
#include "kernel.h"

/*
 * Error state
 */
void int_abort(void)
{
    for(;;);

    return;
}

/*
 * Synchronous exception handler
 */
void synchronous_exception_handler(UD esr, UD far, UD sp)
{
    (void)esr;
    (void)far;
    (void)sp;

    for(;;);

    return;
}

/*
 * System error handler
 */
void system_error_handler(UD esr, UD far, UD sp)
{
    (void)esr;
    (void)far;
    (void)sp;

    for(;;);

    return;
}
