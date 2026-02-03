/**
 * @brief   Configuration for CPU (Do not modify this file)
 * @date    2025.09.29
 *
 * @copyright (C) 2025, eForce Co., Ltd.
 */
#ifndef CPU_CFG_H_
#define CPU_CFG_H_

#ifdef __cplusplus
extern "C" {
#endif

/* ARMv7-A */
#define CPU_CORTEX_A5      1U
#define CPU_CORTEX_A7      2U
#define CPU_CORTEX_A8      3U
#define CPU_CORTEX_A9      4U
#define CPU_CORTEX_A15     5U

/* ARMv8-A */
#define CPU_CORTEX_A53     11U
#define CPU_CORTEX_A57     12U

/* ARMv7-M */
#define CPU_CORTEX_M3      21U
#define CPU_CORTEX_M4      22U
#define CPU_CORTEX_M7      23U

/* CPU core execption level */
#define EL1                0x1
#define EL3                0x3
#define CPU_EL             (EL1) /* EL1 or EL3 */

/* CPU core security mode */
#define NS                 0x0
#define S                  0x1
#define CPU_ACCESS_MODE    (NS) /* NS or S */

/* uC3 platform */
#define SYSTEM_SINGLE_CORE 1U
#define SYSTEM_AMP         2U
#define SYSTEM_SMP         3U /* not supported */
#define SYSTEM_HETERO_CORE 4U

/* Target CPU configuration */
#define USE_CPU            (CPU_CORTEX_A53)
#define USE_SYSTEM         (SYSTEM_HETERO_CORE)

/* for hetero-core */
#define CLUSTER_NUMBER     0U

/* use low power mode? */
#define USE_STANDBY_MODE   0U

#ifdef __cplusplus
}
#endif
#endif /* CPU_CFG_H_ */
