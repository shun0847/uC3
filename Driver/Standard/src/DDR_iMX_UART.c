/**
 * @file    DDR_iMX_UART.c
 * @brief   Micro C Cube Standard, DEVICE DRIVER
 *          Serial Interface for Freescale i.MX6, i.MX7 and i.MX8 series
 * @date    2025.05.01
 * @author  Copyright (c) 2010-2025, eForce Co., Ltd. All rights reserved.
 *
 ****************************************************************************
 * @par     History
 *          - rev 1.0 (2010.11.04)
 *            Initial version.
 *          - rev 1.1 (2011.02.21)
 *            Modified for RVDS warning.
 *          - rev 1.2 (2013.08.16)
 *            Adopted for i.MX6DL UART
 *          - rev 1.3 (2016.05.13) y-kim
 *            Added UART6-8 for i.MX6UL.
 *            Moved clock initailization to hw_init.c.
 *          - rev 1.4 (2018.07.27) i-cho
 *            Adopted for i.MX7D UART.
 *            Change this file name.
 *            Interrupt number is set by CFG_INT_UARTn macro.
 *          - rev 1.5 (2018.10.04) i-cho
 *            Fix bug, and imporve statics analize.
 *          - rev 1.6 (2019.10.21) nozaki
 *            Fix bugs introduced when optimization is enabled.
 *          - rev 1.7 (2019.11.05) nozaki
 *            Set ONEMS register correctly.
 *          - rev 1.8 (2020.02.07) i-cho
 *            Added Reinitialization featrue.
 *            Fixed bug, TXEMPTY bit judgment in ddr_mx6uart_dis_send.
 *            Fixed bug, RXEMPTY bit judgment in ddr_mx6uart_dis_rcv.
 *          - rev 1.9 (2020.11.02) Imada
 *            Fixed C++test warnings
 *          - rev 2.0 (2020.11.19) Imada
 *            Added a busy loop to wait until the software reset status
 *            becomes inactive
 *            Added dly_tsk(0U) in _ddr_imx_uart_ini() as a workaround on 
 *            i.MX8M Plus
 *          - rev 2.1 (2021.04.09)
 *            Fixed bug, acre_isr judgment in _ddr_imx_uart_init.
 *          - rev 2.2 (2025.05.01)
 *            Fix C++Test warnings.
 ****************************************************************************
 */
#include <string.h>
#include "kernel.h"
#include "DDR_COM.h"
#include "DDR_iMX_UART.h"
#include "DDR_iMX_UART_cfg.h"

/* External function prototypes ----------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static void _ddr_imx_uart_intr(T_IMX_UART_MNG *pk_UARTmng);
static ER _ddr_imx_uartdrv(ID FuncID, VP pk_ControlData, T_IMX_UART_MNG *pk_UARTmng);

/* Private typedef ---------------------------------------------------------*/
/* Private macro ----------------------------------------------------------*/

#if 0
/******  DDR_iMX_UART_cfg.h の定義内容  *********************************/

/* UARTチャネルnの設定値 */

#define UART_n                  /* 未使用の場合はコメントアウト         */

#define TXBUF_SZn   1024        /* 送信バッファサイズ                   */
#define RXBUF_SZn   1024        /* 受信バッファサイズ                   */
#define XOFF_SZn    512         /* XOFF送出受信バッファデータ数トリガ   */
#define XON_SZn     128         /* XON送出受信バッファデータ数トリガ    */
                                /* レシーブFIFOデータ数トリガ           */
#define RXTL_n      24          /* 0〜32                                */
                                /* トランスミットFIFOデータ数トリガ     */
#define TXTL_n      8           /* 2〜32                                */
                                /* RTS出力アクティブトリガ              */
#define RTSTL_n     24          /* 0〜32                                */
#define IPL_UARTn   0xA0        /* 割込みレベル                         */
#define DCEDTE_n    0           /* DCE(0)/DTE(1)の選択                  */

/******  DDR_iMX_UART_cfg.h の定義内容はここまで  ***********************/
#endif


/******  UART1 ************************************/
#ifdef UART_1
#ifdef RTSTL_1
#if (RTSTL_1 > 32U)
#error Invalid RTSTL Value
#endif
#else
#define RTSTL_1 24U
#endif

#ifdef RXTL_1
#if (RXTL_1 > 32)
#error Invalid RXTL Value
#endif
#else
#define RXTL_1 24U
#endif

#ifdef TXTL_1
#if ((TXTL_1 < 2U) || (TXTL_1 > 32U))
#error Invalid TXTL Value
#endif
#else
#define TXTL_1 8U
#endif

#ifndef TXBUF_SZ1
#define TXBUF_SZ1 1024U
#endif

#ifndef RXBUF_SZ1
#define RXBUF_SZ1 1024U
#endif

#ifndef XOFF_SZ1
#define XOFF_SZ1 (RXBUF_SZ1 / 2U)
#endif

#ifndef XON_SZ1
#define XON_SZ1 (RXBUF_SZ1 / 8U)
#endif

#ifndef DCEDTE_1
#define DCEDTE_1 0U
#endif

#if (TXBUF_SZ1 == 0U)
#define _ddr_imx_uart_tbuf1  0
#else
VB  _ddr_imx_uart_tbuf1[TXBUF_SZ1];
#endif
VB  _ddr_imx_uart_rbuf1[RXBUF_SZ1];
UB  _ddr_imx_uart_sbuf1[RXBUF_SZ1];
T_IMX_UART_MNG _ddr_imx_uart_data1;

T_CDEV const _ddr_imx_uart_cdev1 = {&_ddr_imx_uart_data1, (FP)_ddr_imx_uartdrv};
T_CFLG const _ddr_imx_uart_cflg1 = {TA_TFIFO|TA_WMUL, 0x00000000};
T_CISR const _ddr_imx_uart_cisr1 = {TA_HLNG, (VP_INT)&_ddr_imx_uart_data1, CFG_INT_UART1, (FP)_ddr_imx_uart_intr, IPL_UART1};
#endif

/******  UART2 ************************************/
#ifdef UART_2
#ifdef RTSTL_2
#if (RTSTL_2 > 32U)
#error Invalid RTSTL Value
#endif
#else
#define RTSTL_2 24U
#endif

#ifdef RXTL_2
#if (RXTL_2 > 32U)
#error Invalid RXTL Value
#endif
#else
#define RXTL_2 24U
#endif

#ifdef TXTL_2
#if ((TXTL_2 < 2U) || (TXTL_2 > 32U))
#error Invalid TXTL Value
#endif
#else
#define TXTL_2 8U
#endif

#ifndef TXBUF_SZ2
#define TXBUF_SZ2 1024U
#endif

#ifndef RXBUF_SZ2
#define RXBUF_SZ2 1024U
#endif

#ifndef XOFF_SZ2
#define XOFF_SZ2 (RXBUF_SZ2 / 2U)
#endif

#ifndef XON_SZ2
#define XON_SZ2 (RXBUF_SZ2 / 8U)
#endif

#ifndef DCEDTE_2
#define DCEDTE_2 0U
#endif

#if (TXBUF_SZ2 == 0U)
#define _ddr_imx_uart_tbuf2  0
#else
VB  _ddr_imx_uart_tbuf2[TXBUF_SZ2];
#endif
VB  _ddr_imx_uart_rbuf2[RXBUF_SZ2];
UB  _ddr_imx_uart_sbuf2[RXBUF_SZ2];
T_IMX_UART_MNG _ddr_imx_uart_data2;

T_CDEV const _ddr_imx_uart_cdev2 = {&_ddr_imx_uart_data2, (FP)_ddr_imx_uartdrv};
T_CFLG const _ddr_imx_uart_cflg2 = {TA_TFIFO|TA_WMUL, 0x00000000};
T_CISR const _ddr_imx_uart_cisr2 = {TA_HLNG, (VP_INT)&_ddr_imx_uart_data2, CFG_INT_UART2, (FP)_ddr_imx_uart_intr, IPL_UART2};
#endif

/******  UART3 ************************************/
#ifdef UART_3
#ifdef RTSTL_3
#if (RTSTL_3 > 32U)
#error Invalid RTSTL Value
#endif
#else
#define RTSTL_3 24U
#endif

#ifdef RXTL_3
#if (RXTL_3 > 32U)
#error Invalid RXTL Value
#endif
#else
#define RXTL_3 24U
#endif

#ifdef TXTL_3
#if ((TXTL_3 < 2U) || (TXTL_3 > 32U))
#error Invalid TXTL Value
#endif
#else
#define TXTL_3 8U
#endif

#ifndef TXBUF_SZ3
#define TXBUF_SZ3 1024U
#endif

#ifndef RXBUF_SZ3
#define RXBUF_SZ3 1024U
#endif

#ifndef XOFF_SZ3
#define XOFF_SZ3 (RXBUF_SZ3 / 2U)
#endif

#ifndef XON_SZ3
#define XON_SZ3 (RXBUF_SZ3 / 8U)
#endif

#ifndef DCEDTE_3
#define DCEDTE_3 0U
#endif

#if (TXBUF_SZ3 == 0U)
#define _ddr_imx_uart_tbuf3  0
#else
VB  _ddr_imx_uart_tbuf3[TXBUF_SZ3];
#endif
VB  _ddr_imx_uart_rbuf3[RXBUF_SZ3];
UB  _ddr_imx_uart_sbuf3[RXBUF_SZ3];
T_IMX_UART_MNG _ddr_imx_uart_data3;

T_CDEV const _ddr_imx_uart_cdev3 = {&_ddr_imx_uart_data3, (FP)_ddr_imx_uartdrv};
T_CFLG const _ddr_imx_uart_cflg3 = {TA_TFIFO|TA_WMUL, 0x00000000};
T_CISR const _ddr_imx_uart_cisr3 = {TA_HLNG, (VP_INT)&_ddr_imx_uart_data3, CFG_INT_UART3, (FP)_ddr_imx_uart_intr, IPL_UART3};
#endif

/******  UART4 ***********************************/
#ifdef UART_4
#ifdef RTSTL_4
#if (RTSTL_4 > 32U)
#error Invalid RTSTL Value
#endif
#else
#define RTSTL_4 24U
#endif

#ifdef RXTL_4
#if (RXTL_4 > 32U)
#error Invalid RXTL Value
#endif
#else
#define RXTL_4 24U
#endif

#ifdef TXTL_4
#if ((TXTL_4 < 2U) || (TXTL_4 > 32U))
#error Invalid TXTL Value
#endif
#else
#define TXTL_4 8U
#endif

#ifndef TXBUF_SZ4
#define TXBUF_SZ4 1024U
#endif

#ifndef RXBUF_SZ4
#define RXBUF_SZ4 1024U
#endif

#ifndef XOFF_SZ4
#define XOFF_SZ4 (RXBUF_SZ4 / 2U)
#endif

#ifndef XON_SZ4
#define XON_SZ4 (RXBUF_SZ4 / 8U)
#endif

#ifndef DCEDTE_4
#define DCEDTE_4 0U
#endif

#if (TXBUF_SZ4 == 0U)
#define _ddr_imx_uart_tbuf4  0
#else
VB  _ddr_imx_uart_tbuf4[TXBUF_SZ4];
#endif
VB  _ddr_imx_uart_rbuf4[RXBUF_SZ4];
UB  _ddr_imx_uart_sbuf4[RXBUF_SZ4];
T_IMX_UART_MNG _ddr_imx_uart_data4;

T_CDEV const _ddr_imx_uart_cdev4 = {&_ddr_imx_uart_data4, (FP)_ddr_imx_uartdrv};
T_CFLG const _ddr_imx_uart_cflg4 = {TA_TFIFO|TA_WMUL, 0x00000000};
T_CISR const _ddr_imx_uart_cisr4 = {TA_HLNG, (VP_INT)&_ddr_imx_uart_data4, CFG_INT_UART4, (FP)_ddr_imx_uart_intr, IPL_UART4};
#endif

/******  UART5 ************************************/
#ifdef UART_5
#ifdef RTSTL_5
#if (RTSTL_5 > 32U)
#error Invalid RTSTL Value
#endif
#else
#define RTSTL_5 24U
#endif

#ifdef RXTL_5
#if (RXTL_5 > 32U)
#error Invalid RXTL Value
#endif
#else
#define RXTL_5 24U
#endif

#ifdef TXTL_5
#if ((TXTL_5 < 2U) || (TXTL_5 > 32U))
#error Invalid TXTL Value
#endif
#else
#define TXTL_5 8U
#endif

#ifndef TXBUF_SZ5
#define TXBUF_SZ5 1024U
#endif

#ifndef RXBUF_SZ5
#define RXBUF_SZ5 1024U
#endif

#ifndef XOFF_SZ5
#define XOFF_SZ5 (RXBUF_SZ5 / 2U)
#endif

#ifndef XON_SZ5
#define XON_SZ5 (RXBUF_SZ5 / 8U)
#endif

#ifndef DCEDTE_5
#define DCEDTE_5 0U
#endif

#if (TXBUF_SZ5 == 0U)
#define _ddr_imx_uart_tbuf5  0
#else
VB  _ddr_imx_uart_tbuf5[TXBUF_SZ5];
#endif
VB  _ddr_imx_uart_rbuf5[RXBUF_SZ5];
UB  _ddr_imx_uart_sbuf5[RXBUF_SZ5];
T_IMX_UART_MNG _ddr_imx_uart_data5;

T_CDEV const _ddr_imx_uart_cdev5 = {&_ddr_imx_uart_data5, (FP)_ddr_imx_uartdrv};
T_CFLG const _ddr_imx_uart_cflg5 = {TA_TFIFO|TA_WMUL, 0x00000000};
T_CISR const _ddr_imx_uart_cisr5 = {TA_HLNG, (VP_INT)&_ddr_imx_uart_data5, CFG_INT_UART5, (FP)_ddr_imx_uart_intr, IPL_UART5};
#endif

/******  UART6 ************************************/
#ifdef UART_6
#ifdef RTSTL_6
#if (RTSTL_6 > 32U)
#error Invalid RTSTL Value
#endif
#else
#define RTSTL_6 24U
#endif

#ifdef RXTL_6
#if (RXTL_6 > 32U)
#error Invalid RXTL Value
#endif
#else
#define RXTL_6 24U
#endif

#ifdef TXTL_6
#if ((TXTL_6 < 2U) || (TXTL_6 > 32U))
#error Invalid TXTL Value
#endif
#else
#define TXTL_6 8U
#endif

#ifndef TXBUF_SZ6
#define TXBUF_SZ6 1024U
#endif

#ifndef RXBUF_SZ6
#define RXBUF_SZ6 1024U
#endif

#ifndef XOFF_SZ6
#define XOFF_SZ6 (RXBUF_SZ6 / 2U)
#endif

#ifndef XON_SZ6
#define XON_SZ6 (RXBUF_SZ6 / 8U)
#endif

#ifndef DCEDTE_6
#define DCEDTE_6 0U
#endif

#if (TXBUF_SZ6 == 0U)
#define _ddr_imx_uart_tbuf6  0
#else
VB  _ddr_imx_uart_tbuf6[TXBUF_SZ6];
#endif
VB  _ddr_imx_uart_rbuf6[RXBUF_SZ6];
UB  _ddr_imx_uart_sbuf6[RXBUF_SZ6];
T_IMX_UART_MNG _ddr_imx_uart_data6;

T_CDEV const _ddr_imx_uart_cdev6 = {&_ddr_imx_uart_data6, (FP)_ddr_imx_uartdrv};
T_CFLG const _ddr_imx_uart_cflg6 = {TA_TFIFO|TA_WMUL, 0x00000000};
T_CISR const _ddr_imx_uart_cisr6 = {TA_HLNG, (VP_INT)&_ddr_imx_uart_data6, CFG_INT_UART6, (FP)_ddr_imx_uart_intr, IPL_UART6};
#endif

/******  UART7 ************************************/
#ifdef UART_7
#ifdef RTSTL_7
#if (RTSTL_7 > 32U)
#error Invalid RTSTL Value
#endif
#else
#define RTSTL_7 24U
#endif

#ifdef RXTL_7
#if (RXTL_7 > 32U)
#error Invalid RXTL Value
#endif
#else
#define RXTL_7 24U
#endif

#ifdef TXTL_7
#if ((TXTL_7 < 2U) || (TXTL_7 > 32U))
#error Invalid TXTL Value
#endif
#else
#define TXTL_7 8U
#endif

#ifndef TXBUF_SZ7
#define TXBUF_SZ7 1024U
#endif

#ifndef RXBUF_SZ7
#define RXBUF_SZ7 1024U
#endif

#ifndef XOFF_SZ7
#define XOFF_SZ7 (RXBUF_SZ7 / 2U)
#endif

#ifndef XON_SZ7
#define XON_SZ7 (RXBUF_SZ7 / 8U)
#endif

#ifndef DCEDTE_7
#define DCEDTE_7 0U
#endif

#if (TXBUF_SZ7 == 0U)
#define _ddr_imx_uart_tbuf7  0
#else
VB  _ddr_imx_uart_tbuf7[TXBUF_SZ7];
#endif
VB  _ddr_imx_uart_rbuf7[RXBUF_SZ7];
UB  _ddr_imx_uart_sbuf7[RXBUF_SZ7];
T_IMX_UART_MNG _ddr_imx_uart_data7;

T_CDEV const _ddr_imx_uart_cdev7 = {&_ddr_imx_uart_data7, (FP)_ddr_imx_uartdrv};
T_CFLG const _ddr_imx_uart_cflg7 = {TA_TFIFO|TA_WMUL, 0x00000000};
T_CISR const _ddr_imx_uart_cisr7 = {TA_HLNG, (VP_INT)&_ddr_imx_uart_data7, CFG_INT_UART7, (FP)_ddr_imx_uart_intr, IPL_UART7};
#endif

/******  UART8 ************************************/
#ifdef UART_8
#ifdef RTSTL_8
#if (RTSTL_8 > 32U)
#error Invalid RTSTL Value
#endif
#else
#define RTSTL_8 24U
#endif

#ifdef RXTL_8
#if (RXTL_8 > 32U)
#error Invalid RXTL Value
#endif
#else
#define RXTL_8 24U
#endif

#ifdef TXTL_8
#if ((TXTL_8 < 2U) || (TXTL_8 > 32U))
#error Invalid TXTL Value
#endif
#else
#define TXTL_8 8U
#endif

#ifndef TXBUF_SZ8
#define TXBUF_SZ8 1024U
#endif

#ifndef RXBUF_SZ8
#define RXBUF_SZ8 1024U
#endif

#ifndef XOFF_SZ8
#define XOFF_SZ8 (RXBUF_SZ8 / 2U)
#endif

#ifndef XON_SZ8
#define XON_SZ8 (RXBUF_SZ8 / 8U)
#endif

#ifndef DCEDTE_8
#define DCEDTE_8 0U
#endif

#if (TXBUF_SZ8 == 0U)
#define _ddr_imx_uart_tbuf8  0
#else
VB  _ddr_imx_uart_tbuf8[TXBUF_SZ8];
#endif
VB  _ddr_imx_uart_rbuf8[RXBUF_SZ8];
UB  _ddr_imx_uart_sbuf8[RXBUF_SZ8];
T_IMX_UART_MNG _ddr_imx_uart_data8;

T_CDEV const _ddr_imx_uart_cdev8 = {&_ddr_imx_uart_data8, (FP)_ddr_imx_uartdrv};
T_CFLG const _ddr_imx_uart_cflg8 = {TA_TFIFO|TA_WMUL, 0x00000000};
T_CISR const _ddr_imx_uart_cisr8 = {TA_HLNG, (VP_INT)&_ddr_imx_uart_data8, CFG_INT_UART8, (FP)_ddr_imx_uart_intr, IPL_UART8};
#endif

/**
 *  _ddr_imx_uart_init
 *
 *  @param[in]  devid device id
 *  @param[in]  uart_port  portno
 *  @retrun error code(E_OK, E_PAR, E_ID, E_OBJ)
 */
ER _ddr_imx_uart_init(ID devid, volatile struct t_uart *uart_port)
{
    ER ercd;

#ifdef UART_1
    if (uart_port == &REG_UART1) {
        memset(&_ddr_imx_uart_data1, 0x00, sizeof(_ddr_imx_uart_data1));
        _ddr_imx_uart_data1.port = &REG_UART1;
        _ddr_imx_uart_data1.tbuf = _ddr_imx_uart_tbuf1;
        _ddr_imx_uart_data1.rbuf = _ddr_imx_uart_rbuf1;
        _ddr_imx_uart_data1.sbuf = _ddr_imx_uart_sbuf1;
        _ddr_imx_uart_data1.tsize = TXBUF_SZ1;
        _ddr_imx_uart_data1.rsize = RXBUF_SZ1;
        _ddr_imx_uart_data1.xoff_size = XOFF_SZ1;
        _ddr_imx_uart_data1.xon_size = XON_SZ1;
        _ddr_imx_uart_data1.aux[0] = (UH)((UH)TXTL_1 << 10) | (UH)((UH)DCEDTE_1 << 6) | RXTL_1;
        _ddr_imx_uart_data1.aux[1] = (UH)((UH)RTSTL_1 << 10);
        _ddr_imx_uart_data1.devhdr = (FP)_ddr_imx_uartdrv;
        ercd = acre_flg((T_CFLG *)&_ddr_imx_uart_cflg1);
        if (ercd > E_OK) {
            _ddr_imx_uart_data1.flgid = (UH)ercd;
            ercd = acre_isr((T_CISR *)&_ddr_imx_uart_cisr1);
            if (ercd > E_OK) {
                (void)dis_int(CFG_INT_UART1);
                _ddr_imx_uart_data1.intno = CFG_INT_UART1;
                REG_UART1.UCR2 = 0U;
                while ((REG_UART1.UTS & 0x1UL)) {
                    ; /* Wait until the software reset status becomes incactive */
                }
                _ddr_imx_uart_data1.isrid = (UH)ercd;
                ercd = vdef_dev(devid, (VP)&_ddr_imx_uart_cdev1);
                (void)ena_int(CFG_INT_UART1);
            } else {
                (void)del_flg((ID)_ddr_imx_uart_data1.flgid);
            }
        }
    } else
#endif

#ifdef UART_2
    if (uart_port == &REG_UART2) {
        memset(&_ddr_imx_uart_data2, 0x00, sizeof(_ddr_imx_uart_data2));
        _ddr_imx_uart_data2.port = &REG_UART2;
        _ddr_imx_uart_data2.tbuf = _ddr_imx_uart_tbuf2;
        _ddr_imx_uart_data2.rbuf = _ddr_imx_uart_rbuf2;
        _ddr_imx_uart_data2.sbuf = _ddr_imx_uart_sbuf2;
        _ddr_imx_uart_data2.tsize = TXBUF_SZ2;
        _ddr_imx_uart_data2.rsize = RXBUF_SZ2;
        _ddr_imx_uart_data2.xoff_size = XOFF_SZ2;
        _ddr_imx_uart_data2.xon_size = XON_SZ2;
        _ddr_imx_uart_data2.aux[0] = (UH)((UH)TXTL_2 << 10) | (UH)((UH)DCEDTE_2 << 6) | RXTL_2;
        _ddr_imx_uart_data2.aux[1] = (UH)((UH)RTSTL_2 << 10);
        _ddr_imx_uart_data2.devhdr = (FP)_ddr_imx_uartdrv;
        ercd = acre_flg((T_CFLG *)&_ddr_imx_uart_cflg2);
        if (ercd > E_OK) {
            _ddr_imx_uart_data2.flgid = (UH)ercd;
            ercd = acre_isr((T_CISR *)&_ddr_imx_uart_cisr2);
            if (ercd > E_OK) {
                (void)dis_int(CFG_INT_UART2);
                _ddr_imx_uart_data2.intno = CFG_INT_UART2;
                REG_UART2.UCR2 = 0U;
                while ((REG_UART2.UTS & 0x1UL)) {
                    ; /* Wait until the software reset status becomes incactive */
                }
                _ddr_imx_uart_data2.isrid = (UH)ercd;
                ercd = vdef_dev(devid, (VP)&_ddr_imx_uart_cdev2);
                (void)ena_int(CFG_INT_UART2);
            } else {
                (void)del_flg((ID)_ddr_imx_uart_data2.flgid);
            }
        }
    } else
#endif

#ifdef UART_3
    if (uart_port == &REG_UART3) {
        memset(&_ddr_imx_uart_data3, 0x00, sizeof(_ddr_imx_uart_data3));
        _ddr_imx_uart_data3.port = &REG_UART3;
        _ddr_imx_uart_data3.tbuf = _ddr_imx_uart_tbuf3;
        _ddr_imx_uart_data3.rbuf = _ddr_imx_uart_rbuf3;
        _ddr_imx_uart_data3.sbuf = _ddr_imx_uart_sbuf3;
        _ddr_imx_uart_data3.tsize = TXBUF_SZ3;
        _ddr_imx_uart_data3.rsize = RXBUF_SZ3;
        _ddr_imx_uart_data3.xoff_size = XOFF_SZ3;
        _ddr_imx_uart_data3.xon_size = XON_SZ3;
        _ddr_imx_uart_data3.aux[0] = (UH)((UH)TXTL_3 << 10) | (UH)((UH)DCEDTE_3 << 6) | RXTL_3;
        _ddr_imx_uart_data3.aux[1] = (UH)((UH)RTSTL_3 << 10);
        _ddr_imx_uart_data3.devhdr = (FP)_ddr_imx_uartdrv;
        ercd = acre_flg((T_CFLG *)&_ddr_imx_uart_cflg3);
        if (ercd > E_OK) {
            _ddr_imx_uart_data3.flgid = (UH)ercd;
            ercd = acre_isr((T_CISR *)&_ddr_imx_uart_cisr3);
            if (ercd > E_OK) {
                (void)dis_int(CFG_INT_UART3);
                _ddr_imx_uart_data3.intno = CFG_INT_UART3;
                REG_UART3.UCR2 = 0U;
                while ((REG_UART3.UTS & 0x1UL)) {
                    ; /* Wait until the software reset status becomes incactive */
                }
                _ddr_imx_uart_data3.isrid = (UH)ercd;
                ercd = vdef_dev(devid, (VP)&_ddr_imx_uart_cdev3);
                (void)ena_int(CFG_INT_UART3);
            } else {
                (void)del_flg((ID)_ddr_imx_uart_data3.flgid);
            }
        }
    } else
#endif

#ifdef UART_4
    if (uart_port == &REG_UART4) {
        memset(&_ddr_imx_uart_data4, 0x00, sizeof(_ddr_imx_uart_data4));
        _ddr_imx_uart_data4.port = &REG_UART4;
        _ddr_imx_uart_data4.tbuf = _ddr_imx_uart_tbuf4;
        _ddr_imx_uart_data4.rbuf = _ddr_imx_uart_rbuf4;
        _ddr_imx_uart_data4.sbuf = _ddr_imx_uart_sbuf4;
        _ddr_imx_uart_data4.tsize = TXBUF_SZ4;
        _ddr_imx_uart_data4.rsize = RXBUF_SZ4;
        _ddr_imx_uart_data4.xoff_size = XOFF_SZ4;
        _ddr_imx_uart_data4.xon_size = XON_SZ4;
        _ddr_imx_uart_data4.aux[0] = (UH)((UH)TXTL_4 << 10) | (UH)((UH)DCEDTE_4 << 6) | RXTL_4;
        _ddr_imx_uart_data4.aux[1] = (UH)((UH)RTSTL_4 << 10);
        _ddr_imx_uart_data4.devhdr = (FP)_ddr_imx_uartdrv;
        ercd = acre_flg((T_CFLG *)&_ddr_imx_uart_cflg4);
        if (ercd > E_OK) {
            _ddr_imx_uart_data4.flgid = (UH)ercd;
            ercd = acre_isr((T_CISR *)&_ddr_imx_uart_cisr4);
            if (ercd > E_OK) {
                (void)dis_int(CFG_INT_UART4);
                _ddr_imx_uart_data4.intno = CFG_INT_UART4;
                REG_UART4.UCR2 = 0U;
                while ((REG_UART4.UTS & 0x1UL)) {
                    ; /* Wait until the software reset status becomes incactive */
                }
                _ddr_imx_uart_data4.isrid = (UH)ercd;
                ercd = vdef_dev(devid, (VP)&_ddr_imx_uart_cdev4);
                (void)ena_int(CFG_INT_UART4);
            } else {
                (void)del_flg((ID)_ddr_imx_uart_data4.flgid);
            }
        }
    } else
#endif

#ifdef UART_5
    if (uart_port == &REG_UART5) {
        memset(&_ddr_imx_uart_data5, 0x00, sizeof(_ddr_imx_uart_data5));
        _ddr_imx_uart_data5.port = &REG_UART5;
        _ddr_imx_uart_data5.tbuf = _ddr_imx_uart_tbuf5;
        _ddr_imx_uart_data5.rbuf = _ddr_imx_uart_rbuf5;
        _ddr_imx_uart_data5.sbuf = _ddr_imx_uart_sbuf5;
        _ddr_imx_uart_data5.tsize = TXBUF_SZ5;
        _ddr_imx_uart_data5.rsize = RXBUF_SZ5;
        _ddr_imx_uart_data5.xoff_size = XOFF_SZ5;
        _ddr_imx_uart_data5.xon_size = XON_SZ5;
        _ddr_imx_uart_data5.aux[0] = (UH)((UH)TXTL_5 << 10) | (UH)((UH)DCEDTE_5 << 6) | RXTL_5;
        _ddr_imx_uart_data5.aux[1] = (UH)((UH)RTSTL_5 << 10);
        _ddr_imx_uart_data5.devhdr = (FP)_ddr_imx_uartdrv;
        ercd = acre_flg((T_CFLG *)&_ddr_imx_uart_cflg5);
        if (ercd > E_OK) {
            _ddr_imx_uart_data5.flgid = (UH)ercd;
            ercd = acre_isr((T_CISR *)&_ddr_imx_uart_cisr5);
            if (ercd > E_OK) {
                (void)dis_int(CFG_INT_UART5);
                _ddr_imx_uart_data5.intno = CFG_INT_UART5;
                REG_UART5.UCR2 = 0U;
                while ((REG_UART5.UTS & 0x1UL)) {
                    ; /* Wait until the software reset status becomes incactive */
                }
                _ddr_imx_uart_data5.isrid = (UH)ercd;
                ercd = vdef_dev(devid, (VP)&_ddr_imx_uart_cdev5);
                (void)ena_int(CFG_INT_UART5);
            } else {
                (void)del_flg((ID)_ddr_imx_uart_data5.flgid);
            }
        }
    } else
#endif

#ifdef UART_6
    if (uart_port == &REG_UART6) {
        memset(&_ddr_imx_uart_data6, 0x00, sizeof(_ddr_imx_uart_data6));
        _ddr_imx_uart_data6.port = &REG_UART6;
        _ddr_imx_uart_data6.tbuf = _ddr_imx_uart_tbuf6;
        _ddr_imx_uart_data6.rbuf = _ddr_imx_uart_rbuf6;
        _ddr_imx_uart_data6.sbuf = _ddr_imx_uart_sbuf6;
        _ddr_imx_uart_data6.tsize = TXBUF_SZ6;
        _ddr_imx_uart_data6.rsize = RXBUF_SZ6;
        _ddr_imx_uart_data6.xoff_size = XOFF_SZ6;
        _ddr_imx_uart_data6.xon_size = XON_SZ6;
        _ddr_imx_uart_data6.aux[0] = (UH)((UH)TXTL_6 << 10) | (UH)((UH)DCEDTE_6 << 6) | RXTL_6;
        _ddr_imx_uart_data6.aux[1] = (UH)((UH)RTSTL_6 << 10);
        _ddr_imx_uart_data6.devhdr = (FP)_ddr_imx_uartdrv;
        ercd = acre_flg((T_CFLG *)&_ddr_imx_uart_cflg6);
        if (ercd > E_OK) {
            _ddr_imx_uart_data6.flgid = (UH)ercd;
            ercd = acre_isr((T_CISR *)&_ddr_imx_uart_cisr6);
            if (ercd > E_OK) {
                (void)dis_int(CFG_INT_UART6);
                _ddr_imx_uart_data6.intno = CFG_INT_UART6;
                REG_UART6.UCR2 = 0U;
                while ((REG_UART6.UTS & 0x1UL)) {
                    ; /* Wait until the software reset status becomes incactive */
                }
                _ddr_imx_uart_data6.isrid = (UH)ercd;
                ercd = vdef_dev(devid, (VP)&_ddr_imx_uart_cdev6);
                (void)ena_int(CFG_INT_UART6);
            } else {
                (void)del_flg((ID)_ddr_imx_uart_data6.flgid);
            }
        }
    } else
#endif

#ifdef UART_7
    if (uart_port == &REG_UART7) {
        memset(&_ddr_imx_uart_data7, 0x00, sizeof(_ddr_imx_uart_data7));
        _ddr_imx_uart_data7.port = &REG_UART7;
        _ddr_imx_uart_data7.tbuf = _ddr_imx_uart_tbuf7;
        _ddr_imx_uart_data7.rbuf = _ddr_imx_uart_rbuf7;
        _ddr_imx_uart_data7.sbuf = _ddr_imx_uart_sbuf7;
        _ddr_imx_uart_data7.tsize = TXBUF_SZ7;
        _ddr_imx_uart_data7.rsize = RXBUF_SZ7;
        _ddr_imx_uart_data7.xoff_size = XOFF_SZ7;
        _ddr_imx_uart_data7.xon_size = XON_SZ7;
        _ddr_imx_uart_data7.aux[0] = (UH)((UH)TXTL_7 << 10) | (UH)((UH)DCEDTE_7 << 6) | RXTL_7;
        _ddr_imx_uart_data7.aux[1] = (UH)((UH)RTSTL_7 << 10);
        _ddr_imx_uart_data7.devhdr = (FP)_ddr_imx_uartdrv;
        ercd = acre_flg((T_CFLG *)&_ddr_imx_uart_cflg7);
        if (ercd > E_OK) {
            _ddr_imx_uart_data7.flgid = (UH)ercd;
            ercd = acre_isr((T_CISR *)&_ddr_imx_uart_cisr7);
            if (ercd > E_OK) {
                (void)dis_int(CFG_INT_UART7);
                _ddr_imx_uart_data7.intno = CFG_INT_UART7;
                REG_UART7.UCR2 = 0U;
                while ((REG_UART7.UTS & 0x1UL)) {
                    ; /* Wait until the software reset status becomes incactive */
                }
                _ddr_imx_uart_data7.isrid = (UH)ercd;
                ercd = vdef_dev(devid, (VP)&_ddr_imx_uart_cdev7);
                (void)ena_int(CFG_INT_UART7);
            } else {
                (void)del_flg((ID)_ddr_imx_uart_data7.flgid);
            }
        }
    } else
#endif

#ifdef UART_8
    if (uart_port == &REG_UART8) {
        memset(&_ddr_imx_uart_data8, 0x00, sizeof(_ddr_imx_uart_data8));
        _ddr_imx_uart_data8.port = &REG_UART8;
        _ddr_imx_uart_data8.tbuf = _ddr_imx_uart_tbuf8;
        _ddr_imx_uart_data8.rbuf = _ddr_imx_uart_rbuf8;
        _ddr_imx_uart_data8.sbuf = _ddr_imx_uart_sbuf8;
        _ddr_imx_uart_data8.tsize = TXBUF_SZ8;
        _ddr_imx_uart_data8.rsize = RXBUF_SZ8;
        _ddr_imx_uart_data8.xoff_size = XOFF_SZ8;
        _ddr_imx_uart_data8.xon_size = XON_SZ8;
        _ddr_imx_uart_data8.aux[0] = (UH)((UH)TXTL_8 << 10) | (UH)((UH)DCEDTE_8 << 6) | RXTL_8;
        _ddr_imx_uart_data8.aux[1] = (UH)((UH)RTSTL_8 << 10);
        _ddr_imx_uart_data8.devhdr = (FP)_ddr_imx_uartdrv;
        ercd = acre_flg((T_CFLG *)&_ddr_imx_uart_cflg8);
        if (ercd > E_OK) {
            _ddr_imx_uart_data8.flgid = (UH)ercd;
            ercd = acre_isr((T_CISR *)&_ddr_imx_uart_cisr8);
            if (ercd > E_OK) {
                (void)dis_int(CFG_INT_UART8);
                _ddr_imx_uart_data8.intno = CFG_INT_UART8;
                REG_UART8.UCR2 = 0U;
                while ((REG_UART8.UTS & 0x1UL)) {
                    ; /* Wait until the software reset status becomes incactive */
                }
                _ddr_imx_uart_data8.isrid = (UH)ercd;
                ercd = vdef_dev(devid, (VP)&_ddr_imx_uart_cdev8);
                (void)ena_int(CFG_INT_UART8);
            } else {
                (void)del_flg((ID)_ddr_imx_uart_data8.flgid);
            }
        }
    } else
#endif
        ercd = E_NOEXS;
    return ercd;
}

/**
 *  _ddr_imx_uart_check_chr
 *
 *  @param[in]  ReceiveData  recv data
 *  @param[in]  chr  character
 *  @param[in]  sts  status
 *  @retrun TRUE/FALSE
 */
BOOL _ddr_imx_uart_check_chr(T_COM_RCV *ReceiveData, VB chr, UB sts)
{
    BOOL flag = FALSE;

    if ((sts & (T_COM_EROR | T_COM_ERP | T_COM_ERF | T_COM_BRK)) != 0U)
        flag = TRUE;
    else if (ReceiveData->rcnt == 0U)
        flag = TRUE;
    else if (ReceiveData->eos != (T_COM_EOS *)0U) {
        if ((ReceiveData->eos->flg[0] != 0U) && (ReceiveData->eos->chr[0] == chr))
            flag = TRUE;
        else if ((ReceiveData->eos->flg[1] != 0U) && (ReceiveData->eos->chr[1] == chr))
            flag = TRUE;
        else if ((ReceiveData->eos->flg[2] != 0U) && (ReceiveData->eos->chr[2] == chr))
            flag = TRUE;
        else if ((ReceiveData->eos->flg[3] != 0U) && (ReceiveData->eos->chr[3] == chr))
            flag = TRUE;
        else {
            /* Do Nothing */
        }
    } else {
        /* Do Nothing */
    }
    return flag;
}

/**
 *  _ddr_imx_uart_getssr
 *
 *  @param[in]  ssr  status register
 */
UB _ddr_imx_uart_getssr(UB ssr)
{
    UB sts = 0U;

    if ((ssr & URXD_PRERR) != 0U)
        sts |= T_COM_ERP;
    if ((ssr & URXD_BRK) != 0U)
        sts |= T_COM_BRK;
    if ((ssr & URXD_FRMERR) != 0U)
        sts |= T_COM_ERF;
    if ((ssr & URXD_OVERRUN) != 0U)
        sts |= T_COM_EROR;
    return sts;
}

/**
 *  _ddr_imx_uart_copy
 *
 *  @param[in]  pk_UARTmng    uart manager
 *  @param[in]  TransmiteData   send data
 *  @retrun TRUE/FALSE
 */
BOOL _ddr_imx_uart_copy(T_IMX_UART_MNG *pk_UARTmng, T_COM_SND *TransmiteData)
{
    while (TransmiteData->tcnt != 0U) {
        if (pk_UARTmng->tcnt < pk_UARTmng->tsize) {
            pk_UARTmng->tbuf[pk_UARTmng->sndp++] = *TransmiteData->tbuf++;
            TransmiteData->tcnt--;
            if (pk_UARTmng->sndp >= pk_UARTmng->tsize) {
                pk_UARTmng->sndp = 0U;
            }
            pk_UARTmng->tcnt++;
        } else {
            break;
        }
    }
    return (TransmiteData->tcnt == 0U) ? TRUE : FALSE;
}

/**
 *  _ddr_imx_uart_send_local_buf
 *
 *  @param[in]  pk_UARTmng    uart manager
 */
void _ddr_imx_uart_send_local_buf(T_IMX_UART_MNG *pk_UARTmng)
{
    while ((pk_UARTmng->port->UTS & UTS_TXFULL) == 0U) {
        pk_UARTmng->port->UTXD = *pk_UARTmng->SndData->tbuf++;
        if (--pk_UARTmng->SndData->tcnt == 0U) {
            pk_UARTmng->SndData = 0U;
            pk_UARTmng->port->UCR1 &= ~(UCR1_TRDYEN | UCR1_TXMPTYEN);
            (void)iset_flg((ID)pk_UARTmng->flgid, TXI_FLG);
            break;
        }
    }
}

/**
 *  _ddr_imx_uart_send_drv_buf
 *
 *  @param[in]  pk_UARTmng    uart manager
 */
void _ddr_imx_uart_send_drv_buf(T_IMX_UART_MNG *pk_UARTmng)
{
    UH sndp;

    sndp = pk_UARTmng->sndp - pk_UARTmng->tcnt;
    if (pk_UARTmng->tcnt > pk_UARTmng->sndp)
        sndp += pk_UARTmng->tsize;
    while ((pk_UARTmng->port->UTS & UTS_TXFULL) == 0U) {
        pk_UARTmng->port->UTXD = pk_UARTmng->tbuf[sndp];
        if (++sndp >= pk_UARTmng->tsize)
            sndp = 0U;
        if (--pk_UARTmng->tcnt == 0U)
            break;
    }
    if (pk_UARTmng->SndData != 0) {
        if (_ddr_imx_uart_copy(pk_UARTmng, pk_UARTmng->SndData)) {
            pk_UARTmng->SndData = 0;
            (void)iset_flg((ID)pk_UARTmng->flgid, TXI_FLG);
        }
    }
    if ((pk_UARTmng->tcnt == 0U) && (pk_UARTmng->SndData == 0)) {
        pk_UARTmng->port->UCR1 &= ~(UCR1_TRDYEN | UCR1_TXMPTYEN);
    }
}

/**
 *  _ddr_imx_uart_txi
 *
 *  @param[in]  pk_UARTmng    uart manager
 */
void _ddr_imx_uart_txi(T_IMX_UART_MNG *pk_UARTmng)
{
    if ((pk_UARTmng->port->UTS & UTS_TXFULL) == 0U) {
        if (pk_UARTmng->status.bit.req_xon_xoff == 1U) {
            pk_UARTmng->port->UTXD = (pk_UARTmng->status.bit.rx_xoff == 0U) ? (UW)XON : (UW)XOFF;
            pk_UARTmng->status.bit.req_xon_xoff = 0U;
        }
        if (pk_UARTmng->status.bit.tx_xoff == 0U) {
            if (pk_UARTmng->tsize == 0U) {
                if (pk_UARTmng->SndData != 0) {
                    _ddr_imx_uart_send_local_buf(pk_UARTmng);
                } else {
                    pk_UARTmng->port->UCR1 &= ~(UCR1_TRDYEN | UCR1_TXMPTYEN);
                    pk_UARTmng->port->UCR4 &= ~(UCR4_TCEN);
                }
            } else {
                if (pk_UARTmng->tcnt != 0U) {
                    _ddr_imx_uart_send_drv_buf(pk_UARTmng);
                } else {
                    pk_UARTmng->port->UCR1 &= ~(UCR1_TRDYEN | UCR1_TXMPTYEN);
                }
            }
        } else {
            pk_UARTmng->port->UCR1 &= ~(UCR1_TRDYEN | UCR1_TXMPTYEN);
            pk_UARTmng->port->UCR4 &= ~(UCR4_TCEN);
        }
    }
}

/**
 *  _ddr_imx_uart_chk_rxoff
 *
 *  @param[in]  pk_UARTmng    uart manager
 */
void _ddr_imx_uart_chk_rxoff(T_IMX_UART_MNG *pk_UARTmng)
{
    if ((pk_UARTmng->status.bit.rx_xoff == 0U) &&
        (pk_UARTmng->rcnt >= pk_UARTmng->xoff_size)) {
        pk_UARTmng->status.bit.rx_xoff = 1U;
        pk_UARTmng->status.bit.req_xon_xoff = 1U;
        pk_UARTmng->port->UCR1 |= (UW)(UCR1_TRDYEN | UCR1_TXMPTYEN);
        pk_UARTmng->port->UCR4 |= UCR4_TCEN;
        _ddr_imx_uart_txi(pk_UARTmng);
    }
}

/**
 *  _ddr_imx_uart_chk_rxon
 *
 *  @param[in]  pk_UARTmng    uart manager
 */
void _ddr_imx_uart_chk_rxon(T_IMX_UART_MNG *pk_UARTmng)
{
    if ((pk_UARTmng->status.bit.rx_xoff == 1U) &&
        (pk_UARTmng->rcnt <= pk_UARTmng->xon_size)) {
        pk_UARTmng->status.bit.rx_xoff = 0U;
        pk_UARTmng->status.bit.req_xon_xoff = 1U;
        pk_UARTmng->port->UCR1 |= (UW)(UCR1_TRDYEN | UCR1_TXMPTYEN);
        pk_UARTmng->port->UCR4 |= UCR4_TCEN;
        _ddr_imx_uart_txi(pk_UARTmng);
    }
}

/**
 *  _ddr_imx_uart_rxi
 *
 *  @param[in]  pk_UARTmng    uart manager
 */
void _ddr_imx_uart_rxi(T_IMX_UART_MNG *pk_UARTmng)
{
    UH rcvp;
    UW urxd;
    VB chr;
    UB sts;

    if ((pk_UARTmng->rcnt < pk_UARTmng->rsize) ||
        (pk_UARTmng->RcvData != 0)) {
        pk_UARTmng->port->USR1 = (UW)(USR1_PARITYERR | USR1_FRAMERR | USR1_AGTIM);
        pk_UARTmng->port->USR2 = (UW)(USR2_BRCD | USR2_ORE);
        while ((pk_UARTmng->port->UTS & UTS_RXEMPTY) == 0U) {
            urxd = pk_UARTmng->port->URXD;
            sts = _ddr_imx_uart_getssr((UB)(urxd >> 8));
            chr = (VB)(urxd & 0xFFU);
            if ((pk_UARTmng->status.bit.sft_flw == 1U) && (chr == XON)) {
                pk_UARTmng->status.bit.tx_xoff = 0U;
                pk_UARTmng->port->UCR1 |= (UW)(UCR1_TRDYEN | UCR1_TXMPTYEN);
                pk_UARTmng->port->UCR4 |= UCR4_TCEN;
                _ddr_imx_uart_txi(pk_UARTmng);
            } else if ((pk_UARTmng->status.bit.sft_flw == 1U) && (chr == XOFF)) {
                pk_UARTmng->status.bit.tx_xoff = 1U;
                pk_UARTmng->port->UCR1 &= ~(UCR1_TRDYEN | UCR1_TXMPTYEN);
            } else {
                if (pk_UARTmng->RcvData != 0) {
                    pk_UARTmng->RcvData->rcnt--;
                    *pk_UARTmng->RcvData->rbuf++ = chr;
                    if (pk_UARTmng->RcvData->sbuf != 0)
                        *pk_UARTmng->RcvData->sbuf++ = sts;
                    if (_ddr_imx_uart_check_chr(pk_UARTmng->RcvData, chr, sts)) {
                        pk_UARTmng->RcvData = 0;
                        (void)iset_flg((ID)pk_UARTmng->flgid, RXI_FLG);
                    }
                } else if (pk_UARTmng->rcnt < pk_UARTmng->rsize) {
                    rcvp = pk_UARTmng->rcvp + pk_UARTmng->rcnt;
                    if (rcvp >= pk_UARTmng->rsize)
                        rcvp -= pk_UARTmng->rsize;
                    pk_UARTmng->rbuf[rcvp] = chr;
                    if (pk_UARTmng->sbuf != (UB *)0U)
                        pk_UARTmng->sbuf[rcvp] = sts;
                    if (pk_UARTmng->status.bit.sft_flw == 1U)
                        _ddr_imx_uart_chk_rxoff(pk_UARTmng);
                    if (++pk_UARTmng->rcnt == pk_UARTmng->rsize) {
                        pk_UARTmng->port->UCR1 &= ~(UCR1_RRDYEN);
                        pk_UARTmng->port->UCR2 &= ~(UCR2_ATEN);
                        pk_UARTmng->port->UCR3 &= ~(UCR3_PARERREN | UCR3_FRAERREN);
                        pk_UARTmng->port->UCR4 &= ~(UCR4_BKEN | UCR4_OREN);
                        break;
                    }
                } else {
                	/* Do Nothing */
                }
            }
        }
    } else {
        pk_UARTmng->port->UCR1 &= ~(UCR1_RRDYEN);
        pk_UARTmng->port->UCR2 &= ~(UCR2_ATEN);
        pk_UARTmng->port->UCR3 &= ~(UCR3_PARERREN | UCR3_FRAERREN);
        pk_UARTmng->port->UCR4 &= ~(UCR4_BKEN | UCR4_OREN);
    }
}

/**
 *  _ddr_imx_uart_send_char
 *
 *  @param[in]  pk_UARTmng    uart manager
 *  @param[in]  TransmitData  send data
 */
void _ddr_imx_uart_send_char(T_IMX_UART_MNG *pk_UARTmng, T_COM_SND *TransmitData)
{
    if (pk_UARTmng->status.bit.req_xon_xoff == 1U) {
        pk_UARTmng->port->UTXD = (pk_UARTmng->status.bit.rx_xoff == 0U) ? (UW)XON : (UW)XOFF;
        pk_UARTmng->status.bit.req_xon_xoff = 0U;
    }
    if (pk_UARTmng->status.bit.tx_xoff == 0U) {
        while ((pk_UARTmng->port->UTS & UTS_TXFULL) == 0U) {
            if (TransmitData->tcnt > 0U) {
                pk_UARTmng->port->UTXD = *TransmitData->tbuf++;
                TransmitData->tcnt--;
            } else {
                break;
            }
        }
    }
}

/**
 *  _ddr_imx_uart_tei
 *
 *  @param[in]  pk_UARTmng    uart manager
 */
void _ddr_imx_uart_tei(T_IMX_UART_MNG *pk_UARTmng)
{
    if ((pk_UARTmng->tcnt == 0U) &&
        ((pk_UARTmng->port->USR2 & USR2_TXDC) != 0U)) {
        if (pk_UARTmng->wclnid != 0U) {
            pk_UARTmng->wclnid = 0U;
            iset_flg((ID)pk_UARTmng->flgid, TEI_FLG);
        }
        pk_UARTmng->port->UCR4 &= ~(UCR4_TCEN);
    }
}

/**
 *  _ddr_imx_uart_intr
 *
 *  @param[in]  pk_UARTmng    uart manager
 */
static void _ddr_imx_uart_intr(T_IMX_UART_MNG *pk_UARTmng)
{
    if ((pk_UARTmng->port->UCR1 & UCR1_RRDYEN) != 0U) {
        _ddr_imx_uart_rxi(pk_UARTmng);
    }
    if ((pk_UARTmng->port->UCR1 & (UCR1_TRDYEN|UCR1_TXMPTYEN)) != 0U) {
        _ddr_imx_uart_txi(pk_UARTmng);
    }
    if ((pk_UARTmng->port->UCR4 & UCR4_TCEN) != 0U) {
        _ddr_imx_uart_tei(pk_UARTmng);
    }
}

/**
 *  _ddr_imx_uart_recv_strings
 *
 *  @param[in]  pk_UARTmng    uart manager
 *  @param[in]  ReceiveData   receive data
 *  @return  TRUE/FALSE
 */
BOOL _ddr_imx_uart_recv_strings(T_IMX_UART_MNG *pk_UARTmng, T_COM_RCV *ReceiveData)
{
    BOOL flag;
    VB chr;
    UB sts;

    for (flag = FALSE; flag == FALSE; ) {
        if (ReceiveData->rcnt == 0U) {
            flag = TRUE;
        } else if (pk_UARTmng->rcnt == 0U) {
            break;
        } else {
            *ReceiveData->rbuf++ = chr = pk_UARTmng->rbuf[pk_UARTmng->rcvp];
            sts = pk_UARTmng->sbuf[pk_UARTmng->rcvp];
            if (ReceiveData->sbuf != 0) {
                *ReceiveData->sbuf++ = sts;
            }
            if (++pk_UARTmng->rcvp >= pk_UARTmng->rsize) {
                pk_UARTmng->rcvp = 0U;
            }
            pk_UARTmng->rcnt--;
            ReceiveData->rcnt--;
            if (pk_UARTmng->status.bit.sft_flw == 1U) {
                _ddr_imx_uart_chk_rxon(pk_UARTmng);
            }
            flag = _ddr_imx_uart_check_chr(ReceiveData, chr, sts);
        }
    }
    return flag;
}

/**
 *  _ddr_imx_uart_snd
 *
 *  @param[in]  ReceiveData   receive data
 *  @param[in]  pk_UARTmng    uart manager
 *  @return E_OK,
 *          E_OBJ
 */
ER _ddr_imx_uart_snd(T_COM_SND *TransmitData, T_IMX_UART_MNG *pk_UARTmng)
{
    FLGPTN flgptn;
    ID tskid;
    ER ercd = E_OK;

    if ((pk_UARTmng->status.bit.init_flg == 0U) || (sns_dpn())) {
        ercd = E_OBJ;
    } else {
        (void)get_tid(&tskid);
        (void)dis_dsp();
        if (((pk_UARTmng->tlockid == 0U) || (pk_UARTmng->tlockid == (UH)tskid)) &&
            (pk_UARTmng->status.bit.ena_tx == 1U) &&
            (pk_UARTmng->SndData == 0)) {
            (void)loc_cpu();
            pk_UARTmng->port->UCR1 &= ~(UCR1_TRDYEN | UCR1_TXMPTYEN);
            pk_UARTmng->port->UCR4 &= ~(UCR4_TCEN);
            if ((pk_UARTmng->tcnt == 0U) && ((pk_UARTmng->port->UTS & 0x0010U) == 0U)) {
                _ddr_imx_uart_send_char(pk_UARTmng, TransmitData);
            }
            (void)unl_cpu();
            if (_ddr_imx_uart_copy(pk_UARTmng, TransmitData) == FALSE) {
                pk_UARTmng->SndData = TransmitData;
                (void)loc_cpu();
                pk_UARTmng->port->UCR1 |= (UW)(UCR1_TRDYEN | UCR1_TXMPTYEN);
                pk_UARTmng->port->UCR4 |= UCR4_TCEN;
                (void)unl_cpu();
                (void)ena_dsp();
                ercd = twai_flg((ID)pk_UARTmng->flgid, TXI_FLG, TWF_ORW, &flgptn, TransmitData->time);
                if (ercd != E_OK) {
                    (void)loc_cpu();
                    pk_UARTmng->port->UCR1 &= ~(UCR1_TRDYEN | UCR1_TXMPTYEN);
                    pk_UARTmng->port->UCR4 &= ~(UCR4_TCEN);
                    pk_UARTmng->SndData = 0;
                    (void)unl_cpu();
                    ercd = pol_flg((ID)pk_UARTmng->flgid, TXI_FLG, TWF_ORW, &flgptn);
                }
                (void)clr_flg((ID)pk_UARTmng->flgid, ~TXI_FLG);
            } else {
                if (pk_UARTmng->tcnt != 0U) {
                    (void)dis_int((INTNO)pk_UARTmng->intno);
                    pk_UARTmng->port->UCR1 |= (UW)(UCR1_TRDYEN | UCR1_TXMPTYEN);
                    pk_UARTmng->port->UCR4 |= UCR4_TCEN;
                    (void)ena_int((INTNO)pk_UARTmng->intno);
                }
                (void)ena_dsp();
            }
        } else {
            (void)ena_dsp();
            ercd = E_OBJ;
        }
    }
    return ercd;
}

/**
 *  _ddr_imx_uart_rcv
 *
 *  @param[in]  ReceiveData   receive data
 *  @param[in]  pk_UARTmng    uart manager
 *  @return E_OK,
 *          E_OBJ
 */
ER _ddr_imx_uart_rcv(T_COM_RCV *ReceiveData, T_IMX_UART_MNG *pk_UARTmng)
{
    FLGPTN flgptn;
    ID tskid;
    ER ercd = E_OK;

    if ((pk_UARTmng->status.bit.init_flg == 0U) || (sns_dpn()) ||
        (pk_UARTmng->status.bit.ena_rx   == 0U)) {
        ercd = E_OBJ;
    } else {
        (void)get_tid(&tskid);
        (void)dis_dsp();
        if (((pk_UARTmng->rlockid == 0U) || (pk_UARTmng->rlockid == (UH)tskid)) &&
            (pk_UARTmng->RcvData  == 0)) {
            (void)loc_cpu();
            pk_UARTmng->port->UCR1 &= ~(UCR1_RRDYEN);
            pk_UARTmng->port->UCR2 &= ~(UCR2_ATEN);
            pk_UARTmng->port->UCR3 &= ~(UCR3_PARERREN | UCR3_FRAERREN);
            pk_UARTmng->port->UCR4 &= ~(UCR4_BKEN | UCR4_OREN);
            (void)unl_cpu();
            if (_ddr_imx_uart_recv_strings(pk_UARTmng, ReceiveData) == FALSE) {
                pk_UARTmng->RcvData = ReceiveData;
                (void)loc_cpu();
                pk_UARTmng->port->UCR1 |= (UCR1_RRDYEN);
                pk_UARTmng->port->UCR2 |= (UCR2_ATEN);
                pk_UARTmng->port->UCR3 |= (UW)(UCR3_PARERREN | UCR3_FRAERREN);
                pk_UARTmng->port->UCR4 |= (UW)(UCR4_BKEN | UCR4_OREN);
                (void)unl_cpu();
                (void)ena_dsp();
                ercd = twai_flg((ID)pk_UARTmng->flgid, RXI_FLG, TWF_ORW, &flgptn, ReceiveData->time);
                if (ercd != E_OK) {
                    pk_UARTmng->RcvData = 0;
                    ercd = pol_flg((ID)pk_UARTmng->flgid, RXI_FLG, TWF_ORW, &flgptn);
                }
                (void)clr_flg((ID)pk_UARTmng->flgid, ~RXI_FLG);
            } else {
                (void)loc_cpu();
                pk_UARTmng->port->UCR1 |= (UCR1_RRDYEN);
                pk_UARTmng->port->UCR2 |= (UCR2_ATEN);
                pk_UARTmng->port->UCR3 |= (UW)(UCR3_PARERREN | UCR3_FRAERREN);
                pk_UARTmng->port->UCR4 |= (UW)(UCR4_BKEN | UCR4_OREN);
                (void)unl_cpu();
                (void)ena_dsp();
            }
        } else {
            (void)ena_dsp();
            ercd = E_OBJ;
        }
    }
    return ercd;
}

/**
 *  _ddr_imx_uart_cln_tx_buf
 *
 *  @param[in]  pk_UARTmng  uart manager
 *  @param[in]  tim         time out
 *  @return E_OK, E_TMOUT
 *          E_OBJ
 */
ER _ddr_imx_uart_cln_tx_buf(T_IMX_UART_MNG *pk_UARTmng, TMO tim)
{
    FLGPTN flgptn;
    ID tskid;
    ER ercd;

    (void)get_tid(&tskid);
    if (((pk_UARTmng->tlockid == 0U) ||
         (pk_UARTmng->tlockid == (UH)tskid)) &&
        ( pk_UARTmng->SndData == 0)) {
        (void)dis_int((INTNO)pk_UARTmng->intno);
        if ((pk_UARTmng->tcnt != 0U) ||
            ((pk_UARTmng->port->USR2 & USR2_TXDC) == 0U)) {
            pk_UARTmng->port->UCR1 |= (UW)(UCR1_TRDYEN | UCR1_TXMPTYEN);
            pk_UARTmng->port->UCR4 |= UCR4_TCEN;
            pk_UARTmng->wclnid = (UH)tskid;
            (void)ena_int((INTNO)pk_UARTmng->intno);
            ercd = twai_flg((ID)pk_UARTmng->flgid, TEI_FLG, TWF_ORW, &flgptn, tim);
            if (ercd != E_OK) {
                pk_UARTmng->wclnid = 0U;
                ercd = pol_flg((ID)pk_UARTmng->flgid, TEI_FLG, TWF_ORW, &flgptn);
            }
            (void)clr_flg((ID)pk_UARTmng->flgid, ~TEI_FLG);
        } else {
            (void)ena_int((INTNO)pk_UARTmng->intno);
            ercd = E_OK;
        }
    } else {
        ercd = E_OBJ;
    }
    return ercd;
}

/**
 *  _ddr_imx_uart_rst_tx_buf
 *
 *  @param[in]  pk_UARTmng   uart manager
 *  @return E_OK, E_OBJ
 */
ER _ddr_imx_uart_rst_tx_buf(T_IMX_UART_MNG *pk_UARTmng)
{
    ER ercd;
    ID tskid;

    (void)get_tid(&tskid);
    if (((pk_UARTmng->tlockid == 0U) ||
         (pk_UARTmng->tlockid == (UH)tskid)) &&
        ( pk_UARTmng->SndData == 0)) {
        (void)dis_int((INTNO)pk_UARTmng->intno);
        pk_UARTmng->tcnt = 0U;
        (void)ena_int((INTNO)pk_UARTmng->intno);
        ercd = E_OK;
    } else {
        ercd = E_OBJ;
    }
    return ercd;
}

/**
 *  _ddr_imx_uart_rst_rx_buf
 *
 *  @param[in]  pk_UARTmng   uart manager
 *  @return E_OK, E_OBJ
 */
ER _ddr_imx_uart_rst_rx_buf(T_IMX_UART_MNG *pk_UARTmng)
{
    ER ercd;
    ID tskid;

    (void)get_tid(&tskid);
    if (((pk_UARTmng->rlockid == 0U) ||
         (pk_UARTmng->rlockid == (UH)tskid)) &&
        ( pk_UARTmng->RcvData == 0)) {
        (void)dis_int((INTNO)pk_UARTmng->intno);
        pk_UARTmng->rcnt = 0U;
        (void)ena_int((INTNO)pk_UARTmng->intno);
        ercd = E_OK;
    } else
        ercd = E_OBJ;
    return ercd;
}

/**
 *  _ddr_imx_uart_dis_send
 *
 *  @param[in]  pk_UARTmng   uart manager
 *  @return E_OK, E_OBJ
 */
ER _ddr_imx_uart_dis_send(T_IMX_UART_MNG *pk_UARTmng)
{
    ER ercd;
    ID tskid;

    (void)get_tid(&tskid);
    if (((pk_UARTmng->tlockid == 0U) ||
         (pk_UARTmng->tlockid == (UH)tskid)) &&
        ( pk_UARTmng->SndData == 0) &&
        ( pk_UARTmng->tcnt    == 0U) &&
        ((pk_UARTmng->port->UTS & UTS_TXEMPTY) != 0U)) {
        (void)dis_int((INTNO)pk_UARTmng->intno);
        pk_UARTmng->port->UCR1 &= ~(UCR1_TRDYEN | UCR1_TXMPTYEN);
        pk_UARTmng->port->UCR2 &= ~(UCR2_TXEN | UCR2_CTS);
        pk_UARTmng->port->UCR4 &= ~(UCR4_TCEN);
        pk_UARTmng->status.bit.ena_tx = 0U;
        (void)ena_int((INTNO)pk_UARTmng->intno);
        ercd = E_OK;
    } else
        ercd = E_OBJ;
    return ercd;
}

/**
 *  _ddr_imx_uart_dis_rcv
 *
 *  @param[in]  pk_UARTmng   uart manager
 *  @return E_OK, E_OBJ
 */
ER _ddr_imx_uart_dis_rcv(T_IMX_UART_MNG *pk_UARTmng)
{
    ER ercd;
    ID tskid;

    (void)get_tid(&tskid);
    if (((pk_UARTmng->rlockid == 0U) ||
         (pk_UARTmng->rlockid == (UH)tskid)) &&
        ( pk_UARTmng->RcvData == 0) &&
        ( pk_UARTmng->rcnt    == 0U) &&
        ((pk_UARTmng->port->UTS & UTS_RXEMPTY) != 0U)) {
        (void)dis_int((INTNO)pk_UARTmng->intno);
        pk_UARTmng->port->UCR1 &= ~(UCR1_RRDYEN);
        pk_UARTmng->port->UCR2 &= ~(UCR2_ATEN | UCR2_RXEN);
        pk_UARTmng->port->UCR3 &= ~(UCR3_PARERREN | UCR3_FRAERREN);
        pk_UARTmng->port->UCR4 &= ~(UCR4_BKEN | UCR4_OREN);
        pk_UARTmng->status.bit.ena_rx = 0U;
        (void)ena_int((INTNO)pk_UARTmng->intno);
        ercd = E_OK;
    } else
        ercd = E_OBJ;
    return ercd;
}

/**
 *  _ddr_imx_uart_ena_send
 *
 *  @param[in]  pk_UARTmng   uart manager
 *  @return E_OK, E_OBJ
 */
ER _ddr_imx_uart_ena_send(T_IMX_UART_MNG *pk_UARTmng)
{
    ER ercd;
    ID tskid;

    (void)get_tid(&tskid);
    if ((pk_UARTmng->rlockid == 0U) ||
        (pk_UARTmng->rlockid == (UH)tskid)) {
        (void)dis_int((INTNO)pk_UARTmng->intno);
        pk_UARTmng->status.bit.ena_tx = 1U;
        pk_UARTmng->port->UCR2 |= (UW)(UCR2_TXEN | UCR2_CTS);
        (void)ena_int((INTNO)pk_UARTmng->intno);
        ercd = E_OK;
    } else
        ercd = E_OBJ;
    return ercd;
}

/**
 *  _ddr_imx_uart_ena_rcv
 *
 *  @param[in]  pk_UARTmng   uart manager
 *  @return E_OK, E_OBJ
 */
ER _ddr_imx_uart_ena_rcv(T_IMX_UART_MNG *pk_UARTmng)
{
    ER ercd;
    ID tskid;

    (void)get_tid(&tskid);
    if ((pk_UARTmng->rlockid == 0U) ||
        (pk_UARTmng->rlockid == (UH)tskid)) {
        (void)dis_int((INTNO)pk_UARTmng->intno);
        pk_UARTmng->status.bit.ena_rx = 1U;
        pk_UARTmng->port->UCR1 |= (UCR1_RRDYEN);
        pk_UARTmng->port->UCR2 |= (UW)(UCR2_ATEN | UCR2_RXEN);
        pk_UARTmng->port->UCR3 |= (UW)(UCR3_PARERREN | UCR3_FRAERREN);
        pk_UARTmng->port->UCR4 |= (UW)(UCR4_BKEN | UCR4_OREN);
        (void)ena_int((INTNO)pk_UARTmng->intno);
        ercd = E_OK;
    } else
        ercd = E_OBJ;
    return ercd;
}

/**
 *  _ddr_imx_uart_snd_brk
 *
 *  @param[in]  pk_UARTmng   uart manager
 *  @param[in]  tim          break time
 *  @return E_OK, E_OBJ
 */
ER _ddr_imx_uart_snd_brk(T_IMX_UART_MNG *pk_UARTmng, TMO tim)
{
    ER ercd;
    ID tskid;

    (void)get_tid(&tskid);
    if (((pk_UARTmng->tlockid == 0U) ||
         (pk_UARTmng->tlockid == (UH)tskid)) &&
        ( pk_UARTmng->SndData == 0) &&
        ( pk_UARTmng->tcnt    == 0U)) {
        if ((pk_UARTmng->port->UTS & UTS_TXEMPTY) != 0U) {
            (void)dis_int((INTNO)pk_UARTmng->intno);
            pk_UARTmng->port->UCR1 |= (UCR1_SNDBRK);
            (void)ena_int((INTNO)pk_UARTmng->intno);
            (void)dly_tsk((RELTIM)tim);
            (void)dis_int((INTNO)pk_UARTmng->intno);
            pk_UARTmng->port->UCR1 &= ~(UCR1_SNDBRK);
            (void)ena_int((INTNO)pk_UARTmng->intno);
            ercd = E_OK;
        } else {
            ercd = E_OBJ;
        }
    } else {
        ercd = E_OBJ;
    }
    return ercd;
}

/**
 *  _ddr_imx_uart_lock_trans
 *
 *  @param[in]  pk_UARTmng   uart manager
 *  @return E_OK, E_OBJ
 */
ER _ddr_imx_uart_lock_trans(T_IMX_UART_MNG *pk_UARTmng)
{
    ER ercd;
    ID tskid;

    (void)get_tid(&tskid);
    (void)dis_int((INTNO)pk_UARTmng->intno);
    if ((pk_UARTmng->tlockid == 0U) ||
        (pk_UARTmng->SndData == 0)) {
        pk_UARTmng->tlockid = (UH)tskid;
        ercd = E_OK;
    } else if (pk_UARTmng->tlockid == (UH)tskid) {
        ercd = E_OK;
    } else {
        ercd = E_OBJ;
    }
    (void)ena_int((INTNO)pk_UARTmng->intno);
    return ercd;
}

/**
 *  _ddr_imx_uart_lock_recv
 *
 *  @param[in]  pk_UARTmng   uart manager
 *  @return E_OK, E_OBJ
 */
ER _ddr_imx_uart_lock_recv(T_IMX_UART_MNG *pk_UARTmng)
{
    ER ercd;
    ID tskid;

    (void)get_tid(&tskid);
    (void)dis_int((INTNO)pk_UARTmng->intno);
    if ((pk_UARTmng->rlockid == 0U) ||
        (pk_UARTmng->RcvData == 0)) {
        pk_UARTmng->rlockid = (UH)tskid;
        ercd = E_OK;
    } else if (pk_UARTmng->rlockid == (UH)tskid) {
        ercd = E_OK;
    } else {
        ercd = E_OBJ;
    }
    (void)ena_int((INTNO)pk_UARTmng->intno);
    return ercd;
}

/**
 *  _ddr_imx_uart_unlock_trans
 *
 *  @param[in]  pk_UARTmng   uart manager
 *  @return E_OK, E_OBJ
 */
ER _ddr_imx_uart_unlock_trans(T_IMX_UART_MNG *pk_UARTmng)
{
    ER ercd;
    ID tskid;

    (void)get_tid(&tskid);
    if (pk_UARTmng->tlockid == (UH)tskid) {
        pk_UARTmng->tlockid = 0U;
        ercd = E_OK;
    } else if (pk_UARTmng->tlockid == 0U) {
        ercd = E_OK;
    } else {
        ercd = E_OBJ;
    }
    return ercd;
}

/**
 *  _ddr_imx_uart_unlock_recv
 *
 *  @param[in]  pk_UARTmng   uart manager
 *  @return E_OK, E_OBJ
 */
ER _ddr_imx_uart_unlock_recv(T_IMX_UART_MNG *pk_UARTmng)
{
    ER ercd;
    ID tskid;

    (void)get_tid(&tskid);
    if (pk_UARTmng->rlockid == (UH)tskid) {
        pk_UARTmng->rlockid = 0U;
        ercd = E_OK;
    } else if (pk_UARTmng->rlockid == 0U) {
        ercd = E_OK;
    } else {
        ercd = E_OBJ;
    }
    return ercd;
}

/**
 *  _ddr_imx_uart_asert_rts
 *
 *  @param[in]  pk_UARTmng   uart manager
 *  @return E_OK, E_OBJ
 */
ER _ddr_imx_uart_asert_rts(T_IMX_UART_MNG *pk_UARTmng)
{
    ER ercd;

    if (pk_UARTmng->status.bit.hrd_flw == 0U) {
        (void)dis_int((INTNO)pk_UARTmng->intno);
        pk_UARTmng->port->UCR2 |= (UCR2_CTS);
        (void)ena_int((INTNO)pk_UARTmng->intno);
        ercd = E_OK;
    } else {
        ercd = E_OBJ;
    }
    return ercd;
}

/**
 *  _ddr_imx_uart_negate_rts
 *
 *  @param[in]  pk_UARTmng   uart manager
 *  @return E_OK, E_OBJ
 */
ER _ddr_imx_uart_negate_rts(T_IMX_UART_MNG *pk_UARTmng)
{
    ER ercd;

    if (pk_UARTmng->status.bit.hrd_flw == 0U) {
        (void)dis_int((INTNO)pk_UARTmng->intno);
        pk_UARTmng->port->UCR2 &= ~(UCR2_CTS);
        (void)ena_int((INTNO)pk_UARTmng->intno);
        ercd = E_OK;
    } else {
        ercd = E_OBJ;
    }
    return ercd;
}

/**
 *  _ddr_imx_uart_asert_dtr
 *
 *  @param[in]  pk_UARTmng   uart manager
 *  @return E_OK
 */
ER _ddr_imx_uart_asert_dtr(T_IMX_UART_MNG *pk_UARTmng)
{
    (void)dis_int((INTNO)pk_UARTmng->intno);
    pk_UARTmng->port->UCR3 |= (UCR3_DSR);
    (void)ena_int((INTNO)pk_UARTmng->intno);

    return E_OK;
}

/**
 *  _ddr_imx_uart_negate_dtr
 *
 *  @param[in]  pk_UARTmng   uart manager
 *  @return E_OK
 */
ER _ddr_imx_uart_negate_dtr(T_IMX_UART_MNG *pk_UARTmng)
{
    (void)dis_int((INTNO)pk_UARTmng->intno);
    pk_UARTmng->port->UCR3 &= ~(UCR3_DSR);
    (void)ena_int((INTNO)pk_UARTmng->intno);

    return E_OK;
}

/**
 *  _ddr_imx_uart_ctr
 *
 *  @param[out] pk_SerialData serial set data
 *  @param[in]  pk_UARTmng   uart manager
 *  @return E_OK, E_OBJ
 */
ER _ddr_imx_uart_ctr(T_COM_CTR const *pk_SerialData, T_IMX_UART_MNG *pk_UARTmng)
{
    ER ercd = E_OK;

    if (pk_UARTmng->status.bit.init_flg == 0U)
        ercd = E_OBJ;

    if (ercd == E_OK)
        if ((pk_SerialData->command & CLN_TXBUF) == CLN_TXBUF)
            ercd = _ddr_imx_uart_cln_tx_buf(pk_UARTmng, pk_SerialData->time);

    if (ercd == E_OK)
        if ((pk_SerialData->command & RST_TXBUF) == RST_TXBUF)
            ercd = _ddr_imx_uart_rst_tx_buf(pk_UARTmng);

    if (ercd == E_OK)
        if ((pk_SerialData->command & RST_RXBUF) == RST_RXBUF)
            ercd = _ddr_imx_uart_rst_rx_buf(pk_UARTmng);

    if (ercd == E_OK)
        if ((pk_SerialData->command & STP_TX) == STP_TX)
            ercd = _ddr_imx_uart_dis_send(pk_UARTmng);

    if (ercd == E_OK)
        if ((pk_SerialData->command & STP_RX) == STP_RX)
            ercd = _ddr_imx_uart_dis_rcv(pk_UARTmng);

    if (ercd == E_OK)
        if ((pk_SerialData->command & STA_TX) == STA_TX)
            ercd = _ddr_imx_uart_ena_send(pk_UARTmng);

    if (ercd == E_OK)
        if ((pk_SerialData->command & STA_RX) == STA_RX)
            ercd = _ddr_imx_uart_ena_rcv(pk_UARTmng);

    if (ercd == E_OK)
        if ((pk_SerialData->command & SND_BRK) == SND_BRK)
            ercd = _ddr_imx_uart_snd_brk(pk_UARTmng, pk_SerialData->time);

    if (ercd == E_OK)
        if ((pk_SerialData->command & LOC_TX) == LOC_TX)
            ercd = _ddr_imx_uart_lock_trans(pk_UARTmng);

    if (ercd == E_OK)
        if ((pk_SerialData->command & LOC_RX) == LOC_RX)
            ercd = _ddr_imx_uart_lock_recv(pk_UARTmng);

    if (ercd == E_OK)
        if ((pk_SerialData->command & UNL_TX) == UNL_TX)
            ercd = _ddr_imx_uart_unlock_trans(pk_UARTmng);

    if (ercd == E_OK)
        if ((pk_SerialData->command & UNL_RX) == UNL_RX)
            ercd = _ddr_imx_uart_unlock_recv(pk_UARTmng);

    if (ercd == E_OK)
        if ((pk_SerialData->command & ASR_RTS) == ASR_RTS)
            ercd = _ddr_imx_uart_asert_rts(pk_UARTmng);

    if (ercd == E_OK)
        if ((pk_SerialData->command & NGT_RTS) == NGT_RTS)
            ercd = _ddr_imx_uart_negate_rts(pk_UARTmng);

    if (ercd == E_OK)
        if ((pk_SerialData->command & ASR_DTR) != 0U)
            ercd = _ddr_imx_uart_asert_dtr(pk_UARTmng);

    if (ercd == E_OK)
        if ((pk_SerialData->command & NGT_DTR) != 0U)
            ercd = _ddr_imx_uart_negate_dtr(pk_UARTmng);

    return ercd;
}

/**
 *  _ddr_imx_uart_ref
 *
 *  @param[out] pk_SerialRef serial status
 *  @param[in]  pk_UARTmng   uart manager
 *  @return E_OK, E_OBJ
 */
ER _ddr_imx_uart_ref(T_COM_REF *pk_SerialRef, T_IMX_UART_MNG *pk_UARTmng)
{
    UH status = 0U;
    ER ercd   = E_OK;
    if (pk_UARTmng->status.bit.init_flg == 1U) {
        (void)dis_int((INTNO)pk_UARTmng->intno);
        pk_SerialRef->rxcnt = pk_UARTmng->rcnt;
        pk_SerialRef->txcnt = pk_UARTmng->tcnt;
        (void)ena_int((INTNO)pk_UARTmng->intno);

        status |= T_COM_INIT;
        if ((pk_UARTmng->port->UCR2 & UCR2_CTSC) == 0U) {
            if ((pk_UARTmng->port->UCR2 & UCR2_CTS) != 0U)
                status |= T_COM_CTS;
        }
        if ((pk_UARTmng->port->USR1 & USR1_RTSS) != 0U)
            status |= T_COM_RTS;
        if ((pk_UARTmng->port->USR2 & USR2_DCDIN) == 0U)
            status |= T_COM_CD;
        if ((pk_UARTmng->port->USR2 & USR2_RIIN) == 0U)
            status |= T_COM_RI;
        if ((pk_UARTmng->port->UCR3 & UCR3_DSR) == 0U)
            status |= T_COM_DTR;
        if (pk_UARTmng->status.bit.ena_tx == 1U)
            status |= T_COM_ENATX;
        if (pk_UARTmng->status.bit.ena_rx == 1U)
            status |= T_COM_ENARX;
        if (pk_UARTmng->status.bit.rx_xoff == 1U)
            status |= T_COM_RXOFF;
        if (pk_UARTmng->status.bit.tx_xoff == 1U)
            status |= T_COM_TXOFF;
        pk_SerialRef->status = status;
    } else {
        ercd = E_OBJ;
    }
    return ercd;
}

/**
 *  _ddr_imx_uart_ini
 */
ER _ddr_imx_uart_ini(T_COM_SMOD const *pk_SerialMode, T_IMX_UART_MNG *pk_UARTmng)
{
    volatile INT i;
    volatile UW ubir;
    volatile UW clksrc;
    volatile UW onems;
    ER ercd = E_OK;
    T_IMX_UART_MSTS status;
    T_RFLG pk_rflg;

    if (pk_SerialMode != 0) {

        if (pk_UARTmng->status.bit.init_flg == 0U) {
            /* Software Reset */
            pk_UARTmng->port->UCR2 = 0U;
            for (;;) {
                if ((pk_UARTmng->port->UTS & UTS_SOFTRST) == 0U) {
                    break;
                } else if ((sns_dpn())) {
                    for (i = 0; i < 10000; i++) ;
                } else {
                    (void)dly_tsk(1U);
                }
            }

            clksrc = UART_CLK;
            if ((clksrc % 6U) == 0U) {
                clksrc /= 6U;
                pk_UARTmng->port->UFCR = pk_UARTmng->aux[0];
            } else if ((clksrc % 7U) == 0U) {
                clksrc /= 7U;
                pk_UARTmng->port->UFCR = (UW)(pk_UARTmng->aux[0] | 0x00000300U);
            } else {
                clksrc /= 5U;
                pk_UARTmng->port->UFCR = (UW)(pk_UARTmng->aux[0] | 0x00000080U);
            }
            onems = clksrc / 1000U;
            pk_UARTmng->port->UCR4 = pk_UARTmng->aux[1];

            status.word = 0U;
            switch (pk_SerialMode->blen) {
                case BLEN7:
                    break;
                case BLEN8:
                    pk_UARTmng->port->UCR2 = UCR2_WS;
                    break;
                default:
                    ercd = E_PAR;
                    break;
            }

            switch (pk_SerialMode->par) {
                case PAR_ODD:
                    pk_UARTmng->port->UCR2 |= (UW)(UCR2_PREN | UCR2_PROE);
                    break;
                case PAR_EVEN:
                    pk_UARTmng->port->UCR2 |= (UCR2_PREN);
                    break;
                case PAR_NONE:
                    /* Nothing to do */
                    break;
                default:
                    break;
            }

            switch (pk_SerialMode->sbit) {
                case SBIT2:
                    pk_UARTmng->port->UCR2 |= (UCR2_STPB);
                    break;
                case SBIT1:
                    /* Nothing to do */
                    break;
                default:
                    ercd = E_PAR;
                    break;
            }

            switch (pk_SerialMode->flow) {
                case FLW_HARD:
                    pk_UARTmng->port->UCR2 |= (UCR2_CTSC);
                    status.bit.hrd_flw = 1U;
                    break;
                case FLW_XON:
                    pk_UARTmng->port->UCR2 |= (UCR2_IRTS);
                    status.bit.sft_flw = 1U;
                    break;
                case FLW_NONE:
                    pk_UARTmng->port->UCR2 |= (UCR2_IRTS);
                    break;
                default:
                    ercd = E_PAR;
                    break;
            }

            if ((pk_SerialMode->baud % 200U) == 0U) {
                ubir = (pk_SerialMode->baud / 200U) * 16U;
                if (ubir == 0U) {
                    ercd = E_PAR;
                }
            } else {
                ubir = 0U;
                ercd = E_PAR;
            }

            if ((ubir < 0x10U) || (ubir > 0x80000U) || ((clksrc/200U) > 0x80000U)) {
                ercd = E_PAR;
            } else if ((ubir < 0x80U) || (ubir > 0x40000U) || ((clksrc/200U) > 0x40000U)) {
                ubir >>= 3U;
                clksrc >>= 3U;
            } else if ((ubir < 0x4U) || (ubir > 0x20000U) || ((clksrc/200U) > 0x20000U)) {
                ubir >>= 2U;
                clksrc >>= 2U;
            } else if ((ubir < 0x2U) || (ubir > 0x10000U) || ((clksrc/200U) > 0x10000U)) {
                ubir >>= 1U;
                clksrc >>= 1U;
            } else {
                /* Do Nothing */
            }
            if (ercd == E_OK) {
                status.bit.init_flg = 1U;
                pk_UARTmng->status.word = status.word;
                (void)dly_tsk(0U); /* Workaround to avoid UBIR clear on i.MX8M Plus */
                pk_UARTmng->port->UBIR = ubir - 1U;
                pk_UARTmng->port->UBMR = (clksrc / 200U) - 1U;
                pk_UARTmng->port->UCR3 = 0x00000704U;
                pk_UARTmng->port->ONEMS = onems;
                if ((sns_dpn())) {
                    for (i = 0; i < 10000; i++) ;
                } else {
                    (void)dly_tsk(1U);
                }
                pk_UARTmng->port->UCR1 |= (UCR1_UARTEN);
            }
        } else {
            ercd = E_OBJ;
        }
    } else {
        if (pk_UARTmng->status.bit.init_flg == 1U) {
            /* Software Reset */
            pk_UARTmng->port->UCR2 = 0U;
            for (;;) {
                if ((pk_UARTmng->port->UTS & UTS_SOFTRST) == 0U) {
                    break;
                } else if ((sns_dpn())) {
                    for(i = 0; i < 10000; i++);
                } else {
                    (void)dly_tsk(1U);
                }
            }
            /* Wait until flag released */
            for (;;) {
                if (ref_flg((ID)pk_UARTmng->flgid, &pk_rflg) == E_OK) {
                    if (pk_rflg.wtskid != 0) {
                        (void)rel_wai(pk_rflg.wtskid);
                    } else {
                        break;
                    }
                } else {
                    break;
                }
            }
            /* clear internal data */
            pk_UARTmng->status.word = 0U;
            pk_UARTmng->SndData = 0;
            pk_UARTmng->RcvData = 0;
            pk_UARTmng->tcnt = 0U;
            pk_UARTmng->rcnt = 0U;
            pk_UARTmng->tlockid = 0U;
            pk_UARTmng->rlockid = 0U;
        }
    }

    return ercd;
}

/**
 *  _ddr_imx_uartdrv
 *  @param[in]  FuncID
 *  @param[in,out] pk_ControlData control data
 *  @param[in]  pk_UARTmng   uart manager
 *  @return E_OK, E_OBJ
 */
static ER _ddr_imx_uartdrv(ID FuncID, VP pk_ControlData, T_IMX_UART_MNG *pk_UARTmng)
{
    ER ercd;

    switch(FuncID) {
        case TA_COM_INI:
            ercd = _ddr_imx_uart_ini((T_COM_SMOD const *)pk_ControlData, pk_UARTmng);
            break;
        case TA_COM_REF:
            ercd = _ddr_imx_uart_ref((T_COM_REF *)pk_ControlData, pk_UARTmng);
            break;
        case TA_COM_CTR:
            ercd = _ddr_imx_uart_ctr((T_COM_CTR const *)pk_ControlData, pk_UARTmng);
            break;
        case TA_COM_PUTS:
            ercd = _ddr_imx_uart_snd((T_COM_SND *)pk_ControlData, pk_UARTmng);
            break;
        case TA_COM_GETS:
            ercd = _ddr_imx_uart_rcv((T_COM_RCV *)pk_ControlData, pk_UARTmng);
            break;
        default:
            ercd = E_NOSPT;
            break;
    }
    return ercd;
}
