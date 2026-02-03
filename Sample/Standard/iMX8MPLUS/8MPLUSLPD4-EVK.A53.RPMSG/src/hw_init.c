/**
 * @file    hw_init.c
 * @brief   Hardware initialization for 8MPLUSLPD4-EVK (i.MX8M Plus, Cortex-A53)
 * @date    2025.02.06
 * @author  Copyright (c) 2025, eForce Co., Ltd. All rights reserved.
 *
 ****************************************************************************
 * @par     History
 *          - rev 1.0 (2025.02.06)
 *            Initial version.
 ****************************************************************************
 */
#include "cpu_cfg.h"
#include "kernel.h"
#include "imx8mplus_uC3.h"

/* Control DBGEN for the timer */
#define TIMER_DBGEN         0x1     /* 0x0 for clear, 0x1 for set */

/* Private function prototypes -----------------------------------------------*/

static void clock_init(void);
static void timer_init(void);

static void clock_init(void)
{
}

/*
 * Setup timer (system counter or etc.) configuration for CA53
 *
 */
static void timer_init(void)
{
    volatile UW tempreg;

    /* DBGEN setting expecially for the system counter
     * See 4.12.4.1 Counter Control Register (SYS_CTR_CONTROL_CNTCR)
     * Only the primary core have to do this setting.
     */
    tempreg = REG_SYS_CTR_CTRL.CNTCR;
#if (TIMER_DBGEN == 0x0)
    tempreg &= ~(0x2U);
#elif (TIMER_DBGEN == 0x1)
    tempreg |= 0x2U;
#else
#error "Invalid TIMER_DBGEN"
#endif

    REG_SYS_CTR_CTRL.CNTCR = tempreg;
}

/*
 * Initialize hardware
 */
void hw_init(void)
{
    clock_init();
    timer_init();
}
