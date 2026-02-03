/**
 * @file    kernel_cfg.h
 * @brief   User configuration for Kernel
 * @date    2020.12.16
 * @author  Copyright (c) 2020, eForce Co., Ltd. All rights reserved.
 *
 ****************************************************************************
 * @par     History
 *          - rev 1.0 (2020.12.16) Imada
 *            Initial version.
 ****************************************************************************
 */
#ifndef _KRN_CFG_H_
#define _KRN_CFG_H_

#ifdef __cplusplus
extern "C" {
#endif

/* System timer event cycle (ms) */
#define CFG_KRN_TICK            (1U)

/* System memory size (byte) */
#define CFG_KRN_SYSMEM_SZ       (0x003000U)
/* Stack memory size (byte) */
#define CFG_KRN_STKMEM_SZ       (0x00C000U)
/* Memory pool size (byte) */
#define CFG_KRN_MPLMEM_SZ       (0x004000U)
/* MMU translation space size (byte) */
#define CFG_KRN_MMUTTB_SZ       (0x100000U)

#define CFG_KRN_TSKPRI_MAX      (8U)
#define CFG_KRN_TSKID_MAX       (32U)
#define CFG_KRN_SEMID_MAX       (16U)
#define CFG_KRN_FLGID_MAX       (16U)
#define CFG_KRN_DTQID_MAX       (16U)
#define CFG_KRN_MBXID_MAX       (16U)
#define CFG_KRN_MTXID_MAX       (0U)
#define CFG_KRN_MBFID_MAX       (0U)
#define CFG_KRN_PORID_MAX       (0U)
#define CFG_KRN_MPFID_MAX       (8U)
#define CFG_KRN_MPLID_MAX       (8U)
#define CFG_KRN_ALMID_MAX       (0U)
#define CFG_KRN_CYCID_MAX       (8U)
#define CFG_KRN_ISRID_MAX       (8U)
#define CFG_KRN_DEVID_MAX       (8U)
#define CFG_KRN_SSB_NUM         (16U)

#ifdef __cplusplus
}
#endif
#endif /* _KRN_CFG_H_ */
