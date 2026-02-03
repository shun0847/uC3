/**
 * @brief   User configuration for i.MX8M Plus UART
 * @date    2025.12.19
 *
 * @copyright (C) 2025, eForce Co., Ltd.
 */
#ifndef DDR_IMX_UART_CFG_H_
#define DDR_IMX_UART_CFG_H_

#include "imx8mplus_uC3.h"
#include "DDR_COM.h"
#include "DDR_iMX_UART.h"
#include "cpu_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UART_CLK        (24000000U) /**< 24MHz */

/** Driver configuration */
/**
 * UART4 configuraion
 */
#define UART_4                              /* 未使用の場合はコメントアウト */
#define TXBUF_SZ4       (1024U)             /* 送信バッファサイズ */
#define RXBUF_SZ4       (1024U)             /* 受信バッファサイズ */
#define OFF_SZ4        (RXBUF_SZ4 / 2U)    /* XOFF送出受信バッファデータ数トリガ */
#define XON_SZ4         (RXBUF_SZ4 / 8U)    /* XON送出受信バッファデータ数トリガ */
#define RXTL_4          (24U)               /* レシーブFIFOデータ数トリガ */
                                            /* 0~32 */
#define TXTL_4          (8U)                /* トランスミットFIFOデータ数トリガ */
                                            /* 2~32 */
#define RTSTL_4         (24U)               /* RTS出力アクティブトリガ */
                                            /* 0~32 */
#define DCEDTE_4        (0U)                /* DCE(0)/DTE(1)の選択 */
#define IPL_UART4       (0xE0U)             /* UART4割り込み優先度 */
#define CFG_INT_UART4   (INT_UART4)         /* UART4 IRQ番号 */
#define REG_UART4       (*(volatile struct t_uart *)UART4_BASE)
                                            /* UART4デバイスアドレス */ 

#ifdef __cplusplus
}
#endif
#endif /* DDR_IMX_UART_CFG_H_ */
