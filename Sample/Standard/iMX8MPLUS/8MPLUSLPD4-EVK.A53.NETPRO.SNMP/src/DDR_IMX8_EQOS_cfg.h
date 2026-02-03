/***********************************************************************
    Ethernet(ENET_QOS) driver configuration
 ***********************************************************************/

#ifndef _DDR_IMX8_EQOS_CFG_H_
#define _DDR_IMX8_EQOS_CFG_H_

/* CPU ID */

#ifdef __cplusplus
extern "C" {
#endif

/*
    Configure PHY Address
*/
#define EQOS_PHY_ADDR    0x1

/*
    Configure Interrupt priority level
*/
#define EQOS_IPL         0xE0

/*
    Configure MDC Clock for SMII
--------------------------------
                CSR clock range
                0 = 60-100 MHz; MDC clock = CSR clock/42
                1 = 100-150 MHz; MDC clock = CSR clock/62
                2 = 20-35 MHz; MDC clock = CSR clock/16
                3 = 35-60 MHz; MDC clock = CSR clock/26
                4 = 150-250 MHz; MDC clock = CSR clock/102
    (Default)   5 = 250-300 MHz; MDC clock = CSR clock/124
                6 = 300-500 MHz; MDC clock = CSR clock/204
                7 = 500-800 MHz; MDC clock = CSR clock/324    
*/
#define EQOS_MDC_CLK     5

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
#define EQOS_PHY_MODE    0


/*
    Configure Address Filter Mode
--------------------------------
    (Default)   0 = Filter disable (Perfect filtering)
                1 = promiscuous mode (receive all packets)
                2 = multicast filter mode
*/
#define EQOS_FILTER_MODE    0

/*
    Configure Rx DMA Descriptor count
--------------------------------
*/
#define EQOS_TXDESC_CNT    4
#define EQOS_RXDESC_CNT    4

/*
    Configure Hardware checksum offloading
--------------------------------
                0 = Disable Tx & Rx Hardware checksum
                1 = Enable Tx Hardware checksum
                2 = Enable Rx Hardware checksum
    (Default)   3 = Enable Tx & Rx Hardware checksum
*/
#define EQOS_CSUM_MODE    3

/*
    Configure RAM section used by network buffer(s) and descriptor(s).
--------------------------------
    Note: Bus error occurs when DTCM area is specified.
          Please specify the memory area Bus error does not occur.
          The memory area must be non-cached.
*/
#define EQOS_RAM_SECTION ".uncache"

/*
    Configure ENET_QOS descriptor buffer use network buffer.
--------------------------------
                0 = descriptor buffer isn't network buffer.
    (Default)   1 = descriptor buffer is network buffer.
*/
#define EQOS_RAM_USE_NETBUF     1

/* Definitions for special PHYs */
#define EQOS_PHY_USE_RTL8211F

#define SNMP_ENA                1

#ifdef __cplusplus
}
#endif
#endif
