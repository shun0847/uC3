/**
 * @file    uC3hook.c
 * @brief   Micro C Cube Standard, KERNEL
 *          Hook Routine
 * @date    2021.02.02
 * @author  Copyright (c) 2016-2021, eForce Co., Ltd. All rights reserved.
 *
 ******************************************************************************
 * @par     History
 *          - rev 1.0 (2016.09.21) y-kim
 *            Initial version.
 *          - rev 1.1 (2017.04.25) yokota
 *            Fixed the IPA warnings.
 *          - rev 1.2 (2018.05.29) yokota
 *            change pc parameter type from UD to ADDR..
 *          - rev 1.3 (2018.06.01) yokota
 *            chane func type to INT. func code type to FN, ercd to ER.
 *          - rev 1.4 (2021.02.02) Imada
 *            Fixed C++test warnings.
 ******************************************************************************
 */
#include "uC3sys.h"

/* External function prototypes ----------------------------------------------*/

extern UW _kernel_systrace_disable_irq(void);
extern void _kernel_systrace_restore_irq(UW imask);

/* Private typedef -----------------------------------------------------------*/

typedef ER (*FP_P0)(void);
typedef ER (*FP_P1)(UD p1);
typedef ER (*FP_P2)(UD p1, UD p2);
typedef ER (*FP_P3)(UD p1, UD p2, UD p3);
typedef ER (*FP_P4)(UD p1, UD p2, UD p3, UD p4);
typedef ER (*FP_P5)(UD p1, UD p2, UD p3, UD p4, UD p5);
typedef void (*FP_SYSTRACE)(UW cur_ctxid, FN func, UW time, ADDR pc, UH count, UD *para);
typedef void (*FP_RETTRACE)(UW cur_ctxid, FN func, UW time, ADDR pc, ER ercd);

/* Private macro -------------------------------------------------------------*/

#define disable_irq()          (_kernel_systrace_disable_irq())
#define restore_irq(imask)     (_kernel_systrace_restore_irq(imask))

/*
 * System Call Hook Routine (no parameter)
 */
ER _kernel_systrace_p0(FP func, INT code, ADDR pc)
{
    _kernel_save_pc(pc);
    FP_SYSTRACE systrace = (FP_SYSTRACE)_kernel_systbl.systrace;
    FN func_cd = ((INT)((UINT)code >> 8) | 0xFF000000);
    if (systrace != 0)
    {
        UW imask = disable_irq();
        UW clk = _kernel_micro_systim();
        systrace(_kernel_systbl.ctxid_s, func_cd, clk, pc, 0U, 0);
        restore_irq(imask);
    }

    ER ercd = ((FP_P0)func)();

    FP_RETTRACE rettrace = (FP_RETTRACE)_kernel_systbl.rettrace;
    if (rettrace != 0)
    {
        UW imask = disable_irq();
        UW clk = _kernel_micro_systim();
        rettrace(_kernel_systbl.ctxid_s, func_cd, clk, pc, ercd);
        restore_irq(imask);
    }

    return ercd;
}

/*
 * System Call Hook Routine (1 parameter)
 */
ER _kernel_systrace_p1(FP func, INT code, UD p1, ADDR pc)
{
    _kernel_save_pc(pc);
    FP_SYSTRACE systrace = (FP_SYSTRACE)_kernel_systbl.systrace;
    FN func_cd = ((INT)((UINT)code >> 8) | 0xFF000000);
    if (systrace != 0)
    {
        UW imask = disable_irq();
        UW clk = _kernel_micro_systim();
        UD para[1U] = { p1 };
        UH count = (UH)code & 0xffU;
        systrace(_kernel_systbl.ctxid_s, func_cd, clk, pc, count, para);
        restore_irq(imask);
    }

    ER ercd = ((FP_P1)func)(p1);

    FP_RETTRACE rettrace = (FP_RETTRACE)_kernel_systbl.rettrace;
    if (rettrace != 0)
    {
        UW imask = disable_irq();
        UW clk = _kernel_micro_systim();
        rettrace(_kernel_systbl.ctxid_s, func_cd, clk, pc, ercd);
        restore_irq(imask);
    }

    return ercd;
}

/*
 * System Call Hook Routine (2 parameter)
 */
ER _kernel_systrace_p2(FP func, INT code, UD p1, UD p2, ADDR pc)
{
    _kernel_save_pc(pc);
    FP_SYSTRACE systrace = (FP_SYSTRACE)_kernel_systbl.systrace;
    FN func_cd = ((INT)((UINT)code >> 8) | 0xFF000000);
    if (systrace != 0)
    {
        UW imask = disable_irq();
        UW clk = _kernel_micro_systim();
        UD para[2U] = { p1, p2 };
        UH count = (UH)code & 0xffU;
        systrace(_kernel_systbl.ctxid_s, func_cd, clk, pc, count, para);
        restore_irq(imask);
    }

    ER ercd = ((FP_P2)func)(p1, p2);

    FP_RETTRACE rettrace = (FP_RETTRACE)_kernel_systbl.rettrace;
    if (rettrace != 0)
    {
        UW imask = disable_irq();
        UW clk = _kernel_micro_systim();
        rettrace(_kernel_systbl.ctxid_s, func_cd, clk, pc, ercd);
        restore_irq(imask);
    }

    return ercd;
}

/*
 * System Call Hook Routine (3 parameter)
 */
ER _kernel_systrace_p3(FP func, INT code, UD p1, UD p2, UD p3, ADDR pc)
{
    _kernel_save_pc(pc);
    FP_SYSTRACE systrace = (FP_SYSTRACE)_kernel_systbl.systrace;
    FN func_cd = ((INT)((UINT)code >> 8) | 0xFF000000);
    if (systrace != 0)
    {
        UW imask = disable_irq();
        UW clk = _kernel_micro_systim();
        UD para[3U] = { p1, p2, p3 };
        UH count = (UH)code & 0xffU;
        systrace(_kernel_systbl.ctxid_s, func_cd, clk, pc, count, para);
        restore_irq(imask);
    }

    ER ercd = ((FP_P3)func)(p1, p2, p3);

    FP_RETTRACE rettrace = (FP_RETTRACE)_kernel_systbl.rettrace;
    if (rettrace != 0)
    {
        UW imask = disable_irq();
        UW clk = _kernel_micro_systim();
        rettrace(_kernel_systbl.ctxid_s, func_cd, clk, pc, ercd);
        restore_irq(imask);
    }

    return ercd;
}

/*
 * System Call Hook Routine (4 parameter)
 */
ER _kernel_systrace_p4(FP func, INT code, UD p1, UD p2, UD p3, UD p4, ADDR pc)
{
    _kernel_save_pc(pc);
    FP_SYSTRACE systrace = (FP_SYSTRACE)_kernel_systbl.systrace;
    FN func_cd = ((INT)((UINT)code >> 8) | 0xFF000000);
    if (systrace != 0)
    {
        UW imask = disable_irq();
        UW clk = _kernel_micro_systim();
        UD para[4U] = { p1, p2, p3, p4 };
        UH count = (UH)code & 0xffU;
        systrace(_kernel_systbl.ctxid_s, func_cd, clk, pc, count, para);
        restore_irq(imask);
    }

    ER ercd = ((FP_P4)func)(p1, p2, p3, p4);

    FP_RETTRACE rettrace = (FP_RETTRACE)_kernel_systbl.rettrace;
    if (rettrace != 0)
    {
        UW imask = disable_irq();
        UW clk = _kernel_micro_systim();
        rettrace(_kernel_systbl.ctxid_s, func_cd, clk, pc, ercd);
        restore_irq(imask);
    }

    return ercd;
}

/*
 * System Call Hook Routine (5 parameter)
 */
ER _kernel_systrace_p5(FP func, INT code, UD p1, UD p2, UD p3, UD p4, UD p5, ADDR pc)
{
    _kernel_save_pc(pc);
    FP_SYSTRACE systrace = (FP_SYSTRACE)_kernel_systbl.systrace;
    FN func_cd = ((INT)((UINT)code >> 8) | 0xFF000000);
    if (systrace != 0)
    {
        UW imask = disable_irq();
        UW clk = _kernel_micro_systim();
        UD para[5U] = { p1, p2, p3, p4, p5 };
        UH count = (UH)code & 0xffU;
        systrace(_kernel_systbl.ctxid_s, func_cd, clk, pc, count, para);
        restore_irq(imask);
    }

    ER ercd = ((FP_P5)func)(p1, p2, p3, p4, p5);

    FP_RETTRACE rettrace = (FP_RETTRACE)_kernel_systbl.rettrace;
    if (rettrace != 0)
    {
        UW imask = disable_irq();
        UW clk = _kernel_micro_systim();
        rettrace(_kernel_systbl.ctxid_s, func_cd, clk, pc, ercd);
        restore_irq(imask);
    }

    return ercd;
}
