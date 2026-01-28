/**
 * @brief   Micro C Cube Standard, DEVICE DRIVER
 *          Serial Interface for Freescale i.MX6 series, i.MX7 series
 * @date    2019.11.22
 *
 * @copyright (c) 2010-2019, eForce Co., Ltd. All rights reserved.
 */
#ifndef IMX_UART_H_
#define IMX_UART_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * UART register
 */
struct t_uart {
    UW  URXD;       /**< UART Receiver Register */
    UW  reserved_0[15];
    UW  UTXD;       /**< UART Transmitter Register */
    UW  reserved_1[15];
    UW  UCR1;       /**< UART Control Register 1 */
    UW  UCR2;       /**< UART Control Register 2 */
    UW  UCR3;       /**< UART Control Register 3 */
    UW  UCR4;       /**< UART Control Register 4 */
    UW  UFCR;       /**< UART FIFO Control Register */
    UW  USR1;       /**< UART Status Register 1 */
    UW  USR2;       /**< UART Status Register 2 */
    UW  UESC;       /**< UART Escape Character Register */
    UW  UTIM;       /**< UART Escape Timer Register */
    UW  UBIR;       /**< UART BRM Incremental Register */
    UW  UBMR;       /**< UART BRM Modulator Register */
    UW  UBRC;       /**< UART Baud Rate Count Register */
    UW  ONEMS;      /**< UART One Millisecond Register */
    UW  UTS;        /**< UART Test Register */
    UW  UMCR;       /**< UART RS-485 Mode Control Register */
};

typedef union t_imx_uart_msts {
    UH      word;
    struct {
        UH      init_flg:1;
        UH      ena_tx:1;
        UH      ena_rx:1;
        UH      sft_flw:1;
        UH      hrd_flw:1;
        UH      sns_brk:1;
        UH      tx_xoff:1;
        UH      rx_xoff:1;
        UH      req_xon_xoff:1;
        UH      er_buf_ovr:1;
        UH      dummy:6;
    } bit;
} T_IMX_UART_MSTS;

typedef struct t_imx_uart_mng {
    T_IMX_UART_MSTS status;
    UH              flgid;
    UH              sndp;
    UH              rcvp;
    UH              tcnt;
    UH              rcnt;
    UH              tsize;
    UH              rsize;
    UH              tlockid;
    UH              rlockid;
    UH              xoff_size;
    UH              xon_size;
    UH              isrid;
    UH              intno;
    UH              wclnid;
    FP              devhdr;
    volatile struct t_uart *port;
    T_COM_SND       *SndData;
    T_COM_RCV       *RcvData;
    VB              *tbuf;
    VB              *rbuf;
    UB              *sbuf;
    UH              aux[2];
} T_IMX_UART_MNG;

#define TXI_FLG     0x00000001U
#define RXI_FLG     0x00000002U
#define TEI_FLG     0x00000004U

/* Bit definitions for UART receiver register (upper-byte)                              */

#define URXD_CHARRDY    0x80U       /* Character Ready                                  */
#define URXD_ERR        0x40U       /* Error Detect                                     */
#define URXD_OVERRUN    0x20U       /* Receiver Overrun                                 */
#define URXD_FRMERR     0x10U       /* Frame Error                                      */
#define URXD_BRK        0x08U       /* BREAK Detect                                     */
#define URXD_PRERR      0x04U       /* Parity Error                                     */

/* Bit definitions for UART Control Register 1                                          */

#define UCR1_ADEN       0x8000U     /* Automatic Baud Rate Detection Interrupt Enable   */
#define UCR1_ADBR       0x4000U     /* Automatic Detection of Baud Rate                 */
#define UCR1_TRDYEN     0x2000U     /* Transmitter Ready Interrupt Enable               */
#define UCR1_IDEN       0x1000U     /* Idle Condition Detected Interrupt Enable         */
#define UCR1_ICD4       0x0000U     /* Idle Condition Detect if Idle for more than 4 frames     */
#define UCR1_ICD8       0x0400U     /* Idle Condition Detect if Idle for more than 8 frames     */
#define UCR1_ICD16      0x0800U     /* Idle Condition Detect if Idle for more than 16 frames    */
#define UCR1_ICD32      0x0C00U     /* Idle Condition Detect if Idle for more than 32 frames    */
#define UCR1_RRDYEN     0x0200U     /* Receiver Ready Interrupt Enable                  */
#define UCR1_RDMAEN     0x0100U     /* Receiver Ready DMA Enable                        */
#define UCR1_IREN       0x0080U     /* Infrared Interface Enable                        */
#define UCR1_TXMPTYEN   0x0040U     /* Transmitter Empty Interrupt Enable               */
#define UCR1_RTSDEN     0x0020U     /* RTS Delta Interrupt Enable                       */
#define UCR1_SNDBRK     0x0010U     /* Send Break                                       */
#define UCR1_TXDMAEN    0x0008U     /* Transmitter Ready DMA Enable                     */
#define UCR1_ATDMAEN    0x0004U     /* Aging DMA Timer Enable                           */
#define UCR1_DOZE       0x0002U     /* DOZE state                                       */
#define UCR1_UARTEN     0x0001U     /* UART Enable                                      */

/* Bit definitions for UART Control Register 2                                          */

#define UCR2_ESCI       0x8000U     /* Escape Sequence Interrupt Enable                 */
#define UCR2_IRTS       0x4000U     /* Ignore RTS Pin                                   */
#define UCR2_CTSC       0x2000U     /* CTS Pin Control                                  */
#define UCR2_CTS        0x1000U     /* Clear to Send                                    */
#define UCR2_ESCEN      0x0800U     /* Escape Enable                                    */
#define UCR2_RTEC_ANY   0x0400U     /* Request to Send Edge Control on any edge         */
#define UCR2_RTEC_FALL  0x0200U     /* Request to Send Edge Control on a falling edge   */
#define UCR2_RTEC_RISE  0x0000U     /* Request to Send Edge Control on a rising edge    */
#define UCR2_PREN       0x0100U     /* Parity Enable                                    */
#define UCR2_PROE       0x0080U     /* Parity Even(0)/Odd(1)                            */
#define UCR2_STPB       0x0040U     /* Stop 1-bit(0)/2-bits(1)                          */
#define UCR2_WS         0x0020U     /* Word Size 7-bits(0)/8-bits(1)                    */
#define UCR2_RTSEN      0x0010U     /* Request to Send Interrupt Enable                 */
#define UCR2_ATEN       0x0008U     /* Aging Timer Enable                               */
#define UCR2_TXEN       0x0004U     /* Transmitter Enable                               */
#define UCR2_RXEN       0x0002U     /* Receiver Enable                                  */
#define UCR2_SRST       0x0001U     /* Software Reset (active low)                      */

/* Bit definitions for UART Control Register 3                                          */

#define UCR3_DPEC_EITH  0x8000U     /* DTR Interrupt Edge Control on either edge        */
#define UCR3_DPEC_FALL  0x4000U     /* DTR Interrupt Edge Control on a falling edge     */
#define UCR3_DPEC_RISE  0x0000U     /* DTR Interrupt Edge Control on a rising edge      */
#define UCR3_DTREN      0x2000U     /* Data Terminal Ready Interrupt Enable             */
#define UCR3_PARERREN   0x1000U     /* Parity Error Interrupt Enable                    */
#define UCR3_FRAERREN   0x0800U     /* Frame Error Interrupt Enable                     */
#define UCR3_DSR        0x0400U     /* Data Set Ready                                   */
#define UCR3_DCD        0x0200U     /* Data Carrier Detect (only DCE mode)              */
#define UCR3_RI         0x0100U     /* Ring Indicator (only DCE mode)                   */
#define UCR3_ADNIMP     0x0080U     /* Autobaud Detection Not Improved                  */
#define UCR3_RXDSEN     0x0040U     /* Receive Status Interrupt Enable                  */
#define UCR3_AIRINTEN   0x0020U     /* Asynchronous IR WAKE Interrupt Enable            */
#define UCR3_AWAKEN     0x0010U     /* Asynchronous WAKE Interrupt Enable               */
#define UCR3_RXDMUXSEL  0x0004U     /* RXD Muxed Input Selected                         */
#define UCR3_INVT       0x0002U     /* Inverted Infrared transmission                   */
#define UCR3_ACIEN      0x0001U     /* Autobaud Counter Interrupt Enable                */

/* Bit definitions for UART Control Register 4                                          */

#define UCR4_INVR       0x0200U     /* Inverted Infrared Reception                      */
#define UCR4_ENIRI      0x0100U     /* Serial Infrared Interrupt Enable                 */
#define UCR4_WKEN       0x0080U     /* WAKE Interrupt Enable                            */
#define UCR4_IRSC       0x0020U     /* IR Special Case                                  */
#define UCR4_LPBYP      0x0010U     /* Low Power Bypass                                 */
#define UCR4_TCEN       0x0008U     /* Transmit Complete Interrupt Enable               */
#define UCR4_BKEN       0x0004U     /* BREAK Condition Detected Interrupt Enable        */
#define UCR4_OREN       0x0002U     /* Receiver Overrun Interrupt Enable                */
#define UCR4_DREN       0x0001U     /* Receiver Data Ready Interrupt Enable             */

/* Bit definitions for UART Status Register 1                                           */

#define USR1_PARITYERR  0x8000U     /* Parity Error Interrupt Flag                      */
#define USR1_RTSS       0x4000U     /* RTS Pin Status                                   */
#define USR1_TRDY       0x2000U     /* Transmitter Ready Interrupt/DMA Flag             */
#define USR1_RTSD       0x1000U     /* RTS Delta                                        */
#define USR1_ESCF       0x0800U     /* Escape Sequence Interrupt Flag                   */
#define USR1_FRAMERR    0x0400U     /* Frame Error Interrupt Flag                       */
#define USR1_RRDY       0x0200U     /* Receiver Ready Interrupt/DMA Flag                */
#define USR1_AGTIM      0x0100U     /* Ageing Timer Interrupt Flag                      */
#define USR1_RXDS       0x0040U     /* Receiver IDLE Interrupt Flag                     */
#define USR1_AIRINT     0x0020U     /* Asynchronous IR WAKE Interrupt Flag              */
#define USR1_AWAKE      0x0010U     /* Asynchronous WAKE Interrupt Flag                 */

/* Bit definitions for UART Status Register 2                                           */

#define USR2_ADET       0x8000U     /* Automatic Baud Rate Detect complete              */
#define USR2_TXFE       0x4000U     /* Transmit Buffer FIFO Empty                       */
#define USR2_DTRF       0x2000U     /* DTR edge triggered interrupt Flag                */
#define USR2_IDLE       0x1000U     /* Idle Condition                                   */
#define USR2_ACST       0x0800U     /* Autobaud Counter Stopped                         */
#define USR2_RIDELT     0x0400U     /* Ring Indicator Delta                             */
#define USR2_RIIN       0x0200U     /* Ring Indicator Input                             */
#define USR2_IRINT      0x0100U     /* Serial Infrared Interrupt Flag                   */
#define USR2_WAKE       0x0080U     /* WAKE                                             */
#define USR2_DCDDELT    0x0040U     /* Data Carrier Detect Delta                        */
#define USR2_DCDIN      0x0020U     /* Data Carrier Detect Input                        */
#define USR2_RTSF       0x0010U     /* RTS Edge Triggered Interrupt Flag                */
#define USR2_TXDC       0x0008U     /* Transmitter Complete                             */
#define USR2_BRCD       0x0004U     /* BREAK Condition Detected                         */
#define USR2_ORE        0x0002U     /* Overrun Error                                    */
#define USR2_RDR        0x0001U     /* Receive Data Ready                               */

/* Bit definitions for UART Test Register                                               */

#define UTS_FRCPERR     0x2000U     /* Force Parity Error                               */
#define UTS_LOOP        0x1000U     /* Loop Tx and Rx for Test                          */
#define UTS_DBGEN       0x0800U     /* Debug Enable                                     */
#define UTS_LOOPIR      0x0400U     /* Loop TX and RX for IR Test                       */
#define UTS_RXDBG       0x0200U     /* RXFIFO Debug Mode                                */
#define UTS_TXEMPTY     0x0040U     /* Tx FIFO Empty                                    */
#define UTS_RXEMPTY     0x0020U     /* Rx FIFO Empty                                    */
#define UTS_TXFULL      0x0010U     /* Tx FIFO Full                                     */
#define UTS_RXFULL      0x0008U     /* Rx FIFO Full                                     */
#define UTS_SOFTRST     0x0001U     /* Software Reset                                   */

extern ER _ddr_imx_uart_init(ID devid, volatile struct t_uart *uart_port);

#ifdef __cplusplus
}
#endif
#endif /* IMX_UART_H_ */
