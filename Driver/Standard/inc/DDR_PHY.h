/**
 * @file    DDR_PHY.h
 * @brief   MICRO C CUBE / COMPACT, DEVICE DRIVER
 *          Ethernet PHY specific common defintions
 * @date    2018.06.04
 * @author  Copyright (c) 2008-2018, eForce Co., Ltd. All rights reserved.
 *
 ****************************************************************************
 * @par     History
 *          - rev 1.0 (2008.12.13)
 *            First release.
 *          - rev 1.1 (2015.03.23)
 *            Modified for 1000Base.
 *          - rev 1.2 (2016.12.14) y-kim
 *            Fixed the IPA warnings.
 *          - rev 1.3 (2018.03.27) yokota
 *            merge from nios2 DDR_PHY.h file. 
 *          - rev 1.4 (2018.06.04) makino
 *            Modified for Standard PHY driver
 ****************************************************************************
 */

#ifndef DDR_PHY_H_
#define DDR_PHY_H_

#ifdef __cplusplus
extern "C" {
#endif

#define PHY_BMCR        0x00U   /* Basic mode Control */
#define PHY_BMSR        0x01U   /* Basic mode Status */
#define PHY_IDR1        0x02U   /* PHY Identifier */
#define PHY_IDR2        0x03U   /* PHY Identifier */
#define PHY_ANAR        0x04U   /* Auto-Negotiation Advertisement */
#define PHY_ANLPAR      0x05U   /* Auto-Negotiation Link Partner Ability */
#define PHY_ANER        0x06U   /* Auto-Negotiation Expansion */
#define PHY_ANNPTR      0x07U   /* Auto-Negotiation Next Page Transmit */
#define PHY_1000_CTL    0x09U   /* 1000-BASET Control */
#define PHY_1000_STS    0x0AU   /* 1000-BASET Status */

#ifdef PHY_DEVICE
#ifdef DP83848
#if (PHY_DEVICE == DP83848)
#define PHY_STS         0x10U
#define PHY_MICR        0x11U
#define PHY_MISR        0x12U
#define PHY_FCSCR       0x14U
#define PHY_RBR         0x17U
#define PHY_LEDCR       0x18U
#define PHY_CR          0x19U
#define PHY_10BTSCR     0x1AU
#endif
#endif
#endif

/* (Register 0) Basic mode Control Register */
#define BMCR_RESET          0x8000U
#define BMCR_LOOPBACK       0x4000U
#define BMCR_SPD            0x2000U
#define BMCR_ANE            0x1000U
#define BMCR_PDOWN          0x0800U
#define BMCR_ISOL           0x0400U
#define BMCR_RS_ANP         0x0200U
#define BMCR_DUPLEX         0x0100U
#define BMCR_COL_TEST       0x0080U
#define BMCR_SPD1000        0x0040U
#define BMCR_SPD100         0x2000U
#define BMCR_SPD10          0x0000U

/* (Register 1) Status Register */
#define BMSR_100_T4         0x8000U
#define BMSR_100_X_FD       0x4000U
#define BMSR_100_X_HD       0x2000U
#define BMSR_10_FD          0x1000U
#define BMSR_10_HD          0x0800U
#define BMSR_EXT_V          0x0100U    /* Exist extended status Information in register 15 */
#define BMSR_PREAMBLE       0x0040U
#define BMSR_ANEG_COMP      0x0020U
#define BMSR_RM_FAULT       0x0010U
#define BMSR_AN_ABLE        0x0008U
#define BMSR_LINK_STAT      0x0004U
#define BMSR_JABBER         0x0002U
#define BMSR_EXT_CAP        0x0001U

/* (Register 4) Auto-Negotiation Advertisement Register */
#define ANAR_NP             0x8000U
#define ANAR_RF             0x2000U
#define ANAR_ASM_DIR        0x0800U
#define ANAR_PAUSE          0x0400U
#define ANAR_T4             0x0200U
#define ANAR_TX_FD          0x0100U
#define ANAR_TX             0x0080U
#define ANAR_10_FD          0x0040U
#define ANAR_10             0x0020U
#define ANAR_SF_802_3u      0x0001U

/* (Register 5) Auto-Negotiation Link Partner Abilty Register (Base Page) */
#define ANLPAR_B_NP         0x8000U
#define ANLPAR_B_ACK        0x4000U
#define ANLPAR_B_R_FAULT    0x2000U
#define ANLPAR_B_ASM_DIR    0x0800U
#define ANLPAR_B_PAUSE      0x0400U
#define ANLPAR_B_T4         0x0200U
#define ANLPAR_B_TX_FD      0x0100U
#define ANLPAR_B_TX         0x0080U
#define ANLPAR_B_10_FD      0x0040U
#define ANLPAR_B_10         0x0020U
#define ANLPAR_B_SF_802_3u  0x0001U

/* (Register 5) Auto-Negotiation Link Partner Abilty Register (Next Page) */
#define ANLPAR_N_NP         0x8000U
#define ANLPAR_N_ACK        0x4000U
#define ANLPAR_N_MP         0x2000U
#define ANLPAR_N_ACK2       0x1000U
#define ANLPAR_N_TOGGLE     0x0800U

/* (Register 6) Auto-Negotiation Expansion */
#define ANER_PDF            0x0010U
#define ANER_LP_NP_ABLE     0x0008U
#define ANER_NP_ABLE        0x0004U
#define ANER_PAGE_RX        0x0002U
#define ANER_LP_AN_ABLE     0x0001U

/* (Register 7) Auto-Negotiation Next Page Transmit Register */
#define ANNPTR_NP           0x8000U
#define ANNPTR_MP           0x2000U
#define ANNPTR_ACK2         0x1000U
#define ANNPTR_TOG_TX       0x0800U
#define ANNPTR_CODE_802_3u  0x0001U

/* (Register 9) 1000BASE-T Control Register */
#define GCTL_MAS_ENB        0x1000U
#define GCTL_MAS_CFG        0x0800U
#define GCTL_MUL_PORT       0x0400U
#define GCTL_1000_FD        0x0200U
#define GCTL_1000_HD        0x0100U

/* (Register 10) 1000BASE-T Status Register */
#define GSTS_MAS_FLT        0x8000U
#define GSTS_MAS_RES        0x4000U
#define GSTS_LOC_RCV        0x2000U
#define GSTS_RMT_RCV        0x1000U
#define GSTS_1000_FD        0x0800U
#define GSTS_1000_HD        0x0400U

#ifdef PHY_DEVICE
#ifdef DP83848
#if (PHY_DEVICE == DP83848)
#define PHYSTS_ANEG_DONE    0x0010U
#define PHYSTS_LOOPBACK     0x0008U
#define PHYSTS_FD           0x0004U
#define PHYSTS_10M          0x0002U
#define PHYSTS_LINK         0x0001U
#endif
#endif
#endif

/* Disable Auto-Negotiaiton */
#define HD10   0x0000U                      /*  10M, Half */
#define HD100  (BMCR_SPD_LSB)               /* 100M, Half */
#define FD10   (BMCR_DUPLEX)                /*  10M, Full */
#define FD100  (BMCR_SPD_LSB | BMCR_DUPLEX) /* 100M, Full */

/* Enable Auto-Negotiation */
#define AHD10  (ANA_TAF_10T_H)              /*  10M, Half */
#define AHD100 (ANA_TAF_100TX_H)            /* 100M, Half */
#define AFD10  (ANA_TAF_10T_F)              /*  10M, Full */
#define AFD100 (ANA_TAF_100TX_F)            /* 100M, Full */


typedef void (*MDIO_TX)(UB adr, UINT reg, UH dat);
typedef UH   (*MDIO_RX)(UB adr, UINT reg);
typedef void (*MEDIA_UPDATE)(UH dev_num, UH sts);

typedef struct eth_mac_if {
    UH           dev_num;
    UH           tag;
    UB           *mac;
    MDIO_TX      mdio_tx;
    MDIO_RX      mdio_rx;
    MEDIA_UPDATE media_update;
} ETH_MAC_IF; 

typedef struct hal_media_if {
    ER (*media_if_ini)(ETH_MAC_IF *mac);
    ER (*media_if_cls)(void);
    ER (*media_if_set)(UH opt, VP val);
    ER (*media_if_get)(UH opt, VP val);
    UB (*media_if_adr)(void);
} HAL_MEDIA_IF;

#ifdef __cplusplus
}
#endif
#endif /* DDR_PHY_H_ */

