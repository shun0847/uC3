/**
 * @brief   Micro C Cube Standard, DEVICE DRIVER
 *          ARM Generic Interrupt Controller v3
 * @date    2025.03.14
 *
 * @copyright (c) 2022-2025, eForce Co., Ltd. All rights reserved.
 */

#include "uC3sys.h"
#include "DDR_AArch64_GICv3_sub.h"
#include "DDR_AArch64_GICv3_cfg.h"
#include "cpu_cfg.h"

#ifndef G1NS
#define G1NS   G1
#define G1S    2U
#endif
/* External variables --------------------------------------------------------*/

T_VINFTBL vinftbl[CFG_GIC_INTNUM_MAX] __attribute__((section(".vinftbl")));

#if (USE_SYSTEM != SYSTEM_SINGLE_CORE)
#if !defined(CFG_GIC_SPIN_LOCK_ID)
UW _ddr_gic_flag __attribute__ ((section(".sync")));
#endif
volatile UD _ddr_gic_sync __attribute__ ((section(".sync")));
#endif /* #if (USE_SYSTEM != SYSTEM_SINGLE_CORE) */
#if defined(UC3BOOT_SECONDARY)
/* 
 * In a target linker script file (*.ld), a memory region with the NOLOAD
 * attribute must be defined to hold variables with the '.noinit' section
 * attribute.
 */
INT _ddr_gic_bootctx __attribute__((section (".noinit")));
#endif /* #if defined(UC3BOOT_SECONDARY) */

/* External functions --------------------------------------------------------*/

/*
 * Write GICC system register ICC_BPR{0,1}_EL1
 */
void seticc_bpr(UD val)
{
#if (CFG_GIC_INT_GRP == G0)
    __asm__ volatile( "msr s3_0_c12_c8_3, %0" : "+r"(val) );
#elif ((CFG_GIC_INT_GRP == G1NS) || (CFG_GIC_INT_GRP == G1S))
    __asm__ volatile( "msr s3_0_c12_c12_3, %0" : "+r"(val) );
#else
#error "Invalid CFG_GIC_INT_GRP"
#endif
    __asm__ volatile( "isb" );
}

/*
 * Read GICC system register ICC_BPR{0,1}_EL1
 */
UD geticc_bpr(void)
{
    UD val;
#if (CFG_GIC_INT_GRP == G0)
    __asm__ volatile( "mrs %0, s3_0_c12_c8_3" : "=r"(val) );
#elif ((CFG_GIC_INT_GRP == G1NS) || (CFG_GIC_INT_GRP == G1S))
    __asm__ volatile( "mrs %0, s3_0_c12_c12_3" : "=r"(val) );
#else
#error "Invalid CFG_GIC_INT_GRP"
#endif
    __asm__ volatile( "isb" );
    return val;
}

/*
 * Write GICC system register ICC_EOIR{0,1}_EL1
 */
void seticc_eoir(UD val)
{
#if (CFG_GIC_INT_GRP == G0)
    __asm__ volatile( "msr s3_0_c12_c8_1, %0" : "+r"(val) );
#elif ((CFG_GIC_INT_GRP == G1NS) || (CFG_GIC_INT_GRP == G1S))
    __asm__ volatile( "msr s3_0_c12_c12_1, %0" : "+r"(val) );
#else
#error "Invalid CFG_GIC_INT_GRP"
#endif
    __asm__ volatile( "isb" );
}

/*
 * Read GICC system register ICC_IAR{0,1}_EL1
 */
UD geticc_iar(void)
{
    UD val;
#if (CFG_GIC_INT_GRP == G0)
    __asm__ volatile( "mrs %0, s3_0_c12_c8_0" : "=r"(val) );
#elif ((CFG_GIC_INT_GRP == G1NS) || (CFG_GIC_INT_GRP == G1S))
    __asm__ volatile( "mrs %0, s3_0_c12_c12_0" : "=r"(val) );
#else
#error "Invalid CFG_GIC_INT_GRP"
#endif
    __asm__ volatile( "isb" );
    return val;
}

/*
 * Write GICC system register ICC_SGI{0,1}R_EL1
 */
void seticc_sgir(UD val)
{
#if (CFG_GIC_INT_GRP == G0)
    __asm__ volatile( "msr s3_0_c12_c11_7, %0" : "+r"(val) );
#elif ((CFG_GIC_INT_GRP == G1NS) || (CFG_GIC_INT_GRP == G1S))
    __asm__ volatile( "msr s3_0_c12_c11_5, %0" : "+r"(val) );
#else
#error "Invalid CFG_GIC_INT_GRP"
#endif
    __asm__ volatile( "isb" );
}

/*
 * Write GICC system register ICC_CTLR_EL{1,3}
 */
void seticc_ctlr(UD val, INT el)
{
    if (el == 0x1) {
        __asm__ volatile( "msr s3_0_c12_c12_4, %0" : "+r"(val) );
    } else if (el == 0x3) {
        __asm__ volatile( "msr s3_6_c12_c12_4, %0" : "+r"(val) );
    } else {
        return;
    }
    __asm__ volatile( "isb" );
}

/*
 * Read GICC system register ICC_CTLR_EL{1,3}
 */
UD geticc_ctlr(INT el)
{
    UD val;

    if (el == 0x1) {
        __asm__ volatile( "mrs %0, s3_0_c12_c12_4" : "=r"(val) );
    } else if (el == 0x3) {
        __asm__ volatile( "mrs %0, s3_6_c12_c12_4" : "=r"(val) );
    } else {
        return 0;
    }
    __asm__ volatile( "isb" );
    return val;
}

/*
 * Write GICC system register ICC_IGRPEN0_EL1
 */
void seticc_igrpen0(UD val)
{
    __asm__ volatile( "msr s3_0_c12_c12_6, %0" : "+r"(val) );
    __asm__ volatile( "isb" );
}

/*
 * Read GICC system register ICC_IGRPEN0_EL1
 */
UD geticc_igrpen0(void)
{
    UD val;
    __asm__ volatile( "mrs %0, s3_0_c12_c12_6" : "=r"(val) );
    __asm__ volatile( "isb" );
    return val;
}

/*
 * Write GICC system register ICC_IGRPEN1_EL{1,3}
 */
void seticc_igrpen1(UD val, INT el)
{
    if (el == 0x1) {
        __asm__ volatile( "msr s3_0_c12_c12_7, %0" : "+r"(val) );
    } else if (el == 0x3) {
        __asm__ volatile( "msr s3_6_c12_c12_7, %0" : "+r"(val) );
    } else {
        return;
    }
    __asm__ volatile( "isb" );
}

/*
 * Read GICC system register ICC_IGRPEN1_EL{1,3}
 */
UD geticc_igrpen1(INT el)
{
    UD val;

    if (el == 0x1) {
        __asm__ volatile( "mrs %0, s3_0_c12_c12_7" : "=r"(val) );
    } else if (el == 0x3) {
        __asm__ volatile( "mrs %0, s3_6_c12_c12_7" : "=r"(val) );
    } else {
        return 0;
    }
    __asm__ volatile( "isb" );
    return val;
}

/*
 * Write GICC system register ICC_SRE_EL{1,3}
 */
void seticc_sre(UD val, INT el)
{
    if (el == 0x1) {
        __asm__ volatile( "msr s3_0_c12_c12_5, %0" : "+r"(val) );
    } else if (el == 0x3) {
        __asm__ volatile( "msr s3_6_c12_c12_5, %0" : "+r"(val) );
    } else {
        return;
    }
    __asm__ volatile( "isb" );
}

/*
 * Read GICC system register ICC_SRE_EL{1,3}
 */
UD geticc_sre(INT el)
{
    UD val;

    if (el == 0x1) {
        __asm__ volatile( "mrs %0, s3_0_c12_c12_5" : "=r"(val) );
    } else if (el == 0x3) {
        __asm__ volatile( "mrs %0, s3_6_c12_c12_5" : "=r"(val) );
    } else {
        return 0;
    }
    __asm__ volatile( "isb" );
    return val;
}

/*
 * Write GICC system register ICC_PMR_EL1
 */
void seticc_pmr(UD val)
{
    __asm__ volatile( "msr s3_0_c4_c6_0, %0" : "+r"(val) );
    __asm__ volatile( "isb" );
}

/*
 * Read GICC system register ICC_PMR_EL1
 */
UD geticc_pmr(void)
{
    UD val;
    __asm__ volatile( "mrs %0, s3_0_c4_c6_0" : "=r"(val) );
    __asm__ volatile( "isb" );
    return val;
}

/*
 * Read GICC system register ICC_RPR_EL1
 */
UD geticc_rpr(void)
{
    UD val;
    __asm__ volatile( "mrs %0, s3_0_c12_c11_3" : "=r"(val) );
    __asm__ volatile( "isb" );
    return val;
}

/*
 * Read system register MPIDR_EL1
 */
UD get_mpidr(void)
{
    UD val;
    __asm__ volatile( "mrs %0, MPIDR_EL1" : "=r"(val) );
    __asm__ volatile( "isb" );
    return val;
}

/*
 * Read exception level CurrentEL
 */
UD get_current_el(void)
{
    UD val;
    __asm__ volatile( "mrs %0, CurrentEL" : "=r"(val) );
    return val >> 2;
}
