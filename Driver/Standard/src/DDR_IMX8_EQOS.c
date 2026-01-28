/***********************************************************************
    MICRO C CUBE / COMPACT, DEVICE DRIVER
    Ethernet driver for i.MX8Plus ENET_QOS(EQOS) for uC3/Standard Series
    Copyright (c) 2021-2025, eForce Co., Ltd. All rights reserved.

    2021/02/17: Created.
    2021/03/02: SNMP was supported. 
    2021/05/12: Added call impossible conditions of the driver function.
    2025/10/21: 64bit CPU(ARM AArch64) support
                Fixed an issue where initial communication failed.
 ***********************************************************************/

#include "kernel.h"
#include "imx8mplus_uC3.h"
#include "DDR_IMX8_EQOS_cfg.h"
#include "DDR_IMX8_EQOS.h"
#include "DDR_PHY.h"
#include "net_hdr.h"

#if defined(NET3_VER) && ((NET3_VER) >= 3)
#include "net_def.h"
#endif

#ifdef NET_C_OS
#include "kernel_id.h"
#endif

#if ((TKERNEL_PRID & 0x00FF) == 0x4C)   /* ARM AArch64 */
#include "DDR_Aarch64_MMU.h"
#define DCACHE_CLEAR(x,y)       (_ddr_aarch64_mmu_clean_data_cache((void *)x, (SIZE)y))
#define DCACHE_INVALID(x,y)     (_ddr_aarch64_mmu_invalid_data_cache((void *)x, (SIZE)y))
#define DCACHE_FLUSH(x,y)       (_ddr_aarch64_mmu_flush_data_cache((void *)x, (SIZE)y))
#else
#define DCACHE_CLEAR(x,y)       (_kernel_clean_data_cache((void *)x, (SIZE)y))
#define DCACHE_INVALID(x,y)     (_kernel_invalid_data_cache((void *)x, (SIZE)y))
#define DCACHE_FLUSH(x,y)       (_kernel_flush_data_cache((void *)x, (SIZE)y))
#endif

/*
    Configuration Macros
*/
#ifndef EQOS_PHY_ADDR
#define EQOS_PHY_ADDR        0
#endif
#if ((0 > EQOS_PHY_ADDR) || (EQOS_PHY_ADDR > 31))
  #error "The value of EQOS_PHY_ADDR is invalid."
#endif

#ifndef EQOS_MDC_CLK
#define EQOS_MDC_CLK         5      /* 266MHz */
#endif
#if ((0 > EQOS_MDC_CLK) || (EQOS_MDC_CLK > 7))
  #error "The value of EQOS_MDC_CLK is invalid."
#endif

#ifndef EQOS_RMII_MODE
#define EQOS_RMII_MODE       2      /* RGMII */
#endif
#if (2 == EQOS_RMII_MODE)
  #define EQOS_PHY_GIGA         1
#else
  #define EQOS_PHY_GIGA         0
  #error "The value of EQOS_RMII_MODE is invalid."
#endif

#ifndef EQOS_PHY_MODE
#define EQOS_PHY_MODE        0
#endif
#if !((EQOS_PHY_MODE == 0) || (EQOS_PHY_MODE == 1) || (EQOS_PHY_MODE == 2) || (EQOS_PHY_MODE == 3) \
    || (EQOS_PHY_MODE == 4) || (EQOS_PHY_MODE == 7) || (EQOS_PHY_MODE == 8))
  #error "The value of EQOS_PHY_MODE is invalid."
#endif

#ifndef EQOS_FILTER_MODE
#define EQOS_FILTER_MODE     0
#endif
#if ((0 > EQOS_FILTER_MODE) || (EQOS_FILTER_MODE > 2))
  #error "The value of EQOS_FILTER_MODE is invalid."
#endif

#ifndef EQOS_CSUM_MODE
#define EQOS_CSUM_MODE       0
#endif
#if ((0 > EQOS_CSUM_MODE) || (EQOS_CSUM_MODE > 3))
  #error "The value of EQOS_CSUM_MODE is invalid."
#endif

#ifndef EQOS_RXDESC_CNT
#define EQOS_RXDESC_CNT      4
#endif
#if ((4 > EQOS_RXDESC_CNT) || (EQOS_RXDESC_CNT > 255))
  #error "The value of EQOS_RXDESC_CNT must be 4 or more."
#endif

#ifndef EQOS_TXDESC_CNT
#define EQOS_TXDESC_CNT      4
#endif
#if ((4 > EQOS_TXDESC_CNT) || (EQOS_TXDESC_CNT > 255))
  #error "The value of EQOS_TXDESC_CNT must be 4 or more."
#endif

#ifndef EQOS_TX_TMO
#define EQOS_TX_TMO         10
#endif

#ifndef EQOS_RX_TMO
#define EQOS_RX_TMO         500
#endif

#ifndef PHY_LINK_INTVAL
#define PHY_LINK_INTVAL     10
#endif

#ifndef PHY_RST_WAIT_TMO
#define PHY_RST_WAIT_TMO    1000
#endif
#if (10 > PHY_RST_WAIT_TMO)
  #error "The value of PHY_RST_WAIT_TMO must be 10 or more."
#endif

#ifndef EQOS_PHY_ISOL_WAIT
#define EQOS_PHY_ISOL_WAIT      100
#endif

#ifndef EQOS_PHY_PUT_WAIT
#define EQOS_PHY_PUT_WAIT       1
#endif

#ifndef SNMP_ENA
#define SNMP_ENA                0
#endif

#ifndef EQOS_DBG_ENA
#define EQOS_DBG_ENA            0
#endif

#ifndef EQOS_RAM_USE_NETBUF         /* descriptor buffer is network buffer? (1:yes, 0:no) */
#define EQOS_RAM_USE_NETBUF     1
#endif

#define EQOS_DEV_MAX            1   /* currently eqos is only 1ch, so always set it to 1. */
#define EQOS_USE_CTL_TSK        0   /* currently ctl_tsk is not needed, so always set it to 0. */
#define EQOS_ALIGN_SZ           64


#if (SNMP_ENA == 1)
#include "net_sts.h"
#include "net_sts_id.h"
#endif

/*
    Definitions
*/
#define INT_TX_BITS         (DMACIER_ETIE)
#define INT_RX_BITS         (DMACIER_ERIE)
#define INT_TX_ERR_BITS     (DMACIER_CDEE)
#define INT_RX_ERR_BITS     (DMACIER_RWTE)
#define INT_ERR_BITS        (INT_TX_ERR_BITS | INT_RX_ERR_BITS | DMACIER_FBEE)

#define EV_EQOS_TX           0x0001
#define EV_EQOS_TX_ERR       0x0002
#define EV_EQOS_TX_RDY       0x0004

#define EV_EQOS_RX           0x0001
#define EV_EQOS_RX_ERR       0x0002
#define EV_EQOS_LNK          0x0004
#define EV_EQOS_LNK_ERR      0x0008

#define EV_EQOS_CTL_EXIT     0x1000
#define EV_EQOS_RX_EXIT      0x2000
#define EV_EQOS_TX_EXIT      0x4000

#if (_kernel_SIZE_SIZE==8)          /* for 64bit CPU */
#define ETH_TSK_STK_MUL         3
#define EQOS_RUN_64BIT          1
#else
#warning "(_kernel_SIZE_SIZE==4)"   /* for 32bit CPU */
#define ETH_TSK_STK_MUL         1
#define EQOS_RUN_64BIT          0
#endif

#ifndef NET_C_OS
/** for OS resource */
#ifndef EQOS_PRI_SND_TSK
#define EQOS_PRI_SND_TSK     (4U)    /**< Send task priority */
#endif
#ifndef EQOS_PRI_RCV_TSK
#define EQOS_PRI_RCV_TSK     (4U)    /**< Receive task priority */
#endif
#ifndef EQOS_PRI_CTL_TSK
#define EQOS_PRI_CTL_TSK     (4U)    /**< Control task priority */
#endif
#ifndef EQOS_STK_SND_TSK
#define EQOS_STK_SND_TSK     (768U * ETH_TSK_STK_MUL)  /**< Send task stack size */
#endif
#ifndef EQOS_STK_RCV_TSK
#define EQOS_STK_RCV_TSK     (768U * ETH_TSK_STK_MUL) /**< Receive task stack size */
#endif
#ifndef EQOS_STK_CTL_TSK
#define EQOS_STK_CTL_TSK     (768U * ETH_TSK_STK_MUL)  /**< Control task stack size */
#endif
#endif

#ifndef EQOS_IPL
#define EQOS_IPL             (240U)  /* Interrupt priority level */
#endif

/* Buffer Descriptor structure */
typedef UW  BDBUF;        
#define PTR2BDBUF(x)    ((UW)(ADDR)(x))
#define BDBUF2PTR(x)    ((VP)(ADDR)(x))

/* DMA Descriptor Chain */
typedef struct t_eqosdesc {
    UW  DESC[4];
    T_NET_BUF  *pkt;    /* Network buffer associated with this desc */
#if (0 == EQOS_RUN_64BIT)
    UW  idx;            /* To adjust the descriptor size, this is disabled in 64-bit operation. */
#endif    
} T_EQOS_DESC;

#ifdef EQOS_RAM_SECTION
volatile T_EQOS_DESC EthRxDesc[EQOS_RXDESC_CNT] __attribute__((section(EQOS_RAM_SECTION)));
volatile T_EQOS_DESC EthTxDesc[EQOS_TXDESC_CNT] __attribute__((section(EQOS_RAM_SECTION)));
#else
volatile T_EQOS_DESC EthRxDesc[EQOS_RXDESC_CNT];
volatile T_EQOS_DESC EthTxDesc[EQOS_TXDESC_CNT];
#endif

#if (0 == EQOS_RAM_USE_NETBUF)
typedef struct t_eqosdesc_buf {
    UW  buf[1600 / sizeof(UW)];
} T_EQOS_DESC_BUF;

#ifdef EQOS_RAM_SECTION
volatile T_EQOS_DESC_BUF EthRxDesc_buf[EQOS_RXDESC_CNT] __attribute__((aligned(EQOS_ALIGN_SZ))) __attribute__((section(EQOS_RAM_SECTION)));
volatile T_EQOS_DESC_BUF EthTxDesc_buf[EQOS_TXDESC_CNT] __attribute__((aligned(EQOS_ALIGN_SZ))) __attribute__((section(EQOS_RAM_SECTION)));
#else
volatile T_EQOS_DESC_BUF EthRxDesc_buf[EQOS_RXDESC_CNT] __attribute__((aligned(EQOS_ALIGN_SZ)));
volatile T_EQOS_DESC_BUF EthTxDesc_buf[EQOS_TXDESC_CNT] __attribute__((aligned(EQOS_ALIGN_SZ)));
#endif
#endif


/* EMAC configuration (from _cfg.h) */
typedef struct t_emac_cfg {
    struct {
        UW  phy_addr    : 5;
        UW  phy_mode    : 5;
        UW  phy_giga    : 1;
        UW  eth_filter  : 2;
        UW  eth_csum    : 2;
        UW  ipl_eth     : 8;
        UW  emac_no     : 2;
        UW  jumbo_frame : 1;    /* Not supported by this driver */
    } dat;
    UH  buf_len ;        /* Length of Tx/Rx buffer */
} T_EMAC_CFG ;

/* EMAC using kernel object */
typedef struct t_emac_obj {
    ID snd_tsk;
    ID rcv_tsk;
    ID ctl_tsk;
    ID snd_flg;
    ID rcv_flg;
    ID snd_mbx;
    ID isr;
    INTNO isr_no;
} T_EMAC_OBJ ;


#if EQOS_DBG_ENA     /* Ethernet Debug Info */
typedef struct t_eth_debug {
    UH send;
    UH send_err;
    UH recv;
    UH recv_err;
    struct {
        UB ES;
        UB FD_LD;
        UB DAF;
        UB length_err;
    } drop;
    
    struct {
        UH dma;
        UB mac;
        UB mtl;
    } intr;
    UH dma_tx;
    UH dma_tx_err;
    UH dma_rx;
    UH dma_rx_err;
    
} T_EQOS_DEBUG;

#define DBG_INC(_eval, _pmc, _cnt)  {if (_eval) { ((_pmc)->dbg._cnt)++; } }   
#define GOTO_RX_DROP(_pmc, _cnt)    { ((_pmc)->dbg._cnt)++; goto rx_drop; }

#else
#define DBG_INC(_eval, _pmc, _cnt)
#define GOTO_RX_DROP(_pmc, _cnt)    { goto rx_drop; }
#endif

/* EMAC control management */
typedef struct t_emac_ctl {
    volatile struct t_enetqos *const   reg; 
    const T_EMAC_CFG *const         cfg;
#ifndef NET_C_OS
    T_EMAC_OBJ *const           obj;
#else
    const T_EMAC_OBJ *const     obj;
#endif    
    volatile T_EQOS_DESC *const   tx_desc;
    volatile T_EQOS_DESC *const   rx_desc;
    const UB            td_cnt;
    const UB            rd_cnt;
    UB                  td_idx;
    UB                  rd_idx;

    UW                  rx_unkwn;
    UW                  tx_discard;

    UINT                lan_stat;
    UH                  devnum;
    UH                  phymod_ext;

#if EQOS_DBG_ENA
    T_EQOS_DEBUG         dbg;
#endif
} T_EMAC_CTL;


static const T_EMAC_CFG emac_cfg[EQOS_DEV_MAX] = {
    {{EQOS_PHY_ADDR,         /* UW phy_addr : 5 */
      EQOS_PHY_MODE,         /* UW phy_mode : 3 */
      EQOS_PHY_GIGA,         /* UW phy_giga : 1 */
      EQOS_FILTER_MODE,      /* UW eth_filter : 2 */
      EQOS_CSUM_MODE,        /* UW eth_csum : 2 */
      EQOS_IPL,              /* UW ipl_eth  : 8 */
      0,                    /* UW emac_no  : 2 */
      0                     /* UW jumbo_frame : 1 */
     },
     1520, /* UW buf_len (PATH_MTU + 20) */
    },
};



/* Private variables ---------------------------------------------------------*/
#define IRQ_EQOS     INT_ENET_QOS_TSN

#ifndef NET_C_OS  
static T_EMAC_OBJ emac_obj[EQOS_DEV_MAX] = { 0 };
#else   
static const T_EMAC_OBJ emac_obj[EQOS_DEV_MAX] = {
  {
      ID_EQOS_SND_TSK, ID_EQOS_RCV_TSK, ID_EQOS_CTL_TSK, 
      ID_EQOS_SND_FLG, ID_EQOS_RCV_FLG, 
      ID_EQOS_SND_MBX, 
      0, IRQ_EQOS
  }
};
#endif


T_EMAC_CTL emc[EQOS_DEV_MAX] = {
  {
    &REG_EQOS,
    &emac_cfg[0], 
    &emac_obj[0], 
    EthTxDesc,
    EthRxDesc,
    EQOS_TXDESC_CNT, 
    EQOS_RXDESC_CNT, 

    0
  },
};


/**
 * OS resources
 */
#ifndef NET_C_OS
void _ddr_imx8_eqos_intr(void);
void _ddr_imx8_eqos_snd_tsk(VP_INT exinf);
void _ddr_imx8_eqos_rcv_tsk(VP_INT exinf);
static const T_CFLG ddr_eqos_snd_cflg = {TA_TFIFO | TA_WMUL, 0};
static const T_CFLG ddr_eqos_rcv_cflg = {TA_TFIFO | TA_WMUL, 0};
static const T_CMBX ddr_eqos_snd_cmbx = {TA_TFIFO | TA_MFIFO, 0, 0};
static T_CTSK ddr_eqos_snd_ctsk = {
    TA_HLNG|TA_FPU, 0, (FP)_ddr_imx8_eqos_snd_tsk, EQOS_PRI_SND_TSK, EQOS_STK_SND_TSK, NULL};
static T_CTSK ddr_eqos_rcv_ctsk = {
    TA_HLNG|TA_FPU, 0, (FP)_ddr_imx8_eqos_rcv_tsk, EQOS_PRI_RCV_TSK, EQOS_STK_RCV_TSK, NULL};
static T_CISR ddr_eqos_cisr = {
    TA_HLNG|TA_FPU, 0, IRQ_EQOS, (FP)_ddr_imx8_eqos_intr, EQOS_IPL};

#if EQOS_USE_CTL_TSK
void _ddr_imx8_eqos_ctl_tsk(VP_INT exinf);
static T_CTSK ddr_eqos_ctl_ctsk = {
    TA_HLNG|TA_FPU, 0, (FP)_ddr_imx8_eqos_ctl_tsk, EQOS_PRI_CTL_TSK, EQOS_STK_CTL_TSK, NULL};
#endif
#endif

/* Function Prototypes */
static ER phy_ini(T_EMAC_CTL *pmc);

/* Valid PHY mode in this Ethernet controller */
#define EMAC_SUP_PHY_MODE   (PME_SUP_ANE | PME_SUP_FD | PME_SUP_10M | PME_SUP_100M | PME_SUP_1000M)

/*
* convert PHY mode (Normal to Extension / Extension to Normal)
 */
static INT cnv_phymod(UB n2e, INT val)
{
    INT ret = 0;

    /* setting convert */
    if (n2e) {  /* Normal to Extension */
        switch (val) {
        case PM_AUTO:
            ret = EMAC_SUP_PHY_MODE;
            break;
        case PM_10M_HALF:
            ret = (PME_SUP_10M);
            break;
        case PM_10M_FULL:
            ret = (PME_SUP_10M | PME_SUP_FD);
            break;
        case PM_10M_AUTO:
            ret = (PME_SUP_10M | PME_SUP_FD | PME_SUP_ANE);
            break;
        case PM_100M_HALF:
            ret = (PME_SUP_100M);
            break;
        case PM_100M_FULL:
            ret = (PME_SUP_100M | PME_SUP_FD);
            break;
        case PM_100M_AUTO:
            ret = (PME_SUP_100M | PME_SUP_FD | PME_SUP_ANE);
            break;
        case PM_1000M_HALF:
            ret = (PME_SUP_1000M);
            break;
        case PM_1000M_FULL:
            ret = (PME_SUP_1000M | PME_SUP_FD);
            break;
        case PM_1000M_AUTO:
            ret = (PME_SUP_1000M | PME_SUP_FD | PME_SUP_ANE);
            break;
        default:        /* Unknown Extension settings */
            ret = -1;
            break;
        }
        /* Check specifications */
        if (0 != (~EMAC_SUP_PHY_MODE & ret)) {
            ret = -1;
        }
    }
    else {      /* Extension to Normal */
        switch (val & ~PME_CFG_NSET) {
        case EMAC_SUP_PHY_MODE:
            ret = PM_AUTO;
            break;
        case (PME_SUP_10M):
            ret = PM_10M_HALF;
            break;
        case (PME_SUP_10M | PME_SUP_FD):
            ret = PM_10M_FULL;
            break;
        case (PME_SUP_10M | PME_SUP_FD | PME_SUP_ANE):
            ret = PM_10M_AUTO;
            break;
        case (PME_SUP_100M):
            ret = PM_100M_HALF;
            break;
        case (PME_SUP_100M | PME_SUP_FD):
            ret = PM_100M_FULL;
            break;
        case (PME_SUP_100M | PME_SUP_FD | PME_SUP_ANE):
            ret = PM_100M_AUTO;
            break;
        case (PME_SUP_1000M):
            ret = PM_1000M_HALF;
            break;
        case (PME_SUP_1000M | PME_SUP_FD):
            ret = PM_1000M_FULL;
            break;
        case (PME_SUP_1000M | PME_SUP_FD | PME_SUP_ANE):
            ret = PM_1000M_AUTO;
            break;
        default:        /* Unknown Normal settings */
            ret = -1;
            break;
        }
    }

    return ret;
}

/*
* get default PHY mode (from DDR_xxx_EQOS_cfg.h - EQOS_PHY_MODE)
 */
static UH get_def_phymod(UB ext)
{
    UH ret = 0;

    if (ext) {  /* Extension setting */
        ret = (UH)cnv_phymod(1, EQOS_PHY_MODE);
    }
    else {      /* Normal setting */
        ret = EQOS_PHY_MODE;
    }

    return ret;
}


static ER EthRxBufferInit(T_EMAC_CTL *pmc)
{
    T_NET_BUF *pkt;
    ER ercd;
    INT i;
    
#if EQOS_RAM_USE_NETBUF
    ercd = E_OBJ;
#else
    ercd = E_OK;
#endif
    for (i = 0; i < pmc->rd_cnt; i++) {
        pmc->rx_desc[i].pkt = NULL;
#if EQOS_RAM_USE_NETBUF
        ercd = net_buf_get(&pkt, pmc->cfg->buf_len, TMO_POL);
        if (E_OK != ercd) {
            break;
        }
#else
        pkt = (T_NET_BUF *)&EthRxDesc_buf[i];
#endif
        pmc->rx_desc[i].pkt = pkt;
        DCACHE_INVALID(pkt, pmc->cfg->buf_len);
    }
    
    return ercd;
}

static ER EthDMARxDescInit(T_EMAC_CTL *pmc)
{
    volatile T_EQOS_DESC *rd;
    T_NET_DEV *dev;
    ER ercd;
    INT i;

    ercd = EthRxBufferInit(pmc);
    if (E_OK != ercd) {
        return ercd;
    }
    dev = &gNET_DEV[pmc->devnum - 1];

    /* Construct Rx Descriptor chain */
    for (i = 0; i < pmc->rd_cnt; i++) {
        rd = &pmc->rx_desc[i];
#if (0 == EQOS_RUN_64BIT)        
        rd->idx = i;
#endif
        rd->DESC[0] = PTR2BDBUF(&rd->pkt->buf[dev->hhdrofs]);
        rd->DESC[1] = 0;
        rd->DESC[2] = 0;
        rd->DESC[3] = RDES3R_OWN | RDES3R_BUF1V;
    }

    /* Set top descriptor address */
    pmc->reg->DMA.CH[0].RXDESC_RING_LENGTH   = (UW)(pmc->rd_cnt - 1);
    pmc->reg->DMA.CH[0].RXDESC_LIST_ADDRESS  = PTR2BDBUF(&pmc->rx_desc[0]);
    pmc->reg->DMA.CH[0].RXDESC_TAIL_POINTER  = PTR2BDBUF(&pmc->rx_desc[i - 1]);
    pmc->rd_idx = 0;
    
    return ercd;
}

static void EthDMATxDescInit(T_EMAC_CTL *pmc)
{
    int i;

    /* Construct Tx Descriptor chain */
    for (i = 0; i < pmc->td_cnt; i++) {
        net_memset((void*)&pmc->tx_desc[i], 0, sizeof(pmc->tx_desc[i]));
#if (0 == EQOS_RUN_64BIT)
        pmc->tx_desc[i].idx = i;
#endif
#if (0 == EQOS_RAM_USE_NETBUF)
        pmc->tx_desc[i].pkt = (T_NET_BUF*)&EthTxDesc_buf[i];
        pmc->tx_desc[i].pkt->hdr = &pmc->tx_desc[i].pkt->buf[2];
#endif
    }

    /* Set top descriptor address */
    pmc->reg->DMA.CH[0].TXDESC_RING_LENGTH   = (UW)(pmc->td_cnt - 1);
    pmc->reg->DMA.CH[0].TXDESC_LIST_ADDRESS  = PTR2BDBUF(&pmc->tx_desc[0]);
    pmc->reg->DMA.CH[0].TXDESC_TAIL_POINTER  = PTR2BDBUF(&pmc->tx_desc[0]);
    pmc->td_idx = 0;
}

static void phy_put(T_EMAC_CTL *pmc, UB reg, UH data)
{
    volatile UW tmp;

    loc_cpu();
    pmc->reg->MAC.MDIO_DATA = data;
    tmp = (pmc->cfg->dat.phy_addr << MACMDIOAR_PA_pos) |
            (reg << MACMDIOAR_RDA_pos)  |
            (EQOS_MDC_CLK << MACMDIOAR_CR_pos) |
            MACMDIOAR_GOC_W |
            MACMDIOAR_GB;
    pmc->reg->MAC.MDIO_ADDRESS = tmp;
    do {
        tmp = pmc->reg->MAC.MDIO_ADDRESS;
        unl_cpu();
        loc_cpu();
    } while (tmp & MACMDIOAR_GB);
    unl_cpu();
    
    tslp_tsk(EQOS_PHY_PUT_WAIT);     /* wait set value done. */
}


/**
 * read PHY register
 *
 * @param reg PHY register address
 * @return Read value
 */
static UH phy_get(T_EMAC_CTL *pmc, UB reg)
{
    volatile UW tmp;
    UH dat;

    loc_cpu();
    tmp = (pmc->cfg->dat.phy_addr << MACMDIOAR_PA_pos) |
            (reg << MACMDIOAR_RDA_pos) |
            (EQOS_MDC_CLK << MACMDIOAR_CR_pos) |
            MACMDIOAR_GOC_R |
            MACMDIOAR_GB; 
    pmc->reg->MAC.MDIO_ADDRESS = tmp;   

    do {
        tmp = pmc->reg->MAC.MDIO_ADDRESS;
        unl_cpu();
        loc_cpu();
    } while (tmp & MACMDIOAR_GB);
    dat = pmc->reg->MAC.MDIO_DATA & MACMDIODR_MD_msk;
    unl_cpu();
    
    return dat;
}

static UH phy_sts(T_EMAC_CTL *pmc)
{
    UH bmsr, physts;
    
    /* PHY mode change request */
    if (pmc->phymod_ext & PME_CFG_NSET) {
        /* Electrical isolation */
        phy_get(pmc, PHY_BMSR);         /* Dummy status read */
        phy_put(pmc, PHY_BMCR, BMCR_ISOL);
        tslp_tsk(EQOS_PHY_ISOL_WAIT);     /* wait isolation done */

        /* PHY initialize */
        phy_ini(pmc);
    }
    physts = PHY_STS_LINK_DOWN;

    /****** Check Link Status *****/
    phy_get(pmc, PHY_BMSR);  /* released latched status (Dummy read) */
    bmsr = phy_get(pmc, PHY_BMSR);
    if (0 == (bmsr & BMSR_LINK_STAT)) {
        /* Link Down */
        return physts;
    }

    if (pmc->phymod_ext & PME_SUP_ANE) {      /* use Auto-Negotiation */
        if (0 == (bmsr & BMSR_ANEG_COMP)) {
            /* Auto Negotition not yet completed */
            return physts;
        }

        /****** Know Link details *****/
        /* Note: In Basic register set, there is no bit avaialble to read the
                 Link details. So, here we use the common capabilities of remote
                 and local stations as Link details.
        */
        if (pmc->phymod_ext & PME_SUP_1000M) {
            bmsr = phy_get(pmc, PHY_1000_CTL) << 2;
            bmsr &= phy_get(pmc, PHY_1000_STS);
            if ((bmsr & GSTS_1000_FD) || (bmsr & GSTS_1000_HD)) {
                physts = PHY_STS_1000FD;        /* Gigabit halfduplex not support */
            }
        }
        if (0 == (physts & PHY_STS_1000FD)) {
            bmsr = phy_get(pmc, PHY_ANLPAR);
            bmsr &= phy_get(pmc, PHY_ANAR);
            if (bmsr & ANLPAR_B_TX_FD)          physts = PHY_STS_100FD;
            else if (bmsr & ANLPAR_B_TX)        physts = PHY_STS_100HD;
            else if (bmsr & ANLPAR_B_10_FD)     physts = PHY_STS_10FD;
            else                                physts = PHY_STS_10HD;
        }
    }
    else {
        physts = (pmc->phymod_ext & PME_SUP_1000M) ? PHY_STS_1000FD
               : (pmc->phymod_ext & PME_SUP_100M)
               ? ((pmc->phymod_ext & PME_SUP_FD) ? PHY_STS_100FD : PHY_STS_100HD)
               : ((pmc->phymod_ext & PME_SUP_FD) ? PHY_STS_10FD  : PHY_STS_10HD ) ;
    }

    return physts;
}

static T_EMAC_CTL *get_emac_ctl(UH dev_num)
{    
    return &emc[0];
}

ER _ddr_imx8_phy_put(UH dev_num, UB reg, UH data)
{
    T_EMAC_CTL *pmc;
    pmc = get_emac_ctl(dev_num);
    if (NULL == pmc) {
        return E_ID;
    }
    phy_put(pmc, reg, data);
    return E_OK;
}

ER _ddr_imx8_phy_get(UH dev_num, UB reg)
{
    T_EMAC_CTL *pmc;
    pmc = get_emac_ctl(dev_num);
    if (NULL == pmc) {
        return E_ID;
    }
    return phy_get(pmc, reg);
}

static ER phy_ini_realtek_rtl8211f(T_EMAC_CTL *pmc)
{
    UH tmp;

    phy_put(pmc, 0x1f, 0xd08);  /* MIIM_RTL8211F_PAGE_SELECT */

    tmp = phy_get(pmc, 0x11);
    tmp |= 0x100;               /* MIIM_RTL8211F_TX_DELAY */
    phy_put(pmc, 0x11, tmp);

    tmp = phy_get(pmc, 0x15);
    tmp |= 0x8;                 /* MIIM_RTL8211F_RX_DELAY */
    phy_put(pmc, 0x15, tmp);

    phy_put(pmc, 0x1f, 0);      /* MIIM_RTL8211F_PAGE_SELECT */

    phy_put(pmc, 0x1f, 0xd04);  /* MIIM_RTL8211F_PAGE_SELECT */
    phy_put(pmc, 0x10, 0x617f);
    phy_put(pmc, 0x1f, 0);      /* MIIM_RTL8211F_PAGE_SELECT */

    return E_OK;
}

#ifdef EQOS_PHY_USE_RTL8211F
ER (*_phy_ini_board)(T_EMAC_CTL*) = phy_ini_realtek_rtl8211f;
#else
ER (*_phy_ini_board)(T_EMAC_CTL*) = NULL;
#endif

ER phy_ini_board(T_EMAC_CTL *pmc)
{
    ER ercd = E_OK;
    if (_phy_ini_board) {
        ercd = _phy_ini_board(pmc);
    }
    return ercd;
}

static ER phy_ini(T_EMAC_CTL *pmc)
{
    int i;
    UH tmp;

    tmp = phy_get(pmc, PHY_IDR1);
    tmp = phy_get(pmc, PHY_IDR2) ;

    /* Reset PHY */
    phy_put(pmc, PHY_BMCR, BMCR_RESET);

    i = PHY_RST_WAIT_TMO/10;
    for (i = PHY_RST_WAIT_TMO/10; 0 < i; i--) {
        dly_tsk(10);
        if (0 == (phy_get(pmc, PHY_BMCR) & BMCR_RESET)) {
            break;
        }
    }
    if (0 >= i) {
        return E_TMOUT;     /* Error: PHY reset command error */
    }

    phy_ini_board(pmc);     /* PHY-specific initialization process. */
    phy_get(pmc, PHY_BMSR);          /* Dummy status read */

    if (0 == pmc->phymod_ext) {  /* invalidate PHY mode settings */
        pmc->phymod_ext = get_def_phymod(1);
    }

    if (pmc->phymod_ext & PME_SUP_ANE) {     /* Auto negotiation */
        tmp = ANAR_SF_802_3u;
        if (pmc->phymod_ext & PME_SUP_10M) {
            tmp |= ANAR_10 ;
            if (pmc->phymod_ext & PME_SUP_FD)     tmp |= ANAR_10_FD;
        }
        if (pmc->phymod_ext & PME_SUP_100M) {
            tmp |= ANAR_TX ;
            if (pmc->phymod_ext & PME_SUP_FD)     tmp |= ANAR_TX_FD;
        }

        phy_put(pmc, PHY_BMCR, BMCR_ANE);
        phy_put(pmc, PHY_ANAR, tmp);
#if (EMAC_SUP_PHY_MODE & PME_SUP_1000M)     /* only Gigabit support */
        tmp = 0;
        if (pmc->phymod_ext & PME_SUP_1000M) {
            tmp |= GCTL_1000_HD;
            if (pmc->phymod_ext & PME_SUP_FD)      tmp |= GCTL_1000_FD;
        }
        phy_put(pmc, PHY_1000_CTL, tmp);
#endif
        phy_put(pmc, PHY_BMCR, BMCR_ANE | BMCR_RS_ANP);
    }
    else {      /* Manual settings */
        tmp = 0;
        if (pmc->phymod_ext & PME_SUP_1000M) {
            tmp |= BMCR_SPD1000;
        }
        else if (pmc->phymod_ext & PME_SUP_100M) {
            tmp |= BMCR_SPD100;
        }
        else {
            tmp |= BMCR_SPD10;
        }
        if (pmc->phymod_ext & PME_SUP_FD) {
            tmp |= BMCR_DUPLEX;
        }
#if (EMAC_SUP_PHY_MODE & PME_SUP_1000M)     /* only Gigabit support */
        phy_put(pmc, PHY_1000_CTL, 0);
#endif
        phy_put(pmc, PHY_BMCR, tmp);
    }

    pmc->phymod_ext &= ~PME_CFG_NSET;     /* clear nset */

    return E_OK;
}

static void eqos_clk_ini(UH physts);    /* proto type */

static void phy_link_evt(T_EMAC_CTL *pmc, UH physts)
{
    volatile UW tmp;
#ifdef STS_SUP
    T_NET_DEV *dev = &gNET_DEV[pmc->devnum - 1];
#endif
    
    if (PHY_STS_LINK_DOWN == physts) {     /* Link Down */
        pmc->reg->MAC.CONFIGURATION &= ~(MACCR_RE | MACCR_TE);  /* Stop Rx & Tx */
        
        loc_cpu();
        pmc->lan_stat &= ~PHY_STS_MSK;
        unl_cpu();
#ifdef STS_SUP
        /* Inform callback */
        #if (SNMP_ENA == 1)
        net_sts_eth_cbk(pmc->devnum, EV_CBK_DEV_LINK, (VP)(ADDR)physts);
        #endif
        if (dev->cbk) {
            dev->cbk(pmc->devnum, EV_CBK_DEV_LINK, (VP)(ADDR)physts);
        }
#endif
    }
    else {  /* Link Up */
        eqos_clk_ini(physts);   /* change clock settings for port speed. */

        /* Note:
            FES=0, PS=1 ... 10Mbps
            FES=1, PS=1 ... 100Mbps
            FES=0, PS=0 ... 1Gbps
            FES=1, PS=0 ... 2.5Gbps (no support)
        */
        tmp = pmc->reg->MAC.CONFIGURATION;
        tmp &= ~(MACCR_DM|MACCR_FES|MACCR_PS);
        if (physts & (PHY_STS_1000FD | PHY_STS_100FD | PHY_STS_10FD)) {
            tmp |= MACCR_DM;
        }
        if (physts & (PHY_STS_100FD | PHY_STS_100HD)) {
            tmp |= MACCR_FES;
        }
        if (physts & (PHY_STS_100FD | PHY_STS_100HD | PHY_STS_10FD | PHY_STS_10HD)) {
            tmp |= MACCR_PS;
        }
        tmp |= MACCR_RE | MACCR_TE;                 /* Restart Rx & Tx */
        pmc->reg->MAC.CONFIGURATION = tmp;
        
        loc_cpu();
        pmc->lan_stat |= physts;
        unl_cpu();
#ifdef STS_SUP
        /* Inform callback */
        #if (SNMP_ENA == 1)
        net_sts_eth_cbk(pmc->devnum, EV_CBK_DEV_LINK, (VP)(ADDR)physts);
        #endif
        if (dev->cbk) {
            dev->cbk(pmc->devnum, EV_CBK_DEV_LINK, (VP)(ADDR)physts);
        }
#endif
        
    }
}

static void phy_link(T_EMAC_CTL *pmc)
{
    UH sts;
    
    /* don't exit until link up */
    while (ETH_STS_INV != (pmc->lan_stat & ETH_STS_MSK)) {
        sts = phy_sts(pmc);
        if (sts != (pmc->lan_stat & PHY_STS_MSK)) {     /* update link */
            phy_link_evt(pmc, sts); 
        }
        if (sts & PHY_STS_MSK) {
            break;
        }
        dly_tsk(PHY_LINK_INTVAL);
    }
}

#ifndef NET_C_OS
static void ddr_eqos_del_rsc(T_EMAC_CTL *pmc)
{
    if (pmc == 0) {
        return;
    }
#if EQOS_USE_CTL_TSK
    if (pmc->obj->ctl_tsk != 0) {
        (void)del_tsk(pmc->obj->ctl_tsk);
        pmc->obj->ctl_tsk = 0;
    }
#endif
    if (pmc->obj->snd_tsk != 0) {
        (void)del_tsk(pmc->obj->snd_tsk);
        pmc->obj->snd_tsk = 0;
    }
    if (pmc->obj->rcv_tsk != 0) {
        (void)del_tsk(pmc->obj->rcv_tsk);
        pmc->obj->rcv_tsk = 0;
    }

    if (pmc->obj->snd_mbx != 0) {
        (void)del_mbx(pmc->obj->snd_mbx);
        pmc->obj->snd_mbx = 0;
    }

    if (pmc->obj->snd_flg != 0) {
        (void)del_flg(pmc->obj->snd_flg);
        pmc->obj->snd_flg = 0;
    }

    if (pmc->obj->rcv_flg != 0) {
        (void)del_flg(pmc->obj->rcv_flg);
        pmc->obj->rcv_flg = 0;
    }

    if (pmc->obj->isr != 0) {
        (void)del_isr(pmc->obj->isr);
        pmc->obj->isr = 0;
    }

    return;
}

static ER _ddr_imx8_eqos_ini_osres(T_EMAC_CTL *pmc)
{
    ER ercd;
    
    do { 
        /* Create RTOS resouce */
#if EQOS_USE_CTL_TSK
        if (pmc->obj->ctl_tsk == 0) {
            ercd = acre_tsk((T_CTSK *)&ddr_eqos_ctl_ctsk);
            if (E_OK >= ercd)   break;
            pmc->obj->ctl_tsk = (ID)ercd;
        }
#endif
        if (pmc->obj->snd_tsk == 0) {
            ercd = acre_tsk((T_CTSK *)&ddr_eqos_snd_ctsk);
            if (E_OK >= ercd)   break;
            pmc->obj->snd_tsk = (ID)ercd;
        }
        if (pmc->obj->rcv_tsk == 0) {
            ercd = acre_tsk((T_CTSK *)&ddr_eqos_rcv_ctsk);
            if (E_OK >= ercd)   break;
            pmc->obj->rcv_tsk = (ID)ercd;
        }
        if (pmc->obj->snd_mbx == 0) {
            ercd = acre_mbx((T_CMBX *)&ddr_eqos_snd_cmbx);
            if (E_OK >= ercd)   break;
            pmc->obj->snd_mbx = (ID)ercd;
        }
        if (pmc->obj->snd_flg == 0) {
            ercd = acre_flg((T_CFLG *)&ddr_eqos_snd_cflg);
            if (E_OK >= ercd)   break;
            pmc->obj->snd_flg = (ID)ercd;
        }
        if (pmc->obj->rcv_flg == 0) {
            ercd = acre_flg((T_CFLG *)&ddr_eqos_rcv_cflg);
            if (E_OK >= ercd)   break;
            pmc->obj->rcv_flg = (ID)ercd;
        }
        if (pmc->obj->isr == 0) {
            ercd = acre_isr((T_CISR *)&ddr_eqos_cisr);
            if (E_OK >= ercd)   break;
            pmc->obj->isr = (ID)ercd;
            pmc->obj->isr_no = (INTNO)IRQ_EQOS;
        }
    } while (0);
    if (E_OK >= ercd) {
        ddr_eqos_del_rsc(pmc);
        ercd = E_OBJ;
    }
    else {
        ercd = E_OK;
    }

    return ercd;
}

#else
static ER osres_chk_tsk(ID tid)
{
    ER ercd;
    T_RTST k_rtst;
    
    ercd = ref_tst(tid, &k_rtst);
    if (E_OK == ercd) {
        if (0 == (k_rtst.tskstat & TTS_DMT)) {
            ercd = E_OBJ;
        }
    }
    return ercd;
}

static ER osres_chk_flg(ID fid)
{
    ER ercd;
    T_RFLG k_rflg;
    
    ercd = ref_flg(fid, &k_rflg);
    if (E_OK == ercd) {
        ercd = clr_flg(fid, (FLGPTN)-1);
    }
    return ercd;
}

static ER osres_chk_mbx(ID mid)
{
    ER ercd;
    T_RMBX k_rmbx;
    
    ercd = ref_mbx(mid, &k_rmbx);
    if (E_OK == ercd) {
        /* do nothing */
    }
    return ercd;
}

static ER osres_chk_isr(INTNO intno, IMASK imask)
{
    ER ercd;
    
    ercd = dis_int(intno);
    if (E_OK == ercd) {
        ercd = vset_ipl(intno, imask);
    }
    return ercd;
}

static ER _ddr_imx8_eqos_ini_osres(T_EMAC_CTL *pmc)
{
    ER ercd;
    
    do {
        /* Check valid RTOS resouce */
        ercd = osres_chk_tsk(pmc->obj->snd_tsk);
        if (E_OK != ercd)   break;
        
        ercd = osres_chk_tsk(pmc->obj->rcv_tsk);
        if (E_OK != ercd)   break;
        
#if EQOS_USE_CTL_TSK
        ercd = osres_chk_tsk(pmc->obj->ctl_tsk);
        if (E_OK != ercd)   break;
#endif        
        ercd = osres_chk_flg(pmc->obj->snd_flg);
        if (E_OK != ercd)   break;
        
        ercd = osres_chk_flg(pmc->obj->rcv_flg);
        if (E_OK != ercd)   break;
        
        ercd = osres_chk_mbx(pmc->obj->snd_mbx);
        if (E_OK != ercd)   break;
        
        ercd = osres_chk_isr(pmc->obj->isr_no, pmc->cfg->dat.ipl_eth);
        if (E_OK != ercd)   break;        

    } while (0);

    return ercd;
}
#endif




static ER _ddr_imx8_eqos_ini_dma(T_EMAC_CTL *pmc)
{
    ER ercd;
    INT i;
    
    pmc->reg->DMA.MODE = DMAMR_SWR;
    for (i = 10; i > 0; i--) {
        dly_tsk(0);
        if (0 == (pmc->reg->DMA.MODE & DMAMR_SWR)) {
            break;
        }
    }
    if (i <= 0) {
        return E_TMOUT;     /* Error: DMA reset error (configuration error?) */
    }
    
    ercd = EthDMARxDescInit(pmc);
    if (E_OK != ercd) {
        return ercd;
    }
    
    EthDMATxDescInit(pmc);
    
    pmc->reg->DMA.SYSBUS_MODE = DMASBMR_FB;    
    pmc->reg->DMA.CH[0].CONTROL   = (1 << DMACCR_DSL_pos) | DMACCR_PBLX8;
    pmc->reg->DMA.CH[0].TX_CONTROL = (32 << DMACTXCR_TXPBL_pos);
    pmc->reg->DMA.CH[0].RX_CONTROL = (32 << DMACRXCR_RXPBL_pos) | DMACRXCR_RPF | (pmc->cfg->buf_len << DMACRXCR_RBSZ_pos);
    pmc->reg->DMA.CH[0].INTERRUPT_ENABLE = DMACIER_NIE | DMACIER_AIE | INT_TX_BITS | INT_RX_BITS | INT_ERR_BITS;
    
    pmc->reg->DMA.CH[0].RX_CONTROL &= ~DMACRXCR_RPF;
    pmc->reg->DMA.CH[0].TX_CONTROL |= DMACTXCR_ST;
    pmc->reg->DMA.CH[0].RX_CONTROL |= DMACRXCR_SR;
    
    return E_OK;
}

static ER _ddr_imx8_eqos_ini_mtl(T_EMAC_CTL *pmc)
{
    UW fifo_sz;
    INT i;
    
    pmc->reg->MTL.TXQ0_OPERATION_MODE = MTLTXQOMR_FTQ;
    for (i = 10; i > 0; i--) {
        dly_tsk(0);
        if (0 == (pmc->reg->MTL.TXQ0_OPERATION_MODE & MTLTXQOMR_FTQ)) {
            break;
        }
    }
    if (i <= 0) {
        return E_TMOUT;     /* Error: MTL init error (configuration error?) */
    }

    fifo_sz = (pmc->reg->MAC.HW_FEATURE1 >> 6) & 0x1F;      /* TXFIFOSIZE */
    fifo_sz = (128 << fifo_sz) / 256 - 1;
    pmc->reg->MTL.TXQ0_OPERATION_MODE = MTLTXQOMR_TSF | (0x02 << MTLTXQOMR_TXQEN_pos);
    pmc->reg->MTL.TXQ0_OPERATION_MODE |= (fifo_sz << MTLTXQOMR_TQS_pos);
    pmc->reg->MTL.TXQ0_QUANTUM_WEIGHT = 0x10;

    fifo_sz = (pmc->reg->MAC.HW_FEATURE1 >> 0) & 0x1F;      /* RXFIFOSIZE */
    fifo_sz = (128 << fifo_sz) / 256 - 1;
    pmc->reg->MTL.RXQ0_OPERATION_MODE = MTLRXQOMR_RSF;
    pmc->reg->MTL.RXQ0_OPERATION_MODE |= (fifo_sz << MTLRXQOMR_RQS_pos);
    //pmc->reg->MTL.RXQ0_OPERATION_MODE |= MTLRXQOMR_FEP | MTLRXQOMR_FUP;   /* debug */
    pmc->reg->MTL.Q0_INTERRUPT_CONTROL_STATUS = 0;         /* disable MTL interrupt */
    
    return E_OK;
}

static ER _ddr_imx8_eqos_ini_mac(T_EMAC_CTL *pmc)
{
    volatile UW reg;
    UB          *mac;    
    
    mac = &gNET_DEV[pmc->devnum - 1].cfg.eth.mac[0];
    pmc->reg->MAC.ADDRESS0_5[0].HIGH = ((mac[5] << 8 )| mac[4]);
    pmc->reg->MAC.ADDRESS0_5[0].LOW = ((mac[3] << 24)| (mac[2] << 16) | (mac[1] << 8) | mac[0]);
    pmc->reg->MAC.ADDRESS0_5[0].HIGH |= MACAxHR_AE;
    
    /* Rx Filter */
    if (1 == pmc->cfg->dat.eth_filter) {
        pmc->reg->MAC.PACKET_FILTER = MACPFR_PR;     /* Promiscuous Mode (Receive All packets)  */
    }
    else if (2 == pmc->cfg->dat.eth_filter) {
        pmc->reg->MAC.PACKET_FILTER = MACPFR_PM;     /* Receive Multicast    */
    }
    else {
        pmc->reg->MAC.PACKET_FILTER = 0;
    }
    
    pmc->reg->MAC.RXQ_CTRL0 = 0x02 << 0;    /* RXQ0EN */
    pmc->reg->MAC.Q0_TX_FLOW_CTRL = 0;
    pmc->reg->MAC.INTERRUPT_ENABLE = 0;           /* disable MAC interrupt */
    reg = 0;
    reg |= MACCR_ACS;               /* Automatic Pad or CRC Stripping */
    reg |= MACCR_FES | MACCR_DM;    /* Duplex, Full Speed */
    if (pmc->cfg->dat.eth_csum & 2) {  
        reg |= MACCR_IPC;           /* Checksum Offload */
    }
    pmc->reg->MAC.CONFIGURATION = reg;    
    
    /* disable MMC interrupt */
    pmc->reg->MAC.MMC_CONTROL = 1;      /* Reset MMC counter */
    pmc->reg->MAC.MMC_RX_INTERRUPT_MASK = 0x0FFFFFFF;
    pmc->reg->MAC.MMC_TX_INTERRUPT_MASK = 0x0FFFFFFF;
    pmc->reg->MAC.MMC_IPC_RX_INTERRUPT_MASK = 0x3FFF3FFF;
    pmc->reg->MAC.MMC_CONTROL = 0;
    
    return E_OK;
}


static void target_clock_init(UB idx, UB mux, UB pre_div, UB post_div)
{
    UW tempreg;
    /* Set {idx} clock root to Osc24M */
    tempreg = REG_CCM_ROOT(idx).TARGET_ROOT;
    tempreg = (tempreg & ~0x7000000UL) | (0UL << 24);
    REG_CCM_ROOT(idx).TARGET_ROOT = tempreg;

    /* Set {idx}  root divider to {pre_div, post_div} */
    tempreg = REG_CCM_ROOT(idx).TARGET_ROOT;
    tempreg = (tempreg & (~(0x70000UL | 0x3FU))) | ((pre_div - 1U) << 16U) | ((post_div - 1U) << 0U);
    REG_CCM_ROOT(idx).TARGET_ROOT = tempreg;

    /* Set {idx}  clock root to {mux} clock */
    tempreg = REG_CCM_ROOT(idx).TARGET_ROOT;
    tempreg = (tempreg & ~0x7000000UL) | (mux << 24);
    REG_CCM_ROOT(idx).TARGET_ROOT = tempreg;

    _kernel_synch_cache();
}

static void eqos_clk_ini(UH physts)
{
    /* disable the clock gate */
    REG_CCM_CCGR(CCGR_QOS_ENET).CCGR_CLR = 3;
    REG_CCM_CCGR(CCGR_ENET_QOS).CCGR_CLR = 3;
    
    if (0 == physts) {  /* Initialize */
        target_clock_init(ENET_AXI_CLK_ROOT, 1, 1, 1);          /* 266MHz, SYSTEM_PLL1_DIV3 */
        target_clock_init(ENET_QOS_CLK_ROOT, 1, 1, 1);          /* 125MHz, SYSTEM_PLL2_DIV8 */
        target_clock_init(ENET_QOS_TIMER_CLK_ROOT, 1, 1, 4);    /* 25MHz,  SYSTEM_PLL2_DIV10 */
    }
    else {  /* Change port speed. */
        /* Note: If the clock is not suitable for the port speed, transmission will fail. */
        if (physts & (PHY_STS_10FD | PHY_STS_10HD)) {
            target_clock_init(ENET_QOS_CLK_ROOT, 1, 1, 50); /* 2.5MHz, SYSTEM_PLL2_DIV8 */
        }
        else if (physts & (PHY_STS_100FD | PHY_STS_100HD)) {
            target_clock_init(ENET_QOS_CLK_ROOT, 1, 1, 5);  /*  25MHz, SYSTEM_PLL2_DIV8 */
        }
        else { /* if (physts & (PHY_STS_1000FD | PHY_STS_1000HD)) */
            target_clock_init(ENET_QOS_CLK_ROOT, 1, 1, 1);  /* 125MHz, SYSTEM_PLL2_DIV8 */
        }
    }

    /* enable the clock gate */
    REG_CCM_CCGR(CCGR_QOS_ENET).CCGR_SET = 3;
    REG_CCM_CCGR(CCGR_ENET_QOS).CCGR_SET = 3;
    _kernel_synch_cache();    
}


ER _ddr_imx8_eqos_ini(UH dev_num)
{
    T_NET       *net;
    volatile UW reg;
    ER          ercd;
    T_EMAC_CTL *pmc;

    if ((dev_num == 0) || (dev_num > NET_DEV_MAX)) {
        return E_ID;
    }

    net = &gNET[dev_num - 1];
    if (NULL == net) {
        return E_PAR;
    }
    if (NULL == net->dev) {
        return E_PAR;
    }
    
    pmc = get_emac_ctl(dev_num);
    if ((pmc->lan_stat & ETH_STS_INI)) {
        return E_OBJ;       /* Error: Device Already initialized */
    }
    pmc->devnum = dev_num;
    pmc->lan_stat = ETH_STS_ERR;

    ercd = _ddr_imx8_eqos_ini_osres(pmc);
    if (E_OK != ercd) {
        return ercd;
    }
    
    if (pmc->cfg->dat.eth_csum & 1) {   /* Enable Tx Hardware checksum */
        net->flag |= HW_CS_TX_IPH4;
        net->flag |= HW_CS_TX_DATA;
    }

#if 1 // (2 == EQOS_RMII_MODE)   /* only support RGMII */
    reg = REG_IOMUXC_GPR.GPR[1] & ~0x007F6000;
    reg |= (1 << 21);       /* IOMUXC_GPR_ENET_QOS_RGMII_EN = 1 */
    reg |= (1 << 19);       /* GPR_ENET_QOS_CLK_GEN_EN = 1 */
    reg |= (1 << 16);       /* GPR_ENET_QOS_INTF_SEL = 1 (RGMII) */
    REG_IOMUXC_GPR.GPR[1] = reg;
    _kernel_synch_cache();
#endif

    eqos_clk_ini(0);    /* Enable ENET_QOS Peripheral Clock */

    /* 11.7.4 Initialization */
    ercd = _ddr_imx8_eqos_ini_dma(pmc);      /* 11.7.4.1 Initializing DMA */
    if (E_OK != ercd) {
        return ercd;
    }    
    ercd = _ddr_imx8_eqos_ini_mtl(pmc);      /* 11.7.4.2 Initializing MTL Registers */
    if (E_OK != ercd) {
        return ercd;
    }    
    ercd = _ddr_imx8_eqos_ini_mac(pmc);      /* 11.7.4.3 Initializing MAC */
    if (E_OK != ercd) {
        return ercd;
    }    
    
    /***** PHY Initialization *****/
    ercd = phy_ini(pmc);
    if (E_OK != ercd) {
#ifndef NET_C_OS
        ddr_eqos_del_rsc(pmc);
#endif
        return ercd;
    }    
    
    sta_tsk(pmc->obj->snd_tsk, (VP)pmc);
    sta_tsk(pmc->obj->rcv_tsk, (VP)pmc);
#if EQOS_USE_CTL_TSK
    sta_tsk(pmc->obj->ctl_tsk, (VP)pmc);
#endif

    pmc->lan_stat = ETH_STS_INI;
    
    /* Enable Interrupt mode operation */
    (void)ena_int(pmc->obj->isr_no);
    
    //pmc->reg->MAC.CONFIGURATION |= MACCR_RE | MACCR_TE;       /* don't communicate until linkup */
    
    return E_OK;
}

ER _ddr_imx8_eqos_cls(UH dev_num)
{
    T_EMAC_CTL *pmc;
    T_NET_BUF *pkt;
    ER ercd;
    FLGPTN ptn;
    int i;
    
    pmc = get_emac_ctl(dev_num);
    if (NULL == pmc) {
        return E_ID;
    }
    else if ((pmc->lan_stat & ETH_STS_INI) == 0x00) {
        return E_OBJ;       /* Error: Device Not initialized */
    }

    loc_cpu();
    pmc->lan_stat = (pmc->lan_stat & ~ETH_STS_MSK) | ETH_STS_INV;

    /* Stop ether interrupt */
    (void)dis_int(pmc->obj->isr_no);    
    pmc->reg->DMA.CH[0].INTERRUPT_ENABLE = 0;                  /* Disable DMA interrupt */
    pmc->reg->DMA.CH[0].TX_CONTROL &= ~DMACTXCR_ST;     /* Disable the Transmit DMA */
    pmc->reg->MTL.TXQ0_OPERATION_MODE |= MTLTXQOMR_FTQ;   /* Flush Transmit FIFO */
    pmc->reg->DMA.CH[0].RX_CONTROL &= ~DMACRXCR_SR;     /* Disable the Recieve DMA */
    pmc->reg->MAC.CONFIGURATION &= ~MACCR_TE;           /* Disable MAC Transmit function */
    pmc->reg->MAC.CONFIGURATION &= ~MACCR_RE;           /* Disable MAC Receive function */
    unl_cpu();

    /* Stop All Ethernet task */
#if EQOS_USE_CTL_TSK
    rel_wai(pmc->obj->ctl_tsk);
#endif
    rel_wai(pmc->obj->rcv_tsk);
    rel_wai(pmc->obj->snd_tsk);
    
    /* Wait Stop Ethernet task */
    ercd = twai_flg(pmc->obj->rcv_flg, (EV_EQOS_CTL_EXIT | EV_EQOS_RX_EXIT | EV_EQOS_TX_EXIT), TWF_ANDW, &ptn, 1000);    
    if (E_OK != ercd) {
        /* Kill active tasks. */
        ptn = 0;
        ercd = pol_flg(pmc->obj->rcv_flg, (EV_EQOS_CTL_EXIT | EV_EQOS_RX_EXIT | EV_EQOS_TX_EXIT), TWF_ORW, &ptn);    
#if EQOS_USE_CTL_TSK
        if (0 == (ptn & EV_EQOS_CTL_EXIT))   (void)ter_tsk(pmc->obj->ctl_tsk);
#endif
        if (0 == (ptn & EV_EQOS_RX_EXIT))    (void)ter_tsk(pmc->obj->rcv_tsk);
        if (0 == (ptn & EV_EQOS_TX_EXIT))    (void)ter_tsk(pmc->obj->snd_tsk);
    }
    
#if EQOS_RAM_USE_NETBUF
    /* Release network buffer using for rx DMA */
    for (i = 0; i < pmc->rd_cnt; i++) {
        if (pmc->rx_desc[i].pkt)
            net_buf_ret(pmc->rx_desc[i].pkt);
    }
#endif
    
    /* Flush tx queue and revoke packets */
    ercd = E_OK;
    while (ercd == E_OK) {
         ercd = prcv_mbx(pmc->obj->snd_mbx, (T_MSG **)&pkt);
        if (ercd == E_OK) {
            net_buf_ret(pkt);
        }
    }

#ifndef NET_C_OS
    ddr_eqos_del_rsc(pmc);
#endif
    
    return E_OK;
}

ER _ddr_imx8_eqos_snd(UH dev_num, T_NET_BUF *pkt)
{
    T_EMAC_CTL *pmc;
    
    pmc = get_emac_ctl(dev_num);
    if (NULL == pmc) {
        return E_ID;
    }
    else if ((pmc->lan_stat & ETH_STS_INI) == 0x00) {
        return E_OBJ;       /* Error: Device Not initialized */
    }
    
    /* Add to Device send queue */
    (void)snd_mbx(pmc->obj->snd_mbx, (T_MSG *)pkt);
    return E_WBLK;
}

ER _ddr_imx8_eqos_cfg(UH dev_num, UH opt, VP val)
{
    T_EMAC_CTL *pmc;
    ER ercd;
    INT tmp;
    
    pmc = get_emac_ctl(dev_num);
    if (NULL == pmc) {
        return E_ID;
    }

    /* Configuration items. (Initialization not required.) */
    ercd = E_OK;
    switch (opt) {
        case CFG_PHY_MODE:
            tmp = cnv_phymod(1, (INT)(ADDR)val);
            if (0 > tmp) {
                ercd = E_PAR;      /* invalid value */
            }
            else {
                pmc->phymod_ext = (UH)tmp | PME_CFG_NSET;
            }
            break;

        case CFG_PHY_MODE_EXT:
            tmp = (INT)(ADDR)val;

            if (0 != (~EMAC_SUP_PHY_MODE & tmp)) {
                ercd = E_PAR;      /* invalid value */
            }
            else if (0 == (PME_SUP_ANE & tmp)) {
                tmp = (tmp & PME_SUP_MASK) & ~PME_SUP_FD;
                if (!((PME_SUP_10M == tmp) || (PME_SUP_100M == tmp)
                                        || (PME_SUP_1000M == tmp))) {
                    ercd = E_PAR;
                }
            }
            if (E_OK == ercd) {
                pmc->phymod_ext = (UH)(ADDR)val | PME_CFG_NSET;
            }
            break;

        default:
            ercd = E_NOSPT;
            break;
    }
    if (E_NOSPT != ercd)   return ercd;

    /* Configuration items. (Initialization required.) */
    if ((pmc->lan_stat & ETH_STS_INI) == 0x00) {
        return E_OBJ;       /* Error: Device Not initialized */
    }

    ercd = E_OK;
    switch (opt) {
        case 0:
        default:
            ercd = E_NOSPT;
            break;
    }

    return ercd;
}

ER _ddr_imx8_eqos_ref(UH dev_num, UH opt, VP val)
{
    T_EMAC_CTL *pmc;
    ER ercd;
    UH* sts;
    #if (SNMP_ENA == 1)
    T_NET_STS_IFS* ifs;
    volatile struct t_enetqos_mac *eqm;
    #endif

    pmc = get_emac_ctl(dev_num);
    if (NULL == pmc) {
        return E_ID;
    }
    if (NULL == val) {
        return E_PAR;
    }

    /* Reference items. (Initialization not required.) */
    ercd = E_OK;
    switch (opt) {
        case CFG_PHY_MODE:
        case CFG_PHY_MODE_EXT:
            if (0 == pmc->phymod_ext) {  /* invalidate PHY mode settings */
                pmc->phymod_ext = get_def_phymod(1);
            }
            *(UH *)val = (opt == CFG_PHY_MODE)
                        ? cnv_phymod(0, pmc->phymod_ext) : pmc->phymod_ext;
            break;

        default:
            ercd = E_NOSPT;
            break;
    }
    if (E_NOSPT != ercd)   return ercd;

    /* Reference items. (Initialization required.) */
    if ((pmc->lan_stat & ETH_STS_INI) == 0x00) {
        return E_OBJ;
    }

    ercd = E_NOSPT;

    if (opt == REF_ETH_LINK_STS) {
        /* Link status */
        sts = (UH*)val;
        loc_cpu();
        *sts = pmc->lan_stat & PHY_STS_MSK;
        unl_cpu();
        ercd = E_OK;
    }
    #if (SNMP_ENA == 1)
    else if (opt == REF_ETH_UPD_STS) {
        /* Update status [in_octet..out_qlen] */
        ifs = (T_NET_STS_IFS*)val;
        /* Status */
        loc_cpu();
        ifs->oper_sts = (pmc->lan_stat & PHY_STS_MSK) ? NET_STS_LINK_UP : NET_STS_LINK_DOWN;
        unl_cpu();
        eqm = &pmc->reg->MAC;
        /* Rx */
        ifs->in_octet = eqm->RX_OCTET_COUNT_GOOD_BAD;
        ifs->in_ucast_pkt = eqm->RX_UNICAST_PACKETS_GOOD;
        ifs->in_nucast_pkt = eqm->RX_BROADCAST_PACKETS_GOOD + eqm->RX_MULTICAST_PACKETS_GOOD;
        /* Undersize, Oversize, FIFO Overflow */
        ifs->in_discard = eqm->RX_UNDERSIZE_PACKETS_GOOD + eqm->RX_OVERSIZE_PACKETS_GOOD + eqm->RX_FIFO_OVERFLOW_PACKETS;
        /* CRC, Alignment, Runt, Jabber, Length error, OutofRangetype, Watchdog, GMII/MII */
        ifs->in_err = eqm->RX_CRC_ERROR_PACKETS + eqm->RX_ALIGNMENT_ERROR_PACKETS + eqm->RX_RUNT_ERROR_PACKETS +
            eqm->RX_JABBER_ERROR_PACKETS + eqm->RX_LENGTH_ERROR_PACKETS + eqm->RX_OUT_OF_RANGE_TYPE_PACKETS +
            eqm->RX_WATCHDOG_ERROR_PACKETS + eqm->RX_RECEIVE_ERROR_PACKETS;
        ifs->in_unknown_proto = pmc->rx_unkwn;
        
        /* Tx */
        ifs->out_octet = eqm->TX_OCTET_COUNT_GOOD_BAD;
        ifs->out_ucast_pkt = eqm->TX_UNICAST_PACKETS_GOOD_BAD;
        ifs->out_nucast_pkt = eqm->TX_BROADCAST_PACKETS_GOOD_BAD + eqm->TX_MULTICAST_PACKETS_GOOD_BAD;
        ifs->out_discard = pmc->tx_discard;
        /* Collinsion, Late Collinsion,  Lost carrier sense, Excessive deferral */
        ifs->out_err = eqm->TX_EXCESSIVE_COLLISION_PACKETS + eqm->TX_LATE_COLLISION_PACKETS +
            eqm->TX_CARRIER_ERROR_PACKETS + eqm->TX_EXCESSIVE_DEFERRAL_ERROR;
        _kernel_synch_cache();
        ercd = E_OK;
    }
    #endif
    else {
        ercd = E_NOSPT;
    }


    return ercd;
}

static void _ddr_imx8_eqos_intr_dma(T_EMAC_CTL *pmc)
{
    volatile UW csr;
    UW ier;
    FLGPTN tx_ptn, rx_ptn;
    
    tx_ptn = 0;
    rx_ptn = 0;
    
    /* Detection of interrupt factors */
    csr = pmc->reg->DMA.CH[0].STATUS;
    ier = pmc->reg->DMA.CH[0].INTERRUPT_ENABLE;
    ier &= csr;
    
    /* Set Eventflag values */
    if (ier & INT_TX_BITS) {
        tx_ptn |= EV_EQOS_TX;
        DBG_INC(1, pmc, dma_tx);
    }
    if (ier & INT_TX_ERR_BITS) {
        tx_ptn |= EV_EQOS_TX_ERR;
        DBG_INC(1, pmc, dma_tx_err);
    }
    
    if (ier & INT_RX_BITS) {
        rx_ptn |= EV_EQOS_RX;
        DBG_INC(1, pmc, dma_rx);
    }
    if (ier & INT_RX_ERR_BITS) {
        rx_ptn |= EV_EQOS_RX_ERR;
        DBG_INC(1, pmc, dma_rx_err);
    }
    
    if (ier & DMACIER_FBEE) {       /* Fatal Bus Error */     
        if (csr & DMACSR_TEB_msk) {     /* Tx DMA Error */
            tx_ptn |= EV_EQOS_TX_ERR;
            DBG_INC(1, pmc, dma_tx_err);
        }
        if (csr & DMACSR_REB_msk) {     /* Rx DMA Error */
            rx_ptn |= EV_EQOS_RX_ERR;
            DBG_INC(1, pmc, dma_rx_err);
        }
    }
    
    /* Clear interrupt */
    if (csr) {
        pmc->reg->DMA.CH[0].STATUS = csr;
    }
    
    /* Notify Send/Receive Tasks */
    if (tx_ptn) {
        (void)iset_flg(pmc->obj->snd_flg, tx_ptn);
    }
    if (rx_ptn) {
        (void)iset_flg(pmc->obj->rcv_flg, rx_ptn);
    }    
}

static void _ddr_imx8_eqos_intr_mac(T_EMAC_CTL *pmc)
{
    /* no use (abnormal branch) */
    volatile UW csr;
    
    /* Clear interrupt */
    pmc->reg->MAC.MMC_CONTROL = 1;  /* Reset MMC counter */
    csr = pmc->reg->MAC.INTERRUPT_STATUS;
    csr = pmc->reg->MAC.TIMESTAMP_STATUS;
    csr = pmc->reg->MAC.LPI_CONTROL_STATUS;
    csr = pmc->reg->MAC.PMT_CONTROL_STATUS;
    (void)csr;
    pmc->reg->MAC.MMC_CONTROL = 0;
}

static void _ddr_imx8_eqos_intr_mtl(T_EMAC_CTL *pmc)
{
    /* no use (abnormal branch) */
    volatile UW csr;
    
    /* Clear interrupt */
    csr = pmc->reg->MTL.Q0_INTERRUPT_CONTROL_STATUS;
    pmc->reg->MTL.Q0_INTERRUPT_CONTROL_STATUS = csr;
}

void _ddr_imx8_eqos_intr(void)
{
    T_EMAC_CTL *pmc = &emc[0];
    volatile UW isr;

    isr = pmc->reg->DMA.INTERRUPT_STATUS;
    if (isr & DMAISR_DC0IS) {
        DBG_INC(1, pmc, intr.dma);
        _ddr_imx8_eqos_intr_dma(pmc);
    }
    if (isr & DMAISR_MACIS) {
        DBG_INC(1, pmc, intr.mac);
        _ddr_imx8_eqos_intr_mac(pmc);
    }
    if (isr & DMAISR_MTLIS) {
        DBG_INC(1, pmc, intr.mtl);
        _ddr_imx8_eqos_intr_mtl(pmc);
    }
}

#if EQOS_USE_CTL_TSK
void _ddr_imx8_eqos_ctl_tsk(VP_INT exinf)
{
    T_EMAC_CTL *pmc = exinf;
    FLGPTN ptn;
    ER ercd;
    
    (void)clr_flg(pmc->obj->rcv_flg, ~EV_EQOS_CTL_EXIT);

    while (ETH_STS_INV != (pmc->lan_stat & ETH_STS_MSK)) {
        ercd = wai_flg(pmc->obj->rcv_flg, EV_EQOS_LNK_ERR, TWF_ORW, &ptn);
        if (E_RLWAI == ercd) {
            continue;
        }
        (void)clr_flg(pmc->obj->rcv_flg, ~(EV_EQOS_LNK_ERR));
        
        phy_link(pmc);
        
        (void)set_flg(pmc->obj->rcv_flg, EV_EQOS_LNK);
    }
    
    (void)set_flg(pmc->obj->rcv_flg, EV_EQOS_CTL_EXIT);
}
#endif

void _ddr_imx8_eqos_rcv_tsk(VP_INT exinf)
{
    T_EMAC_CTL *pmc = exinf;    
    T_NET_BUF *pkt;
    T_NET_DEV *dev;
    ER ercd;
    UW rxlen;
    volatile T_EQOS_DESC *rxdes;
    UH       err;
    FLGPTN   ptn;


    dev = &gNET_DEV[pmc->devnum - 1];
    pmc->rd_idx = 0;
    
    (void)clr_flg(pmc->obj->rcv_flg, ~EV_EQOS_RX_EXIT);
    phy_link(pmc);

#if (0 == EQOS_RAM_USE_NETBUF)
    /* allocate buffer */
    ercd = net_buf_get(&pkt, pmc->cfg->buf_len, TMO_FEVR);
    if (E_OK != ercd) {
        /* Fatal Error! */
        (void)set_flg(pmc->obj->rcv_flg, EV_EQOS_RX_EXIT);
        return;
    }
#endif    

    while (ETH_STS_INV != (pmc->lan_stat & ETH_STS_MSK)) {
        rxdes = &pmc->rx_desc[pmc->rd_idx];

        if (rxdes->DESC[3] & RDES3W_OWN) {
            /* Update Rx descriptor tail pointer */
            ercd = pmc->rd_idx - 1;
            if (0 > ercd) {
                ercd = pmc->rd_cnt - 1;
            }
            pmc->reg->DMA.CH[0].RXDESC_TAIL_POINTER  = PTR2BDBUF(&pmc->rx_desc[ercd]);
            
            ercd = twai_flg(pmc->obj->rcv_flg, (EV_EQOS_RX|EV_EQOS_RX_ERR), TWF_ORW, &ptn, EQOS_RX_TMO);
            if (E_OK == ercd) {
                (void)clr_flg(pmc->obj->rcv_flg, ~(ptn));
            }
            else if (E_RLWAI == ercd) {
                continue;
            }
            else {
                phy_link(pmc);
            }
            continue;
        }

        /* Need to clear Receive status? */

        pmc->rd_idx++;
        if (pmc->rd_idx >= pmc->rd_cnt) {
            pmc->rd_idx = 0;
        }
        
        /* Discard bad packets. */
        if (rxdes->DESC[3] & RDES3W_ES) {
            /* When this bit is set, it indicates the logical OR of the following bits:
              - RDES3[24]: CRC Error
              - RDES3[19]: Dribble Error (only MII check)
              - RDES3[20]: Receive Error
              - RDES3[22]: Watchdog Timeout
              - RDES3[21]: Overflow Error
              - RDES3[23]: Giant Packet
              - RDES2[17]: Destination Address Filter Fail (*)
              - RDES2[16]: SA Address Filter Fail (*)
                *when Flexible RX Parser is enabled (no use)
            */
            if (0 == (rxdes->DESC[3] & RDES3W_DE)) {    /* ignore dribble bit error */
                GOTO_RX_DROP(pmc, drop.ES);
            }
        }
        
        /* RDES3[29]: Last descriptor, RDES3[28]: First descriptor */
        if ((RDES3W_FD | RDES3W_LD) != (rxdes->DESC[3] & (RDES3W_FD | RDES3W_LD))) {
            GOTO_RX_DROP(pmc, drop.FD_LD);
        }
        /* RDES2[17]: Destination Address Filter Fail */   
        if (rxdes->DESC[2] & RDES2W_DAF) {
            GOTO_RX_DROP(pmc, drop.DAF);
        }

        /* check packet length */
        rxlen  = ((rxdes->DESC[3] & RDES3W_PL_msk) >> RDES3W_PL_pos);
        rxlen -= 4;                 /* Subtract FCS size */
        if (rxlen < ETH_HDR_SZ) {
            GOTO_RX_DROP(pmc, drop.length_err);
        }
        
        err = 0;
        /* IP Payload Error */
        if (rxdes->DESC[1] & RDES1W_IPCE) {
            err   |= HW_CS_DATA_ERR;
        }
        /* IP Header Error */
        if (rxdes->DESC[1] & RDES1W_IPHE) {
            err   |= HW_CS_IPH4_ERR;
        }

        DCACHE_INVALID(&rxdes->pkt->buf[dev->hhdrofs], rxlen);
#if EQOS_RAM_USE_NETBUF
        pkt          = rxdes->pkt;
#else
        net_memcpy(&pkt->buf[dev->hhdrofs], &rxdes->pkt->buf[dev->hhdrofs], rxlen);
#endif
        pkt->hdr     = pkt->buf + dev->hhdrofs;
        pkt->hdr_len = ETH_HDR_SZ;
        pkt->dat     = pkt->hdr + pkt->hdr_len;
        pkt->dat_len = rxlen - pkt->hdr_len;
        pkt->dev = dev;
        if (pmc->cfg->dat.eth_csum & 2) {   /* Enable Rx Hardware checksum */
            pkt->flg    |= (HW_CS_RX_IPH4|HW_CS_RX_DATA);
            pkt->flg    |= err;
        }
        
        /* Protocol stack process */
        DBG_INC(1, pmc, recv);
        net_pkt_rcv(pkt);

        /* allocate buffer */
        ercd = net_buf_get(&pkt, pmc->cfg->buf_len, TMO_FEVR);
        if (E_OK != ercd) {
            /* Fatal Error! */
            break;
        }
#if EQOS_RAM_USE_NETBUF
        rxdes->pkt = pkt;
#endif
        if (0) {
rx_drop:
            DBG_INC(1, pmc, recv_err);
        }
        DCACHE_INVALID(&rxdes->pkt->buf[dev->hhdrofs], pmc->cfg->buf_len);
        rxdes->DESC[0] = PTR2BDBUF(&rxdes->pkt->buf[dev->hhdrofs]);
        rxdes->DESC[1] = 0;
        rxdes->DESC[2] = 0;
        rxdes->DESC[3] = RDES3R_BUF1V;
        rxdes->DESC[3] |= RDES3R_OWN;
    }
    
    (void)set_flg(pmc->obj->rcv_flg, EV_EQOS_RX_EXIT);
}

static ER make_tx_desc(T_EMAC_CTL *pmc, T_NET_BUF *pkt)
{
    volatile T_EQOS_DESC *txdes;
    UW           sts;
    
    /* Get free Desc */
    txdes = &pmc->tx_desc[pmc->td_idx];
    if (txdes->DESC[3] & TDES3R_OWN) {
        return E_TMOUT;
    }

    pmc->td_idx++;
    if (pmc->td_idx >= pmc->td_cnt) {
        pmc->td_idx = 0;
    }
    (void)clr_flg(pmc->obj->snd_flg, ~(EV_EQOS_TX|EV_EQOS_TX_ERR));
#if EQOS_RAM_USE_NETBUF
    txdes->pkt  = pkt;
#else
    net_memcpy(txdes->pkt->hdr, pkt->hdr, pkt->hdr_len);
    txdes->pkt->hdr_len = pkt->hdr_len;
    txdes->pkt->flg     = pkt->flg;
    pkt = txdes->pkt;
#endif
    txdes->DESC[0] = PTR2BDBUF(pkt->hdr);   /* 2byte aligned */
    txdes->DESC[1] = 0;
    txdes->DESC[2] = pkt->hdr_len;     /* data length to be transmitted */
    
    sts = pkt->hdr_len;
    sts |= TDES3R_OWN | TDES3R_FD | TDES3R_LD;
    
    if (pmc->cfg->dat.eth_csum & 1) {   /* Enable Tx Hardware checksum */
        if (pkt->flg & HW_CS_TX_IPH4) {
            sts  |= TDES3R_CIC_IPHDR;
        }
        if (pkt->flg & HW_CS_TX_DATA) {
            sts  |= TDES3R_CIC_IPHDR_PSEUDO_PL;
        }    
    }
    
    DCACHE_FLUSH(pkt->hdr, pkt->hdr_len);
    _kernel_synch_cache();
    txdes->DESC[3] = sts;
    
    return E_OK;
}

static ER EthTxFrame(T_EMAC_CTL *pmc, T_NET_BUF *pkt)
{
    FLGPTN       ptn = 0;
    ER           ercd;

#if 0 //EQOS_USE_CTL_TSK    /* set to 1, if link check in _ddr_imx8_eqos_ctl_tsk() is valid. */
    /* Check for Link Status */
    if (PHY_STS_LINK_DOWN == (pmc->lan_stat & PHY_STS_MSK)) {
        /* Wait for few milli seconds and check for link before
           giveup transmission.
           Note: EV_EQOS_LNK should be set when link interrupt occurs.
                 Since the link interrupt is not used, EV_EQOS_LNK will
                 never set.
        */
        ercd = twai_flg(pmc->obj->rcv_flg, EV_EQOS_LNK, TWF_ORW, &ptn, EQOS_TX_TMO);
        if (E_OK != ercd) {
            return E_TMOUT;
        }
    }
#else
    if (PHY_STS_LINK_DOWN == (pmc->lan_stat & PHY_STS_MSK)) {
        return E_TMOUT;
    }
#endif
    
    ercd = make_tx_desc(pmc, pkt);
    if (E_OK != ercd) {
        return ercd;
    }
    
    loc_cpu();
    pmc->reg->DMA.CH[0].TXDESC_TAIL_POINTER  = PTR2BDBUF(&pmc->tx_desc[pmc->td_idx]);
    unl_cpu();
    
    ercd = twai_flg(pmc->obj->snd_flg, (EV_EQOS_TX|EV_EQOS_TX_ERR),TWF_ORW, &ptn, EQOS_TX_TMO);
    if (ptn & EV_EQOS_TX_ERR) {
        ercd = E_TMOUT;
    }
    
    return ercd;
}


void _ddr_imx8_eqos_snd_tsk(VP_INT exinf)
{
    T_EMAC_CTL *pmc = exinf;
    T_NET_BUF *pkt;
    ER ercd;

    (void)clr_flg(pmc->obj->rcv_flg, ~EV_EQOS_TX_EXIT);
    
    while (ETH_STS_INV != (pmc->lan_stat & ETH_STS_MSK)) {
        
        /* Wait for packet from Protocol stack */
        ercd = rcv_mbx(pmc->obj->snd_mbx, (T_MSG **)&pkt);
        if (E_RLWAI == ercd) {
            continue;
        }
        else if (E_OK != ercd) {
            break;
        }

        ercd = EthTxFrame(pmc, pkt);
        if (E_OK != ercd) {
            #if (SNMP_ENA == 1)
            pmc->tx_discard++;
            #endif            
            pkt->ercd = ercd;
        }
        DBG_INC((E_OK == ercd), pmc, send);
        DBG_INC((E_OK != ercd), pmc, send_err);
        
        loc_tcp();
        net_buf_ret(pkt);
        ulc_tcp();
    }
    
    (void)set_flg(pmc->obj->rcv_flg, EV_EQOS_TX_EXIT);
}
