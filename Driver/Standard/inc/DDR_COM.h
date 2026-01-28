/**
 * @brief Micro C Cube Standard, DEVICE DRIVER
 *        Standard Communication Interface
 * @date  2025.10.24
 *
 * @copyright (C) 2008-2025, eForce Co., Ltd.
 */
#ifndef DDR_COM_H
#define DDR_COM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Definitions of control character                                     */

#define     XON         0x11U
#define     XOFF        0x13U

/* Definitions of chatacter length                                      */

#define     BLEN8       0U      /* 8-bits length                        */
#define     BLEN7       1U      /* 7-bits length                        */
#define     BLEN6       2U      /* 6-bits length                        */
#define     BLEN5       3U      /* 5-bits length                        */

/* Definitions of parity bit                                            */

#define     PAR_NONE    0U      /* None parity                          */
#define     PAR_EVEN    1U      /* Even parity                          */
#define     PAR_ODD     2U      /* Odd                                  */

/* Definitions of stop bit length                                       */

#define     SBIT1       0U      /* 1 stop bit                           */
#define     SBIT15      1U      /* 1.5 stop bits                        */
#define     SBIT2       2U      /* 2 stop bits                          */

/* Definitions of flow control                                          */

#define     FLW_NONE    0U      /* None flow control                    */
#define     FLW_XON     1U      /* Software flow control                */
#define     FLW_HARD    2U      /* Hardware flow control                */

/* Definitions of command code                                          */

#define     RST_COM     0xF800U /* Reset COM Port                       */
#define     CLN_TXBUF   0x8000U /* Clean Tx Buffer                      */
#define     RST_BUF     0x6000U /* Reset both Rx and Tx data buffers    */
#define     RST_TXBUF   0x4000U /* Reset Tx data buffer                 */
#define     RST_RXBUF   0x2000U /* Reset Rx data and status Buffer      */
#define     STP_COM     0x1800U /* Stop both receiver and transmitter   */
#define     STP_TX      0x1000U /* Stop transmitter                     */
#define     STP_RX      0x0800U /* Stop receiver                        */
#define     SND_BRK     0x0400U /* Send line BREAK                      */
#define     STA_COM     0x0300U /* Start both receiver and transmitter  */
#define     STA_TX      0x0200U /* Start transmitter                    */
#define     STA_RX      0x0100U /* Start receiver                       */
#define     LOC_TX      0x0080U /* Lock transmitter                     */
#define     LOC_RX      0x0040U /* Lock receiver                        */
#define     UNL_TX      0x0020U /* Unlock transmitter                   */
#define     UNL_RX      0x0010U /* Unlock receiver                      */
#define     ASR_RTS     0x0008U /* Assert RTS                           */
#define     NGT_RTS     0x0004U /* Negate RTS                           */
#define     ASR_DTR     0x0002U /* Assert DTR                           */
#define     NGT_DTR     0x0001U /* Negate DTR                           */

/* Definitions of control code                                          */

#define     TA_COM_INI  1       /* Initialize communication port        */
#define     TA_COM_REF  2       /* Rrefarence communication port        */
#define     TA_COM_CTR  3       /* Control communication port           */
#define     TA_COM_PUTS 4       /* Transmit character stringth          */
#define     TA_COM_GETS 5       /* Receive character stringth           */

/* Definitions of status code                                           */

#define     T_COM_EROVB 0x0001U /* COM FIFO Overrun error               */
#define     T_COM_EROR  0x0002U /* Rx Buffer Overflow error             */
#define     T_COM_ERP   0x0004U /* Parity error                         */
#define     T_COM_ERF   0x0008U /* Framing error                        */
#define     T_COM_BRK   0x0010U /* line break status                    */
#define     T_COM_TXOFF 0x0020U /* Tx XON/XOFF flow control status      */
#define     T_COM_RXOFF 0x0040U /* Rx XON/XOFF flow control status      */
#define     T_COM_RTS   0x0080U /* RTS control status                   */
#define     T_COM_CTS   0x0100U /* CTS control status                   */
#define     T_COM_DTR   0x0200U /* DTR control status                   */
#define     T_COM_DSR   0x0400U /* DSR control status                   */
#define     T_COM_CD    0x0800U /* CD  control status                   */
#define     T_COM_RI    0x1000U /* RI  control status                   */
#define     T_COM_ENARX 0x2000U /* 1= Rx Enable, 0= Rx Disable          */
#define     T_COM_ENATX 0x4000U /* 1= Tx Enable, 0= Tx Disable          */
#define     T_COM_INIT  0x8000U /* 1= Port Initialized                  */

typedef struct t_com_smod {
    UW          baud;
    UB          blen;
    UB          par;
    UB          sbit;
    UB          flow;
} T_COM_SMOD;

typedef struct t_com_ref {
    UH          rxcnt;
    UH          txcnt;
    UH          status;
} T_COM_REF;

typedef struct t_com_ctr {
    UINT        command;
    TMO         time;
} T_COM_CTR;

typedef struct t_com_snd {
    VB const   *tbuf;
    TMO         time;
    UH          tcnt;
} T_COM_SND;

typedef struct t_com_eos {
    UB          flg[4];
    VB          chr[4];
} T_COM_EOS;

typedef struct t_com_rcv {
    VB          *rbuf;
    UB          *sbuf;
    T_COM_EOS   *eos;
    TMO         time;
    UH          rcnt;
} T_COM_RCV;

ER ini_com(ID, const T_COM_SMOD *);
ER ref_com(ID, T_COM_REF *);
ER ctr_com(ID, UINT, TMO);
ER putc_com(ID, VB, TMO);
ER puts_com(ID, VB const *, UINT *, TMO);
ER getc_com(ID, VB *, UB *, TMO);
ER gets_com(ID, VB *, UB *, INT, UINT *, TMO);

#ifdef __cplusplus
}
#endif

#endif
