/**
 * @file    krn_mem.c
 * @brief   Memory definitions to use in kernel inside.
 * @date    2020.05.12
 * @author  Copyright (c) 2016-2020, eForce Co., Ltd. All rights reserved.
 *
 ****************************************************************************
 * @par     History
 *          - rev 1.0 (2020.05.12) yokota
 *            Initial version.
 ****************************************************************************
 */
#include "kernel.h"
#include "kernel_cfg.h"

/* System memory */
UB SYSMEM[CFG_KRN_SYSMEM_SZ] __attribute__((section(".sysmem")));

/* Stack memory */
UB STKMEM[CFG_KRN_STKMEM_SZ] __attribute__((section(".stkmem")));

/* Memory pool */
UB MPLMEM[CFG_KRN_MPLMEM_SZ] __attribute__((section(".mplmem")));

/* MMU space */
UB mmu_space[CFG_KRN_MMUTTB_SZ] __attribute__ ((section(".mmu_tbl")));
