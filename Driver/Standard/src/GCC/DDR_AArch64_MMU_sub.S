/**
 * @file    DDR_AArch64_MMU_sub.S
 * @brief   Micro C Cube Standard, MMU and Cache management (AArch64)
 * @date    2020.01.07
 * @author  Copyright (c) 2016-2020, eForce Co., Ltd. All rights reserved.
 *
 ******************************************************************************
 * @par     History
 *          - rev 1.0 (2016.09.27) y-kim
 *            Initial version.
 *          - rev 1.1 (2020.01.07) yokota
 *            Fix invalid_dcache_all, .
 *            Fix hungup tlbi_alle1is call from el0/el1.
 ******************************************************************************
 */

/*
 * enable AArch64 MMU
 * void mmu_enable(UD ttbr0, UD ttbr1, UD tcr, UD mair, UD sctlr);
 */
        .text
        .balign 4
        .globl  mmu_enable
        .type   mmu_enable, @function
mmu_enable:
        dsb     sy
        msr     ttbr0_el1, x0
        msr     ttbr1_el1, x1
        msr     tcr_el1, x2
        msr     mair_el1, x3
        msr     contextidr_el1, xzr
        isb
        dsb     sy

        msr     sctlr_el1, x4
        isb
        ret

/*
 * clean dcache
 * void dc_cvac(UD addr);
 */
        .globl  dc_cvac
        .type   dc_cvac, @function
dc_cvac:
        dc      cvac, x0
        ret

/*
 * invalidate dcache
 *  void dc_ivac(UD addr);
 */
        .globl  dc_ivac
        .type   dc_ivac, @function
dc_ivac:
        dc      ivac, x0
        ret

/*
 * clean and invalidate dcache
 * void dc_civac(UD addr);
 */
        .globl  dc_civac
        .type   dc_civac, @function
dc_civac:
        dc      civac, x0
        ret

/*
 * invalidate icache all
 * void ic_iallu(void);
 */
        .globl  ic_iallu
        .type   ic_iallu, @function
ic_iallu:
        ic      iallu
        ret

/*
 * invalidate icache
 * void ic_ivau(UD addr);
 */
        .globl  ic_ivau
        .type   ic_ivau, @function
ic_ivau:
        ic      ivau, x0
        ret

/*
 * invalidate TLB cache all
 * void tlbi_alle1is(void);
 */
        .globl  tlbi_alle1is
        .type   tlbi_alle1is, @function
tlbi_alle1is:
        mrs     x0, currentEL
        lsr     x0, x0, #3
        cbz     x0, el0_or_el1
        tlbi    alle1is
        b       tlbi_ret
el0_or_el1:
        tlbi    vmalle1is
tlbi_ret:
        ret

/*
 * invalidate dcache all
 */
        .globl  invalid_dcache_all
        .type   invalid_dcache_all, @function
invalid_dcache_all:
        /* Invalidate data/unified caches */
        mrs     x0, clidr_el1
        ands    x3,  x0,  #0x7000000
        lsr     x3,  x3,  #23
        b.eq    finished

        mov     x10, #0
loop_Lx:
        add     x2,  x10, x10, lsr #1
        lsr     x1,  x0,  x2
        and     x1,  x1,  #7
        cmp     x1,  #2
        b.lt    skip

        msr     csselr_el1, x10
        isb
        mrs     x1, ccsidr_el1
        and     x2,  x1,  #0x7
        add     x2,  x2,  #4
        mov     x4,  #0x3FF
        ands    x4,  x4,  x1,  lsr #3
        clz     w5,  w4
        mov     x7,  #0x00007FFF
        ands    x7,  x7,  x1,  lsr #13

loop_way:
        mov     x9, x4

loop_set:
        lsl     x12,  x9,  x5
        orr     x11, x10, x12
        lsl     x12,  x7,  x2
        orr     x11, x11, x12
        dc      isw, x11
        subs    x9,  x9,  #1
        b.ge    loop_set
        subs    x7,  x7,  #1
        b.ge    loop_way
skip:
        add     x10, x10, #2
        cmp     x3,  x10
        b.gt     loop_Lx

finished:
        dsb     sy
        isb
        ret
