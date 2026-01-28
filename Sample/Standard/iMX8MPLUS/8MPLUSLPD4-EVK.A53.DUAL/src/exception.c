/**
 * @brief   Sample program for 8MPLUSLPD4-EVK (i.MX8M Plus, Cortex-A53)
 *          Exception handler
 * @date    2025.09.29
 *
 * @copyright (C) 2025, eForce Co., Ltd.
 */
#include "kernel.h"

/*
 * Error state
 */
void int_abort(void)
{
    for (;;)
        ;

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

    for (;;)
        ;

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

    for (;;)
        ;

    return;
}

const T_DEXC dexc_sync = {TA_HLNG, (FP)synchronous_exception_handler};

const T_DEXC dexc_serr = {TA_HLNG, (FP)system_error_handler};
