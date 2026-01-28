/**
 * @brief   MMU configuration table
 * @date    2025.12.19
 *
 * @copyright (C) 2025, eForce Co., Ltd.
 */
#include "DDR_AArch64_MMU.h"
#include "cpu_cfg.h"

const T_MEM_CFG mem_cfgtbl0[] = {
        /* Common area */
        /*                   SIZE,                  PADDR,                      VADDR,      AP,    ATTR */
        {   0x0000000010000000ULL,  0x0000000030000000ULL,      0x0000000030000000ULL, AP_RWNA, ATTR_DEV }, /* I/O ports */

        /* CPU core #3 */
        /*                   SIZE,                  PADDR,                      VADDR,      AP,    ATTR */
        {   0x0000000000100000ULL,  0x0000000097000000ULL,      0x0000000097000000ULL, AP_RWNA, ATTR_WB  }, /* Code */
        {   0x0000000000700000ULL,  0x0000000097100000ULL,      0x0000000097100000ULL, AP_RWNA, ATTR_WB  }, /* Data */
        {   0x0000000000100000ULL,  0x0000000097800000ULL,      0x0000000097800000ULL, AP_RWNA, ATTR_NC  }, /* Data(no-cache) */
        {   0x0000000000400000ULL,  0x0000000097900000ULL,      0x0000000097900000ULL, AP_RWNA, ATTR_WB  }, /* System data */
        {   0x0000000000100000ULL,  0x0000000097D00000ULL,      0x0000000097D00000ULL, AP_RWNA, ATTR_WB  }, /* MMU TTB data */
        {   0x0000000000200000ULL,  0x0000000097E00000ULL,      0x0000000097E00000ULL, AP_RWNA, ATTR_WB  }, /* Stack */

        /* Shared */
        /*                   SIZE,                  PADDR,                      VADDR,      AP,    ATTR */
        {   0x0000000000100000ULL,  0x0000000098000000ULL,      0x0000000098000000ULL, AP_RWNA, ATTR_WB  }, /* Sync */
        {   0x0000000000100000ULL,  0x0000000098100000ULL,      0x0000000098100000ULL, AP_RWNA, ATTR_WB  }, /* Global shared */

        /* RPMSG */
        /*                   SIZE,                  PADDR,                      VADDR,      AP,    ATTR */
        {   0x0000000000800000ULL,  0x0000000098800000ULL,      0x0000000098800000ULL, AP_RWNA, ATTR_NC  }, /* RSC TBL */
        {   0x0000000001000000ULL,  0x0000000099000000ULL,      0x0000000099000000ULL, AP_RWNA, ATTR_NC  }, /* VRING,RPMSG */
        {   0x0000000001000000ULL,  0x000000009A000000ULL,      0x000000009A000000ULL, AP_RWNA, ATTR_WB  }, /* Shmem data */

        /* Terminator */
        /*                   SIZE,                  PADDR,                      VADDR,      AP,    ATTR */
        {                       0,                      0,                          0,       0,       0  },
};

#define _H(a) ((0xFFFFFF0000000000ULL) | (a))

const T_MEM_CFG  mem_cfgtbl1[] = {
        /* Common area */
        /*                   SIZE,                  PADDR,                          VADDR,      AP,     ATTR */
        {   0x0000000010000000ULL,  0x0000000030000000ULL,      _H(0x0000000030000000ULL), AP_RWNA, ATTR_DEV }, /* I/O ports */

        /* CPU core #3 */
        /*                   SIZE,                  PADDR,                          VADDR,      AP,    ATTR */
        {   0x0000000000100000ULL,  0x0000000097000000ULL,      _H(0x0000000097000000ULL), AP_RWNA, ATTR_WB  }, /* Code */
        {   0x0000000000700000ULL,  0x0000000097100000ULL,      _H(0x0000000097100000ULL), AP_RWNA, ATTR_WB  }, /* Data */
        {   0x0000000000100000ULL,  0x0000000097800000ULL,      _H(0x0000000097800000ULL), AP_RWNA, ATTR_NC  }, /* Data(no-cache) */
        {   0x0000000000400000ULL,  0x0000000097900000ULL,      _H(0x0000000097900000ULL), AP_RWNA, ATTR_WB  }, /* System data */
        {   0x0000000000100000ULL,  0x0000000097D00000ULL,      _H(0x0000000097D00000ULL), AP_RWNA, ATTR_WB  }, /* MMU TTB data */
        {   0x0000000000200000ULL,  0x0000000097E00000ULL,      _H(0x0000000097E00000ULL), AP_RWNA, ATTR_WB  }, /* Stack */

        /* Shared */
        /*                   SIZE,                  PADDR,                      VADDR,      AP,    ATTR */
        {   0x0000000000100000ULL,  0x0000000098000000ULL,      _H(0x0000000098000000ULL), AP_RWNA, ATTR_WB  }, /* Sync */
        {   0x0000000000100000ULL,  0x0000000098100000ULL,      _H(0x0000000098100000ULL), AP_RWNA, ATTR_WB  }, /* Global shared */

        /* RPMSG */
        /*                   SIZE,                  PADDR,                      VADDR,      AP,    ATTR */
        {   0x0000000000800000ULL,  0x0000000098800000ULL,      _H(0x0000000098800000ULL), AP_RWNA, ATTR_NC  }, /* RSC TBL */
        {   0x0000000001000000ULL,  0x0000000099000000ULL,      _H(0x0000000099000000ULL), AP_RWNA, ATTR_NC  }, /* VRING,RPMSG */
        {   0x0000000001000000ULL,  0x000000009A000000ULL,      _H(0x000000009A000000ULL), AP_RWNA, ATTR_WB  }, /* Shmem data */

        /* Terminator */
        /*                   SIZE,                  PADDR,                          VADDR,      AP,    ATTR */
        {                       0,                      0,                              0,       0,       0  },
};
