/**
 * @brief   CPU core synchronization on the primary CPU core
 *          for 8MPLUSLPD4-EVK (i.MX8M Plus, Cortex-A53)
 * @date    2025.09.29
 *
 * @copyright (C) 2025, eForce Co., Ltd.
 */

#define IOMUXC_GPR_GPR4   (0x30340010)


        .balign  8
        .section .text

/**
 * CPU core synchronization on the primary CPU core
 * void _sync_on_primary(void)
 */
        .globl  _sync_on_primary
        .type   _sync_on_primary, @function

_sync_on_primary:
        sev
        dsb     sy
        nop
        nop
        wfe
        nop

        ret
