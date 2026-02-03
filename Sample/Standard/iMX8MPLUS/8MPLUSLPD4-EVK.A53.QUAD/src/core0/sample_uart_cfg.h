/**
 * @brief   User configuration for UART sample program
 * @date    2025.09.29
 *
 * @copyright (C) 2025, eForce Co., Ltd.
 */
#ifndef SAMPLE_UART_CFG_H_
#define SAMPLE_UART_CFG_H_

#include "DDR_iMX_UART.h"
#include "DDR_iMX_UART_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ID_UART          (2)
#define REG_UART         (REG_UART2)
#define CFG_BAUDRATE     (115200U)
#define CFG_BLEN         (BLEN8)
#define CFG_PAR          (PAR_NONE)
#define CFG_SBIT         (SBIT1)
#define CFG_FLW          (FLW_NONE)

#define DDR_UART_INIT_FN (_ddr_imx_uart_init)

#ifdef __cplusplus
}
#endif
#endif /* SAMPLE_UART_CFG_H_ */
