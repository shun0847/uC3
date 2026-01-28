/**
 * @file    DDR_IMX8_ENET.c
 * @brief   Ethernet Interface for i.MX8 series(ENET)
 * @date    2025.10.20
 * @author  Copyright (c) 2021-2025, eForce Co., Ltd. All rights reserved.
 *
 ******************************************************************************
 * @par     History
 *  - rev 1.0 (2021.02.09)
 *    Initial version.
 *  - rev 1.1 (2021.05.12)
 *    1. Improved driver speed performance.
 *    2. Added call impossible conditions of the driver function.
 *  - rev 1.2 (2021.06.18)
 *    1. Fixed the problem that reception does not return after net_buf is exhausted.
 *    2. Changed to the same reception behavior as other eforce drivers.
 *       (Set UNTIL_RECV_DESC_EMPTY=1 for previous driver reception operation.)
 *  - rev 1.3 (2022.09.20)
 *    i.MX8 QuadMax support
 *  - rev 1.4 (2022.10.20)
 *    Added initialization process for DP83867
 *  - rev 1.5 (2023.06.21)
 *    Fixed a build error problem in the i.MX8Nano/Plus environment.
 *  - rev 1.6 (2025.06.04)
 *    i.MX8M Mini(ARM AArch64) support
 *    Improved the symptom of multiplexing of sending packets.
 *    (Tx descriptor changed to not using network buffer) 
 *  - rev 1.7 (2025.10.20)
 *    Updated to suppress warnings under GCC.
 ******************************************************************************
 */
#include "kernel.h"
#include "DDR_IMX8_ENET_cfg.h"
#include "DDR_IMX8_ENET.h"
#ifdef CPU_IMX_8M_QUADMAX
#include "DDR_iMX8_IRQ_STEER.h"
#include "DDR_iMX8_IRQ_STEER_cfg.h"
#endif

#if defined(CPU_IMX_8M_NANO)
#include "imx8mnano_uC3.h"
#elif defined(CPU_IMX_8M_MINI)
#include "imx8mmini_uC3.h"
#elif defined(CPU_IMX_8M_PLUS)
#include "imx8mplus_uC3.h"
#elif defined(CPU_IMX_8M_QUADMAX) 
#include "imx8qm_uC3.h"
#else
#error "Unknown CPU series."
#endif

#include "uC3sys.h"
#include "DDR_PHY.h"
#include "net_hdr.h"
#if defined(NET3_VER) && ((NET3_VER) >= 3U)
#include "net_def.h"
#include "net_cfg.h"
#include "net_sts.h"
#include "net_sts_id.h"
#endif
#ifdef NET_C_OS
#include "kernel_id.h"
#endif

/* for ether test tool */
#ifdef ETHER_TEST_TOOL_MODE
ID g_ethertesttool_ID_ETH_RCV_TSK;
ID g_ethertesttool_ID_ETH_CTL_TSK;
#endif

#if ((TKERNEL_PRID & 0x00FF) == 0x4C)   /* ARM AArch64 */
#include "DDR_Aarch64_MMU.h"
#define DCACHE_CLEAR(x,y)       (_ddr_aarch64_mmu_clean_data_cache((void *)x, (SIZE)y))
#define DCACHE_INVALID(x,y)     (_ddr_aarch64_mmu_invalid_data_cache((void *)x, (SIZE)y))
#else
#define DCACHE_CLEAR(x,y)       (_kernel_clean_data_cache((void *)x, (SIZE)y))
#define DCACHE_INVALID(x,y)     (_kernel_invalid_data_cache((void *)x, (SIZE)y))
#endif

#ifndef SNMP_ENA
#define SNMP_ENA 1U
#endif
#if SNMP_ENA != 1U
#error "This driver SNMP function is alway enable."
#endif

#define PHY_POLL_TMOUT 500

#ifndef ETH_TX_TMOUT
#define ETH_TX_TMOUT 1000 /* 1 second */
#endif

#if (_kernel_SIZE_SIZE==8)
#define ETH_TSK_STK_MUL         3
#else
#define ETH_TSK_STK_MUL         1
#endif

#ifndef ETH_SND_TSK_STACK_SIZE
#define ETH_SND_TSK_STACK_SIZE (1024U * ETH_TSK_STK_MUL)
#endif

#ifndef ETH_RCV_TSK_STACK_SIZE
#define ETH_RCV_TSK_STACK_SIZE (2048U * ETH_TSK_STK_MUL)
#endif

#ifndef ETH_CTL_TSK_STACK_SIZE
#define ETH_CTL_TSK_STACK_SIZE (1024U * ETH_TSK_STK_MUL)
#endif

#ifndef ENET_PHY_ADDR
#define ENET_PHY_ADDR 0U
#endif
#if ((0U > ENET_PHY_ADDR) || (ENET_PHY_ADDR > 31U))
#error "The value of ENET_PHY_ADDR is invalid."
#endif

#ifndef ENET_IPL
#define ENET_IPL 0xa0U
#endif

#ifndef ENET_PHY_MODE
#define ENET_PHY_MODE 0U
#endif
#if !((ENET_PHY_MODE == 0U) || (ENET_PHY_MODE == 1U) ||                        \
      (ENET_PHY_MODE == 2U) || (ENET_PHY_MODE == 3U) ||                        \
      (ENET_PHY_MODE == 4U) || (ENET_PHY_MODE == 7U) ||                        \
      (ENET_PHY_MODE == 8U) || (ENET_PHY_MODE == 6U) || (ENET_PHY_MODE == 9U))
#error "The value of ENET_PHY_MODE is invalid."
#endif

#ifndef ENET_CSUM_MODE
#define ENET_CSUM_MODE 3U
#endif
#if ((0U > ENET_CSUM_MODE) || (ENET_CSUM_MODE > 3U))
#error "The value of ENET_CSUM_MODE is invalid."
#endif

#ifndef ENET_RMII_MODE
#define ENET_RMII_MODE 1U /* 0 - MII, 1 - RMII, 2 - RGMII */
#endif
#if ((0U > ENET_RMII_MODE) || (ENET_RMII_MODE > 2U))
#error "The value of ENET_RMII_MODE is invalid."
#endif

#ifndef ENET_FILTER_MODE
#define ENET_FILTER_MODE 0U
#endif
#if ((0U > ENET_FILTER_MODE) || (ENET_FILTER_MODE > 2U))
#error "The value of ENET_FILTER_MODE is invalid."
#endif

#ifndef ENET_RXDESC_CNT
#define ENET_RXDESC_CNT 4U
#endif

#ifndef ENET_TXDESC_CNT
#define ENET_TXDESC_CNT 4U
#endif

#ifndef ENET_MAXLEN
#define ENET_MAXLEN 1514U /* Maximum ethernet frame size */
#endif

#ifndef ENET_RAM_USE_NETBUF         /* descriptor buffer is network buffer? (1:yes, 0:no) */
#define ENET_RAM_USE_NETBUF     1
#endif

#ifndef UNTIL_RECV_DESC_EMPTY
#define UNTIL_RECV_DESC_EMPTY   0
#endif

#ifndef ENET_RX_NBUFGET_TMO
#define ENET_RX_NBUFGET_TMO     TMO_POL
#endif

/*************************************************************************
 * Descriptor and Data buffer setup definitions.
 *
 *  ENET_BUFSZ       : Data buffer block size (same for Rx and Tx)
 *  TXBUF_CNT       : TX Descriptor count
 *  RXBUF_CNT       : RX Descriptor count
 ************************************************************************/
#ifndef ENET_BUFSZ
#define ENET_BUFSZ 1536U
#endif
#if ENET_BUFSZ != 1536U || (ENET_BUFSZ & 0x1f) != 0
#error "ENET_BUFSZ must equal 1536"
#endif

#define TBUFSZ ENET_BUFSZ
#define RBUFSZ ENET_BUFSZ
#define RBUFSZ_MAX ENET_BUFSZ

#define TXBUF_CNT ENET_TXDESC_CNT
#define RXBUF_CNT ENET_RXDESC_CNT

#if (TXBUF_CNT * ENET_BUFSZ) < 2048U || (RXBUF_CNT * ENET_BUFSZ) < 2048U
#error "buffer-size * num-of-buffer must be greater than equal 2048"
/* because Tx-BDs may be underrun */
#endif

#define ENET_DEV_MAX            CFG_NET_DEV_MAX   /* currently enet is only 1ch, but "eqos" is absolutely defined 1(i.mx8m plus) */

#if (1 == ENET_RAM_USE_NETBUF)
/* Note: uNet3 requires 4-byte alignment of IP header, and ENET requires 64-byte alignment of desc buffer. */
#define ENET_SHIFT16_AJUST      2
#endif

#define ENET_ALIGN_SZ   64U

/*************************************************************************
 * Timeout (in ms units) definition for various wait events
 * used in this driver program.
 ************************************************************************/
#define PHY_RST_WAIT_TMO                                                       \
    500 /* 500ms timeout to poll PHY Reset over status  */

/*
 * Bit control definitions
 */

#define BIT_INIT_FLAG   0x0001U     /* Initialization done status flag */
#define BIT_RX_ON       0x0002U     /* Rx running status */
#define BIT_TX_ON       0x0004U     /* Tx running status */
#define BIT_RX_OVRN     0x0008U     /* Rx overrun status */
#define BIT_TX_BUSY     0x0010U     /* Waiting for Tx event */
#define BIT_LINK_DOWN   0x0100U     /* PHY media link down status */

/*************************************************************************
 * IRQ synchronization event flag bit definitions
 ************************************************************************/

#define EVENT_SOFT      0x0001U     /* Software INT test event */
#define EVENT_RX        0x0002U     /* Rx OK event */
#define EVENT_RXERR     0x0004U     /* Rx error event */
#define EVENT_TX        0x0008U     /* Tx OK event */
#define EVENT_TXERR     0x0010U     /* Tx error event */
#define EVENT_MII       0x0020U     /* MII complete event */
#define EVENT_LINKUP    0x0200U     /* Link regained */

#define EVENT_CTL_EXIT  0x1000U
#define EVENT_RX_EXIT   0x2000U
#define EVENT_TX_EXIT   0x4000U

/* Buffer Descriptor structure */
typedef UW  BDBUF;        
#define PTR2BDBUF(x)    ((UW)(ADDR)(x))
#define BDBUF2PTR(x)    ((VP)(ADDR)(x))

typedef struct t_eth_bd {
#if ENETDMA_BIGENDIAN
    UH fg;
    UH sz;
#else    
    UH sz;
    UH fg;
#endif
    BDBUF buf_p;
} T_ETH_BD;

/*************************************************************************
 * Command / Status macro definitions:
 ************************************************************************/
/*
 * Declared buffer descriptor
 */
#define ETH_DESC(name, desc_num)                                               \
    \
struct name                                                                    \
    \
{                                                                       \
        T_ETH_BD bd[(desc_num) + 1U];                                          \
        UW padding_1[15];                                                      \
    \
};

ETH_DESC(ENET_TXBD, ENET_RXDESC_CNT)
ETH_DESC(ENET_RXBD, ENET_TXDESC_CNT)

#ifdef ENET_RAM_SECTION
volatile static struct ENET_TXBD enet_txbd __attribute__((section(ENET_RAM_SECTION)));
volatile static struct ENET_RXBD enet_rxbd __attribute__((section(ENET_RAM_SECTION)));
#else
volatile static struct ENET_TXBD enet_txbd;
volatile static struct ENET_RXBD enet_rxbd;
#endif

/*
 * Declared packet buffer
 */
#define ETH_BUFF(name, desc_num, bufsize)                                      \
    \
struct name                                                                    \
    \
{                                                                       \
        UW buf[((bufsize) * (desc_num)) / 4U];                                 \
        UW padding_3[15U];                                                     \
    \
};

ETH_BUFF(ENET_TXBUFF, ENET_TXDESC_CNT, ENET_BUFSZ)
ETH_BUFF(ENET_RXBUFF, ENET_RXDESC_CNT, ENET_BUFSZ)

#if (0 == ENET_RAM_USE_NETBUF)
#ifdef ENET_RAM_SECTION
static struct ENET_RXBUFF enet_rxbuffer __attribute__((section(ENET_RAM_SECTION)));
#else
static struct ENET_RXBUFF enet_rxbuffer;
#endif
#endif
#ifdef ENET_RAM_SECTION
static struct ENET_TXBUFF enet_txbuffer __attribute__((section(ENET_RAM_SECTION)));
#else
static struct ENET_TXBUFF enet_txbuffer;
#endif


/*************************************************************************
 * Control & Status structure definition
 ************************************************************************/
typedef struct t_eth_cfg {
    VP txdesc_ptr;
    VP rxdesc_ptr;
    VP txbuf_ptr;
    VP rxbuf_ptr;
    IMASK imask;
    UH eth_bufsize;
    UB phy_addr;
    UB phy_mode;
    UB mii_mode;
    UB filter_mode;
    UB csum_mode;
    UB tx_desc_cnt;
    UB rx_desc_cnt;
} T_ETH_CFG;

typedef struct t_eth_ctl {
    UINT dev_num;
    ID snd_tsk_id;
    ID rcv_tsk_id;
    ID ctl_tsk_id;
    ID snd_mbx_id;
    ID rcv_mbx_id;
    ID evt_flg_id;
    ID isr_id;
    INTNO intno;
    const T_ETH_CFG* cfg;
    volatile struct t_enet* port;
    T_ETH_BD* rbd_top;
#if DEBUG_ETH == 1
    T_ETH_BD* rbd_p;
#endif
    T_ETH_BD* tbd_top;
    T_ETH_BD* tbd_p;
    UH bit;    /* Bit mapped control / status definitions  */
    UH physts; /* PHY media status                         */
    UH phymod_ext;
    /* before statics data */
    UH r_packet;
    UH r_bc_packet;
    UH r_mc_packet;
    UH r_crc_align;
    UH r_undersize;
    UH r_oversize;
    UH r_frag;
    UH r_jab;
    UH r_drop;
    UH t_packet;
    UH t_bc_packet;
    UH t_mc_packet;
    UH t_crc_align;
    UH t_undersize;
    UH t_oversize;
    UH t_frag;
    UH t_jab;
    UH t_drop;
} T_ETH_CTL;

const T_ETH_CFG g_eth_cfg = {
        (VP)&enet_txbd, (VP)&enet_rxbd, 
#if (0 == ENET_RAM_USE_NETBUF)
        (VP)&enet_txbuffer, (VP)&enet_rxbuffer,
#else
        (VP)&enet_txbuffer, NULL,
#endif
        ENET_IPL, ENET_BUFSZ, ENET_PHY_ADDR, ENET_PHY_MODE,
        ENET_RMII_MODE, ENET_FILTER_MODE, ENET_CSUM_MODE, ENET_TXDESC_CNT,
        ENET_RXDESC_CNT,
};

/*************************************************************************
 * Local data and function declarations
 ************************************************************************/
static T_ETH_CTL g_eth_ctl = {0};

#define REG_ENET        (*(volatile struct t_enet *)(ENET1_BASE))
#ifdef CPU_IMX_8M_QUADMAX
#define INT_ENET       INT_CONNECT_ENET0_FRAME0_EVENT_INT
#else
#define INT_ENET       INT_ENET1_MISC
#endif

static volatile struct t_enet const* g_enet_regs = &REG_ENET;

static const INTNO g_enet_intno = INT_ENET;

static ID g_phy_access_sem = 0;

/*static*/ T_NET_BUF *rxPkt[ENET_RXDESC_CNT];

/*
 * control macros
 */

#ifndef DEBUG_ETH
#define DEBUG_ETH 0
#endif

#define DEF_TXINT (ENET_EIR_TXF | ENET_EIR_TXB)
#define DEF_RXINT (ENET_EIR_RXF | ENET_EIR_RXB)
#define DEF_INT 0U
#define DEF_EIRCLR (0x7fff8000U)    /* EIR event clear */

#define BD_RERRS (RX_BD_LG | RX_BD_NO | RX_BD_CR | RX_BD_OV | RX_BD_TR)

#define ALIGN(p, n)                                                            \
    ((((unsigned long)(p)) + (((unsigned long)(n)) - 1U)) &                     \
     (~(((unsigned long)(n)) - 1U)))

/* Valid PHY mode in this Ethernet controller */
#define EMAC_SUP_PHY_MODE                                                      \
    (PME_SUP_ANE | PME_SUP_FD | PME_SUP_10M | PME_SUP_100M | PME_SUP_1000M)

        
        
#ifndef NET_C_OS

static void _ddr_imx8_enet_snd_tsk(T_ETH_CTL* eth);
static void _ddr_imx8_enet_rcv_tsk(T_ETH_CTL* eth);
static void _ddr_imx8_enet_ctl_tsk(T_ETH_CTL* eth);
static const T_CSEM c_eth_sem = {TA_TFIFO, 1U, 1U, NULL};
static const T_CFLG c_eth_flg = {TA_TPRI | TA_WMUL, 0U};
static const T_CMBX c_eth_snd_mbx = {TA_TFIFO | TA_MFIFO, 0U, NULL};
static const T_CMBX c_eth_rcv_mbx = {TA_TFIFO | TA_MFIFO, 0U, NULL};
static T_CTSK c_dev_snd_tsk = {
    TA_HLNG | TA_FPU, NULL, (FP)_ddr_imx8_enet_snd_tsk, 4U, ETH_SND_TSK_STACK_SIZE, 0U};
static T_CTSK c_dev_rcv_tsk = {
    TA_HLNG | TA_FPU, NULL, (FP)_ddr_imx8_enet_rcv_tsk, 4U, ETH_RCV_TSK_STACK_SIZE, 0U};
static T_CTSK c_dev_ctl_tsk = {
    TA_HLNG | TA_FPU, NULL, (FP)_ddr_imx8_enet_ctl_tsk, 4U, ETH_CTL_TSK_STACK_SIZE, 0U};

void _ddr_imx8_enet_intr(T_ETH_CTL* eth);
#ifdef CPU_IMX_8M_QUADMAX
static T_IMX8_IRQ_STR_CISR cdev_imx_eth_isr = {TA_HLNG | TA_FPU, NULL, INT_ENET,
                                  (FP)_ddr_imx8_enet_intr};
#else
static T_CISR cdev_imx_eth_isr = {TA_HLNG | TA_FPU, NULL, INT_ENET,
                                  (FP)_ddr_imx8_enet_intr, 0x00};
#endif

static ER enet_clk_ini(UH physts);

#else
#define ID_ETH_FLG ID_ETH_RCV_FLG
#endif

#if ENETDMA_BIGENDIAN
static UH swap16(UH value)
{
    UH ret;
    ret  = value << 8;
    ret |= value >> 8;
    return ret;
}

static UW swap32(UW value)
{
#if 1
    UW ret;
    ret  = value              << 24;
    ret |= (value&0x0000FF00) <<  8;
    ret |= (value&0x00FF0000) >>  8;
    ret |= value              >> 24;
    return ret;
#else
    /* Swap 32-bit data */
    __asm__("rev %0, %1" : "=r" (value) : "r" (value));
    return value;    
#endif
}
#else   /* no swap */

#define swap16(x)   (x)
#define swap32(x)   (x)

#endif


/*
* convert PHY mode (Normal to Extension / Extension to Normal)
 */
static INT cnv_phymod(UB n2e, INT val)
{
    INT ret = 0;

    /* setting convert */
    if (n2e) { /* Normal to Extension */
        switch ((UH)val) {
        case PM_AUTO:
            ret = (INT)EMAC_SUP_PHY_MODE;
            break;
        case PM_10M_HALF:
            ret = (INT)(PME_SUP_10M);
            break;
        case PM_10M_FULL:
            ret = (INT)(PME_SUP_10M | PME_SUP_FD);
            break;
        case PM_10M_AUTO:
            ret = (INT)(PME_SUP_10M | PME_SUP_FD | PME_SUP_ANE);
            break;
        case PM_100M_HALF:
            ret = (INT)(PME_SUP_100M);
            break;
        case PM_100M_FULL:
            ret = (INT)(PME_SUP_100M | PME_SUP_FD);
            break;
        case PM_100M_AUTO:
            ret = (INT)(PME_SUP_100M | PME_SUP_FD | PME_SUP_ANE);
            break;
        case PM_1000M_HALF:
            ret = (INT)(PME_SUP_1000M);
            break;
        case PM_1000M_FULL:
            ret = (INT)(PME_SUP_1000M | PME_SUP_FD);
            break;
        case PM_1000M_AUTO:
            ret = (INT)(PME_SUP_1000M | PME_SUP_FD | PME_SUP_ANE);
            break;
        default: /* Unknown Extension settings */
            ret = -1;
            break;
        }
        /* Check specifications */
        if (0 != (((INT)~EMAC_SUP_PHY_MODE) & ret)) {
            ret = -1;
        }
    } else { /* Extension to Normal */
        switch ((UH)((UH)val & ~PME_CFG_NSET)) {
        case EMAC_SUP_PHY_MODE:
            ret = (INT)PM_AUTO;
            break;
        case (PME_SUP_10M):
            ret = (INT)PM_10M_HALF;
            break;
        case (PME_SUP_10M | PME_SUP_FD):
            ret = (INT)PM_10M_FULL;
            break;
        case (PME_SUP_10M | PME_SUP_FD | PME_SUP_ANE):
            ret = (INT)PM_10M_AUTO;
            break;
        case (PME_SUP_100M):
            ret = (INT)PM_100M_HALF;
            break;
        case (PME_SUP_100M | PME_SUP_FD):
            ret = (INT)PM_100M_FULL;
            break;
        case (PME_SUP_100M | PME_SUP_FD | PME_SUP_ANE):
            ret = (INT)PM_100M_AUTO;
            break;
        case (PME_SUP_1000M):
            ret = (INT)PM_1000M_HALF;
            break;
        case (PME_SUP_1000M | PME_SUP_FD):
            ret = (INT)PM_1000M_FULL;
            break;
        case (PME_SUP_1000M | PME_SUP_FD | PME_SUP_ANE):
            ret = (INT)PM_1000M_AUTO;
            break;
        default: /* Unknown Normal settings */
            ret = -1;
            break;
        }
    }

    return ret;
}

/*
* get default PHY mode (from DDR_xxx_ETH_cfg.h - ETH_PHY_MODE)
 */
static UH get_def_phymod(T_ETH_CTL* eth, UB ext)
{
    UH ret = 0U;

    if (ext) { /* Extension setting */
        ret = (UH)cnv_phymod(1U, (INT)eth->cfg->phy_mode);
    } else { /* Normal setting */
        ret = (UH)eth->cfg->phy_mode;
    }

    return ret;
}

/*************************************************************************
 * PHY Register initialization / read / write access function.
 *      phy_get     = Read   data to 16-bit PHY Register
 *      phy_put     = Write  data to 16-bit MAC Register
 ************************************************************************/

/*
 * read from PHY
 */

static UH phy_get(T_ETH_CTL* eth, const UINT reg)
{
    UW mii;
    volatile struct t_enet* port = eth->port;
    
    (void)wai_sem(g_phy_access_sem);
    
    mii = (UW)(ENET_MMFR_ST | ENET_MMFR_OP_R | ENET_MMFR_TA);
    mii |= (UW)((UW)eth->cfg->phy_addr << ENET_MMFR_PA_SHIFT);
    mii |= reg << ENET_MMFR_RA_SHIFT;

    port->EIR = ENET_EIR_MII; /* clear event */
    port->MMFR = mii;

    while (!(port->EIR & ENET_EIR_MII))
        ;
    
    (void)sig_sem(g_phy_access_sem);
    
    mii = port->MMFR & 0xffffUL;

    return (UH)mii;
}

/*
 * write to PHY
 */

static void phy_put(T_ETH_CTL* eth, const UINT reg, UH dat)
{
    UW mii;
    volatile struct t_enet* port = eth->port;

    (void)wai_sem(g_phy_access_sem);
    
    mii = (UW)(ENET_MMFR_ST | ENET_MMFR_OP_W | ENET_MMFR_TA);
    mii |= (UW)((UW)eth->cfg->phy_addr << ENET_MMFR_PA_SHIFT);
    mii |= reg << ENET_MMFR_RA_SHIFT;
    mii |= dat;

    port->EIR = ENET_EIR_MII; /* clear event */
    port->MMFR = mii;

    while (!(port->EIR & ENET_EIR_MII))
        ;
    
    (void)sig_sem(g_phy_access_sem);
}

#ifdef CPU_IMX_8M_QUADMAX
static void dp83867_spec_cfg(T_ETH_CTL* eth)
{
#define REGCR_ADR       0x000DU
#define ADDAR_ADR       0x000EU
    UH val;

    if (eth->cfg->mii_mode == 2U){
        /*  Viterbi Module Configuration */
        phy_put(eth, REGCR_ADR, 0x1fU);
        phy_put(eth, ADDAR_ADR, 0x53U);
        phy_put(eth, REGCR_ADR, 0x401fU);
        val = phy_get(eth, ADDAR_ADR);
        phy_put(eth, ADDAR_ADR, val | 0x5U); /* Skew of RX channel A to align symbols in # of clock cycles*/

        /*  RGMII Control */
        phy_put(eth, REGCR_ADR, 0x1fU);
        phy_put(eth, ADDAR_ADR, 0x32U);
        phy_put(eth, REGCR_ADR, 0x401fU);
        val = phy_get(eth, ADDAR_ADR);
        phy_put(eth, REGCR_ADR, 0x401fU);
        phy_put(eth, ADDAR_ADR, val|0xD3U);  /* TX, RX clock delay is shifted relative to data */

        /* RGMII Delay Control */
        phy_put(eth, REGCR_ADR, 0x1fU);
        phy_put(eth, ADDAR_ADR, 0x86U);
        phy_put(eth, REGCR_ADR, 0x401fU);
        phy_put(eth, ADDAR_ADR, 0x77U);  /* TX 2.00ns / RX 2.00ns */
#if 0
        phy_put(eth, REGCR_ADR, 0x1fU);
        phy_put(eth, ADDAR_ADR, 0x2DU);
        phy_put(eth, REGCR_ADR, 0x401fU);
        val = phy_get(eth, ADDAR_ADR);
        phy_put(eth, ADDAR_ADR, val | 0x8000U);
#endif
        /* LED Control */
        phy_put(eth, REGCR_ADR, 0x1fU);
        phy_put(eth, ADDAR_ADR, 0x172U);
        phy_put(eth, REGCR_ADR, 0x401fU);
        phy_put(eth, ADDAR_ADR, 0x6U);  /* LED_3 */

        phy_put(eth, REGCR_ADR, 0x1fU);
        phy_put(eth, ADDAR_ADR, 0x18U);
        phy_put(eth, REGCR_ADR, 0x401fU);
        phy_put(eth, ADDAR_ADR, 0x8b5bU);  /* LED3:10/100 LED2:link&blinkTX/RX LED1:1000 LED0:link&blinkTX/RX  */
    }
}
#endif

/*************************************************************************
 * Function  : phy_ini
 * Purpose   : PHY device initialization
 * Arguments : none
 * Return    : E_OK     = PHY setup successful
 *             E_TMOUT  = PHY reset command error (timeout)
 *
 * Comments  : only auto-negotiation mode
 ************************************************************************/
static ER phy_ini(T_ETH_CTL* eth)
{
    INT i;
    UH tmp;

    phy_put(eth, PHY_BMCR, BMCR_RESET); /* Apply PHY reset command */
    i = PHY_RST_WAIT_TMO / 100;
    while (1) {
        dly_tsk(100);

        if (!(phy_get(eth, PHY_BMCR) & BMCR_RESET)) {
            break;
        }
        if (i-- <= 0) {
            return E_TMOUT; /* Error: PHY reset command error */
        }
    }

#ifndef CPU_IMX_8M_QUADMAX
    /* setting for AR8031 */
    phy_put(eth, 0xdU, 0x7U);
    phy_put(eth, 0xeU, 0x8016U);
    phy_put(eth, 0xdU, 0x4007U);

    tmp = phy_get(eth, 0xeU);
    tmp &= 0xffe3U;
    tmp |= 0x0018U;
    phy_put(eth, 0xeU, tmp);

    /* introduce tx clock delay */
    phy_put(eth, 0x1dU, 0x5U);
    tmp = phy_get(eth, 0x1eU);
    tmp |= 0x0100U;
    phy_put(eth, 0x1eU, tmp);
#endif

    (void)phy_get(eth, PHY_BMSR);    /* Dummy status read */
    eth->bit |= BIT_LINK_DOWN; /* PHY link down */
    eth->physts = PHY_STS_LINK_DOWN;

    if (0U == eth->phymod_ext) { /* invalidate PHY mode settings */
        eth->phymod_ext = get_def_phymod(eth, 1U);
    }
#ifdef CPU_IMX_8M_QUADMAX
    if ((eth->phymod_ext & PME_CFG_NSET) != PME_CFG_NSET) {
        dp83867_spec_cfg(eth);
    }
#endif

    if (eth->phymod_ext & PME_SUP_ANE) { /* Auto negotiation */
        tmp = ANAR_SF_802_3u;
        if (eth->phymod_ext & PME_SUP_10M) {
            tmp |= ANAR_10;
            if (eth->phymod_ext & PME_SUP_FD)
                tmp |= ANAR_10_FD;
        }
        if (eth->phymod_ext & PME_SUP_100M) {
            tmp |= ANAR_TX;
            if (eth->phymod_ext & PME_SUP_FD)
                tmp |= ANAR_TX_FD;
        }

        phy_put(eth, PHY_BMCR, BMCR_ANE);
        phy_put(eth, PHY_ANAR, tmp);
#if (EMAC_SUP_PHY_MODE & PME_SUP_1000M) /* only Gigabit support */
        tmp = 0U;
        if (eth->phymod_ext & PME_SUP_1000M) {
            tmp |= GCTL_1000_HD;
            if (eth->phymod_ext & PME_SUP_FD)
                tmp |= GCTL_1000_FD;
        }
        phy_put(eth, PHY_1000_CTL, tmp);
#endif
        phy_put(eth, PHY_BMCR, BMCR_ANE | BMCR_RS_ANP);
    } else { /* Manual settings */
        tmp = 0U;
        if (eth->phymod_ext & PME_SUP_1000M) {
            tmp |= BMCR_SPD1000;
        } else if (eth->phymod_ext & PME_SUP_100M) {
            tmp |= BMCR_SPD100;
        } else {
            tmp |= BMCR_SPD10;
        }
        if (eth->phymod_ext & PME_SUP_FD) {
            tmp |= BMCR_DUPLEX;
        }
#if (EMAC_SUP_PHY_MODE & PME_SUP_1000M) /* only Gigabit support */
        phy_put(eth, PHY_1000_CTL, 0U);
#endif
        phy_put(eth, PHY_BMCR, tmp);
    }

    eth->phymod_ext &= ~PME_CFG_NSET; /* clear nset */

    return E_OK;
}

/*************************************************************************
 * Stopped controller
 ************************************************************************/

static void eth_stop(T_ETH_CTL* eth)
{
    volatile struct t_enet* port = eth->port;

    port->EIMR = DEF_INT;
    port->EIR = ENET_EIR_GRA;
    port->TCR |= ENET_TCR_GTS;
    _kernel_synch_cache();
    while (!(port->EIR & ENET_EIR_GRA))
        dly_tsk(1);
    port->ECR &= ~ENET_ECR_ETHEREN;
    _kernel_synch_cache();
}

/*************************************************************************
 * Restarted controller for changed duplex
 ************************************************************************/

static void eth_restart(T_ETH_CTL* eth, UH physts)
{
    UINT i;
    T_ETH_BD* bd_p;
    volatile struct t_enet* port = eth->port;
    UB *tbuf_p;
    UW tbuf_w = swap32((UW)eth->tbd_top->buf_p);
#if (0 == ENET_RAM_USE_NETBUF)
    UB *rbuf_p;
    UW rbuf_w = swap32((UW)eth->rbd_top->buf_p);
#else
    T_NET_DEV* dev = &gNET_DEV[eth->dev_num - 1];
#endif

    eth->tbd_p = eth->tbd_top;
    tbuf_p = BDBUF2PTR(tbuf_w);
#if (0 == ENET_RAM_USE_NETBUF)
    rbuf_p = BDBUF2PTR(rbuf_w);
#endif

    bd_p = eth->tbd_top;
    for (i = 0U; i < eth->cfg->tx_desc_cnt; i++, bd_p++) {
        bd_p->buf_p = PTR2BDBUF(swap32((UW)(ADDR)tbuf_p));
        tbuf_p += eth->cfg->eth_bufsize;
        bd_p->sz = 0U;
        bd_p->fg = 0U;
    }
    bd_p--;
    bd_p->fg |= TX_BD_W;

    bd_p = eth->rbd_top;
    for (i = 0U; i < eth->cfg->rx_desc_cnt; i++, bd_p++) {
#if (0 == ENET_RAM_USE_NETBUF)
        bd_p->buf_p = PTR2BDBUF(swap32((UW)(ADDR)rbuf_p));
        rbuf_p += eth->cfg->eth_bufsize;
#else
        bd_p->buf_p = PTR2BDBUF(swap32(rxPkt[i]->buf + dev->hhdrofs - ENET_SHIFT16_AJUST));
#endif
        bd_p->sz = 0U;
        bd_p->fg = RX_BD_E;
    }
    bd_p--;
    bd_p->fg |= RX_BD_W;

    port->RDSR = PTR2BDBUF(eth->rbd_top);
    port->TDSR = PTR2BDBUF(eth->tbd_top);
    port->MRBR = RBUFSZ_MAX;

    if (physts == PHY_STS_1000FD || physts == PHY_STS_100FD ||
        physts == PHY_STS_10FD) {
        port->TCR = ENET_TCR_FDEN; /* full duplex */
        port->RCR = (UW)(ENET_RCR_MII_MODE | ENET_RCR_CRCFWD |
                    ((eth->cfg->eth_bufsize + 4U) << ENET_RCR_MAXFL_SHIFT));
    } else {
        port->TCR = 0U; /* half duplex */
        port->RCR = (UW)(ENET_RCR_MII_MODE | ENET_RCR_DRT | ENET_RCR_CRCFWD |
                    ((eth->cfg->eth_bufsize + 4U) << ENET_RCR_MAXFL_SHIFT));
    }
    if (eth->cfg->mii_mode == 1U) {
        if (physts == PHY_STS_10FD || physts == PHY_STS_10HD)
            port->RCR |= (UW)(ENET_RCR_RMII_MODE | ENET_RCR_RMII_10T);
        else
            port->RCR |= (ENET_RCR_RMII_MODE);
    } else if (eth->cfg->mii_mode == 2U) {
        if (physts == PHY_STS_10FD || physts == PHY_STS_10HD)
            port->RCR |= (UW)(ENET_RCR_RGMII_EN | ENET_RCR_RMII_10T);
        else
            port->RCR |= (ENET_RCR_RGMII_EN);

        if (physts == PHY_STS_1000FD)
            port->ECR |= (ENET_ECR_SPEED);
    } else {
        /* Do Nothing */
    }

    if (eth->cfg->filter_mode == 1U) {
        port->RCR |= ENET_RCR_PROM;
    }
    /* SNMP counter enable  */
    port->MIBC &= ~(ENET_MIBC_CLEAR);
    _kernel_synch_cache();
    port->MIBC &= ~(ENET_MIBC_DIS);

#if DEBUG_ETH == 1
    port->ECR |= ENET_ECR_ETHEREN | ENET_ECR_DBGEN;
#else
    port->ECR |= ENET_ECR_ETHEREN;
#endif

    port->RDAR = ENET_DES_ACTIVE; /* restart Rx */

    port->EIR = DEF_EIRCLR; /* clear event */
    port->EIMR = DEF_INT;
    _kernel_synch_cache();
}


static void enet_reinit(T_ETH_CTL* eth)
{
    volatile struct t_enet* port = eth->port;
    UW ipgclk;
    UW hold;
    UB* mac;
    T_NET* net;
    T_NET_DEV* dev;
    
    net = &gNET[eth->dev_num - 1];
    dev = net->dev;
    if (dev == NULL) {
        return;
    }
    mac = &dev->cfg.eth.mac[0];

    port->ECR = ENET_ECR_RESET; /* Reset MAC */
    dly_tsk(1); /* wait 8 system clock for reset sequence progress */
#if ENETDMA_BIGENDIAN
    port->ECR &= ~ENET_ECR_DBSWP; /* Discriptor Swap */
#else
    port->ECR |= ENET_ECR_DBSWP; /* Discriptor Swap */
#endif
    ipgclk = ENET_MDC_CLK;
    ipgclk = ipgclk / 5000000U;
    hold = (ipgclk / 100000000U) - 1U;

    port->MSCR = ((ipgclk << 1U) & 0x7eU) | ((hold << 8U) & 0x0300U);

    port->EIR = DEF_EIRCLR; /* clear event */
    port->EIMR = DEF_INT;

    if (((mac[0] | mac[1] | mac[2] | mac[3] | mac[4] | mac[5]) == 0x00U) ||
        ((mac[0] & mac[1] & mac[2] & mac[3] & mac[4] & mac[5]) == 0xFFU)) {
        return;
    }

    port->PALR = ((UW)mac[0] << 24) | ((UW)mac[1] << 16) | ((UW)mac[2] << 8) |
                 ((UW)mac[3]);
    port->PAUR = ((UW)mac[4] << 24) | ((UW)mac[5] << 16);

    if (eth->cfg->filter_mode == 0U) {
        port->GALR = 0U; /* multicast setting (all disable)  */
        port->GAUR = 0U;
    } else {
        port->GALR = 0xFFFFFFFFU; /* multicast setting (all enable)  */
        port->GAUR = 0xFFFFFFFFU;
    }

    port->IALR = 0U;
    port->IAUR = 0U;

    port->TFWR |= ENET_TFWR_STRFWD;

    if (eth->cfg->csum_mode & 1U) {
        port->TACC = (UW)(ENET_TACC_PROCHK | ENET_TACC_IPCHK);
    }
    if (eth->cfg->csum_mode & 2U) {
        port->RACC = (UW)(ENET_RACC_LINEDIS | ENET_RACC_PRODIS | ENET_TACC_IPDIS);
    }
#if (1 == ENET_RAM_USE_NETBUF)
    port->RACC |= ENET_RACC_SHIFT16;
#endif
}

/*************************************************************************
 * Function  : phy_sts_hdr
 * Purpose   : Performs necessary setup after PHY status has changed
 * Arguments : none
 * Return    : none
 * Comments  : none
 ************************************************************************/

static void phy_sts_hdr(T_ETH_CTL* eth)
{
    UH bmsr, physts;
    T_NET_DEV* dev = &gNET_DEV[eth->dev_num - 1];

    /* PHY mode change request */
    if (eth->phymod_ext & PME_CFG_NSET) {
        /* Electrical isolation */
        (void)phy_get(eth, PHY_BMSR); /* Dummy status read */
        (void)phy_put(eth, PHY_BMCR, BMCR_ISOL);
        dly_tsk(100); /* wait isolation done */

        /* PHY initialize */
        enet_reinit(eth);
        (void)phy_ini(eth);
    }
    (void)phy_get(eth, PHY_BMSR); /* released latched status */
    bmsr = phy_get(eth, PHY_BMSR);
    if (!(bmsr & BMSR_LINK_STAT)) {
        if (eth->physts != PHY_STS_LINK_DOWN) {
            eth_stop(eth);
            eth->physts = PHY_STS_LINK_DOWN;
/* inform this event by COMMAND event interface */
#if (SNMP_ENA == 1)
            net_sts_eth_cbk(eth->dev_num, EV_CBK_DEV_LINK, PHY_STS_LINK_DOWN);
#endif
            if (dev->cbk) {
                dev->cbk(eth->dev_num, EV_CBK_DEV_LINK, PHY_STS_LINK_DOWN);
            }
        }
        eth->bit |= BIT_LINK_DOWN;
        return;
    }

    physts = 0U;
    if (eth->phymod_ext & PME_SUP_ANE) {
        /* wait until autonegotiation is done its part */
        while (!((bmsr = phy_get(eth, PHY_BMSR)) & BMSR_ANEG_COMP)) {
            if (!(eth->bit & BIT_INIT_FLAG)) {
                ext_tsk();
            }
            dly_tsk(1);
        }
        if (eth->phymod_ext & PME_SUP_1000M) {
            bmsr = (UH)(phy_get(eth, PHY_1000_CTL) << 2);
            bmsr &= phy_get(eth, PHY_1000_STS);
            if ((bmsr & GSTS_1000_FD) || (bmsr & GSTS_1000_HD)) {
                physts = PHY_STS_1000FD; /* Gigabit halfduplex not support */
            }
        }
        if (0U == (physts & PHY_STS_1000FD)) {
            bmsr = phy_get(eth, PHY_ANLPAR);
            bmsr &= phy_get(eth, PHY_ANAR);
            if (bmsr & ANLPAR_B_TX_FD)
                physts = PHY_STS_100FD;
            else if (bmsr & ANLPAR_B_TX)
                physts = PHY_STS_100HD;
            else if (bmsr & ANLPAR_B_10_FD)
                physts = PHY_STS_10FD;
            else
                physts = PHY_STS_10HD;
        }
    } else {
        physts = (eth->phymod_ext & PME_SUP_1000M)
                     ? PHY_STS_1000FD
                     : (eth->phymod_ext & PME_SUP_100M)
                           ? ((eth->phymod_ext & PME_SUP_FD) ? PHY_STS_100FD
                                                             : PHY_STS_100HD)
                           : ((eth->phymod_ext & PME_SUP_FD) ? PHY_STS_10FD
                                                             : PHY_STS_10HD);
    }

    
    if (eth->physts != physts) {
        enet_clk_ini(physts);   /* change clock settings for port speed. */

        if (eth->physts != PHY_STS_LINK_DOWN)
            eth_stop(eth); /* Oops!! but go stop-sequence if it is happen */
        eth_restart(eth, physts); /* restart */

        eth->physts = physts;
        eth->bit &= ~BIT_LINK_DOWN;
        if (eth->bit & BIT_TX_BUSY) {
            set_flg(eth->evt_flg_id, EVENT_LINKUP);
        }

/* invoke device callback */
#if (SNMP_ENA == 1)
        net_sts_eth_cbk(eth->dev_num, EV_CBK_DEV_LINK, (VP)(ADDR)eth->physts);
#endif
        if (dev->cbk) {
            dev->cbk(eth->dev_num, EV_CBK_DEV_LINK, (VP)(ADDR)eth->physts);
        }
    }
}

/*************************************************************************
 * Function  : _ddr_imx8_enet_intr
 * Purpose   : Handles interrupt events
 * Arguments : none
 * Return    : none
 * Comments  : none
 ************************************************************************/
void _ddr_imx8_enet_intr(T_ETH_CTL* eth)
{
    UW eir;

    volatile struct t_enet* port = eth->port;

    eir = port->EIR;
    eir &= port->EIMR;

    if (eir & DEF_RXINT) {
        iset_flg(eth->evt_flg_id, EVENT_RX);
    }

    if (eir & DEF_TXINT) {
        iset_flg(eth->evt_flg_id, EVENT_TX);
    }

    port->EIR = eir; /* clear event */
    _kernel_synch_cache();
}


/*************************************************************************
 * Task      : _ddr_imx8_enet_ctl_tsk
 * Purpose   : Controls asynchronous reception
 * Arguments : none
 * Return    : none
 *
 * Comments  : none
 ************************************************************************/
void _ddr_imx8_enet_ctl_tsk(T_ETH_CTL* eth)
{
#if (0 == ENET_RAM_USE_NETBUF)
    UB* buf_p;
#else
#if (0 != UNTIL_RECV_DESC_EMPTY)
    TMO tmo_retbuf;
    T_ETH_BD *nbuf_p = NULL;
#endif
    UB index;
#endif
    T_NET_DEV* dev;
    T_NET_BUF* pkt;
    T_ETH_BD *rbd_p, *ebd_p;
    volatile struct t_enet* port = eth->port;
    char* bp;
    ER ercd;
    FLGPTN event;
    UH len;

    (void)clr_flg(eth->evt_flg_id, ~EVENT_CTL_EXIT);

    dev = &gNET_DEV[eth->dev_num - 1];
    rbd_p = eth->rbd_top;

    while (0 != (eth->bit & BIT_INIT_FLAG)) {

#if DEBUG_ETH == 1
        g_eth_ctl.rbd_p = rbd_p;
#endif

        /* check link */
        if ((eth->bit & BIT_LINK_DOWN) != 0x00000000U) {
            while (1) {
                phy_sts_hdr(eth);
                if (!(eth->bit & BIT_LINK_DOWN)) {
                    rbd_p = eth->rbd_top;
                    break;
                }
                tslp_tsk(PHY_POLL_TMOUT);
            }
        }

        /* check data */
        DCACHE_INVALID(rbd_p, sizeof(T_ETH_BD));
        if (rbd_p->fg & RX_BD_E) { /* no rx-data */
            loc_cpu();
            port->EIMR |= (UW)DEF_RXINT;
            unl_cpu();

            ercd = twai_flg(eth->evt_flg_id, EVENT_RX, TWF_ORW, &event,
                            (TMO)PHY_POLL_TMOUT);

            loc_cpu();
            
            port->EIMR &= ~DEF_RXINT;
            unl_cpu();

            if (ercd == E_OK) {
                clr_flg(eth->evt_flg_id, ~EVENT_RX);
            } else if (ercd == E_TMOUT) {
                phy_sts_hdr(eth);
            } else {
                /* Do Nothing */
            }
            continue;
        }

        ebd_p = rbd_p;

        if (rbd_p->fg & RX_BD_W)
            rbd_p = eth->rbd_top;
        else
            rbd_p++;

        if (ebd_p->fg & BD_RERRS) { /* discard if Rx error */
            goto _skip;
        }

        len = swap16(ebd_p->sz);
#if (0 == ENET_RAM_USE_NETBUF)
        /* Allocate buffer */
        ercd = net_buf_get(&pkt, (SIZE)len, TMO_POL);
        if (ercd != E_OK) {
            /* Stack memory not available. Drop this packet */
            goto _skip;
        }

        buf_p = (UB*)pkt->buf + dev->hhdrofs; /* skip offset */
        bp = BDBUF2PTR(swap32((UW)ebd_p->buf_p));
        DCACHE_INVALID(bp, len);
        net_memcpy(buf_p, bp, (SIZE)len);
        if (ebd_p->fg & RX_BD_W) {
            ebd_p->fg = RX_BD_E | RX_BD_W;
        } else {
            ebd_p->fg = RX_BD_E;
        }
        _kernel_synch_cache();
        if (!(port->RDAR & ENET_DES_ACTIVE))
            port->RDAR = ENET_DES_ACTIVE; /* restart */
        _kernel_synch_cache();

#else
        bp = BDBUF2PTR(swap32((UW)ebd_p->buf_p));
        DCACHE_INVALID(bp, len);
        index = *(UB*)(bp - (dev->hhdrofs - ENET_SHIFT16_AJUST));     /* pkt->buf[0] == index */
        pkt = rxPkt[index];
        // Note: (bp == pkt->buf + dev->hhdrofs)
#endif
        /* send to upper layer */
        /* Transfer the received packet to TCP/IP */
        /* In received packet content,            */
        /* pkt->hdr should point Ethernet header  */
        /* pkt->dat should point IP               */
        //pkt->hdr = (UB*)bp;
        pkt->hdr = pkt->buf + dev->hhdrofs;
        pkt->hdr_len = ETH_HDR_SZ;
        pkt->dat = pkt->hdr + pkt->hdr_len;
        pkt->dat_len = len - pkt->hdr_len;
        pkt->dev = dev;
        if (eth->cfg->csum_mode & 2U) {
            pkt->flg |= (HW_CS_RX_IPH4 | HW_CS_RX_DATA);
        }
        snd_mbx(eth->rcv_mbx_id, (T_MSG*)pkt);

#if (1 == ENET_RAM_USE_NETBUF)
#if (0 == UNTIL_RECV_DESC_EMPTY)
        /* allocate buffer */
        ercd = net_buf_get(&pkt, ENET_BUFSZ, TMO_FEVR);
        if (ercd != E_OK) {
            /* Fatal Error! */
            break;
        }
        pkt->buf[0] = index;
        rxPkt[index] = pkt;
        DCACHE_CLEAR(pkt, ENET_BUFSZ);
        ebd_p->buf_p = PTR2BDBUF((UB*)swap32(pkt->buf + dev->hhdrofs - ENET_SHIFT16_AJUST));
        ebd_p->fg |= RX_BD_E;

#else
        /* Continues receiving subsequent descriptors even if the buffer cannot be allocated. */
        rxPkt[index] = NULL;
        ebd_p->sz = index;
        if (NULL == nbuf_p) {
            nbuf_p = ebd_p;
        }
        /* If all recv descriptor's network buffers are empty, wait until the buffer becomes free. */
        tmo_retbuf = (nbuf_p == rbd_p) ? TMO_FEVR : ENET_RX_NBUFGET_TMO ;
        while (1) {
            index = nbuf_p->sz;
            ercd = net_buf_get(&pkt, ENET_BUFSZ, tmo_retbuf);
            if (ercd != E_OK) {
                /* Attempts to acquire the buffer again when the next packet is received. */
                goto _skip_nomem;
            }
            pkt->buf[0] = index;
            rxPkt[index] = pkt;
            DCACHE_CLEAR(pkt, ENET_BUFSZ);
            nbuf_p->buf_p = (UB*)swap32(pkt->buf + dev->hhdrofs - ENET_SHIFT16_AJUST);
            nbuf_p->fg |= RX_BD_E;

            if (nbuf_p == ebd_p) {
                nbuf_p = NULL;
                break;
            }
            nbuf_p = (nbuf_p->fg & RX_BD_W) ? eth->rbd_top : nbuf_p + 1 ;
            tmo_retbuf = TMO_POL;
        }
    _skip_nomem:
#endif
#endif
    
        port->RDAR = ENET_DES_ACTIVE; /* restart */
        rot_rdq(TPRI_SELF);
        continue;

    _skip:
        ebd_p->fg |= RX_BD_E;
        port->RDAR = ENET_DES_ACTIVE; /* restart */
    }

    (void)set_flg(eth->evt_flg_id, EVENT_CTL_EXIT);
    ext_tsk();
}

/*************************************************************************
 * Transmit frame
 ************************************************************************/
ER eth_tx(T_ETH_CTL* eth, T_NET_BUF* pkt)
{
    ER ercd;
    UH len;
    UB* buf;
    FLGPTN event;
    T_ETH_BD *tbd_p;
    volatile struct t_enet* port = eth->port;
    char* bp;

    if (!(eth->bit & BIT_INIT_FLAG)) {
        return E_OBJ; /* Error: Device Not initialized */
    }

    len = pkt->hdr_len;
    buf = pkt->hdr;

    if (len == 0U || len > eth->cfg->eth_bufsize) {
        return E_PAR;
    }

    eth->bit |= BIT_TX_BUSY;
    while (eth->bit & BIT_LINK_DOWN) {
        /* Wait until link is back */
        ercd = twai_flg(eth->evt_flg_id, EVENT_LINKUP, TWF_ORW, &event,
                        ETH_TX_TMOUT);
        if (ercd != E_OK) {
            eth->bit &= ~BIT_TX_BUSY;
            return ercd;
        }
        clr_flg(eth->evt_flg_id, ~EVENT_LINKUP);
    }

    tbd_p = eth->tbd_p;

    while (1) {
        if (port->EIR & DEF_TXINT)
            port->EIR = (UW)DEF_TXINT;
        DCACHE_INVALID(tbd_p, sizeof(T_ETH_BD));
        if (tbd_p->fg & TX_BD_R) { /* nothing buffer */
            if (!(port->TDAR & ENET_DES_ACTIVE)) {
                port->TDAR = ENET_DES_ACTIVE; /* restart */
                continue;
            }
        
            loc_cpu();
            port->EIMR |= (UW)DEF_TXINT;
            unl_cpu();

            ercd = twai_flg(eth->evt_flg_id, EVENT_TX | EVENT_LINKUP, TWF_ORW,
                            &event, TMO_FEVR);

            loc_cpu();
            port->EIMR &= ~DEF_TXINT; /* disable Tx INT */
            unl_cpu();

            if (ercd == E_OK)
                clr_flg(eth->evt_flg_id, ~(event & (EVENT_TX | EVENT_LINKUP)));

            if (event & EVENT_LINKUP) {
                eth->bit &= ~BIT_TX_BUSY;
                return E_TMOUT; /* can not recovery */
            }
            continue;
        }

        tbd_p->fg &= TX_BD_W; /* clear flag */
        bp = BDBUF2PTR(swap32((UW)tbd_p->buf_p));
        tbd_p->sz = (UH)(swap16(len));
        net_memcpy(bp, buf, (SIZE)len);
        DCACHE_CLEAR(bp, len);
        break;
    }

    loc_cpu();
    tbd_p->fg |= TX_BD_L | TX_BD_TC; /* last buffer in frame */
    if (tbd_p->fg & TX_BD_W) {
        tbd_p->fg |= (TX_BD_R | TX_BD_W);
    } else {
        tbd_p->fg |= TX_BD_R;
    }
    unl_cpu();

    if (!(port->TDAR & ENET_DES_ACTIVE))
        port->TDAR = ENET_DES_ACTIVE; /* restart */
    _kernel_synch_cache();

    if (tbd_p->fg & TX_BD_W)
        eth->tbd_p = eth->tbd_top;
    else
        eth->tbd_p++;
    eth->bit &= ~BIT_TX_BUSY;

    return E_OK;
}

#ifdef CPU_IMX_8M_QUADMAX
#include "scfw.h"
#include "types.h"
#include "pm_api.h"
#include "misc_api.h"
extern sc_ipc_t ipcHandle; /* ipc handle */

static ER enet_clk_ini(UH physts)
{
    ER ercd;
    sc_err_t ret;
    uint32_t clk = 125000000;   /* 125MHz */

    if (0 == physts) {  /* Initialize */
        ercd = E_OBJ;
        ret = sc_pm_set_resource_power_mode(ipcHandle, SC_R_ENET_0, SC_PM_PW_MODE_ON);
        if (SC_ERR_NONE != ret){
            goto ERR_END;
        }
        ret = sc_pm_clock_enable(ipcHandle, SC_R_ENET_0, SC_PM_CLK_PER, false, true);
        if (SC_ERR_NONE != ret){
            goto ERR_END;
        }
        ret = sc_pm_set_clock_rate(ipcHandle, SC_R_ENET_0, SC_PM_CLK_PER, &clk);
        if (SC_ERR_NONE != ret){
            goto ERR_END;
        }
        ret = sc_misc_set_control(ipcHandle, SC_R_ENET_0, SC_C_DISABLE_50, 1);
        if (SC_ERR_NONE != ret){
            goto ERR_END;
        }
        ret = sc_misc_set_control(ipcHandle, SC_R_ENET_0, SC_C_TXCLK, 0);
        if (SC_ERR_NONE != ret){
            goto ERR_END;
        }
        ret = sc_misc_set_control(ipcHandle, SC_R_ENET_0, SC_C_SEL_125, 1);
        if (SC_ERR_NONE != ret){
            goto ERR_END;
        }
        ret = sc_misc_set_control(ipcHandle, SC_R_ENET_0, SC_C_CLKDIV, 0);
        if (SC_ERR_NONE != ret){
            goto ERR_END;
        }
        ret = sc_pm_clock_enable(ipcHandle, SC_R_ENET_0, SC_PM_CLK_PER, true, true);
        if (SC_ERR_NONE != ret){
            goto ERR_END;
        }
        _kernel_synch_cache();
    }
    ercd = E_OK;

ERR_END:
    return ercd;
}

#else  /* CPU_IMX_8M_QUADMAX */

static void enet_target_clock_init(UB idx, UB mux, UB pre_div, UB post_div)
{
    UW tempreg;

    /* Set {idx} clock root to Osc24M */
    tempreg = REG_CCM_ROOT(idx).TARGET_ROOT;
    tempreg = (tempreg & ~0x7000000UL) | (0UL << 24);
    /* Set {idx}  root divider to {pre_div, post_div} */
    tempreg |= (tempreg & (~(0x70000UL | 0x3FU))) | ((pre_div - 1U) << 16U) | ((post_div - 1U) << 0U);
    /* Set {idx}  clock root to {mux} clock */
    tempreg |= (tempreg & ~0x7000000UL) | (mux << 24);

    REG_CCM_ROOT(idx).TARGET_ROOT = tempreg;
}

static ER enet_clk_ini(UH physts)
{
    /* disable the clock gate */
    REG_CCM_CCGR(CCGR_ENET1).CCGR_CLR = 3;
    
    if (0 == physts) {  /* Initialize */
        enet_target_clock_init(ENET_AXI_CLK_ROOT, 1, 1, 1);          /* 266MHz, SYSTEM_PLL1_DIV3 */
        enet_target_clock_init(ENET_PHY_REF_CLK_ROOT, 1, 1, 1);          /* 125MHz, SYSTEM_PLL2_DIV8 */
        enet_target_clock_init(ENET_TIMER_CLK_ROOT, 1, 1, 4);        /* 25MHz,  SYSTEM_PLL2_DIV10 */
    }
    else {  /* Change port speed. */
        /* Note: If the clock is not suitable for the port speed, transmission will fail. */
        if (physts & (PHY_STS_10FD | PHY_STS_10HD)) {
            enet_target_clock_init(ENET_PHY_REF_CLK_ROOT, 1, 1, 50); /* 2.5MHz, SYSTEM_PLL2_DIV8 */
        }
        else if (physts & (PHY_STS_100FD | PHY_STS_100HD)) {
            enet_target_clock_init(ENET_PHY_REF_CLK_ROOT, 1, 1, 5);  /*  25MHz, SYSTEM_PLL2_DIV8 */
        }
        else { /* if (physts & (PHY_STS_1000FD | PHY_STS_1000HD)) */
            enet_target_clock_init(ENET_PHY_REF_CLK_ROOT, 1, 1, 1);  /* 125MHz, SYSTEM_PLL2_DIV8 */
        }
    }

    /* enable the clock gate */
    REG_CCM_CCGR(CCGR_ENET1).CCGR_SET = 3;
    _kernel_synch_cache();

    return E_OK;
}
#endif /* CPU_IMX_8M_QUADMAX */

#if (0 != ENET_RAM_USE_NETBUF)
static ER enet_rx_packet_init(T_ETH_CTL* eth)
{
    T_NET_BUF *pkt;
    ER ercd;
    INT i;
#if DEBUG_ETH == 1
    /* DEBUG Note: check descriptor */
    UW pkt_buf[ENET_RXDESC_CNT];    /* descriptor pkt-->buf */
    UW pkt_ali[ENET_RXDESC_CNT];    /* descriptor buffer alignment (all same value) */
    UW pkt_dif[ENET_RXDESC_CNT];    /* offset from pkt->buf to descriptor buffer  */
    T_NET_DEV* dev = &gNET_DEV[eth->dev_num - 1];
    UB calc_hhdrofs;
#endif    

    for (i = 0; i < ENET_RXDESC_CNT; i++) {
        ercd = net_buf_get(&pkt, ENET_BUFSZ, TMO_POL);
        if (E_OK != ercd) {
            break;
        }
        rxPkt[i] = pkt;
        pkt->buf[0] = (UB)i;    /* pkt->buf[0] == rxPkt's index */
#if DEBUG_ETH == 1
        pkt_buf[i] = (UW)(ADDR)pkt->buf;
        pkt_ali[i] = (UW)ALIGN(pkt_buf[i], ENET_ALIGN_SZ);
        pkt_dif[i] = pkt_ali[i] - pkt_buf[i];
#endif
        DCACHE_CLEAR(pkt, ENET_BUFSZ);
    }
    
#if DEBUG_ETH == 1
    /* check network buffer alignment, and dev->hhdrofs settings */
    if (E_OK == ercd) {
        for (i = 1; i < ENET_RXDESC_CNT; i++) {
            if (pkt_dif[0] != pkt_dif[i]) {
                ercd = -1;   /* abnormal alignment */
                break;
            }
        }    
        if (i == ENET_RXDESC_CNT) {
            /* Check that dev->hhdrofs value is correct. */
            calc_hhdrofs = pkt_dif[0] + ENET_SHIFT16_AJUST;
            ercd = (calc_hhdrofs == dev->hhdrofs) ? E_OK : -2;
        }
        tslp_tsk(0);
    }
#endif

    return ercd;
}
#endif /* #if (0 != ENET_RAM_USE_NETBUF) */

/*************************************************************************
 * Function  : _ddr_imx8_enet_ini
 * Purpose   : Initializes the hardware device.
 * Arguments : dev_num = Network interface control block id.
 * Return    : E_OK    = Initialization successful
 *             E_OBJ   = Device already initialized.
 *             E_PAR   = MAC Address error.
 *             E_CLS   = Initialization process cancelled.
 *             E_ID    = Kernel resource creation error.
 *                       dev_num is invalid.
 * Comments  : Network interface provides the mac address of the interface.
 *             call from uNET3 by gNET_DEV[].ini
 ************************************************************************/

ER _ddr_imx8_enet_ini(UH dev_num)
{
    UH rtncd;
    ER ercd;
    UB* mac;
    T_NET* net;
    T_NET_DEV* dev;
    BDBUF buf_p;
    T_ETH_BD* bd_p;
    UW ipgclk;
    UW hold;
    T_ETH_CTL* eth = NULL;
    volatile struct t_enet* port;
    
    if ((dev_num == 0U) || (dev_num > ENET_DEV_MAX)) {
        return E_ID;
    }

    eth = &g_eth_ctl;
    if ((eth->bit & BIT_INIT_FLAG)) {
        return E_OBJ; /* Error: Device Already initialized */
    }

    net = &gNET[dev_num - 1];
    dev = net->dev;
    if (dev == NULL) {
        return E_PAR;
    }
    mac = &dev->cfg.eth.mac[0];

    rtncd = eth->phymod_ext;
    net_memset((void*)eth, 0,
               sizeof(T_ETH_CTL)); /* Clear control/status structure */
    eth->dev_num = dev_num;
    if (eth->bit & BIT_INIT_FLAG) {
        return E_OBJ; /* Error: Already initialized */
    }

#ifndef NET_C_OS
    if (g_phy_access_sem <= 0) {
        g_phy_access_sem = acre_sem((T_CSEM *)&c_eth_sem);
        if (g_phy_access_sem <= 0) {
            return E_ID;
        }
    }

#endif
    
/* Task for reception control */
#ifndef NET_C_OS
    c_dev_ctl_tsk.exinf = eth;
    eth->ctl_tsk_id = acre_tsk((T_CTSK*)&c_dev_ctl_tsk);
    if (eth->ctl_tsk_id <= 0)
        return E_ID; /* Error: Ctl Task creation failed */

    c_dev_snd_tsk.exinf = eth;
    eth->snd_tsk_id = acre_tsk((T_CTSK*)&c_dev_snd_tsk);
    if (eth->snd_tsk_id <= 0)
        return E_ID; /* Error: Rx Task creation failed */

    c_dev_rcv_tsk.exinf = eth;
    eth->rcv_tsk_id = acre_tsk((T_CTSK*)&c_dev_rcv_tsk);
    if (eth->rcv_tsk_id <= 0)
        return E_ID; /* Error: Rx Task creation failed */
#ifdef ETHER_TEST_TOOL_MODE
g_ethertesttool_ID_ETH_RCV_TSK = eth->rcv_tsk_id;
g_ethertesttool_ID_ETH_CTL_TSK = eth->ctl_tsk_id;
#endif
#endif

/* Send and Receive Queue mailbox */
#ifndef NET_C_OS
    /* Create Send MailBox */
    eth->snd_mbx_id = acre_mbx((T_CMBX*)&c_eth_snd_mbx);
    if (eth->snd_mbx_id <= 0)
        return E_ID; /* Error: Send MBX creation failed */

    /* Create Receive MailBox */
    eth->rcv_mbx_id = acre_mbx((T_CMBX*)&c_eth_rcv_mbx);
    if (eth->rcv_mbx_id <= 0)
        return E_ID; /* Error: Recv MBX creation failed */
#endif

/* create event flag for sync with interrupt operation */
#ifndef NET_C_OS
    eth->evt_flg_id = acre_flg((T_CFLG*)&c_eth_flg);
    if (eth->evt_flg_id <= 0)
        return E_ID; /* Error: Event flag creation failed */
#endif

    eth->cfg = &g_eth_cfg;

#ifndef NET_C_OS
    cdev_imx_eth_isr.exinf = eth;
    cdev_imx_eth_isr.intno = g_enet_intno;
    cdev_imx_eth_isr.isratr = TA_HLNG | TA_FPU;
#ifdef CPU_IMX_8M_QUADMAX
    eth->isr_id = ddr_imx8_irq_str_acre_isr((T_IMX8_IRQ_STR_CISR*)&cdev_imx_eth_isr);
#else
    cdev_imx_eth_isr.imask = eth->cfg->imask;
    eth->isr_id = acre_isr((T_CISR*)&cdev_imx_eth_isr);
#endif
    if (eth->isr_id <= E_OK) {
        return E_ID;
    }
#endif
    
    ercd = enet_clk_ini(0);    /* Enable ENET Peripheral Clock */
    if (E_OK != ercd) {
        return ercd;
    }

    if (eth->cfg->csum_mode & 1U) {
        net->flag |= (HW_CS_TX_IPH4 | HW_CS_TX_DATA);
    }

    eth->intno = g_enet_intno;
    eth->phymod_ext = rtncd;
    port = eth->port =
        (volatile struct t_enet*)g_enet_regs;
    port->ECR = ENET_ECR_RESET; /* Reset MAC */
    dly_tsk(1); /* wait 8 system clock for reset sequence progress */
#if ENETDMA_BIGENDIAN
    port->ECR &= ~ENET_ECR_DBSWP; /* Discriptor Swap */
#else
    port->ECR |= ENET_ECR_DBSWP; /* Discriptor Swap */
#endif

    ipgclk = ENET_MDC_CLK;
    ipgclk = ipgclk / 5000000U;
    hold = (ipgclk / 100000000U) - 1U;
    port->MSCR = ((ipgclk << 1U) & 0x7eU) | ((hold << 8U) & 0x0300U);

#if (0 == ENET_RAM_USE_NETBUF)
    buf_p = PTR2BDBUF(ALIGN(eth->cfg->rxbuf_ptr, ENET_ALIGN_SZ));
#else
    ercd = enet_rx_packet_init(eth);
    if (E_OK != ercd) {
        return E_OBJ;
    }
    buf_p = PTR2BDBUF(rxPkt[0]->buf + dev->hhdrofs);
#endif
    bd_p = (T_ETH_BD*)ALIGN(eth->cfg->rxdesc_ptr, ENET_ALIGN_SZ);
    bd_p->buf_p = swap32(buf_p);
    eth->rbd_top = bd_p;
    
    buf_p = PTR2BDBUF((VP)ALIGN(eth->cfg->txbuf_ptr, ENET_ALIGN_SZ));
    bd_p = (T_ETH_BD*)ALIGN(eth->cfg->txdesc_ptr, ENET_ALIGN_SZ);
    bd_p->buf_p = swap32(buf_p);
    eth->tbd_p = eth->tbd_top = bd_p;

    port->EIR = DEF_EIRCLR; /* clear event */
    port->EIMR = DEF_INT;

    if (((mac[0] | mac[1] | mac[2] | mac[3] | mac[4] | mac[5]) == 0x00U) ||
        ((mac[0] & mac[1] & mac[2] & mac[3] & mac[4] & mac[5]) == 0xFFU)) {
        return E_PAR;
    }

    port->PALR = ((UW)mac[0] << 24) | ((UW)mac[1] << 16) | ((UW)mac[2] << 8) |
                 ((UW)mac[3]);
    port->PAUR = ((UW)mac[4] << 24) | ((UW)mac[5] << 16);

    if (eth->cfg->filter_mode == 0U) {
        port->GALR = 0U; /* multicast setting (all disable)  */
        port->GAUR = 0U;
    } else {
        port->GALR = 0xFFFFFFFFU; /* multicast setting (all enable)  */
        port->GAUR = 0xFFFFFFFFU;
    }

    port->IALR = 0U;
    port->IAUR = 0U;

    port->TFWR |= ENET_TFWR_STRFWD;

    if (eth->cfg->csum_mode & 1U) {
        port->TACC = (UW)(ENET_TACC_PROCHK | ENET_TACC_IPCHK);
    }
    if (eth->cfg->csum_mode & 2U) {
        port->RACC = (UW)(ENET_RACC_LINEDIS | ENET_RACC_PRODIS | ENET_TACC_IPDIS);
    }
#if (1 == ENET_RAM_USE_NETBUF)
    port->RACC |= ENET_RACC_SHIFT16;
#endif

#ifdef CPU_IMX_8M_QUADMAX
    ercd = ddr_imx8_irq_str_ena_int(eth->intno);
    if (E_OK != ercd){
        return ercd;
    }
#else
    ena_int(eth->intno);
#endif
    /* setup the PHY controller */

    ercd = phy_ini(eth);

    if (ercd != E_OK) {
#ifdef CPU_IMX_8M_QUADMAX
        ddr_imx8_irq_str_dis_int(eth->intno);
#else
        dis_int(eth->intno);
#endif
        return ercd; /* Error: PHY initialization failed */
    }

    eth->bit |= (BIT_INIT_FLAG | BIT_RX_ON | BIT_TX_ON);
    (void)act_tsk(eth->ctl_tsk_id);
    (void)act_tsk(eth->snd_tsk_id);
    (void)act_tsk(eth->rcv_tsk_id);

    return E_OK;
}

/*************************************************************************
 * Function  : _ddr_imx8_enet_cls
 * Purpose   : UnInitializes the device and its resources.
 * Arguments : none
 * Return    : E_OK    = UnInitialization successful
 * comment   : call from uNET3 by gNET_DEV[].cls
 ************************************************************************/

ER _ddr_imx8_enet_cls(UH dev_num)
{
    T_ETH_CTL* eth = NULL;
    T_NET_BUF *pkt;
    ER ercd;
    FLGPTN ptn;
#if (1 == ENET_RAM_USE_NETBUF)
    INT i;
#endif

    if ((dev_num == 0U) || (dev_num > ENET_DEV_MAX)) {
        return E_ID;
    }

    eth = &g_eth_ctl;
    if (!(eth->bit & BIT_INIT_FLAG)) {
        return E_OBJ; /* Error: Device Not initialized */
    }

    eth->bit &= ~BIT_INIT_FLAG;     /* terminate */
#ifdef CPU_IMX_8M_QUADMAX
    ercd = ddr_imx8_irq_str_dis_int(eth->intno);
    if (E_OK != ercd){
        return ercd;
    }
#else
    (void)dis_int(eth->intno);
#endif
    eth_stop(eth); /* stop sequence issue */

    /* Wait Stop Ethernet task */
    rel_wai(eth->ctl_tsk_id);
    rel_wai(eth->snd_tsk_id);
    rel_wai(eth->rcv_tsk_id);  
    ercd = twai_flg(eth->evt_flg_id, (EVENT_CTL_EXIT | EVENT_RX_EXIT | EVENT_TX_EXIT), TWF_ANDW, &ptn, 1000);    
    if (E_OK != ercd) {
        /* Kill active tasks. */
        ptn = 0;
        ercd = pol_flg(eth->evt_flg_id, (EVENT_CTL_EXIT | EVENT_RX_EXIT | EVENT_TX_EXIT), TWF_ORW, &ptn);    
        if (0 == (ptn & EVENT_CTL_EXIT))   (void)ter_tsk(eth->ctl_tsk_id);
        if (0 == (ptn & EVENT_RX_EXIT))    (void)ter_tsk(eth->snd_tsk_id);
        if (0 == (ptn & EVENT_TX_EXIT))    (void)ter_tsk(eth->rcv_tsk_id);
    }
#if (1 == ENET_RAM_USE_NETBUF)
    /* Release network buffer using for rx DMA */
    for (i = 0; i < sizeof(rxPkt)/sizeof(*rxPkt); i++) {
        pkt = rxPkt[i];
        if (pkt) {
            net_buf_ret(pkt);
        }
    }
#endif
    /* Flush tx queue and revoke packets */
    ercd = E_OK;
    while (ercd == E_OK) {
        ercd = prcv_mbx(eth->snd_mbx_id, (T_MSG **)&pkt);
        if (ercd == E_OK) {
            net_buf_ret(pkt);
        }
    }

#ifndef NET_C_OS
    dis_dsp();
    del_tsk(eth->ctl_tsk_id);
    del_tsk(eth->snd_tsk_id);
    del_tsk(eth->rcv_tsk_id);
    del_mbx(eth->snd_mbx_id);
    del_mbx(eth->rcv_mbx_id);
    del_flg(eth->evt_flg_id);
#ifdef CPU_IMX_8M_QUADMAX
    ddr_imx8_irq_str_del_isr(eth->isr_id);
#else
    del_isr(eth->isr_id);
#endif
    ena_dsp();
#endif

    eth->bit &= ~(BIT_INIT_FLAG | BIT_RX_ON | BIT_TX_ON);
    return E_OK;
}

/*************************************************************************
 * Function  : _ddr_imx8_enet_snd
 * Purpose   :
 * Arguments :
 * Return    : E_WBLK
 * comment   : call from uNET3 by gNET_DEV[].out
 ************************************************************************/

ER _ddr_imx8_enet_snd(UH dev_num, T_NET_BUF* pkt)
{
    T_ETH_CTL* eth = NULL;

    if ((dev_num == 0U) || (dev_num > ENET_DEV_MAX)) {
        return E_ID;
    }
    eth = &g_eth_ctl;
    if (!(eth->bit & BIT_INIT_FLAG)) {
        return E_OBJ; /* Error: Device Not initialized */
    }    

    /* Add to Device send queue */
    (void)snd_mbx(eth->snd_mbx_id, (T_MSG*)pkt);
    return E_WBLK;
}

/*************************************************************************
 * Function  : _ddr_imx8_enet_cfg
 * Purpose   :
 * Arguments :
 * Return    :
 * comment   : call from uNET3 by gNET_DEV[].ctl
 ************************************************************************/

ER _ddr_imx8_enet_cfg(UH dev_num, UH opt, VP val)
{
    ER rtncd = E_OK;
    INT tmp;
    T_ETH_CTL* eth = NULL;

    if ((dev_num == 0U) || (dev_num > ENET_DEV_MAX)) {
        return E_ID;
    }

    eth = &g_eth_ctl;

    /* Configuration items. (Initialization not required.) */
    switch (opt) {
    case CFG_PHY_MODE:
        tmp = cnv_phymod(1U, (INT)(ADDR)val);
        if (0 > tmp) {
            rtncd = E_PAR; /* invalid value */
        } else {
            eth->phymod_ext = (UH)tmp | PME_CFG_NSET;
        }
        break;

    case CFG_PHY_MODE_EXT:
        tmp = (INT)(ADDR)val;

        if (0U != (~EMAC_SUP_PHY_MODE & (UH)tmp)) {
            rtncd = E_PAR; /* invalid value */
        } else if (0U == (PME_SUP_ANE & (UH)tmp)) {
            tmp = (INT)(((UH)tmp & PME_SUP_MASK) & ~PME_SUP_FD);
            if (!((PME_SUP_10M == (UH)tmp) || (PME_SUP_100M == (UH)tmp) ||
                  (PME_SUP_1000M == (UH)tmp))) {
                rtncd = E_PAR;
            }
        } else {
            /* Do Nothing */
        }
        if (E_OK == rtncd) {
            eth->phymod_ext = (UH)(ADDR)val | PME_CFG_NSET;
        }
        break;

    default:
        rtncd = E_NOSPT;
        break;
    }
    if (E_NOSPT != rtncd)
        return rtncd;

    /* Configuration items. (Initialization required.) */
    if (!(eth->bit & BIT_INIT_FLAG)) {
        return E_OBJ; /* Error: Device Not initialized */
    }

    switch (opt) {
    case 0:
    default:
        rtncd = E_NOSPT;
        break;
    }

    return rtncd;
}

/*************************************************************************
 * Function  : _ddr_imx8_enet_ref
 * Purpose   :
 * Arguments :
 * Return    :
 * comment   : call from uNET3 by gNET_DEV[].ref
 ************************************************************************/

ER _ddr_imx8_enet_ref(UH dev_num, UH opt, VP val)
{
    ER rtncd = E_OK;
    T_ETH_CTL* eth = NULL;
    if ((dev_num == 0U) || (dev_num > ENET_DEV_MAX)) {
        return E_ID;
    }

    eth = &g_eth_ctl;

    /* Reference items. (Initialization not required.) */
    if (val == NULL) {
        return E_PAR;
    }

    switch (opt) {
    case CFG_PHY_MODE:
    case CFG_PHY_MODE_EXT:
        if (0U == eth->phymod_ext) { /* invalidate PHY mode settings */
            eth->phymod_ext = get_def_phymod(eth, 1U);
        }
        *(UH*)val = (opt == CFG_PHY_MODE) ? (UH)cnv_phymod(0U, (INT)eth->phymod_ext)
                                          : eth->phymod_ext;
        break;

    default:
        rtncd = E_NOSPT;
        break;
    }

    if (E_NOSPT != rtncd)
        return rtncd;

    /* Reference items. (Initialization required.) */
    if (!(eth->bit & BIT_INIT_FLAG)) {
        return E_OBJ; /* Error: Device Not initialized */
    }

    switch (opt) {
#if (SNMP_ENA == 1)
    case REF_ETH_UPD_STS: {
        volatile struct t_enet* port = eth->port;
        UH tmp;
        T_NET_STS_IFS* ifs;
        /* Update status [in_octet..out_qlen] */
        ifs = (T_NET_STS_IFS*)val;
        /* Status */
        loc_cpu();
        ifs->oper_sts = eth->physts ? NET_STS_LINK_UP : NET_STS_LINK_DOWN;
        /* Rx */
        ifs->in_octet = port->RMON_R_OCTETS;
        tmp = (UH)port->RMON_R_PACKETS;
        if (eth->r_packet > (UH)tmp)
            ifs->in_ucast_pkt += (0x00010000U - (UW)eth->r_packet) + (UW)tmp;
        else
            ifs->in_ucast_pkt += (UW)tmp - (UW)eth->r_packet;
        eth->r_packet = tmp;

        tmp = (UH)port->RMON_R_BC_PKT;
        if (eth->r_bc_packet > (UH)tmp)
            ifs->in_nucast_pkt +=
                (0x00010000U - (UW)eth->r_bc_packet) + (UW)tmp;
        else
            ifs->in_nucast_pkt += (UW)tmp - (UW)eth->r_bc_packet;
        eth->r_bc_packet = tmp;

        tmp = (UH)port->RMON_R_MC_PKT;
        if (eth->r_mc_packet > (UH)tmp)
            ifs->in_nucast_pkt +=
                (0x00010000U - (UW)eth->r_mc_packet) + (UW)tmp;
        else
            ifs->in_nucast_pkt += (UW)tmp - (UW)eth->r_mc_packet;
        eth->r_mc_packet = tmp;

        /* Undersize, Oversize, Jabber */
        tmp = (UH)port->RMON_R_UNDERSIZE;
        if (eth->r_undersize > (UH)tmp)
            ifs->in_discard += (0x00010000U - (UW)eth->r_undersize) + (UW)tmp;
        else
            ifs->in_discard += (UW)tmp - (UW)eth->r_undersize;
        eth->r_undersize = tmp;

        tmp = (UH)port->RMON_R_OVERSIZE;
        if (eth->r_oversize > (UH)tmp)
            ifs->in_discard += (0x00010000U - (UW)eth->r_oversize) + (UW)tmp;
        else
            ifs->in_discard += (UW)tmp - (UW)eth->r_oversize;
        eth->r_oversize = tmp;

        tmp = (UH)port->RMON_R_JAB;
        if (eth->r_jab > (UH)tmp)
            ifs->in_discard += (0x00010000U - (UW)eth->r_jab) + (UW)tmp;
        else
            ifs->in_discard += (UW)tmp - (UW)eth->r_jab;
        eth->r_jab = tmp;

        tmp = (UH)port->RMON_R_DROP;
        if (eth->r_drop > (UH)tmp)
            ifs->in_discard += (0x00010000U - (UW)eth->r_drop) + (UW)tmp;
        else
            ifs->in_discard += (UW)tmp - (UW)eth->r_drop;
        eth->r_drop = tmp;

        /* CRC, Alignment, Runt, Length error, OutofRangetype, Watchdog,*
         * GMII/MII
         */
        tmp = (UH)port->RMON_R_FRAG;
        if (eth->r_frag > (UH)tmp)
            ifs->in_err += (0x00010000U - (UW)eth->r_frag) + (UW)tmp;
        else
            ifs->in_err += (UW)tmp - (UW)eth->r_frag;
        eth->r_frag = tmp;

        tmp = (UH)port->RMON_R_CRC_ALIGN;
        if (eth->r_crc_align > (UH)tmp)
            ifs->in_err += (0x00010000U - (UW)eth->r_crc_align) + (UW)tmp;
        else
            ifs->in_err += (UW)tmp - (UW)eth->r_crc_align;
        eth->r_crc_align = tmp;

        /* Tx */
        ifs->out_octet = port->RMON_T_OCTETS;

        tmp = (UH)port->RMON_T_PACKETS;
        if (eth->t_packet > (UH)tmp)
            ifs->out_ucast_pkt += (0x00010000U - (UW)eth->t_packet) + (UW)tmp;
        else
            ifs->out_ucast_pkt += (UW)tmp - (UW)eth->t_packet;
        eth->t_packet = tmp;

        tmp = (UH)port->RMON_T_BC_PKT;
        if (eth->t_bc_packet > (UH)tmp)
            ifs->out_nucast_pkt +=
                (0x00010000U - (UW)eth->t_bc_packet) + (UW)tmp;
        else
            ifs->out_nucast_pkt += (UW)tmp - (UW)eth->t_bc_packet;
        eth->t_bc_packet = tmp;

        tmp = (UH)port->RMON_T_MC_PKT;
        if (eth->t_mc_packet > (UH)tmp)
            ifs->out_nucast_pkt +=
                (0x00010000U - (UW)eth->t_mc_packet) + (UW)tmp;
        else
            ifs->out_nucast_pkt += (UW)tmp - (UW)eth->t_mc_packet;
        eth->t_mc_packet = tmp;

        /* Undersize, Oversize, Jabber */
        tmp = (UH)port->RMON_T_UNDERSIZE;
        if (eth->t_undersize > (UH)tmp)
            ifs->out_discard += (0x00010000U - (UW)eth->t_undersize) + (UW)tmp;
        else
            ifs->out_discard += (UW)tmp - (UW)eth->t_undersize;
        eth->t_undersize = tmp;

        tmp = (UH)port->RMON_T_OVERSIZE;
        if (eth->t_oversize > (UH)tmp)
            ifs->out_discard += (0x00010000U - (UW)eth->t_oversize) + (UW)tmp;
        else
            ifs->out_discard += (UW)tmp - (UW)eth->t_oversize;
        eth->t_oversize = tmp;

        tmp = (UH)port->RMON_T_JAB;
        if (eth->t_jab > (UH)tmp)
            ifs->out_discard += (0x00010000U - (UW)eth->t_jab) + (UW)tmp;
        else
            ifs->out_discard += (UW)tmp - (UW)eth->t_jab;
        eth->t_jab = tmp;

        tmp = (UH)port->RMON_T_DROP;
        if (eth->t_drop > (UH)tmp)
            ifs->out_discard += (0x00010000U - (UW)eth->t_drop) + (UW)tmp;
        else
            ifs->out_discard += (UW)tmp - (UW)eth->t_drop;
        eth->t_drop = tmp;

        /* CRC, Alignment, Runt, Length error, OutofRangetype, Watchdog,*
         * GMII/MII
         */
        tmp = (UH)port->RMON_T_FRAG;
        if (eth->t_frag > (UH)tmp)
            ifs->out_err += (0x00010000U - (UW)eth->t_frag) + (UW)tmp;
        else
            ifs->out_err += (UW)tmp - (UW)eth->t_frag;
        eth->t_frag = tmp;

        tmp = (UH)port->RMON_T_CRC_ALIGN;
        if (eth->t_crc_align > (UH)tmp)
            ifs->out_err += (0x00010000U - (UW)eth->t_crc_align) + (UW)tmp;
        else
            ifs->out_err += (UW)tmp - (UW)eth->t_crc_align;
        eth->t_crc_align = tmp;
        unl_cpu();

        rtncd = E_OK;
    } break;
#endif
    case REF_ETH_LINK_STS:
        loc_cpu();
        *(UH*)val = eth->physts;
        unl_cpu();
        rtncd = E_OK;
        break;

    default:
        rtncd = E_NOSPT;
        break;
    }

    return rtncd;
}

#ifdef ETHER_TEST_TOOL_MODE
#define net_pkt_rcv     net_pkt_rcv_DEBUG
extern void (*net_pkt_rcv_DEBUG)(T_NET_BUF *pkt);
#endif
/**
 *    Receive Task
 *
 *
 */

void _ddr_imx8_enet_rcv_tsk(T_ETH_CTL* eth)
{
    T_NET_BUF* pkt;
    ER ercd;

    (void)clr_flg(eth->evt_flg_id, ~EVENT_RX_EXIT);

    while (0 != (eth->bit & BIT_INIT_FLAG)) {
        ercd = rcv_mbx(eth->rcv_mbx_id, (T_MSG**)&pkt);

        if (ercd != E_OK) {
            break;
        }
        net_pkt_rcv(pkt);
    }

    (void)set_flg(eth->evt_flg_id, EVENT_RX_EXIT);
}

/**
 *    Send Task
 *
 *
 *    use task to queue the packet if dev out is need to be blocked
 *    if transmission process requires huge task stack size.
 */
void _ddr_imx8_enet_snd_tsk(T_ETH_CTL* eth)
{
    T_NET_BUF* pkt;
    ER ercd;

    (void)clr_flg(eth->evt_flg_id, ~EVENT_TX_EXIT);

    while (0 != (eth->bit & BIT_INIT_FLAG)) {

        ercd = rcv_mbx(eth->snd_mbx_id, (T_MSG**)&pkt);
        if (ercd != E_OK) {
            break;
        }

        ercd = eth_tx(eth, pkt);

        if (ercd != E_OK) {
            pkt->ercd = ercd;
        }

        loc_tcp();
        net_buf_ret(pkt);
        ulc_tcp();
    }

    (void)set_flg(eth->evt_flg_id, EVENT_TX_EXIT);
}

