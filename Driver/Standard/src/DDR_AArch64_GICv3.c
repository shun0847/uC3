/**
 * @file    DDR_AArch64_GICv3.c
 * @brief   Micro C Cube Standard, DEVICE DRIVER
 *          ARM Generic Interrupt Controller v3
 * @date    2025.06.11
 * @author  Copyright (c) 2021-2025, eForce Co., Ltd. All rights reserved.
 *
 ****************************************************************************
 * @par     History
 *          - rev 1.0 (2021.02.02) Imada
 *            Created based on DDR_AArch64_GIC.c
 *            Without support of secure accesses, extended SPI range,
 *            virtual CPU interface, LPI and ITS
 *          - rev 1.1 (2021.04.28) Imada
 *            Fixed GICD IROUTER configuration
 *            Changed a GICD base address macro in init_gicr()
 *          - rev 1.2 (2021.08.24)
 *            To add ddr_aarch64_gicv3_cfg
 *          - rev 1.3 (2021.09.08)
 *            ddr_aarch64_gicv3_cfg was modified
 *          - rev 1.4 (2021.09.28)
 *            CFG_GIC_SPIN_LOCK_ID was added (Compatible with earlier releases)
 *          - rev 1.5 (2022.02.03)
 *            Added G1S, G1NS, G0 mode code.
 *            Fix GICR waker enable.
 *          - rev 1.6 (2022.06.23)
 *            Fix GICR sgi grouping.
 *            Added intinfo check in ena_int.
 *          - rev 1.7 (2024.11.19)
 *            Introduce UC3BOOT_SECONDARY and related functions for uC3+Linux
 *            with an ARMv8-A core running uC3.
 *          - rev 1.8 (2025.03.03)
 *            Fix a compilation error on (USE_SYSTEM != SYSTEM_SINGLE_CORE)
 *          - rev 2.0 (2025.03.14)
 *            GIC-600 was supported
 *            Bug fix: Not successfully enable PPI interrupts
 *          - rev 2.1 (2025.04.28)
 *            Add active interrupt clearing in ddr_aarch64_gicv3_cfg.
 *          - rev 2.2 (2025.05.21)
 *            Cortex-A76 was supported.
 *            Add support for configuring the master core.
 *            Change and introduce some internal macro names.
 *            Fix GICD_CTLR configuration.
 *            Fix condition check in wait_gicr_rwp.
 *          - rev 2.3 (2025.06.11)
 *            To add ddr_aarch64_gicv3_send_sgi_raw.
 *            Fix: Set gicr_sgi->ICFGR[0-1] to the reset values
 *                 specified in the Reference Manual.
 ****************************************************************************
 */
#include "uC3sys.h"
#include "DDR_AArch64_GICv3.h"
#include "DDR_AArch64_GICv3_sub.h"
#include "cpu_cfg.h"

#if (USE_SYSTEM != SYSTEM_SINGLE_CORE)
#if (USE_SYSTEM != SYSTEM_HETERO_CORE)
#include "DDR_AArch64_MCORE_cfg.h"
#include "uC3mcext.h"
#else
#include "uC3xcext.h"
#endif /* #if (USE_SYSTEM != SYSTEM_HETERO_CORE) */
#endif /* #if (USE_SYSTEM != SYSTEM_SINGLE_CORE) */

/* External variables --------------------------------------------------------*/

extern T_VINFTBL vinftbl[];
#if (USE_SYSTEM != SYSTEM_SINGLE_CORE)
#if !defined(CFG_GIC_SPIN_LOCK_ID)
extern UW _ddr_gic_flag;
#endif
extern volatile UD _ddr_gic_sync;
#endif /* #if (USE_SYSTEM != SYSTEM_SINGLE_CORE) */
#if defined(CFG_GIC_INIT_ICFGR_CNT) && (CFG_GIC_INIT_ICFGR_CNT > 0)
extern UW gic_cfg_icfgr[CFG_GIC_INIT_ICFGR_CNT];
#endif
#if defined(UC3BOOT_SECONDARY)
extern INT _ddr_gic_bootctx;
#endif

/* Private function prototypes -----------------------------------------------*/

static void __ena_int(INTNO intno);
static ER _ena_int(INTNO intno);
static void __dis_int(INTNO intno);
static ER _dis_int(INTNO intno);
void ddr_aarch64_gicv3_send_sgi(UINT targetlist, UW intno);
static void spurious_isr(void);
BOOL _kernel_pre_inthdr(T_INTPARA *intpara);
void _kernel_post_inthdr(UW savedt);

/* Private typedef -----------------------------------------------------------*/

/*
 * Generic Interrupt Controller
 */

/* for CPU interface register (removed)
 * CPU interface registers can be accessed by only the msr or mrs instruction.
 * No memory mapped registers provided.
 */

/* for Distributor register */
struct t_gicd {
    UW  CTLR;               /* 0x0000 Distributor Control Register */
    UW  TYPER;              /* 0x0004 Interrupt Controller Type Register */
    UW  IIDR;               /* 0x0008 Distributor Implementer Identification Register */
    UW  TYPER2;             /* 0x000C Interrupt Controller Type Register 2 */
    UW  reserved_0[4U];
#if (GIC_VER == GIC600)
    UW  FCTLR;              /* 0x0020 Function Control Register */
    UW  SAC;                /* 0x0024 Secure Access Control Register */
#else
    UW  reserved_1[2U];
#endif
    UW  reserved_2[6U];
    UW  SETSPI_NSR;         /* 0x0040 Non-secure SPI Set Register */
    UW  reserved_3;
    UW  CLRSPI_NSR;         /* 0x0048 Non-secure SPI Clear Register */
    UW  reserved_4;
    UW  SETSPI_SR;          /* 0x0050 Secure SPI Set Register */
    UW  reserved_5;
    UW  CLRSPI_SR;          /* 0x0058 Secure SPI Clear Register */
    UW  reserved_6[9U];
    UW  IGROUPR[32U];       /* 0x0080 Interrupt Group Registers */
    UW  ISENABLER[32U];     /* 0x0100 Interrupt Set-Enable Registers */
    UW  ICENABLER[32U];     /* 0x0180 Interrupt Clear-Enable Registers */
    UW  ISPENDR[32U];       /* 0x0200 Interrupt Set-Pending Registers*/
    UW  ICPENDR[32U];       /* 0x0280 Interrupt Clear-Pending Registers */
    UW  ISACTIVER[32U];     /* 0x0300 Interrupt Set-Active Registers */
    UW  ICACTIVER[32U];     /* 0x0380 Interrupt Clear-Active Registers */
    UW  IPRIORITYR[255U];   /* 0x0400 Interrupt Priority Registers */
    UW  reserved_7;
    UW  ITARGETSR[255U];    /* 0x0800 Interrupt Processor Targets Registers (not used) */
    UW  reserved_8;
    UW  ICFGR[64U];         /* 0x0C00 Interrupt Configuration Registers */
    UW  IGRPMODR[32U];      /* 0x0D00 Interrupt Group Modifier Registers */
    UW  reserved_9[32U];
    UW  NSACR[64U];         /* 0x0E00 Non-secure Access Control Registers */
    UW  reserved_10[5248U]; /* SGIR, CPENDSGIR and SPENDSGIR not used */
    UD  IROUTER[988U];      /* 0x6100 Interrupt Routing Registers (64-bit) */
};

/* for Redistributor register */
/* RD_base */
struct t_gicr_rd {
    UW  CTLR;               /* 0x0000 Redistributor Control Register */
    UW  IIDR;               /* 0x0004 Redistributor Implementation Identification Register */
    UD  TYPER;              /* 0x0008 Redistributor Type Register (64-bit) */
    UW  STATUSR;            /* 0x0010 Error Reporting Status Register (optional) */
    UW  WAKER;              /* 0x0014 Power Management Control Register */
    UW  MPAMIDR;            /* 0x0018 Report maximum PARTID and PMG Register */
    UW  PARTIDR;            /* 0x001C Set PARTID and PMG Register */
#if (GIC_VER == GIC600)
    UW  FCTLR;              /* 0x0020 Function Control Register */
    UW  PWRR;               /* 0x0024 Power Register */
    UW  CLASS;              /* 0x0028 Secure-only Register */
#else
    UW  Reserved_0[3U];
#endif
    UW  Reserved_1[5U];
    UD  SETLPIR;            /* 0x0040 Set LPI Pending Register (64-bit) */
    UD  CLRLPIR;            /* 0x0048 Clear LPI Pending Register (64-bit) */
    UW  Reserved_2[8U];
    UD  PROPBASER;          /* 0x0070 Common LPI configuration table base register (64-bit) */
    UD  PENDBASER;          /* 0x0078 LPI pending table base register (64-bit) */
    UW  Reserved_3[8U];
    UD  INVLPIR;            /* 0x00A0 Redistributor Invalidate LPI Register (64-bit) */
    UW  Reserved_4[2U];
    UD  INVALLR;            /* 0x00B0 Redistributor Invalidate All Register (64-bit) */
    UW  Reserved_5[2];
    UW  SYNCR;              /* 0x00C0 Redistributor Synchronize Register */
#if (GIC_VER == GIC500) || (GIC_VER == GIC600)
    UW  Reserved_6[16323U];
    UW  PIDR4;              /* 0xFFD0 Peripheral ID 4 Register */
    UW  PIDR5;              /* 0xFFD4 Peripheral ID 5 Register */
    UW  PIDR6;              /* 0xFFD8 Peripheral ID 6 Register */
    UW  PIDR7;              /* 0xFFDC Peripheral ID 7 Register */
    UW  PIDR0;              /* 0xFFE0 Peripheral ID 0 Register */
    UW  PIDR1;              /* 0xFFE4 Peripheral ID 1 Register */
    UW  PIDR2;              /* 0xFFE8 Peripheral ID 2 Register */
    UW  PIDR3;              /* 0xFFEC Peripheral ID 3 Register */
    UW  CIDR0;              /* 0xFFF0 Component ID 0 Register */
    UW  CIDR1;              /* 0xFFF4 Component ID 1 Register */
    UW  CIDR2;              /* 0xFFF8 Component ID 2 Register */
    UW  CIDR3;              /* 0xFFFC Component ID 3 Register */
#endif /* GIC_VER */
};

/* SGI(+PPI)_base */
struct t_gicr_sgi {
    UW  Reserved_0[32U];
    UW  IGROUPR0;           /* 0x0080 Interrupt Group Registers */
    UW  Reserved_1[31U];
    UW  ISENABLER0;         /* 0x0100 Interrupt Set-Enable Registers */
    UW  Reserved_2[31U];
    UW  ICENABLER0;         /* 0x0180 Interrupt Clear-Enable Registers */
    UW  Reserved_3[31U];
    UW  ISPENDR0;           /* 0x0200 Interrupt Set-Pending Registers */
    UW  Reserved_4[31U];
    UW  ICPENDR0;           /* 0x0280 Interrupt Clear-Pending Registers */
    UW  Reserved_5[31U];
    UW  ISACTIVER0;         /* 0x0300 Interrupt Set-Active Registers */
    UW  Reserved_6[31U];
    UW  ICACTIVER0;         /* 0x0380 Interrupt Clear-Active Registers */
    UW  Reserved_7[31U];
    UW  IPRIORITYR[8U];     /* 0x0400 Interrupt Priority Registers */
    UW  Reserved_8[504U];
    UW  ICFGR[2U];          /* 0x0C00 Interrupt Configuration Registers */
    UW  Reserved_9[62U];
    UW  IGRPMODR0;          /* 0x0D00 Interrupt Group Modifier Registers */
    UW  Reserved_10[63U];
    UW  NSACR;              /* 0x0E00 Non-secure Access Control Registers */
#if (GIC_VER == GIC500) || (GIC_VER == GIC600)
    UW  Reserved_11[11391U];
    UW  MISCSTATUSR;        /* 0xC000 Miscellaneous Status Register */
    UW  Reserved_12[31U];
    UW  PPISR;              /* 0xC080 Private Peripheral Interrupt Status Register */
#endif /* GIC_VER */
};


/* Private macro -------------------------------------------------------------*/
#ifndef G1NS
#define G1NS   G1
#define G1S    2U
#endif

#define GICv3   1U
#define GICv4   2U
#define GIC500  3U
#define GIC600  4U

#if (USE_CPU == CPU_CORTEX_A53) || (USE_CPU == CPU_CORTEX_A72)
    #define GIC_VER     (GIC500)
#elif (USE_CPU == CPU_CORTEX_A55) || (USE_CPU == CPU_CORTEX_A76)
    #define GIC_VER     (GIC600)
#else
    #error  "Unsupported CPU"
#endif

#if (CLUSTER_NUMBER > 1U)
    #error "CLUSTER_NUMBER is must be less than 2 (cpu_cfg.h)"
#endif

#if (USE_SYSTEM == SYSTEM_SINGLE_CORE)
    #define SPIN_LOCK()
    #define SPIN_UNLOCK()
#else /* #if (USE_SYSTEM == SYSTEM_SINGLE_CORE) */
    #define SPIN_LOCK()     (_kernel_spin_lock(&_ddr_gic_flag))
    #define SPIN_UNLOCK()   ((void)_kernel_spin_unlock(&_ddr_gic_flag))
#endif /* #if (USE_SYSTEM == SYSTEM_SINGLE_CORE) */

#define GIC_SPINUM_MAX      (CFG_GIC_INTNUM_MAX - 32U)

#define GIC_IROUTER_INIT    (0x00000000000000FFULL) /* All the interrupts are configured to be delivered to the last CPU core number at initialization */

#define GIC_MPIDR_AFF_MASK  (0x000000FF00FFFFFFULL) /* Mask for Aff1-3 */

#define GICD_RWP_MASK       (0x80000000U) /* bit[31]: RWP in GICD_CTLR */
#define GICR_RWP_MASK       (0x00000008U) /* bit[3] : RWP in GICR_CTLR */

#define GICR_WAK_CHILD_ASLP (0x00000004U) /* bit[2] : ChildrenAsleep */
#define GICR_WAK_PRC_ASLP   (0x00000002U) /* bit[1] : ProcessorAsleep */

#define GICR_PWRR_RDGPO     (0x00000008U) /* bit[3] : RDGroupPoweredOff */
#define GICR_PWRR_RDAG      (0x00000002U) /* bit[1] : RDApplyGroup */
#define GICR_PWRR_RDPD      (0x00000001U) /* bit[0] : RDPowerDown */

#if (CPU_ACCESS_MODE == NS)
#define GICD_CTLR_ARE       (0x00000010U) /* bit[4] : ARE_NS only */
#define GICD_CTLR_ENGRP1A   (0x00000002U) /* bit[1] : EnableGrp1A */
#elif (CPU_ACCESS_MODE == S)
#define GICD_CTLR_ARE       (0x00000030U) /* bit[4-5] : ARE_NS and ARE_S */
#define GICD_CTLR_ENGRP1S   (0x00000004U) /* bit[2] : EnableGrp1S  */
#define GICD_CTLR_ENGRP1NS  (0x00000002U) /* bit[1] : EnableGrp1NS */
#define GICD_CTLR_ENGRP0    (0x00000001U) /* bit[0] : EnableGrp0   */
#else
#error "Invalid CPU_ACCESS_MODE"
#endif

#define GICR_CTLR_DPG1S     ((UINT)0x1U << 26)  /* bit[26] : DPG1S  */
#define GICR_CTLR_DPG1NS    ((UINT)0x1U << 25)  /* bit[25] : DPG1NS */
#define GICR_CTLR_DPG0      ((UINT)0x1U << 24)  /* bit[24] : DPG0   */

/* MPIDR_EL1 bit assignments */
#define MPIDR_AFF0_SHIFT    (0)     /* bit[0-7]   : Aff0    */
#define MPIDR_AFF1_SHIFT    (8)     /* bit[8-15]  : Aff1    */
#define MPIDR_AFF2_SHIFT    (16)    /* bit[16-23] : Aff2    */
#define MPIDR_MT_SHIFT      (24)    /* bit[24]    : MT      */
#define MPIDR_U_SHIFT       (30)    /* bit[30]    : U       */
#define MPIDR_AFF3_SHIFT    (32)    /* bit[32-39] : Aff3    */

/* ICC_SGI{0,1}R_EL1 bit assignments */
#define ICC_SGIR_TL_SHIFT    (0)     /* bit[0-15]  : TargetList */
#define ICC_SGIR_AFF1_SHIFT  (16)    /* bit[16-23] : Aff1    */
#define ICC_SGIR_INTID_SHIFT (24)    /* bit[24-27] : INTID   */
#define ICC_SGIR_AFF2_SHIFT  (32)    /* bit[32-39] : Aff2    */
#define ICC_SGIR_IRM_SHIFT   (40)    /* bit[40]    : IRM     */
#define ICC_SGIR_RS_SHIFT    (44)    /* bit[44-47] : RS      */
#define ICC_SGIR_AFF3_SHIFT  (48)    /* bit[48-55] : AFF3    */

#define GICR_CPUBITS_MAX    (28U)   /* 18 + ceil(1,log2(1024)) */
#if (CFG_GIC_NUM_BITS >> GICR_CPUBITS_MAX)
    #error "CFG_GIC_NUM_BITS (DDR_AArch64_MCORE_cfg.h)"
#endif

#if (USE_SYSTEM != SYSTEM_SINGLE_CORE)
// Prioritize the definition of sync ID
#if defined(CFG_GIC_SYNC_ID)
    #define GIC_SYNC_WORD  CFG_GIC_SYNC_ID
#else
#if (CFG_GIC_NUM_CPUS == 1U)
    #define GIC_SYNC_WORD  0x00000002U
#elif (CFG_GIC_NUM_CPUS == 2U)
    #define GIC_SYNC_WORD  0x00000202U
#elif (CFG_GIC_NUM_CPUS == 3U)
    #define GIC_SYNC_WORD  0x00020202U
#elif (CFG_GIC_NUM_CPUS == 4U)
    #define GIC_SYNC_WORD  0x02020202U
#else
    #error  "Unsupported CFG_GIC_NUM_CPUS"
#endif
#endif

// check for master core configuration
#if !defined(MASTER_CORE_ID)
#define MASTER_CORE_ID ID_CORE0
#endif
#endif /* #if (USE_SYSTEM != SYSTEM_SINGLE_CORE) */

/* Private variables ---------------------------------------------------------*/

static UD cluster_path = 0x0000000000000000ULL;

static volatile struct t_gicd       *g_gicd = 0x0;
static volatile struct t_gicr_rd    *g_gicr_rd[CFG_GIC_NUM_CPUS] = {0x0};
static volatile struct t_gicr_sgi   *g_gicr_sgi[CFG_GIC_NUM_CPUS] = {0x0};
#define REG_ICD         (*g_gicd)
#define REG_ICR_RD(n)   (*g_gicr_rd[(n)])
#define REG_ICR_SGI(n)  (*g_gicr_sgi[(n)])
#if defined(CFG_GIC_SPIN_LOCK_ID)
static UW _ddr_gic_flag = CFG_GIC_SPIN_LOCK_ID;
#endif

static inline UINT curr_cpu(void)
{
    return (vget_cid() - (UINT)ID_CORE0);
}

/*
 * Busy loop until register writes complete for GICD.
 * From the GICv3v4 manual, the RWP bit tracks writes to:
 *   GICD_CTLR[2:0], the Group Enables, for transitions from 1 to 0 only.
 *   GICD_CTLR[7:4], the ARE bits, E1NWF bit and DS bit.
 *   GICD_ICENABLER<n>.
 */
static void wait_gicd_rwp(void)
{
    while ((REG_ICD.CTLR & GICD_RWP_MASK));

    return;
}

/*
 * Only the master core is responsible for initializing SPI-related registers(GICD)
 */
static void init_master_core(void)
{
    UW i;
#if defined(CFG_GIC_G1NS_ARRAY) || defined(CFG_GIC_G1NS_ARRAY)
    UW j, k;
#endif
    volatile UW *reg = 0x0;
    volatile UD *reg64 = 0x0;

    REG_ICD.CTLR = 0x0U;    /* Disable GICD */
    wait_gicd_rwp();

    /* Distributor (GICD) initialization */

    /* ARE setting, a value for this varies depending on the access mode */
    REG_ICD.CTLR = GICD_CTLR_ARE;
    wait_gicd_rwp();

    /* Group 0 or 1 for all */
    reg = &REG_ICD.IGROUPR[1]; /* REG_ICD.IGROUPR[0] not used */
    for (i = 0U; i < GIC_SPINUM_MAX; i += 32U)
#if (CFG_GIC_INT_GRP == G0) || (CFG_GIC_INT_GRP == G1S)
        *reg++ = 0x00000000U;
#elif (CFG_GIC_INT_GRP == G1NS)
        *reg++ = 0xFFFFFFFFU;
#else
#error "Invalid CFG_GIC_INT_GRP"
#endif
    reg = &REG_ICD.IGRPMODR[1]; /* REG_ICD.IGRPMODR[0] not used */
    for (i = 0U; i < GIC_SPINUM_MAX; i += 32U)
#if (CFG_GIC_INT_GRP == G0) || (CFG_GIC_INT_GRP == G1NS)
        *reg++ = 0x00000000U;
#elif (CFG_GIC_INT_GRP == G1S)
        *reg++ = 0xFFFFFFFFU;
#else
#error "Invalid CFG_GIC_INT_GRP"
#endif

#if (CFG_GIC_INT_GRP == G1S)
#if  (CFG_GIC_USE_G0 == 1U)
#ifdef CFG_GIC_G0_ARRAY
    k = 0;
    reg = &REG_ICD.IGRPMODR[1]; /* REG_ICD.IGRPMODR[0] not used */
    for (i = 0U; i < GIC_SPINUM_MAX; i += 32U) {
        for (j = 0U; j < 32; j++) {
            if (CFG_GIC_G0_ARRAY[k] == ((i + j) + 32U)) {
                *reg &= ~(1 << j);
                k++;
            }
        }
        reg++;
    }
#endif
#endif
#if  (CFG_GIC_USE_G1NS == 1U)
#ifdef CFG_GIC_G1NS_ARRAY
    k = 0;
    reg = &REG_ICD.IGROUPR[1]; /* REG_ICD.IGROUPR[0] not used */
    for (i = 0U; i < GIC_SPINUM_MAX; i += 32U) {
        for (j = 0U; j < 32; j++) {
            if (CFG_GIC_G1NS_ARRAY[k] == ((i + j) + 32U)) {
                *reg |= (1 << j);
                k++;
            }
        }
        reg++;
    }
#endif
#endif
#endif

    /* Disable all interrupts */
    reg = &REG_ICD.ICENABLER[1]; /* REG_ICD.ICENABLER[0] not used */
    for (i = 0U; i < GIC_SPINUM_MAX; i += 32U) {
        *reg++ = 0xFFFFFFFFU;
        wait_gicd_rwp();
    }

    /* Clear pending all */
    reg = &REG_ICD.ICPENDR[1]; /* REG_ICD.ICPENDR[0] not used */
    for (i = 0U; i < GIC_SPINUM_MAX; i += 32U)
        *reg++ = 0xFFFFFFFFU;

    /* Set triggering mode */
    /* Level or edge configuration */
    reg = &REG_ICD.ICFGR[2];    /* REG_ICD.ICFGR[0..1] not used */
#if defined(CFG_GIC_INIT_ICFGR_CNT) && (CFG_GIC_INIT_ICFGR_CNT > 0)
    for (i = 0; i < CFG_GIC_INIT_ICFGR_CNT; i++) {
        reg[i] = gic_cfg_icfgr[i];
    }
#else
    for (i = 0U; i < GIC_SPINUM_MAX; i += 16U)
        *reg++ = 0x00000000U;    /* level-sensitive */
#endif

    /* Set priority */
    reg = &REG_ICD.IPRIORITYR[8]; /* REG_ICD.IPRIORITYR[0-7] not used */
    for (i = 0U; i < GIC_SPINUM_MAX; i += 4U)
        *reg++ = 0xFFFFFFFFU; /* 0xF0 would be set for NS, 0xF8 for S */

    /* Clear Target CPU all for SPIs */
    reg64 = &REG_ICD.IROUTER[0];
    for (i = 0U; i < GIC_SPINUM_MAX; i++)
        /*
         * TODO:
         * We assume that a target processor does not have 256 or more
         * processor cores per 1 processor cluster
         */
        *reg64++ = GIC_IROUTER_INIT;

#if (CPU_ACCESS_MODE == NS)
    REG_ICD.CTLR = (GICD_CTLR_ARE | GICD_CTLR_ENGRP1A); /* Set EnableGrp1A */
#elif (CPU_ACCESS_MODE == S)
    REG_ICD.CTLR = (GICD_CTLR_ARE | GICD_CTLR_ENGRP1S); /* Set EnableGrp1S */
#if (CFG_GIC_USE_G0 == 1U)
    REG_ICD.CTLR |= GICD_CTLR_ENGRP0;   /* Set EnableGrp0 */
#endif
#if (CFG_GIC_USE_G1NS == 1U)
    REG_ICD.CTLR |= GICD_CTLR_ENGRP1NS; /* Set EnableGrp1NS */
#endif
#else
#error "Invalid CPU_ACCESS_MODE"
#endif
    wait_gicd_rwp();

    vdsb();

    return;
}

/**
 * System-aware master core initialization
 */
static void system_init_master_core(void)
{
#if (USE_SYSTEM == SYSTEM_SINGLE_CORE)
    init_master_core();
#else /* #if (USE_SYSTEM == SYSTEM_SINGLE_CORE) */
    if (vget_cid() == MASTER_CORE_ID) {
        init_master_core();
    } else {
        ; /* Do nothing on the other cores */
    }
#endif /* #if (USE_SYSTEM == SYSTEM_SINGLE_CORE) */

    return;
}

/*
 * Busy loop until register writes complete for GICR.
 * From the GICv3v4 manual, the RWP bit tracks writes to:
 *   GICR_ICENABLER0
 *   GICR_CTLR.DPG1S
 *   GICR_CTLR.DPG1NS
 *   GICR_CTLR.DPG0
 *   GICR_CTLR, which clears EnableLPIs from 1 to 0.
 *   In GICv4.1, GICR_VPROPBASER, which clears Valid
 */
static void wait_gicr_rwp(UINT cpu)
{
    while ((REG_ICR_RD(cpu).CTLR & GICR_RWP_MASK));

    return;
}

/*
 * GIC Redistributor(GICR) register address
 */
static ER init_gicr_reg(UINT cpu) {
    volatile UD addr;

    /* GICR RD base */
#if (GIC_VER == GIC600)
    addr = CFG_GIC_DIST_BASE + ((4UL + (2UL * CFG_ITS_CNT) + (cpu * 2UL)) << 16);
#else
    addr = CFG_GIC_DIST_BASE | ((0x1ULL << (CFG_GIC_NUM_BITS - 1U)) | ((UD)cpu << 17));
#endif
    g_gicr_rd[cpu] = (struct t_gicr_rd *)addr;

    /* GICR SGI base */
#if (GIC_VER == GIC600)
    addr = addr + (0x1ULL << 16);
#else
    addr |= (0x1ULL << 16);    /* 64KB offset */
#endif
    g_gicr_sgi[cpu] = (struct t_gicr_sgi *)addr;

    return E_OK;
}

/*
 * GIC Redistributor(GICR) must be configured on each core
 */
static ER init_gicr(UINT cpu) {
    ER ercd;
    UINT tmp;

    ercd = init_gicr_reg(cpu);
    if (ercd != E_OK) {
        return ercd;
    }

    tmp = REG_ICR_RD(cpu).WAKER;
    REG_ICR_RD(cpu).WAKER = (tmp & ~(0x2U));  /* Clear bit[1] in GICR RD_base WAKER */

    /* Make sure that GICR_TYPER.Processor_Number < 8 */
    tmp = (UINT)((REG_ICR_RD(cpu).TYPER & 0xFFFF00ULL) >> 8);
    if ((tmp > 8U) || (tmp != cpu)) {
        return E_PAR;
    }

#if (CPU_ACCESS_MODE == NS)
    REG_ICR_RD(cpu).CTLR = GICR_CTLR_DPG1NS;  /* Set DPG1NS */
#elif (CPU_ACCESS_MODE == S)
    REG_ICR_RD(cpu).CTLR = GICR_CTLR_DPG1S;  /* Set DPG1S */
#if  (CFG_GIC_USE_G0 == 1U)
    REG_ICR_RD(cpu).CTLR |= GICR_CTLR_DPG0; /* Set DPG0 */
#endif
#if (CFG_GIC_USE_G1NS == 1U)
    REG_ICR_RD(cpu).CTLR |= GICR_CTLR_DPG1NS; /* Set DPG1NS */
#endif
#else
#error "Invalid CPU_ACCESS_MODE"
#endif
    wait_gicr_rwp(cpu);

    vdsb();

    return 0;
}

/*
 * GICD CTLR configuration
 */
static void gicd_ctl_cfg(volatile struct t_gicd *gicd_base, UINT cpuno)
{
    /* GICD_CTLR_ARE varies depending on the current access mode */

#if (USE_SYSTEM == SYSTEM_SINGLE_CORE)
    (void)cpuno;
    gicd_base->CTLR = GICD_CTLR_ARE;
#else
    UW i;
    if (cpuno == (ID_CORE0 - 1U)) {
        gicd_base->CTLR = GICD_CTLR_ARE;
    } else {
        do {
            i = gicd_base->CTLR & GICD_CTLR_ARE;
            if (i == GICD_CTLR_ARE) {
                break;
            }
        } while (1);
    }
#endif

    return;
}

/*
 * GICD IGROUPR configuration
 */
static void gicd_igroup_cfg(volatile struct t_gicd *gicd_base, UINT cpuno)
{
    UINT i;

#if (USE_SYSTEM != SYSTEM_SINGLE_CORE)
    if (cpuno != (ID_CORE0 - 1U)) {
        return;
    }
#else
    (void)cpuno;
#endif

    for (i = 1U; i < 32U; i++) {
#if (CFG_GIC_INT_GRP == G0) || (CFG_GIC_INT_GRP == G1S)
        gicd_base->IGROUPR[i] = 0x00000000U;
#elif (CFG_GIC_INT_GRP == G1NS)
        gicd_base->IGROUPR[i] = 0xFFFFFFFFU;
#else
#error "Invalid CFG_GIC_INT_GRP"
#endif
    }
    for (i = 1U; i < 32U; i++) {
#if (CFG_GIC_INT_GRP == G0) || (CFG_GIC_INT_GRP == G1NS)
        gicd_base->IGRPMODR[i] = 0x00000000U;
#elif (CFG_GIC_INT_GRP == G1S)
        gicd_base->IGRPMODR[i] = 0xFFFFFFFFU;
#else
#error "Invalid CFG_GIC_INT_GRP"
#endif
    }

    return;
}

ER ddr_aarch64_gicv3_cfg(void)
{
    INT el;
    UINT cpu;
    volatile struct t_gicd *gicd = 0x0;
    volatile struct t_gicr_rd *gicr_rd = 0x0;
    volatile struct t_gicr_sgi   *gicr_sgi = 0x0;
    volatile UD addr;
    UW i;

    /* Exception level, CPU ID */
    el = (INT)(get_current_el() & 0x0000FFFFU);
    cpu = curr_cpu();

    /* Register address */
    gicd = (volatile struct t_gicd *)CFG_GIC_DIST_BASE;
    /* GICR RD base */
#if (GIC_VER == GIC600)
    addr = CFG_GIC_DIST_BASE + ((4UL + (2UL * CFG_ITS_CNT) + (cpu * 2UL)) << 16);
#else
    addr = CFG_GIC_DIST_BASE | ((0x1ULL << (CFG_GIC_NUM_BITS - 1U)) | ((UD)cpu << 17));
#endif
    gicr_rd = (struct t_gicr_rd *)addr;

    /* GICR SGI base */
#if (GIC_VER == GIC600)
    addr = addr + (0x1ULL << 16);
#else
    addr |= (0x1ULL << 16);    /* 64KB offset */
#endif
    gicr_sgi = (struct t_gicr_sgi *)addr;

    seticc_sre(0x0FU, el);      /* ICC_SRE_EL3 System Register enable, IRQ/FIQ bypass disable
                                   Lower exception level access enable */

#if defined(UC3BOOT_SECONDARY)
    if (_ddr_gic_bootctx == UBOOT) {
        gicd_ctl_cfg(gicd, cpu);
    }
#else
    gicd_ctl_cfg(gicd, cpu);
#endif /* #if defined(UC3BOOT_SECONDARY) */

    /* Wakeup GICR(GIC Redistributor) */
#if (GIC_VER == GIC600)
    gicr_rd->PWRR = GICR_PWRR_RDAG;
    while ((gicr_rd->PWRR & GICR_PWRR_RDGPO) != 0x00);
#endif
    gicr_rd->WAKER &= ~GICR_WAK_PRC_ASLP;
    while ((gicr_rd->WAKER & GICR_WAK_CHILD_ASLP) != 0x00);

#if (CFG_GIC_INT_GRP == G0) || (CFG_GIC_INT_GRP == G1S)
    gicr_sgi->IGROUPR0 = 0x00000000U;
#elif (CFG_GIC_INT_GRP == G1NS)
    gicr_sgi->IGROUPR0 = 0xFFFFFFFFU;
#else
#error "Invalid CPU_ACCESS_MODE"
#endif

#if (CFG_GIC_INT_GRP == G0) || (CFG_GIC_INT_GRP == G1NS)
    gicr_sgi->IGRPMODR0 = 0x00000000U;
#elif (CFG_GIC_INT_GRP == G1S)
    gicr_sgi->IGRPMODR0 = 0xFFFFFFFFU;
#else
#error "Invalid CPU_ACCESS_MODE"
#endif

#if defined(UC3BOOT_SECONDARY)
    if (_ddr_gic_bootctx == UBOOT) {
        gicd_igroup_cfg(gicd, cpu);
    }
#else
    gicd_igroup_cfg(gicd, cpu);
#endif /* #if defined(UC3BOOT_SECONDARY) */

    vdsb();

    /*
     * SGIs and PPIs
     */
    /* Group0 or Secure Group1 or Non-Secure Group1  */
#if (CFG_GIC_INT_GRP == G0) || (CFG_GIC_INT_GRP == G1S)
    gicr_sgi->IGROUPR0 = 0x00000000U;
#elif (CFG_GIC_INT_GRP == G1NS)
    gicr_sgi->IGROUPR0 = 0xFFFFFFFFU;
#else
#error "Invalid CPU_ACCESS_MODE"
#endif

#if (CFG_GIC_INT_GRP == G0) || (CFG_GIC_INT_GRP == G1NS)
    gicr_sgi->IGRPMODR0 = 0x00000000U;
#elif (CFG_GIC_INT_GRP == G1S)
    gicr_sgi->IGRPMODR0 = 0xFFFFFFFFU;
#else
#error "Invalid CPU_ACCESS_MODE"
#endif

    /* Disable interrupts */
    gicr_sgi->ICENABLER0 = 0xFFFFFFFFU;
    while ((gicd->CTLR & GICD_RWP_MASK));

    /* Clear pending */
    gicr_sgi->ICPENDR0 = 0xFFFFFFFFU;

    /* Clear active interrupts */
    gicr_sgi->ICACTIVER0 = 0xFFFFFFFFU;

    /* Set triggering mode */
    gicr_sgi->ICFGR[0] = 0xAAAAAAAAU; /* SGIs are always edge-triggered.*/
    gicr_sgi->ICFGR[1] = 0x00000000U;

    /* Set priority */
    for (i = 0U; i < 8U; i++) {
        gicr_sgi->IPRIORITYR[i] = 0xFFFFFFFFU; /* 0xF0 would be set for NS, 0xF8 for S */
    }
    vdsb();

    return E_OK;
}

/**
 * Initialize GIC
 */
ER ddr_aarch64_gicv3_init(void)
{
    ER ret = E_OK;
    UD mpidr = 0x0ULL;
    INT el;

    /* Obtain the cluster id path value for SGI */
    mpidr = (get_mpidr() & GIC_MPIDR_AFF_MASK);
    cluster_path = (
            (((mpidr >> MPIDR_AFF3_SHIFT) & 0xFFU) << ICC_SGIR_AFF3_SHIFT) | /* Aff3 */
            (((mpidr >> MPIDR_AFF2_SHIFT) & 0xFFU) << ICC_SGIR_AFF2_SHIFT) | /* Aff2 */
            (((mpidr >> MPIDR_AFF1_SHIFT) & 0xFFU) << ICC_SGIR_AFF1_SHIFT)   /* Aff1 */
        );

    /* Exception level */
    el = (INT)(get_current_el() & 0x0000FFFFU);

    /*
     * Distributor init
     */
    g_gicd = (volatile struct t_gicd *)CFG_GIC_DIST_BASE;
#if defined(UC3BOOT_SECONDARY)
    if (_ddr_gic_bootctx == UBOOT) {
        system_init_master_core();
    }
#else
    system_init_master_core();
#endif /* #if defined(UC3BOOT_SECONDARY) */

    /*
     * Redistributor init
     */
    ret = init_gicr(curr_cpu());
    if (ret) {
        return ret;
    }

    /*
     * CPU interface init
     */

    /* Enable CPU Interface register accesses */
    seticc_sre(0x7ULL, el);

    /* Enable or disable interrupts coupled with a specific group */
#if (CFG_GIC_INT_GRP == G0)
    seticc_igrpen0(0x1ULL);
#elif (CFG_GIC_INT_GRP == G1NS) || (CFG_GIC_INT_GRP == G1S)
#if (CFG_GIC_USE_G0 == 1U)
    seticc_igrpen0(0x1ULL);
#endif
    seticc_igrpen1(0x1ULL, el);
#else
#error "Invalid CFG_GIC_INT_GRP"
#endif

    /* Interrupt priority mask */
    seticc_pmr(CFG_GIC_PRI_MASK);

    /* Interrupt binary point */
    seticc_bpr(CFG_GIC_BIN_POINT);

    /* Clear pending if there is any interrupts */
    do {
        UD intno;
        /* We use only bit[0-15] of ICC_IAR{0,1}_EL1 and ICC_EOIR{0,1}_EL1.
         * It would be enough because GIC-600 supports at most 56K interrupts
         */
        intno = geticc_iar() & 0xFFFFU;
        if (intno == 1023U) {
            break;
        }
        seticc_eoir(intno);
    } while (1);

    /* Enable interrupt */
    seticc_ctlr((0x1ULL << 6), el); /* PMHE enabled */

    vdsb();

    /* Make sure that all the CPU cores are ready to use GIC (synchronization) */
#if (USE_SYSTEM != SYSTEM_SINGLE_CORE)
    SPIN_LOCK();
    _ddr_gic_sync |= (2ULL << (curr_cpu() * 8U));
    vdsb();
    SPIN_UNLOCK();

    while (_ddr_gic_sync != GIC_SYNC_WORD) {
    }
#endif /* #if (USE_SYSTEM != SYSTEM_SINGLE_CORE) */

    return ret;
}

/**
 * Set interrupt detection type
 * @param intno     interrupt number
 * @param type      Interrupt types
 *                  GIC_IRQ_LEVEL_HIGH  - interrupt is level-sensitive.
 *                  GIC_IRQ_EDGE_RISING - interrupt is edge-triggered.
 * @return E_OK     trigger mode set
 *         E_PAR    invalid interrupt number
 */
ER ddr_aarch64_gicv3_int_type(INTNO intno, ddr_aarch64_gic_int_type_t type)
{
    ER ercd;

    if (CFG_GIC_INTNUM_MAX <= intno) {
        ercd = E_PAR;
    } else {
        UW val;
        volatile UW *reg = 0;

        _kernel_lock();

        SPIN_LOCK();
        if (intno < 32U) {
            reg = &REG_ICR_SGI(curr_cpu()).ICFGR[(intno >> 4)];
        } else {
            reg = &REG_ICD.ICFGR[(intno >> 4)];
        }
        val = *reg;
        switch (type) {
            case GIC_IRQ_LEVEL_HIGH:
                val &= ~(0x2U << ((intno & 0x0fU) * 2U));
                break;
            case GIC_IRQ_EDGE_RISING:
                val |= (0x2U << ((intno & 0x0fU) * 2U));
                break;
            default:
                break;

        }
        *reg = val;
        SPIN_UNLOCK();
        _kernel_unlock();
        ercd = E_OK;
    }

    return ercd;
}

static void __ena_int(INTNO intno)
{
    UINT pri;
    volatile UW *reg = 0;
    UW val;
    UW field_mask;

    pri = vinftbl[intno].intinfo & 0xffU;
    if (intno < 32U) {
        reg = &REG_ICR_SGI(curr_cpu()).IPRIORITYR[(intno >> 2)];
    } else {
        reg = &REG_ICD.IPRIORITYR[(intno >> 2)];
    }
    field_mask = (0xffU << ((intno & 3U) * 8U));
    pri = (pri << ((intno & 3U) * 8U));
    val = *reg;
    if ((val & field_mask) != pri) {
        val &= ~field_mask;
        *reg = val | pri;
    }
    if (intno < 32U) {
        REG_ICR_SGI(curr_cpu()).ISENABLER0 = (1U << (intno & 0x1fU));
    } else {
        REG_ICD.ISENABLER[(intno >> 5)] = (1U << (intno & 0x1fU));
    }

    return;
}

static ER _ena_int(INTNO intno)
{
    ER ercd = E_OK;

    if (vinftbl[intno].intinfo != 0U) {

        if (intno >= 32U) {
            /*
             * For SPIs, add the core that handles interrupt.
             */
            volatile UD *reg = 0x0ULL;
            UD val;

            /* Check if or not the target interrupt number has already been enabled */
            val = REG_ICD.ISENABLER[(intno >> 5)];
            val = ((val >> (intno & 0x1fU)) & 0x1U);
            if (val) { /* Enabled */
                ercd = E_PAR;
            } else { /* Disabled */
                reg = &REG_ICD.IROUTER[intno - 32];
                *reg = (get_mpidr() & GIC_MPIDR_AFF_MASK);
            }
        }

        if (ercd == E_OK) {
            __ena_int(intno);
        }

        vdsb();
        return E_OK;
    } else {
        return E_OBJ;
    }
}

/**
 * Enable interrupt
 * @param intno     interrupt number
 * @return E_OK or error code
 */
ER ena_int(INTNO intno)
{
    ER ercd;

    if (intno < CFG_GIC_INTNUM_MAX) {
        _kernel_lock();
        if (intno >= 32U) {
            SPIN_LOCK();
            ercd = _ena_int(intno);
            SPIN_UNLOCK();
        } else {
            ercd = _ena_int(intno);
        }
        _kernel_unlock();
    } else {
        ercd = E_PAR;
    }

    return ercd;
}

static void __dis_int(INTNO intno)
{
    if (intno < 32U) {
        REG_ICR_SGI(curr_cpu()).ICENABLER0 = (1U << (intno & 0x1fU));
    } else {
        REG_ICD.ICENABLER[(intno >> 5)] = (1U << (intno & 0x1fU));
    }
    wait_gicd_rwp();

    return;
}

static ER _dis_int(INTNO intno)
{
    ER ercd = E_OK;

    if (intno >= 32U) {
        volatile UD *reg = 0x0ULL;
        UD val = 0x0ULL;

        /*
         * For SPIs, set the initial value.
         * Check if or not the target interrupt number has already been
         * disabled at first.
         */
        val = REG_ICD.ISENABLER[(intno >> 5)];
        val = ((val >> (intno & 0x1fU)) & 0x1U);
        if (val) { /* Enabled */
            reg = &REG_ICD.IROUTER[intno - 32];
            *reg = GIC_IROUTER_INIT;
        } else { /* Disabled */
            ercd = E_PAR;
        }
    }

    if (ercd == E_OK) {
        __dis_int(intno);
    }

    vdsb();

    return E_OK;
}

/**
 * Disable interrupt
 * @param intno     interrupt number
 * @return E_OK or error code
 */
ER dis_int(INTNO intno)
{
    ER ercd;

    if (intno < CFG_GIC_INTNUM_MAX) {
        _kernel_lock();
        if (intno >= 32U) {
            SPIN_LOCK();
            ercd = _dis_int(intno);
            SPIN_UNLOCK();
        } else {
            ercd = _dis_int(intno);
        }
        _kernel_unlock();
    } else {
        ercd = E_PAR;
    }

    return ercd;
}

/**
 * Send SGI to cores specified by the target core list
 * @param targetlist   Destination target core list (bitmask based on vget_cid())
 * @param intno        SGI number
 */
void ddr_aarch64_gicv3_send_sgi(UINT targetlist, UW intno)
{
    UD val;

    /* targetlist expects a bitmask pattern having (0x1U << ID_CORE0) | ... | (0x1U << ID_COREn).
     * Therefore, it must be 1-bit right shifted because ID_CORE1 = 0x1 */
    val = (cluster_path | ((UD)intno << 24) | (UD)(targetlist >> 1));
    seticc_sgir(val);

    return;
}

/**
 * Send SGI to cores specified by the ddr_aarch64_gic_sgir_t parameter
 */
void ddr_aarch64_gicv3_send_sgi_raw(const ddr_aarch64_gic_sgir_t *sgir)
{
    UD val = 0ULL;

    /* Construct ICC_SGI1R_EL1 value according to its field definitions */
    val |= ((UD)sgir->aff3 << ICC_SGIR_AFF3_SHIFT);
    val |= (((UD)sgir->rs & 0xFULL) << ICC_SGIR_RS_SHIFT); /* RS: only 4-bit */
    val |= (((UD)sgir->irm & 0x1ULL) << ICC_SGIR_IRM_SHIFT); /* IRM: only 1-bit */
    val |= ((UD)sgir->aff2 << ICC_SGIR_AFF2_SHIFT);
    val |= (((UD)sgir->intid & 0xFULL) << ICC_SGIR_INTID_SHIFT); /* INTID: only 4-bit */
    val |= ((UD)sgir->aff1 << ICC_SGIR_AFF1_SHIFT);
    val |= (UD)sgir->targetlist;

    seticc_sgir(val);

    return;
}


/* Kernel internal functions ------------------------- */

static void spurious_isr(void)
{
    return;
}

/**
 * Read the occurred interrupt information
 * @param intpara   interrupt information
 * @return TRUE or FALSE
 */
BOOL _kernel_pre_inthdr(T_INTPARA *intpara)
{
    BOOL retcd;
    UINT intno;
    UW pri_now;

    /* for reference of software if it will use at future */
    pri_now = (UW)geticc_rpr() & 0xFFU;

    /* We use only bit[0-15] of ICC_IAR{0,1}_EL1.
     * It would be enough because GIC-600 supports at most 56K interrupts
     */
    *intpara->savedt = (UW)(geticc_iar() & 0xFFFFULL) | (pri_now << 16);
    intno = *intpara->savedt & 0xFFFFU;

    if (intno < CFG_GIC_INTNUM_MAX) {
        intpara->intinfo = vinftbl[intno].intinfo;
        intpara->intfunc = vinftbl[intno].intfunc;
        intpara->next = vinftbl[intno].next;
        retcd = TRUE;
    } else {
        intpara->intfunc = &spurious_isr;
        retcd = FALSE;
    }

    return retcd;
}

/**
 * End the interrupt handler
 * @param savedt    interrupt information
 */
void _kernel_post_inthdr(UW savedt)
{
    /* We use only bit[0-15] of ICC_EOIR{0,1}_EL1.
     * It would be enough because GIC-600 supports at most 56K interrupts
     */
    seticc_eoir((UD)savedt & 0xFFFFULL);

    return;
}

#if defined(UC3BOOT_SECONDARY)
/**
 * Boot context setting
 * @param ctx    boot context (uC3 boot context) to be set
 */
void ddr_aarch64_gicv3_set_bootctx(ddr_aarch64_gic_bootctx_type_t ctx)
{
    _ddr_gic_bootctx = ctx;

    return;
}
#endif /* #if defined(UC3BOOT_SECONDARY) */
