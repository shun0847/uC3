/**
 * @file    vect.S
 * @brief   Vector table for AArch64
 * @date    2024.11.15
 * @author  Copyright (c) 2024, eForce Co., Ltd. All rights reserved.
 *
 ****************************************************************************
 * @par     History
 *          - rev 1.0 (2020.05.15) yokota
 *            Inital version
 *          - rev 1.1 (2024.11.15)
 *            Change for uC3Std iMX8MPLUS A53 GCC
 ****************************************************************************
 */

        .global  _kernel_synhdr
        .global  _kernel_inthdr
        .global  _kernel_serhdr

        .balign  2048
        .section .vectors

/*
 * AArch64 vector table
 */

        .globl  _vector_table
        .type   _vector_table, @function
_vector_table:

        /**********************************************
         * EL1 with SP0 (used)
         *********************************************/
SyncHandler_0:
        b       _kernel_synhdr

        .balign  128
IrqHandler_0:
        b       _kernel_inthdr

        .balign  128
FiqHandler_0:
        b       FiqHandler_0

        .balign  128
SerrorHandler_0:
        b       _kernel_serhdr

        /**********************************************
         * EL1 with SP1 (if normal, not occured )
         *********************************************/
        .balign  128
SyncHandler_1:
        b       SyncHandler_1

        .balign  128
IrqHandler_1:
        b       IrqHandler_1

        .balign  128
FiqHandler_1:
        b       FiqHandler_1

        .balign  128
SerrorHandler_1:
        b       SerrorHandler_1

        /**********************************************
         * Lower EL using AArch64 (never occured)
         *********************************************/
        .balign  128
        b       UnknownHandler
        .balign  128
        b       UnknownHandler
        .balign  128
        b       UnknownHandler
        .balign  128
        b       UnknownHandler

        /**********************************************
         * Lower EL using AArch32 (never occured)
         *********************************************/
        .balign  128
        b       UnknownHandler
        .balign  128
        b       UnknownHandler
        .balign  128
        b       UnknownHandler
        .balign  128
        b       UnknownHandler

        .balign  2048
        .section .vectors0

/*
 * AArch64 vector table for CORE0 tranpoline
 */
        .globl  _vector_table0
        .type   _vector_table0, @function
_vector_table0:

        ;/**********************************************
        ; * Current EL with SP0 (used)
        ; *********************************************/
        b       UnknownHandler

        .balign  128
        b       UnknownHandler

        .balign  128
        b       UnknownHandler

        .balign  128
        b       UnknownHandler

        /**********************************************
         * Current EL with SP1 (if normal, not occured )
         *********************************************/
        .balign  128
        b       UnknownHandler
        .balign  128
        b       UnknownHandler
        .balign  128
        b       UnknownHandler
        .balign  128
        b       UnknownHandler

        /**********************************************
         * Lower EL using AArch64 (never occured)
         *********************************************/
        .balign  128
        b       ReturnToBootloader
        .balign  128
        b       UnknownHandler
        .balign  128
        b       UnknownHandler
        .balign  128
        b       UnknownHandler

        /**********************************************
         * Lower EL using AArch32 (never occured)
         *********************************************/
        .balign  128
        b       UnknownHandler
        .balign  128
        b       UnknownHandler
        .balign  128
        b       UnknownHandler
        .balign  128
        b       UnknownHandler

/*
 * Unknown Exception Handler (from lower EL)
 */
        .globl  UnknownHandler
        .type   UnknownHandler, @function
UnknownHandler:
        b     UnknownHandler

/*
 * HVC Call for trampoline (from lower EL)
 */
        .globl  ReturnToBootloader
        .type   ReturnToBootloader, @function
ReturnToBootloader:
        ldp     x29, x30, [sp], #16 
        ldp     x27, x28, [sp], #16 
        ldp     x25, x26, [sp], #16 
        ldp     x23, x24, [sp], #16 
        ldp     x21, x22, [sp], #16 
        ldp     x19, x20, [sp], #16 
        ldp     x17, x18, [sp], #16 
        ldp     x15, x16, [sp], #16 
        ldp     x13, x14, [sp], #16 
        ldp     x11, x12, [sp], #16 
        ldp     x9, x10, [sp], #16 
        ldp     x7, x8, [sp], #16 
        ldp     x5, x6, [sp], #16 
        ldp     x3, x4, [sp], #16 
        ldp     x1, x2, [sp], #16 
        ldp     x0, x30, [sp], #16 
        msr     vbar_el2, x0
        ret


