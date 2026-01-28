/**
 * @brief   Sample program for 8MPLUSLPD4-EVK (i.MX8M Plus, Cortex-A53)
 * @date    2025.12.19
 *
 * @copyright (C) 2025, eForce Co., Ltd.
 */
#include "kernel.h"
#include "DDR_AArch64_GICv3.h"
#include "DDR_AArch64_GTIMER.h"
#include "DDR_AArch64_GTIMER_cfg.h"
#include "kernel_cfg.h"
#include "rsc_table.h"

/* External function prototypes ----------------------------------------------*/

extern ER _ddr_xcore_mcore_init(void);
extern void hw_init(void);
extern void sample_rpmsg(void);
extern void synchronous_exception_handler(UD esr, UD far, UD sp);
extern void system_error_handler(UD esr, UD far, UD sp);
#if !defined(__DEBUG)
extern void _sync_on_primary(void);
#endif

/* External variables --------------------------------------------------------*/

extern UB SYSMEM[];
extern UB STKMEM[];
extern UB MPLMEM[];

extern struct remote_resource_table rsc_table[NUM_RSCTBLS];
extern struct remote_resource_table resources[NUM_RSCTBLS];

/* Private function prototypes -----------------------------------------------*/

//static void MainTask(VP_INT exinf);
static void initpr(void);

/*
 * Main task
 */
void MainTask(VP_INT exinf)
{
    for(;;) {
        (void)dly_tsk(500U);
    }
}

/*
 * Initialize the devices and start the sample.
 */
static void initpr(void)
{
    const T_DEXC dexc_sync = {
        TA_HLNG,
        (FP)synchronous_exception_handler
    };

    const T_DEXC dexc_serr = {
        TA_HLNG,
        (FP)system_error_handler
    };

    const T_CTSK ctsk_main = {
        TA_HLNG | TA_ACT | TA_FPU,
        (VP_INT)NULL,
        (FP)MainTask,
        3,
        0x800U,
        (VP)NULL,
        "MainTask"
    };

    (void)_ddr_xcore_mcore_init();
    (void)def_exc(EXC_SYN, (T_DEXC *)&dexc_sync);
    (void)def_exc(EXC_SER, (T_DEXC *)&dexc_serr);
    (void)_ddr_aarch64_gtimer_init(CFG_KRN_TICK, CFG_GTIMER_CLK);
    (void)acre_tsk((T_CTSK *)&ctsk_main);
    sample_rpmsg();
}

/*
 * Initialize uC3 and the devices, then start the sample.
 */
ER main(void)
{
    ER ercd;
    T_CSYS csys;

    /* CPU core synchronization */
#if !defined(__DEBUG)
    _sync_on_primary();
#endif

    /* Dummy */
    SYSMEM[0] = 0U;
    STKMEM[0] = 0U;
    MPLMEM[0] = 0U;

    memcpy(rsc_table, resources, sizeof(resources));

    hw_init();

    ercd = ddr_aarch64_gicv3_init();
    if (ercd) {
        return ercd;
    }

    csys.tskpri_max = CFG_KRN_TSKPRI_MAX;
    csys.tskid_max  = CFG_KRN_TSKID_MAX;
    csys.semid_max  = CFG_KRN_SEMID_MAX;
    csys.flgid_max  = CFG_KRN_FLGID_MAX;
    csys.dtqid_max  = CFG_KRN_DTQID_MAX;
    csys.mbxid_max  = CFG_KRN_MBXID_MAX;
    csys.mtxid_max  = CFG_KRN_MTXID_MAX;
    csys.mbfid_max  = CFG_KRN_MBFID_MAX;
    csys.porid_max  = CFG_KRN_PORID_MAX;
    csys.mpfid_max  = CFG_KRN_MPFID_MAX;
    csys.mplid_max  = CFG_KRN_MPLID_MAX;
    csys.almid_max  = CFG_KRN_ALMID_MAX;
    csys.cycid_max  = CFG_KRN_CYCID_MAX;
    csys.isrid_max  = CFG_KRN_ISRID_MAX;
    csys.devid_max  = CFG_KRN_DEVID_MAX;
    csys.tick       = CFG_KRN_TICK;
    csys.ssb_num    = CFG_KRN_SSB_NUM;
    csys.sysmem_top = (VP)&SYSMEM[0];
    csys.sysmem_end = (VP)&SYSMEM[CFG_KRN_SYSMEM_SZ];
    csys.stkmem_top = (VP)&STKMEM[0];
    csys.stkmem_end = (VP)&STKMEM[CFG_KRN_STKMEM_SZ];
    csys.mplmem_top = (VP)&MPLMEM[0];
    csys.mplmem_end = (VP)&MPLMEM[CFG_KRN_MPLMEM_SZ];
    csys.sysidl     = SYSTEM_IDLE;
    csys.inistk     = STACK_ID_INIT;
    csys.trace      = TRACE_DISABLE;
    csys.agent      = AGENT_DISABLE;

    ercd = start_uC3(&csys, initpr);

    /* It does not reach */
    return ercd;
}
