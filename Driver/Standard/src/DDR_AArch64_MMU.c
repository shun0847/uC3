/**
 * @brief Micro C Cube Standard, MMU and Cache management (AArch64)
 * @date  2025.10.24
 *
 * @copyright (C) 2021-2025, eForce Co., Ltd.
 */
#include <cpu_cfg.h>
#include <DDR_AArch64_MMU.h>

/* External function prototype ----------------------------------------------*/

extern void mmu_enable(UD ttbr0, UD ttbr1, UD tcr, UD mair, UD sctlr);
extern void dc_cvac(UD addr);
extern void dc_ivac(UD addr);
extern void dc_civac(UD addr);
extern void ic_iallu(void);
extern void ic_ivau(UD addr);
extern void tlbi_alle1is(void);
extern void invalid_dcache_all(void);

/* Private function prototype -----------------------------------------------*/

static UD * create_table(
    UD * upper_entry,
    UD * ttb,
    UD sz);
static void setup_l1_entry(
    UD * l1_tbl,
    UD p_addr,
    UD v_addr,
    UD ap,
    UD attr);
static UD * setup_l2_entry(
    UD * l1_tbl,
    UD * ttb,
    UD p_addr,
    UD v_addr,
    UD ap,
    UD attr);
static UD * setup_l3_entry(
    UD * l1_tbl,
    UD * ttb,
    UD p_addr,
    UD v_addr,
    UD ap,
    UD attr);
static UD * make_ttb_sub(
    UD * l1_tbl,
    UD * ttb,
    UD sz,
    UD p_addr,
    UD v_addr,
    UD ap,
    UD attr);
static UD * make_ttb(
    UD * ttb,
    UD bitsz,
    const T_MEM_CFG * cfgtbl);
static SIZE align_addr(VP ptr, SIZE align);
static SIZE align_sz(VP ptr, SIZE sz, SIZE align);
void _ddr_aarch64_mmu_init(
    UD * ttb,
    UD t0_bitsz,
    const T_MEM_CFG * cfgtbl0,
    UD t1_bitsz,
    const T_MEM_CFG * cfgtbl1);

/* Private macro ------------------------------------------------------------*/

#if (USE_CPU == CPU_CORTEX_A53) || (USE_CPU == CPU_CORTEX_A72) || (USE_CPU == CPU_CORTEX_A55) \
 || (USE_CPU == CPU_CORTEX_A76)
#define ICACHE_LINE_LEN     64ULL
#define DCACHE_LINE_LEN     64ULL

#else
#error "Unsupported CPU."
#endif

#define SZ_1GB          0x40000000ULL
#define SZ_2MB          0x00200000ULL
#define SZ_4KB          0x00001000ULL
#define SZ_1GB_ALIGN    (SZ_1GB - 1ULL)
#define SZ_2MB_ALIGN    (SZ_2MB - 1ULL)
#define SZ_4KB_ALIGN    (SZ_4KB - 1ULL)
#define SZ_1GB_OFFSET   30ULL
#define SZ_2MB_OFFSET   21ULL
#define SZ_4KB_OFFSET   12ULL

/*
 * create level2, level3 table
 */
static UD * create_table(
    UD * upper_entry,
    UD * ttb,
    UD sz)
{
    UD e = *upper_entry;
    UD type = e & 3ULL;

    /* if already created, do nothing */
    if (type != 3ULL)
    {
        /* define entry to table  */
        volatile UD * tbl = ttb;
        ttb = &ttb[0x200U];
        *upper_entry = (UD)tbl | 0x3ULL;

        /* create table */
        if (type == 1ULL)        /* block */
        {
            do {
                *tbl = e;
                tbl++;
                e += sz;
            }
            while (tbl < ttb);
        }
        else if (type == 0ULL)   /* invalid */
        {
            do {
                *tbl = 0ULL;
                tbl++;
            }
            while (tbl < ttb);
        }
        else
        {
            ; /* Do nothing */
        }
    }
    return ttb;
}

/*
 * setup level1 entry
 */
static void setup_l1_entry(
    UD * l1_tbl,
    UD p_addr,
    UD v_addr,
    UD ap,
    UD attr)
{
    UW l1_idx = (UW)(v_addr >> SZ_1GB_OFFSET);
    UD entry = p_addr;
    entry |= attr;
    entry |= ap;
    entry |= 1ULL;
    l1_tbl[l1_idx] = entry;
}

/*
 * setup level2 entry
 */
static UD * setup_l2_entry(
    UD * l1_tbl,
    UD * ttb,
    UD p_addr,
    UD v_addr,
    UD ap,
    UD attr)
{
    /* create level2 table */
    UW l1_idx = (UW)(v_addr >> SZ_1GB_OFFSET);
    ttb = create_table(&l1_tbl[l1_idx], ttb, SZ_2MB);

    /* setup level2 entry */
    UD * l2_tbl = (UD *)(l1_tbl[l1_idx] & 0x0000fffffffff000ULL);
    UW l2_idx = (UW)((v_addr & SZ_1GB_ALIGN) >> SZ_2MB_OFFSET);
    UD entry = p_addr;
    entry |= attr;
    entry |= ap;
    entry |= 1ULL;
    l2_tbl[l2_idx] = entry;
    return ttb;
}

/*
 * setup level3 entry
 */
static UD * setup_l3_entry(
    UD * l1_tbl,
    UD * ttb,
    UD p_addr,
    UD v_addr,
    UD ap,
    UD attr)
{
    /* create level2 table */
    UW l1_idx = (UW)(v_addr >> SZ_1GB_OFFSET);
    ttb = create_table(&l1_tbl[l1_idx], ttb, SZ_2MB);

    /* create level3 table */
    UD * l2_tbl = (UD *)(l1_tbl[l1_idx] & 0x0000fffffffff000ULL);
    UW l2_idx = (UW)((v_addr & SZ_1GB_ALIGN) >> SZ_2MB_OFFSET);
    ttb = create_table(&l2_tbl[l2_idx], ttb, SZ_4KB);

    /* setup level3 entry */
    UD * l3_tbl = (UD *)(l2_tbl[l2_idx] & 0x0000fffffffff000ULL);
    UW l3_idx = (UW)((v_addr & SZ_2MB_ALIGN) >> SZ_4KB_OFFSET);
    UD entry = p_addr;
    entry |= attr;
    entry |= ap;
    entry |= 3ULL;
    l3_tbl[l3_idx] = entry;
    return ttb;
}

/*
 * make translation table sub
 */
static UD * make_ttb_sub(
    UD * l1_tbl,
    UD * ttb,
    UD sz,
    UD p_addr,
    UD v_addr,
    UD ap,
    UD attr)
{
    do
    {
        if ((sz >= SZ_1GB)
            && ((p_addr & SZ_1GB_ALIGN) == 0ULL)
            && ((v_addr & SZ_1GB_ALIGN) == 0ULL))
        {
            /* 1GB entry */
            setup_l1_entry(l1_tbl, p_addr, v_addr, ap, attr);
            p_addr  += SZ_1GB;
            v_addr  += SZ_1GB;
            sz      -= SZ_1GB;
        }
        else if ((sz >= SZ_2MB)
            && ((p_addr & SZ_2MB_ALIGN) == 0ULL)
            && ((v_addr & SZ_2MB_ALIGN) == 0ULL))
        {
            /* 2MB entry */
            ttb = setup_l2_entry(l1_tbl, ttb, p_addr, v_addr, ap, attr);
            p_addr  += SZ_2MB;
            v_addr  += SZ_2MB;
            sz      -= SZ_2MB;
        }
        else
        {
            /* 4KB entry */
            ttb = setup_l3_entry(l1_tbl, ttb, p_addr, v_addr, ap, attr);
            p_addr  += SZ_4KB;
            v_addr  += SZ_4KB;
            sz      -= SZ_4KB;
        }
    }
    while (sz > 0ULL);

    return ttb;
}

/*
 * make translation table
 */
static UD * make_ttb(
    UD * ttb,
    UD bitsz,
    const T_MEM_CFG * cfgtbl)
{
    UW cfg_idx = 0U;
    UW i;

    /* create level0 table */
    volatile UD * l1_tbl = &ttb[512U];
    ttb[0] = (UD)l1_tbl | 3ULL;

    /* create level1 table */
    UW l1_cnt = (1U << (bitsz - SZ_1GB_OFFSET));
    ttb = (UD *)&l1_tbl[l1_cnt];
    for (i = 0U; i < l1_cnt; i++)
    {
        l1_tbl[i] = 0ULL;
    }

    while (1U)
    {
        UD sz      = cfgtbl[cfg_idx].sz;
        UD p_addr  = cfgtbl[cfg_idx].p_addr;
        UD v_addr  = cfgtbl[cfg_idx].v_addr;
        UD ap      = cfgtbl[cfg_idx].ap;
        UD attr    = cfgtbl[cfg_idx].attr;
        if (sz == 0ULL)
        {
            break;
        }

        /* clear the most significant bits */
        v_addr = (v_addr & ((UD)-1 >> (64ULL - bitsz)));

        ttb = make_ttb_sub((UD *)l1_tbl, ttb, sz, p_addr, v_addr, ap, attr);
        cfg_idx++;
    }
    return ttb;
}

/**
 * Initialize Cortex-A MMU
 * void _ddr_aarch64_mmu_init(UD * ttb, UD t0_bitsz, const mem_cfg_t * cfgtbl0, UD t1_bitsz, const mem_cfg_t * cfgtbl1);
 * @param ttb       translation table address
 * @param t0_bitsz  bit size of the least significant area
 * @param cfgtbl0   memory configuration table for the least significant area
 * @param t1_bitsz  bit size of the most significant area
 * @param cfgtbl1   memory configuration table for the most significant area
 */
void _ddr_aarch64_mmu_init(
    UD * ttb,
    UD t0_bitsz,
    const T_MEM_CFG * cfgtbl0,
    UD t1_bitsz,
    const T_MEM_CFG * cfgtbl1)
{
    UD _t0bitsz, _t1bitsz;
    UD ttbr0 = (UD)ttb;
    UD ttbr1 = 0ULL;

    if (t0_bitsz < 40ULL) {
        _t0bitsz = 40ULL;
    } else if (t0_bitsz > 48ULL) {
        _t0bitsz = 48ULL;
    } else {
        _t0bitsz = t0_bitsz;
    }
    if (t1_bitsz < 40ULL) {
        _t1bitsz = 40ULL;
    } else if (t1_bitsz > 48ULL) {
        _t1bitsz = 48ULL;
    } else {
        _t1bitsz = t1_bitsz;
    }

    ttb = make_ttb(ttb, _t0bitsz, cfgtbl0);
    ttbr1 = (UD)ttb;
    (void)make_ttb(ttb, _t1bitsz, cfgtbl1);

    UD tcr = TCR_IPS_48BITS         \
                | TCR_TG1_4KB       \
                | TCR_IRGN1_WBW     \
                | TCR_ORGN1_WBW     \
                | TCR_SH1_ISH       \
                | TCR_T1SZ(64ULL - _t1bitsz) \
                | TCR_TG0_4KB       \
                | TCR_IRGN0_WBW     \
                | TCR_ORGN0_WBW     \
                | TCR_SH0_ISH       \
                | TCR_T0SZ(64ULL - _t0bitsz);

    UD mair = MAIR_VAL;
    UD sctlr = SCTLR_ICACHE        \
               | SCTLR_DCACHE      \
               | SCTLR_SA0         \
               | SCTLR_SA          \
               | SCTLR_MMU_ENA;
    mmu_enable(ttbr0, ttbr1, tcr, mair, sctlr);
}

static SIZE align_addr(VP ptr, SIZE align)
{
    SIZE addr = (SIZE)ptr;
    addr &= ~(align - 1ULL);
    return addr;
}

static SIZE align_sz(VP ptr, SIZE sz, SIZE align)
{
	SIZE offset;

    if (sz == 0ULL)
    {
        sz = align;
    }
    else
    {
        offset = (((SIZE)ptr) & (align - 1ULL));
        sz += offset + (align - 1ULL);
        sz &= ~(align - 1ULL);
    }
    return sz;
}

/**
 * Clean data cache
 * void _ddr_aarch64_mmu_clean_data_cache(VP ptr, SIZE sz);
 * @param ptr  start address
 * @param sz   size
 */
void _ddr_aarch64_mmu_clean_data_cache(VP ptr, SIZE sz)
{
    SIZE i;
    SIZE start = align_addr(ptr, DCACHE_LINE_LEN);
    sz = align_sz(ptr, sz, DCACHE_LINE_LEN);
    for (i = 0ULL; i < sz; i += DCACHE_LINE_LEN)
    {
        dc_cvac(start + i);
    }
    vdsb();
}

/**
 * Invalidate data cache
 * void _ddr_aarch64_mmu_invalid_data_cache(VP ptr, SIZE sz);
 * @param ptr  start address
 * @param sz   size
 */
void _ddr_aarch64_mmu_invalid_data_cache(VP ptr, SIZE sz)
{
    SIZE i;
    SIZE start = align_addr(ptr, DCACHE_LINE_LEN);
    sz = align_sz(ptr, sz, DCACHE_LINE_LEN);
    for (i = 0ULL; i < sz; i += DCACHE_LINE_LEN)
    {
        dc_ivac(start + i);
    }
    vdsb();
}

/**
 * Flush data cache
 * void _ddr_aarch64_mmu_flush_data_cache(VP ptr, SIZE sz);
 * @param ptr  start address
 * @param sz   size
 */
void _ddr_aarch64_mmu_flush_data_cache(VP ptr, SIZE sz)
{
    SIZE i;
    SIZE start = align_addr(ptr, DCACHE_LINE_LEN);
    sz = align_sz(ptr, sz, DCACHE_LINE_LEN);
    for (i = 0ULL; i < sz; i += DCACHE_LINE_LEN)
    {
        dc_civac(start + i);
    }
    vdsb();
}

/**
 * Invalidate instruction cache
 * void _ddr_aarch64_mmu_invalid_inst_cache(VP ptr, SIZE sz);
 * @param ptr  start address
 * @param sz   size
 */
void _ddr_aarch64_mmu_invalid_inst_cache(VP ptr, SIZE sz)
{
    if (sz == 0ULL)
    {
        /* invalidate all */
        ic_iallu();
    }
    else
    {
        SIZE i;
        SIZE start = align_addr(ptr, ICACHE_LINE_LEN);
        sz = align_sz(ptr, sz, ICACHE_LINE_LEN);
        for (i = 0ULL; i < sz; i += ICACHE_LINE_LEN)
        {
            ic_ivau(start + i);
        }
    }
    vdsb();
    visb();
}

/**
 * Invalidate TLB cache all
 * void _ddr_aarch64_mmu_invalid_tlb(void);
 */
void _ddr_aarch64_mmu_invalid_tlb(void)
{
    tlbi_alle1is();
    vdsb();
    visb();
}

/**
 * Invalidate all caches (instruction/data)
 * void _ddr_aarch64_mmu_invalid_cache(void);
 */
void _ddr_aarch64_mmu_invalid_cache(void)
{
    /* Invalidate L1 instruction cache */
    ic_iallu();

    /* Invalidate data/unified caches */
    invalid_dcache_all();
}
