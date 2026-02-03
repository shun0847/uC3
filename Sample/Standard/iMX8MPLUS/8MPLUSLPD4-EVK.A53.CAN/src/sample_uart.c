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
#include <string.h>
#include "kernel.h"
#include "DDR_COM.h"
#include "sample_uart_cfg.h"
/* ▽▽▽ SNY CANドライバを使用するため追加 add ▽▽▽ */
#include "sample_can_cfg.h"
/* △△△ SNY CANドライバを使用するため追加 add △△△ */

/* Private function prototypes -----------------------------------------------*/

static void uart_send_task(VP_INT exinf);
static void uart_recv_task(VP_INT exinf);
/* ▽▽▽ SNY コンソール出力のため追加 add ▽▽▽ */
static void putc_chg_dec2hex(uint16_t num);
static void putc_flexcan_dlc(uint16_t num);

/* Private typedef -----------------------------------------------------------*/

#define FLEXCAN_STDID_FROM_WORD(w) \
    ( (uint32_t)((w) & CAN_ID_STD_MASK) >> CAN_ID_STD_SHIFT )
#define STDID_WIDTH 11U
/* △△△ SNY コンソール出力のため追加 add △△△ */

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
/* ▽▽▽ SNY コンソール出力のため追加 add ▽▽▽ */
static void putc_chg_dec2hex(uint16_t num)
{
    (void)putc_com(ID_UART, (VB)' ', TMO_FEVR);
    (void)putc_com(ID_UART, (VB)'0', TMO_FEVR);
    (void)putc_com(ID_UART, (VB)'x', TMO_FEVR);
    if (num == 0) {
        (void)putc_com(ID_UART, (VB)'0', TMO_FEVR);
    } else {
        int started = 0;
        for (int pos = 28; pos >= 0; pos -= 4) {
            uint8_t nib = (num >> pos) & 0xF;
            if (!started) {
                if (nib == 0 && pos > 0) continue;
                started = 1;
            }
            char c = (nib < 10) ? ('0' + nib) : ('A' + (nib - 10)); // 小文字なら 'a'
            (void)putc_com(ID_UART, (VB)c, TMO_FEVR);
        }
    }
}

static void putc_flexcan_dlc(uint16_t num){
    (void)putc_com(ID_UART, (VB)' ', TMO_FEVR);
    (void)putc_com(ID_UART, (VB)' ', TMO_FEVR);
    if(num < 9){
        (void)putc_com(ID_UART, (VB)num+'0', TMO_FEVR);
#if (defined(USE_CANFD) && USE_CANFD)
    } else if(num < 13){
        (void)putc_com(ID_UART, (VB)'1', TMO_FEVR);
        (void)putc_com(ID_UART, (VB)'2', TMO_FEVR);
    } else if(num < 17){
        (void)putc_com(ID_UART, (VB)'1', TMO_FEVR);
        (void)putc_com(ID_UART, (VB)'6', TMO_FEVR);
    } else if(num < 21){
        (void)putc_com(ID_UART, (VB)'2', TMO_FEVR);
        (void)putc_com(ID_UART, (VB)'0', TMO_FEVR);
    } else if(num < 25){
        (void)putc_com(ID_UART, (VB)'2', TMO_FEVR);
        (void)putc_com(ID_UART, (VB)'4', TMO_FEVR);
    } else if(num < 33){
        (void)putc_com(ID_UART, (VB)'3', TMO_FEVR);
        (void)putc_com(ID_UART, (VB)'2', TMO_FEVR);
    } else if(num < 49){
        (void)putc_com(ID_UART, (VB)'4', TMO_FEVR);
        (void)putc_com(ID_UART, (VB)'8', TMO_FEVR);
    } else {
        (void)putc_com(ID_UART, (VB)'6', TMO_FEVR);
        (void)putc_com(ID_UART, (VB)'4', TMO_FEVR);
#endif
    }
    (void)putc_com(ID_UART, (VB)' ', TMO_FEVR);
}
/* △△△ SNY コンソール出力のため追加 add △△△ */

/*
 * UART send task
 */
static void uart_send_task(VP_INT exinf)
{
    UINT txcnt;
    T_MSGBLK *blk = 0;
    
    /* ▽▽▽ SNY 数値のカウントアップ機能は削除 del ▽▽▽ */
    /* INT i = 0; */
    /* △△△ SNY 数値のカウントアップ機能は削除 del △△△ */
    VB chr;
    ER ercd;
    flexcan_frame_t rxframe;
    uint32_t id = 0;

    const T_COM_SMOD  uart_ini = {
        CFG_BAUDRATE,
        CFG_BLEN,
        CFG_PAR,
        CFG_SBIT,
        CFG_FLW
    };

    /* ▽▽▽ SNY 機能変更に伴いメッセージ変更 chg ▽▽▽ */
    VB banner_str[]
        = "=======================\n\rCHANEL  ID  DLC  Data\n\r================================\n\r";
    /* △△△ SNY CANドライバを送受信するため追加 chg △△△ */
    (void)ini_com(ID_UART, &uart_ini);
    (void)ctr_com(ID_UART, STA_COM, 0);

    (void)ctr_com(ID_UART, SND_BRK, 100);
    txcnt = strlen(banner_str);
    (void)puts_com(ID_UART, banner_str, &txcnt, TMO_FEVR);

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
            /* ▽▽▽ SNY CANドライバを送受信するため追加 chg ▽▽▽ */
            /* chr = (VB)(i + '0');                          */
            /* (void)putc_com(ID_UART, chr, TMO_FEVR);       */
            /* if (++i >= 10) {                              */
            /*     i = 0;                                    */
            /* }                                             */
            
            if (kStatus_Success == flexcan_receiveframe(EXAMPLE_CAN1, &rxframe))
            {
                (void)putc_com(ID_UART, (VB)'C', TMO_FEVR);
                (void)putc_com(ID_UART, (VB)'A', TMO_FEVR);
                (void)putc_com(ID_UART, (VB)'N', TMO_FEVR);
                (void)putc_com(ID_UART, (VB)'1', TMO_FEVR);
                (void)putc_com(ID_UART, (VB)' ', TMO_FEVR);
                id = FLEXCAN_STDID_FROM_WORD(rxframe.id);
                putc_chg_dec2hex(id);
                putc_flexcan_dlc(rxframe.length);
                putc_chg_dec2hex(rxframe.dataByte0);
                putc_chg_dec2hex(rxframe.dataByte1);
                putc_chg_dec2hex(rxframe.dataByte2);
                putc_chg_dec2hex(rxframe.dataByte3);
                putc_chg_dec2hex(rxframe.dataByte4);
                putc_chg_dec2hex(rxframe.dataByte5);
                putc_chg_dec2hex(rxframe.dataByte6);
                putc_chg_dec2hex(rxframe.dataByte7);
                (void)putc_com(ID_UART, (VB)'\n', TMO_FEVR);
                (void)putc_com(ID_UART, (VB)'\r', TMO_FEVR);
            }
            #ifdef USE_CAN2
            if (kStatus_Success == flexcan_receiveframe(EXAMPLE_CAN2, &rxframe))
            {
                (void)putc_com(ID_UART, (VB)'C', TMO_FEVR);
                (void)putc_com(ID_UART, (VB)'A', TMO_FEVR);
                (void)putc_com(ID_UART, (VB)'N', TMO_FEVR);
                (void)putc_com(ID_UART, (VB)'2', TMO_FEVR);
                (void)putc_com(ID_UART, (VB)' ', TMO_FEVR);
                id = FLEXCAN_STDID_FROM_WORD(rxframe.id);
                putc_chg_dec2hex(id);
                putc_flexcan_dlc(rxframe.length);
                putc_chg_dec2hex(rxframe.dataByte0);
                putc_chg_dec2hex(rxframe.dataByte1);
                putc_chg_dec2hex(rxframe.dataByte2);
                putc_chg_dec2hex(rxframe.dataByte3);
                putc_chg_dec2hex(rxframe.dataByte4);
                putc_chg_dec2hex(rxframe.dataByte5);
                putc_chg_dec2hex(rxframe.dataByte6);
                putc_chg_dec2hex(rxframe.dataByte7);
                (void)putc_com(ID_UART, (VB)'\n', TMO_FEVR);
                (void)putc_com(ID_UART, (VB)'\r', TMO_FEVR);
            }
            #endif
            flexcan_sendframe();
            /* △△△ SNY CANドライバを送受信するため追加 chg △△△ */
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

    (void)DDR_UART_INIT_FN(ID_UART, &REG_UART);

    g_mpfID = acre_mpf((T_CMPF *)&cmpf);
    g_mbxID = acre_mbx((T_CMBX *)&cmbx);
    (void)acre_tsk((T_CTSK *)&ctsk_snd);
    (void)acre_tsk((T_CTSK *)&ctsk_rcv);
}
