/**
 * @brief   User configuration for ARM Generic Timer for i.MX8M Plus
 * @date    2025.09.29
 *
 * @copyright (C) 2025, eForce Co., Ltd.
 */
#ifndef DDR_AARCH64_GTIMER_CFG_H_
#define DDR_AARCH64_GTIMER_CFG_H_

#ifndef __MACRO_ONLY
#define __APU__
#include "imx8mplus_uC3.h"

#ifdef __cplusplus
extern "C" {
#endif
#endif /* __MACRO_ONLY */

#define CFG_GTIMER_CLK (8000000U) /* 8MHz : after u-boot launched */

#ifndef __MACRO_ONLY

#define CFG_GTIMER_INTNO    (INT_NS_PHYSICAL_TIMER)
#define CFG_GTIMER_IPL      (0xE0U)
#define CFG_GTIMER_REG_BASE (SYSCNT_CTRL_REG_BASE)
#define MASTER_CORE_ID      (ID_CORE0)

#ifdef __cplusplus
}
#endif
#endif /* __MACRO_ONLY */

#endif /* DDR_AARCH64_GTIMER_CFG_H_ */
