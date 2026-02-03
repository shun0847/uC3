/**
 * @brief   Micro C Cube Standard, DEVICE DRIVER
 *          ARM Generic Interrupt Controller v3
 * @date    2025.03.14
 *
 * @copyright (c) 2021-2025, eForce Co., Ltd. All rights reserved.
 */
#ifndef DDR_AARCH64V3_SUB_GIC_H_
#define DDR_AARCH64V3_SUB_GIC_H_

#include "DDR_AArch64_GICv3_cfg.h"
#include "cpu_cfg.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * Functions
 */
/* ICC_BPR1_EL1 */
void seticc_bpr(UD val);
UD geticc_bpr(void);

/* ICC_EOIR1_EL1 */
void seticc_eoir(UD val);

/* ICC_IAR1_EL1 */
UD geticc_iar(void);

/* ICC_SGIR1_EL1 */
void seticc_sgir(UD val);

/* ICC_CTLR_EL{1,3} */
void seticc_ctlr(UD val, INT el);
UD geticc_ctlr(INT el);

/* ICC_IGRPEN0_EL1 */
void seticc_igrpen0(UD val);
UD geticc_igrpen0(void);

/* ICC_IGRPEN1_EL{1,3} */
void seticc_igrpen1(UD val, INT el);
UD geticc_igrpen1(INT el);

/* ICC_SRE_EL{1,3} */
void seticc_sre(UD val, INT el);
UD geticc_sre(INT el);

/* ICC_PMR_EL1 */
void seticc_pmr(UD val);
UD geticc_pmr(void);

/* ICC_RPR_EL1 */
UD geticc_rpr(void);

/* MPIDR_EL1 */
UD get_mpidr(void);

/* CurrentEL */
UD get_current_el(void);

#ifdef __cplusplus
}
#endif

#endif  /* DDR_AARCH64V3_SUB_GIC_H_ */
