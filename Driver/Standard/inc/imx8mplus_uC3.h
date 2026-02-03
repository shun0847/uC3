/**
 * @brief   Micro C Cube Standard, DEVICE DRIVER
 *          i.MX8M Plus depedent definitions
 * @date    2021.03.29
 *
 * @copyright (c) 2020-2021, eForce Co., Ltd. All rights reserved.
 */
#if !defined(IMX8MPLUS_H_)
#define IMX8MPLUS_H_

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(CORTEX_M7__)
#include "Cortex-M7.h"
#endif

/*
 * Memory map
 */
#if !defined(CORTEX_M7__)
// Cortex-A53
#define SRAM0_BASE              (0x0000000000900000ULL)   /* 512KB OCRAM */
#define SRAM1_BASE              (0x0000000000980000ULL)   /* 64KB OCRAM with ECC */

#define AIPS1_BASE              (0x0000000030000000ULL)
#define AIPS2_BASE              (0x0000000030400000ULL)
#define AIPS3_BASE              (0x0000000030800000ULL)
#define AIPS4_BASE              (0x0000000032C00000ULL)
#define AIPS5_BASE              (0x0000000030C00000ULL)
#define GIC_BASE                (0x0000000038800000ULL)
#define DDR_BASE                (0x0000000040000000ULL)

#else
// Cortex-M7
#define QSPI_RXBUF_BASE         (0x34000000U)
#define SRAM0_BASE              (0x20200000U)   /* 512KB OCRAM */
#define SRAM1_BASE              (0x20280000U)   /* 64KB OCRAM with ECC */

#define AIPS1_BASE              (0x30000000U)
#define AIPS2_BASE              (0x30400000U)
#define AIPS3_BASE              (0x30800000U)
#define AIPS4_BASE              (0x32C00000U)
#define AIPS5_BASE              (0x30C00000U)

#define APBH_DMA_BASE           (0x33000000U)
#define DDR_BASE                (0x40000000U)
#define QSPI0_BASE              (0xC0000000U)
#endif

/*
 * AIPS1 memory map
 */
#define GPIO1_BASE              (AIPS1_BASE + 0x00200000U)
#define GPIO2_BASE              (AIPS1_BASE + 0x00210000U)
#define GPIO3_BASE              (AIPS1_BASE + 0x00220000U)
#define GPIO4_BASE              (AIPS1_BASE + 0x00230000U)
#define GPIO5_BASE              (AIPS1_BASE + 0x00240000U)
#define IOMUXC_BASE             (AIPS1_BASE + 0x00330000U)
#define IOMUXC_GPR_BASE         (AIPS1_BASE + 0x00340000U)
#define CCM_BASE                (AIPS1_BASE + 0x00380000U)
#define SRC_BASE                (AIPS1_BASE + 0x00390000U)
#define GPC_BASE                (AIPS1_BASE + 0x003A0000U)

/*
 * AIPS2 memory map
 */
#define SYSCNT_CTRL_REG_BASE    (AIPS2_BASE + 0x002C0000U)
#define SYSCNT_CMP_REG_BASE     (AIPS2_BASE + 0x002B0000U)
#define SYSCNT_RD_REG_BASE      (AIPS2_BASE + 0x002A0000U)

/*
 * AIPS3 memory map
 */
#define UART1_BASE              (AIPS3_BASE + 0x0060000U)
#define UART2_BASE              (AIPS3_BASE + 0x0090000U)
#define UART3_BASE              (AIPS3_BASE + 0x0080000U)
#define UART4_BASE              (AIPS3_BASE + 0x0260000U)
#define MU_A_BASE               (AIPS3_BASE + 0x02A0000U)
#define MU_B_BASE               (AIPS3_BASE + 0x02B0000U)
#define SEMAPHORE_HS_BASE       (AIPS3_BASE + 0x02C0000U)
#define ENET1_BASE              (AIPS3_BASE + 0x03E0000U)
#define ENET2_TSN_BASE          (AIPS3_BASE + 0x03F0000U)   /* ENET_QOS */

/*
 * AIPS4 memory map
 */

/*
 * AIPS5 memory map
 */

/* General-Purpose I/O Controller registers                                 */
struct t_gpio {
    UW      DR;                 /* GPIO Data register                       */
    UW      GDIR;               /* GPIO Data Direction register             */
    UW      PSR;                /* GPIO Pad Status register                 */
    UW      ICR1;               /* GPIO Interrupt Configuration register-1  */
    UW      ICR2;               /* GPIO Interrupt Configuration register-2  */
    UW      IMR;                /* GPIO Interrupt mask register             */
    UW      ISR;                /* GPIO Interrupt Status register           */
    UW      EDGE_SEL;           /* GPIO edge select register                */
};

#define REG_GPIO1       (*(volatile struct t_gpio *)GPIO1_BASE)
#define REG_GPIO2       (*(volatile struct t_gpio *)GPIO2_BASE)
#define REG_GPIO3       (*(volatile struct t_gpio *)GPIO3_BASE)
#define REG_GPIO4       (*(volatile struct t_gpio *)GPIO4_BASE)

/** CCM - Register Layout Typedef */
struct t_ccm {
    UW GPR0;                              /**< General Purpose Register, offset: 0x0 */
    UW GPR0_SET;                          /**< General Purpose Register, offset: 0x4 */
    UW GPR0_CLR;                          /**< General Purpose Register, offset: 0x8 */
    UW GPR0_TOG;                          /**< General Purpose Register, offset: 0xC */
    UB RESERVED_0[2032];
    struct {                                         /* offset: 0x800, array step: 0x10 */
        UW PLL_CTRL;                          /**< CCM PLL Control Register, array offset: 0x800, array step: 0x10 */
        UW PLL_CTRL_SET;                      /**< CCM PLL Control Register, array offset: 0x804, array step: 0x10 */
        UW PLL_CTRL_CLR;                      /**< CCM PLL Control Register, array offset: 0x808, array step: 0x10 */
        UW PLL_CTRL_TOG;                      /**< CCM PLL Control Register, array offset: 0x80C, array step: 0x10 */
    } PLL_CTRL[39];
    UB RESERVED_1[13712];
    struct {                                         /* offset: 0x4000, array step: 0x10 */
        UW CCGR;                              /**< CCM Clock Gating Register, array offset: 0x4000, array step: 0x10 */
        UW CCGR_SET;                          /**< CCM Clock Gating Register, array offset: 0x4004, array step: 0x10 */
        UW CCGR_CLR;                          /**< CCM Clock Gating Register, array offset: 0x4008, array step: 0x10 */
        UW CCGR_TOG;                          /**< CCM Clock Gating Register, array offset: 0x400C, array step: 0x10 */
    } CCGR[191];
    UB RESERVED_2[13328];
    struct {                                         /* offset: 0x8000, array step: 0x80 */
        UW TARGET_ROOT;                       /**< Target Register, array offset: 0x8000, array step: 0x80 */
        UW TARGET_ROOT_SET;                   /**< Target Register, array offset: 0x8004, array step: 0x80 */
        UW TARGET_ROOT_CLR;                   /**< Target Register, array offset: 0x8008, array step: 0x80 */
        UW TARGET_ROOT_TOG;                   /**< Target Register, array offset: 0x800C, array step: 0x80 */
        UW MISC;                              /**< Miscellaneous Register, array offset: 0x8010, array step: 0x80 */
        UW MISC_ROOT_SET;                     /**< Miscellaneous Register, array offset: 0x8014, array step: 0x80 */
        UW MISC_ROOT_CLR;                     /**< Miscellaneous Register, array offset: 0x8018, array step: 0x80 */
        UW MISC_ROOT_TOG;                     /**< Miscellaneous Register, array offset: 0x801C, array step: 0x80 */
        UW POST;                              /**< Post Divider Register, array offset: 0x8020, array step: 0x80 */
        UW POST_ROOT_SET;                     /**< Post Divider Register, array offset: 0x8024, array step: 0x80 */
        UW POST_ROOT_CLR;                     /**< Post Divider Register, array offset: 0x8028, array step: 0x80 */
        UW POST_ROOT_TOG;                     /**< Post Divider Register, array offset: 0x802C, array step: 0x80 */
        UW PRE;                               /**< Pre Divider Register, array offset: 0x8030, array step: 0x80 */
        UW PRE_ROOT_SET;                      /**< Pre Divider Register, array offset: 0x8034, array step: 0x80 */
        UW PRE_ROOT_CLR;                      /**< Pre Divider Register, array offset: 0x8038, array step: 0x80 */
        UW PRE_ROOT_TOG;                      /**< Pre Divider Register, array offset: 0x803C, array step: 0x80 */
        UB RESERVED_0[48];
        UW ACCESS_CTRL;                       /**< Access Control Register, array offset: 0x8070, array step: 0x80 */
        UW ACCESS_CTRL_ROOT_SET;              /**< Access Control Register, array offset: 0x8074, array step: 0x80 */
        UW ACCESS_CTRL_ROOT_CLR;              /**< Access Control Register, array offset: 0x8078, array step: 0x80 */
        UW ACCESS_CTRL_ROOT_TOG;              /**< Access Control Register, array offset: 0x807C, array step: 0x80 */
    } ROOT[142];
};

#define REG_CCM        (*(volatile struct t_ccm *)CCM_BASE)

/**
 * CCM_TARGET_ROOT mapping
 */
enum t_ccm_target_root_index {
    ARM_A53_CLK_ROOT        = 0U,
    ARM_M7_CLK_ROOT         = 1U,
    MAIN_AXI_CLK_ROOT       = 16U,
    AHB_CLK_ROOT            = 32U,
    IPG_CLK_ROOT            = 33U,
    DRAM_ALT_CLK_ROOT       = 64U,
    UART1_CLK_ROOT          = 94U,
    UART2_CLK_ROOT          = 95U,
    UART3_CLK_ROOT          = 96U,
    UART4_CLK_ROOT          = 97U,
    /* ENET_QOS */
    ENET_AXI_CLK_ROOT       = 17U,
    ENET_QOS_CLK_ROOT       = 81U,  /* linux 129? */
    ENET_QOS_TIMER_CLK_ROOT = 82U,
    ENET_REF_CLK_ROOT       = 83U,
    /* ENET? */
    ENET_TIMER_CLK_ROOT     = 84U,
    ENET_PHY_REF_CLK_ROOT   = 85U,
};

/**
 * CCM_CCGR mapping
 */
enum t_ccm_ccgr_index {
    CCGR_DEBUG          = 4U,
    CCGR_DRAM           = 5U,
    CCGR_ENET1          = 10U,
    CCGR_MU             = 33U,
    CCGR_OCRAM          = 35U,
    CCGR_OCRAM_S        = 36U,
    CCGR_QOS_ENET       = 46U,
    CCGR_ENET_QOS       = 59U,
    CCGR_SEC_DEBUG      = 60U,
    CCGR_SEMA1          = 61U,
    CCGR_SEMA2          = 62U,
    CCGR_SIM_ENET       = 64U,
    CCGR_SIM_M          = 65U,
    CCGR_SIM_MAIN       = 66U,
    CCGR_SIM_S          = 67U,
    CCGR_SIM_WKUP       = 68U,
    CCGR_UART1          = 73U,
    CCGR_UART2          = 74U,
    CCGR_UART3          = 75U,
    CCGR_UART4          = 76U,
};

#define REG_CCM_ROOT(index)     (REG_CCM.ROOT[(index)])
#define REG_CCM_CCGR(index)     (REG_CCM.CCGR[(index)])

/**
 * IOMUX Controller (IOMUXC)
 */
struct t_iomuxc_gpr {
    UW GPR[25];
};

struct t_iomuxc {
    UW reserved_0[5];
    UW SW_MUX_CTL_PAD[143];
    UW SW_PAD_CTL_PAD[156];
    UW SELECT_INPUT[94];
};

#define REG_IOMUXC_GPR      (*(volatile struct t_iomuxc_gpr *)IOMUXC_GPR_BASE)
#define REG_IOMUXC          (*(volatile struct t_iomuxc *)IOMUXC_BASE)

/**
 * IOMUXC_SW_MUX_CTL_PAD index mapping
 */
enum t_iomuxc_sw_mux_ctl_pad {
    SW_MUX_CTL_PAD_GPIO1_IO00 = 0,                      /*    0 - 0x0014: IOMUXC_SW_MUX_CTL_PAD_GPIO1_IO00 */
    SW_MUX_CTL_PAD_GPIO1_IO01,                          /*    1 - 0x0018: IOMUXC_SW_MUX_CTL_PAD_GPIO1_IO01 */
    SW_MUX_CTL_PAD_GPIO1_IO02,                          /*    2 - 0x001C: IOMUXC_SW_MUX_CTL_PAD_GPIO1_IO02 */
    SW_MUX_CTL_PAD_GPIO1_IO03,                          /*    3 - 0x0020: IOMUXC_SW_MUX_CTL_PAD_GPIO1_IO03 */
    SW_MUX_CTL_PAD_GPIO1_IO04,                          /*    4 - 0x0024: IOMUXC_SW_MUX_CTL_PAD_GPIO1_IO04 */
    SW_MUX_CTL_PAD_GPIO1_IO05,                          /*    5 - 0x0028: IOMUXC_SW_MUX_CTL_PAD_GPIO1_IO05 */
    SW_MUX_CTL_PAD_GPIO1_IO06,                          /*    6 - 0x002C: IOMUXC_SW_MUX_CTL_PAD_GPIO1_IO06 */
    SW_MUX_CTL_PAD_GPIO1_IO07,                          /*    7 - 0x0030: IOMUXC_SW_MUX_CTL_PAD_GPIO1_IO07 */
    SW_MUX_CTL_PAD_GPIO1_IO08,                          /*    8 - 0x0034: IOMUXC_SW_MUX_CTL_PAD_GPIO1_IO08 */
    SW_MUX_CTL_PAD_GPIO1_IO09,                          /*    9 - 0x0038: IOMUXC_SW_MUX_CTL_PAD_GPIO1_IO09 */
    SW_MUX_CTL_PAD_GPIO1_IO10,                          /*   10 - 0x003C: IOMUXC_SW_MUX_CTL_PAD_GPIO1_IO10 */
    SW_MUX_CTL_PAD_GPIO1_IO11,                          /*   11 - 0x0040: IOMUXC_SW_MUX_CTL_PAD_GPIO1_IO11 */
    SW_MUX_CTL_PAD_GPIO1_IO12,                          /*   12 - 0x0044: IOMUXC_SW_MUX_CTL_PAD_GPIO1_IO12 */
    SW_MUX_CTL_PAD_GPIO1_IO13,                          /*   13 - 0x0048: IOMUXC_SW_MUX_CTL_PAD_GPIO1_IO13 */
    SW_MUX_CTL_PAD_GPIO1_IO14,                          /*   14 - 0x004C: IOMUXC_SW_MUX_CTL_PAD_GPIO1_IO14 */
    SW_MUX_CTL_PAD_GPIO1_IO15,                          /*   15 - 0x0050: IOMUXC_SW_MUX_CTL_PAD_GPIO1_IO15 */
    SW_MUX_CTL_PAD_ENET_MDC,                            /*   16 - 0x0054: IOMUXC_SW_MUX_CTL_PAD_ENET_MDC */
    SW_MUX_CTL_PAD_ENET_MDIO,                           /*   17 - 0x0058: IOMUXC_SW_MUX_CTL_PAD_ENET_MDIO */
    SW_MUX_CTL_PAD_ENET_TD3,                            /*   18 - 0x005C: IOMUXC_SW_MUX_CTL_PAD_ENET_TD3 */
    SW_MUX_CTL_PAD_ENET_TD2,                            /*   19 - 0x0060: IOMUXC_SW_MUX_CTL_PAD_ENET_TD2 */
    SW_MUX_CTL_PAD_ENET_TD1,                            /*   20 - 0x0064: IOMUXC_SW_MUX_CTL_PAD_ENET_TD1 */
    SW_MUX_CTL_PAD_ENET_TD0,                            /*   21 - 0x0068: IOMUXC_SW_MUX_CTL_PAD_ENET_TD0 */
    SW_MUX_CTL_PAD_ENET_TX_CTL,                         /*   22 - 0x006C: IOMUXC_SW_MUX_CTL_PAD_ENET_TX_CTL */
    SW_MUX_CTL_PAD_ENET_TXC,                            /*   23 - 0x0070: IOMUXC_SW_MUX_CTL_PAD_ENET_TXC */
    SW_MUX_CTL_PAD_ENET_RX_CTL,                         /*   24 - 0x0074: IOMUXC_SW_MUX_CTL_PAD_ENET_RX_CTL */
    SW_MUX_CTL_PAD_ENET_RXC,                            /*   25 - 0x0078: IOMUXC_SW_MUX_CTL_PAD_ENET_RXC */
    SW_MUX_CTL_PAD_ENET_RD0,                            /*   26 - 0x007C: IOMUXC_SW_MUX_CTL_PAD_ENET_RD0 */
    SW_MUX_CTL_PAD_ENET_RD1,                            /*   27 - 0x0080: IOMUXC_SW_MUX_CTL_PAD_ENET_RD1 */
    SW_MUX_CTL_PAD_ENET_RD2,                            /*   28 - 0x0084: IOMUXC_SW_MUX_CTL_PAD_ENET_RD2 */
    SW_MUX_CTL_PAD_ENET_RD3,                            /*   29 - 0x0088: IOMUXC_SW_MUX_CTL_PAD_ENET_RD3 */
    SW_MUX_CTL_PAD_SD1_CLK,                             /*   30 - 0x008C: IOMUXC_SW_MUX_CTL_PAD_SD1_CLK */
    SW_MUX_CTL_PAD_SD1_CMD,                             /*   31 - 0x0090: IOMUXC_SW_MUX_CTL_PAD_SD1_CMD */
    SW_MUX_CTL_PAD_SD1_DATA0,                           /*   32 - 0x0094: IOMUXC_SW_MUX_CTL_PAD_SD1_DATA0 */
    SW_MUX_CTL_PAD_SD1_DATA1,                           /*   33 - 0x0098: IOMUXC_SW_MUX_CTL_PAD_SD1_DATA1 */
    SW_MUX_CTL_PAD_SD1_DATA2,                           /*   34 - 0x009C: IOMUXC_SW_MUX_CTL_PAD_SD1_DATA2 */
    SW_MUX_CTL_PAD_SD1_DATA3,                           /*   35 - 0x00A0: IOMUXC_SW_MUX_CTL_PAD_SD1_DATA3 */
    SW_MUX_CTL_PAD_SD1_DATA4,                           /*   36 - 0x00A4: IOMUXC_SW_MUX_CTL_PAD_SD1_DATA4 */
    SW_MUX_CTL_PAD_SD1_DATA5,                           /*   37 - 0x00A8: IOMUXC_SW_MUX_CTL_PAD_SD1_DATA5 */
    SW_MUX_CTL_PAD_SD1_DATA6,                           /*   38 - 0x00AC: IOMUXC_SW_MUX_CTL_PAD_SD1_DATA6 */
    SW_MUX_CTL_PAD_SD1_DATA7,                           /*   39 - 0x00B0: IOMUXC_SW_MUX_CTL_PAD_SD1_DATA7 */
    SW_MUX_CTL_PAD_SD1_RESET_B,                         /*   40 - 0x00B4: IOMUXC_SW_MUX_CTL_PAD_SD1_RESET_B */
    SW_MUX_CTL_PAD_SD1_STROBE,                          /*   41 - 0x00B8: IOMUXC_SW_MUX_CTL_PAD_SD1_STROBE */
    SW_MUX_CTL_PAD_SD2_CD_B,                            /*   42 - 0x00BC: IOMUXC_SW_MUX_CTL_PAD_SD2_CD_B */
    SW_MUX_CTL_PAD_SD2_CLK,                             /*   43 - 0x00C0: IOMUXC_SW_MUX_CTL_PAD_SD2_CLK */
    SW_MUX_CTL_PAD_SD2_CMD,                             /*   44 - 0x00C4: IOMUXC_SW_MUX_CTL_PAD_SD2_CMD */
    SW_MUX_CTL_PAD_SD2_DATA0,                           /*   45 - 0x00C8: IOMUXC_SW_MUX_CTL_PAD_SD2_DATA0 */
    SW_MUX_CTL_PAD_SD2_DATA1,                           /*   46 - 0x00CC: IOMUXC_SW_MUX_CTL_PAD_SD2_DATA1 */
    SW_MUX_CTL_PAD_SD2_DATA2,                           /*   47 - 0x00D0: IOMUXC_SW_MUX_CTL_PAD_SD2_DATA2 */
    SW_MUX_CTL_PAD_SD2_DATA3,                           /*   48 - 0x00D4: IOMUXC_SW_MUX_CTL_PAD_SD2_DATA3 */
    SW_MUX_CTL_PAD_SD2_RESET_B,                         /*   49 - 0x00D8: IOMUXC_SW_MUX_CTL_PAD_SD2_RESET_B */
    SW_MUX_CTL_PAD_SD2_WP,                              /*   50 - 0x00DC: IOMUXC_SW_MUX_CTL_PAD_SD2_WP */
    SW_MUX_CTL_PAD_NAND_ALE,                            /*   51 - 0x00E0: IOMUXC_SW_MUX_CTL_PAD_NAND_ALE */
    SW_MUX_CTL_PAD_NAND_CE0_B,                          /*   52 - 0x00E4: IOMUXC_SW_MUX_CTL_PAD_NAND_CE0_B */
    SW_MUX_CTL_PAD_NAND_CE1_B,                          /*   53 - 0x00E8: IOMUXC_SW_MUX_CTL_PAD_NAND_CE1_B */
    SW_MUX_CTL_PAD_NAND_CE2_B,                          /*   54 - 0x00EC: IOMUXC_SW_MUX_CTL_PAD_NAND_CE2_B */
    SW_MUX_CTL_PAD_NAND_CE3_B,                          /*   55 - 0x00F0: IOMUXC_SW_MUX_CTL_PAD_NAND_CE3_B */
    SW_MUX_CTL_PAD_NAND_CLE,                            /*   56 - 0x00F4: IOMUXC_SW_MUX_CTL_PAD_NAND_CLE */
    SW_MUX_CTL_PAD_NAND_DATA00,                         /*   57 - 0x00F8: IOMUXC_SW_MUX_CTL_PAD_NAND_DATA00 */
    SW_MUX_CTL_PAD_NAND_DATA01,                         /*   58 - 0x00FC: IOMUXC_SW_MUX_CTL_PAD_NAND_DATA01 */
    SW_MUX_CTL_PAD_NAND_DATA02,                         /*   59 - 0x0100: IOMUXC_SW_MUX_CTL_PAD_NAND_DATA02 */
    SW_MUX_CTL_PAD_NAND_DATA03,                         /*   60 - 0x0104: IOMUXC_SW_MUX_CTL_PAD_NAND_DATA03 */
    SW_MUX_CTL_PAD_NAND_DATA04,                         /*   61 - 0x0108: IOMUXC_SW_MUX_CTL_PAD_NAND_DATA04 */
    SW_MUX_CTL_PAD_NAND_DATA05,                         /*   62 - 0x010C: IOMUXC_SW_MUX_CTL_PAD_NAND_DATA05 */
    SW_MUX_CTL_PAD_NAND_DATA06,                         /*   63 - 0x0110: IOMUXC_SW_MUX_CTL_PAD_NAND_DATA06 */
    SW_MUX_CTL_PAD_NAND_DATA07,                         /*   64 - 0x0114: IOMUXC_SW_MUX_CTL_PAD_NAND_DATA07 */
    SW_MUX_CTL_PAD_NAND_DQS,                            /*   65 - 0x0118: IOMUXC_SW_MUX_CTL_PAD_NAND_DQS */
    SW_MUX_CTL_PAD_NAND_RE_B,                           /*   66 - 0x011C: IOMUXC_SW_MUX_CTL_PAD_NAND_RE_B */
    SW_MUX_CTL_PAD_NAND_READY_B,                        /*   67 - 0x0120: IOMUXC_SW_MUX_CTL_PAD_NAND_READY_B */
    SW_MUX_CTL_PAD_NAND_WE_B,                           /*   68 - 0x0124: IOMUXC_SW_MUX_CTL_PAD_NAND_WE_B */
    SW_MUX_CTL_PAD_NAND_WP_B,                           /*   69 - 0x0128: IOMUXC_SW_MUX_CTL_PAD_NAND_WP_B */
    SW_MUX_CTL_PAD_SAI5_RXFS,                           /*   70 - 0x012C: IOMUXC_SW_MUX_CTL_PAD_SAI5_RXFS */
    SW_MUX_CTL_PAD_SAI5_RXC,                            /*   71 - 0x0130: IOMUXC_SW_MUX_CTL_PAD_SAI5_RXC */
    SW_MUX_CTL_PAD_SAI5_RXD0,                           /*   72 - 0x0134: IOMUXC_SW_MUX_CTL_PAD_SAI5_RXD0 */
    SW_MUX_CTL_PAD_SAI5_RXD1,                           /*   73 - 0x0138: IOMUXC_SW_MUX_CTL_PAD_SAI5_RXD1 */
    SW_MUX_CTL_PAD_SAI5_RXD2,                           /*   74 - 0x013C: IOMUXC_SW_MUX_CTL_PAD_SAI5_RXD2 */
    SW_MUX_CTL_PAD_SAI5_RXD3,                           /*   75 - 0x0140: IOMUXC_SW_MUX_CTL_PAD_SAI5_RXD3 */
    SW_MUX_CTL_PAD_SAI5_MCLK,                           /*   76 - 0x0144: IOMUXC_SW_MUX_CTL_PAD_SAI5_MCLK */
    SW_MUX_CTL_PAD_SAI1_RXFS,                           /*   77 - 0x0148: IOMUXC_SW_MUX_CTL_PAD_SAI1_RXFS */
    SW_MUX_CTL_PAD_SAI1_RXC,                            /*   78 - 0x014C: IOMUXC_SW_MUX_CTL_PAD_SAI1_RXC */
    SW_MUX_CTL_PAD_SAI1_RXD0,                           /*   79 - 0x0150: IOMUXC_SW_MUX_CTL_PAD_SAI1_RXD0 */
    SW_MUX_CTL_PAD_SAI1_RXD1,                           /*   80 - 0x0154: IOMUXC_SW_MUX_CTL_PAD_SAI1_RXD1 */
    SW_MUX_CTL_PAD_SAI1_RXD2,                           /*   81 - 0x0158: IOMUXC_SW_MUX_CTL_PAD_SAI1_RXD2 */
    SW_MUX_CTL_PAD_SAI1_RXD3,                           /*   82 - 0x015C: IOMUXC_SW_MUX_CTL_PAD_SAI1_RXD3 */
    SW_MUX_CTL_PAD_SAI1_RXD4,                           /*   83 - 0x0160: IOMUXC_SW_MUX_CTL_PAD_SAI1_RXD4 */
    SW_MUX_CTL_PAD_SAI1_RXD5,                           /*   84 - 0x0164: IOMUXC_SW_MUX_CTL_PAD_SAI1_RXD5 */
    SW_MUX_CTL_PAD_SAI1_RXD6,                           /*   85 - 0x0168: IOMUXC_SW_MUX_CTL_PAD_SAI1_RXD6 */
    SW_MUX_CTL_PAD_SAI1_RXD7,                           /*   86 - 0x016C: IOMUXC_SW_MUX_CTL_PAD_SAI1_RXD7 */
    SW_MUX_CTL_PAD_SAI1_TXFS,                           /*   87 - 0x0170: IOMUXC_SW_MUX_CTL_PAD_SAI1_TXFS */
    SW_MUX_CTL_PAD_SAI1_TXC,                            /*   88 - 0x0174: IOMUXC_SW_MUX_CTL_PAD_SAI1_TXC */
    SW_MUX_CTL_PAD_SAI1_TXD0,                           /*   89 - 0x0178: IOMUXC_SW_MUX_CTL_PAD_SAI1_TXD0 */
    SW_MUX_CTL_PAD_SAI1_TXD1,                           /*   90 - 0x017C: IOMUXC_SW_MUX_CTL_PAD_SAI1_TXD1 */
    SW_MUX_CTL_PAD_SAI1_TXD2,                           /*   91 - 0x0180: IOMUXC_SW_MUX_CTL_PAD_SAI1_TXD2 */
    SW_MUX_CTL_PAD_SAI1_TXD3,                           /*   92 - 0x0184: IOMUXC_SW_MUX_CTL_PAD_SAI1_TXD3 */
    SW_MUX_CTL_PAD_SAI1_TXD4,                           /*   93 - 0x0188: IOMUXC_SW_MUX_CTL_PAD_SAI1_TXD4 */
    SW_MUX_CTL_PAD_SAI1_TXD5,                           /*   94 - 0x018C: IOMUXC_SW_MUX_CTL_PAD_SAI1_TXD5 */
    SW_MUX_CTL_PAD_SAI1_TXD6,                           /*   95 - 0x0190: IOMUXC_SW_MUX_CTL_PAD_SAI1_TXD6 */
    SW_MUX_CTL_PAD_SAI1_TXD7,                           /*   96 - 0x0194: IOMUXC_SW_MUX_CTL_PAD_SAI1_TXD7 */
    SW_MUX_CTL_PAD_SAI1_MCLK,                           /*   97 - 0x0198: IOMUXC_SW_MUX_CTL_PAD_SAI1_MCLK */
    SW_MUX_CTL_PAD_SAI2_RXFS,                           /*   98 - 0x019C: IOMUXC_SW_MUX_CTL_PAD_SAI2_RXFS */
    SW_MUX_CTL_PAD_SAI2_RXC,                            /*   99 - 0x01A0: IOMUXC_SW_MUX_CTL_PAD_SAI2_RXC */
    SW_MUX_CTL_PAD_SAI2_RXD0,                           /*  100 - 0x01A4: IOMUXC_SW_MUX_CTL_PAD_SAI2_RXD0 */
    SW_MUX_CTL_PAD_SAI2_TXFS,                           /*  101 - 0x01A8: IOMUXC_SW_MUX_CTL_PAD_SAI2_TXFS */
    SW_MUX_CTL_PAD_SAI2_TXC,                            /*  102 - 0x01AC: IOMUXC_SW_MUX_CTL_PAD_SAI2_TXC */
    SW_MUX_CTL_PAD_SAI2_TXD0,                           /*  103 - 0x01B0: IOMUXC_SW_MUX_CTL_PAD_SAI2_TXD0 */
    SW_MUX_CTL_PAD_SAI2_MCLK,                           /*  104 - 0x01B4: IOMUXC_SW_MUX_CTL_PAD_SAI2_MCLK */
    SW_MUX_CTL_PAD_SAI3_RXFS,                           /*  105 - 0x01B8: IOMUXC_SW_MUX_CTL_PAD_SAI3_RXFS */
    SW_MUX_CTL_PAD_SAI3_RXC,                            /*  106 - 0x01BC: IOMUXC_SW_MUX_CTL_PAD_SAI3_RXC */
    SW_MUX_CTL_PAD_SAI3_RXD,                            /*  107 - 0x01C0: IOMUXC_SW_MUX_CTL_PAD_SAI3_RXD */
    SW_MUX_CTL_PAD_SAI3_TXFS,                           /*  108 - 0x01C4: IOMUXC_SW_MUX_CTL_PAD_SAI3_TXFS */
    SW_MUX_CTL_PAD_SAI3_TXC,                            /*  109 - 0x01C8: IOMUXC_SW_MUX_CTL_PAD_SAI3_TXC */
    SW_MUX_CTL_PAD_SAI3_TXD,                            /*  110 - 0x01CC: IOMUXC_SW_MUX_CTL_PAD_SAI3_TXD */
    SW_MUX_CTL_PAD_SAI3_MCLK,                           /*  111 - 0x01D0: IOMUXC_SW_MUX_CTL_PAD_SAI3_MCLK */
    SW_MUX_CTL_PAD_SPDIF_TX,                            /*  112 - 0x01D4: IOMUXC_SW_MUX_CTL_PAD_SPDIF_TX */
    SW_MUX_CTL_PAD_SPDIF_RX,                            /*  113 - 0x01D8: IOMUXC_SW_MUX_CTL_PAD_SPDIF_RX */
    SW_MUX_CTL_PAD_SPDIF_EXT_CLK,                       /*  114 - 0x01DC: IOMUXC_SW_MUX_CTL_PAD_SPDIF_EXT_CLK */
    SW_MUX_CTL_PAD_ECSPI1_SCLK,                         /*  115 - 0x01E0: IOMUXC_SW_MUX_CTL_PAD_ECSPI1_SCLK */
    SW_MUX_CTL_PAD_ECSPI1_MOSI,                         /*  116 - 0x01E4: IOMUXC_SW_MUX_CTL_PAD_ECSPI1_MOSI */
    SW_MUX_CTL_PAD_ECSPI1_MISO,                         /*  117 - 0x01E8: IOMUXC_SW_MUX_CTL_PAD_ECSPI1_MISO */
    SW_MUX_CTL_PAD_ECSPI1_SS0,                          /*  118 - 0x01EC: IOMUXC_SW_MUX_CTL_PAD_ECSPI1_SS0 */
    SW_MUX_CTL_PAD_ECSPI2_SCLK,                         /*  119 - 0x01F0: IOMUXC_SW_MUX_CTL_PAD_ECSPI2_SCLK */
    SW_MUX_CTL_PAD_ECSPI2_MOSI,                         /*  120 - 0x01F4: IOMUXC_SW_MUX_CTL_PAD_ECSPI2_MOSI */
    SW_MUX_CTL_PAD_ECSPI2_MISO,                         /*  121 - 0x01F8: IOMUXC_SW_MUX_CTL_PAD_ECSPI2_MISO */
    SW_MUX_CTL_PAD_ECSPI2_SS0,                          /*  122 - 0x01FC: IOMUXC_SW_MUX_CTL_PAD_ECSPI2_SS0 */
    SW_MUX_CTL_PAD_I2C1_SCL,                            /*  123 - 0x0200: IOMUXC_SW_MUX_CTL_PAD_I2C1_SCL */
    SW_MUX_CTL_PAD_I2C1_SDA,                            /*  124 - 0x0204: IOMUXC_SW_MUX_CTL_PAD_I2C1_SDA */
    SW_MUX_CTL_PAD_I2C2_SCL,                            /*  125 - 0x0208: IOMUXC_SW_MUX_CTL_PAD_I2C2_SCL */
    SW_MUX_CTL_PAD_I2C2_SDA,                            /*  126 - 0x020C: IOMUXC_SW_MUX_CTL_PAD_I2C2_SDA */
    SW_MUX_CTL_PAD_I2C3_SCL,                            /*  127 - 0x0210: IOMUXC_SW_MUX_CTL_PAD_I2C3_SCL */
    SW_MUX_CTL_PAD_I2C3_SDA,                            /*  128 - 0x0214: IOMUXC_SW_MUX_CTL_PAD_I2C3_SDA */
    SW_MUX_CTL_PAD_I2C4_SCL,                            /*  129 - 0x0218: IOMUXC_SW_MUX_CTL_PAD_I2C4_SCL */
    SW_MUX_CTL_PAD_I2C4_SDA,                            /*  130 - 0x021C: IOMUXC_SW_MUX_CTL_PAD_I2C4_SDA */
    SW_MUX_CTL_PAD_UART1_RXD,                           /*  131 - 0x0220: IOMUXC_SW_MUX_CTL_PAD_UART1_RXD */
    SW_MUX_CTL_PAD_UART1_TXD,                           /*  132 - 0x0224: IOMUXC_SW_MUX_CTL_PAD_UART1_TXD */
    SW_MUX_CTL_PAD_UART2_RXD,                           /*  133 - 0x0228: IOMUXC_SW_MUX_CTL_PAD_UART2_RXD */
    SW_MUX_CTL_PAD_UART2_TXD,                           /*  134 - 0x022C: IOMUXC_SW_MUX_CTL_PAD_UART2_TXD */
    SW_MUX_CTL_PAD_UART3_RXD,                           /*  135 - 0x0230: IOMUXC_SW_MUX_CTL_PAD_UART3_RXD */
    SW_MUX_CTL_PAD_UART3_TXD,                           /*  136 - 0x0234: IOMUXC_SW_MUX_CTL_PAD_UART3_TXD */
    SW_MUX_CTL_PAD_UART4_RXD,                           /*  137 - 0x0238: IOMUXC_SW_MUX_CTL_PAD_UART4_RXD */
    SW_MUX_CTL_PAD_UART4_TXD,                           /*  138 - 0x023C: IOMUXC_SW_MUX_CTL_PAD_UART4_TXD */
    SW_MUX_CTL_PAD_HDMI_DDC_SCL,                        /*  139 - 0x0240: IOMUXC_SW_MUX_CTL_PAD_HDMI_DDC_SCL */
    SW_MUX_CTL_PAD_HDMI_DDC_SDA,                        /*  140 - 0x0244: IOMUXC_SW_MUX_CTL_PAD_HDMI_DDC_SDA */
    SW_MUX_CTL_PAD_HDMI_CEC,                            /*  141 - 0x0248: IOMUXC_SW_MUX_CTL_PAD_HDMI_CEC */
    SW_MUX_CTL_PAD_HDMI_HPD,                            /*  142 - 0x024C: IOMUXC_SW_MUX_CTL_PAD_HDMI_HPD */
};

/**
 * IOMUXC_SW_PAD_CTL_PAD index mapping
 */
enum t_iomuxc_sw_pad_ctl_pad {
    SW_PAD_CTL_PAD_BOOT_MODE0 = 0,                      /*    0 - 0x0250: IOMUXC_SW_PAD_CTL_PAD_BOOT_MODE0 */
    SW_PAD_CTL_PAD_BOOT_MODE1,                          /*    1 - 0x0254: IOMUXC_SW_PAD_CTL_PAD_BOOT_MODE1 */
    SW_PAD_CTL_PAD_BOOT_MODE2,                          /*    2 - 0x0258: IOMUXC_SW_PAD_CTL_PAD_BOOT_MODE2 */
    SW_PAD_CTL_PAD_BOOT_MODE3,                          /*    3 - 0x025C: IOMUXC_SW_PAD_CTL_PAD_BOOT_MODE3 */
    SW_PAD_CTL_PAD_JTAG_MOD,                            /*    4 - 0x0260: IOMUXC_SW_PAD_CTL_PAD_JTAG_MOD */
    SW_PAD_CTL_PAD_JTAG_TDI,                            /*    5 - 0x0264: IOMUXC_SW_PAD_CTL_PAD_JTAG_TDI */
    SW_PAD_CTL_PAD_JTAG_TMS,                            /*    6 - 0x0268: IOMUXC_SW_PAD_CTL_PAD_JTAG_TMS */
    SW_PAD_CTL_PAD_JTAG_TCK,                            /*    7 - 0x026C: IOMUXC_SW_PAD_CTL_PAD_JTAG_TCK */
    SW_PAD_CTL_PAD_JTAG_TDO,                            /*    8 - 0x0270: IOMUXC_SW_PAD_CTL_PAD_JTAG_TDO */
    SW_PAD_CTL_PAD_GPIO1_IO00,                          /*    9 - 0x0274: IOMUXC_SW_PAD_CTL_PAD_GPIO1_IO00 */
    SW_PAD_CTL_PAD_GPIO1_IO01,                          /*   10 - 0x0278: IOMUXC_SW_PAD_CTL_PAD_GPIO1_IO01 */
    SW_PAD_CTL_PAD_GPIO1_IO02,                          /*   11 - 0x027C: IOMUXC_SW_PAD_CTL_PAD_GPIO1_IO02 */
    SW_PAD_CTL_PAD_GPIO1_IO03,                          /*   12 - 0x0280: IOMUXC_SW_PAD_CTL_PAD_GPIO1_IO03 */
    SW_PAD_CTL_PAD_GPIO1_IO04,                          /*   13 - 0x0284: IOMUXC_SW_PAD_CTL_PAD_GPIO1_IO04 */
    SW_PAD_CTL_PAD_GPIO1_IO05,                          /*   14 - 0x0288: IOMUXC_SW_PAD_CTL_PAD_GPIO1_IO05 */
    SW_PAD_CTL_PAD_GPIO1_IO06,                          /*   15 - 0x028C: IOMUXC_SW_PAD_CTL_PAD_GPIO1_IO06 */
    SW_PAD_CTL_PAD_GPIO1_IO07,                          /*   16 - 0x0290: IOMUXC_SW_PAD_CTL_PAD_GPIO1_IO07 */
    SW_PAD_CTL_PAD_GPIO1_IO08,                          /*   17 - 0x0294: IOMUXC_SW_PAD_CTL_PAD_GPIO1_IO08 */
    SW_PAD_CTL_PAD_GPIO1_IO09,                          /*   18 - 0x0298: IOMUXC_SW_PAD_CTL_PAD_GPIO1_IO09 */
    SW_PAD_CTL_PAD_GPIO1_IO10,                          /*   19 - 0x029C: IOMUXC_SW_PAD_CTL_PAD_GPIO1_IO10 */
    SW_PAD_CTL_PAD_GPIO1_IO11,                          /*   20 - 0x02A0: IOMUXC_SW_PAD_CTL_PAD_GPIO1_IO11 */
    SW_PAD_CTL_PAD_GPIO1_IO12,                          /*   21 - 0x02A4: IOMUXC_SW_PAD_CTL_PAD_GPIO1_IO12 */
    SW_PAD_CTL_PAD_GPIO1_IO13,                          /*   22 - 0x02A8: IOMUXC_SW_PAD_CTL_PAD_GPIO1_IO13 */
    SW_PAD_CTL_PAD_GPIO1_IO14,                          /*   23 - 0x02AC: IOMUXC_SW_PAD_CTL_PAD_GPIO1_IO14 */
    SW_PAD_CTL_PAD_GPIO1_IO15,                          /*   24 - 0x02B0: IOMUXC_SW_PAD_CTL_PAD_GPIO1_IO15 */
    SW_PAD_CTL_PAD_ENET_MDC,                            /*   25 - 0x02B4: IOMUXC_SW_PAD_CTL_PAD_ENET_MDC */
    SW_PAD_CTL_PAD_ENET_MDIO,                           /*   26 - 0x02B8: IOMUXC_SW_PAD_CTL_PAD_ENET_MDIO */
    SW_PAD_CTL_PAD_ENET_TD3,                            /*   27 - 0x02BC: IOMUXC_SW_PAD_CTL_PAD_ENET_TD3 */
    SW_PAD_CTL_PAD_ENET_TD2,                            /*   28 - 0x02C0: IOMUXC_SW_PAD_CTL_PAD_ENET_TD2 */
    SW_PAD_CTL_PAD_ENET_TD1,                            /*   29 - 0x02C4: IOMUXC_SW_PAD_CTL_PAD_ENET_TD1 */
    SW_PAD_CTL_PAD_ENET_TD0,                            /*   30 - 0x02C8: IOMUXC_SW_PAD_CTL_PAD_ENET_TD0 */
    SW_PAD_CTL_PAD_ENET_TX_CTL,                         /*   31 - 0x02CC: IOMUXC_SW_PAD_CTL_PAD_ENET_TX_CTL */
    SW_PAD_CTL_PAD_ENET_TXC,                            /*   32 - 0x02D0: IOMUXC_SW_PAD_CTL_PAD_ENET_TXC */
    SW_PAD_CTL_PAD_ENET_RX_CTL,                         /*   33 - 0x02D4: IOMUXC_SW_PAD_CTL_PAD_ENET_RX_CTL */
    SW_PAD_CTL_PAD_ENET_RXC,                            /*   34 - 0x02D8: IOMUXC_SW_PAD_CTL_PAD_ENET_RXC */
    SW_PAD_CTL_PAD_ENET_RD0,                            /*   35 - 0x02DC: IOMUXC_SW_PAD_CTL_PAD_ENET_RD0 */
    SW_PAD_CTL_PAD_ENET_RD1,                            /*   36 - 0x02E0: IOMUXC_SW_PAD_CTL_PAD_ENET_RD1 */
    SW_PAD_CTL_PAD_ENET_RD2,                            /*   37 - 0x02E4: IOMUXC_SW_PAD_CTL_PAD_ENET_RD2 */
    SW_PAD_CTL_PAD_ENET_RD3,                            /*   38 - 0x02E8: IOMUXC_SW_PAD_CTL_PAD_ENET_RD3 */
    SW_PAD_CTL_PAD_SD1_CLK,                             /*   39 - 0x02EC: IOMUXC_SW_PAD_CTL_PAD_SD1_CLK */
    SW_PAD_CTL_PAD_SD1_CMD,                             /*   40 - 0x02F0: IOMUXC_SW_PAD_CTL_PAD_SD1_CMD */
    SW_PAD_CTL_PAD_SD1_DATA0,                           /*   41 - 0x02F4: IOMUXC_SW_PAD_CTL_PAD_SD1_DATA0 */
    SW_PAD_CTL_PAD_SD1_DATA1,                           /*   42 - 0x02F8: IOMUXC_SW_PAD_CTL_PAD_SD1_DATA1 */
    SW_PAD_CTL_PAD_SD1_DATA2,                           /*   43 - 0x02FC: IOMUXC_SW_PAD_CTL_PAD_SD1_DATA2 */
    SW_PAD_CTL_PAD_SD1_DATA3,                           /*   44 - 0x0300: IOMUXC_SW_PAD_CTL_PAD_SD1_DATA3 */
    SW_PAD_CTL_PAD_SD1_DATA4,                           /*   45 - 0x0304: IOMUXC_SW_PAD_CTL_PAD_SD1_DATA4 */
    SW_PAD_CTL_PAD_SD1_DATA5,                           /*   46 - 0x0308: IOMUXC_SW_PAD_CTL_PAD_SD1_DATA5 */
    SW_PAD_CTL_PAD_SD1_DATA6,                           /*   47 - 0x030C: IOMUXC_SW_PAD_CTL_PAD_SD1_DATA6 */
    SW_PAD_CTL_PAD_SD1_DATA7,                           /*   48 - 0x0310: IOMUXC_SW_PAD_CTL_PAD_SD1_DATA7 */
    SW_PAD_CTL_PAD_SD1_RESET_B,                         /*   49 - 0x0314: IOMUXC_SW_PAD_CTL_PAD_SD1_RESET_B */
    SW_PAD_CTL_PAD_SD1_STROBE,                          /*   50 - 0x0318: IOMUXC_SW_PAD_CTL_PAD_SD1_STROBE */
    SW_PAD_CTL_PAD_SD2_CD_B,                            /*   51 - 0x031C: IOMUXC_SW_PAD_CTL_PAD_SD2_CD_B */
    SW_PAD_CTL_PAD_SD2_CLK,                             /*   52 - 0x0320: IOMUXC_SW_PAD_CTL_PAD_SD2_CLK */
    SW_PAD_CTL_PAD_SD2_CMD,                             /*   53 - 0x0324: IOMUXC_SW_PAD_CTL_PAD_SD2_CMD */
    SW_PAD_CTL_PAD_SD2_DATA0,                           /*   54 - 0x0328: IOMUXC_SW_PAD_CTL_PAD_SD2_DATA0 */
    SW_PAD_CTL_PAD_SD2_DATA1,                           /*   55 - 0x032C: IOMUXC_SW_PAD_CTL_PAD_SD2_DATA1 */
    SW_PAD_CTL_PAD_SD2_DATA2,                           /*   56 - 0x0330: IOMUXC_SW_PAD_CTL_PAD_SD2_DATA2 */
    SW_PAD_CTL_PAD_SD2_DATA3,                           /*   57 - 0x0334: IOMUXC_SW_PAD_CTL_PAD_SD2_DATA3 */
    SW_PAD_CTL_PAD_SD2_RESET_B,                         /*   58 - 0x0338: IOMUXC_SW_PAD_CTL_PAD_SD2_RESET_B */
    SW_PAD_CTL_PAD_SD2_WP,                              /*   59 - 0x033C: IOMUXC_SW_PAD_CTL_PAD_SD2_WP */
    SW_PAD_CTL_PAD_NAND_ALE,                            /*   60 - 0x0340: IOMUXC_SW_PAD_CTL_PAD_NAND_ALE */
    SW_PAD_CTL_PAD_NAND_CE0_B,                          /*   61 - 0x0344: IOMUXC_SW_PAD_CTL_PAD_NAND_CE0_B */
    SW_PAD_CTL_PAD_NAND_CE1_B,                          /*   62 - 0x0348: IOMUXC_SW_PAD_CTL_PAD_NAND_CE1_B */
    SW_PAD_CTL_PAD_NAND_CE2_B,                          /*   63 - 0x034C: IOMUXC_SW_PAD_CTL_PAD_NAND_CE2_B */
    SW_PAD_CTL_PAD_NAND_CE3_B,                          /*   64 - 0x0350: IOMUXC_SW_PAD_CTL_PAD_NAND_CE3_B */
    SW_PAD_CTL_PAD_NAND_CLE,                            /*   65 - 0x0354: IOMUXC_SW_PAD_CTL_PAD_NAND_CLE */
    SW_PAD_CTL_PAD_NAND_DATA00,                         /*   66 - 0x0358: IOMUXC_SW_PAD_CTL_PAD_NAND_DATA00 */
    SW_PAD_CTL_PAD_NAND_DATA01,                         /*   67 - 0x035C: IOMUXC_SW_PAD_CTL_PAD_NAND_DATA01 */
    SW_PAD_CTL_PAD_NAND_DATA02,                         /*   68 - 0x0360: IOMUXC_SW_PAD_CTL_PAD_NAND_DATA02 */
    SW_PAD_CTL_PAD_NAND_DATA03,                         /*   69 - 0x0364: IOMUXC_SW_PAD_CTL_PAD_NAND_DATA03 */
    SW_PAD_CTL_PAD_NAND_DATA04,                         /*   70 - 0x0368: IOMUXC_SW_PAD_CTL_PAD_NAND_DATA04 */
    SW_PAD_CTL_PAD_NAND_DATA05,                         /*   71 - 0x036C: IOMUXC_SW_PAD_CTL_PAD_NAND_DATA05 */
    SW_PAD_CTL_PAD_NAND_DATA06,                         /*   72 - 0x0370: IOMUXC_SW_PAD_CTL_PAD_NAND_DATA06 */
    SW_PAD_CTL_PAD_NAND_DATA07,                         /*   73 - 0x0374: IOMUXC_SW_PAD_CTL_PAD_NAND_DATA07 */
    SW_PAD_CTL_PAD_NAND_DQS,                            /*   74 - 0x0378: IOMUXC_SW_PAD_CTL_PAD_NAND_DQS */
    SW_PAD_CTL_PAD_NAND_RE_B,                           /*   75 - 0x037C: IOMUXC_SW_PAD_CTL_PAD_NAND_RE_B */
    SW_PAD_CTL_PAD_NAND_READY_B,                        /*   76 - 0x0380: IOMUXC_SW_PAD_CTL_PAD_NAND_READY_B */
    SW_PAD_CTL_PAD_NAND_WE_B,                           /*   77 - 0x0384: IOMUXC_SW_PAD_CTL_PAD_NAND_WE_B */
    SW_PAD_CTL_PAD_NAND_WP_B,                           /*   78 - 0x0388: IOMUXC_SW_PAD_CTL_PAD_NAND_WP_B */
    SW_PAD_CTL_PAD_SAI5_RXFS,                           /*   79 - 0x038C: IOMUXC_SW_PAD_CTL_PAD_SAI5_RXFS */
    SW_PAD_CTL_PAD_SAI5_RXC,                            /*   80 - 0x0390: IOMUXC_SW_PAD_CTL_PAD_SAI5_RXC */
    SW_PAD_CTL_PAD_SAI5_RXD0,                           /*   81 - 0x0394: IOMUXC_SW_PAD_CTL_PAD_SAI5_RXD0 */
    SW_PAD_CTL_PAD_SAI5_RXD1,                           /*   82 - 0x0398: IOMUXC_SW_PAD_CTL_PAD_SAI5_RXD1 */
    SW_PAD_CTL_PAD_SAI5_RXD2,                           /*   83 - 0x039C: IOMUXC_SW_PAD_CTL_PAD_SAI5_RXD2 */
    SW_PAD_CTL_PAD_SAI5_RXD3,                           /*   84 - 0x03A0: IOMUXC_SW_PAD_CTL_PAD_SAI5_RXD3 */
    SW_PAD_CTL_PAD_SAI5_MCLK,                           /*   85 - 0x03A4: IOMUXC_SW_PAD_CTL_PAD_SAI5_MCLK */
    SW_PAD_CTL_PAD_SAI1_RXFS,                           /*   86 - 0x03A8: IOMUXC_SW_PAD_CTL_PAD_SAI1_RXFS */
    SW_PAD_CTL_PAD_SAI1_RXC,                            /*   87 - 0x03AC: IOMUXC_SW_PAD_CTL_PAD_SAI1_RXC */
    SW_PAD_CTL_PAD_SAI1_RXD0,                           /*   88 - 0x03B0: IOMUXC_SW_PAD_CTL_PAD_SAI1_RXD0 */
    SW_PAD_CTL_PAD_SAI1_RXD1,                           /*   89 - 0x03B4: IOMUXC_SW_PAD_CTL_PAD_SAI1_RXD1 */
    SW_PAD_CTL_PAD_SAI1_RXD2,                           /*   90 - 0x03B8: IOMUXC_SW_PAD_CTL_PAD_SAI1_RXD2 */
    SW_PAD_CTL_PAD_SAI1_RXD3,                           /*   91 - 0x03BC: IOMUXC_SW_PAD_CTL_PAD_SAI1_RXD3 */
    SW_PAD_CTL_PAD_SAI1_RXD4,                           /*   92 - 0x03C0: IOMUXC_SW_PAD_CTL_PAD_SAI1_RXD4 */
    SW_PAD_CTL_PAD_SAI1_RXD5,                           /*   93 - 0x03C4: IOMUXC_SW_PAD_CTL_PAD_SAI1_RXD5 */
    SW_PAD_CTL_PAD_SAI1_RXD6,                           /*   94 - 0x03C8: IOMUXC_SW_PAD_CTL_PAD_SAI1_RXD6 */
    SW_PAD_CTL_PAD_SAI1_RXD7,                           /*   95 - 0x03CC: IOMUXC_SW_PAD_CTL_PAD_SAI1_RXD7 */
    SW_PAD_CTL_PAD_SAI1_TXFS,                           /*   96 - 0x03D0: IOMUXC_SW_PAD_CTL_PAD_SAI1_TXFS */
    SW_PAD_CTL_PAD_SAI1_TXC,                            /*   97 - 0x03D4: IOMUXC_SW_PAD_CTL_PAD_SAI1_TXC */
    SW_PAD_CTL_PAD_SAI1_TXD0,                           /*   98 - 0x03D8: IOMUXC_SW_PAD_CTL_PAD_SAI1_TXD0 */
    SW_PAD_CTL_PAD_SAI1_TXD1,                           /*   99 - 0x03DC: IOMUXC_SW_PAD_CTL_PAD_SAI1_TXD1 */
    SW_PAD_CTL_PAD_SAI1_TXD2,                           /*  100 - 0x03E0: IOMUXC_SW_PAD_CTL_PAD_SAI1_TXD2 */
    SW_PAD_CTL_PAD_SAI1_TXD3,                           /*  101 - 0x03E4: IOMUXC_SW_PAD_CTL_PAD_SAI1_TXD3 */
    SW_PAD_CTL_PAD_SAI1_TXD4,                           /*  102 - 0x03E8: IOMUXC_SW_PAD_CTL_PAD_SAI1_TXD4 */
    SW_PAD_CTL_PAD_SAI1_TXD5,                           /*  103 - 0x03EC: IOMUXC_SW_PAD_CTL_PAD_SAI1_TXD5 */
    SW_PAD_CTL_PAD_SAI1_TXD6,                           /*  104 - 0x03F0: IOMUXC_SW_PAD_CTL_PAD_SAI1_TXD6 */
    SW_PAD_CTL_PAD_SAI1_TXD7,                           /*  105 - 0x03F4: IOMUXC_SW_PAD_CTL_PAD_SAI1_TXD7 */
    SW_PAD_CTL_PAD_SAI1_MCLK,                           /*  106 - 0x03F8: IOMUXC_SW_PAD_CTL_PAD_SAI1_MCLK */
    SW_PAD_CTL_PAD_SAI2_RXFS,                           /*  107 - 0x03FC: IOMUXC_SW_PAD_CTL_PAD_SAI2_RXFS */
    SW_PAD_CTL_PAD_SAI2_RXC,                            /*  108 - 0x0400: IOMUXC_SW_PAD_CTL_PAD_SAI2_RXC */
    SW_PAD_CTL_PAD_SAI2_RXD0,                           /*  109 - 0x0404: IOMUXC_SW_PAD_CTL_PAD_SAI2_RXD0 */
    SW_PAD_CTL_PAD_SAI2_TXFS,                           /*  110 - 0x0408: IOMUXC_SW_PAD_CTL_PAD_SAI2_TXFS */
    SW_PAD_CTL_PAD_SAI2_TXC,                            /*  111 - 0x040C: IOMUXC_SW_PAD_CTL_PAD_SAI2_TXC */
    SW_PAD_CTL_PAD_SAI2_TXD0,                           /*  112 - 0x0410: IOMUXC_SW_PAD_CTL_PAD_SAI2_TXD0 */
    SW_PAD_CTL_PAD_SAI2_MCLK,                           /*  113 - 0x0414: IOMUXC_SW_PAD_CTL_PAD_SAI2_MCLK */
    SW_PAD_CTL_PAD_SAI3_RXFS,                           /*  114 - 0x0418: IOMUXC_SW_PAD_CTL_PAD_SAI3_RXFS */
    SW_PAD_CTL_PAD_SAI3_RXC,                            /*  115 - 0x041C: IOMUXC_SW_PAD_CTL_PAD_SAI3_RXC */
    SW_PAD_CTL_PAD_SAI3_RXD,                            /*  116 - 0x0420: IOMUXC_SW_PAD_CTL_PAD_SAI3_RXD */
    SW_PAD_CTL_PAD_SAI3_TXFS,                           /*  117 - 0x0424: IOMUXC_SW_PAD_CTL_PAD_SAI3_TXFS */
    SW_PAD_CTL_PAD_SAI3_TXC,                            /*  118 - 0x0428: IOMUXC_SW_PAD_CTL_PAD_SAI3_TXC */
    SW_PAD_CTL_PAD_SAI3_TXD,                            /*  119 - 0x042C: IOMUXC_SW_PAD_CTL_PAD_SAI3_TXD */
    SW_PAD_CTL_PAD_SAI3_MCLK,                           /*  120 - 0x0430: IOMUXC_SW_PAD_CTL_PAD_SAI3_MCLK */
    SW_PAD_CTL_PAD_SPDIF_TX,                            /*  121 - 0x0434: IOMUXC_SW_PAD_CTL_PAD_SPDIF_TX */
    SW_PAD_CTL_PAD_SPDIF_RX,                            /*  122 - 0x0438: IOMUXC_SW_PAD_CTL_PAD_SPDIF_RX */
    SW_PAD_CTL_PAD_SPDIF_EXT_CLK,                       /*  123 - 0x043C: IOMUXC_SW_PAD_CTL_PAD_SPDIF_EXT_CLK */
    SW_PAD_CTL_PAD_ECSPI1_SCLK,                         /*  124 - 0x0440: IOMUXC_SW_PAD_CTL_PAD_ECSPI1_SCLK */
    SW_PAD_CTL_PAD_ECSPI1_MOSI,                         /*  125 - 0x0444: IOMUXC_SW_PAD_CTL_PAD_ECSPI1_MOSI */
    SW_PAD_CTL_PAD_ECSPI1_MISO,                         /*  126 - 0x0448: IOMUXC_SW_PAD_CTL_PAD_ECSPI1_MISO */
    SW_PAD_CTL_PAD_ECSPI1_SS0,                          /*  127 - 0x044C: IOMUXC_SW_PAD_CTL_PAD_ECSPI1_SS0 */
    SW_PAD_CTL_PAD_ECSPI2_SCLK,                         /*  128 - 0x0450: IOMUXC_SW_PAD_CTL_PAD_ECSPI2_SCLK */
    SW_PAD_CTL_PAD_ECSPI2_MOSI,                         /*  129 - 0x0454: IOMUXC_SW_PAD_CTL_PAD_ECSPI2_MOSI */
    SW_PAD_CTL_PAD_ECSPI2_MISO,                         /*  130 - 0x0458: IOMUXC_SW_PAD_CTL_PAD_ECSPI2_MISO */
    SW_PAD_CTL_PAD_ECSPI2_SS0,                          /*  131 - 0x045C: IOMUXC_SW_PAD_CTL_PAD_ECSPI2_SS0 */
    SW_PAD_CTL_PAD_I2C1_SCL,                            /*  132 - 0x0460: IOMUXC_SW_PAD_CTL_PAD_I2C1_SCL */
    SW_PAD_CTL_PAD_I2C1_SDA,                            /*  133 - 0x0464: IOMUXC_SW_PAD_CTL_PAD_I2C1_SDA */
    SW_PAD_CTL_PAD_I2C2_SCL,                            /*  134 - 0x0468: IOMUXC_SW_PAD_CTL_PAD_I2C2_SCL */
    SW_PAD_CTL_PAD_I2C2_SDA,                            /*  135 - 0x046C: IOMUXC_SW_PAD_CTL_PAD_I2C2_SDA */
    SW_PAD_CTL_PAD_I2C3_SCL,                            /*  136 - 0x0470: IOMUXC_SW_PAD_CTL_PAD_I2C3_SCL */
    SW_PAD_CTL_PAD_I2C3_SDA,                            /*  137 - 0x0474: IOMUXC_SW_PAD_CTL_PAD_I2C3_SDA */
    SW_PAD_CTL_PAD_I2C4_SCL,                            /*  138 - 0x0478: IOMUXC_SW_PAD_CTL_PAD_I2C4_SCL */
    SW_PAD_CTL_PAD_I2C4_SDA,                            /*  139 - 0x047C: IOMUXC_SW_PAD_CTL_PAD_I2C4_SDA */
    SW_PAD_CTL_PAD_UART1_RXD,                           /*  140 - 0x0480: IOMUXC_SW_PAD_CTL_PAD_UART1_RXD */
    SW_PAD_CTL_PAD_UART1_TXD,                           /*  141 - 0x0484: IOMUXC_SW_PAD_CTL_PAD_UART1_TXD */
    SW_PAD_CTL_PAD_UART2_RXD,                           /*  142 - 0x0488: IOMUXC_SW_PAD_CTL_PAD_UART2_RXD */
    SW_PAD_CTL_PAD_UART2_TXD,                           /*  143 - 0x048C: IOMUXC_SW_PAD_CTL_PAD_UART2_TXD */
    SW_PAD_CTL_PAD_UART3_RXD,                           /*  144 - 0x0490: IOMUXC_SW_PAD_CTL_PAD_UART3_RXD */
    SW_PAD_CTL_PAD_UART3_TXD,                           /*  145 - 0x0494: IOMUXC_SW_PAD_CTL_PAD_UART3_TXD */
    SW_PAD_CTL_PAD_UART4_RXD,                           /*  146 - 0x0498: IOMUXC_SW_PAD_CTL_PAD_UART4_RXD */
    SW_PAD_CTL_PAD_UART4_TXD,                           /*  147 - 0x049C: IOMUXC_SW_PAD_CTL_PAD_UART4_TXD */
    SW_PAD_CTL_PAD_HDMI_DDC_SCL,                        /*  148 - 0x04A0: IOMUXC_SW_PAD_CTL_PAD_HDMI_DDC_SCL */
    SW_PAD_CTL_PAD_HDMI_DDC_SDA,                        /*  149 - 0x04A4: IOMUXC_SW_PAD_CTL_PAD_HDMI_DDC_SDA */
    SW_PAD_CTL_PAD_HDMI_CEC,                            /*  150 - 0x04A8: IOMUXC_SW_PAD_CTL_PAD_HDMI_CEC */
    SW_PAD_CTL_PAD_HDMI_HPD,                            /*  151 - 0x04AC: IOMUXC_SW_PAD_CTL_PAD_HDMI_HPD */
    SW_PAD_CTL_PAD_CLKIN1,                              /*  152 - 0x04B0: IOMUXC_SW_PAD_CTL_PAD_CLKIN1 */
    SW_PAD_CTL_PAD_CLKIN2,                              /*  153 - 0x04B4: IOMUXC_SW_PAD_CTL_PAD_CLKIN2 */
    SW_PAD_CTL_PAD_CLKOUT1,                             /*  154 - 0x04B8: IOMUXC_SW_PAD_CTL_PAD_CLKOUT1 */
    SW_PAD_CTL_PAD_CLKOUT2,                             /*  155 - 0x04BC: IOMUXC_SW_PAD_CTL_PAD_CLKOUT2 */
};

/**
 * IOMUXC_SELECT_INPUT index mapping
 */
enum t_iomuxc_select_input {
    SELECT_INPUT_AUDIOMIX_PDM_MIC_PDM_BITSTREAM_0 = 0,  /*    0 - 0x04C0: IOMUXC_AUDIOMIX_PDM_MIC_PDM_BITSTREAM_SELECT_INPUT_0 */
    SELECT_INPUT_AUDIOMIX_PDM_MIC_PDM_BITSTREAM_1,      /*    1 - 0x04C4: IOMUXC_AUDIOMIX_PDM_MIC_PDM_BITSTREAM_SELECT_INPUT_1 */
    SELECT_INPUT_AUDIOMIX_PDM_MIC_PDM_BITSTREAM_2,      /*    2 - 0x04C8: IOMUXC_AUDIOMIX_PDM_MIC_PDM_BITSTREAM_SELECT_INPUT_2 */
    SELECT_INPUT_AUDIOMIX_PDM_MIC_PDM_BITSTREAM_3,      /*    3 - 0x04CC: IOMUXC_AUDIOMIX_PDM_MIC_PDM_BITSTREAM_SELECT_INPUT_3 */
    SELECT_INPUT_AUDIOMIX_SAI1_RXSYNC,                  /*    4 - 0x04D0: IOMUXC_AUDIOMIX_SAI1_RXSYNC_SELECT_INPUT */
    SELECT_INPUT_AUDIOMIX_SAI1_TXBCLK,                  /*    5 - 0x04D4: IOMUXC_AUDIOMIX_SAI1_TXBCLK_SELECT_INPUT */
    SELECT_INPUT_AUDIOMIX_SAI1_TXSYNC,                  /*    6 - 0x04D8: IOMUXC_AUDIOMIX_SAI1_TXSYNC_SELECT_INPUT */
    SELECT_INPUT_AUDIOMIX_SAI2_RXDATA_1,                /*    7 - 0x04DC: IOMUXC_AUDIOMIX_SAI2_RXDATA_SELECT_INPUT_1 */
    SELECT_INPUT_AUDIOMIX_SAI3_MCLK,                    /*    8 - 0x04E0: IOMUXC_AUDIOMIX_SAI3_MCLK_SELECT_INPUT */
    SELECT_INPUT_AUDIOMIX_SAI3_RXDATA_0,                /*    9 - 0x04E4: IOMUXC_AUDIOMIX_SAI3_RXDATA_SELECT_INPUT_0 */
    SELECT_INPUT_AUDIOMIX_SAI3_TXBCLK,                  /*   10 - 0x04E8: IOMUXC_AUDIOMIX_SAI3_TXBCLK_SELECT_INPUT */
    SELECT_INPUT_AUDIOMIX_SAI3_TXSYNC,                  /*   11 - 0x04EC: IOMUXC_AUDIOMIX_SAI3_TXSYNC_SELECT_INPUT */
    SELECT_INPUT_AUDIOMIX_SAI5_MCLK,                    /*   12 - 0x04F0: IOMUXC_AUDIOMIX_SAI5_MCLK_SELECT_INPUT */
    SELECT_INPUT_AUDIOMIX_SAI5_RXBCLK,                  /*   13 - 0x04F4: IOMUXC_AUDIOMIX_SAI5_RXBCLK_SELECT_INPUT */
    SELECT_INPUT_AUDIOMIX_SAI5_RXDATA_0,                /*   14 - 0x04F8: IOMUXC_AUDIOMIX_SAI5_RXDATA_SELECT_INPUT_0 */
    SELECT_INPUT_AUDIOMIX_SAI5_RXDATA_1,                /*   15 - 0x04FC: IOMUXC_AUDIOMIX_SAI5_RXDATA_SELECT_INPUT_1 */
    SELECT_INPUT_AUDIOMIX_SAI5_RXDATA_2,                /*   16 - 0x0500: IOMUXC_AUDIOMIX_SAI5_RXDATA_SELECT_INPUT_2 */
    SELECT_INPUT_AUDIOMIX_SAI5_RXDATA_3,                /*   17 - 0x0504: IOMUXC_AUDIOMIX_SAI5_RXDATA_SELECT_INPUT_3 */
    SELECT_INPUT_AUDIOMIX_SAI5_RXSYNC,                  /*   18 - 0x0508: IOMUXC_AUDIOMIX_SAI5_RXSYNC_SELECT_INPUT */
    SELECT_INPUT_AUDIOMIX_SAI5_TXBCLK,                  /*   19 - 0x050C: IOMUXC_AUDIOMIX_SAI5_TXBCLK_SELECT_INPUT */
    SELECT_INPUT_AUDIOMIX_SAI5_TXSYNC,                  /*   20 - 0x0510: IOMUXC_AUDIOMIX_SAI5_TXSYNC_SELECT_INPUT */
    SELECT_INPUT_AUDIOMIX_SAI6_MCLK,                    /*   21 - 0x0514: IOMUXC_AUDIOMIX_SAI6_MCLK_SELECT_INPUT */
    SELECT_INPUT_AUDIOMIX_SAI6_RXBCLK,                  /*   22 - 0x0518: IOMUXC_AUDIOMIX_SAI6_RXBCLK_SELECT_INPUT */
    SELECT_INPUT_AUDIOMIX_SAI6_RXDATA_0,                /*   23 - 0x051C: IOMUXC_AUDIOMIX_SAI6_RXDATA_SELECT_INPUT_0 */
    SELECT_INPUT_AUDIOMIX_SAI6_RXSYNC,                  /*   24 - 0x0520: IOMUXC_AUDIOMIX_SAI6_RXSYNC_SELECT_INPUT */
    SELECT_INPUT_AUDIOMIX_SAI6_TXBCLK,                  /*   25 - 0x0524: IOMUXC_AUDIOMIX_SAI6_TXBCLK_SELECT_INPUT */
    SELECT_INPUT_AUDIOMIX_SAI6_TXSYNC,                  /*   26 - 0x0528: IOMUXC_AUDIOMIX_SAI6_TXSYNC_SELECT_INPUT */
    SELECT_INPUT_AUDIOMIX_SAI7_MCLK,                    /*   27 - 0x052C: IOMUXC_AUDIOMIX_SAI7_MCLK_SELECT_INPUT */
    SELECT_INPUT_AUDIOMIX_SAI7_RXBCLK,                  /*   28 - 0x0530: IOMUXC_AUDIOMIX_SAI7_RXBCLK_SELECT_INPUT */
    SELECT_INPUT_AUDIOMIX_SAI7_RXDATA_0,                /*   29 - 0x0534: IOMUXC_AUDIOMIX_SAI7_RXDATA_SELECT_INPUT_0 */
    SELECT_INPUT_AUDIOMIX_SAI7_RXSYNC,                  /*   30 - 0x0538: IOMUXC_AUDIOMIX_SAI7_RXSYNC_SELECT_INPUT */
    SELECT_INPUT_AUDIOMIX_SAI7_TXBCLK,                  /*   31 - 0x053C: IOMUXC_AUDIOMIX_SAI7_TXBCLK_SELECT_INPUT */
    SELECT_INPUT_AUDIOMIX_SAI7_TXSYNC,                  /*   32 - 0x0540: IOMUXC_AUDIOMIX_SAI7_TXSYNC_SELECT_INPUT */
    SELECT_INPUT_AUDIOMIX_EARC_PHY_SPDIF_IN,            /*   33 - 0x0544: IOMUXC_AUDIOMIX_EARC_PHY_SPDIF_IN_SELECT_INPUT */
    SELECT_INPUT_AUDIOMIX_SPDIF_EXTCLK,                 /*   34 - 0x0548: IOMUXC_AUDIOMIX_SPDIF_EXTCLK_SELECT_INPUT */
    SELECT_INPUT_CAN1_CANRX,                            /*   35 - 0x054C: IOMUXC_CAN1_CANRX_SELECT_INPUT */
    SELECT_INPUT_CAN2_CANRX,                            /*   36 - 0x0550: IOMUXC_CAN2_CANRX_SELECT_INPUT */
    SELECT_INPUT_CCM_GPC_PMIC_VFUNCTIONAL_READY,        /*   37 - 0x0554: IOMUXC_CCM_GPC_PMIC_VFUNCTIONAL_READY_SELECT_INPUT */
    SELECT_INPUT_ECSPI1_CSPI_CLK_IN,                    /*   38 - 0x0558: IOMUXC_ECSPI1_CSPI_CLK_IN_SELECT_INPUT */
    SELECT_INPUT_ECSPI1_MISO,                           /*   39 - 0x055C: IOMUXC_ECSPI1_MISO_SELECT_INPUT */
    SELECT_INPUT_ECSPI1_MOSI,                           /*   40 - 0x0560: IOMUXC_ECSPI1_MOSI_SELECT_INPUT */
    SELECT_INPUT_ECSPI1_SS_B_0,                         /*   41 - 0x0564: IOMUXC_ECSPI1_SS_B_SELECT_INPUT_0 */
    SELECT_INPUT_ECSPI2_CSPI_CLK_IN,                    /*   42 - 0x0568: IOMUXC_ECSPI2_CSPI_CLK_IN_SELECT_INPUT */
    SELECT_INPUT_ECSPI2_MISO,                           /*   43 - 0x056C: IOMUXC_ECSPI2_MISO_SELECT_INPUT */
    SELECT_INPUT_ECSPI2_MOSI,                           /*   44 - 0x0570: IOMUXC_ECSPI2_MOSI_SELECT_INPUT */
    SELECT_INPUT_ECSPI2_SS_B_0,                         /*   45 - 0x0574: IOMUXC_ECSPI2_SS_B_SELECT_INPUT_0 */
    SELECT_INPUT_ENET1_IPG_CLK_RMII,                    /*   46 - 0x0578: IOMUXC_ENET1_IPG_CLK_RMII_SELECT_INPUT */
    SELECT_INPUT_ENET1_MDIO,                            /*   47 - 0x057C: IOMUXC_ENET1_MDIO_SELECT_INPUT */
    SELECT_INPUT_ENET1_RXDATA_0,                        /*   48 - 0x0580: IOMUXC_ENET1_RXDATA_0_SELECT_INPUT */
    SELECT_INPUT_ENET1_RXDATA_1,                        /*   49 - 0x0584: IOMUXC_ENET1_RXDATA_1_SELECT_INPUT */
    SELECT_INPUT_ENET1_RXEN,                            /*   50 - 0x0588: IOMUXC_ENET1_RXEN_SELECT_INPUT */
    SELECT_INPUT_ENET1_RXERR,                           /*   51 - 0x058C: IOMUXC_ENET1_RXERR_SELECT_INPUT */
    SELECT_INPUT_ENET_QOS_GMII_MDI_I,                   /*   52 - 0x0590: IOMUXC_ENET_QOS_GMII_MDI_I_SELECT_INPUT */
    SELECT_INPUT_GPT1_CAPIN1,                           /*   53 - 0x0594: IOMUXC_GPT1_CAPIN1_SELECT_INPUT */
    SELECT_INPUT_GPT1_CAPIN2,                           /*   54 - 0x0598: IOMUXC_GPT1_CAPIN2_SELECT_INPUT */
    SELECT_INPUT_GPT1_CLKIN,                            /*   55 - 0x059C: IOMUXC_GPT1_CLKIN_SELECT_INPUT */
    SELECT_INPUT_PCIE_CLKREQ_B,                         /*   56 - 0x05A0: IOMUXC_PCIE_CLKREQ_B_SELECT_INPUT */
    SELECT_INPUT_I2C1_SCL_IN,                           /*   57 - 0x05A4: IOMUXC_I2C1_SCL_IN_SELECT_INPUT */
    SELECT_INPUT_I2C1_SDA_IN,                           /*   58 - 0x05A8: IOMUXC_I2C1_SDA_IN_SELECT_INPUT */
    SELECT_INPUT_I2C2_SCL_IN,                           /*   59 - 0x05AC: IOMUXC_I2C2_SCL_IN_SELECT_INPUT */
    SELECT_INPUT_I2C2_SDA_IN,                           /*   60 - 0x05B0: IOMUXC_I2C2_SDA_IN_SELECT_INPUT */
    SELECT_INPUT_I2C3_SCL_IN,                           /*   61 - 0x05B4: IOMUXC_I2C3_SCL_IN_SELECT_INPUT */
    SELECT_INPUT_I2C3_SDA_IN,                           /*   62 - 0x05B8: IOMUXC_I2C3_SDA_IN_SELECT_INPUT */
    SELECT_INPUT_I2C4_SCL_IN,                           /*   63 - 0x05BC: IOMUXC_I2C4_SCL_IN_SELECT_INPUT */
    SELECT_INPUT_I2C4_SDA_IN,                           /*   64 - 0x05C0: IOMUXC_I2C4_SDA_IN_SELECT_INPUT */
    SELECT_INPUT_I2C5_SCL_IN,                           /*   65 - 0x05C4: IOMUXC_I2C5_SCL_IN_SELECT_INPUT */
    SELECT_INPUT_I2C5_SDA_IN,                           /*   66 - 0x05C8: IOMUXC_I2C5_SDA_IN_SELECT_INPUT */
    SELECT_INPUT_I2C6_SCL_IN,                           /*   67 - 0x05CC: IOMUXC_I2C6_SCL_IN_SELECT_INPUT */
    SELECT_INPUT_I2C6_SDA_IN,                           /*   68 - 0x05D0: IOMUXC_I2C6_SDA_IN_SELECT_INPUT */
    SELECT_INPUT_ISP_FL_TRIG_0,                         /*   69 - 0x05D4: IOMUXC_ISP_FL_TRIG_0_SELECT_INPUT */
    SELECT_INPUT_ISP_FL_TRIG_1,                         /*   70 - 0x05D8: IOMUXC_ISP_FL_TRIG_1_SELECT_INPUT */
    SELECT_INPUT_ISP_SHUTTER_TRIG_0,                    /*   71 - 0x05DC: IOMUXC_ISP_SHUTTER_TRIG_0_SELECT_INPUT */
    SELECT_INPUT_ISP_SHUTTER_TRIG_1,                    /*   72 - 0x05E0: IOMUXC_ISP_SHUTTER_TRIG_1_SELECT_INPUT */
    SELECT_INPUT_UART1_UART_RTS_B,                      /*   73 - 0x05E4: IOMUXC_UART1_UART_RTS_B_SELECT_INPUT */
    SELECT_INPUT_UART1_UART_RXD_MUX,                    /*   74 - 0x05E8: IOMUXC_UART1_UART_RXD_MUX_SELECT_INPUT */
    SELECT_INPUT_UART2_UART_RTS_B,                      /*   75 - 0x05EC: IOMUXC_UART2_UART_RTS_B_SELECT_INPUT */
    SELECT_INPUT_UART2_UART_RXD_MUX,                    /*   76 - 0x05F0: IOMUXC_UART2_UART_RXD_MUX_SELECT_INPUT */
    SELECT_INPUT_UART3_UART_RTS_B,                      /*   77 - 0x05F4: IOMUXC_UART3_UART_RTS_B_SELECT_INPUT */
    SELECT_INPUT_UART3_UART_RXD_MUX,                    /*   78 - 0x05F8: IOMUXC_UART3_UART_RXD_MUX_SELECT_INPUT */
    SELECT_INPUT_UART4_UART_RTS_B,                      /*   79 - 0x05FC: IOMUXC_UART4_UART_RTS_B_SELECT_INPUT  */
    SELECT_INPUT_UART4_UART_RXD_MUX,                    /*   80 - 0x0600: IOMUXC_UART4_UART_RXD_MUX_SELECT_INPUT */
    SELECT_INPUT_USDHC3_CARD_CLK_IN,                    /*   81 - 0x0604: IOMUXC_USDHC3_CARD_CLK_IN_SELECT_INPUT  */
    SELECT_INPUT_USDHC3_CARD_DET,                       /*   82 - 0x0608: IOMUXC_USDHC3_CARD_DET_SELECT_INPUT  */
    SELECT_INPUT_USDHC3_CMD_IN,                         /*   83 - 0x060C: IOMUXC_USDHC3_CMD_IN_SELECT_INPUT  */
    SELECT_INPUT_USDHC3_DAT0_IN,                        /*   84 - 0x0610: IOMUXC_USDHC3_DAT0_IN_SELECT_INPUT  */
    SELECT_INPUT_USDHC3_DAT1_IN,                        /*   85 - 0x0614: IOMUXC_USDHC3_DAT1_IN_SELECT_INPUT  */
    SELECT_INPUT_USDHC3_DAT2_IN,                        /*   86 - 0x0618: IOMUXC_USDHC3_DAT2_IN_SELECT_INPUT  */
    SELECT_INPUT_USDHC3_DAT3_IN,                        /*   87 - 0x061C: IOMUXC_USDHC3_DAT3_IN_SELECT_INPUT  */
    SELECT_INPUT_USDHC3_DAT4_IN,                        /*   88 - 0x0620: IOMUXC_USDHC3_DAT4_IN_SELECT_INPUT  */
    SELECT_INPUT_USDHC3_DAT5_IN,                        /*   89 - 0x0624: IOMUXC_USDHC3_DAT5_IN_SELECT_INPUT  */
    SELECT_INPUT_USDHC3_DAT6_IN,                        /*   90 - 0x0628: IOMUXC_USDHC3_DAT6_IN_SELECT_INPUT  */
    SELECT_INPUT_USDHC3_DAT7_IN,                        /*   91 - 0x062C: IOMUXC_USDHC3_DAT7_IN_SELECT_INPUT  */
    SELECT_INPUT_USDHC3_STROBE,                         /*   92 - 0x0630: IOMUXC_USDHC3_STROBE_SELECT_INPUT  */
    SELECT_INPUT_USDHC3_WP_ON,                          /*   93 - 0x0634: IOMUXC_USDHC3_WP_ON_SELECT_INPUT  */
};

#define REG_IOMUXC_SW_MUX_CTL_PAD(index)        (REG_IOMUXC.SW_MUX_CTL_PAD[(index)])
#define REG_IOMUXC_SW_PAD_CTL_PAD(index)        (REG_IOMUXC.SW_PAD_CTL_PAD[(index)])
#define REG_IOMUXC_SELECT_INPUT(index)          (REG_IOMUXC.SELECT_INPUT[(index)])


/**
 * Messaging Unit (MU)
 */
struct t_mu {
    UW TR[4];   /**< Processor x Transmit Register 0, offset: 0x0 */
    UW RR[4];   /**< Processor x Receive Register 0, offset: 0x10 */
    UW SR;      /**< Processor x Status Register, offset: 0x20 */
    UW CR;      /**< Processor x Control Register, offset: 0x24 */
};

/** Cortex-M7 side */
#define REG_MU_A    (*(volatile struct t_mu *)MU_A_BASE)
/** Cortex-A53 side */
#define REG_MU_B    (*(volatile struct t_mu *)MU_B_BASE)


/**
 * Semaphore (SEMA4)
 */
struct t_sema4 {
    UB  GATE[16];       /**< Semaphores Gate 0 Register, offset: 0x0 */
    UB  reserved_0[48];
    UH  CP0INE;         /**< Semaphores Processor n IRQ Notification Enable, array offset: 0x40, array step: 0x8 */
    UB  reserved_1[6];
    UH  CP1INE;
    UB  reserved_2[54];
    UH  CP0NTF;         /**< Semaphores Processor n IRQ Notification, array offset: 0x80, array step: 0x8 */
    UB  reserved_3[6];
    UH  CP1NTF;
    UB  reserved_4[118];
    UH  RSTGT;          /**< Semaphores (Secure) Reset Gate n, offset: 0x100 */
    UB  reserved_5[2];
    UH  RSTNTF;         /**< Semaphores (Secure) Reset IRQ Notification, offset: 0x104 */
};

#define REG_SEMA4       (*(volatile struct t_sema4 *)SEMAPHORE_HS_BASE)
#define REG_SEMA4_GATE(n) (REG_SEMA4.GATE[(n)])

#if !defined(CORTEX_M7__) /* CA53 */
/**
 * System Counter (CONTROL)
 */
struct t_sys_ctr_ctrl {
    UW  CNTCR;      /**< Counter Control Register, offset: 0x000 */
    UW  CNTSR;      /**< Counter Status Register, offset: 0x004 */
    UW  CNTCV[2];   /**< Counter Count Value Low/High Register, offset: 0x008 */
    UW  Reserved_0[4];
    UW  CNTFID[3];  /**< Frequency Modes Table {0,1,2} Register, offset: 0x020 */
    UW  Reserved_1[1001];
    UW  CNTID[1];   /**< Counter ID Register, offset: 0xFD0 */
};

/**
 * System Counter (READ)
 */
struct t_sys_ctr_read {
    UW  CNTCV[2];   /**< Counter Count Value Low/High Register, offset: 0x000 */
    UW  Reserved_0[1010];
    UW  CNTID[1];   /**< Counter ID Register, offset: 0xFD0 */
};

/**
 * System Counter (COMPARE)
 */
struct t_sys_ctr_cmp {
    UW  Reserved_0[8];
    UW  CMPCV[2];   /**< Compare Count Value Low/High Register, offset: 0x020 */
    UW  Reserved_1;
    UW  CMPCR;      /**< Compare Control Register, offset: 0x02C */
    UW  Reserved_2[1000];
    UW  CNTID[1];   /**< Counter ID Register, offset: 0xFD0 */
};

#define REG_SYS_CTR_CTRL        (*(volatile struct t_sys_ctr_ctrl *)SYSCNT_CTRL_REG_BASE)
#define REG_SYS_CTR_CMP         (*(volatile struct t_sys_ctr_cmp *)(SYSCNT_CMP_REG_BASE)
#define REG_SYS_CTR_READ        (*(volatile struct t_sys_ctr_read *)(SYSCNT_RD_REG_BASE)
#endif

/**
 * Interrupt Number (Private Interrupts)
 */

#if !defined(CORTEX_M7__) /* CA53 */

/* Cortex-A53 Interrupt */

/* SGI (INTID  0 - 15 ) */
#define INT_SOFT_0              (0U)    /* SGI  0 */
#define INT_SOFT_1              (1U)    /* SGI  1 */
#define INT_SOFT_2              (2U)    /* SGI  2 */
#define INT_SOFT_3              (3U)    /* SGI  3 */
#define INT_SOFT_4              (4U)    /* SGI  4 */
#define INT_SOFT_5              (5U)    /* SGI  5 */
#define INT_SOFT_6              (6U)    /* SGI  6 */
#define INT_SOFT_7              (7U)    /* SGI  7 */
#define INT_SOFT_8              (8U)    /* SGI  8 */
#define INT_SOFT_9              (9U)    /* SGI  9 */
#define INT_SOFT_10             (10U)   /* SGI 10 */
#define INT_SOFT_11             (11U)   /* SGI 11 */
#define INT_SOFT_12             (12U)   /* SGI 12 */
#define INT_SOFT_13             (13U)   /* SGI 13 */
#define INT_SOFT_14             (14U)   /* SGI 14 */
#define INT_SOFT_15             (15U)   /* SGI 15 */
/* PPI (INTID 16 - 31 ) */             
#define INT_RESERVED_16         (16U)   /* PPI 0 */
#define INT_RESERVED_17         (17U)   /* PPI 1 */
#define INT_RESERVED_18         (18U)   /* PPI 2 */
#define INT_RESERVED_19         (19U)   /* PPI 3 */
#define INT_RESERVED_20         (20U)   /* PPI 4 */
#define INT_RESERVED_21         (21U)   /* PPI 5 */
#define INT_RESERVED_22         (22U)   /* PPI 6 */
#define INT_RESERVED_23         (23U)   /* PPI 7 */
#define INT_RESERVED_24         (24U)   /* PPI 8 */
#define INT_VIRT_MAINTENANCE    (25U)   /* Virtual maintenance interrupt */
#define INT_HYP_TIMER           (26U)   /* Hypervisor timer              */
#define INT_VIRT_TIMER          (27U)   /* Virtual timer                 */
#define INT_FIQ                 (28U)   /* Legacy FIQ signal             */
#define INT_S_PHYSICAL_TIMER    (29U)   /* Secure physical timer         */
#define INT_NS_PHYSICAL_TIMER   (30U)   /* Non-secure physical timer     */
#define INT_IRQ                 (31U)   /* Legacy IRQ signal             */
/* SPI (INTID 32 or upper) */
#define INT_BOOT                (32U)
#define INT_DAP                 (33U)
#define INT_SDMA1               (34U)
#define INT_GPU3D               (35U)
#define INT_BUTTON_SHRT         (36U)
#define INT_LCDIF1              (37U)
#define INT_LCDIF2              (38U)
#define INT_VPUG1               (39U)
#define INT_VPUG2               (40U)
#define INT_QOS                 (41U)
#define INT_WDOG3               (42U)
#define INT_HS1                 (43U)
#define INT_GPMI_CPLT           (44U)
#define INT_ML                  (45U)
#define INT_BCH_CPLT            (46U)
#define INT_GPMI_TMOUT          (47U)
#define INT_ISI_CAMERA0         (48U)
#define INT_MIPI_CSI1           (49U)
#define INT_MIPI_DSI            (50U)
#define INT_SRTC_CSLDT          (51U)
#define INT_SRTC_SECUR          (52U)
#define INT_CSU                 (53U)
#define INT_USDHC1              (54U)
#define INT_USDHC2              (55U)
#define INT_USDHC3              (56U)
#define INT_GPU2D               (57U)
#define INT_UART1               (58U)
#define INT_UART2               (59U)
#define INT_UART3               (60U)
#define INT_UART4               (61U)
#define INT_VPU_ENC             (62U)
#define INT_ECSPI1              (63U)
#define INT_ECSPI2              (64U)
#define INT_ECSPI3              (65U)
#define INT_SDMA3               (66U)
#define INT_I2C1                (67U)
#define INT_I2C2                (68U)
#define INT_I2C3                (69U)
#define INT_I2C4                (70U)
#define INT_RDC                 (71U)
#define INT_USB1                (72U)
#define INT_USB2                (73U)
#define INT_ISI_CAMERA1         (74U)
#define INT_HDMI_TX             (75U)
#define INT_MIC_VAD_EV          (76U)
#define INT_MIC_VAD_ERR         (77U)
#define INT_GPT6                (78U)
#define INT_SCTR1               (79U)
#define INT_SCTR2               (80U)
#define INT_TEMP                (81U)
#define INT_SAI3                (82U)
#define INT_GPT5                (83U)
#define INT_GPT4                (84U)
#define INT_GPT3                (85U)
#define INT_GPT2                (86U)
#define INT_GPT1                (87U)
#define INT_GPIO1_7             (88U)
#define INT_GPIO1_6             (89U)
#define INT_GPIO1_5             (90U)
#define INT_GPIO1_4             (91U)
#define INT_GPIO1_3             (92U)
#define INT_GPIO1_2             (93U)
#define INT_GPIO1_1             (94U)
#define INT_GPIO1_0             (95U)
#define INT_GPIO1_L             (96U)
#define INT_GPIO1_H             (97U)
#define INT_GPIO2_L             (98U)
#define INT_GPIO2_H             (99U)
#define INT_GPIO3_L             (100U)
#define INT_GPIO3_H             (101U)
#define INT_GPIO4_L             (102U)
#define INT_GPIO4_H             (103U)
#define INT_GPIO5_L             (104U)
#define INT_GPIO5_H             (105U)
#define INT_ISP1                (106U)
#define INT_ISP2                (107U)
#define INT_I2C5                (108U)
#define INT_I2C6                (109U)
#define INT_WDOG1               (110U)
#define INT_WDOG2               (111U)
#define INT_MIPI_CSI2           (112U)
#define INT_PWM1                (113U)
#define INT_PWM2                (114U)
#define INT_PWM3                (115U)
#define INT_PWM4                (116U)
#define INT_CCM1                (117U)
#define INT_CCM2                (118U)
#define INT_GPC1                (119U)
#define INT_MU1_A53             (120U)
#define INT_SRC                 (121U)
#define INT_SAI5_6              (122U)
#define INT_RTIC                (123U)
#define INT_PERFUNIT            (124U)
#define INT_CTI                 (125U)
#define INT_WDG                 (126U)
#define INT_SAI1                (127U)
#define INT_SAI2                (128U)
#define INT_MU1_M7              (129U)
#define INT_DDRC_PERFMON        (130U)
#define INT_DDRC_ERR_DFI        (131U)
#define INT_DEWARP              (132U)
#define INT_AXI_ERROR           (133U)
#define INT_L2RAM_ERROR         (134U)
#define INT_SDMA2               (135U)
#define INT_SJC                 (136U)
#define INT_CAAM1               (137U)
#define INT_CAAM2               (138U)
#define INT_FLEXSPI             (139U)
#define INT_TZASC               (140U)
#define INT_MIC_EV              (141U)
#define INT_MIC_ERR             (142U)
#define INT_SAI7                (143U)
#define INT_PERFMON1            (144U)
#define INT_PERFMON2            (145U)
#define INT_CAAM3               (146U)
#define INT_CAAM_ERR            (147U)
#define INT_HS2                 (148U)
#define INT_CTI_CM7             (149U)
#define INT_ENET1_TXRX1         (150U)
#define INT_ENET1_TXRX2         (151U)
#define INT_ENET1_MISC          (152U)
#define INT_ENET1_TIMER         (153U)
#define INT_ASRC                (154U)
#define INT_PCIE_GLUE0          (155U)
#define INT_PCIE_GLUE1          (156U)
#define INT_PCIE_GLUE2          (157U)
#define INT_PCIE_GLUE3          (158U)
#define INT_PCIE_CH_H           (159U)
#define INT_XCVR_EARC0          (160U)
#define INT_XCVR_EARC1          (161U)
#define INT_AUD2HTX             (162U)
#define INT_AUD_EDMA_ERR        (163U)
#define INT_AUD_EDMA_L          (164U)
#define INT_AUD_EDMA_H          (165U)
#define INT_ENET_QOS_PMT        (166U)
#define INT_ENET_QOS_TSN        (167U)
#define INT_MU2_A53             (168U)
#define INT_MU2_AUDIO           (169U)
#define INT_MU3_M7              (170U)
#define INT_MU3_AUDIO           (171U)
#define INT_PCIE_RCEP           (172U)
#define INT_PCIE_RCEP_PME       (173U)
#define INT_CANFD1_0            (174U)
#define INT_CANFD1_1            (175U)
#define INT_CANFD2_0            (176U)
#define INT_CANFD2_1            (177U)
#define INT_XCVR_EARC_SPDIF     (178U)
#define INT_DDRC_ERR            (179U)
#define INT_USB1_WAKEUP         (180U)
#define INT_USB2_WAKEUP         (181U)
#define INT_OCRAM_ECC           (182U)
#define INT_OCRAM_ECC_ERR       (183U)
#define INT_OCRAMS_ECC          (184U)
#define INT_OCRAMS_ECC_ERR      (185U)

#else /* #if !defined(CORTEX_M7__) */
/* Cortex-M7 Interrrupt */

/* #define INT_Reserved          (IRQ0) */
#define INT_DAP             (IRQ1)
#define INT_SDMA1           (IRQ2)
#define INT_GPU3D           (IRQ3)
#define INT_BUTTON_SHRT     (IRQ4)
#define INT_LCDIF1          (IRQ5)
#define INT_LCDIF2          (IRQ6)
#define INT_VPUG1           (IRQ7)
#define INT_VPUG2           (IRQ8)
#define INT_QOS             (IRQ9)
#define INT_WDOG3           (IRQ10)
#define INT_HS_CP1          (IRQ11)
#define INT_GPMI_CPLT       (IRQ12)
#define INT_ML              (IRQ13)
#define INT_BCH_CPLT        (IRQ14)
#define INT_GPMI_TMOUT      (IRQ15)
#define INT_ISI_CAMERA0     (IRQ16)
#define INT_MIPI_CSI1       (IRQ17)
#define INT_MIPI_DSI        (IRQ18)
#define INT_SRTC_CSLDT      (IRQ19)
#define INT_SRTC_SECUR      (IRQ20)
#define INT_CSU             (IRQ21)
#define INT_USDHC1          (IRQ22)
#define INT_USDHC2          (IRQ23)
#define INT_USDHC3          (IRQ24)
#define INT_GPU2D           (IRQ25)
#define INT_UART1           (IRQ26)
#define INT_UART2           (IRQ27)
#define INT_UART3           (IRQ28)
#define INT_UART4           (IRQ29)
#define INT_VPU_ENC         (IRQ30)
#define INT_ECSPI1          (IRQ31)
#define INT_ECSPI2          (IRQ32)
#define INT_ECSPI3          (IRQ33)
#define INT_SDMA3           (IRQ34)
#define INT_I2C1            (IRQ35)
#define INT_I2C2            (IRQ36)
#define INT_I2C3            (IRQ37)
#define INT_I2C4            (IRQ38)
#define INT_RDC             (IRQ39)
#define INT_USB1            (IRQ40)
#define INT_USB2            (IRQ41)
#define INT_ISI_CAMERA1     (IRQ42)
#define INT_HDMI_TX         (IRQ43)
#define INT_MIC_VAD_EV      (IRQ44)
#define INT_MIC_VAD_ERR     (IRQ45)
#define INT_GPT6            (IRQ46)
#define INT_SCTR1           (IRQ47)
#define INT_SCTR2           (IRQ48)
#define INT_TEMP            (IRQ49)
#define INT_SAI3            (IRQ50)
#define INT_GPT5            (IRQ51)
#define INT_GPT4            (IRQ52)
#define INT_GPT3            (IRQ53)
#define INT_GPT2            (IRQ54)
#define INT_GPT1            (IRQ55)
#define INT_GPIO1_7         (IRQ56)
#define INT_GPIO1_6         (IRQ57)
#define INT_GPIO1_5         (IRQ58)
#define INT_GPIO1_4         (IRQ59)
#define INT_GPIO1_3         (IRQ60)
#define INT_GPIO1_2         (IRQ61)
#define INT_GPIO1_1         (IRQ62)
#define INT_GPIO1_0         (IRQ63)
#define INT_GPIO1_L         (IRQ64)
#define INT_GPIO1_H         (IRQ65)
#define INT_GPIO2_L         (IRQ66)
#define INT_GPIO2_H         (IRQ67)
#define INT_GPIO3_L         (IRQ68)
#define INT_GPIO3_H         (IRQ69)
#define INT_GPIO4_L         (IRQ70)
#define INT_GPIO4_H         (IRQ71)
#define INT_GPIO5_L         (IRQ72)
#define INT_GPIO5_H         (IRQ73)
#define INT_ISP1            (IRQ74)
#define INT_ISP2            (IRQ75)
#define INT_I2C5            (IRQ76)
#define INT_I2C6            (IRQ77)
#define INT_WDOG1           (IRQ78)
#define INT_WDOG2           (IRQ79)
#define INT_MIPI_CSI2       (IRQ80)
#define INT_PWM1            (IRQ81)
#define INT_PWM2            (IRQ82)
#define INT_PWM3            (IRQ83)
#define INT_PWM4            (IRQ84)
#define INT_CCM1            (IRQ85)
#define INT_CCM2            (IRQ86)
#define INT_GPC1            (IRQ87)
#define INT_MU1_A53         (IRQ88)
#define INT_SRC             (IRQ89)
#define INT_SAI5_6          (IRQ90)
#define INT_RTIC            (IRQ91)
#define INT_PERFUNIT        (IRQ92)
#define INT_CTI             (IRQ93)
#define INT_WDG             (IRQ94)
#define INT_SAI1            (IRQ95)
#define INT_SAI2            (IRQ96)
#define INT_MU1_M7          (IRQ97)
#define INT_DDRC_PERFMON    (IRQ98)
#define INT_DDRC_ERR_DFI    (IRQ99)
#define INT_DEWARP          (IRQ100)
#define INT_AXI_ERROR       (IRQ101)
#define INT_L2RAM_ERROR     (IRQ102)
#define INT_SDMA2           (IRQ103)
#define INT_SJC             (IRQ104)
#define INT_CAAM1           (IRQ105)
#define INT_CAAM2           (IRQ106)
#define INT_FLEXSPI         (IRQ107)
#define INT_TZASC           (IRQ108)
#define INT_MIC_EV          (IRQ109)
#define INT_MIC_ERR         (IRQ110)
#define INT_SAI7            (IRQ111)
#define INT_PERFMON1        (IRQ112)
#define INT_PERFMON2        (IRQ113)
#define INT_CAAM3           (IRQ114)
#define INT_CAAM_ERR        (IRQ115)
#define INT_HS2             (IRQ116)
#define INT_CTI_CM7         (IRQ117)
#define INT_ENET1_TXRX1     (IRQ118)
#define INT_ENET1_TXRX2     (IRQ119)
#define INT_ENET1_MISC      (IRQ120)
#define INT_ENET1_TIMER     (IRQ121)
#define INT_ASRC            (IRQ122)
#define INT_PCIE_GLUE0      (IRQ123)
#define INT_PCIE_GLUE1      (IRQ124)
#define INT_PCIE_GLUE2      (IRQ125)
#define INT_PCIE_GLUE3      (IRQ126)
#define INT_PCIE_CH_H       (IRQ127)
#define INT_XCVR_EARC0      (IRQ128)
#define INT_XCVR_EARC1      (IRQ129)
#define INT_AUD2HTX         (IRQ130)
#define INT_AUD_EDMA_ERR    (IRQ131)
#define INT_AUD_EDMA_L      (IRQ132)
#define INT_AUD_EDMA_H      (IRQ133)
#define INT_ENET_QOS_PMT    (IRQ134)
#define INT_ENET_QOS_TSN    (IRQ135)
#define INT_MU2_A53         (IRQ136)
#define INT_MU2_AUDIO       (IRQ137)
#define INT_MU3_M7          (IRQ138)
#define INT_MU3_AUDIO       (IRQ139)
#define INT_PCIE_RCEP       (IRQ140)
#define INT_PCIE_RCEP_PME   (IRQ141)
#define INT_CANFD1_0        (IRQ142)
#define INT_CANFD1_1        (IRQ143)
#define INT_CANFD2_0        (IRQ144)
#define INT_CANFD2_1        (IRQ145)
#define INT_XCVR_EARC_SPDIF (IRQ146)
#define INT_DDRC_ERR        (IRQ147)
#define INT_USB1_WAKEUP     (IRQ148)
#define INT_USB2_WAKEUP     (IRQ149)
#define INT_OCRAM_ECC       (IRQ150)
#define INT_OCRAM_ECC_ERR   (IRQ151)
#define INT_OCRAMS_ECC      (IRQ152)
#define INT_OCRAMS_ECC_ERR  (IRQ153)

#endif /* #if !defined(CORTEX_M7__) */


#if defined(__cplusplus)
}
#endif
#endif /* IMX8MPLUS_H_ */
