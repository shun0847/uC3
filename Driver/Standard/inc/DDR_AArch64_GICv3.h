/**
 * @file    DDR_AArch64_GICv3.h
 * @brief   Micro C Cube Standard, DEVICE DRIVER
 *          ARM Generic Interrupt Controller v3
 * @date    2025.06.11
 * @author  Copyright (c) 2021-2025, eForce Co., Ltd. All rights reserved.
 *
 ****************************************************************************
 * @par     History
 *          - rev 1.0 (2021.01.07) Imada
 *            Initial version.
 *          - rev 1.1 (2021.08.24)
 *            To add ddr_aarch64_gicv3_cfg
 *          - rev 1.2 (2024.11.19)
 *            To add ddr_aarch64_gic_boot_type_t and
 *            ddr_aarch64_gicv3_set_bootctx
 *          - rev 1.3 (2025.06.11)
 *            To add ddr_aarch64_gic_sgir_t and
 *            ddr_aarch64_gicv3_send_sgi_raw
 ****************************************************************************
 */
#ifndef DDR_AARCH64V3_GIC_H_
#define DDR_AARCH64V3_GIC_H_

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum {
    GIC_IRQ_LEVEL_HIGH = 0U,    /* interrupt is level-sensitive */
    GIC_IRQ_EDGE_RISING         /* interrupt is edge-triggered. */
} ddr_aarch64_gic_int_type_t;

typedef enum {
    UBOOT = 1,  /* uC3 boot from u-boot the 'go' command */
    LINUX       /* uC3 boot from Linux */
} ddr_aarch64_gic_bootctx_type_t;

typedef struct {
    UH  targetlist;
    UB  aff1;
    UB  intid;
    UB  aff2;
    UB  irm;
    UB  rs;
    UB  aff3;
} ddr_aarch64_gic_sgir_t;

extern ER ddr_aarch64_gicv3_cfg(void);
extern ER ddr_aarch64_gicv3_init(void);
extern ER ddr_aarch64_gicv3_int_type(INTNO intno, ddr_aarch64_gic_int_type_t type);
extern void ddr_aarch64_gicv3_send_sgi(UINT targetlist, UW intno);
extern void ddr_aarch64_gicv3_send_sgi_raw(const ddr_aarch64_gic_sgir_t *sgir);
#if defined(UC3BOOT_SECONDARY)
extern void ddr_aarch64_gicv3_set_bootctx(ddr_aarch64_gic_bootctx_type_t ctx);
#endif /* #if defined(UC3BOOT_SECONDARY) */

#ifdef __cplusplus
}
#endif

#endif  /* DDR_AARCH64V3_GIC_H_ */
