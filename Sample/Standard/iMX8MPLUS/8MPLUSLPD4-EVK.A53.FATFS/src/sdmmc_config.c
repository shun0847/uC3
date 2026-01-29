/*
 * Copyright 2020 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "sdmmc_config.h"
#include "imx8mplus_uC3.h"
#include "kernel.h"
#include "fsl_clock.h"
#include "fsl_usdhc.h"
#include "sample_fatfs_cfg.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define USDHC3_ISR_IMASK (0xE0U)

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
static void USDHC3_ISR(VP_INT exinf);

/*******************************************************************************
 * Variables
 ******************************************************************************/
/*!brief sdmmc dma buffer */
AT_NONCACHEABLE_SECTION_ALIGN(uint32_t s_sdmmcHostDmaBuffer[BOARD_SDMMC_HOST_DMA_DESCRIPTOR_BUFFER_SIZE],
                              SDMMCHOST_DMA_DESCRIPTOR_BUFFER_ALIGN_SIZE);
#if defined SDMMCHOST_ENABLE_CACHE_LINE_ALIGN_TRANSFER && SDMMCHOST_ENABLE_CACHE_LINE_ALIGN_TRANSFER
/* two cache line length for sdmmc host driver maintain unalign transfer */
SDK_ALIGN(static uint8_t s_sdmmcCacheLineAlignBuffer[BOARD_SDMMC_DATA_BUFFER_ALIGN_SIZE * 2U],
          BOARD_SDMMC_DATA_BUFFER_ALIGN_SIZE);
#endif

sdmmchost_t s_host;
static T_CISR s_usdhc3_cisr = {TA_HLNG, 0, INT_USDHC3, (FP)USDHC3_ISR, USDHC3_ISR_IMASK};

/*******************************************************************************
 * Code
 ******************************************************************************/
uint32_t BOARD_USDHC3ClockConfiguration(void)
{
    uint32_t freq;

    freq = CLOCK_GetClockRootFreq(kCLOCK_Usdhc3ClkRoot);
    return freq;
}

#ifdef MMC_ENABLED
static void BOARD_MMC_Pin_Config(uint32_t freq)
{
}

void BOARD_MMC_Config(void *card, uint32_t hostIRQPriority)
{
    assert(card);

    s_host.dmaDesBuffer         = s_sdmmcHostDmaBuffer;
    s_host.dmaDesBufferWordsNum = BOARD_SDMMC_HOST_DMA_DESCRIPTOR_BUFFER_SIZE;
#if ((defined __DCACHE_PRESENT) && __DCACHE_PRESENT) || (defined FSL_FEATURE_HAS_L1CACHE && FSL_FEATURE_HAS_L1CACHE)
    s_host.enableCacheControl = BOARD_SDMMC_HOST_CACHE_CONTROL;
#if defined SDMMCHOST_ENABLE_CACHE_LINE_ALIGN_TRANSFER && SDMMCHOST_ENABLE_CACHE_LINE_ALIGN_TRANSFER
    s_host.cacheAlignBuffer     = s_sdmmcCacheLineAlignBuffer;
    s_host.cacheAlignBufferSize = BOARD_SDMMC_DATA_BUFFER_ALIGN_SIZE * 2U;
#endif
#endif

    ((mmc_card_t *)card)->host                                = &s_host;
    ((mmc_card_t *)card)->host->hostController.base           = BOARD_SDMMC_MMC_HOST_BASEADDR;
    ((mmc_card_t *)card)->host->hostController.sourceClock_Hz = BOARD_USDHC3ClockConfiguration();
    ((mmc_card_t *)card)->usrParam.ioStrength                 = BOARD_MMC_Pin_Config;
    ((mmc_card_t *)card)->usrParam.maxFreq                    = BOARD_SDMMC_MMC_HOST_SUPPORT_HS200_FREQ;

    ((mmc_card_t *)card)->hostVoltageWindowVCC  = BOARD_SDMMC_MMC_VCC_SUPPLY;
    ((mmc_card_t *)card)->hostVoltageWindowVCCQ = BOARD_SDMMC_MMC_VCCQ_SUPPLY;

    (void)hostIRQPriority;

    {
        ER_ID ercd_isr = acre_isr(&s_usdhc3_cisr);
        ER ercd_dis    = dis_int(INT_USDHC3);
        ER ercd_ena    = ena_int(INT_USDHC3);

    }

#if defined(__GIC_PRIO_BITS)
    GIC_SetPriority(BOARD_SDMMC_MMC_HOST_IRQ, hostIRQPriority);
    GIC_EnableIRQ(BOARD_SDMMC_MMC_HOST_IRQ);
#else
    NVIC_SetPriority(BOARD_SDMMC_MMC_HOST_IRQ, hostIRQPriority);
    NVIC_EnableIRQ(BOARD_SDMMC_MMC_HOST_IRQ);
#endif
}
#endif

void USDHC3_IRQHandler(void)
{
    extern volatile uint32_t g_usdhc3_irq_count;
    g_usdhc3_irq_count++;
    USDHC_TransferHandleIRQ(BOARD_SDMMC_MMC_HOST_BASEADDR, &s_host.handle);
}

static void USDHC3_ISR(VP_INT exinf)
{
    (void)exinf;
    USDHC3_IRQHandler();
}
