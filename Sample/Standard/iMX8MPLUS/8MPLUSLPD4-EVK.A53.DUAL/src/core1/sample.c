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
#include "GL_kernel_id.h"

/* External function prototypes ----------------------------------------------*/

extern ER   _ddr_xcore_mcore_init(void);
extern void hw_init(void);
#if !defined(__DEBUG)
extern void _sync_on_secondary(void);
#endif

/* External variables --------------------------------------------------------*/

extern UB SYSMEM[];
extern UB STKMEM[];
extern UB MPLMEM[];

/* Private function prototypes -----------------------------------------------*/

static void Core1Task(VP_INT exinf);
static void initpr(void);

/**
 * Core1Task
 */
void Core1Task(VP_INT exinf)
{
    while (1) {
        (void)vset_flg((ID)ID_CORE1, Core1FlagID, 0x0001U);
        // set_flg(Core1FlagID, 0x0001U) is also available because flag setting is done on this CPU core

        (void)dly_tsk(500U);
        (void)clr_flg(Core1FlagID, ~0x0001U);

        (void)vclr_flg((ID)ID_CORE1, Core1FlagID, ~0x0001U);
        // vclr_flg(Core1FlagID, ~0x0001U) is also available because flag setting is done on this CPU core

        (void)slp_tsk();
    }
}

/**
 * Initialize the devices and start the sample.
 */
static const T_CTSK ctsk_core1 = {TA_HLNG | TA_ACT | TA_FPU, (VP_INT)0, (FP)Core1Task, 8, 0x800, 0, "Core1Task"};
static const T_CFLG cflg = {TA_TFIFO | TA_WMUL, 0x00000000U, "Flg"};

static void initpr(void)
{
    extern const T_DEXC dexc_sync;
    extern const T_DEXC dexc_serr;

    (void)_ddr_xcore_mcore_init();

    (void)def_exc(EXC_SYN, (T_DEXC *)&dexc_sync);
    (void)def_exc(EXC_SER, (T_DEXC *)&dexc_serr);

    (void)_ddr_aarch64_gtimer_init(CFG_KRN_TICK, CFG_GTIMER_CLK);

    Core1TaskID = acre_tsk((T_CTSK *)&ctsk_core1);
    Core1FlagID = acre_flg((T_CFLG *)&cflg);
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
    _sync_on_secondary();
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
