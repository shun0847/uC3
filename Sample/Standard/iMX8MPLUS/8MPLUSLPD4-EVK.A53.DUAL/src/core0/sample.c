/**
 * @brief   Sample program for 8MPLUSLPD4-EVK (i.MX8M Plus, Cortex-A53)
 * @date    2025.09.29
 *
 * @copyright (C) 2025, eForce Co., Ltd.
 */
#include "kernel.h"
#include "DDR_AArch64_GICv3.h"
#include "DDR_AArch64_GTIMER.h"
#include "DDR_AArch64_GTIMER_cfg.h"
#include "kernel_cfg.h"

/* External function prototypes ----------------------------------------------*/

extern ER   _ddr_xcore_mcore_init(void);
extern void hw_init(void);
extern void sample_uart_start(void);
#if !defined(__DEBUG)
extern void _sync_on_primary(void);
#endif

/* External variables --------------------------------------------------------*/

extern UB SYSMEM[];
extern UB STKMEM[];
extern UB MPLMEM[];

/* Private function prototypes -----------------------------------------------*/

static void initpr(void);

/**
 * Initialize the devices and start the sample.
 */
static void initpr(void)
{
    extern const T_DEXC dexc_sync;
    extern const T_DEXC dexc_serr;

    (void)_ddr_xcore_mcore_init();

    (void)def_exc(EXC_SYN, (T_DEXC *)&dexc_sync);
    (void)def_exc(EXC_SER, (T_DEXC *)&dexc_serr);

    (void)_ddr_aarch64_gtimer_init(CFG_KRN_TICK, CFG_GTIMER_CLK);

    sample_uart_start();
}

/**
 * Initialize uC3 and the devices, then start the sample.
 */
ER main(void)
{
    ER     ercd;
    T_CSYS csys;

    /* CPU core synchronization */
#if !defined(__DEBUG)
    _sync_on_primary();
#endif

    /** Dummy */
    SYSMEM[0] = 0U;
    STKMEM[0] = 0U;
    MPLMEM[0] = 0U;

    hw_init();

    ercd = ddr_aarch64_gicv3_init();
    if (ercd) {
        return ercd;
    }

    csys.tskpri_max = CFG_KRN_TSKPRI_MAX;
    csys.tskid_max = CFG_KRN_TSKID_MAX;
    csys.semid_max = CFG_KRN_SEMID_MAX;
    csys.flgid_max = CFG_KRN_FLGID_MAX;
    csys.dtqid_max = CFG_KRN_DTQID_MAX;
    csys.mbxid_max = CFG_KRN_MBXID_MAX;
    csys.mtxid_max = CFG_KRN_MTXID_MAX;
    csys.mbfid_max = CFG_KRN_MBFID_MAX;
    csys.porid_max = CFG_KRN_PORID_MAX;
    csys.mpfid_max = CFG_KRN_MPFID_MAX;
    csys.mplid_max = CFG_KRN_MPLID_MAX;
    csys.almid_max = CFG_KRN_ALMID_MAX;
    csys.cycid_max = CFG_KRN_CYCID_MAX;
    csys.isrid_max = CFG_KRN_ISRID_MAX;
    csys.devid_max = CFG_KRN_DEVID_MAX;
    csys.tick = CFG_KRN_TICK;
    csys.ssb_num = CFG_KRN_SSB_NUM;
    csys.sysmem_top = (VP)&SYSMEM[0];
    csys.sysmem_end = (VP)&SYSMEM[CFG_KRN_SYSMEM_SZ];
    csys.stkmem_top = (VP)&STKMEM[0];
    csys.stkmem_end = (VP)&STKMEM[CFG_KRN_STKMEM_SZ];
    csys.mplmem_top = (VP)&MPLMEM[0];
    csys.mplmem_end = (VP)&MPLMEM[CFG_KRN_MPLMEM_SZ];
    csys.sysidl = SYSTEM_IDLE;
    csys.inistk = STACK_ID_INIT;
    csys.trace = TRACE_DISABLE;
    csys.agent = AGENT_DISABLE;

    ercd = start_uC3(&csys, initpr);

    /* It does not reach */
    return ercd;
}
