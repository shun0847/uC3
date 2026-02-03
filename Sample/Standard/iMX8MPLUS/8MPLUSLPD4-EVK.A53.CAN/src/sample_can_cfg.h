/* ▽▽▽ SNY CANドライバを使用するため追加 add ▽▽▽ */
#ifndef _SAMPLE_CAN_CFG_H__H_
#define _SAMPLE_CAN_CFG_H__H_

#include "fsl_flexcan.h"
#include "MIMX8ML8_ca53.h"
/* -----------------------------------------------------------------------------
 * マクロ定義
 * ----------------------------------------------------------------------------- */
/* App.h */
#define EXAMPLE_CAN1 FLEXCAN1
#define EXAMPLE_CAN1_CLK_FREQ                                                                    \
    (CLOCK_GetPllFreq(kCLOCK_SystemPll1Ctrl) / (CLOCK_GetRootPreDivider(kCLOCK_RootFlexCan1)) / \
     (CLOCK_GetRootPostDivider(kCLOCK_RootFlexCan1)))
#ifdef USE_CAN2
#define EXAMPLE_CAN2 FLEXCAN2
#define EXAMPLE_CAN2_CLK_FREQ                                                                    \
    (CLOCK_GetPllFreq(kCLOCK_SystemPll1Ctrl) / (CLOCK_GetRootPreDivider(kCLOCK_RootFlexCan2)) / \
     (CLOCK_GetRootPostDivider(kCLOCK_RootFlexCan2)))
#endif
/* USE_IMPROVED_TIMING_CONFIG マクロを設定して、API を使用して改善された CAN / CAN FD タイミング値を計算します。 */
#define USE_IMPROVED_TIMING_CONFIG (1U)

/* 最初の有効な MB は、ERR005641 の予約済み TX MB として使用する必要があることを考慮します。 */
#define RX_QUEUE_BUFFER_BASE  (1U)
#define RX_QUEUE_BUFFER_SIZE  (4U)
#define TX_MESSAGE_BUFFER_NUM (8U)

/* Tx MB ID. */
#define TX_CAN1_ID 0x100UL
#define TX_CAN2_ID 0x200UL
/* RX MB ID. */
#define RX_CAN1_ID 0x101UL
#define RX_CAN2_ID 0x201UL
/* RX MB 個別マスク。これにより、FLEXCAN は受信メッセージの ID の 1 ～ 11 ビットをすべてチェックします。 */
#define RX_CAN1_ID_FULL_MASK 0x7FFUL

/*
 *    DWORD_IN_MB    DLC    BYTES_IN_MB             Maximum MBs
 *    2              8      kFLEXCAN_8BperMB    32(1 RAM block)  64(2 RAM block)  96(3 RAM block)
 *    4              10     kFLEXCAN_16BperMB   21(1 RAM block)  42(2 RAM block)  63(3 RAM block)
 *    8              13     kFLEXCAN_32BperMB   12(1 RAM block)  24(2 RAM block)  36(3 RAM block)
 *    16             15     kFLEXCAN_64BperMB   7(1 RAM block)   14(2 RAM block)  21(3 RAM block)
 *
 * 各メッセージバッファ内のDword、バイト単位のデータ長、ペイロードサイズは揃える必要がある、
 * メッセージ バッファは各ペイロード構成に応じて制限されます。
 */
#if (defined(USE_CANFD) && USE_CANFD)
#define DLC         (15)
#define BYTES_IN_MB kFLEXCAN_64BperMB
#else
#define DLC         (8)
#endif

/*I2C3通信*/
#define BOARD_CODEC_I2C            I2C3
#define BOARD_CODEC_I2C_CLOCK_FREQ (24000000U)
#define I2C_RETRY_TIMES (0xFFFFU)

/* -----------------------------------------------------------------------------
 * グローバル関数
 * ----------------------------------------------------------------------------- */
extern void flexcan_clockinit(void);
extern void flexcan_pininit(void);
extern void flexcan_init(void);
extern void i2c3_init(void);
extern void flexcan_sendframe(void);
extern status_t flexcan_receiveframe(CAN_Type *base, flexcan_frame_t* frame);

#endif
/* △△△ SNY CANドライバを使用するため追加 add △△△ */