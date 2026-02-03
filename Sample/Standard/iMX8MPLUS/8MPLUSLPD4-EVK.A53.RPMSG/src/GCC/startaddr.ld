/**
 * @file    startaddr.ld
 * @brief   Start address settings for memory area.
 * @date    2025.12.19
 * @author  Copyright (c) 2024-2025, eForce Co., Ltd. All rights reserved.
 *
 ****************************************************************************
 * @par     History
 *          - rev 1.0 (2024.11.25)
 *            Initial version.
 *          - rev 1.1 (2025.12.19)
 *            Fixed memory map to match multi core sample.
 ****************************************************************************
 */

#ifndef __START_ADDR_LD__
#define __START_ADDR_LD__

#include "cpu_cfg.h"

#ifdef UC3BOOT_CORE3
/*
 * UC3_CORE3 settings
 */
#define LD_START_ADDR_CODE          (0x97000000)
#define LD_START_ADDR_RAM_DATA      (0x97100000)
#define LD_START_ADDR_RAM_DATA_NC   (0x97800000)
#define LD_START_ADDR_SYS_DATA      (0x97900000)
#define LD_START_ADDR_MMU_TBL       (0x97D00000)
#define LD_START_ADDR_STACK         (0x97E00000)
#define LD_START_ADDR_RES           (0x98800000)
#else /* UC3BOOT_CORE3 */
/*
 * UC3_CORE0 settings
 */
#define LD_START_ADDR_CODE          (0x40800000)
#define LD_START_ADDR_RAM_DATA      (0x40900000)
#define LD_START_ADDR_RAM_DATA_NC   (0x41000000)
#define LD_START_ADDR_SYS_DATA      (0x41100000)
#define LD_START_ADDR_MMU_TBL       (0x41500000)
#define LD_START_ADDR_STACK         (0x41600000)
#define LD_START_ADDR_RES           (0x41800000)
#endif /* UC3BOOT_CORE3 */

/*
 * for linker script
 */
_MAP_CODE_START        = LD_START_ADDR_CODE;
_MAP_CODE_SIZE         = (LD_START_ADDR_RAM_DATA - LD_START_ADDR_CODE);
_MAP_RAM_DATA_START    = LD_START_ADDR_RAM_DATA;
_MAP_RAM_DATA_SIZE     = (LD_START_ADDR_RAM_DATA_NC - LD_START_ADDR_RAM_DATA);
_MAP_RAM_DATA_NC_START = LD_START_ADDR_RAM_DATA_NC;
_MAP_RAM_DATA_NC_SIZE  = (LD_START_ADDR_SYS_DATA - LD_START_ADDR_RAM_DATA_NC);
_MAP_SYS_DATA_START    = LD_START_ADDR_SYS_DATA;
_MAP_SYS_DATA_SIZE     = (LD_START_ADDR_MMU_TBL - LD_START_ADDR_SYS_DATA);
_MAP_MMU_TBL_START     = LD_START_ADDR_MMU_TBL;
_MAP_MMU_TBL_SIZE      = (LD_START_ADDR_STACK - LD_START_ADDR_MMU_TBL);
_MAP_STACK_START       = LD_START_ADDR_STACK;
_MAP_RES_START         = LD_START_ADDR_RES;
_MAP_RES_SIZE          = (0x00001000);

#endif /* __START_ADDR_LD__ */
