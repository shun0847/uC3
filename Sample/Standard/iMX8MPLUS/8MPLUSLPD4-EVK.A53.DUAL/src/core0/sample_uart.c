/**
 * @brief   Serial sample program for 8MPLUSLPD4-EVK (i.MX8M Plus, Cortex-A53)
 * @date    2025.09.29
 *
 * @copyright (C) 2025, eForce Co., Ltd.
 */
#include <string.h>
#include "kernel.h"
#include "DDR_COM.h"
#include "sample_uart_cfg.h"
#include "GL_kernel_id.h"

/* Private function prototypes -----------------------------------------------*/

static void uart_send_task(VP_INT exinf);
static void uart_recv_task(VP_INT exinf);

/* Private typedef -----------------------------------------------------------*/

/* User input message */
typedef struct t_msgblk {
    T_MSG header;
    UINT  cnt;
    VB    buf[120];
} T_MSGBLK;

/* Private variables ---------------------------------------------------------*/

static VB const *const  core1_msg1 = "Core1 task is active...\r\n";
static VB const *const  core1_msg2 = "Core1 task is sleeping...\r\n";
static const T_COM_SMOD uart_ini = {CFG_BAUDRATE, CFG_BLEN, CFG_PAR, CFG_SBIT, CFG_FLW};
static VB const *const  banner_str = "\n\r\teForce Operating System Sample Program\r\n";


/**
 * UART send task
 */
static void uart_send_task(VP_INT exinf)
{
    UINT      txcnt;
    T_MSGBLK *blk = 0;
    VB        chr;
    ER        ercd;
    FLGPTN    flgptn;

    (void)ini_com(ID_UART, &uart_ini);
    (void)ctr_com(ID_UART, STA_COM, 0);

    (void)ctr_com(ID_UART, SND_BRK, 100);
    txcnt = strlen(banner_str);
    (void)puts_com(ID_UART, (VB *)banner_str, &txcnt, TMO_FEVR);

    /*
     * Check the task status of core1 with 400ms interval then print a message
     * until the enter key is pushed
     */
    while (1) {
        ercd = getc_com(ID_UART, &chr, 0, 10);
        if (ercd == E_OK) {
            (void)putc_com(ID_UART, chr, TMO_FEVR);
            if (chr == (VB)'\r') {
                (void)putc_com(ID_UART, (VB)'\n', TMO_FEVR);
            }
            break;
        }
        (void)dly_tsk(390U);
        ercd = vpol_flg(ID_CORE1, Core1FlagID, 0x0001U, TWF_ORW, &flgptn);
        if (ercd == E_OK) {
            txcnt = strlen(core1_msg1);
            (void)puts_com(ID_UART, (VB *)core1_msg1, &txcnt, TMO_FEVR);
        } else {
            txcnt = strlen(core1_msg2);
            (void)puts_com(ID_UART, (VB *)core1_msg2, &txcnt, TMO_FEVR);
        }
    }

    (void)snd_mbx(MbxID, (T_MSG *)blk);
    (void)tslp_tsk(1U);
    for (;;) {
        (void)rcv_mbx(MbxID, (T_MSG **)&blk);
        (void)puts_com(ID_UART, blk->buf, &blk->cnt, TMO_FEVR);
        (void)rel_mpf(MpfID, (VP)blk);
        (void)ctr_com(ID_UART, CLN_TXBUF, 100);
    }
}

/**
 * UART receive task
 */
static void uart_recv_task(VP_INT exinf)
{
    T_MSGBLK *blk = 0;

    (void)rcv_mbx(MbxID, (T_MSG **)&blk);
    for (;;) {
        (void)get_mpf(MpfID, (VP *)&blk);
        blk->cnt = sizeof(blk->buf) - 1U;
        (void)gets_com(ID_UART, blk->buf, 0, '\r', &blk->cnt, TMO_FEVR);
        blk->buf[blk->cnt] = '\n';
        blk->cnt++;
        (void)snd_mbx(MbxID, (T_MSG *)blk);
    }
}

/**
 * Wakeup task
 */
void WupTask(VP_INT exinf)
{
    ER     ercd;
    FLGPTN flgptn;

    while (1) {
        (void)dly_tsk(1000U);
        ercd = vpol_flg(ID_CORE1, Core1FlagID, 0x0001U, TWF_ORW, &flgptn);
        if (ercd == E_TMOUT) {
            (void)vwup_tsk((ID)ID_CORE1, Core1TaskID);
        }
    }
}

/*
 * Start the UART sample.
 */

const T_CMPF cmpf = {TA_TFIFO, 20U, sizeof(T_MSGBLK), 0, "Mpf"};
const T_CMBX cmbx = {TA_TFIFO | TA_MFIFO, 0, 0, "Mbx"};
const T_CTSK ctsk_snd = {TA_HLNG | TA_ACT | TA_FPU, (VP_INT)NULL, (FP)uart_send_task, 4, 0x800U, (VP)NULL,
                         "uart_send_task"};
const T_CTSK ctsk_rcv = {TA_HLNG | TA_ACT | TA_FPU, (VP_INT)NULL, (FP)uart_recv_task, 5, 0x800U, (VP)NULL,
                         "uart_recv_task"};
const T_CTSK ctsk_wup = {TA_HLNG | TA_ACT | TA_FPU, (VP_INT)NULL, (FP)WupTask, 8, 0x800, (VP)NULL, "WupTask"};

void sample_uart_start(void)
{
    (void)DDR_UART_INIT_FN(ID_UART, &REG_UART);

    MpfID = acre_mpf((T_CMPF *)&cmpf);
    MbxID = acre_mbx((T_CMBX *)&cmbx);
    SndTaskID = acre_tsk((T_CTSK *)&ctsk_snd);
    RcvTaskID = acre_tsk((T_CTSK *)&ctsk_rcv);
    WupTaskID = acre_tsk((T_CTSK *)&ctsk_wup);
}
