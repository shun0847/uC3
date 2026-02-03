/***********************************************************************
    MICRO C CUBE / COMPACT, DEVICE DRIVER
    Ethernet driver for STM32H7xx Series
    Copyright (c)  2020, eForce Co., Ltd. All rights reserved.

    2020/01/27: Created.
 ***********************************************************************/

#ifndef _DDR_ETH_H_
#define _DDR_ETH_H_

#include "COMMONDEF.h"

#ifdef __cplusplus
extern "C" {
#endif

struct t_enetqos {
struct t_enetqos_mac { /* MAC */
  UW CONFIGURATION;                                     /* 000 - 0x0000: MAC Configuration Register */
  UW EXT_CONFIGURATION;                                 /* 001 - 0x0004: MAC Extended Configuration Register */
  UW PACKET_FILTER;                                     /* 002 - 0x0008: MAC_Packet_Filter */
  UW WATCHDOG_TIMEOUT;                                  /* 003 - 0x000C: Watchdog Timeout */
  UW HASH_TABLE_REG0;                                   /* 004 - 0x0010: MAC Hash Table Register 0 */
  UW HASH_TABLE_REG1;                                   /* 005 - 0x0014: MAC Hash Table Register 1 */
  UW reserved_0[14];
  UW VLAN_TAG_CTRL;                                     /* 020 - 0x0050: MAC VLAN Tag Control */
  UW VLAN_TAG_DATA;                                     /* 021 - 0x0054: MAC VLAN Tag Data */
  UW VLAN_HASH_TABLE;                                   /* 022 - 0x0058: MAC VLAN Hash Table */
  UW reserved_1[1];
  UW VLAN_INCL;                                         /* 024 - 0x0060: VLAN Tag Inclusion or Replacement */
  UW INNER_VLAN_INCL;                                   /* 025 - 0x0064: MAC Inner VLAN Tag Inclusion or Replacement */
  UW reserved_2[2];
  UW Q0_TX_FLOW_CTRL;                                   /* 028 - 0x0070: MAC Q0 Tx Flow Control */
  UW Q1_TX_FLOW_CTRL;                                   /* 029 - 0x0074: MAC Q1 Tx Flow Control */
  UW Q2_TX_FLOW_CTRL;                                   /* 030 - 0x0078: MAC Q2 Tx Flow Control */
  UW Q3_TX_FLOW_CTRL;                                   /* 031 - 0x007C: MAC Q3 Tx Flow Control */
  UW Q4_TX_FLOW_CTRL;                                   /* 032 - 0x0080: MAC Q0 Tx Flow Control */
  UW reserved_3[3];
  UW RX_FLOW_CTRL;                                      /* 036 - 0x0090: MAC Rx Flow Control */
  UW RXQ_CTRL4;                                         /* 037 - 0x0094: Receive Queue Control 4 */
  UW TXQ_PRTY_MAP0;                                     /* 038 - 0x0098: Transmit Queue Priority Mapping 0 */
  UW TXQ_PRTY_MAP1;                                     /* 039 - 0x009C: Transmit Queue Priority Mapping 1 */
  UW RXQ_CTRL0;                                         /* 040 - 0x00A0: Receive Queue Control 0 */
  UW RXQ_CTRL1;                                         /* 041 - 0x00A4: Receive Queue Control 1 */
  UW RXQ_CTRL2;                                         /* 042 - 0x00A8: Receive Queue Control 2 */
  UW RXQ_CTRL3;                                         /* 043 - 0x00AC: Receive Queue Control 3 */
  UW INTERRUPT_STATUS;                                  /* 044 - 0x00B0: Interrupt Status */
  UW INTERRUPT_ENABLE;                                  /* 045 - 0x00B4: Interrupt Enable */
  UW RX_TX_STATUS;                                      /* 046 - 0x00B8: Receive Transmit Status */
  UW reserved_4[1];
  UW PMT_CONTROL_STATUS;                                /* 048 - 0x00C0: PMT Control and Status */
  UW RWK_PACKET_FILTER;                                 /* 049 - 0x00C4: Remote Wakeup Filter */
  UW reserved_5[2];
  UW LPI_CONTROL_STATUS;                                /* 052 - 0x00D0: LPI Control and Status */
  UW LPI_TIMERS_CONTROL;                                /* 053 - 0x00D4: LPI Timers Control */
  UW LPI_ENTRY_TIMER;                                   /* 054 - 0x00D8: Tx LPI Entry Timer Control */
  UW ONEUS_TIC_COUNTER;                                 /* 055 - 0x00DC: One-microsecond Reference Timer */
  UW reserved_6[6];
  UW PHYIF_CONTROL_STATUS;                              /* 062 - 0x00F8: PHY Interface Control and Status */
  UW reserved_7[5];
  UW VERSION;                                           /* 068 - 0x0110: MAC Version */
  UW DEBUG;                                             /* 069 - 0x0114: MAC Debug */
  UW reserved_8[1];
  UW HW_FEATURE0;                                       /* 071 - 0x011C: Optional Features or Functions 0 */
  UW HW_FEATURE1;                                       /* 072 - 0x0120: Optional Features or Functions 1 */
  UW HW_FEATURE2;                                       /* 073 - 0x0124: Optional Features or Functions 2 */
  UW HW_FEATURE3;                                       /* 074 - 0x0128: Optional Features or Functions 3 */
  UW reserved_9[53];
  UW MDIO_ADDRESS;                                      /* 128 - 0x0200: MDIO Address */
  UW MDIO_DATA;                                         /* 129 - 0x0204: MAC MDIO Data */
  UW reserved_10[10];
  UW CSR_SW_CTRL;                                       /* 140 - 0x0230: CSR Software Control */
  UW FPE_CTRL_STS;                                      /* 141 - 0x0234: Frame Preemption Control */
  UW reserved_11[2];
  UW PRESN_TIME_NS;                                     /* 144 - 0x0240: 32-bit Binary Rollover Equivalent Time */
  UW PRESN_TIME_UPDT;                                   /* 145 - 0x0244: MAC 1722 Presentation Time */
  UW reserved_12[46];
  struct {
    UW HIGH;                                            /* 192 - 0x0300: MAC Address0 High */ /* 202 - 0x0328: MAC Address5 High */
    UW LOW;                                             /* 193 - 0x0304: MAC Address0 Low */  /* 203 - 0x032C: MAC Address5 Low */
  } ADDRESS0_5[5-0+1];
  UW reserved_13[40];
  struct {
    UW HIGH;                                            /* 244 - 0x03D0: MAC Address26 High */  /* 318 - 0x04F8: MAC Address63 High */
    UW LOW;                                             /* 245 - 0x03D4: MAC Address26 Low */   /* 319 - 0x04FC: MAC Address63 Low */    
  } ADDRESS26_63[63-26+1];
  UW reserved_14[128];
  UW MMC_CONTROL;                                       /* 448 - 0x0700: MMC Control */
  UW MMC_RX_INTERRUPT;                                  /* 449 - 0x0704: MMC Rx Interrupt */
  UW MMC_TX_INTERRUPT;                                  /* 450 - 0x0708: MMC Tx Interrupt */
  UW MMC_RX_INTERRUPT_MASK;                             /* 451 - 0x070C: MMC Rx Interrupt Mask */
  UW MMC_TX_INTERRUPT_MASK;                             /* 452 - 0x0710: MMC Tx Interrupt Mask */
  UW TX_OCTET_COUNT_GOOD_BAD;                           /* 453 - 0x0714: Tx Octet Count Good and Bad */
  UW TX_PACKET_COUNT_GOOD_BAD;                          /* 454 - 0x0718: Tx Packet Count Good and Bad */
  UW TX_BROADCAST_PACKETS_GOOD;                         /* 455 - 0x071C: Tx Broadcast Packets Good */
  UW TX_MULTICAST_PACKETS_GOOD;                         /* 456 - 0x0720: Tx Multicast Packets Good */
  UW TX_64OCTETS_PACKETS_GOOD_BAD;                      /* 457 - 0x0724: Tx Good and Bad 64-Byte Packets */
  UW TX_65TO127OCTETS_PACKETS_GOOD_BAD;                 /* 458 - 0x0728: Tx Good and Bad 65 to 127-Byte Packets */
  UW TX_128TO255OCTETS_PACKETS_GOOD_BAD;                /* 459 - 0x072C: Tx Good and Bad 128 to 255-Byte Packets */
  UW TX_256TO511OCTETS_PACKETS_GOOD_BAD;                /* 460 - 0x0730: Tx Good and Bad 256 to 511-Byte Packets */
  UW TX_512TO1023OCTETS_PACKETS_GOOD_BAD;               /* 461 - 0x0734: Tx Good and Bad 512 to 1023-Byte Packets */
  UW TX_1024TOMAXOCTETS_PACKETS_GOOD_BAD;               /* 462 - 0x0738: Tx Good and Bad 1024 to Max-Byte Packets */
  UW TX_UNICAST_PACKETS_GOOD_BAD;                       /* 463 - 0x073C: Good and Bad Unicast Packets Transmitted */
  UW TX_MULTICAST_PACKETS_GOOD_BAD;                     /* 464 - 0x0740: Good and Bad Multicast Packets Transmitted */
  UW TX_BROADCAST_PACKETS_GOOD_BAD;                     /* 465 - 0x0744: Good and Bad Broadcast Packets Transmitted */
  UW TX_UNDERFLOW_ERROR_PACKETS;                        /* 466 - 0x0748: Tx Packets Aborted By Underflow Error */
  UW TX_SINGLE_COLLISION_GOOD_PACKETS;                  /* 467 - 0x074C: Single Collision Good Packets Transmitted */
  UW TX_MULTIPLE_COLLISION_GOOD_PACKETS;                /* 468 - 0x0750: Multiple Collision Good Packets Transmitted */
  UW TX_DEFERRED_PACKETS;                               /* 469 - 0x0754: Deferred Packets Transmitted */
  UW TX_LATE_COLLISION_PACKETS;                         /* 470 - 0x0758: Late Collision Packets Transmitted */
  UW TX_EXCESSIVE_COLLISION_PACKETS;                    /* 471 - 0x075C: Excessive Collision Packets Transmitted */
  UW TX_CARRIER_ERROR_PACKETS;                          /* 472 - 0x0760: Carrier Error Packets Transmitted */
  UW TX_OCTET_COUNT_GOOD;                               /* 473 - 0x0764: Bytes Transmitted in Good Packets */
  UW TX_PACKET_COUNT_GOOD;                              /* 474 - 0x0768: Good Packets Transmitted */
  UW TX_EXCESSIVE_DEFERRAL_ERROR;                       /* 475 - 0x076C: Packets Aborted By Excessive Deferral Error */
  UW TX_PAUSE_PACKETS;                                  /* 476 - 0x0770: Pause Packets Transmitted */
  UW TX_VLAN_PACKETS_GOOD;                              /* 477 - 0x0774: Good VLAN Packets Transmitted */
  UW TX_OSIZE_PACKETS_GOOD;                             /* 478 - 0x0778: Good Oversize Packets Transmitted */
  UW reserved_15[1];
  UW RX_PACKETS_COUNT_GOOD_BAD;                         /* 480 - 0x0780: Good and Bad Packets Received */
  UW RX_OCTET_COUNT_GOOD_BAD;                           /* 481 - 0x0784: Bytes in Good and Bad Packets Received */
  UW RX_OCTET_COUNT_GOOD;                               /* 482 - 0x0788: Bytes in Good Packets Received */
  UW RX_BROADCAST_PACKETS_GOOD;                         /* 483 - 0x078C: Good Broadcast Packets Received */
  UW RX_MULTICAST_PACKETS_GOOD;                         /* 484 - 0x0790: Good Multicast Packets Received */
  UW RX_CRC_ERROR_PACKETS;                              /* 485 - 0x0794: CRC Error Packets Received */
  UW RX_ALIGNMENT_ERROR_PACKETS;                        /* 486 - 0x0798: Alignment Error Packets Received */
  UW RX_RUNT_ERROR_PACKETS;                             /* 487 - 0x079C: Runt Error Packets Received */
  UW RX_JABBER_ERROR_PACKETS;                           /* 488 - 0x07A0: Jabber Error Packets Received */
  UW RX_UNDERSIZE_PACKETS_GOOD;                         /* 489 - 0x07A4: Good Undersize Packets Received */
  UW RX_OVERSIZE_PACKETS_GOOD;                          /* 490 - 0x07A8: Good Oversize Packets Received */
  UW RX_64OCTETS_PACKETS_GOOD_BAD;                      /* 491 - 0x07AC: Good and Bad 64-Byte Packets Received */
  UW RX_65TO127OCTETS_PACKETS_GOOD_BAD;                 /* 492 - 0x07B0: Good and Bad 64-to-127 Byte Packets Received */
  UW RX_128TO255OCTETS_PACKETS_GOOD_BAD;                /* 493 - 0x07B4: Good and Bad 128-to-255 Byte Packets Received */
  UW RX_256TO511OCTETS_PACKETS_GOOD_BAD;                /* 494 - 0x07B8: Good and Bad 256-to-511 Byte Packets Received */
  UW RX_512TO1023OCTETS_PACKETS_GOOD_BAD;               /* 495 - 0x07BC: Good and Bad 512-to-1023 Byte Packets Received */
  UW RX_1024TOMAXOCTETS_PACKETS_GOOD_BAD;               /* 496 - 0x07C0: Good and Bad 1024-to-Max Byte Packets Received */
  UW RX_UNICAST_PACKETS_GOOD;                           /* 497 - 0x07C4: Good Unicast Packets Received */
  UW RX_LENGTH_ERROR_PACKETS;                           /* 498 - 0x07C8: Length Error Packets Received */
  UW RX_OUT_OF_RANGE_TYPE_PACKETS;                      /* 499 - 0x07CC: Out-of-range Type Packets Received */
  UW RX_PAUSE_PACKETS;                                  /* 500 - 0x07D0: Pause Packets Received */
  UW RX_FIFO_OVERFLOW_PACKETS;                          /* 501 - 0x07D4: Missed Packets Due to FIFO Overflow */
  UW RX_VLAN_PACKETS_GOOD_BAD;                          /* 502 - 0x07D8: Good and Bad VLAN Packets Received */
  UW RX_WATCHDOG_ERROR_PACKETS;                         /* 503 - 0x07DC: Watchdog Error Packets Received */
  UW RX_RECEIVE_ERROR_PACKETS;                          /* 504 - 0x07E0: Receive Error Packets Received */
  UW RX_CONTROL_PACKETS_GOOD;                           /* 505 - 0x07E4: Good Control Packets Received */
  UW reserved_16[1];
  UW TX_LPI_USEC_CNTR;                                  /* 507 - 0x07EC: Microseconds Tx LPI Asserted */
  UW TX_LPI_TRAN_CNTR;                                  /* 508 - 0x07F0: Number of Times Tx LPI Asserted */
  UW RX_LPI_USEC_CNTR;                                  /* 509 - 0x07F4: Microseconds Rx LPI Sampled */
  UW RX_LPI_TRAN_CNTR;                                  /* 510 - 0x07F8: Number of Times Rx LPI Entered */
  UW reserved_17[1];
  UW MMC_IPC_RX_INTERRUPT_MASK;                         /* 512 - 0x0800: MMC IPC Receive Interrupt Mask */
  UW reserved_18[1];
  UW MMC_IPC_RX_INTERRUPT;                              /* 514 - 0x0808: MMC IPC Receive Interrupt */
  UW reserved_19[1];
  UW RXIPV4_GOOD_PACKETS;                               /* 516 - 0x0810: Good IPv4 Datagrams Received */
  UW RXIPV4_HEADER_ERROR_PACKETS;                       /* 517 - 0x0814: IPv4 Datagrams Received with Header Errors */
  UW RXIPV4_NO_PAYLOAD_PACKETS;                         /* 518 - 0x0818: IPv4 Datagrams Received with No Payload */
  UW RXIPV4_FRAGMENTED_PACKETS;                         /* 519 - 0x081C: IPv4 Datagrams Received with Fragmentation */
  UW RXIPV4_UDP_CHECKSUM_DISABLED_PACKETS;              /* 520 - 0x0820: IPv4 Datagrams Received with UDP Checksum Disabled */
  UW RXIPV6_GOOD_PACKETS;                               /* 521 - 0x0824: Good IPv6 Datagrams Received */
  UW RXIPV6_HEADER_ERROR_PACKETS;                       /* 522 - 0x0828: IPv6 Datagrams Received with Header Errors */
  UW RXIPV6_NO_PAYLOAD_PACKETS;                         /* 523 - 0x082C: IPv6 Datagrams Received with No Payload */
  UW RXUDP_GOOD_PACKETS;                                /* 524 - 0x0830: IPv6 Datagrams Received with Good UDP */
  UW RXUDP_ERROR_PACKETS;                               /* 525 - 0x0834: IPv6 Datagrams Received with UDP Checksum Error */
  UW RXTCP_GOOD_PACKETS;                                /* 526 - 0x0838: IPv6 Datagrams Received with Good TCP Payload */
  UW RXTCP_ERROR_PACKETS;                               /* 527 - 0x083C: IPv6 Datagrams Received with TCP Checksum Error */
  UW RXICMP_GOOD_PACKETS;                               /* 528 - 0x0840: IPv6 Datagrams Received with Good ICMP Payload */
  UW RXICMP_ERROR_PACKETS;                              /* 529 - 0x0844: IPv6 Datagrams Received with ICMP Checksum Error */
  UW reserved_20[2];
  UW RXIPV4_GOOD_OCTETS;                                /* 532 - 0x0850: Good Bytes Received in IPv4 Datagrams */
  UW RXIPV4_HEADER_ERROR_OCTETS;                        /* 533 - 0x0854: Bytes Received in IPv4 Datagrams with Header Errors */
  UW RXIPV4_NO_PAYLOAD_OCTETS;                          /* 534 - 0x0858: Bytes Received in IPv4 Datagrams with No Payload */
  UW RXIPV4_FRAGMENTED_OCTETS;                          /* 535 - 0x085C: Bytes Received in Fragmented IPv4 Datagrams */
  UW RXIPV4_UDP_CHECKSUM_DISABLE_OCTETS;                /* 536 - 0x0860: Bytes Received with UDP Checksum Disabled */
  UW RXIPV6_GOOD_OCTETS;                                /* 537 - 0x0864: Bytes Received in Good IPv6 Datagrams */
  UW RXIPV6_HEADER_ERROR_OCTETS;                        /* 538 - 0x0868: Bytes Received in IPv6 Datagrams with Data Errors */
  UW RXIPV6_NO_PAYLOAD_OCTETS;                          /* 539 - 0x086C: Bytes Received in IPv6 Datagrams with No Payload */
  UW RXUDP_GOOD_OCTETS;                                 /* 540 - 0x0870: Bytes Received in Good UDP Segment */
  UW RXUDP_ERROR_OCTETS;                                /* 541 - 0x0874: Bytes Received in UDP Segment with Checksum Errors */
  UW RXTCP_GOOD_OCTETS;                                 /* 542 - 0x0878: Bytes Received in Good TCP Segment */
  UW RXTCP_ERROR_OCTETS;                                /* 543 - 0x087C: Bytes Received in TCP Segment with Checksum Errors */
  UW RXICMP_GOOD_OCTETS;                                /* 544 - 0x0880: Bytes Received in Good ICMP Segment */
  UW RXICMP_ERROR_OCTETS;                               /* 545 - 0x0884: Bytes Received in ICMP Segment with Checksum Errors */
  UW reserved_21[6];
  UW MMC_FPE_TX_INTERRUPT;                              /* 552 - 0x08A0: MMC FPE Transmit Interrupt */
  UW MMC_FPE_TX_INTERRUPT_MASK;                         /* 553 - 0x08A4: MMC FPE Transmit Mask Interrupt */
  UW MMC_TX_FPE_FRAGMENT_CNTR;                          /* 554 - 0x08A8: MMC FPE Transmitted Fragment Counter */
  UW MMC_TX_HOLD_REQ_CNTR;                              /* 555 - 0x08AC: MMC FPE Transmitted Hold Request Counter */
  UW reserved_22[4];
  UW MMC_FPE_RX_INTERRUPT;                              /* 560 - 0x08C0: MMC FPE Receive Interrupt */
  UW MMC_FPE_RX_INTERRUPT_MASK;                         /* 561 - 0x08C4: MMC FPE Receive Interrupt Mask */
  UW MMC_RX_PACKET_ASSEMBLY_ERR_CNTR;                   /* 562 - 0x08C8: MMC Receive Packet Reassembly Error Counter */
  UW MMC_RX_PACKET_SMD_ERR_CNTR;                        /* 563 - 0x08CC: MMC Receive Packet SMD Error Counter */
  UW MMC_RX_PACKET_ASSEMBLY_OK_CNTR;                    /* 564 - 0x08D0: MMC Receive Packet Successful Reassembly Counter */
  UW MMC_RX_FPE_FRAGMENT_CNTR;                          /* 565 - 0x08D4: MMC FPE Received Fragment Counter */
  UW reserved_23[10];

  struct {
    UW L3_L4_CONTROL;                                    /* 576 - 0x0900: Layer 3 and Layer 4 Control of Filter 0 */  /* 660 - 0x0A50: Layer 3 and Layer 4 Control of Filter 0 */
    UW LAYER4_ADDRESS;                                   /* 577 - 0x0904: Layer 4 Address 0 */
    UW reserved_24[2];
    UW LAYER3_ADDR0_REG;                                 /* 580 - 0x0910: Layer 3 Address 0 Register 0 */
    UW LAYER3_ADDR1_REG;                                 /* 581 - 0x0914: Layer 3 Address 1 Register 0 */
    UW LAYER3_ADDR2_REG;                                 /* 582 - 0x0918: Layer 3 Address 2 Register 0 */
    UW LAYER3_ADDR3_REG;                                 /* 583 - 0x091C: Layer 3 Address 3 Register 0 */             /* 667 - 0x0A6C: Layer 3 Address 3 Register 7 */
    UW reserved_25[4];
  } L3L4_REG[8];
  UW reserved_39_1[32];
  UW TIMESTAMP_CONTROL;                                 /* 704 - 0x0B00: Timestamp Control */
  UW SUB_SECOND_INCREMENT;                              /* 705 - 0x0B04: Subsecond Increment */
  UW SYSTEM_TIME_SECONDS;                               /* 706 - 0x0B08: System Time Seconds */
  UW SYSTEM_TIME_NANOSECONDS;                           /* 707 - 0x0B0C: System Time Nanoseconds */
  UW SYSTEM_TIME_SECONDS_UPDATE;                        /* 708 - 0x0B10: System Time Seconds Update */
  UW SYSTEM_TIME_NANOSECONDS_UPDATE;                    /* 709 - 0x0B14: System Time Nanoseconds Update */
  UW TIMESTAMP_ADDEND;                                  /* 710 - 0x0B18: Timestamp Addend */
  UW SYSTEM_TIME_HIGHER_WORD_SECONDS;                   /* 711 - 0x0B1C: System Time - Higher Word Seconds */
  UW TIMESTAMP_STATUS;                                  /* 712 - 0x0B20: Timestamp Status */
  UW reserved_40[3];
  UW TX_TIMESTAMP_STATUS_NANOSECONDS;                   /* 716 - 0x0B30: Transmit Timestamp Status Nanoseconds */
  UW TX_TIMESTAMP_STATUS_SECONDS;                       /* 717 - 0x0B34: Transmit Timestamp Status Seconds */
  UW reserved_41[2];
  UW AUXILIARY_CONTROL;                                 /* 720 - 0x0B40: Auxiliary Timestamp Control */
  UW reserved_42[1];
  UW AUXILIARY_TIMESTAMP_NANOSECONDS;                   /* 722 - 0x0B48: Auxiliary Timestamp Nanoseconds */
  UW AUXILIARY_TIMESTAMP_SECONDS;                       /* 723 - 0x0B4C: Auxiliary Timestamp Seconds */
  UW TIMESTAMP_INGRESS_ASYM_CORR;                       /* 724 - 0x0B50: Timestamp Ingress Asymmetry Correction */
  UW TIMESTAMP_EGRESS_ASYM_CORR;                        /* 725 - 0x0B54: imestamp Egress Asymmetry Correction */
  UW TIMESTAMP_INGRESS_CORR_NANOSECOND;                 /* 726 - 0x0B58: Timestamp Ingress Correction Nanosecond */
  UW TIMESTAMP_EGRESS_CORR_NANOSECOND;                  /* 727 - 0x0B5C: Timestamp Egress Correction Nanosecond */
  UW TIMESTAMP_INGRESS_CORR_SUBNANOSEC;                 /* 728 - 0x0B60: Timestamp Ingress Correction Subnanosecond */
  UW TIMESTAMP_EGRESS_CORR_SUBNANOSEC;                  /* 729 - 0x0B64: Timestamp Egress Correction Subnanosecond */
  UW TIMESTAMP_INGRESS_LATENCY;                         /* 730 - 0x0B68: Timestamp Ingress Latency */
  UW TIMESTAMP_EGRESS_LATENCY;                          /* 731 - 0x0B6C: Timestamp Egress Latency */
  UW PPS_CONTROL;                                       /* 732 - 0x0B70: PPS Control */
  UW reserved_43[3];
  UW PPS0_TARGET_TIME_SECONDS;                          /* 736 - 0x0B80: PPS0 Target Time Seconds */
  UW PPS0_TARGET_TIME_NANOSECONDS;                      /* 737 - 0x0B84: PPS0 Target Time Nanoseconds */
  UW PPS0_INTERVAL;                                     /* 738 - 0x0B88: PPS0 Interval */
  UW PPS0_WIDTH;                                        /* 739 - 0x0B8C: PPS0 Width */
  UW PPS1_TARGET_TIME_SECONDS;                          /* 740 - 0x0B90: PPS1 Target Time Seconds */
  UW PPS1_TARGET_TIME_NANOSECONDS;                      /* 741 - 0x0B94: PPS1 Target Time Nanoseconds */
  UW PPS1_INTERVAL;                                     /* 742 - 0x0B98: PPS1 Interval */
  UW PPS1_WIDTH;                                        /* 743 - 0x0B9C: PPS1 Width */
  UW PPS2_TARGET_TIME_SECONDS;                          /* 744 - 0x0BA0: PPS2 Target Time Seconds */
  UW PPS2_TARGET_TIME_NANOSECONDS;                      /* 745 - 0x0BA4: PPS2 Target Time Nanoseconds */
  UW PPS2_INTERVAL;                                     /* 746 - 0x0BA8: PPS2 Interval */
  UW PPS2_WIDTH;                                        /* 747 - 0x0BAC: PPS2 Width */
  UW PPS3_TARGET_TIME_SECONDS;                          /* 748 - 0x0BB0: PPS3 Target Time Seconds */
  UW PPS3_TARGET_TIME_NANOSECONDS;                      /* 749 - 0x0BB4: PPS3 Target Time Nanoseconds */
  UW PPS3_INTERVAL;                                     /* 750 - 0x0BB8: PPS3 Interval */
  UW PPS3_WIDTH;                                        /* 751 - 0x0BBC: PPS3 Width */
  UW PTO_CONTROL;                                       /* 752 - 0x0BC0: PTP Offload Engine Control */
  UW SOURCE_PORT_IDENTITY0;                             /* 753 - 0x0BC4: Source Port Identity 0 */
  UW SOURCE_PORT_IDENTITY1;                             /* 754 - 0x0BC8: Source Port Identity 1 */
  UW SOURCE_PORT_IDENTITY2;                             /* 755 - 0x0BCC: Source Port Identity 2 */
  UW LOG_MESSAGE_INTERVAL;                              /* 756 - 0x0BD0: Log Message Interval */
  UW reserved_44[11];
} MAC;
struct t_enetqos_mtl { /* MTL */
  UW OPERATION_MODE;                                    /* 768 - 0x0C00: MTL Operation Mode */
  UW reserved_45[1];
  UW DBG_CTL;                                           /* 770 - 0x0C08: FIFO Debug Access Control and Status */
  UW DBG_STS;                                           /* 771 - 0x0C0C: FIFO Debug Status */
  UW FIFO_DEBUG_DATA;                                   /* 772 - 0x0C10: FIFO Debug Data */
  UW reserved_46[3];
  UW INTERRUPT_STATUS;                                  /* 776 - 0x0C20: MTL Interrupt Status */
  UW reserved_47[3];
  UW RXQ_DMA_MAP0;                                      /* 780 - 0x0C30: Receive Queue and DMA Channel Mapping 0 */
  UW RXQ_DMA_MAP1;                                      /* 781 - 0x0C34: Receive Queue and DMA Channel Mapping 1 */
  UW reserved_48[2];
  UW TBS_CTRL;                                          /* 784 - 0x0C40: Time Based Scheduling Control */
  UW reserved_49[3];
  UW EST_CONTROL;                                       /* 788 - 0x0C50: Enhancements to Scheduled Transmission Control */
  UW reserved_50[1];
  UW EST_STATUS;                                        /* 790 - 0x0C58: Enhancements to Scheduled Transmission Status */
  UW reserved_51[1];
  UW EST_SCH_ERROR;                                     /* 792 - 0x0C60: EST Scheduling Error */
  UW EST_FRM_SIZE_ERROR;                                /* 793 - 0x0C64: EST Frame Size Error */
  UW EST_FRM_SIZE_CAPTURE;                              /* 794 - 0x0C68: EST Frame Size Capture */
  UW reserved_52[1];
  UW EST_INTR_ENABLE;                                   /* 796 - 0x0C70: EST Interrupt Enable */
  UW reserved_53[3];
  UW EST_GCL_CONTROL;                                   /* 800 - 0x0C80: EST GCL Control */
  UW EST_GCL_DATA;                                      /* 801 - 0x0C84: EST GCL Data */
  UW reserved_54[2];
  UW FPE_CTRL_STS;                                      /* 804 - 0x0C90: Frame Preemption Control and Status */
  UW FPE_ADVANCE;                                       /* 805 - 0x0C94: Frame Preemption Hold and Release Advance */
  UW reserved_55[2];
  UW RXP_CONTROL_STATUS;                                /* 808 - 0x0CA0: RXP Control Status */
  UW RXP_INTERRUPT_CONTROL_STATUS;                      /* 809 - 0x0CA4: RXP Interrupt Control Status */
  UW RXP_DROP_CNT;                                      /* 810 - 0x0CA8: RXP Drop Count */
  UW RXP_ERROR_CNT;                                     /* 811 - 0x0CAC: RXP Error Count */
  UW RXP_INDIRECT_ACC_CONTROL_STATUS;                   /* 812 - 0x0CB0: RXP Indirect Access Control and Status */
  UW RXP_INDIRECT_ACC_DATA;                             /* 813 - 0x0CB4: RXP Indirect Access Data */
  UW reserved_56[18];
  UW TXQ0_OPERATION_MODE;                               /* 832 - 0x0D00: Queue 0 Transmit Operation Mode */
  UW TXQ0_UNDERFLOW;                                    /* 833 - 0x0D04: Queue 0 Underflow Counter */
  UW TXQ0_DEBUG;                                        /* 834 - 0x0D08: Queue 0 Transmit Debug */
  UW reserved_57[2];
  UW TXQ0_ETS_STATUS;                                   /* 837 - 0x0D14: Queue 0 ETS Status */
  UW TXQ0_QUANTUM_WEIGHT;                               /* 838 - 0x0D18: Queue 0 Quantum or Weights */
  UW reserved_58[4];
  UW Q0_INTERRUPT_CONTROL_STATUS;                       /* 843 - 0x0D2C: Queue 0 Interrupt Control Status */
  UW RXQ0_OPERATION_MODE;                               /* 844 - 0x0D30: Queue 0 Receive Operation Mode */
  UW RXQ0_MISSED_PACKET_OVERFLOW_CNT;                   /* 845 - 0x0D34: Queue 0 Missed Packet and Overflow Counter */
  UW RXQ0_DEBUG;                                        /* 846 - 0x0D38: Queue 0 Receive Debug */
  UW RXQ0_CONTROL;                                      /* 847 - 0x0D3C: Queue 0 Receive Control */
  UW TXQ1_OPERATION_MODE;                               /* 848 - 0x0D40: Queue 1 Transmit Operation Mode */
  UW TXQ1_UNDERFLOW;                                    /* 849 - 0x0D44: Queue 1 Underflow Counter */
  UW TXQ1_DEBUG;                                        /* 850 - 0x0D48: Queue 1 Transmit Debug */
  UW reserved_59[1];
  UW TXQ1_ETS_CONTROL;                                  /* 852 - 0x0D50: Queue 1 ETS Control */
  UW TXQ1_ETS_STATUS;                                   /* 853 - 0x0D54: Queue 1 ETS Status */
  UW TXQ1_QUANTUM_WEIGHT;                               /* 854 - 0x0D58: Queue 1 idleSlopeCredit, Quantum or Weights */
  UW TXQ1_SENDSLOPECREDIT;                              /* 855 - 0x0D5C: Queue 1 sendSlopeCredit */
  UW TXQ1_HICREDIT;                                     /* 856 - 0x0D60: Queue 1 hiCredit */
  UW TXQ1_LOCREDIT;                                     /* 857 - 0x0D64: Queue 1 loCredit */
  UW reserved_60[1];
  UW Q1_INTERRUPT_CONTROL_STATUS;                       /* 859 - 0x0D6C: Queue 1 Interrupt Control Status */
  UW RXQ1_OPERATION_MODE;                               /* 860 - 0x0D70: Queue 1 Receive Operation Mode */
  UW RXQ1_MISSED_PACKET_OVERFLOW_CNT;                   /* 861 - 0x0D74: Queue 1 Missed Packet and Overflow Counter */
  UW RXQ1_DEBUG;                                        /* 862 - 0x0D78: Queue 1 Receive Debug */
  UW RXQ1_CONTROL;                                      /* 863 - 0x0D7C: Queue 1 Receive Control */
  UW TXQ2_OPERATION_MODE;                               /* 864 - 0x0D80: Queue 2 Transmit Operation Mode */
  UW TXQ2_UNDERFLOW;                                    /* 865 - 0x0D84: Queue 2 Underflow Counter */
  UW TXQ2_DEBUG;                                        /* 866 - 0x0D88: Queue 2 Transmit Debug */
  UW reserved_61[1];
  UW TXQ2_ETS_CONTROL;                                  /* 868 - 0x0D90: Queue 2 ETS Control */
  UW TXQ2_ETS_STATUS;                                   /* 869 - 0x0D94: Queue 2 ETS Status */
  UW TXQ2_QUANTUM_WEIGHT;                               /* 870 - 0x0D98: Queue 2 idleSlopeCredit, Quantum or Weights */
  UW TXQ2_SENDSLOPECREDIT;                              /* 871 - 0x0D9C: Queue 2 sendSlopeCredit */
  UW TXQ2_HICREDIT;                                     /* 872 - 0x0DA0: Queue 2 hiCredit */
  UW TXQ2_LOCREDIT;                                     /* 873 - 0x0DA4: Queue 2 loCredit */
  UW reserved_62[1];
  UW Q2_INTERRUPT_CONTROL_STATUS;                       /* 875 - 0x0DAC: Queue 2 Interrupt Control Status */
  UW RXQ2_OPERATION_MODE;                               /* 876 - 0x0DB0: Queue 2 Receive Operation Mode */
  UW RXQ2_MISSED_PACKET_OVERFLOW_CNT;                   /* 877 - 0x0DB4: Queue 2 Missed Packet and Overflow Counter */
  UW RXQ2_DEBUG;                                        /* 878 - 0x0DB8: Queue 2 Receive Debug */
  UW RXQ2_CONTROL;                                      /* 879 - 0x0DBC: Queue 2 Receive Control */
  UW TXQ3_OPERATION_MODE;                               /* 880 - 0x0DC0: Queue 3 Transmit Operation Mode */
  UW TXQ3_UNDERFLOW;                                    /* 881 - 0x0DC4: Queue 3 Underflow Counter */
  UW TXQ3_DEBUG;                                        /* 882 - 0x0DC8: Queue 3 Transmit Debug */
  UW reserved_63[1];
  UW TXQ3_ETS_CONTROL;                                  /* 884 - 0x0DD0: Queue 3 ETS Control */
  UW TXQ3_ETS_STATUS;                                   /* 885 - 0x0DD4: Queue 3 ETS Status */
  UW TXQ3_QUANTUM_WEIGHT;                               /* 886 - 0x0DD8: Queue 3 idleSlopeCredit, Quantum or Weights */
  UW TXQ3_SENDSLOPECREDIT;                              /* 887 - 0x0DDC: Queue 3 sendSlopeCredit */
  UW TXQ3_HICREDIT;                                     /* 888 - 0x0DE0: Queue 3 hiCredit */
  UW TXQ3_LOCREDIT;                                     /* 889 - 0x0DE4: Queue 3 loCredit */
  UW reserved_64[1];
  UW Q3_INTERRUPT_CONTROL_STATUS;                       /* 891 - 0x0DEC: Queue 3 Interrupt Control Status */
  UW RXQ3_OPERATION_MODE;                               /* 892 - 0x0DF0: Queue 3 Receive Operation Mode */
  UW RXQ3_MISSED_PACKET_OVERFLOW_CNT;                   /* 893 - 0x0DF4: Queue 3 Missed Packet and Overflow Counter */
  UW RXQ3_DEBUG;                                        /* 894 - 0x0DF8: Queue 3 Receive Debug */
  UW RXQ3_CONTROL;                                      /* 895 - 0x0DFC: Queue 3 Receive Control */
  UW TXQ4_OPERATION_MODE;                               /* 896 - 0x0E00: Queue 4 Transmit Operation Mode */
  UW TXQ4_UNDERFLOW;                                    /* 897 - 0x0E04: Queue 4 Underflow Counter */
  UW TXQ4_DEBUG;                                        /* 898 - 0x0E08: Queue 4 Transmit Debug */
  UW reserved_65[1];
  UW TXQ4_ETS_CONTROL;                                  /* 900 - 0x0E10: Queue 4 ETS Control */
  UW TXQ4_ETS_STATUS;                                   /* 901 - 0x0E14: Queue 4 ETS Status */
  UW TXQ4_QUANTUM_WEIGHT;                               /* 902 - 0x0E18: Queue 4 idleSlopeCredit, Quantum or Weights */
  UW TXQ4_SENDSLOPECREDIT;                              /* 903 - 0x0E1C: Queue 4 sendSlopeCredit */
  UW TXQ4_HICREDIT;                                     /* 904 - 0x0E20: Queue 4 hiCredit */
  UW TXQ4_LOCREDIT;                                     /* 905 - 0x0E24: Queue 4 loCredit */
  UW reserved_66[1];
  UW Q4_INTERRUPT_CONTROL_STATUS;                       /* 907 - 0x0E2C: Queue 4 Interrupt Control Status */
  UW RXQ4_OPERATION_MODE;                               /* 908 - 0x0E30: Queue 4 Receive Operation Mode */
  UW RXQ4_MISSED_PACKET_OVERFLOW_CNT;                   /* 909 - 0x0E34: Queue 4 Missed Packet and Overflow Counter */
  UW RXQ4_DEBUG;                                        /* 910 - 0x0E38: Queue 4 Receive Debug */
  UW RXQ4_CONTROL;                                      /* 911 - 0x0E3C: Queue 4 Receive Control */
  UW reserved_67[112];
} MTL;
struct t_enetqos_dma { /* DMA */
  UW MODE;                                              /* 1024 - 0x1000: DMA Bus Mode */
  UW SYSBUS_MODE;                                       /* 1025 - 0x1004: DMA System Bus Mode */
  UW INTERRUPT_STATUS;                                  /* 1026 - 0x1008: DMA Interrupt Status */
  UW DEBUG_STATUS0;                                     /* 1027 - 0x100C: DMA Debug Status 0 */
  UW DEBUG_STATUS1;                                     /* 1028 - 0x1010: DMA Debug Status 1 */
  UW reserved_68[11];
  UW AXI_LPI_ENTRY_INTERVAL;                            /* 1040 - 0x1040: AXI LPI Entry Interval Control */
  UW reserved_69[3];
  UW TBS_CTRL;                                          /* 1044 - 0x1050: TBS Control */
  UW reserved_70[43];

  struct {
    UW CONTROL;                                       /* 1088 - 0x1100: DMA Channel 0 Control */                /* 1216 - 0x1300: DMA Channel 4 Control */
    UW TX_CONTROL;                                    /* 1089 - 0x1104: DMA Channel 0 Transmit Control */
    UW RX_CONTROL;                                    /* 1090 - 0x1108: DMA Channel 0 Receive Control */
    UW reserved_71[2];
    UW TXDESC_LIST_ADDRESS;                           /* 1093 - 0x1114: Channel 0 Tx Descriptor List Address register */
    UW reserved_72[1];
    UW RXDESC_LIST_ADDRESS;                           /* 1095 - 0x111C: Channel 0 Rx Descriptor List Address register */
    UW TXDESC_TAIL_POINTER;                           /* 1096 - 0x1120: Channel 0 Tx Descriptor Tail Pointer */
    UW reserved_73[1];
    UW RXDESC_TAIL_POINTER;                           /* 1098 - 0x1128: Channel 0 Rx Descriptor Tail Pointer */
    UW TXDESC_RING_LENGTH;                            /* 1099 - 0x112C: Channel 0 Tx Descriptor Ring Length */
    UW RXDESC_RING_LENGTH;                            /* 1100 - 0x1130: Channel 0 Rx Descriptor Ring Length */
    UW INTERRUPT_ENABLE;                              /* 1101 - 0x1134: Channel 0 Interrupt Enable */
    UW RX_INTERRUPT_WATCHDOG_TIMER;                   /* 1102 - 0x1138: Channel 0 Receive Interrupt Watchdog Timer */
    UW SLOT_FUNCTION_CONTROL_STATUS;                  /* 1103 - 0x113C: Channel 0 Slot Function Control and Status */
    UW reserved_74[1];
    UW CURRENT_APP_TXDESC;                            /* 1105 - 0x1144: Channel 0 Current Application Transmit Descriptor */
    UW reserved_75[1];
    UW CURRENT_APP_RXDESC;                            /* 1107 - 0x114C: Channel 0 Current Application Receive Descriptor */
    UW reserved_76[1];
    UW CURRENT_APP_TXBUFFER;                          /* 1109 - 0x1154: Channel 0 Current Application Transmit Buffer Address */
    UW reserved_77[1];
    UW CURRENT_APP_RXBUFFER;                          /* 1111 - 0x115C: Channel 0 Current Application Receive Buffer Address */
    UW STATUS;                                        /* 1112 - 0x1160: DMA Channel 0 Status */
    UW MISS_FRAME_CNT;                                /* 1113 - 0x1164: Channel 0 Missed Frame Counter */
    UW RXP_ACCEPT_CNT;                                /* 1114 - 0x1168: Channel 0 RXP Frames Accepted Counter */
    UW RX_ERI_CNT;                                    /* 1115 - 0x116C: Channel 0 Receive ERI Counter */          /* 1243 - 0x136C: Channel 4 Receive ERI Counter */
    UW reserved_78[4];
  } CH[5];
} DMA;
};

#define REG_EQOS   (*(volatile struct t_enetqos *)ENET2_TSN_BASE)


/*************************************************************************
 * RCC AHB1 Clock Register (RCC_AHB1ENR), use Ethernet
 ************************************************************************/
#define AHB1ENR_ETH1RXEN    BIT17   /* Ethernet Reception Clock Enable */
#define AHB1ENR_ETH1TXEN    BIT16   /* Ethernet Transmission Clock Enable */
#define AHB1ENR_ETH1MACEN   BIT15   /* Ethernet MAC bus interface Clock Enable */ 

/*************************************************************************
 * RCC AHB1 Peripheral Reset Register(RCC_AHB1RSTR), use Ethernet
 ************************************************************************/     
#define AHB1RSTR_ETH1MACRST BIT15   /* ETH1MAC block reset */     
     
/*************************************************************************
 * SYSCFG peripheral mode configuration register (SYSCFG_PMCR), use Ethernet
 ************************************************************************/
#define PMCR_EPIS_pos       21      /* [2:0] Ethernet PHY Interface Selection */
#define PMCR_EPIS_msk       (0x07 << PMCR_EPIS_pos)
#define PMCR_EPIS_MII       (0x00 << PMCR_EPIS_pos)     /* MII */
#define PMCR_EPIS_RMII      (0x04 << PMCR_EPIS_pos)     /* RMII */
    
/*************************************************************************
 * MAC Configuration Register (MAC_CONFIGURATION)
 ************************************************************************/
#define MACCR_SARC_pos      28      /* [2:0] Source Address Insertion or Replacement Control */
#define MACCR_IPC           BIT27   /* Checksum Offload */
#define MACCR_IPG_pos       24      /* [2:0] Inter-Packet Gap */
#define MACCR_GPSLCE        BIT23   /* Giant Packet Size Limit Control Enable */
#define MACCR_S2KP          BIT22   /* IEEE 802.3as Support for 2K Packets */
#define MACCR_CST           BIT21   /* CRC stripping for Type packets */
#define MACCR_ACS           BIT20   /* Automatic Pad or CRC Stripping */
#define MACCR_WD            BIT19   /* Watchdog Disable */
#define MACCR_BE            BIT18   /* Packet Burst Enable */
#define MACCR_JD            BIT17   /* Jabber Disable */
#define MACCR_JE            BIT16   /* Jumbo Packet Enable */
#define MACCR_PS            BIT15   /* Port Select */
#define MACCR_FES           BIT14   /* Speed */
#define MACCR_DM            BIT13   /* Duplex Mode */
#define MACCR_LM            BIT12   /* Loopback Mode */
#define MACCR_ECRSFD        BIT11   /* Enable Carrier Sense Before Transmission in Full-Duplex Mode */
#define MACCR_DO            BIT10   /* Disable Receive Own */
#define MACCR_DCRS          BIT9    /* Disable Carrier Sense During Transmission */
#define MACCR_DR            BIT8    /* Disable Retry */
#define MACCR_BL_pos        5       /* [1:0] Back-Off Limit */
#define MACCR_DC            BIT4    /* Deferral Check */
#define MACCR_PRELEN_pos    2       /* [1:0] Preamble Length for Transmit packets */
#define MACCR_TE            BIT1    /* Transmitter Enable */
#define MACCR_RE            BIT0    /* Receiver Enable */

/*************************************************************************
 * MAC Packet Filter (MAC_PACKET_FILTER)
 ************************************************************************/
#define MACPFR_RA           BIT31   /* Receive All */    
#define MACPFR_DNTU         BIT21   /* Drop Non-TCP/UDP over IP Packets */    
#define MACPFR_IPFE         BIT20   /* Layer 3 and Layer 4 Filter Enable */    
#define MACPFR_VTFE         BIT16   /* VLAN Tag Filter Enable */    
#define MACPFR_HPF          BIT10   /* Hash or Perfect Filter */    
#define MACPFR_SAF          BIT9    /* Source Address Filter Enable */    
#define MACPFR_SAIF         BIT8    /* SA Inverse Filtering */    
#define MACPFR_PCF_pos      6       /* [1:0] Pass Control Packets */    
#define MACPFR_DBF          BIT5    /* Disable Broadcast Packets */    
#define MACPFR_PM           BIT4    /* Pass All Multicast */    
#define MACPFR_DAIF         BIT3    /* DA Inverse Filtering */    
#define MACPFR_HMC          BIT2    /* Hash Multicast */    
#define MACPFR_HUC          BIT1    /* Hash Unicast */    
#define MACPFR_PR           BIT0    /* Promiscuous Mode */    
    
/*************************************************************************
 * DMA Bus Mode (DMA_MODE)
 ************************************************************************/
#define DMAMR_INTM_2        BIT17       /* Interrupt Mode */
#define DMAMR_INTM_1        BIT16
#define DMAMR_DSPW          BIT8        /* Descriptor Posted Write */
#define DMAMR_SWR           BIT0        /* Software Reset */

/*************************************************************************
 * DMA System Bus Mode (DMA_SYSBUS_MODE)
 ************************************************************************/
#define DMASBMR_EN_LPI          BIT31   /* Enable Low Power Interface (LPI) */
#define DMASBMR_LPI_XIT_PKT     BIT30   /* Unlock on Magic Packet or Remote Wake-Up Packet */
#define DMASBMR_WR_OSR_LMT_pos  24      /* [3:0] AXI Maximum Write Outstanding Request Limit */
#define DMASBMR_RD_OSR_LMT_pos  16      /* [3:0] AXI Maximum Read Outstanding Request Limit */
#define DMASBMR_ONEKBBE         BIT13   /* 1 KB Boundary Crossing Enable for the EQOS-AXI Master */
#define DMASBMR_AAL             BIT12   /* Address-Aligned Beats */
#define DMASBMR_AALE            BIT10   /* Automatic AXI LPI enable  */
#define DMASBMR_BLEN16          BIT3    /* AXI Burst Length 16 */
#define DMASBMR_BLEN8           BIT2    /* AXI Burst Length 8 */
#define DMASBMR_BLEN4           BIT1    /* AXI Burst Length 4 */
#define DMASBMR_FB              BIT0    /* Fixed Burst Length */

/*************************************************************************
 * DMA Channel 0 Control (DMA_CH0_CONTROL)
 ************************************************************************/
#define DMACCR_DSL_pos      18          /*  [2:0] Descriptor Skip Length */
#define DMACCR_PBLX8        BIT16       /*  8xPBL mode */

/*************************************************************************
 * DMA Channel 0 Transmit Control (DMA_CH0_TX_CONTROL)
 ************************************************************************/
#define DMACTXCR_EDSE       BIT28       /* Enhanced Descriptor Enable */
#define DMACTXCR_TXPBL_pos  16       /* [5:0] Transmit Programmable Burst Length */
#define DMACTXCR_IPBL       BIT15       /* Ignore PBL Requirement */
#define DMACTXCR_OSF        BIT4        /* Operate on Second Packet */
#define DMACTXCR_ST         BIT0        /* Start or Stop Transmission Command */

/*************************************************************************
 * DMA Channel 0 Receive Control (DMA_CH0_RX_CONTROL)
 ************************************************************************/
#define DMACRXCR_RPF        BIT31       /* DMA Rx Channel Packet Flush */
#define DMACRXCR_RXPBL_pos  16          /* [5:0] Receive Programmable Burst Length */
#define DMACRXCR_RBSZ_pos   1           /* [13:0] Receive Buffer size */
#define DMACRXCR_SR         BIT0        /* Start or Stop Receive */

/*************************************************************************
 * Queue 0 Transmit Operation Mode (MTL_TXQ0_OPERATION_MODE)
 ************************************************************************/
#define MTLTXQOMR_TQS_pos   16          /* [4:0] Transmit Queue Size */
#define MTLTXQOMR_TTC_pos   4           /* [2:0] Transmit Threshold Control */
#define MTLTXQOMR_TXQEN_pos 2           /* [1:0] Transmit Queue Enable */
#define MTLTXQOMR_TSF       BIT1        /* Transmit Store and Forward */
#define MTLTXQOMR_FTQ       BIT0        /* Flush Transmit Queue */

/*************************************************************************
 * Queue 0 Receive Operation Mode (MTL_RXQ0_OPERATION_MODE)
 ************************************************************************/
#define MTLRXQOMR_RQS_pos     20        /* [4:0] Receive Queue Size */
#define MTLRXQOMR_RFD_pos     14        /* [3:0] Threshold for Deactivating Flow Control */
#define MTLRXQOMR_RFA_pos     8         /* [3:0] Threshold for Activating Flow Control */
#define MTLRXQOMR_EHFC        BIT7      /* Enable Hardware Flow Control */
#define MTLRXQOMR_DIS_TCP_EF  BIT6      /* Disable Dropping of TCP/IP Checksum Error Packets */
#define MTLRXQOMR_RSF         BIT5      /* Receive Queue Store and Forward */
#define MTLRXQOMR_FEP         BIT4      /* Forward Error Packets */
#define MTLRXQOMR_FUP         BIT3      /* Forward Undersized Good Packets */
#define MTLRXQOMR_RTC_pos     0         /* [1:0]Receive Queue Threshold Control */
    
/*************************************************************************
 * Channel 0 Interrupt Enable (DMA_CH0_INTERRUPT_ENABLE)
 ************************************************************************/
#define DMACIER_NIE         BIT15       /* Normal Interrupt Summary Enable */     
#define DMACIER_AIE         BIT14       /* Abnormal Interrupt Summary Enable */     
#define DMACIER_CDEE        BIT13       /* Context Descriptor Error Enable */     
#define DMACIER_FBEE        BIT12       /* Fatal Bus Error Enable */     
#define DMACIER_ERIE        BIT11       /* Early Receive Interrupt Enable */     
#define DMACIER_ETIE        BIT10       /* Early Transmit Interrupt Enable */     
#define DMACIER_RWTE        BIT9        /* Receive Watchdog Timeout Enable */     
#define DMACIER_RSE         BIT8        /* Receive Stopped Enable */     
#define DMACIER_RBUE        BIT7        /* Receive Buffer Unavailable Enable */     
#define DMACIER_RIE         BIT6        /* Receive Interrupt Enable */     
#define DMACIER_TBUE        BIT2        /* Transmit Buffer Unavailable Enable */     
#define DMACIER_TXSE        BIT1        /* Transmit Stopped Enable */     
#define DMACIER_TIE         BIT0        /* Transmit Interrupt Enable */     

/*************************************************************************
 * MDIO Address (MAC_MDIO_ADDRESS)
 ************************************************************************/
#define MACMDIOAR_PSE       BIT27       /* Preamble Suppression Enable */     
#define MACMDIOAR_BTB       BIT26       /* Back to Back transactions */     
#define MACMDIOAR_PA_pos    21          /* [4:0] Physical Layer Address */     
#define MACMDIOAR_RDA_pos   16          /* [4:0] Register/Device Address */     
#define MACMDIOAR_NTC_pos   12          /* [2:0] Number of Training Clocks */     
#define MACMDIOAR_CR_pos    8           /* [3:0] CSR Clock Range */     
#define MACMDIOAR_SKAP      BIT4        /* Skip Address Packet */     
#define MACMDIOAR_GOC_pos   2           /* [1:0] MII Operation Command */     
#define MACMDIOAR_GOC_R     (BIT3|BIT2) /*   11: Read */     
#define MACMDIOAR_GOC_W     (BIT2)      /*   01: Write */     
#define MACMDIOAR_C45E      BIT1        /* Clause 45 PHY Enable */     
#define MACMDIOAR_GB        BIT0        /* GMII Busy */     

/*************************************************************************
 * MAC MDIO Data (MAC_MDIO_DATA)
 ************************************************************************/     
#define MACMDIODR_MD_pos    0           /* [15:0] MII Data */        
#define MACMDIODR_MD_msk    (0xFFFF << MACMDIODR_MD_pos)
    

/*************************************************************************
 * MAC Address x High (MAC_ADDRESSx_HIGH)
 ************************************************************************/
#define MACAxHR_AE          BIT31     /* Address Enable */
#define MACAxHR_DCS_pos     16        /* [4:0] DMA Channel Select */          
#define MACAxHR_SA          BIT30     /* Source Address (x=1...) */     
#define MACAxHR_MBC_pos     24        /* [5:0] Mask Byte Control (x=1...) */     
     

/*************************************************************************
 * DMA Debug Status 0 (DMA_DEBUG_STATUS0)
 ************************************************************************/
#define DMADSR_TPS0_pos     12          /* [3:0] DMA Channel Transmit Process State */    
#define DMADSR_RPS0_pos     8           /* [3:0] DMA Channel Receive Process State */    
#define DMADSR_RPS0_msk     (0x07 << DMADSR_RPS0_pos)
#define DMADSR_RPS0_RDU     (0x04 << DMADSR_RPS0_pos)   /* 100: Suspended (Rx Descriptor Unavailable) */
#define DMADSR_AXRHSTS      BIT1        /* AXI Master Read Channe */    
#define DMADSR_AXWHSTS      BIT0        /* AHB Master Write Channel */    
    
/*************************************************************************
 * DMA Interrupt Status (DMA_INTERRUPT_STATUS)
 ************************************************************************/
#define DMAISR_MACIS        BIT17       /* MAC Interrupt Status */
#define DMAISR_MTLIS        BIT16       /* MTL Interrupt Status */
#define DMAISR_DC4IS        BIT4        /* DMA Channel 4 Interrupt Status */
#define DMAISR_DC3IS        BIT3        /* DMA Channel 3 Interrupt Status */
#define DMAISR_DC2IS        BIT2        /* DMA Channel 2 Interrupt Status */
#define DMAISR_DC1IS        BIT1        /* DMA Channel 1 Interrupt Status */
#define DMAISR_DC0IS        BIT0        /* DMA Channel 0 Interrupt Status */

/*************************************************************************
 * DMA Channel 0 Status (DMA_CH0_STATUS)
 ************************************************************************/
#define DMACSR_REB_pos      19          /* [2:0]: Rx DMA Error Bits */
#define DMACSR_REB_msk      (0x7 << DMACSR_REB_pos)
#define DMACSR_TEB_pos      16          /* [2:0]: Tx DMA Error Bits */
#define DMACSR_TEB_msk      (0x7 << DMACSR_TEB_pos)
#define DMACSR_NIS          BIT15       /* Normal Interrupt Summary */
#define DMACSR_AIS          BIT14       /* Abnormal Interrupt Summary */
#define DMACSR_CDE          BIT13       /* Context Descriptor Error */
#define DMACSR_FBE          BIT12       /* Fatal Bus Error */
#define DMACSR_ERI          BIT11       /* Early Receive Interrupt */
#define DMACSR_ETI          BIT10       /* Early Transmit Interrupt */
#define DMACSR_RWT          BIT9        /* Receive Watchdog Timeout */
#define DMACSR_RPS          BIT8        /* Receive Process Stopped */
#define DMACSR_RBU          BIT7        /* Receive Buffer Unavailable */
#define DMACSR_RI           BIT6        /* Receive Interrupt */
#define DMACSR_TBU          BIT2        /* Transmit Buffer Unavailable */
#define DMACSR_TPS          BIT1        /* Transmit Process Stopped */
#define DMACSR_TI           BIT0        /* Transmit Interrupt */

/*************************************************************************
 * Queue 0 Interrupt Control Status (MTL_Q0_INTERRUPT_CONTROL_STATUS)
 ************************************************************************/
#define MTLQICSR_RXOIE      BIT24   /* Receive Queue Overflow Interrupt Enable */
#define MTLQICSR_RXOVFIS    BIT16   /* Receive Queue Overflow Interrupt Status */
#define MTLQICSR_ABPSIE     BIT9    /* Average Bits Per Slot Interrupt Enable */
#define MTLQICSR_TXUIE      BIT8    /* Transmit Queue Underflow Interrupt Enable */
#define MTLQICSR_ABPSIS     BIT1    /* Average Bits Per Slot Interrupt Status */
#define MTLQICSR_TXUNFIS    BIT0    /* Transmit Queue Underflow Interrupt Status */
       
/*************************************************************************
 * RDESx
 ************************************************************************/
/* RDES1 (write-back format) */     
#define RDES1W_TD           BIT15   /* Timestamp Dropped */   
#define RDES1W_TSA          BIT14   /* Timestamp Available */   
#define RDES1W_PV           BIT13   /* PTP Version */   
#define RDES1W_PFT          BIT12   /* PTP Packet Type */   
#define RDES1W_PMT_pos      8       /* [3:0] PTP Message Type */   
#define RDES1W_IPCE         BIT7    /* IP Payload Error */   
#define RDES1W_IPCB         BIT6    /* IP Checksum Bypassed */   
#define RDES1W_IPV6         BIT5    /* IPv6 header Present */   
#define RDES1W_IPV4         BIT4    /* IPV4 Header Present */   
#define RDES1W_IPHE         BIT3    /* IP Header Error */   
#define RDES1W_PT_pos       0       /* [2:0] Payload Type */   
#define RDES1W_PT_mskpos    (0x3 << RDES1W_PT_pos)

/* RDES2 (write-back format) */     
#define RDES2W_L3L4FM_pos   29      /* [2:0] L3/L4 Filter Number Matched */   
#define RDES2W_L4FM         BIT28   /* L4 Filter Match */   
#define RDES2W_L3FM         BIT27   /* L3 Filter Match */   
#define RDES2W_MADRM_pos    19      /* [7:0] MAC Address Match or Hash Value */   
#define RDES2W_HF           BIT18   /* Hash Filter Status */   
#define RDES2W_DAF          BIT17   /* Destination Address Filter Fail */   
#define RDES2W_SAF          BIT16   /* SA Address Filter Fail */   
#define RDES2W_OTS          BIT15   /* Outer VLAN Tag Filter Status (OTS) */   
#define RDES2W_ITS          BIT14   /* Inner VLAN Tag Filter Status (ITS) */   
#define RDES2W_ARPNR        BIT10   /* ARP Reply Not Generated */   
#define RDES2W_HL_pos       0       /* [9:0] L3/L4 Header Length */   
     
/* RDES3 (read format) */     
#define RDES3R_OWN          BIT31   /* Own bit */     
#define RDES3R_IOC          BIT30   /* Interrupt Enabled on Completion */     
#define RDES3R_BUF2V        BIT25   /* Buffer 2 Address Valid */     
#define RDES3R_BUF1V        BIT24   /* Buffer 1 Address Valid */     
     
/* RDES3 (write-back format) */     
#define RDES3W_OWN          BIT31   /* Own bit */     
#define RDES3W_CTXT         BIT30   /* Receive Context Descriptor */     
#define RDES3W_FD           BIT29   /* First Descriptor */     
#define RDES3W_LD           BIT28   /* Last Descriptor */     
#define RDES3W_RS2V         BIT27   /* Receive Status RDES2 Valid */     
#define RDES3W_RS1V         BIT26   /* Receive Status RDES1 Valid */     
#define RDES3W_RS0V         BIT25   /* Receive Status RDES0 Valid */     
#define RDES3W_CE           BIT24   /* CRC Error */     
#define RDES3W_GP           BIT23   /* Giant Packet */     
#define RDES3W_RWT          BIT22   /* Receive Watchdog Timeout */     
#define RDES3W_OE           BIT21   /* Overflow Error */   
#define RDES3W_RE           BIT20   /* Receive Error */   
#define RDES3W_DE           BIT19   /* Dribble Bit Error */   
#define RDES3W_LT_pos       16      /* [2:0] Length/Type Field */   
#define RDES3W_ES           BIT15   /* Error Summary */   
#define RDES3W_PL_pos       0       /* [14:0] Packet Length */   
#define RDES3W_PL_msk       (0x7FFF << RDES3W_PL_pos)
     

/*************************************************************************
 * TDESx
 ************************************************************************/
/* TDES3 (read format) */     
#define TDES3R_OWN          BIT31   /* Own bit */   
#define TDES3R_CTXT         BIT30   /* Context Type */   
#define TDES3R_FD           BIT29   /* First Descriptor */   
#define TDES3R_LD           BIT28   /* Last Descriptor */   
#define TDES3R_CPC_pos      26      /* [1:0] CRC Pad Control */   
#define TDES3R_SAIC_pos     23      /* [2:0] SA Insertion Control */   
#define TDES3R_THL_pos      19      /* [3:0] THL: TCP Header Length */   
#define TDES3R_TSE          BIT18   /* TCP Segmentation Enable */   
#define TDES3R_CIC_pos      16      /* [1:0] Checksum Insertion Control or TCP Payload Length */   
  #define TDES3R_CIC_IPHDR  BIT16   /* 01: Only IP header checksum calculation and insertion are enabled. */   
  #define TDES3R_CIC_IPHDR_PSEUDO_PL  (BIT16 | BIT17)
                                    /* 11: IP header checksum and payload checksum calculation and ... */
#define TDES3R_TPL          BIT15   /* Reserved or TCP Payload Length */   
#define TDES3R_FL_pos       0       /* [14:0] Packet Length or TCP Payload Length */   


#ifdef __cplusplus
}
#endif
#endif /* _DDR_ETH_H_ */
