/**
 * @brief   Micro C Cube Standard, XCore extension compiler specifc
 *          Synchronization and Asynchronization System Call
 * @date    2023.09.11
 *
 * @copyright (c) 2023, eForce Co., Ltd.
 */
#include "uC3sys.h"
#include "uC3xcext.h"

#include "DDR_AArch64_MP_XCORE_cfg.h"

/* External variables --------------------------------------------------------*/

T_XCORE_EXT _kernel_xcore_ext __attribute__((section(".hsync")));

/* Sync & Async Table Implementation for Domain 0 */
T_MCORE_ASYNCTBL _kernel_asynctbl[CORE_NUM] __attribute__((section(".hsync")));
T_SYNCTBL _kernel_synctbl[CORE_NUM] __attribute__((section(".hsync")));

/* External functions --------------------------------------------------------*/
extern void ini_bss(VP, VP); /* use external ini_bss defined at prst.S */

/* External Symbol --------------------------------------------------------*/

/**
 * linker script symbols
 * These are must be 4byte aligned.
 */
extern UB __hsync_start[]; /* start address of multicore kernel sync area */
extern UB __hsync_end[];   /* end address of multicore kernel sync area */
extern UB __sync_start[];  /* start address of sync area (for gic dirver) */
extern UB __sync_end[];    /* end address of sync area (for gic dirver) */
extern UB __domain_shared_bss_start[]; /* start address of domain shared data */
extern UB __domain_shared_bss_end[];   /* end address of domain shared data */
extern UB __global_shared_bss_start[]; /* start address of global shared data */
extern UB __global_shared_bss_end[];   /* end address of global shared data */


void init_hsync(void)
{
    if ((ADDR)__hsync_start < (ADDR)__hsync_end) {
        ini_bss((VP)__hsync_start, (VP)__hsync_end);
    }
}

void init_sync(void)
{
    if ((ADDR)__sync_start < (ADDR)__sync_end) {
        ini_bss((VP)__sync_start, (VP)__sync_end);
    }
}

void init_global_data(void)
{
    if ((ADDR)__global_shared_bss_start < (ADDR)__global_shared_bss_end) {
        ini_bss((VP)__global_shared_bss_start, (VP)__global_shared_bss_end);
    }
}

void init_domain_data(void)
{
    if ((ADDR)__domain_shared_bss_start < (ADDR)__domain_shared_bss_end) {
        ini_bss((VP)__domain_shared_bss_start, (VP)__domain_shared_bss_end);
    }
}
