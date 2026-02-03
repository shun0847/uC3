/**
 * @file    uC3cpu.h
 * @brief   Micro C Cube Standard, KERNEL
 *          AArch64 dependent internal definitions
 * @date    2016.09.21
 * @author  Copyright (c) 2016, eForce Co., Ltd. All rights reserved.
 *
 ****************************************************************************
 * @par     History
 *          - rev 1.0 (2016.09.21) y-kim
 *            Created based on uC3cpu.h of ARMv7-A.
 ****************************************************************************
 */
#ifndef _UC3CPU_H_
#define _UC3CPU_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct t_intpara {
    UW      *savedt;
    UW      intinfo;
    FP      intfunc;
    T_ISR   *next;
} T_INTPARA;

typedef struct t_vinftbl {
    UW      intinfo;
    FP      intfunc;
    T_ISR   *next;
    T_ISR   *prev;
} T_VINFTBL;

extern IMASK _kernel_get_imask(void);
extern IMASK _kernel_set_imask(IMASK imask);
extern void _kernel_start_multi_task(void);

extern T_VINFTBL * _kernel_vinftbl_begin(void);
extern T_VINFTBL * _kernel_vinftbl_end(void);
extern SIZE _kernel_vinftbl_length(void);
extern void _kernel_save_pc(UD pc);

#ifdef __cplusplus
}
#endif

#endif
