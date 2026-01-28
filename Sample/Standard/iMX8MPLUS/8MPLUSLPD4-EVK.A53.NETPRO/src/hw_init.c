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
 *          - rev 1.5 (BETA 2025.10.16) Makino
 *            Added ENET,ENET_QOS pin initialization processing.
 ****************************************************************************
 */
#include "cpu_cfg.h"
#include "kernel.h"
#include "imx8mplus_uC3.h"
#include "DDR_iMX_UART_cfg.h"

/* Control DBGEN for the timer */
#define TIMER_DBGEN         0x1     /* 0x0 for clear, 0x1 for set */

/* Private function prototypes -----------------------------------------------*/

static void uart_init(void);
static void clock_init(void);
static void timer_init(void);
static void eqos_init(void);
static void enet_init(void);

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


static void wait_cnt(UW cnt)
{
    volatile UW i;
    for (i = 0; i < cnt; i++)   ;
}

static void eqos_init(void)
{
    /** Configure pins */
    REG_IOMUXC_SW_MUX_CTL_PAD(SW_MUX_CTL_PAD_ENET_MDC) = 0;     /* IOMUXC_SW_MUX_CTL_PAD_ENET_MDC */
    REG_IOMUXC_SW_MUX_CTL_PAD(SW_MUX_CTL_PAD_ENET_MDIO) = 0;    /* IOMUXC_SW_MUX_CTL_PAD_ENET_MDIO */
    REG_IOMUXC_SW_MUX_CTL_PAD(SW_MUX_CTL_PAD_ENET_TD3) = 0;     /* ALT0_ENET_QOS_RGMII_TD3 */
    REG_IOMUXC_SW_MUX_CTL_PAD(SW_MUX_CTL_PAD_ENET_TD2) = 0;     /* ALT0_ENET_QOS_RGMII_TD2 */
    REG_IOMUXC_SW_MUX_CTL_PAD(SW_MUX_CTL_PAD_ENET_TD1) = 0;     /* ALT0_ENET_QOS_RGMII_TD1 */
    REG_IOMUXC_SW_MUX_CTL_PAD(SW_MUX_CTL_PAD_ENET_TD0) = 0;     /* ALT0_ENET_QOS_RGMII_TD0 */
    REG_IOMUXC_SW_MUX_CTL_PAD(SW_MUX_CTL_PAD_ENET_TX_CTL) = 0;  /* ALT0_ENET_QOS_RGMII_TX_CTL */
    REG_IOMUXC_SW_MUX_CTL_PAD(SW_MUX_CTL_PAD_ENET_TXC) = 0;     /* ALT0_CCM_ENET_QOS_CLOCK_GENERATE_TX_CLK */
    REG_IOMUXC_SW_MUX_CTL_PAD(SW_MUX_CTL_PAD_ENET_RX_CTL) = 0;  /* ALT0_ENET_QOS_RGMII_RX_CTL */
    REG_IOMUXC_SW_MUX_CTL_PAD(SW_MUX_CTL_PAD_ENET_RXC) = 0;     /* ALT0_CCM_ENET_QOS_CLOCK_GENERATE_RX_CLK */
    REG_IOMUXC_SW_MUX_CTL_PAD(SW_MUX_CTL_PAD_ENET_RD0) = 0;     /* ALT0_ENET_QOS_RGMII_RD0 */
    REG_IOMUXC_SW_MUX_CTL_PAD(SW_MUX_CTL_PAD_ENET_RD1) = 0;     /* ALT0_ENET_QOS_RGMII_RD1 */
    REG_IOMUXC_SW_MUX_CTL_PAD(SW_MUX_CTL_PAD_ENET_RD2) = 0;     /* ALT0_ENET_QOS_RGMII_RD2 */
    REG_IOMUXC_SW_MUX_CTL_PAD(SW_MUX_CTL_PAD_ENET_RD3) = 0;     /* ALT0_ENET_QOS_RGMII_RD3 */
    _kernel_synch_cache();

    REG_IOMUXC_SELECT_INPUT(SELECT_INPUT_ENET_QOS_GMII_MDI_I) = 1;   /* SELECT_ENET_MDIO_ALT0 */
    _kernel_synch_cache();
 
    REG_IOMUXC_SW_PAD_CTL_PAD(SW_PAD_CTL_PAD_ENET_MDC) = 0x03;
    REG_IOMUXC_SW_PAD_CTL_PAD(SW_PAD_CTL_PAD_ENET_MDIO) = 0x03;
    REG_IOMUXC_SW_PAD_CTL_PAD(SW_PAD_CTL_PAD_ENET_TD3) = 0x1f;
    REG_IOMUXC_SW_PAD_CTL_PAD(SW_PAD_CTL_PAD_ENET_TD2) = 0x1f;
    REG_IOMUXC_SW_PAD_CTL_PAD(SW_PAD_CTL_PAD_ENET_TD1) = 0x1f;
    REG_IOMUXC_SW_PAD_CTL_PAD(SW_PAD_CTL_PAD_ENET_TD0) = 0x1f;
    REG_IOMUXC_SW_PAD_CTL_PAD(SW_PAD_CTL_PAD_ENET_TX_CTL) = 0x1f;
    REG_IOMUXC_SW_PAD_CTL_PAD(SW_PAD_CTL_PAD_ENET_TXC) = 0x1f;
    REG_IOMUXC_SW_PAD_CTL_PAD(SW_PAD_CTL_PAD_ENET_RX_CTL) = 0x91;
    REG_IOMUXC_SW_PAD_CTL_PAD(SW_PAD_CTL_PAD_ENET_RXC) = 0x91;
    REG_IOMUXC_SW_PAD_CTL_PAD(SW_PAD_CTL_PAD_ENET_RD0) = 0x91;
    REG_IOMUXC_SW_PAD_CTL_PAD(SW_PAD_CTL_PAD_ENET_RD1) = 0x91;
    REG_IOMUXC_SW_PAD_CTL_PAD(SW_PAD_CTL_PAD_ENET_RD2) = 0x91;
    REG_IOMUXC_SW_PAD_CTL_PAD(SW_PAD_CTL_PAD_ENET_RD3) = 0x91;
    _kernel_synch_cache();

#if 1 /* ENET_nRST (GPIO) */
    REG_IOMUXC_SW_MUX_CTL_PAD(SW_MUX_CTL_PAD_SAI2_RXC) = 5;     /* ALT5_GPIO4_IO[22] */
    REG_IOMUXC_SW_PAD_CTL_PAD(SW_PAD_CTL_PAD_SAI2_RXC) = 0x19;
    REG_GPIO4.GDIR |= 1 << 22;  /* output mode */
    REG_GPIO4.DR |= 1 << 22;
    wait_cnt(10000);
    REG_GPIO4.DR |= 0 << 22;
    wait_cnt(10000);
    REG_GPIO4.DR |= 1 << 22;
    _kernel_synch_cache();
#endif
}

static void enet_init(void)
{
    /** Configure pins */
    REG_IOMUXC_SW_MUX_CTL_PAD(SW_MUX_CTL_PAD_SAI1_RXD2) = 4;    /* ALT4_ENET1_MDC */
    REG_IOMUXC_SW_MUX_CTL_PAD(SW_MUX_CTL_PAD_SAI1_RXD3) = 4;    /* ALT4_ENET1_MDIO */
    REG_IOMUXC_SW_MUX_CTL_PAD(SW_MUX_CTL_PAD_SAI1_TXD3) = 4;    /* ALT4_ENET1_RGMII_TD3 */
    REG_IOMUXC_SW_MUX_CTL_PAD(SW_MUX_CTL_PAD_SAI1_TXD2) = 4;    /* ALT4_ENET1_RGMII_TD2 */
    REG_IOMUXC_SW_MUX_CTL_PAD(SW_MUX_CTL_PAD_SAI1_TXD1) = 4;    /* ALT4_ENET1_RGMII_TD1 */
    REG_IOMUXC_SW_MUX_CTL_PAD(SW_MUX_CTL_PAD_SAI1_TXD0) = 4;    /* ALT4_ENET1_RGMII_TD0 */
    REG_IOMUXC_SW_MUX_CTL_PAD(SW_MUX_CTL_PAD_SAI1_TXD4) = 4;    /* ALT4_ENET1_RGMII_TX_CTL */
    REG_IOMUXC_SW_MUX_CTL_PAD(SW_MUX_CTL_PAD_SAI1_TXD5) = 4;    /* ALT4_ENET1_RGMII_TXC */
    REG_IOMUXC_SW_MUX_CTL_PAD(SW_MUX_CTL_PAD_SAI1_TXC) = 4;     /* ALT4_ENET1_RGMII_RXC */
    REG_IOMUXC_SW_MUX_CTL_PAD(SW_MUX_CTL_PAD_SAI1_TXFS) = 4;    /* ALT4_ENET1_RGMII_RX_CTL */
    REG_IOMUXC_SW_MUX_CTL_PAD(SW_MUX_CTL_PAD_SAI1_RXD4) = 4;    /* ALT4_ENET1_RGMII_RD0 */
    REG_IOMUXC_SW_MUX_CTL_PAD(SW_MUX_CTL_PAD_SAI1_RXD5) = 4;    /* ALT4_ENET1_RGMII_RD1 */
    REG_IOMUXC_SW_MUX_CTL_PAD(SW_MUX_CTL_PAD_SAI1_RXD6) = 4;    /* ALT4_ENET1_RGMII_RD2 */
    REG_IOMUXC_SW_MUX_CTL_PAD(SW_MUX_CTL_PAD_SAI1_RXD7) = 4;    /* ALT4_ENET1_RGMII_RD3 */
    _kernel_synch_cache();

    REG_IOMUXC_SELECT_INPUT(SELECT_INPUT_ENET1_MDIO) = 1;       /* SELECT_SAI1_RXD3_ALT4 */
    REG_IOMUXC_SELECT_INPUT(SELECT_INPUT_ENET1_RXDATA_0) = 1;   /* SELECT_SAI1_RXD4_ALT4 */
    REG_IOMUXC_SELECT_INPUT(SELECT_INPUT_ENET1_RXDATA_1) = 1;   /* SELECT_SAI1_RXD5_ALT4 */
    REG_IOMUXC_SELECT_INPUT(SELECT_INPUT_ENET1_RXEN) = 1;       /* SELECT_SAI1_TXFS_ALT4 */
    //REG_IOMUXC_SELECT_INPUT(SELECT_INPUT_ENET1_RXERR) = 1;      /* SELECT_SAI1_TXD6_ALT4 */
    _kernel_synch_cache();
 
    REG_IOMUXC_SW_PAD_CTL_PAD(SW_PAD_CTL_PAD_SAI1_RXD2) = 0x03;
    REG_IOMUXC_SW_PAD_CTL_PAD(SW_PAD_CTL_PAD_SAI1_RXD3) = 0x03;
    REG_IOMUXC_SW_PAD_CTL_PAD(SW_PAD_CTL_PAD_SAI1_TXD3) = 0x1f;
    REG_IOMUXC_SW_PAD_CTL_PAD(SW_PAD_CTL_PAD_SAI1_TXD2) = 0x1f;
    REG_IOMUXC_SW_PAD_CTL_PAD(SW_PAD_CTL_PAD_SAI1_TXD1) = 0x1f;
    REG_IOMUXC_SW_PAD_CTL_PAD(SW_PAD_CTL_PAD_SAI1_TXD0) = 0x1f;
    REG_IOMUXC_SW_PAD_CTL_PAD(SW_PAD_CTL_PAD_SAI1_TXD4) = 0x1f;
    REG_IOMUXC_SW_PAD_CTL_PAD(SW_PAD_CTL_PAD_SAI1_TXD5) = 0x1f;
    REG_IOMUXC_SW_PAD_CTL_PAD(SW_PAD_CTL_PAD_SAI1_TXC) = 0x91;
    REG_IOMUXC_SW_PAD_CTL_PAD(SW_PAD_CTL_PAD_SAI1_TXFS) = 0x91;
    REG_IOMUXC_SW_PAD_CTL_PAD(SW_PAD_CTL_PAD_SAI1_RXD4) = 0x91;
    REG_IOMUXC_SW_PAD_CTL_PAD(SW_PAD_CTL_PAD_SAI1_RXD5) = 0x91;
    REG_IOMUXC_SW_PAD_CTL_PAD(SW_PAD_CTL_PAD_SAI1_RXD6) = 0x91;
    REG_IOMUXC_SW_PAD_CTL_PAD(SW_PAD_CTL_PAD_SAI1_RXD7) = 0x91;
    _kernel_synch_cache();

#if 1 /* ENET1_nRST (GPIO) */
    REG_IOMUXC_SW_MUX_CTL_PAD(SW_MUX_CTL_PAD_SAI1_RXD0) = 5;    /* ALT5_GPIO4_IO[2] */
    REG_IOMUXC_SW_PAD_CTL_PAD(SW_PAD_CTL_PAD_SAI1_RXD0) = 0x19;
    REG_GPIO4.GDIR |= 1 << 2;  /* output mode */
    REG_GPIO4.DR |= 1 << 2;
    wait_cnt(10000);
    REG_GPIO4.DR |= 0 << 2;
    wait_cnt(10000);
    REG_GPIO4.DR |= 1 << 2;
    _kernel_synch_cache();
#endif
}

/*
 * Initialize hardware
 */
void hw_init(void)
{
    clock_init();
    uart_init();
    timer_init();
    eqos_init();
    enet_init();
}
