/**
 * @brief   User configuration for ARM Generic Interrupt Controller
 * @date    2025.12.19
 *
 * @copyright (C) 2025, eForce Co., Ltd.
 */
#ifndef DDR_AARCH64_GICV3_CFG_H_
#define DDR_AARCH64_GICV3_CFG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "imx8mplus_uC3.h"
#include "cpu_cfg.h"

/* Priority mask for GIC CPU interface */
#define CFG_GIC_PRI_MASK            (0xF0ULL)

/* Binary point for GIC CPU interface */
#define CFG_GIC_BIN_POINT           (3ULL)

/* Base address of GIC distributor register */
#define CFG_GIC_DIST_BASE           (GIC_BASE)

/* Number of interrupts */
#define CFG_GIC_INTNUM_MAX          (32U + 154U) /* 32 for SGI/PPI, 154 for SPI */

/* Interrupt group number */
#define G0                 0x0 /* G0: Group0 Always Secure(FIQ) */
#define G1NS               0x1 /* G1NS: NonSecure Group1(IRQ) */
#define G1S                0x2 /* G1S: Secure Group1(IRQ) */
#if (CPU_ACCESS_MODE == S)
#define CFG_GIC_INT_GRP  (G1S)
/* G1S:Secure Group1(IRQ) mode Only */
/*extern UW cfg_g0_array[];*/
/*extern UW cfg_g1ns_array[];*/
#define CFG_GIC_USE_G0   (0U)
/* #define CFG_GIC_G0_ARRAY            (cfg_g0_array) */
#define CFG_GIC_USE_G1NS (0U)
/* #define CFG_GIC_G1NS_ARRAY          (cfg_g1ns_array) */
#elif (CPU_ACCESS_MODE == NS)
#define CFG_GIC_INT_GRP (G1NS)
#else
#error "Invalid CPU_ACCESS_MODE"
#endif

/* The number of CPUs using GIC */
#define CFG_GIC_NUM_CPUS            (4U)
/* Core synchronization ID */
#define CFG_GIC_SYNC_ID             (0x02020000U)

/* The number of bits to specify GIC-500 redistributor addresses
 * See "3.2 The GIC-500 register map" in ARM CoreLink GIC-500 Generic 
 * Interrupt Controller Technical Reference Manual
 * bits = 18 + max(1, ceil(log2 (total_number_of_cpus))) */
#define CFG_GIC_NUM_BITS            (20U)

#ifdef __cplusplus
}
#endif
#endif /* DDR_AARCH64_GICV3_CFG_H_ */
