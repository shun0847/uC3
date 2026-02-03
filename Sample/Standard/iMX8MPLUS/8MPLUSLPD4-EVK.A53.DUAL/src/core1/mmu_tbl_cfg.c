/**
 * @brief   MMU configuration table
 * @date    2025.09.29
 *
 * @copyright (C) 2025, eForce Co., Ltd.
 */
#include "DDR_AArch64_MMU.h"
#include "imx8mplus_uC3.h"

const T_MEM_CFG mem_cfgtbl0[] = {
  /* Common area */
  /*                   SIZE,                  PADDR,                     VADDR,        AP,    ATTR */
    {0x0000000010000000ULL, 0x0000000030000000ULL, 0x0000000030000000ULL, AP_RWNA, ATTR_DEV}, /* I/O ports */

  /* CPU core #1 */
  /*                   SIZE,                  PADDR,                     VADDR,        AP,    ATTR */
    {0x0000000000100000ULL, 0x0000000040900000ULL, 0x0000000040900000ULL, AP_RWNA, ATTR_WB }, /* Code */
    {0x0000000000600000ULL, 0x0000000041000000ULL, 0x0000000041000000ULL, AP_RWNA, ATTR_WB }, /* Data */
    {0x0000000000100000ULL, 0x0000000041700000ULL, 0x0000000041700000ULL, AP_RWNA, ATTR_NC }, /* Data(no-cache) */
    {0x0000000000400000ULL, 0x0000000041C00000ULL, 0x0000000041C00000ULL, AP_RWNA, ATTR_WB }, /* System data */
    {0x0000000000100000ULL, 0x0000000042100000ULL, 0x0000000042100000ULL, AP_RWNA, ATTR_WB }, /* MMU TTB data */
    {0x0000000000200000ULL, 0x0000000042400000ULL, 0x0000000042400000ULL, AP_RWNA, ATTR_WB }, /* Stack(SP0, SP1) */

  /* Shared */
  /*                   SIZE,                  PADDR,                     VADDR,        AP,    ATTR */
    {0x0000000000100000ULL, 0x0000000042600000ULL, 0x0000000042600000ULL, AP_RWNA, ATTR_WB }, /* Sync */
    {0x0000000000100000ULL, 0x0000000042700000ULL, 0x0000000042700000ULL, AP_RWNA, ATTR_WB }, /* Global shared */

  /* Terminator */
    {0,                     0,                     0,                     0,       0       },
};

#define _H(a) ((0xFFFFFF0000000000ULL) | (a))

const T_MEM_CFG mem_cfgtbl1[] = {
  /* Common area */
  /*                   SIZE,                  PADDR,                          VADDR,      AP,     ATTR */
    {0x0000000010000000ULL, 0x0000000030000000ULL, _H(0x0000000030000000ULL), AP_RWNA, ATTR_DEV}, /* I/O ports */

  /* CPU core #1 */
  /*                   SIZE,                  PADDR,                          VADDR,      AP,    ATTR */
    {0x0000000000100000ULL, 0x0000000040900000ULL, _H(0x0000000040900000ULL), AP_RWNA, ATTR_WB }, /* Code */
    {0x0000000000600000ULL, 0x0000000041000000ULL, _H(0x0000000041000000ULL), AP_RWNA, ATTR_WB }, /* Data */
    {0x0000000000100000ULL, 0x0000000041700000ULL, _H(0x0000000041700000ULL), AP_RWNA, ATTR_NC }, /* Data(no-cache) */
    {0x0000000000400000ULL, 0x0000000041C00000ULL, _H(0x0000000041C00000ULL), AP_RWNA, ATTR_WB }, /* System data */
    {0x0000000000100000ULL, 0x0000000042100000ULL, _H(0x0000000042100000ULL), AP_RWNA, ATTR_WB }, /* MMU TTB data */
    {0x0000000000200000ULL, 0x0000000042400000ULL, _H(0x0000000042400000ULL), AP_RWNA, ATTR_WB }, /* Stack(SP0, SP1) */

  /* Shared */
  /*                   SIZE,                  PADDR,                          VADDR,      AP,    ATTR */
    {0x0000000000100000ULL, 0x0000000042600000ULL, _H(0x0000000042600000ULL), AP_RWNA, ATTR_WB }, /* Sync */
    {0x0000000000100000ULL, 0x0000000042700000ULL, _H(0x0000000042700000ULL), AP_RWNA, ATTR_WB }, /* Global shared */

  /* Terminator */
  /*                   SIZE,                  PADDR,                          VADDR,      AP,    ATTR */
    {0,                     0,                     0,                         0,       0       },
};
