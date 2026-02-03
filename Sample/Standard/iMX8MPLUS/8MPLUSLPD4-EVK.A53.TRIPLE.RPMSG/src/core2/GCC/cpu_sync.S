/**
 * @brief   CPU core synchronization on a secondary CPU core
 *          for 8MPLUSLPD4-EVK (i.MX8M Plus, Cortex-A53)
 * @date    2025.09.29
 *
 * @copyright (C) 2025, eForce Co., Ltd.
 */


        .balign  8
        .section .text

/**
 * CPU core synchronization on a secondary CPU core
 * void _sync_on_secondary(void)
 */
        .globl  _sync_on_secondary
        .type   _sync_on_secondary, @function

_sync_on_secondary:
        dsb     sy
        nop
        nop
        wfe
        nop

        ret
