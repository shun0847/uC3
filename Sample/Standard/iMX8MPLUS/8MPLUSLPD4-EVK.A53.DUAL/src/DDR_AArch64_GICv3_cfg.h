/**
 * @brief   User configuration for ARM Generic Interrupt Controller
 * @date    2025.09.29
 *
 * @copyright (C) 2025, eForce Co., Ltd.
 */
#ifndef DDR_AARCH64_GICV3_CFG_H_
#define DDR_AARCH64_GICV3_CFG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "imx8mplus_uC3.h"

/* Priority mask for GIC CPU interface */
#define CFG_GIC_PRI_MASK   (0xF0ULL)

/* Binary point for GIC CPU interface */
#define CFG_GIC_BIN_POINT  (3ULL)

/* Base address of GIC distributor register */
#define CFG_GIC_DIST_BASE  (GIC_BASE)

/* Number of interrupts */
#define CFG_GIC_INTNUM_MAX (32U + 154U) /* 32 for SGI/PPI, 154 for SPI */

/* Interrupt group number */
#define G0                 0x0 /* G0: Group0, G1: Group1 */
#define G1                 0x1 /* G0: Group0, G1: Group1 */
#define CFG_GIC_INT_GRP    (G1)

/* The number of CPUs using GIC */
#define CFG_GIC_NUM_CPUS   (2U)

/* The number of bits to specify GIC-500 redistributor addresses
 * See "3.2 The GIC-500 register map" in ARM CoreLink GIC-500 Generic 
 * Interrupt Controller Technical Reference Manual
 * bits = 18 + max(1, ceil(log2 (total_number_of_cpus))) */
#define CFG_GIC_NUM_BITS   (20U)

#ifdef __cplusplus
}
#endif
#endif /* DDR_AARCH64_GICV3_CFG_H_ */
