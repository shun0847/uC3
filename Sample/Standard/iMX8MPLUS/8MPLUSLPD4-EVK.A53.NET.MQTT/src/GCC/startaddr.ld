/**
 * @file    startaddr.ld
 * @brief   Start address settings for memory area.
 * @date    2024.11.25
 * @author  Copyright (c) 2024, eForce Co., Ltd. All rights reserved.
 *
 ****************************************************************************
 * @par     History
 *          - rev 1.0 (2024.11.25)
 *            Initial version.
 ****************************************************************************
 */

#ifndef __START_ADDR_LD__
#define __START_ADDR_LD__

#include "cpu_cfg.h"

#ifdef UC3BOOT_CORE3
/*
 * UC3_CORE3 settings
 */
#define LD_START_ADDR_CODE          (0x95000000)
#define LD_START_ADDR_RAM_DATA      (0x95100000)
#define LD_START_ADDR_RAM_DATA_NC   (0x95800000)
#define LD_START_ADDR_SYS_DATA      (0x95900000)
#define LD_START_ADDR_MMU_TBL       (0x95D00000)
#define LD_START_ADDR_STACK         (0x95E00000)
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

#endif /* __START_ADDR_LD__ */
