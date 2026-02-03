/***********************************************************************
    Ethernet(ENET_NET) driver configuration
 ***********************************************************************/

#ifndef _DDR_IMX8_ENET_CFG_H_
#define _DDR_IMX8_ENET_CFG_H_

/* CPU ID */

#ifdef __cplusplus
extern "C" {
#endif

//#define CPU_IMX_8M_NANO
//#define CPU_IMX_8M_MINI
#define CPU_IMX_8M_PLUS
//#define CPU_IMX_8M_QUADMAX

/*
    Configure PHY Address
*/
#define ENET_PHY_ADDR    0x0

/*
    Configure Interrupt priority level
*/
#define ENET_IPL         0xE0

/*
    Configure MII/RMII/RGMII
*/
#define ENET_RMII_MODE   2   /* 2:RGMII only supported */


#define ENET_MDC_CLK     266000000UL  /* 66MHz */

/*
    Configure PHY Mode
--------------------------------
    (Default)   0 = Auto select mode
                1 = 10M Half Duplex manual mode
                7 = 10M Full Duplex manual mode
                2 = 10M Full/Half (Duplex auto select mode)
                3 = 100M Half Duplex manual mode
                8 = 100M Full Duplex manual mode
                4 = 100M Full/Half (Duplex auto select mode)
                6 = 1000M Full Duplex manual mode
*/
#define ENET_PHY_MODE    0


/*
    Configure Address Filter Mode
--------------------------------
    (Default)   0 = Filter disable (Perfect filtering)
                1 = promiscuous mode (receive all packets)
                2 = multicast filter mode
*/
#define ENET_FILTER_MODE    0

/*
    Configure Rx DMA Descriptor count
--------------------------------
*/
#define ENET_TXDESC_CNT    4
#define ENET_RXDESC_CNT    4

/*
    Configure Hardware checksum offloading
--------------------------------
                0 = Disable Tx & Rx Hardware checksum
                1 = Enable Tx Hardware checksum
                2 = Enable Rx Hardware checksum
    (Default)   3 = Enable Tx & Rx Hardware checksum
*/
#define ENET_CSUM_MODE    3

/*
    Configure RAM section used by network buffer(s) and descriptor(s).
--------------------------------
    Note: Bus error occurs when DTCM area is specified.
          Please specify the memory area Bus error does not occur.
          The memory area must be non-cached.
*/
#define ENET_RAM_SECTION ".uncache"

#define SNMP_ENA                1

#ifdef __cplusplus
}
#endif
#endif
