/**
 * @file    mmu_tbl_cfg.c
 * @brief   MMU configuration table
 * @date    2021.01.07
 * @author  Copyright (c) 2021, eForce Co., Ltd. All rights reserved.
 *
 ****************************************************************************
 * @par     History
 *          - rev 1.0 (2021.01.07) Imada
 *            Initial version.
 ****************************************************************************
 */
#include "DDR_AArch64_MMU.h"
#include "cpu_cfg.h"

const T_MEM_CFG mem_cfgtbl0[] = {
        /* Common area */
        /*                   SIZE,                  PADDR,                      VADDR,      AP,    ATTR */
        {   0x0000000010000000ULL,  0x0000000030000000ULL,      0x0000000030000000ULL, AP_RWNA, ATTR_DEV }, /* I/O ports */

#ifdef UC3BOOT_CORE3
        /* CPU core #0 */
        /*                   SIZE,                  PADDR,                      VADDR,      AP,    ATTR */
        {   0x0000000000100000ULL,  0x0000000095000000ULL,      0x0000000095000000ULL, AP_RWNA, ATTR_WB  }, /* Code */
        {   0x0000000000700000ULL,  0x0000000095100000ULL,      0x0000000095100000ULL, AP_RWNA, ATTR_WB  }, /* Data */
        {   0x0000000000100000ULL,  0x0000000095800000ULL,      0x0000000095800000ULL, AP_RWNA, ATTR_NC  }, /* Data(no-cache) */
        {   0x0000000000400000ULL,  0x0000000095900000ULL,      0x0000000095900000ULL, AP_RWNA, ATTR_WB  }, /* System data */
        {   0x0000000000100000ULL,  0x0000000095D00000ULL,      0x0000000095D00000ULL, AP_RWNA, ATTR_WB  }, /* MMU TTB data */
        {   0x0000000000200000ULL,  0x0000000095E00000ULL,      0x0000000095E00000ULL, AP_RWNA, ATTR_WB  }, /* Stack */
#else
        /* CPU core #0 */
        /*                   SIZE,                  PADDR,                      VADDR,      AP,    ATTR */
        {   0x0000000000100000ULL,  0x0000000040800000ULL,      0x0000000040800000ULL, AP_RWNA, ATTR_WB  }, /* Code */
        {   0x0000000000700000ULL,  0x0000000040900000ULL,      0x0000000040900000ULL, AP_RWNA, ATTR_WB  }, /* Data */
        {   0x0000000000100000ULL,  0x0000000041000000ULL,      0x0000000041000000ULL, AP_RWNA, ATTR_NC  }, /* Data(no-cache) */
        {   0x0000000000400000ULL,  0x0000000041100000ULL,      0x0000000041100000ULL, AP_RWNA, ATTR_WB  }, /* System data */
        {   0x0000000000100000ULL,  0x0000000041500000ULL,      0x0000000041500000ULL, AP_RWNA, ATTR_WB  }, /* MMU TTB data */
        {   0x0000000000200000ULL,  0x0000000041600000ULL,      0x0000000041600000ULL, AP_RWNA, ATTR_WB  }, /* Stack */
#endif /* #ifdef UC3BOOT_CORE3 */

        /* Terminator */
        /*                   SIZE,                  PADDR,                      VADDR,      AP,    ATTR */
        {                       0,                      0,                          0,       0,       0  },
};

#define _H(a) ((0xFFFFFF0000000000ULL) | (a))

const T_MEM_CFG  mem_cfgtbl1[] = {
        /* Common area */
        /*                   SIZE,                  PADDR,                          VADDR,      AP,     ATTR */
        {   0x0000000010000000ULL,  0x0000000030000000ULL,      _H(0x0000000030000000ULL), AP_RWNA, ATTR_DEV }, /* I/O ports */

#ifdef UC3BOOT_CORE3
        /* CPU core #0 */
        /*                   SIZE,                  PADDR,                          VADDR,      AP,    ATTR */
        {   0x0000000000100000ULL,  0x0000000095000000ULL,      _H(0x0000000095000000ULL), AP_RWNA, ATTR_WB  }, /* Code */
        {   0x0000000000700000ULL,  0x0000000095100000ULL,      _H(0x0000000095100000ULL), AP_RWNA, ATTR_WB  }, /* Data */
        {   0x0000000000100000ULL,  0x0000000095800000ULL,      _H(0x0000000095800000ULL), AP_RWNA, ATTR_NC  }, /* Data(no-cache) */
        {   0x0000000000400000ULL,  0x0000000095900000ULL,      _H(0x0000000095900000ULL), AP_RWNA, ATTR_WB  }, /* System data */
        {   0x0000000000100000ULL,  0x0000000095D00000ULL,      _H(0x0000000095D00000ULL), AP_RWNA, ATTR_WB  }, /* MMU TTB data */
        {   0x0000000000200000ULL,  0x0000000095E00000ULL,      _H(0x0000000095E00000ULL), AP_RWNA, ATTR_WB  }, /* Stack */
#else
         /* CPU core #0 */
        /*                   SIZE,                  PADDR,                          VADDR,      AP,    ATTR */
        {   0x0000000000100000ULL,  0x0000000040800000ULL,      _H(0x0000000040800000ULL), AP_RWNA, ATTR_WB  }, /* Code */
        {   0x0000000000700000ULL,  0x0000000040900000ULL,      _H(0x0000000040900000ULL), AP_RWNA, ATTR_WB  }, /* Data */
        {   0x0000000000100000ULL,  0x0000000041000000ULL,      _H(0x0000000041000000ULL), AP_RWNA, ATTR_NC  }, /* Data(no-cache) */
        {   0x0000000000400000ULL,  0x0000000041100000ULL,      _H(0x0000000041100000ULL), AP_RWNA, ATTR_WB  }, /* System data */
        {   0x0000000000100000ULL,  0x0000000041500000ULL,      _H(0x0000000041500000ULL), AP_RWNA, ATTR_WB  }, /* MMU TTB data */
        {   0x0000000000200000ULL,  0x0000000041600000ULL,      _H(0x0000000041600000ULL), AP_RWNA, ATTR_WB  }, /* Stack */
#endif /* #ifdef UC3BOOT_CORE3 */

        /* Terminator */
        /*                   SIZE,                  PADDR,                          VADDR,      AP,    ATTR */
        {                       0,                      0,                              0,       0,       0  },
};
