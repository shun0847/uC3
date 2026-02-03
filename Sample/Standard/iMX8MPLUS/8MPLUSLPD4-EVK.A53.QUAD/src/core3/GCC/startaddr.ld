/**
 * @brief   Start address settings for memory area.
 * @date    2025.09.29
 *
 * @copyright (C) 2025, eForce Co., Ltd.
 */

#ifndef __START_ADDR_LD__
#define __START_ADDR_LD__

#include "cpu_cfg.h"

/*
 * UC3_CORE0 settings
 */
#define LD_START_ADDR_CODE          (0x40B00000)
#define LD_START_ADDR_RAM_DATA      (0x41D00000)
#define LD_START_ADDR_RAM_DATA_NC   (0x42700000)
#define LD_START_ADDR_SYS_DATA      (0x43400000)
#define LD_START_ADDR_MMU_TBL       (0x43B00000)
#define LD_START_ADDR_STACK         (0x44200000)
#define LD_START_ADDR_DOMAIN_SHARED (0x44400000)
#define LD_START_ADDR_GLOBAL_SHARED (0x44500000)

/*
 * for linker script
 */
_MAP_CODE_START             = LD_START_ADDR_CODE;
_MAP_CODE_SIZE              = (0x00100000);
_MAP_RAM_DATA_START         = LD_START_ADDR_RAM_DATA;
_MAP_RAM_DATA_SIZE          = (0x00600000);
_MAP_RAM_DATA_NC_START      = LD_START_ADDR_RAM_DATA_NC;
_MAP_RAM_DATA_NC_SIZE       = (0x00100000);
_MAP_SYS_DATA_START         = LD_START_ADDR_SYS_DATA;
_MAP_SYS_DATA_SIZE          = (0x00400000);
_MAP_MMU_TBL_START          = LD_START_ADDR_MMU_TBL;
_MAP_MMU_TBL_SIZE           = (0x00100000);
_MAP_STACK_START            = LD_START_ADDR_STACK;
_MAP_DOMAIN_SHARED_START    = LD_START_ADDR_DOMAIN_SHARED;
_MAP_DOMAIN_SHARED_SIZE     = (0x00100000);
_MAP_GLOBAL_SHARED_START    = LD_START_ADDR_GLOBAL_SHARED;
_MAP_GLOBAL_SHARED_SIZE     = (0x00100000);

#endif /* __START_ADDR_LD__ */
