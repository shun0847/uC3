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

static void uart_send_task(VP_INT exinf);
static void uart_recv_task(VP_INT exinf);
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
 * UART send task
 */
static void uart_send_task(VP_INT exinf)
{
    UINT txcnt;
    T_MSGBLK *blk = 0;
    INT i = 0;
    VB chr;
    ER ercd;

    const T_COM_SMOD  uart_ini = {
        CFG_BAUDRATE,
        CFG_BLEN,
        CFG_PAR,
        CFG_SBIT,
        CFG_FLW
    };

    VB banner_str[]
        = "\n\r\teForce Operating System Sample Program\r\n";

    (void)ini_com(ID_UART, &uart_ini);
    (void)ctr_com(ID_UART, STA_COM, 0);

    (void)ctr_com(ID_UART, SND_BRK, 100);
    txcnt = strlen(banner_str);
    UART_PRINTF("%s", banner_str);
    //(void)puts_com(ID_UART, banner_str, &txcnt, TMO_FEVR);

    do {
        ercd = getc_com(ID_UART, &chr, 0, 10);
        if (ercd == E_OK) {
            (void)putc_com(ID_UART, chr, TMO_FEVR);
            if (chr == (VB)'\r') {
                (void)putc_com(ID_UART, (VB)'\n', TMO_FEVR);
            }
        }
        else
        {
            (void)dly_tsk(999U);
            chr = (VB)(i + '0');
            (void)putc_com(ID_UART, chr, TMO_FEVR);
            if (++i >= 10) {
                i = 0;
            }
        }
    } while (ercd != E_OK);

    (void)snd_mbx(g_mbxID, (T_MSG *)blk);
    (void)tslp_tsk(1);
    for(;;) {
        rcv_mbx(g_mbxID, (T_MSG **)&blk);
        (void)puts_com(ID_UART, blk->buf, &blk->cnt, TMO_FEVR);
        (void)rel_mpf(g_mpfID, (VP)blk);
        (void)ctr_com(ID_UART, CLN_TXBUF, 100);
    }
}

/*
 * UART receive task
 */
static void uart_recv_task(VP_INT exinf)
{
    T_MSGBLK *blk = 0;

    rcv_mbx(g_mbxID, (T_MSG **)&blk);
    for(;;) {
        get_mpf(g_mpfID, (VP *)&blk);
        blk->cnt = sizeof(blk->buf) - 1U;   /* parasoft-suppress BD-PB-NP "2017/09/01 Reviewed" */
        (void)gets_com(ID_UART, blk->buf, 0, '\r', &blk->cnt, TMO_FEVR);
        blk->buf[blk->cnt] = '\n';
        blk->cnt++;
        (void)snd_mbx(g_mbxID, (T_MSG *)blk);
    }
}

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
    
#if 0

    const T_CTSK ctsk_snd = {
        TA_HLNG | TA_ACT | TA_FPU,
        (VP_INT)0,
        (FP)uart_send_task,
        4,
        0x800,
        0,
        "uart_send_task"};

    const T_CTSK ctsk_rcv = {
        TA_HLNG | TA_ACT | TA_FPU,
        (VP_INT)0,
        (FP)uart_recv_task,
        5,
        0x800,
        0,
        "uart_recv_task"};

    (void)acre_tsk((T_CTSK *)&ctsk_snd);
    (void)acre_tsk((T_CTSK *)&ctsk_rcv);
#endif
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

    /* ここに“挟みたい処理”を書く（例：ログタグ付与、改行統一、フィルタ等） */
    // preprocess(buf);

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
