/**
 * @file    hw_init.c
 * @brief   Hardware initialization for 8MPLUSLPD4-EVK (i.MX8M Plus, Cortex-A53)
 * @date    2025.02.06
 * @author  Copyright (c) 2021-2025, eForce Co., Ltd. All rights reserved.
 *
 ****************************************************************************
 * @par     History
 *          - rev 1.0 (2021.01.19) Imada
 *            Initial version.
 *          - rev 1.1 (2021.10.21) Imada
 *            TIMER_DBGEN enabled by default.
 *          - rev 1.2 (2022.03.04) Imada
 *            Fixed CA53 core frequency change.
 *          - rev 1.3 (2024.10.09) Imada
 *            Added UART4 initialization.
 *            Introduced UC3BOOT_FROM_LINUX for linux boot.
 *          - rev 1.4 (2025.02.06) 
 *            Fixed C warnings.
 ****************************************************************************
 */
#include "cpu_cfg.h"
#include "kernel.h"
#include "imx8mplus_uC3.h"
#include "DDR_iMX_UART_cfg.h"
#include "fsl_clock.h"
#include "fsl_iomuxc.h"
#include "sample_uart_cfg.h"

/* Control DBGEN for the timer */
#define TIMER_DBGEN         0x1     /* 0x0 for clear, 0x1 for set */

/* Private function prototypes -----------------------------------------------*/

static void uart_init(void);
static void clock_init(void);
static void timer_init(void);
static void usdhc3_init(void);

/*
 * Setup IOMUX pins and enable clock for UART
 */
static void uart_init(void)
{
    volatile UW tempreg;
#ifdef UART_2
    /* Set UART2 clock root to Osc24M */
    tempreg = REG_CCM_ROOT(UART2_CLK_ROOT).TARGET_ROOT;
    tempreg = (tempreg & ~0x7000000U) | (0U << 24);
    REG_CCM_ROOT(UART2_CLK_ROOT).TARGET_ROOT = tempreg;

    /* Set UART2 root divider to {1, 1} */
    tempreg = REG_CCM_ROOT(UART2_CLK_ROOT).TARGET_ROOT;
    tempreg = (tempreg & (~(0x70000U | 0x3FU))) | ((1U - 1U) << 16) | ((1U - 1U) << 0);
    REG_CCM_ROOT(UART2_CLK_ROOT).TARGET_ROOT = tempreg;

    /** Configure pins */
    REG_IOMUXC_SW_MUX_CTL_PAD(SW_MUX_CTL_PAD_UART2_RXD) = 0U | (0U << 4);
    REG_IOMUXC_SW_MUX_CTL_PAD(SW_MUX_CTL_PAD_UART2_TXD) = 0U | (0U << 4);
    REG_IOMUXC_SW_PAD_CTL_PAD(SW_PAD_CTL_PAD_UART2_RXD) = 6U | (2U << 3); // Fast rate, x6 drive strength
    REG_IOMUXC_SW_PAD_CTL_PAD(SW_PAD_CTL_PAD_UART2_TXD) = 6U | (2U << 3); // Fast rate, x6 drive strength
    REG_IOMUXC_SELECT_INPUT(SELECT_INPUT_UART2_UART_RXD_MUX) = 6U;   /** UART2_RX_DATA_ALT0 */

    REG_CCM_CCGR(CCGR_UART2).CCGR_CLR = 0x3333U; /** disable the clock gate */
    REG_CCM_CCGR(CCGR_UART2).CCGR_SET = 0x3333U; /** enable the clock gate */
    _kernel_synch_cache();
#endif

#ifdef UART_4
    /* Set UART4 clock root to Osc24M */
    tempreg = REG_CCM_ROOT(UART4_CLK_ROOT).TARGET_ROOT;
    tempreg = (tempreg & ~0x7000000U) | (0U << 24);
    REG_CCM_ROOT(UART4_CLK_ROOT).TARGET_ROOT = tempreg;

    /* Set UART4 root divider to {1, 1} */
    tempreg = REG_CCM_ROOT(UART4_CLK_ROOT).TARGET_ROOT;
    tempreg = (tempreg & (~(0x70000U | 0x3FU))) | ((1U - 1U) << 16) | ((1UL - 1UL) << 0);
    REG_CCM_ROOT(UART4_CLK_ROOT).TARGET_ROOT = tempreg;

    /** Configure pins */
    REG_IOMUXC_SW_MUX_CTL_PAD(SW_MUX_CTL_PAD_UART4_RXD) = 0U | (0U << 4);
    REG_IOMUXC_SW_MUX_CTL_PAD(SW_MUX_CTL_PAD_UART4_TXD) = 0U | (0U << 4);
    REG_IOMUXC_SW_PAD_CTL_PAD(SW_PAD_CTL_PAD_UART4_RXD) = 6U | (2U << 3); // Fast rate, x6 drive strength
    REG_IOMUXC_SW_PAD_CTL_PAD(SW_PAD_CTL_PAD_UART4_TXD) = 6U | (2U << 3); // Fast rate, x6 drive strength
    REG_IOMUXC_SELECT_INPUT(SELECT_INPUT_UART4_UART_RXD_MUX) = 8U;   /** UART4_RX_DATA_ALT0 */

    REG_CCM_CCGR(CCGR_UART4).CCGR_SET = 0x2222U; /** enable the clock gate */
    _kernel_synch_cache();
#endif
}

static void clock_init(void)
{
#ifndef UC3BOOT_FROM_LINUX
    volatile UW tempreg;
    /* Set A53 clock root to SYSTEMPLL2_CLK */
    tempreg = REG_CCM_ROOT(ARM_A53_CLK_ROOT).TARGET_ROOT;
    tempreg = (tempreg & ~0x7000000U) | (0x2U << 24);
    REG_CCM_ROOT(ARM_A53_CLK_ROOT).TARGET_ROOT = tempreg;

    /* Set A53 root divider to {1, 1} */
    /* A53 runs @ 1200MHz with this setting (same as the speed u-boot sets) */
    tempreg = REG_CCM_ROOT(ARM_A53_CLK_ROOT).TARGET_ROOT;
    tempreg = (tempreg & (~(0x70000U | 0x3FU))) | ((1U - 1U) << 16U) | ((1U - 1U) << 0U);
    REG_CCM_ROOT(ARM_A53_CLK_ROOT).TARGET_ROOT = tempreg;

    /* Set A53 clock root to ARM_PLL_CLK */
    tempreg = REG_CCM_ROOT(ARM_A53_CLK_ROOT).TARGET_ROOT;
    tempreg = (tempreg & ~0x7000000U) | (0x1U << 24);
    REG_CCM_ROOT(ARM_A53_CLK_ROOT).TARGET_ROOT = tempreg;

    _kernel_synch_cache();
#endif /* #ifndef UC3BOOT_FROM_LINUX */
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

static void usdhc3_init(void)
{
    /* USDHC3 root clock: SysPLL1/2 -> ~200MHz (mux=1, post=2) */
    CLOCK_UpdateRoot(kCLOCK_RootUsdhc3, 1U, 1U, 2U);
    CLOCK_EnableClock(kCLOCK_Usdhc3);

    /* Pad config: drive + fast */
    const uint32_t usdhc_pad_clk = IOMUXC_SW_PAD_CTL_PAD_DSE(6U) |
                                   IOMUXC_SW_PAD_CTL_PAD_FSEL_MASK;
    const uint32_t usdhc_pad_data = usdhc_pad_clk |
                                    IOMUXC_SW_PAD_CTL_PAD_PUE_MASK |
                                    IOMUXC_SW_PAD_CTL_PAD_PE_MASK |
                                    IOMUXC_SW_PAD_CTL_PAD_HYS_MASK;

    /* USDHC3 pins on ENET pads (EVK eMMC) */
    IOMUXC_SetPinMux(IOMUXC_NAND_WE_B_USDHC3_CLK, 0U);
    IOMUXC_SetPinConfig(IOMUXC_NAND_WE_B_USDHC3_CLK, usdhc_pad_clk);

    IOMUXC_SetPinMux(IOMUXC_NAND_WP_B_USDHC3_CMD, 0U);
    IOMUXC_SetPinConfig(IOMUXC_NAND_WP_B_USDHC3_CMD, usdhc_pad_data);

    IOMUXC_SetPinMux(IOMUXC_NAND_DATA04_USDHC3_DATA0, 0U);
    IOMUXC_SetPinConfig(IOMUXC_NAND_DATA04_USDHC3_DATA0, usdhc_pad_data);

    IOMUXC_SetPinMux(IOMUXC_NAND_DATA05_USDHC3_DATA1, 0U);
    IOMUXC_SetPinConfig(IOMUXC_NAND_DATA05_USDHC3_DATA1, usdhc_pad_data);

    IOMUXC_SetPinMux(IOMUXC_NAND_DATA06_USDHC3_DATA2, 0U);
    IOMUXC_SetPinConfig(IOMUXC_NAND_DATA06_USDHC3_DATA2, usdhc_pad_data);

    IOMUXC_SetPinMux(IOMUXC_NAND_DATA07_USDHC3_DATA3, 0U);
    IOMUXC_SetPinConfig(IOMUXC_NAND_DATA07_USDHC3_DATA3, usdhc_pad_data);

    IOMUXC_SetPinMux(IOMUXC_NAND_RE_B_USDHC3_DATA4, 0U);
    IOMUXC_SetPinConfig(IOMUXC_NAND_RE_B_USDHC3_DATA4, usdhc_pad_data);

    IOMUXC_SetPinMux(IOMUXC_NAND_CE2_B_USDHC3_DATA5, 0U);
    IOMUXC_SetPinConfig(IOMUXC_NAND_CE2_B_USDHC3_DATA5, usdhc_pad_data);

    IOMUXC_SetPinMux(IOMUXC_NAND_CE3_B_USDHC3_DATA6, 0U);
    IOMUXC_SetPinConfig(IOMUXC_NAND_CE3_B_USDHC3_DATA6, usdhc_pad_data);

    IOMUXC_SetPinMux(IOMUXC_NAND_CLE_USDHC3_DATA7, 0U);
    IOMUXC_SetPinConfig(IOMUXC_NAND_CLE_USDHC3_DATA7, usdhc_pad_data);
#if 1
    /* eMMC RESET_B */
    IOMUXC_SetPinMux(IOMUXC_NAND_READY_B_USDHC3_RESET_B, 0U);
    IOMUXC_SetPinConfig(IOMUXC_NAND_READY_B_USDHC3_RESET_B, usdhc_pad_data);
#endif
    /* HS200 strobe */
    IOMUXC_SetPinMux(IOMUXC_NAND_CE1_B_USDHC3_STROBE, 0U);
    IOMUXC_SetPinConfig(IOMUXC_NAND_CE1_B_USDHC3_STROBE, usdhc_pad_clk);
#if 0
    /* USDHC3 VSELECT */
    IOMUXC_SetPinMux(IOMUXC_GPIO1_IO11_USDHC3_VSELECT, 0U);
    IOMUXC_SetPinConfig(IOMUXC_GPIO1_IO11_USDHC3_VSELECT, usdhc_pad_data);
#endif
    _kernel_synch_cache();

}

/*
 * Initialize hardware
 */
void hw_init(void)
{
    clock_init();
    uart_init();
    usdhc3_init();
    timer_init();
}
