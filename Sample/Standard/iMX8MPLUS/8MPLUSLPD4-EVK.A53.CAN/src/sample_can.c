/* ▽▽▽ SNY CANドライバを使用するため追加 add ▽▽▽ */

#include "fsl_flexcan.h"
#include "sample_can_cfg.h"
#include "kernel.h"
#include "imx8mplus_uC3.h"
#include "MIMX8ML8_ca53.h"
#include "fsl_i2c.h"

/* -----------------------------------------------------------------------------
 * プロトタイプ宣言
 * ----------------------------------------------------------------------------- */
void flexcan_clockinit(void);
void flexcan_pininit(void);
void flexcan_init(void);
void i2c3_init(void);
void flexcan_sendframe(void);
status_t flexcan_receiveframe(CAN_Type *base, flexcan_frame_t* frame);

/* -----------------------------------------------------------------------------
 * 変数
 * ----------------------------------------------------------------------------- */
#if (defined(USE_CANFD) && USE_CANFD)
flexcan_fd_frame_t rxFrame[RX_QUEUE_BUFFER_SIZE * 2];
flexcan_fd_frame_t txFrame;
#else
flexcan_frame_t rxFrame;
flexcan_frame_t txFrame;
#endif

/* -----------------------------------------------------------------------------
 * 関数
 * ----------------------------------------------------------------------------- */
void flexcan_clockinit(void)
{
    CLOCK_SetRootMux(kCLOCK_RootFlexCan1, kCLOCK_FlexCanRootmuxSysPll1); /* Set FLEXCAN1 source to SYSTEM PLL1 800MHZ */
    CLOCK_SetRootDivider(kCLOCK_RootFlexCan1, 2U, 5U);                   /* Set root clock to 800MHZ / 10 = 80MHZ */
#ifdef USE_CAN2
    CLOCK_SetRootMux(kCLOCK_RootI2c3, kCLOCK_I2cRootmuxOsc24M); // MUX=000: 24M_REF_CLK
    CLOCK_SetRootDivider(kCLOCK_RootI2c3, 1U, 1U);                // PRE=1, POST=1
    CLOCK_SetRootMux(kCLOCK_RootFlexCan2, kCLOCK_FlexCanRootmuxSysPll1); /* Set FLEXCAN2 source to SYSTEM PLL1 800MHZ */
    CLOCK_SetRootDivider(kCLOCK_RootFlexCan2, 2U, 5U);                   /* Set root clock to 800MHZ / 10 = 80MHZ */
#endif
}

void flexcan_pininit(void) 
{   /* FLEXCAN1 */
    REG_IOMUXC_SW_MUX_CTL_PAD(SW_MUX_CTL_PAD_SPDIF_EXT_CLK) = 5U | (0U << 4);  /* MUX_MODE=101 ALT5_GPIO_IO05, SION=0 */
    REG_IOMUXC_SW_PAD_CTL_PAD(SW_PAD_CTL_PAD_SPDIF_EXT_CLK) = 0x142;
    REG_IOMUXC_SW_MUX_CTL_PAD(SW_MUX_CTL_PAD_SPDIF_RX) = 4U | (0U << 4);  /* MUX_MODE=100 ALT4_CAN1_RX, SION=0 */
    REG_IOMUXC_SW_PAD_CTL_PAD(SW_PAD_CTL_PAD_SPDIF_RX) |= 0x151;
    REG_IOMUXC_SW_MUX_CTL_PAD(SW_MUX_CTL_PAD_SPDIF_TX) = 4U | (0U << 4);  /* MUX_MODE=100 ALT4_CAN1_TX, SION=0 */
    REG_IOMUXC_SW_PAD_CTL_PAD(SW_PAD_CTL_PAD_SPDIF_TX) = 0x151; 
    REG_IOMUXC_SELECT_INPUT(SELECT_INPUT_CAN1_CANRX) = 2U;   /*   SELECT_SPDIF_RX_ALT4 — Selecting Pad: SPDIF_RX for Mode: ALT4 */
#ifdef USE_CAN2
    /*I2C3*/
    REG_IOMUXC_SW_MUX_CTL_PAD(SW_MUX_CTL_PAD_I2C3_SCL) = 0U | (1U << 4);  /* MUX_MODE=000 ALT0_I2C3_SCL, SION=1(ENABLED) */
    REG_IOMUXC_SW_PAD_CTL_PAD(SW_PAD_CTL_PAD_I2C3_SCL) = 0x0A4;
    REG_IOMUXC_SELECT_INPUT(SELECT_INPUT_I2C3_SCL_IN) = 4U;   /*  MUX_MODE=100 SELECT_I2C3_SCL_ALT0 — I2C3_SCL for Mode: ALT0 */
    REG_IOMUXC_SW_MUX_CTL_PAD(SW_MUX_CTL_PAD_I2C3_SDA) = 0U | (1U << 4);  /* MUX_MODE=000 ALT0_I2C3_SDA, SION=1(ENABLED) */
    REG_IOMUXC_SW_PAD_CTL_PAD(SW_PAD_CTL_PAD_I2C3_SDA) = 0x0A4;
    REG_IOMUXC_SELECT_INPUT(SELECT_INPUT_I2C3_SDA_IN) = 4U;   /*  MUX_MODE=100 SELECT_I2C3_SCL_ALT0 — I2C3_SCL for Mode: ALT0 */
    /* FLEXCAN2 */
    REG_IOMUXC_SW_MUX_CTL_PAD(SW_MUX_CTL_PAD_SAI2_MCLK) = 5U | (0U << 4);  /* MUX_MODE=101 ALT5_GPIO4_IO27, SION=0 */
    REG_IOMUXC_SW_PAD_CTL_PAD(SW_PAD_CTL_PAD_SAI2_MCLK) = 0x142;
    REG_IOMUXC_SW_MUX_CTL_PAD(SW_MUX_CTL_PAD_SAI5_MCLK) = 6U | (0U << 4);  /* MUX_MODE=110 ALT6_CAN1_RX, SION=0 */
    REG_IOMUXC_SW_PAD_CTL_PAD(SW_PAD_CTL_PAD_SAI5_MCLK) |= 0x151;
    REG_IOMUXC_SW_MUX_CTL_PAD(SW_MUX_CTL_PAD_SAI5_RXD3) = 6U | (0U << 4);  /* MUX_MODE=110 ALT6_CAN1_RX, SION=0 */
    REG_IOMUXC_SW_PAD_CTL_PAD(SW_PAD_CTL_PAD_SAI5_RXD3) = 0x151; 
    REG_IOMUXC_SELECT_INPUT(SELECT_INPUT_CAN2_CANRX) = 0U;   /*   SELECT_SPDIF_RX_ALT6 — Selecting Pad: SAI5_MCLK for Mode: ALT6 */
#endif
}

void i2c3_init(void)
{
    i2c_master_config_t masterConfig;
    /*
     * masterConfig.debugEnable = false;
     * masterConfig.ignoreAck = false;
     * masterConfig.pinConfig = kLPI2C_2PinOpenDrain;
     * masterConfig.baudRate_Hz = 100000U;
     * masterConfig.busIdleTimeout_ns = 0;
     * masterConfig.pinLowTimeout_ns = 0;
     * masterConfig.sdaGlitchFilterWidth_ns = 0;
     * masterConfig.sclGlitchFilterWidth_ns = 0;
     */
    I2C_MasterGetDefaultConfig(&masterConfig);
    I2C_MasterInit(BOARD_CODEC_I2C, &masterConfig, BOARD_CODEC_I2C_CLOCK_FREQ);
    
    i2c_master_transfer_t masterXfer;
    memset(&masterXfer, 0, sizeof(masterXfer));
    uint8_t v = 0xF7;          // 送信したい1バイト

    masterXfer.slaveAddress   = 0x20;
    masterXfer.direction      = kI2C_Write;
    masterXfer.subaddress     = 0x06;
    masterXfer.subaddressSize = 1;
    masterXfer.data           = &v;
    masterXfer.dataSize       = 1;
    masterXfer.flags          = kI2C_TransferDefaultFlag;

    I2C_MasterTransferBlocking(BOARD_CODEC_I2C, &masterXfer);
}

void flexcan_init(void)
{
    flexcan_config_t flexcanConfig;
    flexcan_rx_mb_config_t mbConfig;
    uint32_t i;

    /* Get FlexCAN module default Configuration. */
    /*
     * flexcanConfig.clkSrc                 = kFLEXCAN_ClkSrc0;
     * flexcanConfig.bitRate                = 1000000U;
     * flexcanConfig.bitRateFD              = 2000000U;
     * flexcanConfig.maxMbNum               = 16;
     * flexcanConfig.enableLoopBack         = false;
     * flexcanConfig.enableSelfWakeup       = false;
     * flexcanConfig.enableIndividMask      = false;
     * flexcanConfig.disableSelfReception   = false;
     * flexcanConfig.enableListenOnlyMode   = false;
     * flexcanConfig.enableDoze             = false;
     */
    FLEXCAN_GetDefaultConfig(&flexcanConfig);
    flexcanConfig.bitRate = 500000U;

    /* Enable Rx Individual Mask and Queue feature. */
    flexcanConfig.enableIndividMask = true;

#if defined(EXAMPLE_CAN1_CLK_SOURCE)
    flexcanConfig.clkSrc = EXAMPLE_CAN1_CLK_SOURCE;
#endif

/* Use the FLEXCAN API to automatically get the ideal bit timing configuration. */
#if (defined(USE_IMPROVED_TIMING_CONFIG) && USE_IMPROVED_TIMING_CONFIG)
    flexcan_timing_config_t timing_config;
    memset(&timing_config, 0, sizeof(flexcan_timing_config_t));
#if (defined(USE_CANFD) && USE_CANFD)
    if (FLEXCAN_FDCalculateImprovedTimingValues(EXAMPLE_CAN1, flexcanConfig.bitRate, flexcanConfig.bitRateFD,
                                                EXAMPLE_CAN1_CLK_FREQ, &timing_config))
    {
        /* Update the improved timing configuration*/
        memcpy(&(flexcanConfig.timingConfig), &timing_config, sizeof(flexcan_timing_config_t));
    }
    else
    {
        /* 処理なし */
    }
#else
    if (FLEXCAN_CalculateImprovedTimingValues(EXAMPLE_CAN1, flexcanConfig.bitRate, EXAMPLE_CAN1_CLK_FREQ, &timing_config))
    {
        /* Update the improved timing configuration*/
        memcpy(&(flexcanConfig.timingConfig), &timing_config, sizeof(flexcan_timing_config_t));
    }
    else
    {
        /* 処理なし */
    }
#endif
#endif

#if (defined(USE_CANFD) && USE_CANFD)
    FLEXCAN_FDInit(EXAMPLE_CAN1, &flexcanConfig, EXAMPLE_CAN1_CLK_FREQ, BYTES_IN_MB, true);
#else
    FLEXCAN_Init(EXAMPLE_CAN1, &flexcanConfig, EXAMPLE_CAN1_CLK_FREQ);
#ifdef USE_CAN2
    FLEXCAN_Init(EXAMPLE_CAN2, &flexcanConfig, EXAMPLE_CAN2_CLK_FREQ);
#endif
#endif

        /* Setup Tx Message Buffer. */
#if (defined(USE_CANFD) && USE_CANFD)
        FLEXCAN_SetFDTxMbConfig(EXAMPLE_CAN1, TX_MESSAGE_BUFFER_NUM, true);
#else
        FLEXCAN_SetTxMbConfig(EXAMPLE_CAN1, TX_MESSAGE_BUFFER_NUM, true);
#ifdef USE_CAN2
        FLEXCAN_SetTxMbConfig(EXAMPLE_CAN2, TX_MESSAGE_BUFFER_NUM, true);
#endif
#endif

    /* Setup Rx Message Buffer. */
    /* Suppose to receive message ID 0x21. */
    mbConfig.format = kFLEXCAN_FrameFormatStandard;
    mbConfig.type   = kFLEXCAN_FrameTypeData;
    mbConfig.id     = FLEXCAN_ID_STD(RX_CAN1_ID);

    for (i = 0U; i < RX_QUEUE_BUFFER_SIZE * 2U -1; i++)
    {
        /* Setup Rx individual ID mask 0xff. */
        /* Rx MB default matched ID is 0x21, it can match ID 0x321 after masked by 0xff. */
        FLEXCAN_SetRxIndividualMask(EXAMPLE_CAN1, RX_QUEUE_BUFFER_BASE + i,
                                    FLEXCAN_RX_MB_STD_MASK(RX_CAN1_ID_FULL_MASK, 0, 0));
#if (defined(USE_CANFD) && USE_CANFD)
        FLEXCAN_SetFDRxMbConfig(EXAMPLE_CAN1, RX_QUEUE_BUFFER_BASE + i, &mbConfig, true);
#else
        FLEXCAN_SetRxMbConfig(EXAMPLE_CAN1, RX_QUEUE_BUFFER_BASE + i, &mbConfig, true);
#endif
#ifdef USE_CAN2
    mbConfig.id     = FLEXCAN_ID_STD(RX_CAN2_ID);
        FLEXCAN_SetRxIndividualMask(EXAMPLE_CAN2, RX_QUEUE_BUFFER_BASE + i,
                                    FLEXCAN_RX_MB_STD_MASK(RX_CAN1_ID_FULL_MASK, 0, 0));
#if (defined(USE_CANFD) && USE_CANFD)
        FLEXCAN_SetFDRxMbConfig(EXAMPLE_CAN2, RX_QUEUE_BUFFER_BASE + i, &mbConfig, true);
#else
        FLEXCAN_SetRxMbConfig(EXAMPLE_CAN2, RX_QUEUE_BUFFER_BASE + i, &mbConfig, true);
#endif
#endif
    }
}

void flexcan_sendframe(void){
    txFrame.id     = FLEXCAN_ID_STD(TX_CAN1_ID);
    txFrame.format = (uint8_t)kFLEXCAN_FrameFormatStandard;
    txFrame.type   = (uint8_t)kFLEXCAN_FrameTypeData;
    txFrame.length = (uint8_t)DLC;
    txFrame.dataByte0 = 0;
    txFrame.dataByte1 = 1;
    txFrame.dataByte2 = 2;
    txFrame.dataByte3 = 3;
    txFrame.dataByte4 = 4;
    txFrame.dataByte5 = 5;
    txFrame.dataByte6 = 6;
    txFrame.dataByte7 = 7;
#if (defined(USE_CANFD) && USE_CANFD)
    txFrame.brs = 1U;
    txFrame.edl = 1U;
#endif

#if (defined(USE_CANFD) && USE_CANFD)
    (void)FLEXCAN_TransferFDSendBlocking(EXAMPLE_CAN1, TX_MESSAGE_BUFFER_NUM, &txFrame);
#else
    (void)FLEXCAN_TransferSendBlocking(EXAMPLE_CAN1, TX_MESSAGE_BUFFER_NUM, &txFrame);
#endif
#ifdef USE_CAN2
    txFrame.id     = FLEXCAN_ID_STD(TX_CAN2_ID);
    (void)FLEXCAN_TransferSendBlocking(EXAMPLE_CAN2, TX_MESSAGE_BUFFER_NUM, &txFrame);
#endif
}

status_t flexcan_receiveframe(CAN_Type *base,flexcan_frame_t* frame)
{
    status_t result;
    result = FLEXCAN_TransferReceiveBlocking(base, RX_QUEUE_BUFFER_BASE, &rxFrame);
    if (result == kStatus_Success)
    {
        memcpy(frame, &rxFrame, sizeof (flexcan_frame_t));
    }
    
    return result;
}

/* △△△ SNY CANドライバを使用するため追加 add △△△ */