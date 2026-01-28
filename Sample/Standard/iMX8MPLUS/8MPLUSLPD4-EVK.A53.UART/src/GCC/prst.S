/**
 * @file    prst.S
 * @brief   Reset handler for 8MPLUSLPD4-EVK (i.MX8M Plus, Cortex-A53)
 * @date    2026.01.05
 * @author  Copyright (c) 2021-2026, eForce Co., Ltd. All rights reserved.
 *
 ******************************************************************************
 * @par     History
 *          - rev 1.0 (2021.02.02) Imada
 *            Initial version.
 *          - rev 1.1 (2021.10.21) Imada
 *            Added the isb instruction just after the msr instruction.
 *          - rev 1.1 (2021.10.21) Imada
 *            Added the isb instruction just after the msr instruction.
 *          - rev 1.2 (2024.09.25) Imada
 *            Introduced GICv3 configuation.
 *            Fixed an error on the orr instruction with OR-ed arguments.
 *            Added functionality to boot Core#3 up.
 *          - rev 1.3 (2024.11.15)
 *            Change from EWARM to GCC
 *          - rev 1.4 (2025.04.28)
 *            Add _ddr_aarch64_gicv3_cfg invocation.
 *          - rev 1.5 (2025.10.02)
 *            Fixed ddr_aarch64_gicv3_cfg declaration.
 *          - rev 1.6 (2026.01.05)
 *            Remove E2H flag setting.
 ******************************************************************************
 */

#include "cpu_cfg.h"

/* definition bit of DAIF */
#define I_BIT      (0x2)
#define F_BIT      (0x1)

#ifdef UC3BOOT_SECONDARY
#define UBOOT      (0x1)
#endif

/* CA53 core #0 START_ADDRH */
#define SRC_GPR1   (0x0000000030390074)
#ifdef UC3BOOT_CORE3
/* CA53 core #3 START_ADDRH */
#define SRC_GPR7   (0x000000003039008C)
#define FUNC_ID    (0xC4000003)
/* uC3 will run on top of Core#3 */
#define UC3_CORE   (0x3)
#endif

#ifdef UC3BOOT_SECONDARY
        .globl  ddr_aarch64_gicv3_set_bootctx
#endif
        .globl  ddr_aarch64_gicv3_cfg
        .globl  _ddr_aarch64_mmu_init
        .globl  _ddr_aarch64_mmu_invalid_tlb
        .globl  _ddr_aarch64_mmu_invalid_cache
        .globl  mem_cfgtbl0
        .globl  mem_cfgtbl1
        .globl  mmu_space
#ifdef UC3BOOT_CORE3
        .global  _psci_smc
#endif
        .globl  _vector_table
        .globl  _vector_table0

        .balign  8
        .section .prst

        .global _RPST
        .type   _RPST, %function

/*
 * Power On Reset Handler
 */
        .globl  Reset_Handler
        .type   Reset_Handler, %function
_RPST:
Reset_Handler:
        msr     daifset,  #(I_BIT | F_BIT)
        isb

#ifdef UC3BOOT_CORE3
        mrs     x0,  mpidr_el1
        and     x0,  x0,  #0xff
        cbz     x0,  rh_boot_secondary
        b       rh_main
rh_boot_secondary:
        mrs     x0,  vbar_el2
        stp     x0,  x30,  [sp, #-16]!

        /*
         * Set vector address
         */
        ldr     x0,  =_vector_table0
        msr     vbar_el2,  x0
        isb

        stp     x1,  x2,  [sp, #-16]!
        stp     x3,  x4,  [sp, #-16]!
        stp     x5,  x6,  [sp, #-16]!
        stp     x7,  x8,  [sp, #-16]!
        stp     x9,  x10,  [sp, #-16]!
        stp     x11,  x12,  [sp, #-16]!
        stp     x13,  x14,  [sp, #-16]!
        stp     x15,  x16,  [sp, #-16]!
        stp     x17,  x18,  [sp, #-16]!
        stp     x19,  x20,  [sp, #-16]!
        stp     x21,  x22,  [sp, #-16]!
        stp     x23,  x24,  [sp, #-16]!
        stp     x25,  x26,  [sp, #-16]!
        stp     x27,  x28,  [sp, #-16]!
        stp     x29,  x30,  [sp, #-16]!

        /*
         * Disable MMU
         */
        mrs     x0,  sctlr_el1
        and     x0,  x0,  #~(0x00001000)
        and     x0,  x0,  #~(0x00000004)
        and     x0,  x0,  #~(0x00000001)
        msr     sctlr_el1,  x0
        dsb     ish
        isb

        /*
         * Invalidate all caches and TLBs
         */
        bl      _ddr_aarch64_mmu_invalid_cache
        bl      _ddr_aarch64_mmu_invalid_tlb

        /*
         * Set vector address
         */
        /*
         * ldr     x0, =_vector_table0
         * msr     vbar_el1, x0
         * isb
         */

        /*
         * Switch exception level from EL2 to EL1
         */
        mov     x0,  #(0x1 << 31)   // AArch64
        orr     x0,  x0,  #(0x1 << 1)  // SWIO
        msr     hcr_el2,  x0
        movz    x0,  #0x1c4
        msr     spsr_el2,  x0
        adr     x0,  el2_to_el1_primary
        msr     elr_el2,  x0
        eret
el2_to_el1_primary:
        /*
         * Relocatable exec finished
         */
        adr     x30, boot_secondary
        br      x30
boot_secondary:
        ldr     x0,  =FUNC_ID
        ldr     x1,  =UC3_CORE
        adr     x2,  Reset_Handler
        bl      _psci_smc
        hvc     #0
        dsb     sy
        isb
        nop
        nop
        nop
#endif /* UC3BOOT_CORE3 */

/*
 * Main reset handler body for a core on which uC3 will run
 */
rh_main:
#if defined(__DEBUG) && defined(UC3BOOT_CORE3)
        mov     x0,  xzr
debug_loop:
        cmp     x0,  #0x1
        b.ne    debug_loop
#endif

        /*
         * Switch the stack region for EL2 (to use uC3 sp_el1)
         */
        ldr     x2,  =__el1_stack
        mov     sp,  x2

        /*
         * Disable MMU
         */
        mrs     x0,  sctlr_el1
        and     x0,  x0,  #~(0x00001000)
        and     x0,  x0,  #~(0x00000004)
        and     x0,  x0,  #~(0x00000001)
        msr     sctlr_el1,  x0
        dsb     ish
        isb

        /*
         * Invalidate all caches and TLBs
         */
        bl      _ddr_aarch64_mmu_invalid_cache
        bl      _ddr_aarch64_mmu_invalid_tlb

        /*
         * Set vector address
         */
        ldr     x0,  =_vector_table
        msr     vbar_el1,  x0
        isb

        /*
         * Enable ECC protection
         */
        mrs     x0,  S3_1_C11_C0_2    /* register L2CTLR_EL1 */
        orr     x0,  x0,  #(1 << 22)
        msr     S3_1_C11_C0_2,  x0
        isb

        /*
         * Configure cpu auxiliary control register EL1
         */
        ldr     x0, =0x80CA000        /* L1 Data prefetch control - 5, Enable device split throttle, 2 independent data prefetch streams */
        msr     S3_1_C15_C2_0,  x0    /* CPUACTLR_EL1 */
        isb

        /*
         * Enalbe SMPEN bit
         */
        mrs     x0,  S3_1_C15_C2_1
        orr     x0,  x0,  #0x40
        msr     S3_1_C15_C2_1,  x0
        isb

        /*
         * Enable access to  FP/SIMD registers
         */
        mov     x0,  #(3 << 20)
        msr     cpacr_el1,  x0
        isb

        /*
         * MMU initialization and address map setup
         */
        ldr     x0,  =mmu_space
        mov     x1,  #40
        ldr     x2,  =mem_cfgtbl0
        mov     x3,  #40
        ldr     x4,  =mem_cfgtbl1
        bl      _ddr_aarch64_mmu_init

#ifdef UC3BOOT_SECONDARY
         /* Boot context configuration for GICv3
          * Initial X0 : 0x1 set by boot_secondary above (for both release and debugging)
          *            : 0x2 set by cpuctl/parameters for release, by a debugger for debugging
          */
        ldr     x0,  =UBOOT
        bl      ddr_aarch64_gicv3_set_bootctx
#endif

        /*
         * GICv3 configuration
         */
        bl      ddr_aarch64_gicv3_cfg

        /*
         * Initialize stack pointer of each exception level
         */
        ldr     x0,  =__el1_stack
        msr     sp_el1,  x0
        ldr     x0,  =__el0_stack
        msr     sp_el0,  x0
        isb

        /*
         * Switch exception level from EL2 to EL1
         */
        mov     x0,  #(0x1 << 31)      /* AArch64 */
        orr     x0,  x0,  #(0x1 << 34) /* E2H */
        orr     x0,  x0,  #(0x1 << 1)  /* SWIO */
        msr     hcr_el2,  x0
        movz    x0,  #0x1c4
        msr     spsr_el2,  x0
        adr     x0,  el2_to_el1
        msr     elr_el2,  x0
        isb
        eret
el2_to_el1:
        /*
         * Relocatable exec finished
         */
        adr     x30,  abs_addr
        br      x30
abs_addr:
#ifdef UC3BOOT_CORE3
        /*
         *  Set reset vector base address for A53 core #3
         *  RVBARADDR3 = {SRC_GPR7[15:0], SRC_GPR8[21:2]}
         *  Set RVBARADDR3 as 0x0ULL to restart with the boot ROM
         */
        mov     x0,  xzr
        ldr     x1,  =SRC_GPR7    /* meaning the address of SRC_GPR7 */
        str     w0,  [x1]
        add     w1,  w1,  #0x4     /* meaning the address of SRC_GPR8 */
        str     w0,  [x1]
#else
        /*
         *  Set reset vector base address for A53 core #0
         *  RVBARADDR0 = {SRC_GPR1[15:0], SRC_GPR2[21:2]}
         *  Set RVBARADDR0 as 0x0ULL to restart with the boot ROM
         */
        mov     x0,  xzr
        ldr     x1,  =SRC_GPR1    /* meaning the address of SRC_GPR1 */
        str     w0,  [x1]
        add     w1,  w1,  #0x4     /* meaning the address of SRC_GPR2 */
        str     w0,  [x1]
#endif

        /* clear sbss */
        ldr     x0,  =__sbss_start
        ldr     x1,  =__sbss_end
        bl      ini_bss

        /* clear bss */
        ldr     x0,  =__bss_start__
        ldr     x1,  =__bss_end__
        bl      ini_bss

        ldr     x30,  =main
        br      x30

#if 0
/*
 * Initialize data section
 */
ini_data:
        cmp     x0,  x1
        b.ge    ini_data_end
        ldr     x3,  [x2],  #8
        str     x3,  [x0],  #8
        b       ini_data
ini_data_end:
        ret
#endif

/*
 * Initialize bss section
 */
        .globl  ini_bss
        .type   ini_bss, @function
ini_bss:
        cmp     x0,  x1
        b.ge    ini_bss_end
        str     xzr, [x0],  #8
        b       ini_bss
ini_bss_end:
        ret

