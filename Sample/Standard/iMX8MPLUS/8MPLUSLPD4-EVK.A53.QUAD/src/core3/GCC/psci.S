/**
 * @brief   PSCI client interface (AArch64)
 * @date    2025.09.29
 *
 * @copyright (C) 2025, eForce Co., Ltd.
 */


        .balign 8
        .text

/**
 * SMC(Secure Monitor Call) based PSCI client interface
 * Considering only the four arguments specified by x0-x3
 * void _psci_smc(uint32_t funcid, uint64_t x1, uint64_t x2, uint64_t x3)
 */
        .global  _psci_smc
        .type    _psci_smc, @function

_psci_smc:
        /* set x4-x7 as 0x0 */
        ldr     x4, =0x0            /* Parameter x4 */
        ldr     x5, =0x0            /* Parameter x5 */
        ldr     x6, =0x0            /* Parameter x6 */
        ldr     x7, =0x0            /* Parameter x7 */
        smc     #0
        dsb     sy
        isb

        ret
