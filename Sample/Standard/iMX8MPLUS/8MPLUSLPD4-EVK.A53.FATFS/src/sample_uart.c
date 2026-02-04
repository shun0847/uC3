/**
 * @file    sample_uart.c
 * @brief   UART sample program
 * @date    2021.01.05
 * @author  Copyright (c) 2021, eForce Co., Ltd. All rights reserved.
 *
 ****************************************************************************
 * @par     History
 *          - rev 1.0 (2021.01.05) Imada
 *            Initial version.
 ****************************************************************************
 */
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "kernel.h"
#include "DDR_COM.h"
#include "sample_uart_cfg.h"

/* Private function prototypes -----------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* User input message */
typedef struct t_msgblk {
    T_MSG   header;
    UINT    cnt;
    VB      buf[120];
} T_MSGBLK;

/* Private variables ---------------------------------------------------------*/

static ID g_mpfID;
static ID g_mbxID;
ID g_sndTaskID;

/*
 * Start the UART sample.
 */
void sample_uart_start(void)
{
    const T_CMPF cmpf = {
        TA_TFIFO,
        20,
        sizeof(T_MSGBLK),
        0,
        "Mpf"};

    const T_CMBX cmbx = {
        TA_TFIFO | TA_MFIFO,
        0,
        0,
        "Mbx"};

    const T_COM_SMOD  uart_ini = {
        CFG_BAUDRATE,
        CFG_BLEN,
        CFG_PAR,
        CFG_SBIT,
        CFG_FLW
    };

    (void)DDR_UART_INIT_FN(ID_UART, &REG_UART);

    g_mpfID = acre_mpf((T_CMPF *)&cmpf);
    g_mbxID = acre_mbx((T_CMBX *)&cmbx);

    (void)ini_com(ID_UART, &uart_ini);
    (void)ctr_com(ID_UART, STA_COM, 0);

    (void)ctr_com(ID_UART, SND_BRK, 100);
}

void UART_PRINTF(const char *fmt, ...)
{
    static char buf[UART_PRINTF_BUF_SIZE];
    va_list ap;

    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    if (n < 0) {
        return;
    }

    /* vsnprintf がバッファを超えた場合でも、バッファ内はNUL終端される */
    buf[sizeof(buf) - 1] = '\0';

    UINT txcnt;

    if (n >= (int)sizeof(buf)) {
        txcnt = (UINT)(sizeof(buf) - 1);
    } else {
        txcnt = (UINT)n;
    }
    txcnt = (UINT)strlen(buf);
    (void)puts_com(ID_UART, buf, &txcnt, TMO_FEVR);

    return; 
}

void UART_PUTCHAR(char ch)
{
    (void)putc_com(ID_UART, (VB)ch, TMO_FEVR);
}

char UART_GETCHAR(void)
{
    VB ch;
    ER ercd;
    do
    {
        ercd = getc_com(ID_UART, &ch, NULL, TMO_FEVR);
    } while (ercd != E_OK);
    return (char)ch;
}
