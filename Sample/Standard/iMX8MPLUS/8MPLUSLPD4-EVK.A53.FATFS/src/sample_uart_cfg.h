/**
 * @file    sample_uart_cfg.h
 * @brief   User configuration for UART sample program
 * @date    2024.10.08
 * @author  Copyright (c) 2020-2024, eForce Co., Ltd. All rights reserved.
 *
 ****************************************************************************
 * @par     History
 *          - rev 1.0 (2020.12.23) Imada
 *            Initial version.
 *          - rev 1.1 (2024.10.08) Imada
 *            Use UART4 for uC3 booting up on Core#3.
 ****************************************************************************
 */
#ifndef SAMPLE_UART_CFG_H_
#define SAMPLE_UART_CFG_H_

#include "DDR_COM.h"
#include "DDR_iMX_UART.h"
#include "DDR_iMX_UART_cfg.h"
#include "cpu_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef UC3BOOT_CORE3
#define ID_UART             (4)
#define REG_UART            (REG_UART4)
#else
#define ID_UART             (2)
#define REG_UART            (REG_UART2)
#endif /* #define UC3BOOT_CORE3 */

#define CFG_BAUDRATE        (115200U)
#define CFG_BLEN            (BLEN8)
#define CFG_PAR             (PAR_NONE)
#define CFG_SBIT            (SBIT1)
#define CFG_FLW             (FLW_NONE)

#define DDR_UART_INIT_FN    (_ddr_imx_uart_init)

#define UART_PRINTF_BUF_SIZE (512U)

extern void UART_PRINTF(const char *fmt, ...);
extern void UART_PUTCHAR(char ch);
extern char UART_GETCHAR(void);

#ifdef __cplusplus
}
#endif
#endif /* SAMPLE_UART_CFG_H_ */
