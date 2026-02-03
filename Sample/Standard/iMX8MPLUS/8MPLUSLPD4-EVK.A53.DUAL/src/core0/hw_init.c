/**
 * @brief   Hardware initialization for 8MPLUSLPD4-EVK (i.MX8M Plus, Cortex-A53)
 * @date    2025.09.29
 *
 * @copyright (C) 2025, eForce Co., Ltd.
 */
#include "kernel.h"
#include "imx8mplus_uC3.h"
#include "DDR_iMX_UART_cfg.h"

/* Control DBGEN for the timer */
#define TIMER_DBGEN 0x1 /* 0x0 for clear, 0x1 for set */

/* Private function prototypes -----------------------------------------------*/

static void uart_init(void);
static void clock_init(void);
static void timer_init(void);

/*
 * Setup IOMUX pins and enable clock for UART
 */
static void uart_init(void)
{
    volatile UW tempreg;
#ifdef UART_2
    /* Set UART2 clock root to Osc24M */
    tempreg = REG_CCM_ROOT(UART2_CLK_ROOT).TARGET_ROOT;
    tempreg = (tempreg & ~0x7000000UL) | (0UL << 24);
    REG_CCM_ROOT(UART2_CLK_ROOT).TARGET_ROOT = tempreg;

    /* Set UART2 root divider to {1, 1} */
    tempreg = REG_CCM_ROOT(UART2_CLK_ROOT).TARGET_ROOT;
    tempreg = (tempreg & (~(0x70000UL | 0x3FU))) | ((1UL - 1UL) << 16) | ((1UL - 1UL) << 0);
    REG_CCM_ROOT(UART2_CLK_ROOT).TARGET_ROOT = tempreg;

    /** Configure pins */
    REG_IOMUXC_SW_MUX_CTL_PAD(SW_MUX_CTL_PAD_UART2_RXD) = 0UL | (0U << 4);
    REG_IOMUXC_SW_MUX_CTL_PAD(SW_MUX_CTL_PAD_UART2_TXD) = 0UL | (0U << 4);
    REG_IOMUXC_SW_PAD_CTL_PAD(SW_PAD_CTL_PAD_UART2_RXD) = 6UL | (2U << 3);    // Fast rate, x6 drive strength
    REG_IOMUXC_SW_PAD_CTL_PAD(SW_PAD_CTL_PAD_UART2_TXD) = 6UL | (2U << 3);    // Fast rate, x6 drive strength
    REG_IOMUXC_SELECT_INPUT(SELECT_INPUT_UART2_UART_RXD_MUX) = 6UL;           /** UART2_RX_DATA_ALT0 */

    REG_CCM_CCGR(CCGR_UART2).CCGR_CLR = 0x3333U; /** disable the clock gate */
    REG_CCM_CCGR(CCGR_UART2).CCGR_SET = 0x3333U; /** enable the clock gate */
    _kernel_synch_cache();
#endif
}

static void clock_init(void)
{
    volatile UW tempreg;
    /* Set A53 clock root to SYSTEMPLL2_CLK */
    tempreg = REG_CCM_ROOT(ARM_A53_CLK_ROOT).TARGET_ROOT;
    tempreg = (tempreg & ~0x7000000UL) | (0x2UL << 24);
    REG_CCM_ROOT(ARM_A53_CLK_ROOT).TARGET_ROOT = tempreg;

    /* Set A53 root divider to {1, 1} */
    /* A53 runs @ 1200MHz with this setting (same as the speed u-boot sets) */
    tempreg = REG_CCM_ROOT(ARM_A53_CLK_ROOT).TARGET_ROOT;
    tempreg = (tempreg & (~(0x70000UL | 0x3FU))) | ((1U - 1U) << 16U) | ((1U - 1U) << 0U);
    REG_CCM_ROOT(ARM_A53_CLK_ROOT).TARGET_ROOT = tempreg;

    /* Set A53 clock root to ARM_PLL_CLK */
    tempreg = REG_CCM_ROOT(ARM_A53_CLK_ROOT).TARGET_ROOT;
    tempreg = (tempreg & ~0x7000000UL) | (0x1UL << 24);
    REG_CCM_ROOT(ARM_A53_CLK_ROOT).TARGET_ROOT = tempreg;

    _kernel_synch_cache();
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
    uart_init();
    timer_init();
}
