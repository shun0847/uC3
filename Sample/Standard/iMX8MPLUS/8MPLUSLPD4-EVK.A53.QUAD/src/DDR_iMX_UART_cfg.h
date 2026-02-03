/**
 * @brief   User configuration for i.MX8M Plus UART
 * @date    2025.09.29
 *
 * @copyright (C) 2025, eForce Co., Ltd.
 */
#ifndef DDR_IMX_UART_CFG_H_
#define DDR_IMX_UART_CFG_H_

#include "imx8mplus_uC3.h"
#include "DDR_COM.h"
#include "DDR_iMX_UART.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UART_CLK      (24000000U) /**< 24MHz */

/**
 * UART2 configuraion
 */

/** Driver configuration */
#define UART_2                         /* 未使用の場合はコメントアウト */
#define TXBUF_SZ2     (1024U)          /* 送信バッファサイズ */
#define RXBUF_SZ2     (1024U)          /* 受信バッファサイズ */
#define XOFF_SZ2      (RXBUF_SZ2 / 2U) /* XOFF送出受信バッファデータ数トリガ */
#define XON_SZ2       (RXBUF_SZ2 / 8U) /* XON送出受信バッファデータ数トリガ */
#define RXTL_2        (24U)            /* レシーブFIFOデータ数トリガ */
                                       /* 0~32 */
#define TXTL_2        (8U)             /* トランスミットFIFOデータ数トリガ */
                                       /* 2~32 */
#define RTSTL_2       (24U)            /* RTS出力アクティブトリガ */
                                       /* 0~32 */
#define DCEDTE_2      (0U)             /* DCE(0)/DTE(1)の選択 */
#define IPL_UART2     (0xE0U)          /* UART2割り込み優先度 */
#define CFG_INT_UART2 (INT_UART2)      /* UART2 IRQ番号 */
#define REG_UART2     (*(volatile struct t_uart *)UART2_BASE)
/* UART2デバイスアドレス */

#ifdef __cplusplus
}
#endif
#endif /* DDR_IMX_UART_CFG_H_ */
