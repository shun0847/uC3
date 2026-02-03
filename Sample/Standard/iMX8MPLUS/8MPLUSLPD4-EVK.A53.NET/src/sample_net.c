/**
 * @file    sample_net.c
 * @brief   uNet3 Sample program for 8MPLUSLPD4-EVK (i.MX8M Plus, Cortex-A53)
 * @date    2025.10.03
 * @author  Copyright (c) 2025, eForce Co., Ltd. All rights reserved.
 *
 ****************************************************************************
 * @par     History
 *          - rev 1.0 (2025.10.03)
 *            Initial version.
 ****************************************************************************
 */
#include "kernel.h"
#include "DDR_AArch64_GICv3.h"
#include "DDR_AArch64_GTIMER.h"
#include "DDR_AArch64_GTIMER_cfg.h"
#include "kernel_cfg.h"
#include "net_hdr.h"
#include "net_strlib.h"
#include "DDR_COM.h"
#include "sample_uart_cfg.h"

/* External function prototypes ----------------------------------------------*/

extern ER net_setup(void);
extern ER net_app_ini(void);
extern void hw_init(void);
extern void sample_uart_start(void);
extern void synchronous_exception_handler(UD esr, UD far, UD sp);
extern void system_error_handler(UD esr, UD far, UD sp);

/* External variables --------------------------------------------------------*/
#if SAMPLE_ENA_HTTPd
extern TMO LedTmo;
#else
TMO LedTmo = 500;
#endif
VB const* const demo_str = "\n\r\teForce Operating System Sample Program "
                           "V1.0\n\r\t\tSerial Port (UART1)\r\n";

extern UB SYSMEM[];
extern UB STKMEM[];
extern UB MPLMEM[];

/* Private function prototypes -----------------------------------------------*/

static void MainTask(VP_INT exinf);
static void initpr(void);


void puts_com_opt(VB* msg)
{
    UINT txcnt;
    txcnt = (UINT)net_strlen(msg);
    (void)puts_com((ID)ID_UART, msg, &txcnt, TMO_FEVR);
}

/*
 * Main task
 */
void MainTask(VP_INT exinf)
{
    UINT txcnt;
    ER ercd;
    const T_COM_SMOD uart_ini = {CFG_BAUDRATE, CFG_BLEN, CFG_PAR, CFG_SBIT,
                                 CFG_FLW};

    (void)DDR_UART_INIT_FN(ID_UART, &REG_UART);

    (void)ini_com((ID)ID_UART, &uart_ini);
    (void)ctr_com((ID)ID_UART, STA_COM, 0);

    (void)ctr_com((ID)ID_UART, SND_BRK, 100);
    txcnt = net_strlen(demo_str);
    (void)puts_com((ID)ID_UART, (VB*)demo_str, &txcnt, TMO_FEVR);

    /* Initialize Network */
    ercd = net_setup();
    if (ercd != E_OK) {
        return;
    }
    
    /* Initialize Sample Application */
    ercd = net_app_ini();
    if (ercd != E_OK) {
        return;
    }

    for(;;) {
        (void)dly_tsk((RELTIM)LedTmo);
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

    (void)def_exc(EXC_SYN, (T_DEXC *)&dexc_sync);
    (void)def_exc(EXC_SER, (T_DEXC *)&dexc_serr);
    (void)_ddr_aarch64_gtimer_init(CFG_KRN_TICK, CFG_GTIMER_CLK);
    (void)acre_tsk((T_CTSK *)&ctsk_main);
    //sample_uart_start();
}

/*
 * Initialize uC3 and the devices, then start the sample.
 */
ER main(void)
{
    ER ercd;
    T_CSYS csys;

    /* Dummy */
    SYSMEM[0] = 0U;
    STKMEM[0] = 0U;
    MPLMEM[0] = 0U;

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
