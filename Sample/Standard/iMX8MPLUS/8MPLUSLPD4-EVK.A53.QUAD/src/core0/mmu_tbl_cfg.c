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

  /* CPU core #0 */
  /*                   SIZE,                  PADDR,                     VADDR,        AP,    ATTR */
    {0x0000000000100000ULL, 0x0000000040800000ULL, 0x0000000040800000ULL, AP_RWNA, ATTR_WB }, /* Code */
    {0x0000000000600000ULL, 0x0000000040C00000ULL, 0x0000000040C00000ULL, AP_RWNA, ATTR_WB }, /* Data */
    {0x0000000000100000ULL, 0x0000000042400000ULL, 0x0000000042400000ULL, AP_RWNA, ATTR_NC }, /* Data(no-cache) */
    {0x0000000000400000ULL, 0x0000000042800000ULL, 0x0000000042800000ULL, AP_RWNA, ATTR_WB }, /* System data */
    {0x0000000000100000ULL, 0x0000000043800000ULL, 0x0000000043800000ULL, AP_RWNA, ATTR_WB }, /* MMU TTB data */
    {0x0000000000200000ULL, 0x0000000043C00000ULL, 0x0000000043C00000ULL, AP_RWNA, ATTR_WB }, /* Stack(SP0, SP1) */

  /* Shared */
  /*                   SIZE,                  PADDR,                     VADDR,        AP,    ATTR */
    {0x0000000000100000ULL, 0x0000000044400000ULL, 0x0000000044400000ULL, AP_RWNA, ATTR_WB }, /* Sync */
    {0x0000000000100000ULL, 0x0000000044500000ULL, 0x0000000044500000ULL, AP_RWNA, ATTR_WB }, /* Global shared */

  /* Terminator */
  /*                   SIZE,                  PADDR,                  VADDR,            AP,    ATTR */
    {0,                     0,                     0,                     0,       0       },
};

#define _H(a) ((0xFFFFFF0000000000ULL) | (a))

const T_MEM_CFG mem_cfgtbl1[] = {
  /* Common area */
  /*                   SIZE,                  PADDR,                     VADDR,        AP,    ATTR */
    {0x0000000010000000ULL, 0x0000000030000000ULL, _H(0x0000000030000000ULL), AP_RWNA, ATTR_DEV}, /* I/O ports */

  /* CPU core #0 */
  /*                   SIZE,                  PADDR,                     VADDR,        AP,    ATTR */
    {0x0000000000100000ULL, 0x0000000040800000ULL, _H(0x0000000040800000ULL), AP_RWNA, ATTR_WB }, /* Code */
    {0x0000000000600000ULL, 0x0000000040C00000ULL, _H(0x0000000040C00000ULL), AP_RWNA, ATTR_WB }, /* Data */
    {0x0000000000100000ULL, 0x0000000042400000ULL, _H(0x0000000042400000ULL), AP_RWNA, ATTR_NC }, /* Data(no-cache) */
    {0x0000000000400000ULL, 0x0000000042800000ULL, _H(0x0000000042800000ULL), AP_RWNA, ATTR_WB }, /* System data */
    {0x0000000000100000ULL, 0x0000000043800000ULL, _H(0x0000000043800000ULL), AP_RWNA, ATTR_WB }, /* MMU TTB data */
    {0x0000000000200000ULL, 0x0000000043C00000ULL, _H(0x0000000043C00000ULL), AP_RWNA, ATTR_WB }, /* Stack(SP0, SP1) */

  /* Shared */
  /*                   SIZE,                  PADDR,                     VADDR,        AP,    ATTR */
    {0x0000000000100000ULL, 0x0000000044400000ULL, _H(0x0000000044400000ULL), AP_RWNA, ATTR_WB }, /* Sync */
    {0x0000000000100000ULL, 0x0000000044500000ULL, _H(0x0000000044500000ULL), AP_RWNA, ATTR_WB }, /* Global shared */

  /* Terminator */
  /*                   SIZE,                  PADDR,                  VADDR,            AP,    ATTR */
    {0,                     0,                     0,                         0,       0       },
};
