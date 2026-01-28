/**
 * @file    DDR_AArch64_GTIMER.c
 * @brief   Micro C Cube Standard, DEVICE DRIVER
 *          ARM Generic Timer
 * @date    2025.06.11
 * @author  Copyright (c) 2016-2025, eForce Co., Ltd. All rights reserved.
 *
 ****************************************************************************
 * @par     History
 *          - rev 1.0 (2016.09.27) y-kim
 *            Created based on DDR_CortexA_GTIMER.c
 *          - rev 1.1 (2018.05.29) yokota
 *            implement _kernel_micro_systim
 *          - rev 1.2 (2021.01.19) Imada
 *            Fixed C++ test warningso
 *          - rev 1.3 (2023.07.14)
 *            Fixed overflow on tmr_rate calculation
 *          - rev 1.4 (2025.05.20)
 *            Fixed the issue of timer counter overflow during ICE breaks.
 *            Add support for configuring the master core.
 *          - rev 1.5 (2025.06.11)
 *            Fix timer calculation of _kernel_micro_systim.
 ****************************************************************************
 */
#include "kernel.h"
#include "DDR_AArch64_GTIMER.h"
#include "DDR_AArch64_GTIMER_cfg.h"

/* External function prototypes ----------------------------------------------*/

extern UW cp15_get_cpuid_pfr1(void);
extern void cp15_set_cntp_ctl(UW val);
extern UW cp15_get_cntp_ctl(void);
extern void cp15_set_cntp_tval(UW val);
extern W cp15_get_cntp_tval(void);
extern void cp15_set_cntfrq(UW val);

/* Private function prototypes -----------------------------------------------*/

static BOOL is_supported(void);
static void tmr_start(void);
static void tmr_stop(void);
static void tmr_isr(VP_INT exinf);

/* Private typedef -----------------------------------------------------------*/

/* ARM generic counter */
struct t_gcnt {
    UW CNTCR;       /* counter control register */
    UW CNTSR;       /* counter status register  */
    UW CNTCV0;      /* counter count value register (lower 32bit) */
    UW CNTCV1;      /* counter count value register (upper 32bit) */
    UW reserved[4]; /* reserved */
    UW CNTFID0;     /* frequency modes table   */
                    /* 0 is the base frequency */
};

/* Private macro -------------------------------------------------------------*/

/* ARM Generic Counter CNTCR (Counter Control Register) */
#define CNTCR_ENABLE        (1U << 0)    /* Enable*/
#define CNTCR_HDBG          (1U << 1)    /* Halt on debug */

/* CP15 CNTP_CTL (PL1 Physical Timer Control register) */
#define CNTP_CTRL_ENABLE    (1U << 0)
#define CNTP_CTRL_INT_MASK  (1U << 1)
#define CNTP_CTRL_INT_STAT  (1U << 2)

/* CP15 ID_PFR1 (Processor Feature Register 1) */
#define PFR1_GTIMER_MASK    0x000f0000U
#define PFR1_GTIMER_IMPL    0x00010000U
#define PFR1_GTIMER_NO_IMPL 0x00000000U

// check for master core configuration
#if !defined(MASTER_CORE_ID)
#define MASTER_CORE_ID ID_CORE0
#endif

/* Private variables ---------------------------------------------------------*/

static UW g_tmr_systim = 0U;
static UW g_tmr_rate = 0U;
static UW g_tmr_tick = 0U;
static W g_tmr_suspend = 0;

/**
 * Check generic timer support
 * @return TRUE or FALSE
 */
static BOOL is_supported(void)
{
    return ((cp15_get_cpuid_pfr1() & PFR1_GTIMER_MASK) == PFR1_GTIMER_IMPL)
            ? (TRUE)
            : (FALSE);
}

/**
 * Enable generic timer
 */
static void tmr_start(void)
{
    UW ctrl;

    ctrl = cp15_get_cntp_ctl();
    ctrl &= ~CNTP_CTRL_INT_MASK;
    ctrl |= CNTP_CTRL_ENABLE;
    cp15_set_cntp_ctl(ctrl);
}

/**
 * Disable generic timer
 */
static void tmr_stop(void)
{
    UW ctrl;

    ctrl = cp15_get_cntp_ctl();
    ctrl |= CNTP_CTRL_INT_MASK;
    ctrl &= ~CNTP_CTRL_ENABLE;
    cp15_set_cntp_ctl(ctrl);
}

/**
 * Timer event has triggered
 */
static void tmr_isr(VP_INT exinf)
{
    W reload = 0;
    W cur_tval = 0;

    (void)loc_cpu();

    /* After an event has triggered, a read of a TimerValue register
     * indicates the time since the event triggered.
     * So, subtract current CNTP_TVAL.
     */
    cur_tval = cp15_get_cntp_tval();
    if (cur_tval < 0) {
        reload = (W)g_tmr_rate + cur_tval;
        if (reload <= 0) {
            /*
             * In the event of a delay exceeding the timer count,
             * the timer handler should be reset to its reference value.
             *
             * If tms_isr() is called 1 ms after the interrupt arrival,
             * forcibly treat it as 1 ms elapsed.
             */
            reload = (W)g_tmr_rate;
        }
    } else {
        /* This state should never occur under any valid conditions */
        if (cur_tval < (W)g_tmr_rate) {
            reload = (W)g_tmr_rate - cur_tval;
        } else{
            reload = (W)g_tmr_rate;
        }
    }
    cp15_set_cntp_tval(reload - 1U);
    vdsb();

    g_tmr_systim += g_tmr_tick;
    vdsb();

    (void)unl_cpu();

    (void)isig_tim();
}

/**
 * Initialize ARM generic timer
 * @param tick      timer event cycle (ms)
 * @param base_clk  base frequency (hz)
 * @return E_OK or error code
 */
ER _ddr_aarch64_gtimer_init(UINT tick, UW base_clk)
{
    ER ercd;

    if (is_supported() == FALSE) {
        ercd = E_NOSPT;
    } else {
        T_CISR tmr_cisr = {TA_HLNG, (VP_INT)0, CFG_GTIMER_INTNO, (FP)tmr_isr,
                           CFG_GTIMER_IPL};

        ercd = acre_isr(&tmr_cisr);
        if (ercd >= E_OK) {
            BOOL iflag;
            UW cntp_ctl;
            UINT cid;

            cntp_ctl = cp15_get_cntp_ctl();
            cntp_ctl &= ~CNTP_CTRL_ENABLE;
            cntp_ctl |= CNTP_CTRL_INT_MASK;
            cp15_set_cntp_ctl(cntp_ctl);

            iflag = sns_loc();
            if (!iflag) {
                (void)loc_cpu();
            }

            vdsb();
            g_tmr_tick = (tick * 1000U);
            g_tmr_rate = (UW)((UD)((UD)tick * (UD)base_clk) / (UD)1000U);
            g_tmr_systim = 0U;
            vdsb();

            cntp_ctl &= ~CNTP_CTRL_INT_MASK;
            cntp_ctl |= CNTP_CTRL_ENABLE;
            cp15_set_cntp_tval(g_tmr_rate - 1U);
            cp15_set_cntp_ctl(cntp_ctl); /* enable down timer */

            /*
             * arch timer is started.
             * only master core
             */
            cid = vget_cid();
            if (cid == MASTER_CORE_ID) {
#ifndef CFG_NO_SYSTEM_COUNTER
                volatile struct t_gcnt *reg =
                    (volatile struct t_gcnt *)CFG_GTIMER_REG_BASE;

                /*
                 * Update the timer if it is not at the right frequency.
                 */
                if (reg->CNTFID0 != base_clk) {
                    reg->CNTFID0 = base_clk;
                }
                reg->CNTCR |= (CNTCR_ENABLE);
#endif /* CFG_NO_SYSTEM_COUNTER */
            }

            if (!iflag) {
                (void)unl_cpu();
            }

            (void)ena_int(CFG_GTIMER_INTNO);
            ercd = E_OK;
        }
    }
    return ercd;
}

/*=====================================================================*/

/**
 * Kernel internal functions
 * should not call following functions in user application.
 */

/**
 * Read the system time by a microsecond unit
 * @return system time (us)
 */
UW _kernel_micro_systim(void)
{
    UW clk = 0U;
    UW tcn = 0U;
    static UW pre_clk = 0U;

    if (g_tmr_tick != 0U) {
        if ((cp15_get_cntp_ctl() & CNTP_CTRL_INT_STAT) != 0U) {
            clk = g_tmr_systim + g_tmr_tick;
            tcn = (UW)(-cp15_get_cntp_tval());
        } else {
            clk = g_tmr_systim;
            tcn = g_tmr_rate - (UW)cp15_get_cntp_tval();
            if ((cp15_get_cntp_ctl() & CNTP_CTRL_INT_STAT) != 0U) {
                clk += g_tmr_tick;
                tcn = (UW)(-cp15_get_cntp_tval());
            }
        }
        clk += ((tcn * g_tmr_tick) / g_tmr_rate);
    }
    if (clk < pre_clk) {
        clk += g_tmr_tick;
    }
    pre_clk = clk;

    return clk;
}

/**
 * Stop the system timer
 */
void _kernel_micro_systim_stop(void)
{
    g_tmr_suspend = cp15_get_cntp_tval();
    tmr_stop();
}

/**
 * Restart the system timer
 */
void _kernel_micro_systim_start(void)
{
    cp15_set_cntp_tval((UW)g_tmr_suspend);
    tmr_start();
}
