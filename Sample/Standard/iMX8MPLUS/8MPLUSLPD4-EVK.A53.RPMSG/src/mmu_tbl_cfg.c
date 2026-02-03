/**
 * @file    mmu_tbl_cfg.c
 * @brief   MMU configuration table
 * @date    2025.12.19
 * @author  Copyright (c) 2021-2025, eForce Co., Ltd. All rights reserved.
 *
 ****************************************************************************
 * @par     History
 *          - rev 1.0 (2021.01.07) Imada
 *            Initial version.
 *          - rev 1.1 (2025.12.19)
 *            Fixed memory map to match multi core sample.
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
        {   0x0000000000100000ULL,  0x0000000097000000ULL,      0x0000000097000000ULL, AP_RWNA, ATTR_WB  }, /* Code */
        {   0x0000000000700000ULL,  0x0000000097100000ULL,      0x0000000097100000ULL, AP_RWNA, ATTR_WB  }, /* Data */
        {   0x0000000000100000ULL,  0x0000000097800000ULL,      0x0000000097800000ULL, AP_RWNA, ATTR_NC  }, /* Data(no-cache) */
        {   0x0000000000400000ULL,  0x0000000097900000ULL,      0x0000000097900000ULL, AP_RWNA, ATTR_WB  }, /* System data */
        {   0x0000000000100000ULL,  0x0000000097D00000ULL,      0x0000000097D00000ULL, AP_RWNA, ATTR_WB  }, /* MMU TTB data */
        {   0x0000000000200000ULL,  0x0000000097E00000ULL,      0x0000000097E00000ULL, AP_RWNA, ATTR_WB  }, /* Stack */
        {   0x0000000000800000ULL,  0x0000000098800000ULL,      0x0000000098800000ULL, AP_RWNA, ATTR_NC  }, /* RSC TBL */
        {   0x0000000001000000ULL,  0x0000000099000000ULL,      0x0000000099000000ULL, AP_RWNA, ATTR_NC  }, /* VRING,RPMSG */
        {   0x0000000001000000ULL,  0x000000009A000000ULL,      0x000000009A000000ULL, AP_RWNA, ATTR_WB  }, /* Shmem data */
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
        {   0x0000000000100000ULL,  0x0000000097000000ULL,      _H(0x0000000097000000ULL), AP_RWNA, ATTR_WB  }, /* Code */
        {   0x0000000000700000ULL,  0x0000000097100000ULL,      _H(0x0000000097100000ULL), AP_RWNA, ATTR_WB  }, /* Data */
        {   0x0000000000100000ULL,  0x0000000097800000ULL,      _H(0x0000000097800000ULL), AP_RWNA, ATTR_NC  }, /* Data(no-cache) */
        {   0x0000000000400000ULL,  0x0000000097900000ULL,      _H(0x0000000097900000ULL), AP_RWNA, ATTR_WB  }, /* System data */
        {   0x0000000000100000ULL,  0x0000000097D00000ULL,      _H(0x0000000097D00000ULL), AP_RWNA, ATTR_WB  }, /* MMU TTB data */
        {   0x0000000000200000ULL,  0x0000000097E00000ULL,      _H(0x0000000097E00000ULL), AP_RWNA, ATTR_WB  }, /* Stack */
        {   0x0000000000800000ULL,  0x0000000098800000ULL,      _H(0x0000000098800000ULL), AP_RWNA, ATTR_NC  }, /* RSC TBL */
        {   0x0000000001000000ULL,  0x0000000099000000ULL,      _H(0x0000000099000000ULL), AP_RWNA, ATTR_NC  }, /* VRING,RPMSG */
        {   0x0000000001000000ULL,  0x000000009A000000ULL,      _H(0x000000009A000000ULL), AP_RWNA, ATTR_WB  }, /* Shmem data */
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
