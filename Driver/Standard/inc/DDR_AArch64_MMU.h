/**
 * @brief Micro C Cube Standard, MMU and Cache management (AArch64)
 * @date  2025.10.24
 *
 * @copyright (C) 2021-2025, eForce Co., Ltd.
 */
#ifndef DDR_AARCH64_MMU_H_
#define DDR_AARCH64_MMU_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <kernel.h>

/*
 *      Memory region attributes
 */
#define MT_DEVICE_NGNRNE    0   /* Device memory : equivalent to the Strongly-ordered */
#define MT_DEVICE_NGNRE     1   /* Device memory */
#define MT_DEVICE_NGRE      2   /* Device memory */
#define MT_DEVICE_GRE       3   /* Device memory */
#define MT_NORMAL_NC        4   /* Normal memory. Non-cacheable */
#define MT_NORMAL_WT        5   /* Normal Memory, Write-through */
#define MT_NORMAL_WB        6   /* Normal Memory, Write-back */

#define MAIR_VAL    \
               ((0x00ULL << (MT_DEVICE_NGNRNE  * 8)) | \
                (0x04ULL << (MT_DEVICE_NGNRE   * 8)) | \
                (0x08ULL << (MT_DEVICE_NGRE    * 8)) | \
                (0x0cULL << (MT_DEVICE_GRE     * 8)) | \
                (0x44ULL << (MT_NORMAL_NC      * 8)) | \
                (0xBBULL << (MT_NORMAL_WT      * 8)) | \
                (0xffULL << (MT_NORMAL_WB      * 8)))

/*
 *      Access permissions
 *
 *                          Privileged permissions      User permissions
 */
#define AP_RW       (1ULL << 6)   /* Read/write                Read/write */
#define AP_RO       (3ULL << 6)   /* Read-only                 Read-only  */
#define AP_RWNA     (0ULL << 6)   /* Read/write                No access  */
#define AP_RONA     (2ULL << 6)   /* Read-only                 No access  */

/*
 *      Memory region attributes
 */
#define ATTR_UXN    (1ULL << 54)  /* Unprivileged execute never */
#define ATTR_PXN    (1ULL << 53)  /* Privileged execute-never bit */
#define ATTR_CONT   (1ULL << 52)  /* Contiguous set or entries */

#define ATTR_NG     (1ULL << 11)  /* Not global */
#define ATTR_AF     (1ULL << 10)  /* Access flag */
#define ATTR_NSH    (0ULL << 8)   /* Shareability : Non-shareable */
#define ATTR_OSH    (2ULL << 8)   /* Shareability : Outer Shareable */
#define ATTR_ISH    (3ULL << 8)   /* Shareability : Inner Shareable */
#define ATTR_NS     (1ULL << 5)   /* Non-secure */

#define ATTR_INDX(n)    (((n) & 7) << 2)  /* Attributes index */

#define ATTR_DEV    (ATTR_AF | ATTR_INDX(MT_DEVICE_NGNRNE))
#define ATTR_WT     (ATTR_AF | ATTR_ISH |  ATTR_INDX(MT_NORMAL_WT))
#define ATTR_WB     (ATTR_AF | ATTR_ISH |  ATTR_INDX(MT_NORMAL_WB))
#define ATTR_NC     (ATTR_AF | ATTR_ISH |  ATTR_INDX(MT_NORMAL_NC))

/*
 * Translation Control Register (EL1)
 */
#define TCR_TBI1        (1ULL << 38)  /* Top Byte ignored TTBR1 */
#define TCR_TBI0        (1ULL << 37)  /* Top Byte ignored TTBR0 */
#define TCR_AS          (1ULL << 36)  /* ASID Size */
                                      /* Intermediate Physical Address Size */
#define TCR_IPS_32BITS  (0ULL << 32)  /* 32 bits, 4GB */
#define TCR_IPS_36BITS  (1ULL << 32)  /* 36 bits, 64GB */
#define TCR_IPS_40BITS  (2ULL << 32)  /* 40 bits, 1TB */
#define TCR_IPS_42BITS  (3ULL << 32)  /* 42 bits, 4TB */
#define TCR_IPS_44BITS  (4ULL << 32)  /* 44 bits, 16TB */
#define TCR_IPS_48BITS  (5ULL << 32)  /* 48 bits, 256TB */
                                      /* Granule size TTBR1 */
#define TCR_TG1_4KB     (0ULL << 30)  /*      4KB */
#define TCR_TG1_64KB    (1ULL << 30)  /*      64KB */
#define TCR_TG1_16KB    (2ULL << 30)  /*      16KB */
                                      /* Shareability attribute TTBR1 */
#define TCR_SH1_NSH     (0ULL << 28)  /*      Non-shareable */
#define TCR_SH1_OSH     (2ULL << 28)  /*      Outer Shareable */
#define TCR_SH1_ISH     (3ULL << 28)  /*      Inner Shareable */
                                      /* Outer cacheability attribute TTBR1 */
#define TCR_ORGN1_NC    (0ULL << 26)  /*      Outer Non-cacheable */
#define TCR_ORGN1_WBW   (1ULL << 26)  /*      Outer Write-Back Write-Allocate Cacheable */
#define TCR_ORGN1_WT    (2ULL << 26)  /*      Outer Write-Through Cacheable */
#define TCR_ORGN1_WBNW  (3ULL << 26)  /*      Outer Write-Back no Write-Allocate Cacheable */
                                      /* Inner cacheability attribute using TTBR_1 */
#define TCR_IRGN1_NC    (0ULL << 24)  /*      Inner Non-cacheable */
#define TCR_IRGN1_WBW   (1ULL << 24)  /*      Inner Write-Back Write-Allocate Cacheable */
#define TCR_IRGN1_WT    (2ULL << 24)  /*      Inner Write-Through Cacheable */
#define TCR_IRGN1_WBNW  (3ULL << 24)  /*      Inner Write-Back no Write-Allocate Cacheable */
#define TCR_EPD1        (1ULL << 23)  /* Translation table walk disable TTBR1 */
#define TCR_A1          (1ULL << 22)  /* Selects whether TTBR0 or TTBR1 defines the ASID */
#define TCR_T1SZ(n)     (((n) & 0x3fULL) << 16)   /* Size offset TTBR1 */
                                      /* Granule size TTBR0 */
#define TCR_TG0_4KB     (0ULL << 14)  /*      4KB */
#define TCR_TG0_64KB    (1ULL << 14)  /*      64KB */
#define TCR_TG0_16KB    (2ULL << 14)  /*      16KB */
                                      /* Shareability attribute TTBR0 */
#define TCR_SH0_NSH     (0ULL << 12)  /*      Non-shareable */
#define TCR_SH0_OSH     (2ULL << 12)  /*      Outer Shareable */
#define TCR_SH0_ISH     (3ULL << 12)  /*      Inner Shareable */
                                      /* Outer cacheability attribute TTBR0 */
#define TCR_ORGN0_NC    (0ULL << 10)  /*      Outer Non-cacheable */
#define TCR_ORGN0_WBW   (1ULL << 10)  /*      Outer Write-Back Write-Allocate Cacheable */
#define TCR_ORGN0_WT    (2ULL << 10)  /*      Outer Write-Through Cacheable */
#define TCR_ORGN0_WBNW  (3ULL << 10)  /*      Outer Write-Back no Write-Allocate Cacheable */
                                      /* Inner cacheability attribute using TTBR_1 */
#define TCR_IRGN0_NC    (0ULL << 8)   /*      Inner Non-cacheable */
#define TCR_IRGN0_WBW   (1ULL << 8)   /*      Inner Write-Back Write-Allocate Cacheable */
#define TCR_IRGN0_WT    (2ULL << 8)   /*      Inner Write-Through Cacheable */
#define TCR_IRGN0_WBNW  (3ULL << 8)   /*      Inner Write-Back no Write-Allocate Cacheable */
#define TCR_EPD0        (1ULL << 7)   /* Translation table walk disable TTBR0 */
#define TCR_T0SZ(n)     ((n) & 0x3fULL)    /* Size offset TTBR0 */

/*
 * System Control Register (EL1)
 */
#define SCTLR_ICACHE    (1ULL << 12)  /* Instruction cache */
#define SCTLR_DCACHE    (1ULL << 2)   /* Data cache */
#define SCTLR_MMU_ENA   (1ULL << 0)   /* MMU enable */
#define SCTLR_SA0       (1ULL << 4)   /* Stack Aligment Check enable for EL0 */
#define SCTLR_SA        (1ULL << 3)   /* Stack Aligment Check enable */

/*
 * Memory region configuration table
 */
typedef struct {
    UD    sz;       /* size */
    UD    p_addr;   /* physical address */
    UD    v_addr;   /* virtual address */
    UD    ap;       /* access privilege */
    UD    attr;     /* attribute */
} T_MEM_CFG;

extern void _ddr_aarch64_mmu_clean_data_cache(VP ptr, SIZE sz);
extern void _ddr_aarch64_mmu_invalid_data_cache(VP ptr, SIZE sz);
extern void _ddr_aarch64_mmu_flush_data_cache(VP ptr, SIZE sz);
extern void _ddr_aarch64_mmu_invalid_inst_cache(VP ptr, SIZE sz);
extern void _ddr_aarch64_mmu_invalid_tlb(void);
extern void _ddr_aarch64_mmu_invalid_cache(void);

#ifdef __cplusplus
}
#endif

#endif  /* DDR_AARCH64_MMU_H_ */
