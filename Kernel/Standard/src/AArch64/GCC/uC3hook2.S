/**
 * @file    uC3hook2.c
 * @brief   Micro C Cube Standard, KERNEL
 *          Hook Routine Entry
 * @date    2021.09.09
 * @author  Copyright (c) 2016, eForce Co., Ltd. All rights reserved.
 *
 ******************************************************************************
 * @par     History
 *          - rev 1.0 (2016.09.21) y-kim
 *            Initial version.
 *          - rev 1.1 (2021.09.10) yokota
 *            add isb after msr instructions.
 ******************************************************************************
 */

/* DAIF bit definition of PSTATE */

#define I_BIT   0x2

        .globl  _kernel_systrace_p0
        .globl  _kernel_systrace_p1
        .globl  _kernel_systrace_p2
        .globl  _kernel_systrace_p3
        .globl  _kernel_systrace_p4
        .globl  _kernel_systrace_p5

/*
 * UW _kernel_systrace_disable_irq(void);
 */
        .text
        .balign 4
        .globl  _kernel_systrace_disable_irq
        .type   _kernel_systrace_disable_irq, @function
_kernel_systrace_disable_irq:
        mrs     x0,  daif
        msr     daifset, #I_BIT
        isb
        ret

/*
 * void _kernel_systrace_restore_irq(UW imask);
 */
        .globl  _kernel_systrace_restore_irq
        .type   _kernel_systrace_restore_irq, @function
_kernel_systrace_restore_irq:
        msr     daif, x0
        isb
        ret

/*
 * ER _kernel_systrace_p0_entry(FP func, UW code);
 */
        .globl  _kernel_systrace_p0_entry
        .type   _kernel_systrace_p0_entry, @function
_kernel_systrace_p0_entry:
        mov     x2,  x30
        b       _kernel_systrace_p0

/*
 * ER _kernel_systrace_p1_entry(FP func, UW code, UD p1);
 */
        .globl  _kernel_systrace_p1_entry
        .type   _kernel_systrace_p1_entry, @function
_kernel_systrace_p1_entry:
        mov     x3,  x30
        b       _kernel_systrace_p1

/*
 * ER _kernel_systrace_p2_entry(FP func, UW code, UD p1, UD p2);
 */
        .globl  _kernel_systrace_p2_entry
        .type   _kernel_systrace_p2_entry, @function
_kernel_systrace_p2_entry:
        mov     x4,  x30
        b       _kernel_systrace_p2

/*
 * ER _kernel_systrace_p3_entry(FP func, UW code, UD p1, UD p2, UD p3);
 */
        .globl  _kernel_systrace_p3_entry
        .type   _kernel_systrace_p3_entry, @function
_kernel_systrace_p3_entry:
        mov     x5,  x30
        b       _kernel_systrace_p3

/*
 * ER _kernel_systrace_p4_entry(FP func, UW code, UD p1, UD p2, UD p3, UD p4);
 */
        .globl  _kernel_systrace_p4_entry
        .type   _kernel_systrace_p4_entry, @function
_kernel_systrace_p4_entry:
        mov     x6,  x30
        b       _kernel_systrace_p4

/*
 * ER _kernel_systrace_p5_entry(FP func, UW code, UD p1, UD p2, UD p3, UD p4, UD p5);
 */
        .globl  _kernel_systrace_p5_entry
        .type   _kernel_systrace_p5_entry, @function
_kernel_systrace_p5_entry:
        mov     x7,  x30
        b       _kernel_systrace_p5
