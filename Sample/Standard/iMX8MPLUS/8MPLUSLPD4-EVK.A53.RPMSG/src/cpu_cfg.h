/**
 * @file    cpu_cfg.h
 * @brief   Configuration for CPU (Do not modify this file)
 * @date    2024.09.25
 * @author  Copyright (c) 2021-2024, eForce Co., Ltd. All rights reserved.
 *
 ****************************************************************************
 * @par     History
 *          - rev 1.0 (2021.01.25) Imada
 *            Initial version.
 *          - rev 1.0 (2024.09.25) Imada
 *            Fix parameters for Linux boot.
 ****************************************************************************
 */
#ifndef CPU_CFG_H_
#define CPU_CFG_H_

#ifdef __cplusplus
extern "C" {
#endif

/* ARMv7-A */
#define CPU_CORTEX_A5       1U
#define CPU_CORTEX_A7       2U
#define CPU_CORTEX_A8       3U
#define CPU_CORTEX_A9       4U
#define CPU_CORTEX_A15      5U

/* ARMv8-A */
#define CPU_CORTEX_A53      11U
#define CPU_CORTEX_A57      12U

/* ARMv7-M */
#define CPU_CORTEX_M3       21U
#define CPU_CORTEX_M4       22U
#define CPU_CORTEX_M7       23U

/* CPU core execption level */
#define EL1                 0x1
#define EL2                 0x2
#define EL3                 0x3
#define CPU_EL              (EL1)   /* u-boot uses EL2 */

/* CPU core security mode */
#define NS                  0x0
#define S                   0x1
#define CPU_ACCESS_MODE     (NS)    /* NS or S */

/* uC3 platform */
#define SYSTEM_SINGLE_CORE  1U
#define SYSTEM_AMP          2U
#define SYSTEM_SMP          3U  /* not supported */

/* Target CPU configuration */
#define USE_CPU             (CPU_CORTEX_A53)
#define USE_SYSTEM          (SYSTEM_SINGLE_CORE)

/* for hetero-core */
#define CLUSTER_NUMBER      0U

/* use low power mode? */
#define USE_STANDBY_MODE    0U
  
/* Using Core#3 as a uC3 core is supported for only GCC */
#ifdef __GNUC__
/*
  * Define UC3BOOT_SECONDARY if you want to boot uC3 up on one of
secondary cores.
  */
#define UC3BOOT_SECONDARY

/* Define UC3BOOT_CORE3 if you want to boot uC3 on Core#3 and use UART4
  * UC3BOOT_SECONDARY must be defined if you want to define UC3BOOT_CORE3.
  */
#define UC3BOOT_CORE3

#if !defined(UC3BOOT_SECONDARY) && defined(UC3BOOT_CORE3)
#error "UC3BOOT_SECONDARY must be defined for UC3BOOT_CORE3"
#endif
#endif /* #ifdef __GNUC__ */
#ifdef __cplusplus
}
#endif
#endif /* CPU_CFG_H_ */
