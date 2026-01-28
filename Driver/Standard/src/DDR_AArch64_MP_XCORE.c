/**
 * @file    DDR_AArch64_MP_XCORE.c
 * @brief   Micro C Cube Standard, XCore extension
 *          Driver for Synchronization and Asynchronization System Call
 * @date    2025.06.11
 * @author  Copyright (c) 2023-2025, eForce Co., Ltd.
 *
 ******************************************************************************
 * @par     History
 *          - rev 1.0 (2023.09.11)
 *            Created
 *          - rev 1.1 (2025.05.20)
 *            Add bss-initialization function.
 *            While waiting for the startup of other cores, interrupts are disabled.
 *            Add support for configuring the master core.
 *          - rev 1.2 (2025.06.11)
 *            Remove call to init_sync().
 *            Changed to use ddr_aarch64_gicv3_send_sgi_raw for Generates SGIs.
 ******************************************************************************
 */
#include <string.h>
#include "uC3sys.h"
#include "cpu_cfg.h"
#include "DDR_AArch64_MP_XCORE_cfg.h"
#if defined(USE_GICV2)
#include "DDR_AArch64_GIC.h"
#elif defined(USE_GICV3)
#include "DDR_AArch64_GICv3.h"
#endif
#include "uC3xcext.h"


/* External functions --------------------------------------------------------*/
extern void init_hsync();
extern void init_sync();
extern void init_global_data();
extern void init_domain_data();
extern UD get_mpidr();  /* See the GIC driver for details */

/* External variables --------------------------------------------------------*/

extern T_MCORE_ASYNCTBL _kernel_asynctbl[];
extern T_SYNCTBL _kernel_synctbl[];

/* Private functions -------------------------------------------------------------*/
/* Private variables -------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/

#define XCORE_MAGIC (0x12345678ULL)

#define MPIDR_MT_MASK (0x0000000001000000ULL) /* bit[24]: MT */

#if !defined(MASTER_CORE_ID)
#define MASTER_CORE_ID ID_CORE0
#endif

static ER ddr_sgi_init_func(ID domain_id, ID core_id, IMASK ipl);

static const T_XCORE_DOMAIN_CFG _kernel_domain_cfg[] = {
    {.core_max = CORE_NUM,
     .async_max = ASYNCBUF_NUM,
     .master_core = (ID)MASTER_CORE_ID,
     .sgi_intno = CFG_MCORE_SYSCALL_INTNO,
     .sgi_ipl = CFG_MCORE_SYSCALL_IPL,
     .sgi_initfunc = (FP)ddr_sgi_init_func,
     .ipi_intno = {-1, -1, -1, -1},
     .ipi_ipl = {0xFF, 0xFF, 0xFF, 0xFF},
     .ipi_initfunc = {0, 0, 0, 0}},
};

/**
 * SGI interrupt
 */
static void ddr_xcore_sgi_isr(VP_INT exinf)
{
    (void)exinf;
    _kernel_xcore_enqssb();
}

/**
 * SGI initialization for MP/XCore Communication
 */
static ER ddr_sgi_init_func(ID domain_id, ID core_id, IMASK ipl)
{
    T_CISR sgicall_cisr = {0};
    ER ercd = E_OK;

    sgicall_cisr.exinf = 0;
    sgicall_cisr.imask = ipl;
    sgicall_cisr.intno = _kernel_domain_cfg[0].sgi_intno;
    sgicall_cisr.isr = (FP)ddr_xcore_sgi_isr;
    sgicall_cisr.isratr = TA_NULL;
    ercd = acre_isr((T_CISR *)&sgicall_cisr);

    _kernel_xcore_ext.domain[0].asynctbl[core_id - 1] =
        ((XPTR)(ADDR)&_kernel_asynctbl[core_id - 1]);
    _kernel_xcore_ext.domain[0].synctbl[core_id - 1] =
        ((XPTR)(ADDR)&_kernel_synctbl[core_id - 1]);
    _kernel_asynctbl[core_id - 1].lock = 0U;
    _kernel_synctbl[core_id - 1].lock = 0U;

    if (ercd >= E_OK) {
        _kernel_xcore_ext.domain[0].ready[(UW)core_id - 1U] = 1U;
        _kernel_synch_cache();
        (void)ena_int(_kernel_domain_cfg[0].sgi_intno);
        ercd = E_OK;
    }
    return ercd;
}

/**
 * @brief
 * Zero-clear shared variables used by the XCore driver
 *
 * @param addr0
 * @param addr1
 */
void ini_bss(VP addr0, VP addr1)
{
    if (addr0 == 0) {
        return;
    }
    ADDR addrv0 = (ADDR)addr0;
    ADDR addrv1 = (ADDR)addr1;
    while (addrv1 > addrv0) {
        UD* vp1=(UD*)addrv0;
        *vp1 = 0U;
        addrv0 += 8U;
    }
}

/**
 * get domain core id
 *
 * @note only one domain is available for MPCore/XCore model
 */
ID get_domain_id(void)
{
    return (ID)1L;
}

/**
 * get hetero core id (for xcore kernel)
 */
ID get_hcid(void)
{
    return (ID)(((UW)get_domain_id() << 24) | ((UW)get_cid()));
}

/**
 * set event for inter-core system call
 */
void _kernel_async_sync_event_set(ID hcoreid)
{
#if defined(USE_GICV2)
    _ddr_aarch64_gic_send_sgi((UINT)TO_COREID(hcoreid),
                            _kernel_domain_cfg[0].sgi_intno);
#elif defined(USE_GICV3)
    UD mpidr = 0x0ULL;

    if (TO_COREID(hcoreid) >= 0x8) {
        return;
    }

    mpidr = get_mpidr();
    if ((mpidr & MPIDR_MT_MASK) != 0U) { /* Multithreaded core detected */
        /**
         * Support only if "MPIDR_EL1.AFF0: 0x0" and
         * "MPIDR_EL1.AFF1: core ID" are both satisfied.
         */
        ddr_aarch64_gic_sgir_t sgir = {0U};
        sgir.targetlist = 1U;
        sgir.aff1       = (UB)TO_COREID(hcoreid) - 1U;   /* target cpuid */
        sgir.intid      = (UB)_kernel_domain_cfg[0].sgi_intno;
        sgir.aff2       = (UB)((mpidr >> 16) & 0xFFU);
        sgir.irm        = 0U;   /* Do not broadcast.*/
        sgir.rs         = 0U;
        sgir.aff3       = (UB)((mpidr >> 32) & 0xFFU);
        ddr_aarch64_gicv3_send_sgi_raw(&sgir);
    } else {  /* Non-multithreaded core detected */
        ddr_aarch64_gicv3_send_sgi((0x1U << (UINT)TO_COREID(hcoreid)),
                                _kernel_domain_cfg[0].sgi_intno);
    }
#endif
    _kernel_synch_cache();
}

/**
 * clear event for inter-core system call (unused)
 */
void _kernel_async_sync_event_clear(void) {}

/**
 * abort event for inter-core system call (unused)
 */
void _kernel_async_sync_event_aborted(void) {}

/**
 * Initialize for inter-core system call
 */
ER _ddr_xcore_mcore_init(void)
{
    ER ercd = E_OK;
    volatile T_XCORE_EXT *xcore_ext =
        (volatile T_XCORE_EXT *)&_kernel_xcore_ext;

    if (CFG_DOMAIN_MAX > TMAX_DOMAIN) {
        ercd = E_PAR;
    } else {
        /* initialize all of xcore related bss sections */
        if (_kernel_domain_cfg[0].master_core == (ID)get_cid()) {
            init_hsync();
            init_global_data();
            init_domain_data();
            while ((*(volatile UD *)&xcore_ext->magic) != 0ULL) continue;
        }

        if (_kernel_domain_cfg[0].master_core == (ID)get_cid()) {
            xcore_ext->domain_max = CFG_DOMAIN_MAX;
            xcore_ext->sync_syscall_mode = CFG_SYNC_SYSCALL_MODE;
            xcore_ext->magic = XCORE_MAGIC;
            _kernel_synch_cache();
        }

        /* waiting for synchronization with other cores */
        while ((*(volatile UD *)&xcore_ext->magic) != XCORE_MAGIC) continue;

        xcore_ext->domain[0].core_max = _kernel_domain_cfg[0].core_max;
        xcore_ext->domain[0].async_max = _kernel_domain_cfg[0].async_max;

        if (_kernel_domain_cfg[0].sgi_initfunc != NULL) {
            ercd = ((ER(*)(ID, ID, IMASK))_kernel_domain_cfg[0].sgi_initfunc)(
                (ID)0L, (ID)get_cid(), _kernel_domain_cfg[0].sgi_ipl);
        }
    }

    return ercd;
}

/**
 * Wait until the other cores boot up
 */
void _kernel_start_multi_task(void)
{
    UINT idx = get_cid() - ID_CORE0;
    T_XCORE_EXT *xcore_ext = (T_XCORE_EXT *)&_kernel_xcore_ext;
    UINT sync_flag = 0U;
    UINT i;

#if defined(CFG_MCORE_SYNC_ID)
    sync_flag = CFG_MCORE_SYNC_ID;
#else
    for (i = xcore_ext->domain[0].core_max; i; i--) {
        sync_flag <<= 8U;
        sync_flag |= 2U;
    }
#endif

    _kernel_spin_lock(&xcore_ext->domain[0].lock);
    if ((idx * 8U) <= 24U) {
        xcore_ext->domain[0].sync_core |= (2U << (idx * 8U));
        _kernel_synch_cache();
    }
    _kernel_spin_unlock(&xcore_ext->domain[0].lock);

    _kernel_lock();
    while (*((volatile UINT *)&xcore_ext->domain[0].sync_core) != sync_flag)
        continue;
    _kernel_unlock();

    xcore_ext->domain[0].ready[idx] = 2U;

    xcore_ext->ready[0] = 2U;
    _kernel_synch_cache();
}
