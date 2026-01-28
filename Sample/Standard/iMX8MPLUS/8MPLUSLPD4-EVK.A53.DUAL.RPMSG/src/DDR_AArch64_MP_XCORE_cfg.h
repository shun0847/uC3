/**
 * @brief   User configuration for i.MX8M Plus Cortex-A53
 *          MP Hetero core communication.
 * @date    2025.12.19
 *
 * @copyright (C) 2025, eForce Co., Ltd.
 */
#ifndef DDR_AARCH64_MP_XCORE_CFG_H_
#define DDR_AARCH64_MP_XCORE_CFG_H_

#include "uC3sys.h"
#include "uC3xcext.h"
#include "imx8mplus_uC3.h"

/* Domain and Core information */
#define CFG_DOMAIN_SYNC         (1U)                 /* not required for homogeneous structure */
#define CFG_SYNC_SYSCALL_MODE   (SYNCCALL_SYNC_MODE) /* Sync or Async (Normally Sync) for some system call */
#define CFG_DOMAIN_MAX          (1U)                 /* Number of Domains */

#define CFG_MCORE_SYSCALL_INTNO (INT_SOFT_0)    /* SGI number (0...15) */
#define CFG_MCORE_SYSCALL_IPL   (0x20U)         /* Interrupt Priority (16...224) */
#define ASYNCBUF_NUM            (16U)           /* FIFO level of Async system call */
#define CORE_NUM                (4U)            /* Number of Cores */
#define CFG_MCORE_SYNC_ID       (0x02020000U)   /* Core synchronization ID */

/* GIC specification */
#define USE_GICV3               /* GIC specification on this processor */

DEF_ASYNCTBL(T_MCORE_ASYNCTBL, ASYNCBUF_NUM);

#endif /* DDR_AARCH64_MP_XCORE_CFG_H_ */
