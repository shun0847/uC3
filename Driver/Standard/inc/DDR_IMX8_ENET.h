/**
 * @file    DDR_IMX8_ENET.h
 * @brief   Ethernet Interface for i.MX8 series(ENET)
 * @date    2022.08.31
 * @author  Copyright (c) 2022, eForce Co., Ltd. All rights reserved.
 *
 ******************************************************************************
 * @par     History
 *          - rev 1.0 (2021.02.09)
 *              Created
 *          - rev 1.1 (2021.05.12)
 *              Improved driver speed performance.
 *          - rev 1.2 (2022.08.31)
 *              i.MX8 QuadMax support
 ******************************************************************************
 */
#ifndef DDR_IMX8_ENET_H_
#define DDR_IMX8_ENET_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Ethernet MAC controller registers (ENET)
 */
struct t_enet {
    UB fill1[4];           /* Reserved space                           */
    UW EIR;                /* Interrupt Event Register                 */
    UW EIMR;               /* Interrupt Mask Register                  */
    UB fill2[4];           /* Reserved space                           */
    UW RDAR;               /* Receive Descriptor Active Register       */
    UW TDAR;               /* Transmit Descriptor Active Register      */
    UB fill3[12];          /* Reserved space                           */
    UW ECR;                /* Ethernet Control Register                */
    UB fill4[24];          /* Reserved space                           */
    UW MMFR;               /* MII Management Frame Register            */
    UW MSCR;               /* MII Speed Control Register               */
    UB fill5[28];          /* Reserved space                           */
    UW MIBC;               /* MIB Control Register                     */
    UB fill6[28];          /* Reserved space                           */
    UW RCR;                /* Receive Control Register                 */
    UB fill7[60];          /* Reserved space                           */
    UW TCR;                /* Transmit Control Register                */
    UB fill8[28];          /* Reserved space                           */
    UW PALR;               /* Physical Address Lower Register          */
    UW PAUR;               /* Physical Address Upper Register          */
    UW OPD;                /* Opcode/Pause Duration Register           */
    UB fill9[40];          /* Reserved space                           */
    UW IAUR;               /* Descriptor Individual Upper Address Register */
    UW IALR;               /* Descriptor Individual Lower Address Register */
    UW GAUR;               /* Descriptor Group Upper Address Register  */
    UW GALR;               /* Descriptor Group Lower Address Register  */
    UB fill10[28];         /* Reserved space                           */
    UW TFWR;               /* Transmit FIFO Watermark Register         */
    UB fill11[56];         /* Reserved space                           */
    UW RDSR;               /* Receive Descriptor Ring Start Register   */
    UW TDSR;               /* Transmit Buffer Descriptor Ring Start Register */
    UW MRBR;               /* Maximum Receive Buffer Size Register     */
    UB fill12[4];          /* Reserved space                           */
    UW RSFL;               /* Receive FIFO Section Full Threshold      */
    UW RSEM;               /* Receive FIFO Section Empty Threshold     */
    UW RAEM;               /* Receive FIFO Almost Empty Threshold      */
    UW RAFL;               /* Receive FIFO Almost Full Threshold       */
    UW TSEM;               /* Transmit FIFO Section Empty Threshold    */
    UW TAEM;               /* Transmit FIFO Almost Empty Threshold     */
    UW TAFL;               /* Transmit FIFO Almost Full Threshold      */
    UW TIPG;               /* Transmit Inter-Packet Gap                */
    UW FTRL;               /* Frame Truncation Length                  */
    UB fill13[12];         /* Reserved space                           */
    UW TACC;               /* Transmit Accelerator Function Configuration */
    UW RACC;               /* Receive Accelerator Function Configuration */
    UB fill14[56];         /* Reserved space                           */
    UW RMON_T_DROP;        /* Count of frames not counted correctly    */
    UW RMON_T_PACKETS;     /* RMON Tx packet count                     */
    UW RMON_T_BC_PKT;      /* RMON Tx Broadcast Packets                */
    UW RMON_T_MC_PKT;      /* RMON Tx Multicast Packets                */
    UW RMON_T_CRC_ALIGN;   /* RMON Tx Packets w CRC/Align error        */
    UW RMON_T_UNDERSIZE;   /* RMON Tx Packets < 64 bytes, good CRC     */
    UW RMON_T_OVERSIZE;    /* RMON Tx Packets > MAX_FL bytes, good CRC */
    UW RMON_T_FRAG;        /* RMON Tx Packets < 64 bytes, bad CRC      */
    UW RMON_T_JAB;         /* RMON Tx Packets > MAX_FL bytes, bad CRC  */
    UW RMON_T_COL;         /* RMON Tx collision count                  */
    UW RMON_T_P64;         /* RMON Tx 64 byte packets                  */
    UW RMON_T_P65TO127;    /* RMON Tx 65 to 127 byte packets            */
    UW RMON_T_P128TO255;   /* RMON Tx 128 to 255 byte packets          */
    UW RMON_T_P256TO511;   /* RMON Tx 256 to 511 byte packets          */
    UW RMON_T_P512TO1023;  /* RMON Tx 512 to 1023 byte packets         */
    UW RMON_T_P1024TO2047; /* RMON Tx 1024 to 2047 byte packets        */
    UW RMON_T_P_GTE2048;   /* RMON Tx packets w > 2048 bytes           */
    UW RMON_T_OCTETS;      /* RMON Tx Octets                           */
    UW IEEE_T_DROP;        /* Count of frames not counted correctly    */
    UW IEEE_T_FRAME_OK;    /* Frames Transmitted OK                    */
    UW IEEE_T_1COL;        /* Frames Transmitted with Single Collision */
    UW IEEE_T_MCOL;        /* Frames Transmitted with Multiple Collisions  */
    UW IEEE_T_DEF;         /* Frames Transmitted after Deferral Delay  */
    UW IEEE_T_LCOL;        /* Frames Transmitted with Late Collision   */
    UW IEEE_T_EXCOL;       /* Frames Transmitted with Excessive Collisions */
    UW IEEE_T_MACERR;      /* Frames Transmitted with Tx FIFO Underrun */
    UW IEEE_T_CSERR;       /* Frames Transmitted with Carrier Sense Error  */
    UW IEEE_T_SQE;         /* Frames Transmitted with SQE Error        */
    UW IEEE_T_FDXFC;       /* Flow Control Pause frames transmitted    */
    UW IEEE_T_OCTETS_OK;   /* Octet count for Frames Transmitted w/o Error */
    UB fill15[12];         /* Reserved space                           */
    UW RMON_R_PACKETS;     /* RMON Rx packet count                     */
    UW RMON_R_BC_PKT;      /* RMON Rx Broadcast Packets                */
    UW RMON_R_MC_PKT;      /* RMON Rx Multicast Packets                */
    UW RMON_R_CRC_ALIGN;   /* RMON Rx Packets w CRC/Align error        */
    UW RMON_R_UNDERSIZE;   /* RMON Rx Packets < 64 bytes, good CRC     */
    UW RMON_R_OVERSIZE;    /* RMON Rx Packets > MAX_FL bytes, good CRC */
    UW RMON_R_FRAG;        /* RMON Rx Packets < 64 bytes, bad CRC      */
    UW RMON_R_JAB;         /* RMON Rx Packets > MAX_FL bytes, bad CRC  */
    UW RMON_R_RESVD_0;     /* Reserved                                 */
    UW RMON_R_P64;         /* RMON Rx 64 byte packets                  */
    UW RMON_R_P65TO127;    /* RMON Rx 65 to 127 byte packets           */
    UW RMON_R_P128TO255;   /* RMON Rx 128 to 255 byte packets          */
    UW RMON_R_P256TO511;   /* RMON Rx 256 to 511 byte packets          */
    UW RMON_R_P512TO1023;  /* RMON Rx 512 to 1023 byte packets         */
    UW RMON_R_P1024TO2047; /* RMON Rx 1024 to 2047 byte packets        */
    UW RMON_R_P_GTE2048;   /* RMON Rx packets w > 2048 bytes           */
    UW RMON_R_OCTETS;      /* RMON Rx Octets                           */
    UW RMON_R_DROP;        /* Count of frames not counted correctly    */
    UW RMON_R_FRAME_OK;    /* Frames Received OK                       */
    UW IEEE_R_CRC;         /* Frames Received with CRC Error           */
    UW IEEE_R_ALIGN;       /* Frames Received with Alignment Error     */
    UW IEEE_R_MACERR;      /* Receive Fifo Overflow count              */
    UW IEEE_R_FDXFC;       /* Flow Control Pause frames received       */
    UW IEEE_R_OCTETS_OK;   /* Octet count for Frames Rcvd w/o Error    */
    UB fill16[284];        /* Reserved space                           */
    UW ATCR;               /* Timer Control Register                   */
    UW ATVR;               /* Timer Value Register                     */
    UW ATOFF;              /* Timer Offset Register                    */
    UW ATPER;              /* Timer Period Register                    */
    UW ATCOR;              /* Timer Correction Register                */
    UW ATINC;              /* Time-Stamping Clock Period Register      */
    UW ATSTMP;             /* Timestamp of Last Transmitted Frame      */
#ifdef CPU_IMX_8M_QUADMAX
    UB fill17[356];        /* Reserved space                           */
    UW MDATA;              /* Pattern Match Data Register              */
    UW MMASK;              /* Match Entry Mask Register                */
    UW MCONFIG;            /* Match Entry Rules Configuration Register */
    UW MENTRYRW;           /* Match Entry Read/Write Command Register  */
    UW RXPCTL;             /* Receive Parser Control Register          */
    UW MAXFRMOFF;          /* Maximum Frame Offset                     */
    UW RXPARST;            /* Receive Parser Status                    */
    UW PARSDSCD;           /* Parser Discard Count                     */
    UB fill18[4];          /* Reserved space                           */
    UW PRSACPT0;           /* Parser Accept Count 0                    */
    UW PRSRJCT0;           /* Parser Reject Count 0                    */
    UW PRSACPT1;           /* Parser Accept Count 1                    */
    UW PRSRJCT1;           /* Parser Reject Count 1                    */
    UW PRSACPT2;           /* Parser Accept Count 2                    */
    UW PRSRJCT2;           /* Parser Reject Count 2                    */
    UB fill19[72];         /* Reserved space                           */
#else /* CPU_IMX_8M_QUADMAX */
    UB fill17[488];        /* Reserved space                           */
#endif /* CPU_IMX_8M_QUADMAX */
    UW TGSR;               /* Timer Global Status Register             */
    struct {
        UW TCSR; /* Timer Control Status Register            */
        UW TCCR; /* Timer Compare Capture Register           */
    } CHANNEL[4];
};

/*
 * Ethernet interrupt event register
 */

#define ENET_EIR_BABR       0x40000000U     /* Babbling receive error */
#define ENET_EIR_BABT       0x20000000U     /* Babbling transmit error */
#define ENET_EIR_GRA        0x10000000U     /* Graceful stop complete */
#define ENET_EIR_TXF        0x08000000U     /* Transmit frame interrupt */
#define ENET_EIR_TXB        0x04000000U     /* Transmit buffer interrupt */
#define ENET_EIR_RXF        0x02000000U     /* Receive frame interrupt */
#define ENET_EIR_RXB        0x01000000U     /* Receive buffer interrupt */
#define ENET_EIR_MII        0x00800000U     /* MII interrupt */
#define ENET_EIR_EBERR      0x00400000U     /* Ethernet bus error */
#define ENET_EIR_LC         0x00200000U     /* Late collision */
#define ENET_EIR_RL         0x00100000U     /* Collision retry error */
#define ENET_EIR_UN         0x00080000U     /* Transmit FIFO underrun */
#define ENET_EIR_PLR        0x00040000U     /* Payload receive error */
#define ENET_EIR_WAKEUP     0x00020000U     /* Node wake-up request indication */
#define ENET_EIR_TS_AVAIL   0x00010000U     /* Transmit timestamp available */
#define ENET_EIR_TS_TIMER   0x00008000U     /* Timestamp timer */

#define ENET_EIR_RXFLUSH_2	0x00004000U		/* RX DMA Ring 2 flush indication.  */
#define ENET_EIR_RXFLUSH_1	0x00002000U		/* RX DMA Ring 1 flush indication.  */
#define ENET_EIR_RXFLUSH_0	0x00001000U		/* RX DMA Ring 0 flush indication.  */
#ifdef CPU_IMX_8M_QUADMAX
#define ENET_EIR_PARSERR    0x00000400U		/* Receive parser error */
#define ENET_EIR_PARSRF     0x00000200U		/* Receive frame rejected */
#endif /* CPU_IMX_8M_QUADMAX */
#define ENET_EIR_TXF2		0x00000080U		/* Transmit frame interrupt, class 2  */
#define ENET_EIR_TXB2		0x00000040U		/* Transmit buffer interrupt, class 2 */
#define ENET_EIR_RXF2		0x00000020U		/* Receive frame interrupt, class 2 */
#define ENET_EIR_RXB2		0x00000010U		/* Receive buffer interrupt, class 2 */
#define ENET_EIR_TXF1		0x00000008U		/* Transmit frame interrupt, class 1 */
#define ENET_EIR_TXB1		0x00000004U		/* Transmit buffer interrupt, class 1 */


/*
 * Receive descriptor active register
 * Transmit descriptor active register
 */

#define ENET_DES_ACTIVE 0x01000000U /* Descriptor active */

/*
 * Ethernet control register
 */

#define ENET_ECR_DBSWP 0x00000100U   /* DBSWP  */
#define ENET_ECR_STOPEN 0x00000080U  /* STOPEN Signal control */
#define ENET_ECR_DBGEN 0x00000040U   /* Debug enable */
#define ENET_ECR_SPEED 0x00000020U   /* 1000Mbps mode */
#define ENET_ECR_EN1588 0x00000010U  /* EN1588 enable */
#define ENET_ECR_SLEEP 0x00000008U   /* Sleep mode enable */
#define ENET_ECR_MAGICEN 0x00000004U /* Magic packet detection enable */
#define ENET_ECR_ETHEREN 0x00000002U /* Ethernet enable */
#define ENET_ECR_RESET 0x00000001U   /* Ethernet MAC reset */

/*
 * MII management frame register
 */

#define ENET_MMFR_ST 0x40000000U      /* Start of frame delimiter(only 01) */
#define ENET_MMFR_OP_R 0x20000000U    /* Operation code: 10(read) */
#define ENET_MMFR_OP_W 0x10000000U    /* Operation code: 01(write) */
#define ENET_MMFR_PA_SHIFT 23U        /* PHY address (shift bit) */
#define ENET_MMFR_RA_SHIFT 18U        /* Register address (shift bit) */
#define ENET_MMFR_TA 0x00020000U      /* Turn around (only 10) */
#define ENET_MMFR_DA_MASK 0x0000ffffU /* Data bit mask */

/*
 * MII speed control register
 */

#define ENET_MSCR_HOLDT_SHIFT 8U     /* Holdtime on MDIO output (shift bit) */
#define ENET_MSCR_DISPRE 0x00000008U /* Disable preamble */
#define ENET_MSCR_SPD_SHIFT 1U       /* MII speed (shift bit) */

/*
 * MIB control register
 */

#define ENET_MIBC_DIS 0x80000000U   /* Disable MIB logic */
#define ENET_MIBC_IDLE 0x40000000U  /* MIB idle */
#define ENET_MIBC_CLEAR 0x20000000U /* MIB clear */

/*
 * Receive control register
 */

#define ENET_RCR_GRS 0x80000000U    /* Graceful receive stopped */
#define ENET_RCR_NLC 0x40000000U    /* Payload length check disable */
#define ENET_RCR_MAXFL_SHIFT 16     /* Maximum frame length */
#define ENET_RCR_CFEN 0x00008000U   /* MAC control frame enable */
#define ENET_RCR_CRCFWD 0x00004000U /* Terminate/forward received CRC */
#define ENET_RCR_PAUFWD 0x00002000U /* Terminate/forward pause frames */
#define ENET_RCR_PADEN                                                         \
    0x00001000U /* Enable frame padding remove on receive  \                   \
                   */
#define ENET_RCR_RMII_10T 0x00000200U  /* Enables 10Mbps mode of the RMII */
#define ENET_RCR_RMII_MODE 0x00000100U /* RMII mode enable */
#define ENET_RCR_RGMII_EN 0x00000040U  /* RGMII mode enable */
#define ENET_RCR_FCE 0x00000020U       /* Flow control enable */
#define ENET_RCR_BCREJ 0x00000010U     /* Broadcast frame reject */
#define ENET_RCR_PROM 0x00000008U      /* Promiscuous mode */
#define ENET_RCR_MII_MODE 0x00000004U  /* 0=reserved 1=MII or RMII mode */
#define ENET_RCR_DRT                                                           \
    0x00000002U /* Disable receive on transmit (0=full-duplex or xmit monitor  \
                   \                                                           \
                   1=half-duplex)              */
#define ENET_RCR_LOOP 0x00000001U /* internal loopback */

/*
 * Transmit control register
 */

#define ENET_TCR_CRCFWD                                                        \
    0x00000200U /* Forward frame from application with CRC */
#define ENET_TCR_ADDINS 0x00000100U /* Set MAC address on transmit */
#define ENET_TCR_ADDSEL                                                        \
    0x00000000U /* Source MAC address select on transmit   \                   \
                   */
#define ENET_TCR_RFC_PAUSE 0x00000010U /* Receive frame control pause */
#define ENET_TCR_TFC_PAUSE 0x00000008U /* Transmit frame control pause */
#define ENET_TCR_FDEN 0x00000004U      /* Full duplex enable */
#define ENET_TCR_GTS 0x00000001U       /* Graceful transmit stop */

/*
 * Transmit FIFO watermark register
 */

#define ENET_TFWR_STRFWD 0x00000100U /* Store and forward enable */

/*
 * Transmit accelerator function configuration
 */

#define ENET_TACC_PROCHK                                                       \
    0x00000010U /* Enables insertion of protocol checksum */
#define ENET_TACC_IPCHK                                                        \
    0x00000008U /* Enables insertion of IP header checksum */
#define ENET_TACC_SHIFT16 0x00000001U /* TX FIFO shift-16 */

/*
 * Recive accelerator function configuration
 */

#define ENET_RACC_SHIFT16 0x00000080U /* RX FIFO shift-16 */
#define ENET_RACC_LINEDIS                                                      \
    0x00000040U /* Enable discard of frames with MAC layer errors */
#define ENET_RACC_PRODIS                                                       \
    0x00000004U /* Enable discard of frames with wrong protocol checksum  */
#define ENET_TACC_IPDIS                                                        \
    0x00000002U /* Enable discard of frames with wrong IPv4 header checksum */
#define ENET_TACC_PADREM                                                       \
    0x00000001U /* Enable padding removal for short IP frames */

/*
 * Timer control register
 */

#define ENET_ATCR_SLAVE 0x00002000U   /* Enable timer slave mode */
#define ENET_ATCR_CAPTURE 0x00000800U /* Capture timer value */
#define ENET_ATCR_RESTART 0x00000200U /* Reset timer */
#define ENET_ATCR_PINPER                                                       \
    0x00000080U /* Enables event signal output assertion on period event  */
#define ENET_ATCR_PEREN 0x00000010U  /* Enable periodical event */
#define ENET_ATCR_OFFRST 0x00000008U /* Reset timer on offset event */
#define ENET_ATCR_OFFEN 0x00000004U  /* Enable one-short offset event */
#define ENET_ATCR_EN 0x00000001U     /* Enable timer */

#define ENETDMA_BIGENDIAN   0       /* enetdma endian support (0:little, 1:big) */
#if ENETDMA_BIGENDIAN
/*
 * Receive descriptor bit field
 */

#define RX_BD_E 0x0080U            /* Empty */
#define RX_BD_RO1 0x0040U          /* software ownership */
#define RX_BD_W 0x0020U            /* Wrap */
#define RX_BD_TO2 0x0010U          /* software ownership */
#define RX_BD_L 0x0008U            /* Last in frame */
#define RX_BD_M 0x0001U            /* Miss */
#define RX_BD_BC 0x8000U           /* broadcast */
#define RX_BD_MC 0x4000U           /* multicast */
#define RX_BD_LG 0x2000U           /* Rx frame length violation */
#define RX_BD_NO 0x1000U           /* Receive non-octet aligned frame */
#define RX_BD_CR 0x0400U           /* CRC error */
#define RX_BD_OV 0x0200U           /* Overrun */
#define RX_BD_TR 0x0100U           /* truncated */

/*
 * Transmit descriptor bit field
 */

#define TX_BD_R 0x0080U            /* Ready */
#define TX_BD_TO1 0x0040U          /* software ownership */
#define TX_BD_W 0x0020U            /* Warp */
#define TX_BD_TO2 0x0010U          /* software ownership */
#define TX_BD_L 0x0008U            /* Last in frame */
#define TX_BD_TC 0x0004U           /* Tx CRC */
#define TX_BD_ABC 0x0002U          /* Append bad CRC */

#else
/*
 * Receive descriptor bit field
 */

#define RX_BD_E 0x8000U   /* Empty */
#define RX_BD_RO1 0x4000U /* software ownership */
#define RX_BD_W 0x2000U   /* Wrap */
#define RX_BD_TO2 0x1000U /* software ownership */
#define RX_BD_L 0x0800U   /* Last in frame */
#define RX_BD_M 0x0100U   /* Miss */
#define RX_BD_BC 0x0080U  /* broadcast */
#define RX_BD_MC 0x0040U  /* multicast */
#define RX_BD_LG 0x0020U  /* Rx frame length violation */
#define RX_BD_NO 0x0010U  /* Receive non-octet aligned frame */
#define RX_BD_CR 0x0004U  /* CRC error */
#define RX_BD_OV 0x0002U  /* Overrun */
#define RX_BD_TR 0x0001U  /* truncated */

/*
 * Transmit descriptor bit field
 */

#define TX_BD_R 0x8000U   /* Ready */
#define TX_BD_TO1 0x4000U /* software ownership */
#define TX_BD_W 0x2000U   /* Warp */
#define TX_BD_TO2 0x1000U /* software ownership */
#define TX_BD_L 0x0800U   /* Last in frame */
#define TX_BD_TC 0x0400U  /* Tx CRC */
#define TX_BD_ABC 0x0200U /* Append bad CRC */

#endif

#ifdef __cplusplus
}
#endif
#endif /* DDR_IMX8_ENET_H_ */
